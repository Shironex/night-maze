// Lighting of the game: the settings of the moon, the flashlight and the point lights.
// See docs/modules/game/flashlight.md
#include "game/Lighting.hpp"

#include "gfx/ColorSpace.hpp"

#include <algorithm>
#include <cstddef>

namespace game {

SpecularModel specularModelOf(LightingMode mode) {
    return mode == LightingMode::BlinnPhong ? SpecularModel::BlinnPhong : SpecularModel::Phong;
}

bool usesNormalMap(const LightingSettings& settings) {
    return settings.normalMapping && settings.mode != LightingMode::Gouraud;
}

glm::vec3 moonDirection(const LightingSettings& settings) {
    return scene::directionFromAngles(settings.moonYawDegrees, settings.moonPitchDegrees);
}

scene::LightSet buildLightSet(const LightingSettings& settings, const glm::vec3& eye,
                              const glm::vec3& viewDirection,
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

    // The flashlight is held at the eye and points where the player looks.
    lights.spot = {
        .position = eye,
        .direction = viewDirection,
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
