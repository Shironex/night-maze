// Tests of gfx::coverFit: the part of a picture that covers a target of another shape.
#include "gfx/CoverFit.hpp"

#include <doctest/doctest.h>

#include <array>
#include <initializer_list>

namespace {

// The shape of the shown part of a picture: its width divided by its height, in pixels
// of the picture.
float shownAspect(const gfx::UvRect& shown, int pictureWidth, int pictureHeight) {
    const float width = (shown.right - shown.left) * static_cast<float>(pictureWidth);
    const float height = (shown.bottom - shown.top) * static_cast<float>(pictureHeight);
    return width / height;
}

} // namespace

TEST_CASE("a target of the same shape shows the whole picture, at any size") {
    for (const int scale : {1, 2, 3}) {
        const gfx::UvRect shown = gfx::coverFit(1280, 720, 640 * scale, 360 * scale);
        CHECK(shown.left == doctest::Approx(0.0F));
        CHECK(shown.top == doctest::Approx(0.0F));
        CHECK(shown.right == doctest::Approx(1.0F));
        CHECK(shown.bottom == doctest::Approx(1.0F));
    }
}

TEST_CASE("a wider target keeps the full width and loses rows at the top and the bottom") {
    // 2560 x 1080 (64 : 27) against 16 : 9: three quarters of the height are shown.
    const gfx::UvRect shown = gfx::coverFit(1280, 720, 2560, 1080);
    CHECK(shown.left == doctest::Approx(0.0F));
    CHECK(shown.right == doctest::Approx(1.0F));
    CHECK(shown.top == doctest::Approx(0.125F));
    CHECK(shown.bottom == doctest::Approx(0.875F));
}

TEST_CASE("a narrower target keeps the full height and loses columns on both sides") {
    // 960 x 720 (4 : 3) against 16 : 9: three quarters of the width are shown.
    const gfx::UvRect shown = gfx::coverFit(1280, 720, 960, 720);
    CHECK(shown.top == doctest::Approx(0.0F));
    CHECK(shown.bottom == doctest::Approx(1.0F));
    CHECK(shown.left == doctest::Approx(0.125F));
    CHECK(shown.right == doctest::Approx(0.875F));
}

TEST_CASE("the shown part has the shape of the target and stays in the middle") {
    const std::array<std::array<int, 2>, 5> targets = {
        {{1100, 700}, {1366, 768}, {700, 1100}, {3840, 1080}, {1, 1}}};
    for (const auto& target : targets) {
        const gfx::UvRect shown = gfx::coverFit(1280, 720, target[0], target[1]);
        const float targetAspect = static_cast<float>(target[0]) / static_cast<float>(target[1]);
        // Not stretched: the shown pixels have the shape of the target.
        CHECK(shownAspect(shown, 1280, 720) == doctest::Approx(targetAspect));
        // Cover and not letterbox: nothing outside of the picture is ever shown.
        CHECK(shown.left >= 0.0F);
        CHECK(shown.top >= 0.0F);
        CHECK(shown.right <= 1.0F + 1e-6F);
        CHECK(shown.bottom <= 1.0F + 1e-6F);
        // The same amount is cut off on both sides.
        CHECK(shown.left == doctest::Approx(1.0F - shown.right));
        CHECK(shown.top == doctest::Approx(1.0F - shown.bottom));
    }
}

TEST_CASE("a size below one gives the whole picture") {
    const std::array<std::array<int, 4>, 5> sizes = {{{0, 720, 1280, 720},
                                                      {1280, 0, 1280, 720},
                                                      {1280, 720, 0, 720},
                                                      {1280, 720, 1280, 0},
                                                      {-4, 720, 1280, 720}}};
    for (const auto& size : sizes) {
        const gfx::UvRect shown = gfx::coverFit(size[0], size[1], size[2], size[3]);
        CHECK(shown.left == 0.0F);
        CHECK(shown.top == 0.0F);
        CHECK(shown.right == 1.0F);
        CHECK(shown.bottom == 1.0F);
    }
}
