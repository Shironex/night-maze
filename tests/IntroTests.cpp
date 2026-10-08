// Tests of game/Intro.hpp: the script of the intro, what it shows at a moment, where its
// camera is and when its sounds are played.
#include "game/Intro.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using game::IntroCard;
using game::IntroFrame;
using game::SoundCue;

// One frame at 60 pictures per second, in seconds.
constexpr float FRAME_SECONDS = 1.0F / 60.0F;

// The second at which card number card comes up: the sum of the cards before it.
float cardStart(std::size_t card) {
    float seconds = 0.0F;
    for (std::size_t i = 0; i < card; ++i) {
        seconds += game::introCards().at(i).seconds;
    }
    return seconds;
}

// Plays the whole intro in frames of frameSeconds, the way the application does, and
// returns every sound in the order it was played.
std::vector<SoundCue> cuesOfRun(float frameSeconds) {
    std::vector<SoundCue> played;
    float clock = 0.0F;
    // A little past the end: the frames after the last one must add nothing.
    while (clock < game::introSeconds() + 2.0F) {
        const float next = clock + frameSeconds;
        for (const SoundCue cue : game::introCuesBetween(clock, next)) {
            played.push_back(cue);
        }
        clock = next;
    }
    return played;
}

} // namespace

TEST_CASE("the intro has five cards of two lines with the exact text of the story") {
    const std::vector<std::vector<std::string>> expected = {
        {"Every winter the moon drops pieces of itself.",
         "They fall in the old maze. The village sleeps badly."},
        {"Someone has always walked in and carried them out.",
         "This winter the lamplighter's hands shake."},
        {"Outsiders call them crystals. We call them splinters.",
         "They remember being light. The lamp believes them."},
        {"Keep the lamp lit.", "Something walks where it is not."},
        {"One lamp is left in the village.", "It is yours."},
    };
    REQUIRE(game::INTRO_CARD_COUNT == expected.size());
    for (std::size_t card = 0; card < game::INTRO_CARD_COUNT; ++card) {
        const game::IntroLines lines = game::introLines(card, true);
        CHECK(lines.first == expected.at(card).at(0));
        CHECK(lines.second == expected.at(card).at(1));
    }
}

TEST_CASE("without the shadow only the second line of the fourth card changes") {
    for (std::size_t card = 0; card < game::INTRO_CARD_COUNT; ++card) {
        const game::IntroLines with = game::introLines(card, true);
        const game::IntroLines without = game::introLines(card, false);
        CHECK(std::string(with.first) == without.first);
        if (card == 3) {
            CHECK(std::string(without.second) == "It forgets quickly.");
        } else {
            CHECK(std::string(with.second) == without.second);
        }
    }
    CHECK_THROWS_AS(game::introLines(game::INTRO_CARD_COUNT, true), std::out_of_range);
}

TEST_CASE("the text of the cards can be written into a menu document as it is") {
    // ui::UiLayer::setText takes its text as RML, where < and & start something else.
    // The apostrophe of "lamplighter's" is a plain character there.
    for (std::size_t card = 0; card < game::INTRO_CARD_COUNT; ++card) {
        for (const bool shade : {true, false}) {
            const game::IntroLines lines = game::introLines(card, shade);
            for (const char* line : {lines.first, lines.second}) {
                CHECK(std::strlen(line) > 0);
                CHECK(std::strpbrk(line, "<>&") == nullptr);
                for (const char* character = line; *character != '\0'; ++character) {
                    // Printable ASCII only: no dash of another kind, no curly quote.
                    CHECK(*character >= ' ');
                    CHECK(*character <= '~');
                }
            }
        }
    }
}

TEST_CASE("the intro takes thirty seconds, about six per card") {
    CHECK(game::introSeconds() == doctest::Approx(30.0F));
    for (const IntroCard& card : game::introCards()) {
        CHECK(card.seconds >= 5.0F);
        CHECK(card.seconds <= 7.0F);
    }
}

