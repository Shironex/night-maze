// Tests of the parts of the bloom that need no OpenGL context: the size of its render
// targets and the weights of its blur.
#include "game/Bloom.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>

// The passes themselves (bright pass, blur, composite) are shaders and are checked by
// running the game.

TEST_CASE("a bloom target is half the scene in each direction") {
    CHECK(game::bloomTargetExtent(1280) == 640);
    CHECK(game::bloomTargetExtent(720) == 360);
    CHECK(game::bloomTargetExtent(2560) == 1280);
}

TEST_CASE("an odd scene size is halved with the rest dropped") {
    CHECK(game::bloomTargetExtent(1281) == 640);
    CHECK(game::bloomTargetExtent(719) == 359);
    CHECK(game::bloomTargetExtent(3) == 1);
}

TEST_CASE("a bloom target is never smaller than one pixel") {
    // A window dragged down to a sliver: 1 / 2 is 0 as an integer, and a texture of
    // size 0 cannot be attached to a framebuffer.
    CHECK(game::bloomTargetExtent(2) == 1);
    CHECK(game::bloomTargetExtent(1) == 1);
    CHECK(game::bloomTargetExtent(0) == 1);
}

TEST_CASE("the blur kernel adds up to one") {
    const auto weights = game::bloomBlurWeights();
    // The whole kernel as the shader reads it: the centre once, every other distance
    // on both sides.
    float sum = weights[0];
    for (std::size_t distance = 1; distance < weights.size(); ++distance) {
        sum += 2.0F * weights[distance];
    }
    // 1 means that a blur pass neither adds light nor loses any.
    CHECK(sum == doctest::Approx(1.0F).epsilon(0.00001));
}

TEST_CASE("the blur weights fall with the distance and stay above zero") {
    const auto weights = game::bloomBlurWeights();
    REQUIRE(weights.size() == static_cast<std::size_t>(game::BLOOM_BLUR_RADIUS) + 1);
    for (std::size_t distance = 1; distance < weights.size(); ++distance) {
        CHECK(weights[distance] < weights[distance - 1]);
        CHECK(weights[distance] > 0.0F);
    }
}

TEST_CASE("the blur weights follow the Gaussian function") {
    const auto weights = game::bloomBlurWeights();
    // The division by the sum scales every weight by the same number, so the ratio of
    // two weights is the ratio of the bell at their distances: exp(-d * d / (2 s * s)).
    const float sigma = game::BLOOM_BLUR_SIGMA;
    for (std::size_t distance = 1; distance < weights.size(); ++distance) {
        const auto d = static_cast<float>(distance);
        const float expected = std::exp(-(d * d) / (2.0F * sigma * sigma));
        CHECK(weights[distance] / weights[0] == doctest::Approx(expected));
    }
    // With sigma 3 and six pixels on each side: the numbers worked out by hand. The
    // bell is 1, 0.946, 0.801, 0.607, 0.411, 0.249 and 0.135 at the distances 0 to 6,
    // and the whole kernel sums to 1 + 2 * 3.149 = 7.298 before the division.
    CHECK(weights[0] == doctest::Approx(0.1370F).epsilon(0.001));
    CHECK(weights[6] == doctest::Approx(0.0185F).epsilon(0.01));
}

TEST_CASE("the bloom settings start inside their ranges") {
    const game::BloomSettings settings;
    CHECK(settings.enabled);
    CHECK(settings.threshold > 0.0F);
    CHECK(settings.intensity > 0.0F);
    CHECK(settings.blurIterations >= game::MIN_BLOOM_BLUR_ITERATIONS);
    CHECK(settings.blurIterations <= game::MAX_BLOOM_BLUR_ITERATIONS);
}
