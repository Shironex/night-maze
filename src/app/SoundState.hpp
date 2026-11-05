// What the application plays: the sound device, and what it remembers about the cues and
// the loops it has started.
#pragma once

#include "audio/AudioEngine.hpp"
#include "game/sound/RoundMusic.hpp"

namespace game {

/// The audio engine with its sounds and the state around it. The functions of
/// NightMazeApp.cpp (playCue, updateAmbience, updateRoundMusic) use it; the debug UI reads
/// it.
struct SoundState {
    /// The sound device with the sounds of the cues, loaded once in the constructor of the
    /// application in the order of game::SoundCue. Without a device it does nothing
    /// (audio::AudioEngine).
    audio::AudioEngine audio;
    /// What playCue remembers for the debug UI: the name of the last cue (a text of the
    /// cue table, which lives as long as the program) and the number of cues so far.
    const char* lastCueName = "none";
    int cuesPlayed = 0;
    /// How loud that last cue was played (playCue).
    float lastCueVolume = 1.0F;
    /// Seconds for which a sample of the wind of the maze is still heard on a screen
    /// that has no wind (updateAmbience): set when the ambient slider of the settings
    /// screen moves. 0: no sample runs.
    double windSampleLeft = 0.0;
    /// Set by the debug UI: play a sample of the wind (windSampleRequest).
    bool windSampleRequested = false;
    /// False until updateAmbience has run once: the first picture of the program is
    /// drawn after that call, and the theme of the menu waits for it
    /// (game::MenuThemeRequest::pictureShown).
    bool pictureShown = false;
    /// What updateAmbience last told the audio engine about the theme of the menu, to
    /// write one line into the log when it changes.
    bool menuThemeOn = false;
    /// The same for the music of a round (game/sound/RoundMusic.hpp): the bed, the chase layer
    /// and the sting of a won night, which plays once and is faded when its screen is left.
    bool mazeAmbientOn = false;
    bool shadeChaseOn = false;
    bool nightEndOn = false;
    /// The volume of the bed under the chase layer (game::advanceAmbientDuck): 1, or on
    /// its way to the ducked volume and back.
    float ambientDuck = 1.0F;
    /// The wait between the win and the sting (game::advanceNightEndSting).
    NightEndSting nightEndSting;
    /// Seconds until the volume slider of the settings screen may play its next sample
    /// click (handleControlChanges). 0: the next change is heard at once.
    double volumeSampleWait = 0.0;
};

} // namespace game