TEST_CASE("the first card is text on black, the others lie over a picture") {
    const auto& cards = game::introCards();
    CHECK(cards.at(0).black);
    for (std::size_t card = 1; card < game::INTRO_CARD_COUNT; ++card) {
        CHECK_FALSE(cards.at(card).black);
    }
}

TEST_CASE("the lamp comes on with the cut from the second card to the third") {
    const auto& cards = game::introCards();
    // High over the maze with the light off, then through the corridors with it on.
    CHECK(cards.at(1).shot == game::MenuShot::HighGlide);
    CHECK_FALSE(cards.at(1).flashlightOn);
    for (std::size_t card = 2; card < game::INTRO_CARD_COUNT; ++card) {
        CHECK(cards.at(card).shot == game::MenuShot::CorridorWalk);
        CHECK(cards.at(card).flashlightOn);
    }
    // The click of the switch is the first sound of the third card, at the cut itself.
    REQUIRE(cards.at(2).cueCount >= 1);
    CHECK(cards.at(2).cues.at(0).cue == SoundCue::FlashlightOn);
    CHECK(cards.at(2).cues.at(0).atSeconds == 0.0F);
}

TEST_CASE("under the black of the first card the glide of the second is already running") {
    const auto& cards = game::introCards();
    CHECK(cards.at(0).shot == cards.at(1).shot);
    CHECK(cards.at(0).flashlightOn == cards.at(1).flashlightOn);
    // The first card ends where the second begins: no jump when the black lifts.
    CHECK(cards.at(0).shotSeconds + cards.at(0).seconds ==
          doctest::Approx(cards.at(1).shotSeconds));
}

TEST_CASE("every moment belongs to one card, and the cards follow each other") {
    CHECK(game::introFrame(0.0F).card == 0);
    CHECK(game::introFrame(0.0F).cardSeconds == 0.0F);
    for (std::size_t card = 0; card < game::INTRO_CARD_COUNT; ++card) {
        const float start = cardStart(card);
        const float seconds = game::introCards().at(card).seconds;
        // The cut belongs to the card that begins.
        CHECK(game::introFrame(start).card == card);
        CHECK(game::introFrame(start).cardSeconds == doctest::Approx(0.0F));
        CHECK(game::introFrame(start + seconds * 0.5F).card == card);
        CHECK(game::introFrame(start + seconds * 0.5F).cardSeconds ==
              doctest::Approx(seconds * 0.5F));
        CHECK(game::introFrame(start + seconds - 0.001F).card == card);
    }
    // A moment before the start counts as the start.
    CHECK(game::introFrame(-3.0F).card == 0);
    CHECK(game::introFrame(-3.0F).cardSeconds == 0.0F);
    CHECK_FALSE(game::introFrame(-3.0F).finished);
}

TEST_CASE("a card fades in, stands and fades out, and is gone before the cut") {
    for (std::size_t card = 0; card < game::INTRO_CARD_COUNT; ++card) {
        const float start = cardStart(card);
        const float seconds = game::introCards().at(card).seconds;
        const float fadeInEnd = game::INTRO_TEXT_DELAY_SECONDS + game::INTRO_TEXT_FADE_SECONDS;
        const float fadeOutStart =
            seconds - game::INTRO_TEXT_GAP_SECONDS - game::INTRO_TEXT_FADE_SECONDS;

        // Nothing at the cut and during the short wait after it.
        CHECK(game::introFrame(start).textOpacity == 0.0F);
        CHECK(game::introFrame(start + game::INTRO_TEXT_DELAY_SECONDS * 0.5F).textOpacity == 0.0F);
        // Half way through the fade in: half of the text.
        CHECK(game::introFrame(start + game::INTRO_TEXT_DELAY_SECONDS +
                               game::INTRO_TEXT_FADE_SECONDS * 0.5F)
                  .textOpacity == doctest::Approx(0.5F).epsilon(0.01));
        // All of it between the two fades.
        CHECK(game::introFrame(start + fadeInEnd + 0.01F).textOpacity == 1.0F);
        CHECK(game::introFrame(start + seconds * 0.5F).textOpacity == 1.0F);
        CHECK(game::introFrame(start + fadeOutStart - 0.01F).textOpacity == 1.0F);
        // Half way through the fade out, and nothing in the gap before the cut.
        CHECK(game::introFrame(start + fadeOutStart + game::INTRO_TEXT_FADE_SECONDS * 0.5F)
                  .textOpacity == doctest::Approx(0.5F).epsilon(0.01));
        CHECK(game::introFrame(start + seconds - game::INTRO_TEXT_GAP_SECONDS * 0.5F).textOpacity ==
              0.0F);

        // The text stands in full for at least four seconds: time to read two lines.
        CHECK(fadeOutStart - fadeInEnd >= 4.0F);
    }
}

