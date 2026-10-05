// Tests of the formula of the vignette, which needs no OpenGL context.
// See docs/modules/renderer/post-process.md
#include "game/Vignette.hpp"

#include <doctest/doctest.h>

#include <cmath>

// The vignette itself is computed by post/composite.frag, which repeats this formula,
// and is checked by running the game.

namespace {

// The four corners of the screen as texture coordinates.
constexpr glm::vec2 BOTTOM_LEFT{0.0F, 0.0F};
constexpr glm::vec2 BOTTOM_RIGHT{1.0F, 0.0F};
constexpr glm::vec2 TOP_LEFT{0.0F, 1.0F};
constexpr glm::vec2 TOP_RIGHT{1.0F, 1.0F};

} // namespace

TEST_CASE("the corner distance is the length of half the screen diagonal") {
    CHECK(game::VIGNETTE_CORNER_DISTANCE == doctest::Approx(std::sqrt(0.5F)));
    CHECK(glm::length(TOP_RIGHT - game::SCREEN_CENTER) ==
          doctest::Approx(game::VIGNETTE_CORNER_DISTANCE));
}

TEST_CASE("the vignette leaves the middle of the screen as it is") {
    CHECK(game::vignetteFactor(game::SCREEN_CENTER, 0.3F, 0.4F) == doctest::Approx(1.0F));
    CHECK(game::vignetteFactor(game::SCREEN_CENTER, 1.0F, 0.1F) == doctest::Approx(1.0F));
    // Everything inside the radius: a point 0.3 from the middle with a radius of 0.4.
    CHECK(game::vignetteFactor(glm::vec2{0.8F, 0.5F}, 1.0F, 0.4F) == doctest::Approx(1.0F));
}

TEST_CASE("a vignette of strength zero changes nothing anywhere") {
    CHECK(game::vignetteFactor(BOTTOM_LEFT, 0.0F, 0.4F) == doctest::Approx(1.0F));
    CHECK(game::vignetteFactor(glm::vec2{0.9F, 0.2F}, 0.0F, 0.1F) == doctest::Approx(1.0F));
}

TEST_CASE("the four corners lose the share of light the strength says") {
    const float strength = 0.3F;
    const float radius = 0.4F;
    CHECK(game::vignetteFactor(BOTTOM_LEFT, strength, radius) == doctest::Approx(0.7F));
    CHECK(game::vignetteFactor(BOTTOM_RIGHT, strength, radius) == doctest::Approx(0.7F));
    CHECK(game::vignetteFactor(TOP_LEFT, strength, radius) == doctest::Approx(0.7F));
    CHECK(game::vignetteFactor(TOP_RIGHT, strength, radius) == doctest::Approx(0.7F));
    // Strength 1: black corners.
    CHECK(game::vignetteFactor(TOP_RIGHT, 1.0F, radius) == doctest::Approx(0.0F));
}

TEST_CASE("the vignette gets darker from the radius to the corner") {
    const float strength = 0.5F;
    const float radius = 0.2F;
    // Along the diagonal, in seven steps of 0.05 in each direction: from 0.2 away from
    // the middle in x and y (just outside the radius) to 0.5, the corner itself.
    float before = 1.0F;
    for (int step = 4; step <= 10; ++step) {
        const float offset = static_cast<float>(step) * 0.05F;
        const float factor = game::vignetteFactor(game::SCREEN_CENTER + offset, strength, radius);
        CHECK(factor < before);
        CHECK(factor >= 1.0F - strength);
        before = factor;
    }
    // Halfway between the radius and the corner smoothstep is one half, so half of the
    // strength is applied. The point lies on the horizontal line through the middle.
    const float halfway = (radius + game::VIGNETTE_CORNER_DISTANCE) / 2.0F;
    CHECK(game::vignetteFactor(game::SCREEN_CENTER + glm::vec2{halfway, 0.0F}, strength, radius) ==
          doctest::Approx(1.0F - strength / 2.0F));
}

TEST_CASE("the vignette is measured in texture coordinates, the same in both directions") {
    // The middle of the right edge and the middle of the top edge are both 0.5 away,
    // whatever shape the window has: the vignette is not corrected for the aspect ratio.
    const float strength = 0.6F;
    const float radius = 0.3F;
    CHECK(game::vignetteFactor(glm::vec2{1.0F, 0.5F}, strength, radius) ==
          doctest::Approx(game::vignetteFactor(glm::vec2{0.5F, 1.0F}, strength, radius)));
}

TEST_CASE("the default vignette is subtle") {
    const game::VignetteSettings settings;
    CHECK(settings.enabled);
    CHECK(settings.radius < game::VIGNETTE_CORNER_DISTANCE);
    // The corners keep at least half of their light, and the middles of the edges
    // almost all of it.
    CHECK(game::vignetteFactor(TOP_RIGHT, settings.strength, settings.radius) >= 0.5F);
    CHECK(game::vignetteFactor(glm::vec2{1.0F, 0.5F}, settings.strength, settings.radius) > 0.85F);
}
