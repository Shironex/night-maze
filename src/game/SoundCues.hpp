// SoundCues: which short sounds the game plays and the rules that decide when.
#pragma once

#include "game/GameState.hpp"
#include "game/Round.hpp"

#include <cstddef>
#include <vector>

namespace game {

// Plain data and pure functions without a sound card and without the audio library,
// like the rest of the game_logic library, so tests can check every rule. Nothing here
// plays a sound: the functions only say WHICH sound belongs to what happened. The
// application (NightMazeApp) asks them and hands the answers to audio::AudioEngine,
// which knows files and numbers and nothing about the game.

/// One short sound of the game. The numbers are the places of the cues in the table of
/// their files (soundCueFile) and in the list the audio layer loads, so they start at
/// 0 and have no gaps.
enum class SoundCue {
    FlashlightOn = 0, ///< the flashlight key switched the light on
    FlashlightOff,    ///< the flashlight key switched the light off
    /// The light did not come on or went out because the battery is empty: the key was
    /// pressed on an empty battery, or the battery ran out with the light on.
    FlashlightDead,
    LowBatteryPulse, ///< the warning that repeats while the light runs on a low battery
    CrystalPickup,   ///< a crystal was collected
    LeverPull,       ///< a lever was pulled and its wall starts to sink
    GateOpen,        ///< enough crystals are collected: the gate starts to sink
    WindedBreath,    ///< the breath that repeats while the player is winded
    FlaskPickup,     ///< a flask of tea was picked up
    ShadeNear,       ///< the hum that repeats while the shade is near
    Caught,          ///< the shade reached the player and carries them back to the start
    /// The wind under the intro: one long file, as long as the intro itself, with its
    /// fade in and its fade out inside the file (game/Intro.hpp).
    IntroWind,
    IntroBell, ///< one far bell of the village, in the intro
    /// A step of the player: three sounds of the same kind, used in turn
    /// (advanceFootsteps), so walking does not sound like a machine.
    Footstep1,
    Footstep2,
    Footstep3,
    /// A step of the shade: two soft sounds, used in turn (advanceShadeSteps).
    ShadeStep1,
    ShadeStep2,
    /// The wind of the maze at night. The only cue that is a LOOP: it is not played
    /// once, it is switched on and off (mazeWindPlays), and its file ends where it
    /// begins.
    MazeWind,
};

/// How many cues there are: the number of entries of SoundCue.
constexpr std::size_t SOUND_CUE_COUNT = 19;

/// The place of a cue in the list of sounds: its number as an index.
std::size_t soundCueIndex(SoundCue cue);

/// The sound file of a cue, relative to the assets directory (core::assetPath), for
/// example "audio/crystal_pickup.wav". Every cue has a file of its own. This table is
/// the one place where the names are written.
const char* soundCueFile(SoundCue cue);

/// A cue in words, for the debug window: "flashlight on", "gate open".
const char* soundCueName(SoundCue cue);

/// True for a cue that is part of the air of a place and tells the player nothing: the
/// two winds. Such a cue follows the ambient volume of the settings
/// (GameSettings::ambientVolume), every other cue the effects volume.
bool soundCueIsAmbient(SoundCue cue);

/// The cue of one press of the flashlight key. battery is the charge at the moment of
/// the press and wasOn the switch before it. An empty battery gives FlashlightDead,
/// whatever the switch says: the light cannot come on. Otherwise the switch flips, and
/// the cue is the one of its new state.
SoundCue flashlightKeyCue(float battery, bool wasOn);

/// What the cues of a fixed step are decided from: a few numbers of the round, copied
/// BEFORE the step. Comparing them with the round after the step tells what the step
/// did, so game::updateRound needs no extra way to report it.
struct RoundSoundSnapshot {
    int collectedCount = 0;    ///< Round::collectedCount
    int flasksCollected = 0;   ///< Round::flasksCollected
    bool gateOpen = false;     ///< Round::gateOpen
    float battery = 1.0F;      ///< Round::battery
    bool flashlightOn = false; ///< the switch of the flashlight
};

/// The snapshot of a round as it is now. flashlightOn is the switch of the flashlight
/// (LightingSettings::flashlightOn), which is not part of the round.
RoundSoundSnapshot soundSnapshot(const Round& round, bool flashlightOn);

/// The cues of one fixed step: before is the snapshot taken right before
/// game::updateRound, round and flashlightOn are the round and the switch after it.
///
///   - CrystalPickup when more crystals are collected than before. Once per step, also
///     when the step collected two crystals: two copies of one sound at the same moment
///     are only one louder sound.
///   - GateOpen when the gate was closed before and is open now.
///   - FlashlightDead when the battery ran out in this step with the light on: it held
///     a charge before, it is empty now, and the switch went from on to off.
///   - FlaskPickup when more flasks are picked up than before, once per step like the
///     crystal. It has a sound of its own, so a flask is never taken for a crystal.
///
/// The list is empty for most steps. Its order is the order above.
///
/// The snapshot has to belong to the same round. A round that was started again must
/// not be compared with a snapshot of the round before: take the snapshot right before
/// every step, as the application does, and a fresh round fires nothing (also one that
/// starts with its gate open, in a maze without crystals or without a gate).
std::vector<SoundCue> roundStepCues(const RoundSoundSnapshot& before, const Round& round,
                                    bool flashlightOn);

/// How long the low battery pulse waits between two beats, in seconds, at the two ends
/// of the low range: slow right below the threshold (GameplaySettings::lowBatteryThreshold),
/// fast just before the battery is empty. In between the wait shrinks evenly with the
/// charge. A calm heartbeat that becomes a hurried one.
constexpr float LOW_BATTERY_PULSE_SLOW_SECONDS = 2.0F;
constexpr float LOW_BATTERY_PULSE_FAST_SECONDS = 0.8F;

/// True while the battery is low: above empty and below the threshold of the settings.
/// The same range in which the light flickers (game::flashlightFlicker). With
/// a threshold of 0 it is never true.
bool batteryIsLow(float battery, const GameplaySettings& settings);

/// True while the low battery pulse sounds: the round is being played, the flashlight
/// is on and the battery is low (batteryIsLow). With the light off nothing drains and
/// there is nothing to warn about, and an empty battery is silent like its light.
bool lowBatteryPulseSounds(const Round& round, bool flashlightOn, const GameplaySettings& settings);

/// The wait between two beats for a charge, in seconds: LOW_BATTERY_PULSE_SLOW_SECONDS
/// at the threshold, LOW_BATTERY_PULSE_FAST_SECONDS at empty. A charge outside of the
/// low range gives the slow wait.
float lowBatteryPulseInterval(float battery, const GameplaySettings& settings);

/// The clock of the low battery pulse. It counts in fixed steps of the simulation, not
/// in frames, so the beats come at the same moments at every frame rate.
struct LowBatteryPulse {
    /// Seconds until the next beat. 0 means that the next step in which the pulse
    /// sounds beats at once: the first beat comes at the moment the battery gets low.
    float secondsToNextBeat = 0.0F;
};

/// Advances the clock of the pulse by one fixed step of stepSeconds seconds, for the
/// round and the switch as they are AFTER game::updateRound. Returns true when a beat
/// is due in this step: play SoundCue::LowBatteryPulse then.
///
///   - While the battery is not low (charged above the threshold by a crystal, or
///     empty) the clock is reset: the next time the battery gets low, the first beat
///     comes at once.
///   - While the battery is low but the pulse does not sound (the light is off, the
///     round is won) the clock stands still.
///   - Otherwise it runs. After a beat the wait starts again from the interval of the
///     present charge (lowBatteryPulseInterval).
///
/// One call gives at most one beat, however long the step is: the wait is set anew
/// after a beat and not counted on, so no time is carried over that could become
/// a second beat. A new round resets the clock by assigning a new LowBatteryPulse.
bool advanceLowBatteryPulse(LowBatteryPulse& pulse, const Round& round, bool flashlightOn,
                            const GameplaySettings& settings, float stepSeconds);

/// How long a winded player waits between two breaths, in seconds. The heavy breathing
/// is one breath played again and again on a clock, like the low battery pulse, and not
/// a loop of the audio layer: it has to stop with the breath that is running. The wait is
/// a little longer than the sound of the breath (0.95 s, tools/make_sounds.py): a sound
/// that starts again before it ended is cut off.
constexpr float WINDED_BREATH_SECONDS = 1.1F;

/// The clock of the breathing of a winded player, counted in fixed steps like
/// LowBatteryPulse.
struct WindedBreath {
    /// Seconds until the next breath. 0 means that the next step in which the player
    /// is winded breathes at once.
    float secondsToNextBreath = 0.0F;
};

/// Advances the clock of the breathing by one fixed step of stepSeconds seconds. winded
/// is Stamina::winded after the step (game/Player.hpp). Returns true when a breath is
/// due in this step: play SoundCue::WindedBreath then.
///
///   - While the player is not winded the clock is reset: the next time the stamina
///     runs out, the first breath comes at once.
///   - Otherwise it runs, and after a breath the wait is WINDED_BREATH_SECONDS.
///
/// One call gives at most one breath, however long the step is, for the same reason as
/// advanceLowBatteryPulse. A new round resets the clock by assigning a new WindedBreath.
bool advanceWindedBreath(WindedBreath& breath, bool winded, float stepSeconds);

/// The shade is heard before it is seen: a low hum, again and again, the faster the
/// nearer the shade is. It is the same kind of clock as the low battery pulse: the hum
/// is always as loud, and what grows is how often it comes.
///
/// SHADE_HUM_DISTANCE: from this many metres the hum is heard, measured along the
/// passages the shade has to walk (Shade::wayMetres, seven cells of the maze), not in
/// a straight line: a shade behind the next wall with a long way round is not a danger
/// yet, and one that is coming down the corridor is. Farther away there is silence. The two waits
/// are the ones at that distance and at no distance at all. In between the wait shrinks evenly. The
/// short one is a little longer than the sound of the hum (0.62 s, tools/make_sounds.py): a sound
/// that starts again before it ended is cut off.
constexpr float SHADE_HUM_DISTANCE = 14.0F;
constexpr float SHADE_HUM_SLOW_SECONDS = 2.4F;
constexpr float SHADE_HUM_FAST_SECONDS = 0.7F;

/// True while the hum sounds: the round is being played, it has a shade, and the way of
/// the shade to the player is not longer than SHADE_HUM_DISTANCE. A calm round is
/// silent. The hum does not ask whether the shade moves: a shade that stands in the
/// light is as near as one that walks.
bool shadeHumSounds(const Round& round);

/// The wait between two hums for a distance in metres: SHADE_HUM_FAST_SECONDS at 0,
/// SHADE_HUM_SLOW_SECONDS at SHADE_HUM_DISTANCE and beyond.
float shadeHumInterval(float distance);

/// The clock of the hum, counted in fixed steps like LowBatteryPulse.
struct ShadeHum {
    /// Seconds until the next hum. 0 means that the next step in which the hum sounds
    /// hums at once: the first one comes at the moment the shade gets near.
    float secondsToNextHum = 0.0F;
};

/// Advances the clock of the hum by one fixed step of stepSeconds seconds, for the round
/// as it is AFTER the step of the shade (game::updateRoundShade). Returns true when a hum is due
/// in this step: play SoundCue::ShadeNear then.
///
///   - While the hum does not sound (shadeHumSounds) the clock is reset.
///   - Otherwise it runs, and after a hum the wait is the interval of the present
///     distance (shadeHumInterval).
///
/// One call gives at most one hum, for the same reason as advanceLowBatteryPulse. A new
/// round resets the clock by assigning a new ShadeHum.
bool advanceShadeHum(ShadeHum& hum, const Round& round, float stepSeconds);

/// A sound together with how loud it is played: 1 is the file as it is, 0.5 half of its
/// amplitude. The audio layer takes the number with every play, so one file serves for
/// a near step and for a far one.
struct CuePlay {
    SoundCue cue = SoundCue::Footstep1;
    float volume = 1.0F;
};

/// The steps of the player are counted in METRES WALKED and not in seconds: one step
/// every FOOTSTEP_WALK_METRES. So they come faster when the player is faster, slower
/// when a wall takes a part of the movement away, and not at all when the player
/// stands or pushes straight against a wall. At the walking speed of 3 m/s that is two
/// steps per second. A sprint takes longer steps (FOOTSTEP_SPRINT_METRES): at 5.5 m/s
/// two and three quarters per second, faster than walking and still not a rattle.
constexpr float FOOTSTEP_WALK_METRES = 1.5F;
constexpr float FOOTSTEP_SPRINT_METRES = 2.0F;

/// How loud a step is played. A sprinted step is the file as it is and a walked one is
/// softer: the heavier step of a sprint is the same sound, louder.
constexpr float FOOTSTEP_WALK_VOLUME = 0.6F;
constexpr float FOOTSTEP_SPRINT_VOLUME = 1.0F;

/// How many sounds the steps of the player and of the shade are chosen from.
constexpr int FOOTSTEP_SOUND_COUNT = 3;
constexpr int SHADE_STEP_SOUND_COUNT = 2;

/// The clock of the steps of somebody who walks: the player has one and the shade has
/// one. It counts metres, in fixed steps of the simulation.
struct StepClock {
    /// Metres still to walk until the next step sounds. A new clock starts with half
    /// a step, so the first one comes soon after the walker sets off.
    float metresToNextStep = FOOTSTEP_WALK_METRES / 2.0F;

