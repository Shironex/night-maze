// Colour space conversion shared by the shaders: between sRGB encoded colours (what
// a picture file, a colour picker and the screen use) and linear colours (what lighting
// math needs). This file is not a shader of its own. It has no #version line: the shader
// loader puts its text in place of the #include line (gfx/ShaderSource.hpp).
// See docs/modules/renderer/post-process.md

// The scene is drawn into a floating point buffer that holds LINEAR colours, and the
// last pass of the frame (post/composite.frag) encodes them to sRGB for the screen.
// Both functions are the exact formulas of the sRGB standard, the same ones as
// gfx::srgbToLinear and gfx::linearToSrgb in C++ and the one the graphics card uses when
// it reads a GL_SRGB8 texture: a straight line for the darkest values, a power curve
// with the exponent 2.4 above them.

// The numbers of the sRGB standard, under the same names as in src/gfx/ColorSpace.cpp.
const float SRGB_LINEAR_SEGMENT_SLOPE = 12.92; // slope of the straight line
const float SRGB_ENCODED_THRESHOLD = 0.04045;  // encoded values up to here are on the line
const float SRGB_LINEAR_THRESHOLD = 0.0031308; // the same point as a linear value
const float SRGB_CURVE_OFFSET = 0.055;
const float SRGB_CURVE_EXPONENT = 2.4;

// An sRGB encoded colour as a linear colour. For colours that are written into a shader
// as numbers chosen by eye on a screen, and for data shown as a colour (the debug
// views): after the encoding at the end of the frame the screen shows the very numbers
// that went in here.
vec3 srgbToLinear(vec3 encoded) {
    vec3 value = clamp(encoded, 0.0, 1.0);
    vec3 line = value / SRGB_LINEAR_SEGMENT_SLOPE;
    vec3 curve = pow((value + SRGB_CURVE_OFFSET) / (1.0 + SRGB_CURVE_OFFSET),
                     vec3(SRGB_CURVE_EXPONENT));
    // step(edge, x) is 0 where x < edge and 1 elsewhere, per channel: it picks the line
    // for the dark values and the curve for the rest without an if per channel.
    return mix(line, curve, step(SRGB_ENCODED_THRESHOLD, value));
}

// A linear colour as an sRGB encoded colour: the inverse of srgbToLinear. Values outside
// 0 to 1 are clamped: the screen cannot show them.
vec3 linearToSrgb(vec3 linear) {
    vec3 value = clamp(linear, 0.0, 1.0);
    vec3 line = value * SRGB_LINEAR_SEGMENT_SLOPE;
    vec3 curve = (1.0 + SRGB_CURVE_OFFSET) * pow(value, vec3(1.0 / SRGB_CURVE_EXPONENT)) -
                 SRGB_CURVE_OFFSET;
    return mix(line, curve, step(SRGB_LINEAR_THRESHOLD, value));
}
