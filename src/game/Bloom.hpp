// Bloom: the settings of the glow around bright things, the size of its render targets
// and the weights of its blur.
// See docs/modules/renderer/post-process.md
#pragma once

#include <array>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. The passes themselves are drawn by game::PostProcess.

/// The bloom targets are this many times smaller than the scene framebuffer in each
/// direction: half the width and half the height, a quarter of the pixels. A blur costs
/// a quarter as much there, and the same kernel reaches twice as far on the screen.
/// The blur hides the lower resolution.
constexpr int BLOOM_DOWNSCALE = 2;

/// How many pixels the blur reads on EACH side of the pixel it writes. With the pixel
/// itself that is 2 * 6 + 1 = 13 texture reads per pass. The same number is in
/// post/blur.frag (BLUR_RADIUS): the two must agree.
constexpr int BLOOM_BLUR_RADIUS = 6;

/// The number of different weights of the blur: one for the pixel itself and one for
/// each distance up to the radius. The kernel is symmetric, so the weight for distance
/// 2 is used for the pixel 2 to the left and for the pixel 2 to the right.
constexpr int BLOOM_BLUR_WEIGHT_COUNT = BLOOM_BLUR_RADIUS + 1;

/// The width of the Gaussian bell the weights are taken from (its standard deviation),
/// in pixels of the bloom target. Half of the radius: the bell has then fallen to 14 %
/// of its height at the last pixel that is read, so little of it is cut off.
constexpr float BLOOM_BLUR_SIGMA = 3.0F;

/// Range of BloomSettings::blurIterations. One iteration is a horizontal and a vertical
/// pass. Each one more makes the glow wider and costs two more passes.
constexpr int MIN_BLOOM_BLUR_ITERATIONS = 1;
constexpr int MAX_BLOOM_BLUR_ITERATIONS = 10;

/// What can be changed about the bloom while the game runs. The debug UI edits the
/// fields. The defaults are part of the look of the night, tuned together with the glow
/// of the crystals (CRYSTAL_GLOW_STRENGTH) and the brightness of the sky.
struct BloomSettings {
    /// Whether the bloom passes are drawn and their result is added to the picture.
    /// Switched off, the frame is exactly the one without bloom.
    bool enabled = true;

    /// Only light brighter than this takes part: a brightness (luminance) in the linear
    /// HDR values of the scene framebuffer, where 1 is the white of the screen before
    /// exposure. The bright pass keeps what a pixel has ABOVE the threshold, so a pixel
    /// just over it glows faintly and nothing switches on at once. The default is below
    /// the glowing crystals (at every moment of their pulse), the moon disc and the
    /// brightest stars, and above the stone walls in the beam of the flashlight.
    float threshold = 0.8F;

    /// The blurred glow is multiplied by this number before it is added to the scene.
    /// 0 adds nothing.
    float intensity = 1.0F;

    /// How many times the blur runs (horizontal pass, then vertical pass). Repeating
    /// a Gaussian blur gives a wider Gaussian blur: after n iterations the glow is as
    /// wide as one blur with sigma BLOOM_BLUR_SIGMA * sqrt(n). With the default that is
    /// about 7 pixels of the bloom target, so about 15 pixels of a scene twice as large.
    int blurIterations = 6;
};

/// The size of a bloom target in one direction for a scene framebuffer of sceneExtent
/// pixels in that direction: sceneExtent / BLOOM_DOWNSCALE as an integer division (the
/// rest is dropped), but at least 1, because a texture of size 0 cannot be drawn into.
/// A scene of 1281 pixels gets 640, a scene of 1 pixel gets 1.
int bloomTargetExtent(int sceneExtent);

/// The weights of the separable Gaussian blur. Element 0 is the weight of the pixel
/// itself, element d the weight of each of the two pixels d steps away.
///
/// They are the Gaussian function exp(-d * d / (2 * sigma * sigma)) at d = 0, 1, 2, ...
/// with sigma = BLOOM_BLUR_SIGMA, divided by their sum over the whole kernel (the centre
/// once, every other distance twice). The division makes the whole kernel add up to 1:
/// a blur then moves light around without adding or losing any, however often it runs.
std::array<float, BLOOM_BLUR_WEIGHT_COUNT> bloomBlurWeights();

} // namespace game
