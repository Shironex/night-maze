// Tests of the rules of the HUD that are plain arithmetic: its scale and the time of
// a sentence.
#include "debug/HudRules.hpp"

#include <doctest/doctest.h>

TEST_CASE("the HUD grows with the height of the window") {
    // The sizes are written for a window 720 pixels high.
    CHECK(debug::hudScale(1.0F, 720.0F) == doctest::Approx(1.0F));
    CHECK(debug::hudScale(1.0F, 1080.0F) == doctest::Approx(1.5F));
    CHECK(debug::hudScale(1.0F, 1440.0F) == doctest::Approx(2.0F));
    CHECK(debug::hudScale(1.0F, 2160.0F) == doctest::Approx(3.0F));
}

TEST_CASE("a larger display scale wins over the height of the window") {
    // A window of 720 pixels on a display scaled to 150 %.
    CHECK(debug::hudScale(1.5F, 720.0F) == doctest::Approx(1.5F));
    // A window smaller than the reference: the HUD is never smaller than the desktop.
    CHECK(debug::hudScale(1.0F, 360.0F) == doctest::Approx(1.0F));
    // A window without a height (minimized).
    CHECK(debug::hudScale(1.25F, 0.0F) == doctest::Approx(1.25F));
    // The window wins when it asks for more than the display.
    CHECK(debug::hudScale(1.5F, 2160.0F) == doctest::Approx(3.0F));
}

TEST_CASE("a sentence is fully there for its time, then fades, then is gone") {
    constexpr float HOLD = 6.0F;
    constexpr float FADE = 0.5F;
    CHECK(debug::hintOpacity(0.0F, HOLD, FADE) == 1.0F);
    CHECK(debug::hintOpacity(3.0F, HOLD, FADE) == 1.0F);
    CHECK(debug::hintOpacity(6.0F, HOLD, FADE) == 1.0F);
    // Half of the fade is over.
    CHECK(debug::hintOpacity(6.25F, HOLD, FADE) == doctest::Approx(0.5F));
    CHECK(debug::hintOpacity(6.5F, HOLD, FADE) == doctest::Approx(0.0F));
    CHECK(debug::hintOpacity(60.0F, HOLD, FADE) == 0.0F);
}

TEST_CASE("a sentence is not there before its moment, and can go without a fade") {
    CHECK(debug::hintOpacity(-1.0F, 6.0F, 0.5F) == 0.0F);
    CHECK(debug::hintOpacity(4.0F, 4.0F, 0.0F) == 1.0F);
    CHECK(debug::hintOpacity(4.01F, 4.0F, 0.0F) == 0.0F);
}

TEST_CASE("a sentence never gets brighter again while it fades") {
    float before = 1.0F;
    for (int i = 0; i <= 100; ++i) {
        const float opacity = debug::hintOpacity(static_cast<float>(i) * 0.1F, 4.0F, 0.5F);
        CHECK(opacity <= before);
        CHECK(opacity >= 0.0F);
        before = opacity;
    }
}
