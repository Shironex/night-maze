// SoundCues: which short sounds the game plays and the rules that decide when.
// See docs/modules/game/sound-cues.md
#include "game/SoundCues.hpp"

#include <algorithm>
#include <array>

namespace game {

namespace {

// What is written down about one cue.
struct SoundCueInfo {
    const char* file; // relative to the assets directory
    const char* name; // for the debug window
};

// The table of the cues, in the order of the enum SoundCue: entry number i belongs to
// the cue with the number i. The array has exactly SOUND_CUE_COUNT entries, so a cue
// added to the enum and to the count without a line here leaves an entry with two null
// pointers, which the tests find (tests/SoundCueTests.cpp).
constexpr std::array<SoundCueInfo, SOUND_CUE_COUNT> SOUND_CUES = {{
    {.file = "audio/flashlight_on.wav", .name = "flashlight on"},
    {.file = "audio/flashlight_off.wav", .name = "flashlight off"},
    {.file = "audio/flashlight_dead.wav", .name = "flashlight dead"},
    {.file = "audio/low_battery_pulse.wav", .name = "low battery pulse"},
    {.file = "audio/crystal_pickup.wav", .name = "crystal pickup"},
    {.file = "audio/lever_pull.wav", .name = "lever pull"},
    {.file = "audio/gate_open.wav", .name = "gate open"},
    {.file = "audio/winded_breath.wav", .name = "winded breath"},
    {.file = "audio/flask_pickup.wav", .name = "flask pickup"},
    {.file = "audio/shade_near.wav", .name = "shade near"},
    {.file = "audio/caught.wav", .name = "caught"},
    {.file = "audio/intro_wind.wav", .name = "intro wind"},
    {.file = "audio/intro_bell.wav", .name = "intro bell"},
}};

} // namespace

std::size_t soundCueIndex(SoundCue cue) {
    return static_cast<std::size_t>(cue);
}

const char* soundCueFile(SoundCue cue) {
    // at() checks the number: a value that is no cue throws instead of reading past the
    // table.
    return SOUND_CUES.at(soundCueIndex(cue)).file;
}

const char* soundCueName(SoundCue cue) {
    return SOUND_CUES.at(soundCueIndex(cue)).name;
}

SoundCue flashlightKeyCue(float battery, bool wasOn) {
    // An empty battery: the switch moves, but no light comes. The switch itself is
    // already off then (game::updateRound holds it off), so wasOn does not matter.
    if (battery <= 0.0F) {
        return SoundCue::FlashlightDead;
    }
    return wasOn ? SoundCue::FlashlightOff : SoundCue::FlashlightOn;
}

RoundSoundSnapshot soundSnapshot(const Round& round, bool flashlightOn) {
    return {.collectedCount = round.collectedCount,
            .flasksCollected = round.flasksCollected,
            .gateOpen = round.gateOpen,
            .battery = round.battery,
            .flashlightOn = flashlightOn};
}

std::vector<SoundCue> roundStepCues(const RoundSoundSnapshot& before, const Round& round,
                                    bool flashlightOn) {
    // An empty vector holds no memory, so the many steps without a cue cost nothing.
    std::vector<SoundCue> cues;

    // "More than before" and not "different": a count that went down belongs to another
    // round, and that is not a pickup.
    if (round.collectedCount > before.collectedCount) {
        cues.push_back(SoundCue::CrystalPickup);
    }
    if (!before.gateOpen && round.gateOpen) {
        cues.push_back(SoundCue::GateOpen);
    }
    // The battery ran out in THIS step. The charge before is asked for, and not only
    // the switch: after the flashlight key is pressed on an empty battery the switch is
    // on for a moment, and the next step turns it off again. That press already played
    // its own FlashlightDead (flashlightKeyCue), and the step must not play it a second
    // time. A crystal collected in the same step charges the battery before this is
    // asked, so the light it saved makes no sound of dying.
    if (before.flashlightOn && before.battery > 0.0F && round.battery <= 0.0F && !flashlightOn) {
        cues.push_back(SoundCue::FlashlightDead);
    }
    // "More than before", for the same reason as with the crystals.
    if (round.flasksCollected > before.flasksCollected) {
        cues.push_back(SoundCue::FlaskPickup);
    }
    return cues;
}

bool batteryIsLow(float battery, const GameplaySettings& settings) {
    return battery > 0.0F && battery < settings.lowBatteryThreshold;
}

bool lowBatteryPulseSounds(const Round& round, bool flashlightOn,
                           const GameplaySettings& settings) {
    return round.state == RoundState::Playing && flashlightOn &&
           batteryIsLow(round.battery, settings);
}

float lowBatteryPulseInterval(float battery, const GameplaySettings& settings) {
    // Also covers a threshold of 0, so the division below is safe.
    if (!batteryIsLow(battery, settings)) {
        return LOW_BATTERY_PULSE_SLOW_SECONDS;
    }
    // How weak the battery is: 0 at the threshold, 1 when it is empty. The same number
    // the flicker of the light is computed from (game::flashlightFlicker).
    const float weakness = 1.0F - battery / settings.lowBatteryThreshold;
    return LOW_BATTERY_PULSE_SLOW_SECONDS +
           (LOW_BATTERY_PULSE_FAST_SECONDS - LOW_BATTERY_PULSE_SLOW_SECONDS) * weakness;
}

bool advanceLowBatteryPulse(LowBatteryPulse& pulse, const Round& round, bool flashlightOn,
                            const GameplaySettings& settings, float stepSeconds) {
    // Not low: charged again or empty. The clock goes back to "beat at once", for the
    // next time the battery gets low.
    if (!batteryIsLow(round.battery, settings)) {
        pulse.secondsToNextBeat = 0.0F;
        return false;
    }
    // Low, but silent: the light is off or the round is over. The clock waits where it
    // is, so switching the light off and on again does not bring an extra beat.
    if (!lowBatteryPulseSounds(round, flashlightOn, settings)) {
        return false;
    }

    pulse.secondsToNextBeat -= stepSeconds;
    if (pulse.secondsToNextBeat > 0.0F) {
        return false;
    }
    // A beat. The wait is SET to the interval, and what the step overshot is dropped:
    // adding the interval instead would keep the beats exact to the step, but after
    // a very long step it would leave a negative wait and the next steps would beat
    // one after the other until it is used up.
    pulse.secondsToNextBeat = lowBatteryPulseInterval(round.battery, settings);
    return true;
}

bool advanceWindedBreath(WindedBreath& breath, bool winded, float stepSeconds) {
    // Not winded: the clock goes back to "breathe at once", for the next time.
    if (!winded) {
        breath.secondsToNextBreath = 0.0F;
        return false;
    }

    breath.secondsToNextBreath -= stepSeconds;
    if (breath.secondsToNextBreath > 0.0F) {
        return false;
    }
    // A breath. The wait is SET and what the step overshot is dropped, as in
    // advanceLowBatteryPulse.
    breath.secondsToNextBreath = WINDED_BREATH_SECONDS;
    return true;
}

bool shadeHumSounds(const Round& round) {
    return round.state == RoundState::Playing && round.shade.present &&
           round.shade.wayMetres <= SHADE_HUM_DISTANCE;
}

float shadeHumInterval(float distance) {
    // How far away the shade is as a part of the distance it is heard from: 0 when it
    // is here, 1 at the edge of hearing and beyond.
    const float share = std::clamp(distance / SHADE_HUM_DISTANCE, 0.0F, 1.0F);
    return SHADE_HUM_FAST_SECONDS + (SHADE_HUM_SLOW_SECONDS - SHADE_HUM_FAST_SECONDS) * share;
}

bool advanceShadeHum(ShadeHum& hum, const Round& round, float stepSeconds) {
    // Not near, or no shade at all: the clock goes back to "hum at once".
    if (!shadeHumSounds(round)) {
        hum.secondsToNextHum = 0.0F;
        return false;
    }

    hum.secondsToNextHum -= stepSeconds;
    if (hum.secondsToNextHum > 0.0F) {
        return false;
    }
    // A hum. The wait is SET and what the step overshot is dropped, as in
    // advanceLowBatteryPulse.
    hum.secondsToNextHum = shadeHumInterval(round.shade.wayMetres);
    return true;
}

} // namespace game
