// RoundMusic: the quiet music under a round, the layer of the chase and the sting of a
// won night, and the rules that decide when each is heard.
#pragma once

#include "game/GameState.hpp"
#include "game/Round.hpp"
#include "game/Shade.hpp"
#include "game/SoundCues.hpp"

#include <cstddef>

namespace game {

// Plain data and pure functions like SoundCues.hpp: nothing here plays a sound. The
// application asks the rules once per frame and hands the answers to
// audio::AudioEngine. All three pieces are in the music group, so the music slider of
// the settings screen sets them, and they stay well under the wind and the sounds of
// the shade, which carry the tension.

/// The files, relative to the assets directory. Recordings, not generated
/// (assets/audio/README.md).
constexpr const char* MAZE_AMBIENT_FILE = "audio/maze_ambient.flac";
constexpr const char* SHADE_CHASE_FILE = "audio/shade_chase.flac";
constexpr const char* NIGHT_END_FILE = "audio/night_end.flac";

/// Their places in the list of sounds the audio layer loads: right after the theme of
/// the menu.
constexpr std::size_t MAZE_AMBIENT_SOUND = MENU_THEME_SOUND + 1;
constexpr std::size_t SHADE_CHASE_SOUND = MENU_THEME_SOUND + 2;
constexpr std::size_t NIGHT_END_SOUND = MENU_THEME_SOUND + 3;

/// How many sounds the fixed part of the list has: the cues, the theme and these three.
constexpr std::size_t FIXED_SOUND_COUNT = NIGHT_END_SOUND + 1;

/// The music bed of a round comes this long after the round starts, so a night opens
/// with the wind alone, and then fades in slowly. It goes faster: a pause or a catch
/// should be quiet soon.
constexpr float MAZE_AMBIENT_DELAY_SECONDS = 4.0F;
constexpr float MAZE_AMBIENT_FADE_IN_SECONDS = 6.0F;
constexpr float MAZE_AMBIENT_FADE_OUT_SECONDS = 1.5F;

/// The layer of the chase comes in over 2 s when the shade starts to hunt and goes over
/// 4 s when it gives up or is burned away: the danger is quick, the relief slow.
constexpr float SHADE_CHASE_FADE_IN_SECONDS = 2.0F;
constexpr float SHADE_CHASE_FADE_OUT_SECONDS = 4.0F;

/// Under the chase the music bed is ducked to this volume (about -8 dB), and it takes
/// MAZE_AMBIENT_DUCK_SECONDS to go the whole way down or up.
constexpr float MAZE_AMBIENT_DUCKED_VOLUME = 0.4F;
constexpr float MAZE_AMBIENT_DUCK_SECONDS = 2.0F;

/// The sting of a won night starts this long after the player walks through the gate:
/// the music bed has faded out by then, and the toll of the bell has struck. Leaving the
/// result screen fades it out over NIGHT_END_FADE_OUT_SECONDS.
constexpr float NIGHT_END_DELAY_SECONDS = 1.5F;
constexpr float NIGHT_END_FADE_OUT_SECONDS = 2.0F;

/// What decides whether the music of a round is heard.
struct RoundMusicRequest {
    /// The screen the game is on.
    GameMode mode = GameMode::MainMenu;
    /// The window of the game is the active one.
    bool windowFocused = true;
    /// The menu camera shows the game: the round stands still under it.
    bool menuCamera = false;
    /// The shade has caught the player and the picture fades to black.
    bool catching = false;
};

/// True while the shade hunts the player: it walks to a noise or chases them, also
/// while the lamp holds it for a moment in the middle of that. Not while it wanders,
/// stands in its grace time, or is quiet after a banish. Asked from the hunt and not from
/// shadeState, which says "lit" before "chasing": a lamp on a chasing shade must not
/// switch the layer off and on.
bool shadeHunts(const Shade& shade);

/// True while the music bed is heard: a round is played (and still running), the window
/// is active, the menu camera is off, no catch fades to black, and the round has run
/// MAZE_AMBIENT_DELAY_SECONDS. So it fades out in the pause and its menus, on the result
/// screen, in a catch, and comes back after the start of the next round.
bool mazeAmbientPlays(const RoundMusicRequest& request, const Round& round);

/// True while the layer of the chase is heard: as mazeAmbientPlays without the delay,
/// and the shade of the round hunts (shadeHunts).
bool shadeChasePlays(const RoundMusicRequest& request, const Round& round);

/// The volume of the music bed one frame later: it moves from volume towards
/// MAZE_AMBIENT_DUCKED_VOLUME while ducked and towards 1 otherwise, the whole way in
/// MAZE_AMBIENT_DUCK_SECONDS, and never past its target.
float advanceAmbientDuck(float volume, bool ducked, float frameSeconds);

/// True for the screens that follow a won night: the result screen, and after the last
/// night the village and the ending card. The sting is heard only there. Every screen is
/// named in the code, like menuThemePlays.
bool nightEndScreen(GameMode mode);

/// The clock of the sting: seconds until it starts, or less than 0 when none waits.
struct NightEndSting {
    float secondsLeft = -1.0F;
};

/// Arms the clock: the night was won in this step.
void armNightEndSting(NightEndSting& sting);

/// Advances the clock by one frame. Returns true in the one frame the sting is due. A
/// sting that is still waiting when the screen is left (a fast "Next night") is dropped.
bool advanceNightEndSting(NightEndSting& sting, GameMode mode, float frameSeconds);

} // namespace game
