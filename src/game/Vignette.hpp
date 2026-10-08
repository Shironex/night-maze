// Vignette: the settings of the darkening towards the corners of the screen and its
// formula.
#pragma once

#include <glm/glm.hpp>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. The vignette itself is computed for every pixel by the composite pass
// (post/composite.frag), which has the same formula under the same name: the two files
// must agree.

/// The middle of the screen as a texture coordinate.
constexpr glm::vec2 SCREEN_CENTER{0.5F, 0.5F};

/// The distance from the middle of the screen to a corner, in texture coordinates:
/// the length of (0.5, 0.5), which is the square root of 0.5. The vignette is strongest
/// there. The same number is in post/composite.frag.
constexpr float VIGNETTE_CORNER_DISTANCE = 0.70710678F;

/// What can be changed about the vignette while the game runs. The debug UI edits the
/// fields. With the defaults the corners lose almost half of their light: the eye is
/// led to the middle of the picture, where the flashlight shines, and the edges of the
/// view close in like the dark does.
///
/// The vignette multiplies the finished picture (after tone mapping) by
///
///     factor = 1 - strength * smoothstep(radius, VIGNETTE_CORNER_DISTANCE, distance)
///
/// distance is measured from the middle of the screen in texture coordinates, which
/// run from 0 to 1 in BOTH directions whatever shape the window has. So the vignette is
/// NOT corrected for the aspect ratio: in a wide window the bright middle is an ellipse
/// as wide as the window, not a circle, and all four corners are always equally dark.
struct VignetteSettings {
    /// Whether the composite pass darkens the corners. Switched off, the frame is
    /// exactly the one without a vignette.
    bool enabled = true;

    /// How much of the light the corners lose: 0 nothing, 1 all of it (black corners).
    float strength = 0.45F;

    /// Where the darkening starts, as a distance from the middle of the screen in
    /// texture coordinates. 0.5 is the middle of an edge, so with the default the
    /// darkening begins a little inside the edges. A smaller radius gives a wider and
    /// softer dark border. It has to stay below VIGNETTE_CORNER_DISTANCE.
    float radius = 0.4F;
};

/// The number the colour of a pixel is multiplied by: 1 inside the radius, then falling
/// smoothly to 1 - strength in the corners. uv is the texture coordinate of the pixel,
/// (0, 0) in the bottom left corner of the screen and (1, 1) in the top right one. The
/// same formula as vignetteFactor in post/composite.frag.
float vignetteFactor(const glm::vec2& uv, float strength, float radius);

} // namespace game