TEST_CASE("the text never jumps: its opacity moves a little from frame to frame") {
    float before = game::introFrame(0.0F).textOpacity;
    const int frames = static_cast<int>((game::introSeconds() + 1.0F) / FRAME_SECONDS);
    for (int frame = 1; frame <= frames; ++frame) {
        const float now = game::introFrame(static_cast<float>(frame) * FRAME_SECONDS).textOpacity;
        CHECK(now >= 0.0F);
        CHECK(now <= 1.0F);
        // One frame of a fade of 0.7 seconds is a step of 0.024.
        CHECK(std::abs(now - before) < 0.03F);
        before = now;
    }
}

TEST_CASE("the black covers the first card, lifts from the second and falls at the end") {
    const float second = cardStart(1);
    // All of the first card, to its last moment.
    CHECK(game::introFrame(0.0F).blackOpacity == 1.0F);
    CHECK(game::introFrame(second * 0.5F).blackOpacity == 1.0F);
    CHECK(game::introFrame(second - 0.001F).blackOpacity == 1.0F);
    // It lifts over INTRO_BLACK_FADE_SECONDS, without a jump at the cut.
    CHECK(game::introFrame(second).blackOpacity == doctest::Approx(1.0F));
    CHECK(game::introFrame(second + game::INTRO_BLACK_FADE_SECONDS * 0.5F).blackOpacity ==
          doctest::Approx(0.5F).epsilon(0.01));
    CHECK(game::introFrame(second + game::INTRO_BLACK_FADE_SECONDS + 0.01F).blackOpacity == 0.0F);

    // The pictures in between are clear, also right after their cuts: only the card
    // after a black one fades in.
    for (std::size_t card = 2; card < game::INTRO_CARD_COUNT; ++card) {
        CHECK(game::introFrame(cardStart(card)).blackOpacity == 0.0F);
        CHECK(game::introFrame(cardStart(card) + 1.0F).blackOpacity == 0.0F);
    }

    // It falls over the last INTRO_BLACK_FADE_SECONDS and is complete at the end.
    const float total = game::introSeconds();
    CHECK(game::introFrame(total - game::INTRO_BLACK_FADE_SECONDS - 0.01F).blackOpacity == 0.0F);
    CHECK(game::introFrame(total - game::INTRO_BLACK_FADE_SECONDS * 0.5F).blackOpacity ==
          doctest::Approx(0.5F).epsilon(0.01));
    CHECK(game::introFrame(total).blackOpacity == 1.0F);
}

TEST_CASE("the last card is read before the black falls over it") {
    // The text of the last card is gone before the picture is: nothing is cut off.
    const std::size_t last = game::INTRO_CARD_COUNT - 1;
    const float seconds = game::introCards().at(last).seconds;
    const float textGone = cardStart(last) + seconds - game::INTRO_TEXT_GAP_SECONDS;
    CHECK(game::introFrame(textGone).textOpacity == doctest::Approx(0.0F).epsilon(0.01));
    CHECK(game::introFrame(textGone).blackOpacity < 1.0F);
    // While the text stands in full, most of the picture is still there.
    const float fullUntil = textGone - game::INTRO_TEXT_FADE_SECONDS;
    CHECK(game::introFrame(fullUntil - 0.3F).textOpacity == 1.0F);
    CHECK(game::introFrame(fullUntil - 0.3F).blackOpacity == 0.0F);
}

