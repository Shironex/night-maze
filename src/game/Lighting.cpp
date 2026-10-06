// Lighting of the game: the settings of the moon, the flashlight and the point lights.
// See docs/modules/game/flashlight.md
#include "game/Lighting.hpp"

#include "gfx/ColorSpace.hpp"

#include <algorithm>
#include <cstddef>

namespace game {

namespace {

// The up direction of the world. The hand holds the flashlight straight below the eye.
constexpr glm::vec3 WORLD_UP{0.0F, 1.0F, 0.0F};

// A vector shorter than this cannot be brought to length 1 (see scene/LightBlock.cpp).
constexpr float MIN_DIRECTION_LENGTH = 0.0001F;

} // namespace

SpecularModel specularModelOf(LightingMode mode) {
    return mode == LightingMode::BlinnPhong ? SpecularModel::BlinnPhong : SpecularModel::Phong;
}

bool usesNormalMap(const LightingSettings& settings) {
    return settings.normalMapping && settings.mode != LightingMode::Gouraud;
}

glm::vec3 moonDirection(const LightingSettings& settings) {
    return scene::directionFromAngles(settings.moonYawDegrees, settings.moonPitchDegrees);
}

FlashlightPose flashlightPose(const LightingSettings& settings, const glm::vec3& eye,
                              const glm::vec3& forward, const glm::vec3& right) {
    // The hand: to the right of the eye and below it. The right vector of the camera is
    // level and "below" is straight down, so the first offset is the whole horizontal
    // distance from the eye and the second the whole vertical one.
    const glm::vec3 position =
        eye + right * settings.flashlightHandRight - WORLD_UP * settings.flashlightHandDown;

    // The point on the line of view the beam is aimed at.
    const float convergeDistance =
        std::max(settings.flashlightConvergeDistance, MIN_FLASHLIGHT_CONVERGE_DISTANCE);
    const glm::vec3 target = eye + forward * convergeDistance;

    // From the hand to that point. The two fall together in one case only: a camera that
    // looks straight down and a hand exactly as far below the eye as the point is in
    // front of it. A vector of length 0 has no direction, so the beam then simply
    // points where the camera looks.
    const glm::vec3 toTarget = target - position;
    if (glm::length(toTarget) < MIN_DIRECTION_LENGTH) {
        return {.position = position, .direction = forward};
    }
    return {.position = position, .direction = glm::normalize(toTarget)};
}

scene::LightSet buildLightSet(const LightingSettings& settings, const FlashlightPose& flashlight,
                              std::span<const glm::vec3> pointPositions) {
    // The colours of the settings are sRGB values: they are picked on the screen. The
    // shaders compute with linear light, so this function is the one place where the
    // four colours are converted. The intensities are plain factors and stay as they
    // are.
    scene::LightSet lights;
    lights.ambient = gfx::srgbToLinear(settings.ambient);

    lights.directional = {
        .direction = moonDirection(settings),
        .color = gfx::srgbToLinear(settings.moonColor),
        .intensity = settings.moonIntensity,
    };

    // The flashlight is held in the hand and aimed at a point in front of the eye: the
    // caller has computed both (flashlightPose), once for the light and its shadow map.
    lights.spot = {
        .position = flashlight.position,
        .direction = flashlight.direction,
        .color = gfx::srgbToLinear(settings.flashlightColor),
        .intensity = settings.flashlightIntensity,
        .attenuation = scene::attenuationForRadius(settings.flashlightRange),
        // A cone cannot be wider inside than outside. The panel keeps the two angles in
        // order, this line keeps them in order whoever sets them.
        .innerConeDegrees =
            std::min(settings.flashlightInnerDegrees, settings.flashlightOuterDegrees),
        .outerConeDegrees = settings.flashlightOuterDegrees,
    };
    lights.spotEnabled = settings.flashlightOn;

    // All point lights share one colour, one intensity and one radius. Only the place
    // differs.
    const scene::Attenuation pointAttenuation = scene::attenuationForRadius(settings.pointRadius);
    const glm::vec3 pointColor = gfx::srgbToLinear(settings.pointColor);
    const std::size_t pointCount =
        std::min(pointPositions.size(), static_cast<std::size_t>(scene::MAX_POINT_LIGHTS));
    for (std::size_t i = 0; i < pointCount; ++i) {
        lights.points[i] = {
            .position = pointPositions[i],
            .color = pointColor,
            .intensity = settings.pointIntensity,
            .attenuation = pointAttenuation,
        };
    }
    lights.pointCount = static_cast<int>(pointCount);
    return lights;
}

} // namespace game
