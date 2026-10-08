// Intro: the five cards a campaign begins with, as a script of plain data.
#pragma once

#include "game/Difficulty.hpp"
#include "game/Maze.hpp"
#include "game/MenuCamera.hpp"
#include "game/SoundCues.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace game {

// Plain data and pure functions without a window, without OpenGL, without the menu
// library and without a sound card, like the rest of the game_logic library, so tests
// can check every rule. Nothing here draws, plays or keeps a clock: the functions only
// say WHAT the intro shows and plays a number of seconds after it began. The application
// (NightMazeApp) counts those seconds and asks.
//
// The intro is played live: every card but the first lies over a picture of the game
// itself, taken by the menu camera (game/MenuCamera.hpp) in one fixed maze. So it never
// goes out of date when the look of the game changes, and it needs no video file.

/// The maze of the intro: this seed on this level, whatever the player has chosen. The
/// moments of the shots below were picked by eye in this maze.
constexpr std::uint32_t INTRO_MAZE_SEED = 1;
constexpr Difficulty INTRO_DIFFICULTY = Difficulty::Easy;

/// One sound of the intro and its moment.
struct IntroCue {
    /// Seconds after the card it belongs to came up.
    float atSeconds = 0.0F;
    SoundCue cue = SoundCue::IntroWind;
};

/// The largest number of sounds one card has.
constexpr std::size_t MAX_INTRO_CARD_CUES = 2;

/// One card: two lines of text, how long it stays, the picture behind it and its
/// sounds.
struct IntroCard {
    /// The two lines. The second line of one card depends on whether the shadow is in
    /// the game: ask introLines for the text to show.
    const char* firstLine = "";
    const char* secondLine = "";

    /// How long the card takes, from the cut to it until the cut to the next one.
    float seconds = 0.0F;

    /// True: no picture, the text stands on black. The camera fields below are still
    /// set: they say what waits under the black, so the first picture can fade in
    /// already moving.
    bool black = false;

    /// The shot of the menu camera behind the card, and where in the loop of that shot
    /// the card begins, in seconds (what the switch --menu-time takes). The shot runs
    /// on from there for as long as the card is shown.
    MenuShot shot = MenuShot::HighGlide;
    float shotSeconds = 0.0F;

    /// Whether the flashlight of the camera is on. The high glide has it off and the
    /// corridor walk has it on, so the cut between the two is the moment the lamp
    /// comes on.
    bool flashlightOn = false;

    /// True: the picture of the card shows the shade, standing still in the middle of
    /// shadeCell. It is a prop of the picture and not the shade of a round: it does not
    /// walk, it makes no sound and it catches nobody. Ask introShadeCell, which leaves
    /// it out in a game without the shadow.
    bool showsShade = false;
    MazeCell shadeCell;

    /// The sounds of the card, in the order of their moments. Only the first cueCount
    /// entries are used.
    std::array<IntroCue, MAX_INTRO_CARD_CUES> cues{};
    std::size_t cueCount = 0;
};

/// How many cards the intro has.
constexpr std::size_t INTRO_CARD_COUNT = 5;

/// The script: the five cards in their order.
const std::array<IntroCard, INTRO_CARD_COUNT>& introCards();

/// How long the whole intro takes, in seconds: the sum of the cards. The file of the
/// wind (SoundCue::IntroWind) is exactly this long, because the game cannot fade
/// a sound: tools/make_sounds.py names the same number (INTRO_SECONDS).
float introSeconds();

/// The two lines of a card as they are shown.
struct IntroLines {
    const char* first = "";
    const char* second = "";
};

/// The text of card number card (0 to INTRO_CARD_COUNT - 1). shadeInGame tells whether
/// the shadow, the enemy of the game, takes part in it. The fourth card warns of it
/// ("Something walks where it is not."). In a game without the shadow that line would
/// promise something the player never meets, so the card then says what an unlit lamp
/// really costs ("It forgets quickly."). Throws std::out_of_range for a number that is
/// no card.
IntroLines introLines(std::size_t card, bool shadeInGame);

