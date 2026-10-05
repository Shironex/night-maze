// ColorSpace: whether colour numbers are sRGB encoded or linear, and the conversion.
// See docs/modules/gfx/color-space.md
#include "gfx/ColorSpace.hpp"

#include <algorithm>
#include <cmath>

namespace gfx {

namespace {

// The numbers of the sRGB standard (IEC 61966-2-1).
//
// Below the threshold the curve is a straight line through zero with this slope. A pure
// power curve would be infinitely steep at zero, which cannot be inverted well.
constexpr float LINEAR_SEGMENT_SLOPE = 12.92F;
// Encoded values up to this one lie on the straight line.
constexpr float ENCODED_THRESHOLD = 0.04045F;
// The same point of the curve as a linear value: ENCODED_THRESHOLD / LINEAR_SEGMENT_SLOPE.
constexpr float LINEAR_THRESHOLD = 0.0031308F;
// Above the threshold: ((encoded + offset) / (1 + offset)) to the power of the exponent.
constexpr float CURVE_OFFSET = 0.055F;
constexpr float CURVE_EXPONENT = 2.4F;

} // namespace

float srgbToLinear(float encoded) {
    const float value = std::clamp(encoded, 0.0F, 1.0F);
    if (value <= ENCODED_THRESHOLD) {
        return value / LINEAR_SEGMENT_SLOPE;
    }
    return std::pow((value + CURVE_OFFSET) / (1.0F + CURVE_OFFSET), CURVE_EXPONENT);
}

float linearToSrgb(float linear) {
    const float value = std::clamp(linear, 0.0F, 1.0F);
    if (value <= LINEAR_THRESHOLD) {
        return value * LINEAR_SEGMENT_SLOPE;
    }
    return (1.0F + CURVE_OFFSET) * std::pow(value, 1.0F / CURVE_EXPONENT) - CURVE_OFFSET;
}

glm::vec3 srgbToLinear(const glm::vec3& encoded) {
    return {srgbToLinear(encoded.r), srgbToLinear(encoded.g), srgbToLinear(encoded.b)};
}

glm::vec3 linearToSrgb(const glm::vec3& linear) {
    return {linearToSrgb(linear.r), linearToSrgb(linear.g), linearToSrgb(linear.b)};
}

} // namespace gfx
