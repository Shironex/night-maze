// Shadows: the settings of a shadow map, and the math around it that needs no OpenGL.
// See docs/modules/renderer/shadows.md
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

scene::Aabb shadowCasterBounds(const Terrain& terrain) {
    // The pillars are the tallest things that stand on the ground. One of them on the
    // highest point of the land is higher than anything really is: the maze lies in
    // the low middle. That costs a little depth range and keeps the rule simple.
    return {
        .min = {terrain.minX(), terrain.minHeight(), terrain.minZ()},
        .max = {terrain.maxX(), terrain.maxHeight() + PILLAR_HEIGHT, terrain.maxZ()},
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

float shadowTexelSize(const scene::LightSpace& lightSpace, int mapSize) {
    if (mapSize < 1) {
        return 0.0F;
    }
    return std::max(lightSpace.extent.x, lightSpace.extent.y) / static_cast<float>(mapSize);
}

int pcfRadiusInUse(const ShadowSettings& settings) {
    if (!settings.pcf) {
        return 0;
    }
    return std::clamp(settings.pcfRadius, MIN_PCF_RADIUS, MAX_PCF_RADIUS);
}

} // namespace game