/// The cell in which the picture of card number card shows the shade: at the far end of
/// the corridor the camera of the fourth card walks down, where the flashlight of the
/// camera reaches it. Empty for a card without the shade, and for every card when
/// shadeInGame is false: in a calm night the corridor stays empty, like the mazes of the
/// game. Throws std::out_of_range for a number that is no card.
std::optional<MazeCell> introShadeCell(std::size_t card, bool shadeInGame);

/// How a card comes and goes, in seconds. After the cut to its picture the text waits
/// for INTRO_TEXT_DELAY_SECONDS, fades in over INTRO_TEXT_FADE_SECONDS, stands, fades
/// out over the same time and is gone INTRO_TEXT_GAP_SECONDS before the next cut. So
/// a cut never happens under text, and the eye has a moment for the new picture alone.
constexpr float INTRO_TEXT_DELAY_SECONDS = 0.2F;
constexpr float INTRO_TEXT_FADE_SECONDS = 0.7F;
constexpr float INTRO_TEXT_GAP_SECONDS = 0.3F;

/// How long the black takes to lift from the first picture, and to come down over the
/// last one at the end of the intro, in seconds.
constexpr float INTRO_BLACK_FADE_SECONDS = 1.2F;

/// The hint "Press any key to skip": it fades in over the first
/// INTRO_HINT_FADE_SECONDS, stands until INTRO_HINT_SECONDS after the start and fades
/// out over INTRO_HINT_FADE_SECONDS. A hint that stayed would be read with every card.
constexpr float INTRO_HINT_SECONDS = 4.0F;
constexpr float INTRO_HINT_FADE_SECONDS = 0.5F;

/// How much of the text of a card is there, cardSeconds after the cut to the card, for
/// a card that takes seconds: 0 during the delay, up along a straight line, 1 for
/// a while, down again and 0 for the gap at the end. The cards of the intro use it, and
/// so do the other cards of text of the game (game/Campaign.hpp).
float cardTextOpacity(float cardSeconds, float seconds);

/// How much of the hint "Press any key to skip" is there, seconds after the cards
/// began (INTRO_HINT_SECONDS and INTRO_HINT_FADE_SECONDS).
float skipHintOpacity(float seconds);

/// A key or a mouse button skips the intro only after this many seconds. The key that
/// started the game (Enter in the launcher, the click on its button) may still be down
/// when the window opens, and it must not skip what the player has not seen yet.
constexpr float INTRO_SKIP_DELAY_SECONDS = 0.5F;

/// What the intro shows at one moment.
struct IntroFrame {
    /// True when the moment lies at or past the end of the intro. The other fields then
    /// describe its last moment: the last card, no text, all black.
    bool finished = false;

    /// The card of this moment, 0 to INTRO_CARD_COUNT - 1, and the seconds since the cut
    /// to it.
    std::size_t card = 0;
    float cardSeconds = 0.0F;

    /// How much of the text is there: 0 is none, 1 is all of it.
    float textOpacity = 0.0F;

    /// How much black lies over the picture: 1 hides it completely.
    float blackOpacity = 1.0F;

    /// How much of the skip hint is there.
    float hintOpacity = 0.0F;

    /// True when a key may skip the intro at this moment (INTRO_SKIP_DELAY_SECONDS).
    bool skippable = false;
};

/// The intro seconds after it began. A negative number counts as 0.
IntroFrame introFrame(float seconds);

/// Where the camera of the intro is and whether its flashlight is on, for a frame of
/// the intro (introFrame). The settings are the ones a fresh menu camera has (speed,
/// eye height): the shots must not change with what a debug panel or the command line
/// did to the menu camera of the game. menuSeconds is the moment in the loop of the
/// shot: hand both to game::menuCameraPose.
struct IntroCamera {
    MenuCameraSettings settings;
    float menuSeconds = 0.0F;
    bool flashlightOn = false;
};
IntroCamera introCamera(const IntroFrame& frame);

/// The sounds whose moments lie in the stretch of time from before (included) to after
/// (not included), both in seconds since the intro began, in the order of the script.
/// The application calls it once per frame with the clock before and after the frame:
/// every moment then lies in exactly one stretch, so every sound is played exactly once,
/// however long or short the frames are. A sound at second 0 belongs to the first
/// stretch that starts at 0. Nothing lies past the end of the intro.
std::vector<SoundCue> introCuesBetween(float before, float after);

} // namespace game
