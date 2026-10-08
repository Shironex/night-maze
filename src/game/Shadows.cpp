// Shadows: the settings of a shadow map, and the math around it that needs no OpenGL.
#include "game/Shadows.hpp"

#include "game/MazeLayout.hpp"
#include "game/Terrain.hpp"
#include "scene/LightSpace.hpp"

#include <algorithm>

namespace game {

int shadowMapSize(ShadowResolution resolution) {
    return resolution == ShadowResolution::Low ? SHADOW_MAP_SIZE_LOW : SHADOW_MAP_SIZE_HIGH;
}

int pcfKernelSide(int radius) {
    // The middle texel and radius texels on each side of it.
    return 2 * radius + 1;
}

ShadowSettings flashlightShadowDefaults() {
    ShadowSettings settings;
    settings.resolution = ShadowResolution::Low;
    settings.constantBias = FLASHLIGHT_SHADOW_CONSTANT_BIAS;
    settings.slopeBias = FLASHLIGHT_SHADOW_SLOPE_BIAS;
    return settings;
}

scene::Aabb shadowCasterBounds(const Terrain& terrain) {
    // The gatehouse of the exit is the tallest thing that stands on the ground. One on
    // the highest point of the land is higher than anything really is: the maze lies in
    // the low middle. That costs a little depth range and keeps the rule simple.
    return {
        .min = {terrain.minX(), terrain.minHeight(), terrain.minZ()},
        .max = {terrain.maxX(), terrain.maxHeight() + GATE_HOUSE_HEIGHT, terrain.maxZ()},
    };
}

float shadowBias(float constantBias, float slopeBias, float facing) {
    return constantBias + slopeBias * (1.0F - std::clamp(facing, 0.0F, 1.0F));
}

float biasInDepthUnits(float biasMetres, float depthRange) {
    if (depthRange <= 0.0F) {
        return 0.0F;
    }
    return biasMetres / depthRange;
}

float biasForShader(float biasMetres, const scene::LightSpace& lightSpace) {
    if (lightSpace.kind == scene::LightProjection::Perspective) {
        // Metres, as they are: the shader moves the fragment towards the light.
        return biasMetres;
    }
    // The depth range of the box of the light: its z extent in metres is the stored
    // range from 0 to 1.
    return biasInDepthUnits(biasMetres, lightSpace.extent.z);
}

float shadowTexelSize(const scene::LightSpace& lightSpace, int mapSize) {
    if (mapSize < 1) {
        return 0.0F;
    }
    return std::max(lightSpace.extent.x, lightSpace.extent.y) / static_cast<float>(mapSize);
}

float shadowTexelSizeAt(const scene::LightSpace& lightSpace, int mapSize, float distance) {
    const float atFarPlane = shadowTexelSize(lightSpace, mapSize);
    if (lightSpace.kind != scene::LightProjection::Perspective || lightSpace.farPlane <= 0.0F) {
        // A box: the same size at every distance.
        return atFarPlane;
    }
    // The extent of a perspective light space is measured at its far plane. The sides
    // of the pyramid are straight lines through the light, so the size at another
    // distance is that size times distance / far plane.
    return atFarPlane * std::max(distance, 0.0F) / lightSpace.farPlane;
}

int pcfRadiusInUse(const ShadowSettings& settings) {
    if (!settings.pcf) {
        return 0;
    }
    return std::clamp(settings.pcfRadius, MIN_PCF_RADIUS, MAX_PCF_RADIUS);
}

} // namespace game