TEST_CASE("the skip hint is there for the first seconds and then fades away") {
    CHECK(game::introFrame(0.0F).hintOpacity == 0.0F);
    CHECK(game::introFrame(game::INTRO_HINT_FADE_SECONDS).hintOpacity == doctest::Approx(1.0F));
    CHECK(game::introFrame(2.0F).hintOpacity == 1.0F);
    CHECK(game::introFrame(game::INTRO_HINT_SECONDS).hintOpacity == doctest::Approx(1.0F));
    CHECK(game::introFrame(game::INTRO_HINT_SECONDS + game::INTRO_HINT_FADE_SECONDS * 0.5F)
              .hintOpacity == doctest::Approx(0.5F).epsilon(0.01));
    CHECK(game::introFrame(game::INTRO_HINT_SECONDS + game::INTRO_HINT_FADE_SECONDS + 0.01F)
              .hintOpacity == 0.0F);
    // Gone for the rest of the intro.
    CHECK(game::introFrame(12.0F).hintOpacity == 0.0F);
    CHECK(game::introFrame(game::introSeconds()).hintOpacity == 0.0F);
}

TEST_CASE("a key skips the intro only after its first half second") {
    CHECK_FALSE(game::introFrame(0.0F).skippable);
    CHECK_FALSE(game::introFrame(game::INTRO_SKIP_DELAY_SECONDS - 0.01F).skippable);
    CHECK(game::introFrame(game::INTRO_SKIP_DELAY_SECONDS).skippable);
    CHECK(game::introFrame(20.0F).skippable);
    // The hint does not promise what is not possible yet: it is complete no earlier
    // than the moment a key works.
    CHECK(game::INTRO_HINT_FADE_SECONDS >= game::INTRO_SKIP_DELAY_SECONDS);
}

TEST_CASE("the intro is finished at its end and not a moment earlier") {
    const float total = game::introSeconds();
    CHECK_FALSE(game::introFrame(0.0F).finished);
    CHECK_FALSE(game::introFrame(total - 0.001F).finished);
    CHECK(game::introFrame(total).finished);
    CHECK(game::introFrame(total + 100.0F).finished);
    // Past the end the frame is the last moment: the last card, no text, all black.
    const IntroFrame after = game::introFrame(total + 100.0F);
    CHECK(after.card == game::INTRO_CARD_COUNT - 1);
    CHECK(after.textOpacity == 0.0F);
    CHECK(after.blackOpacity == 1.0F);
}

TEST_CASE("the camera of a card starts at its moment of the shot and runs on with the card") {
    for (std::size_t card = 0; card < game::INTRO_CARD_COUNT; ++card) {
        const IntroCard& entry = game::introCards().at(card);
        const game::IntroCamera atCut = game::introCamera(game::introFrame(cardStart(card)));
        CHECK(atCut.settings.shot == entry.shot);
        CHECK(atCut.menuSeconds == doctest::Approx(entry.shotSeconds));
        CHECK(atCut.flashlightOn == entry.flashlightOn);

        const game::IntroCamera later = game::introCamera(game::introFrame(cardStart(card) + 2.5F));
        CHECK(later.menuSeconds == doctest::Approx(entry.shotSeconds + 2.5F));

        // The camera of a fresh menu camera: the shots do not depend on what the debug
        // window did to the one of the game.
        CHECK(atCut.settings.speed == game::DEFAULT_MENU_CAMERA_SPEED);
        CHECK(atCut.settings.eyeHeight == game::DEFAULT_MENU_CAMERA_EYE_HEIGHT);
        CHECK(atCut.settings.timeOffset == 0.0F);
    }
}

