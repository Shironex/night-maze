// Fog: the settings of the ground fog and the formulas it is computed with.
#include "game/Fog.hpp"

#include <algorithm>
#include <cmath>

namespace game {

float fogHeightFactor(float height, float baseHeight, float heightFalloff) {
    // Below the base the difference is negative: std::max makes it 0, and exp(0) is 1.
    const float heightAboveBase = std::max(height - baseHeight, 0.0F);
    return std::exp(-heightFalloff * heightAboveBase);
}

float fogAmount(float density, float heightFactor, float distance) {
    // exp(-x) is the share of the light that gets through. The fog takes the rest.
    return 1.0F - std::exp(-density * heightFactor * distance);
}

float fogAmountAt(const FogSettings& settings, const glm::vec3& eye, const glm::vec3& point) {
    const float heightFactor =
        fogHeightFactor(point.y, settings.baseHeight, settings.heightFalloff);
    return fogAmount(settings.density, heightFactor, glm::length(point - eye));
}

glm::vec3 worldPositionFromDepth(const glm::vec2& uv, float depth,
                                 const glm::mat4& inverseViewProjection) {
    // Step 1: from 0..1 to -1..1. The fourth component 1 makes it a point.
    const glm::vec4 ndc{glm::vec3{uv, depth} * 2.0F - 1.0F, 1.0F};
    // Step 2: back through the projection and the view.
    const glm::vec4 world = inverseViewProjection * ndc;
    // Step 3: undo the perspective division.
    return glm::vec3{world} / world.w;
}

} // namespace game
