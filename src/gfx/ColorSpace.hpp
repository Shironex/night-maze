// ColorSpace: whether colour numbers are sRGB encoded or linear, and the conversion.
// See docs/modules/renderer/post-process.md
#pragma once

#include <glm/glm.hpp>

namespace gfx {

// Plain math without OpenGL, so tests can use it.

/// What the numbers of a colour mean.
///
/// A picture file and a colour picked on the screen store sRGB values: numbers bent by
/// a curve so that the 256 steps of a byte are spread the way the eye sees brightness
/// (more steps in the dark tones). Lighting math (multiplying by a light, adding lights)
/// is only correct on linear values, which are proportional to the amount of light.
enum class ColorSpace {
    /// sRGB encoded: colour pictures (the albedo of walls, ground, gate and crystals, the
    /// sky). The graphics card decodes them to linear values when a shader reads them.
    Srgb,
    /// Already linear, or not a colour at all: normal maps hold directions. The numbers
    /// reach the shader exactly as they are stored.
    Linear,
};

/// One sRGB encoded channel (0 to 1) as a linear value (0 to 1).
///
/// This is the exact function of the sRGB standard, in two pieces: a straight line for
/// the darkest values and a power curve with the exponent 2.4 above them. The shorter
/// pow(value, 2.2) is only close to it. The exact one is used because the graphics card
/// decodes GL_SRGB8 textures with it too (OpenGL 4.1, section 3.8.17): a colour typed
/// in and the same colour read from a texture then give the same linear value.
/// Values outside 0 to 1 are clamped first.
float srgbToLinear(float encoded);

/// One linear channel (0 to 1) as an sRGB encoded value (0 to 1): the inverse of
/// srgbToLinear. The same formula is in assets/shaders/common/color.glsl, where the
/// last pass of a frame applies it. Values outside 0 to 1 are clamped first.
float linearToSrgb(float linear);

/// srgbToLinear for the three channels of a colour.
glm::vec3 srgbToLinear(const glm::vec3& encoded);

/// linearToSrgb for the three channels of a colour.
glm::vec3 linearToSrgb(const glm::vec3& linear);

} // namespace gfx