    /// Which of the sounds the next step plays, counted from 0. They are used in turn.
    int nextSound = 0;
};

/// What the clock of the player is told about one fixed step.
struct FootstepInput {
    /// How far the feet moved over the ground in this step, in metres: the distance
    /// between the positions before and after Player::update, without the height.
    float metres = 0.0F;
    /// The length of the step in seconds, to know the speed.
    float stepSeconds = 0.0F;
    /// Player::walkSpeed and Player::sprintSpeed. A step counts as sprinted when the
    /// feet really moved faster than half way between the two: a player who sprints
    /// into a wall and only slides along it slowly makes the sound of walking.
    float walkSpeed = 0.0F;
    float sprintSpeed = 0.0F;
    /// True in noclip mode: a player who flies has no feet on the ground.
    bool flying = false;
};

/// Advances the clock of the steps of the player by one fixed step. Returns true when
/// a step is due, with its sound and its volume in play.
///
///   - Flying: silent, and the clock is set back to a new one.
///   - Not moving (standing, reading the map, pushing against a wall): the clock stands
///     still, so the way walked so far is kept.
///   - Otherwise the metres of the step are taken off the way. When none is left, the
///     next of the FOOTSTEP_SOUND_COUNT sounds is due, and the way is set to
///     FOOTSTEP_WALK_METRES, or to FOOTSTEP_SPRINT_METRES after a sprinted step.
///
/// One call gives at most one step, however far the feet moved (the debug UI can put
/// the player anywhere), for the same reason as advanceLowBatteryPulse. A new round
/// resets the clock by assigning a new StepClock.
bool advanceFootsteps(StepClock& clock, const FootstepInput& input, CuePlay& play);

/// The shade is heard walking, as a second sign next to its hum: soft steps, slower
/// than the ones of the player, and only from near by.
///
/// SHADE_STEP_DISTANCE: the steps are heard when the way of the shade to the player
/// (Shade::wayMetres, along the passages like the hum) is not longer than this. The hum
/// starts at 14 m, so the steps come later: first "it is near", then "it is walking".
/// SHADE_STEP_METRES: one step per this many metres. The shade walks 4 m/s, which gives
/// one and a half steps per second, clearly slower than the player.
/// The two volumes are the ones right next to the player and at SHADE_STEP_DISTANCE. In
/// between the volume falls evenly with the way.
constexpr float SHADE_STEP_DISTANCE = 9.0F;
constexpr float SHADE_STEP_METRES = 2.6F;
constexpr float SHADE_STEP_NEAR_VOLUME = 1.0F;
constexpr float SHADE_STEP_FAR_VOLUME = 0.3F;

/// How far the shade walked over the ground in its last step, in metres: the distance
/// between Shade::previousPosition and Shade::position, without the height. 0 while it
/// stands: in its grace time, in the light and for a moment after the light has left
/// it. 0 for a round without a shade.
float shadeStepMetres(const Round& round);

/// How loud a step of the shade is played for a way in metres: SHADE_STEP_NEAR_VOLUME at
/// 0, SHADE_STEP_FAR_VOLUME at SHADE_STEP_DISTANCE and beyond.
float shadeStepVolume(float wayMetres);

/// Advances the clock of the steps of the shade by one fixed step, for the round as it
/// is AFTER the step of the shade (game::updateRoundShade). Returns true when a step is
/// heard, with its sound and its volume in play.
///
///   - While the round is not played or has no shade the clock is set back to a new
///     one.
///   - While the shade stands (shadeStepMetres is 0) the clock stands still: a shade
///     that is held by the light makes no sound of walking.
///   - While it walks the metres are taken off the way, near or far. A step that is due
///     is HEARD only when the way to the player is not longer than SHADE_STEP_DISTANCE.
///
/// One call gives at most one step, like advanceFootsteps.
bool advanceShadeSteps(StepClock& clock, const Round& round, CuePlay& play);

/// How long the wind of the maze takes to come and to go, in seconds. A loop that is
/// switched on or off at once makes the loudspeaker jump, which is heard as a click, so
/// the audio layer fades it. Going is faster than coming: a menu should be quiet soon.
constexpr float MAZE_WIND_FADE_IN_SECONDS = 1.5F;
constexpr float MAZE_WIND_FADE_OUT_SECONDS = 0.6F;

/// How long the wind is heard after the ambient slider of the settings screen was
/// moved, so the player hears the level that was just chosen, and how fast it comes in
/// then.
constexpr float MAZE_WIND_SAMPLE_SECONDS = 1.5F;
constexpr float MAZE_WIND_SAMPLE_FADE_IN_SECONDS = 0.25F;

/// What decides whether the wind of the maze is heard.
struct MazeWindRequest {
    /// The screen the game is on.
    GameMode mode = GameMode::MainMenu;
    /// The window of the game is the active one.
    bool windowFocused = true;
    /// The menu camera shows the game (MenuCameraSettings::enabled): the round stands
    /// still under it.
    bool menuCamera = false;
    /// A sample of the wind was asked for and still runs: the ambient slider of the
    /// settings screen was moved, or the button of the debug window was pressed.
    bool sample = false;
};

/// True while the wind of the maze is heard. The application asks once per frame and
/// tells the audio layer the answer, which fades the loop in or out when it changes.
///
///   - Never during the intro: the intro has a wind of its own, and the two must not be
///     heard together. Never over the title card of a night or the ending card either:
///     a card is text on black, and the round has not started or is over
///     (game::isFilm).
///   - Never while the window is not the active one.
///   - While a round is played (game::updatesRound) and the menu camera is off. So the
///     wind fades out in the pause menu, on the result screen, in the main menu and
///     its pages (free play, the list of nights) and on the settings screen, and comes
///     back with the round.
///   - On every other screen only while a sample runs.
bool mazeWindPlays(const MazeWindRequest& request);

} // namespace game
