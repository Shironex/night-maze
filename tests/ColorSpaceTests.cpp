// Tests of gfx::srgbToLinear and gfx::linearToSrgb: the conversion between sRGB encoded
// and linear colours.
#include "gfx/ColorSpace.hpp"

#include <doctest/doctest.h>

#include <cmath>

namespace {

// A byte of a picture (0 to 255) as the number between 0 and 1 a shader sees.
constexpr float BYTE_MAX = 255.0F;

} // namespace

TEST_CASE("black and white are the same in both colour spaces") {
    CHECK(gfx::srgbToLinear(0.0F) == 0.0F);
    CHECK(gfx::srgbToLinear(1.0F) == doctest::Approx(1.0F));
    CHECK(gfx::linearToSrgb(0.0F) == 0.0F);
    CHECK(gfx::linearToSrgb(1.0F) == doctest::Approx(1.0F));
}

TEST_CASE("srgbToLinear gives the known values of the sRGB standard") {
    // Middle grey of a picture file (byte 128) carries only about a fifth of the light
    // of white. This gap is the reason for the whole conversion.
    CHECK(gfx::srgbToLinear(128.0F / BYTE_MAX) == doctest::Approx(0.21586F).epsilon(0.0001));
    // The usual reference pair: an encoded half is 0.214 of the light.
    CHECK(gfx::srgbToLinear(0.5F) == doctest::Approx(0.21404F).epsilon(0.0001));
    // And the other way: half of the light is shown as the encoded value 0.735.
    CHECK(gfx::linearToSrgb(0.5F) == doctest::Approx(0.73536F).epsilon(0.0001));
}

TEST_CASE("the darkest values lie on a straight line") {
    // Below the threshold the encoded value is divided by 12.92.
    CHECK(gfx::srgbToLinear(0.02F) == doctest::Approx(0.02F / 12.92F));
    CHECK(gfx::linearToSrgb(0.001F) == doctest::Approx(0.001F * 12.92F));
    // The two pieces of the curve meet at the threshold without a jump.
    const float below = gfx::srgbToLinear(0.04045F);
    const float above = gfx::srgbToLinear(0.04046F);
    CHECK(above > below);
    CHECK(above - below < 0.00001F);
}

TEST_CASE("srgbToLinear is close to the power 2.2 but not equal to it") {
    // The short formula pow(x, 2.2) is an approximation of the standard: the same
    // shape, a visibly different number in the dark tones.
    const float exact = gfx::srgbToLinear(0.2F);
    const float approximation = std::pow(0.2F, 2.2F);
    CHECK(exact == doctest::Approx(0.03310F).epsilon(0.001));
    CHECK(approximation == doctest::Approx(0.02899F).epsilon(0.001));
    // About an eighth apart: too much to mix the two formulas in one pipeline.
    CHECK(std::abs(exact - approximation) > 0.004F);
    CHECK(std::abs(exact - approximation) < 0.005F);
}

TEST_CASE("encoding undoes decoding for every byte of a picture") {
    // What a texture byte goes through in a frame without lighting: decoded on reading,
    // encoded by the composite pass. The screen must get the byte back.
    for (int byte = 0; byte <= 255; ++byte) {
        const float encoded = static_cast<float>(byte) / BYTE_MAX;
        const float roundTrip = gfx::linearToSrgb(gfx::srgbToLinear(encoded));
        // Closer than a quarter of one step of a byte.
        CHECK(std::abs(roundTrip - encoded) < 0.25F / BYTE_MAX);
    }
}

TEST_CASE("both conversions keep the order of brightness") {
    float previousLinear = -1.0F;
    float previousEncoded = -1.0F;
    for (int byte = 0; byte <= 255; ++byte) {
        const float value = static_cast<float>(byte) / BYTE_MAX;
        const float linear = gfx::srgbToLinear(value);
        const float encoded = gfx::linearToSrgb(value);
        CHECK(linear > previousLinear);
        CHECK(encoded > previousEncoded);
        previousLinear = linear;
        previousEncoded = encoded;
    }
}

TEST_CASE("values outside 0 to 1 are clamped") {
    CHECK(gfx::srgbToLinear(-0.5F) == 0.0F);
    CHECK(gfx::srgbToLinear(2.0F) == doctest::Approx(1.0F));
    CHECK(gfx::linearToSrgb(-0.5F) == 0.0F);
    // A colour brighter than white (HDR) cannot be shown brighter than white.
    CHECK(gfx::linearToSrgb(4.0F) == doctest::Approx(1.0F));
}

TEST_CASE("a colour is converted channel by channel") {
    const glm::vec3 encoded{0.2F, 0.5F, 0.8F};
    const glm::vec3 linear = gfx::srgbToLinear(encoded);
    CHECK(linear.r == gfx::srgbToLinear(0.2F));
    CHECK(linear.g == gfx::srgbToLinear(0.5F));
    CHECK(linear.b == gfx::srgbToLinear(0.8F));

    const glm::vec3 back = gfx::linearToSrgb(linear);
    CHECK(back.r == doctest::Approx(encoded.r));
    CHECK(back.g == doctest::Approx(encoded.g));
    CHECK(back.b == doctest::Approx(encoded.b));
}

TEST_CASE("a decoded dark colour is darker than its encoded numbers") {
    // Why the lights had to be tuned again: the ambient colour typed in as 0.1 is only
    // 0.01 of the light once it is converted.
    CHECK(gfx::srgbToLinear(0.1F) == doctest::Approx(0.01002F).epsilon(0.001));
}
