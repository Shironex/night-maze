// Vignette: the settings of the darkening towards the corners of the screen and its
// formula.
// See docs/modules/renderer/post-process.md
#include "game/Vignette.hpp"

namespace game {

float vignetteFactor(const glm::vec2& uv, float strength, float radius) {
    const float distance = glm::length(uv - SCREEN_CENTER);
    // smoothstep is 0 up to the radius, 1 from the corner distance on, and in between
    // an S shaped curve (3t^2 - 2t^3) without a visible start or end.
    const float darkening = glm::smoothstep(radius, VIGNETTE_CORNER_DISTANCE, distance);
    return 1.0F - strength * darkening;
}

} // namespace game
