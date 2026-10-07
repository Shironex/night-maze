// Fog: the settings of the ground fog and the formulas it is computed with.
// See docs/modules/renderer/post-process.md
#pragma once

#include <glm/glm.hpp>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. The fog itself is computed for every pixel by the composite pass
// (post/composite.frag), which has the same three formulas under the same names: the two
// files must agree. The pass is drawn by game::PostProcess.

/// What can be changed about the fog while the game runs. The debug UI edits the fields.
/// The defaults are part of the look of the night: a thin mist that lies in the
/// corridors of the maze and leaves the sky clear.
///
/// The fog is exponential fog with a height factor:
///
///     heightFactor = exp(-heightFalloff * max(height - baseHeight, 0))
///     amount       = 1 - exp(-density * heightFactor * distance)
///     colour       = mix(colour of the scene, colour of the fog, amount)
///
/// distance is the length of the straight line from the eye to the surface a pixel
/// shows, and height is the world height (y) of that surface.
struct FogSettings {
    /// Whether the composite pass adds the fog. Switched off, the frame is exactly the
    /// one without fog: the depth texture is not even read.
    bool enabled = true;

    /// How thick the fog is at and below baseHeight, per metre. After 1 / density
    /// metres 63 % of what lies behind the fog is replaced by the colour of the fog
    /// (1 - 1 / e), after ln(2) / density metres half of it. With the default that is
    /// about 9 m and about 6.3 m, three cells of the maze. It is not thicker than that
    /// on purpose: the ground four cells ahead must still show (a test checks it).
    float density = 0.11F;

    /// Up to this world height (y, in metres) the fog has its full density. The ground
    /// of the maze lies between 0 and about 0.6 m at height scale 1 (game::MAZE_RELIEF),
    /// so the default keeps most of the floor in the thickest fog. The number is fixed:
    /// it does not follow the height scale of the terrain.
    float baseHeight = 0.5F;

    /// How fast the fog thins out above baseHeight, per metre: the density is halved
    /// every ln(2) / heightFalloff metres. With the default that is about 1.7 m, so the
    /// tops of the walls (3 m, which is 2.5 m above the base) stand in fog of about
    /// a third of the density (exp(-1) = 0.37), and the hills and the sky are almost
    /// clear. 0 gives fog of the same density everywhere.
    float heightFalloff = 0.4F;

    /// The colour a surface fades into: a dark grey with a little blue in it, so far
    /// walls sink into the night and not into a pale mist. An sRGB value, as a colour picker
    /// shows it: game::PostProcess converts it to a linear colour for the shader. It is
    /// mixed into the HDR picture before exposure and tone mapping. The Reinhard and
    /// ACES curves press dark tones down, so with them the fog on the screen is darker
    /// than this colour looks in the picker. With None (clamp) at exposure 1 the two
    /// match.
    glm::vec3 color{0.09F, 0.105F, 0.13F};
};

/// How much of the full density the fog has at a world height: 1 at and below
/// baseHeight, and above it exp(-heightFalloff * (height - baseHeight)), which falls
/// towards 0. The same formula as fogHeightFactor in post/composite.frag.
float fogHeightFactor(float height, float baseHeight, float heightFalloff);

/// How much of a surface is replaced by the colour of the fog, from 0 (none) to 1 (all
/// of it): 1 - exp(-density * heightFactor * distance). distance is in metres, and
/// heightFactor is the result of fogHeightFactor. The same formula as fogAmount in
/// post/composite.frag.
///
/// This is the law of exponential fog: every metre of fog lets the same share of the
/// light through, so after distance metres exp(-density * distance) of it is left.
float fogAmount(float density, float heightFactor, float distance);

/// The two functions above together, for a surface at the world position point seen
/// from eye: what the composite pass computes for one pixel.
///
/// KNOWN LIMIT: the height factor is taken at the surface only, not along the whole
/// line of sight. Seen from high above, ground in the fog gets as much fog as if the
/// whole way down led through it, which is too much. Seen from inside the fog,
/// something tall gets too little: the line to its top starts in thick fog. An exact
/// answer needs the density summed up along the line (an integral). This shortcut keeps
/// what matters for the picture: the fog lies low, and the sky stays clear.
float fogAmountAt(const FogSettings& settings, const glm::vec3& eye, const glm::vec3& point);

/// The world position of the surface a pixel shows, from its place on the screen and
/// the value of the depth texture there. The same steps as worldPositionFromDepth in
/// post/composite.frag:
///
///   1. uv (0 to 1 across the screen) and depth (0 to 1) are brought to normalised
///      device coordinates, where all three run from -1 to 1: ndc = value * 2 - 1.
///   2. inverseViewProjection, the inverse of projection * view, takes that point back
///      towards world space. The result has four components.
///   3. Its first three components are divided by the fourth one (w).
///
/// Why step 3: on the way to the screen the graphics card divides the clip space
/// position by its w (the perspective division). The inverse matrix cannot undo
/// a division, so its result is the world position divided by that same w, and its
/// fourth component is 1 divided by that w. Dividing by it gives the position back.
///
/// A pixel of the sky has depth 1 and gives a point on the far clipping plane, in the
/// direction the pixel is seen in.
glm::vec3 worldPositionFromDepth(const glm::vec2& uv, float depth,
                                 const glm::mat4& inverseViewProjection);

} // namespace game
