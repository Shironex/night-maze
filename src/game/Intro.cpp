// Intro: the five cards the game opens with on its first start, as a script of plain data.
#include "game/Intro.hpp"

#include <algorithm>

namespace game {

namespace {

// The second line of the fourth card in a game without the shadow (introLines), and the
// number of that card.
constexpr std::size_t SHADE_CARD = 3;
constexpr const char* CALM_SECOND_LINE = "It forgets quickly.";

// The script. Every card takes six seconds, thirty in all.
//
// The first card is text on black. Under it the glide of the second card is already
// running (it starts six seconds earlier in its loop), so the black lifts from a picture
// that moves. The glide has the flashlight off, the three walks have it on: the cut
// from the second card to the third is the moment the lamp comes on, with its click.
//
// The moments of the shots (shotSeconds) were picked by eye in the maze of
// INTRO_MAZE_SEED on INTRO_DIFFICULTY, with the switches --menu-camera, --menu-shot and
// --menu-time. They go out of date when the generator of the maze or the route of the
// menu camera changes: then look for new ones.
constexpr std::array<IntroCard, INTRO_CARD_COUNT> INTRO_CARDS = {{
    // 1: the wind is there from the first moment, then one far bell.
    {.firstLine = "Every winter the moon drops pieces of itself.",
     .secondLine = "They fall in the old maze. The village sleeps badly.",
     .seconds = 6.0F,
     .black = true,
     .shot = MenuShot::HighGlide,
     .shotSeconds = 8.0F,
     .flashlightOn = false,
     .cues = {{{.atSeconds = 0.0F, .cue = SoundCue::IntroWind},
               {.atSeconds = 1.5F, .cue = SoundCue::IntroBell}}},
     .cueCount = 2},
    // 2: high over the maze in the moonlight.
    {.firstLine = "Someone has always walked in and carried them out.",
     .secondLine = "This winter the lamplighter's hands shake.",
     .seconds = 6.0F,
     .black = false,
     .shot = MenuShot::HighGlide,
     .shotSeconds = 14.0F,
     .flashlightOn = false,
     .cues = {},
     .cueCount = 0},
    // 3: the lamp comes on, and the walk goes towards a crystal at the end of
    // a corridor. Its chime comes when the crystal is in view.
    {.firstLine = "Outsiders call them crystals. We call them splinters.",
     .secondLine = "They remember being light. The lamp believes them.",
     .seconds = 6.0F,
     .black = false,
     .shot = MenuShot::CorridorWalk,
     .shotSeconds = 439.5F,
     .flashlightOn = true,
     .cues = {{{.atSeconds = 0.0F, .cue = SoundCue::FlashlightOn},
               {.atSeconds = 1.2F, .cue = SoundCue::CrystalPickup}}},
     .cueCount = 2},
    // 4: a long corridor with the shadows of the pillars, and two beats of the pulse
    // of a low battery.
    {.firstLine = "Keep the lamp lit.",
     .secondLine = "Something walks where it is not.",
     .seconds = 6.0F,
     .black = false,
     .shot = MenuShot::CorridorWalk,
     .shotSeconds = 149.0F,
     .flashlightOn = true,
     .cues = {{{.atSeconds = 1.5F, .cue = SoundCue::LowBatteryPulse},
               {.atSeconds = 3.5F, .cue = SoundCue::LowBatteryPulse}}},
     .cueCount = 2},
    // 5: past the gate of the exit, and the bell once more.
    {.firstLine = "One lamp is left in the village.",
     .secondLine = "It is yours.",
     .seconds = 6.0F,
     .black = false,
     .shot = MenuShot::CorridorWalk,
     .shotSeconds = 236.5F,
     .flashlightOn = true,
     .cues = {{{.atSeconds = 2.0F, .cue = SoundCue::IntroBell}}},
     .cueCount = 1},
}};

// A number brought into the range from 0 to 1.
float unit(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

// How much of the text of a card is there, cardSeconds after the cut to the card, for
// a card that takes seconds: up along a straight line, 1 for a while, down again.
float textOpacity(float cardSeconds, float seconds) {
    const float rising = (cardSeconds - INTRO_TEXT_DELAY_SECONDS) / INTRO_TEXT_FADE_SECONDS;
    const float falling =
        (seconds - INTRO_TEXT_GAP_SECONDS - cardSeconds) / INTRO_TEXT_FADE_SECONDS;
    return unit(std::min(rising, falling));
}

} // namespace

const std::array<IntroCard, INTRO_CARD_COUNT>& introCards() {
    return INTRO_CARDS;
}

float introSeconds() {
    float seconds = 0.0F;
    for (const IntroCard& card : INTRO_CARDS) {
        seconds += card.seconds;
    }
    return seconds;
}

IntroLines introLines(std::size_t card, bool shadeInGame) {
    // at() checks the number: one that is no card throws instead of reading past the
    // script.
    const IntroCard& entry = INTRO_CARDS.at(card);
    if (card == SHADE_CARD && !shadeInGame) {
        return {.first = entry.firstLine, .second = CALM_SECOND_LINE};
    }
    return {.first = entry.firstLine, .second = entry.secondLine};
}

IntroFrame introFrame(float seconds) {
    const float total = introSeconds();
    IntroFrame frame;
    frame.finished = seconds >= total;
    const float moment = std::clamp(seconds, 0.0F, total);

    // The card of the moment: the first one whose end lies after it. At the very end
    // no card does, and the last one stays.
    float cardStart = 0.0F;
    for (std::size_t i = 0;
         i + 1 < INTRO_CARD_COUNT && moment >= cardStart + INTRO_CARDS.at(i).seconds; ++i) {
        cardStart += INTRO_CARDS.at(i).seconds;
        frame.card = i + 1;
    }
    const IntroCard& card = INTRO_CARDS.at(frame.card);
    frame.cardSeconds = moment - cardStart;
    frame.textOpacity = textOpacity(frame.cardSeconds, card.seconds);

    // The black. All of it on a card without a picture. On the card after such a card
    // it lifts, and at the end of the intro it comes down again, so the main menu can
    // come in out of black.
    if (card.black) {
        frame.blackOpacity = 1.0F;
    } else {
        const bool afterBlack = frame.card > 0 && INTRO_CARDS.at(frame.card - 1).black;
        const float lifting =
            afterBlack ? 1.0F - frame.cardSeconds / INTRO_BLACK_FADE_SECONDS : 0.0F;
        const float falling = 1.0F - (total - moment) / INTRO_BLACK_FADE_SECONDS;
        frame.blackOpacity = unit(std::max(lifting, falling));
    }

    frame.hintOpacity = unit(std::min(moment / INTRO_HINT_FADE_SECONDS,
                                      (INTRO_HINT_SECONDS + INTRO_HINT_FADE_SECONDS - moment) /
                                          INTRO_HINT_FADE_SECONDS));
    frame.skippable = moment >= INTRO_SKIP_DELAY_SECONDS;
    return frame;
}

IntroCamera introCamera(const IntroFrame& frame) {
    const IntroCard& card = INTRO_CARDS.at(frame.card);
    IntroCamera camera;
    camera.settings.shot = card.shot;
    camera.menuSeconds = card.shotSeconds + frame.cardSeconds;
    camera.flashlightOn = card.flashlightOn;
    return camera;
}

std::vector<SoundCue> introCuesBetween(float before, float after) {
    std::vector<SoundCue> cues;
    float cardStart = 0.0F;
    for (const IntroCard& card : INTRO_CARDS) {
        for (std::size_t i = 0; i < card.cueCount; ++i) {
            const float moment = cardStart + card.cues.at(i).atSeconds;
            if (moment >= before && moment < after) {
                cues.push_back(card.cues.at(i).cue);
            }
        }
        cardStart += card.seconds;
    }
    return cues;
}

} // namespace game