TEST_CASE("the sounds of the script, in their order and at their moments") {
    const auto& cards = game::introCards();
    // The wind starts with the first card and the bell rings a little later.
    REQUIRE(cards.at(0).cueCount == 2);
    CHECK(cards.at(0).cues.at(0).cue == SoundCue::IntroWind);
    CHECK(cards.at(0).cues.at(0).atSeconds == 0.0F);
    CHECK(cards.at(0).cues.at(1).cue == SoundCue::IntroBell);
    // The glide is silent but for the wind.
    CHECK(cards.at(1).cueCount == 0);
    // The click, then the chime of the crystal.
    REQUIRE(cards.at(2).cueCount == 2);
    CHECK(cards.at(2).cues.at(1).cue == SoundCue::CrystalPickup);
    // Two beats of the pulse, at least the length of its sound apart (0.45 s).
    REQUIRE(cards.at(3).cueCount == 2);
    CHECK(cards.at(3).cues.at(0).cue == SoundCue::LowBatteryPulse);
    CHECK(cards.at(3).cues.at(1).cue == SoundCue::LowBatteryPulse);
    CHECK(cards.at(3).cues.at(1).atSeconds - cards.at(3).cues.at(0).atSeconds >= 0.45F);
    // The bell once more.
    REQUIRE(cards.at(4).cueCount == 1);
    CHECK(cards.at(4).cues.at(0).cue == SoundCue::IntroBell);

    // Every sound lies inside its card, and the sounds of a card are in order.
    for (const IntroCard& card : cards) {
        REQUIRE(card.cueCount <= game::MAX_INTRO_CARD_CUES);
        for (std::size_t i = 0; i < card.cueCount; ++i) {
            CHECK(card.cues.at(i).atSeconds >= 0.0F);
            CHECK(card.cues.at(i).atSeconds < card.seconds);
            if (i > 0) {
                CHECK(card.cues.at(i).atSeconds > card.cues.at(i - 1).atSeconds);
            }
        }
    }
}

TEST_CASE("a run of the intro plays every sound exactly once, at any frame rate") {
    const std::vector<SoundCue> expected = {
        SoundCue::IntroWind,     SoundCue::IntroBell,       SoundCue::FlashlightOn,
        SoundCue::CrystalPickup, SoundCue::LowBatteryPulse, SoundCue::LowBatteryPulse,
        SoundCue::IntroBell,
    };
    // 144, 60 and 30 pictures per second, the longest frame the clock of the game
    // hands out (0.25 s), and a step that is no fraction of a second.
    for (const float frameSeconds : {1.0F / 144.0F, FRAME_SECONDS, 1.0F / 30.0F, 0.25F, 0.0371F}) {
        CAPTURE(frameSeconds);
        CHECK(cuesOfRun(frameSeconds) == expected);
    }
}

TEST_CASE("a stretch of time holds the sounds from its start up to, not with, its end") {
    // The wind lies at second 0: in the first stretch, and in no later one.
    CHECK(game::introCuesBetween(0.0F, FRAME_SECONDS) ==
          std::vector<SoundCue>{SoundCue::IntroWind});
    CHECK(game::introCuesBetween(FRAME_SECONDS, 2.0F * FRAME_SECONDS).empty());

    // The click of the lamp lies exactly on the cut to the third card.
    const float cut = cardStart(2);
    CHECK(game::introCuesBetween(cut - 0.1F, cut).empty());
    CHECK(game::introCuesBetween(cut, cut + 0.1F) == std::vector<SoundCue>{SoundCue::FlashlightOn});
    CHECK(game::introCuesBetween(cut - 0.1F, cut + 0.1F) ==
          std::vector<SoundCue>{SoundCue::FlashlightOn});

    // An empty stretch holds nothing, also at the moment of a sound.
    CHECK(game::introCuesBetween(cut, cut).empty());
    CHECK(game::introCuesBetween(0.0F, 0.0F).empty());
    // One long stretch over two cards holds the sounds of both, in the order of the
    // script.
    CHECK(game::introCuesBetween(cardStart(2), cardStart(4)) ==
          std::vector<SoundCue>{SoundCue::FlashlightOn, SoundCue::CrystalPickup,
                                SoundCue::LowBatteryPulse, SoundCue::LowBatteryPulse});
    // Nothing lies past the end.
    CHECK(game::introCuesBetween(game::introSeconds(), game::introSeconds() + 60.0F).empty());
}

TEST_CASE("the intro uses the easy maze of seed 1, whatever the player chose") {
    CHECK(game::INTRO_MAZE_SEED == 1U);
    CHECK(game::INTRO_DIFFICULTY == game::Difficulty::Easy);
}
