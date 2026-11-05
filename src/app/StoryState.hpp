// The story told around the rounds: the clocks of the intro, of the cards of the nights and
// of the reveal of the village, and the storybook intro.
#pragma once

#include "game/land/Village.hpp"
#include "game/screens/Intro.hpp"
#include "game/screens/Storybook.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace game {

/// The number of a storybook sound that is not there (StoryState::storybookMusicSound).
constexpr std::size_t NO_STORYBOOK_SOUND = static_cast<std::size_t>(-1);

/// What the screens of the story keep between frames. The functions of
/// NightMazeAppScreens.cpp advance it; the frame reads it to place its camera.
struct StoryState {
    /// The intro. The clock counts the seconds since the intro began, in the real time of
    /// the frames like the clock of the menu camera: what is shown is a function of it
    /// (game::introFrame).
    double introSeconds = 0.0;
    /// The card whose two lines stand in the document: they are written when the card
    /// changes, not in every frame. INTRO_CARD_COUNT means none yet.
    std::size_t introCardShown = INTRO_CARD_COUNT;
    /// The clock of the title card of a night and of the ending card: seconds since the
    /// card came up (game::nightCardFrame, game::endingCardFrame). The two cards use the
    /// document of the intro.
    double cardSeconds = 0.0;
    /// The storybook intro (game/screens/Storybook.hpp): its manifest when the style of the run
    /// asks for it and the file can be read, the time of each picture (its spoken line
    /// can make it longer), the numbers of its sounds in the audio engine (NO_STORYBOOK_SOUND for
    /// none), and whether the intro that plays now is the storybook. It is not when its
    /// pictures cannot be loaded: the classic intro plays then.
    std::optional<Storybook> storybook;
    std::vector<float> storybookBeatSeconds;
    std::size_t storybookMusicSound = NO_STORYBOOK_SOUND;
    std::vector<std::size_t> storybookVoiceSounds;
    bool storybookPlaying = false;
    /// Set by the debug UI: play the intro again (introRequest).
    bool introRequested = false;

    /// The camera of the player at the moment the result of a night came up, and the
    /// seconds since then: what the reveal of the village starts from
    /// (game::villageRevealPose). The clock follows the real time of the frames.
    VillageRevealPose revealStart;
    double revealSeconds = 0.0;
};

} // namespace game
