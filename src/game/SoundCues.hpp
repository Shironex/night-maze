// SoundCues: which short sounds the game plays and the rules that decide when.
// See docs/modules/game/sound-cues.md
#pragma once

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
};

/// How many cues there are: the number of entries of SoundCue.
constexpr std::size_t SOUND_CUE_COUNT = 9;

/// The place of a cue in the list of sounds: its number as an index.
std::size_t soundCueIndex(SoundCue cue);

/// The sound file of a cue, relative to the assets directory (core::assetPath), for
/// example "audio/crystal_pickup.wav". Every cue has a file of its own. This table is
/// the one place where the names are written.
const char* soundCueFile(SoundCue cue);

/// A cue in words, for the debug window: "flashlight on", "gate open".
const char* soundCueName(SoundCue cue);

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

/// How long a winded player waits between two breaths, in seconds. The audio layer only
/// plays sounds from start to end and has no loops, so the heavy breathing is one
/// breath played again and again on a clock, like the low battery pulse. The wait is
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

} // namespace game
