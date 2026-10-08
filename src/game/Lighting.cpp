// Lighting of the game: the settings of the moon, the flashlight and the point lights.
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

std::vector<PointLightSpot> nearestPointLights(std::span<const glm::vec3> positions,
                                               const glm::vec3& eye, int maxCount) {
    // Every position becomes a spot of full strength with the look of the settings: the
    // light of a crystal. The choice itself is the one for spots.
    std::vector<PointLightSpot> lights;
    lights.reserve(positions.size());
    for (const glm::vec3& position : positions) {
        lights.push_back({.position = position});
    }
    return nearestPointLightSpots(lights, eye, maxCount);
}

std::vector<PointLightSpot> nearestPointLightSpots(std::span<const PointLightSpot> lights,
                                                   const glm::vec3& eye, int maxCount) {
    if (maxCount < 1) {
        return {};
    }

    // Every light with its distance from the eye, sorted from the nearest to the
    // farthest. stable_sort keeps two lights at the same distance in the order they came
    // in, so the same input always gives the same result.
    struct Candidate {
        PointLightSpot light;
        float distance;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(lights.size());
    for (const PointLightSpot& light : lights) {
        candidates.push_back({.light = light, .distance = glm::distance(light.position, eye)});
    }
    std::ranges::stable_sort(
        candidates, [](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });

    // The edge of the set: the distance of the first light that is left out. With no
    // light left out there is no edge, and every light keeps its full strength.
    const auto count = static_cast<std::size_t>(maxCount);
    const bool someLeftOut = candidates.size() > count;
    const float edge = someLeftOut ? candidates[count].distance : 0.0F;

    std::vector<PointLightSpot> chosen;
    chosen.reserve(std::min(candidates.size(), count));
    for (std::size_t i = 0; i < candidates.size() && i < count; ++i) {
        float fade = 1.0F;
        if (someLeftOut) {
            // 0 at the edge, 1 from POINT_LIGHT_FADE_DISTANCE inside of it.
            fade =
                std::clamp((edge - candidates[i].distance) / POINT_LIGHT_FADE_DISTANCE, 0.0F, 1.0F);
        }
        // The spot as it came in. Only its strength changes: a light that was already
        // dim (the ember of the gate) fades from there.
        PointLightSpot light = candidates[i].light;
        light.strength *= fade;
        chosen.push_back(light);
    }
    return chosen;
}

scene::LightSet buildLightSet(const LightingSettings& settings, const FlashlightPose& flashlight,
                              std::span<const PointLightSpot> pointLights) {
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

    // The lights of the crystals share one colour and one radius. The place differs, and
    // the intensity where a light is fading out of the set (nearestPointLights). A spot
    // with a look of its own (the lamp of the gate) brings its three numbers with it.
    const scene::Attenuation pointAttenuation = scene::attenuationForRadius(settings.pointRadius);
    const glm::vec3 pointColor = gfx::srgbToLinear(settings.pointColor);
    const std::size_t pointCount =
        std::min(pointLights.size(), static_cast<std::size_t>(scene::MAX_POINT_LIGHTS));
    for (std::size_t i = 0; i < pointCount; ++i) {
        const PointLightSpot& spot = pointLights[i];
        if (spot.ownLook) {
            lights.points[i] = {
                .position = spot.position,
                .color = gfx::srgbToLinear(spot.color),
                .intensity = spot.intensity * spot.strength,
                .attenuation = scene::attenuationForRadius(spot.radius),
            };
            continue;
        }
        lights.points[i] = {
            .position = spot.position,
            .color = pointColor,
            .intensity = settings.pointIntensity * spot.strength,
            .attenuation = pointAttenuation,
        };
    }
    lights.pointCount = static_cast<int>(pointCount);
    return lights;
}

} // namespace game
