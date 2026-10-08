// Tests of game/SoundCues.hpp: the table of the cues, the cue of the flashlight key, the
// cues of a fixed step (with the one of a flask), the clock of the low battery pulse,
// the clock of the breathing of a winded player, the hum and the steps of the shade, the
// steps of the player, the rule of the wind of the maze and the bell of the open gate.
#include "game/SoundCues.hpp"

#include "game/Exit.hpp"
#include "game/GameState.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "game/Terrain.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float. 120 steps are one second.
constexpr float STEP = 1.0F / 120.0F;

// A place far away from every crystal and from the exit of the maze used here.
constexpr glm::vec3 NOWHERE{-50.0F, 0.0F, -50.0F};

// The golden maze: 4 x 4 cells from seed 1, with a gate and two crystals.
game::MazeWorld goldenWorld() {
    return game::buildMazeWorld(4, 4, 1U);
}

// A round by hand, for the rules that only read a few of its numbers: it is being
// played, with the given charge.
game::Round roundWithBattery(float battery) {
    game::Round round;
    round.battery = battery;
    return round;
}

// Runs the clock of the pulse for a number of fixed steps and counts the beats.
int beatsIn(game::LowBatteryPulse& pulse, const game::Round& round, bool flashlightOn,
            const game::GameplaySettings& settings, int steps) {
    int beats = 0;
    for (int i = 0; i < steps; ++i) {
        if (game::advanceLowBatteryPulse(pulse, round, flashlightOn, settings, STEP)) {
            ++beats;
        }
    }
    return beats;
}

} // namespace

TEST_CASE("every cue has a file and a name of its own") {
    std::set<std::string> files;
    std::set<std::string> names;
    for (std::size_t i = 0; i < game::SOUND_CUE_COUNT; ++i) {
        const auto cue = static_cast<game::SoundCue>(i);
        CHECK(game::soundCueIndex(cue) == i);
        // A cue that was added to the enum but not to the table has null pointers.
        REQUIRE(game::soundCueFile(cue) != nullptr);
        REQUIRE(game::soundCueName(cue) != nullptr);
        const std::string file = game::soundCueFile(cue);
        // Every sound lies in assets/audio and is a WAV file: the only format the
        // build of miniaudio decodes.
        CHECK(file.starts_with("audio/"));
        CHECK(file.ends_with(".wav"));
        files.insert(file);
        names.insert(game::soundCueName(cue));
    }
    // A set keeps one copy of equal texts, so equal names would make it smaller.
    CHECK(files.size() == game::SOUND_CUE_COUNT);
    CHECK(names.size() == game::SOUND_CUE_COUNT);
    // The last entry of the enum is the last entry of the table.
    CHECK(game::soundCueIndex(game::SoundCue::GateBell) == game::SOUND_CUE_COUNT - 1);
    // New cues are added after the old ones, so every old cue keeps its number: the
    // nineteen that were there before the shade learned to hunt end with the wind, and
    // the two of the hunting shade come before the bell of the gate.
    CHECK(game::soundCueIndex(game::SoundCue::MazeWind) == 18U);
    CHECK(game::soundCueIndex(game::SoundCue::ShadeAlert) == 19U);
    CHECK(game::soundCueIndex(game::SoundCue::ShadeBanish) == 20U);
    CHECK(game::SOUND_CUE_COUNT == 22U);
}

TEST_CASE("the flashlight key clicks on, clicks off, and clicks dead on an empty battery") {
    CHECK(game::flashlightKeyCue(0.5F, false) == game::SoundCue::FlashlightOn);
    CHECK(game::flashlightKeyCue(0.5F, true) == game::SoundCue::FlashlightOff);
    // A battery that is almost empty still works.
    CHECK(game::flashlightKeyCue(0.001F, false) == game::SoundCue::FlashlightOn);
    // Empty: dead, whatever the switch says.
    CHECK(game::flashlightKeyCue(0.0F, false) == game::SoundCue::FlashlightDead);
    CHECK(game::flashlightKeyCue(0.0F, true) == game::SoundCue::FlashlightDead);
}

TEST_CASE("a step that changes nothing has no cue") {
    const game::Round round = roundWithBattery(0.5F);
    CHECK(game::roundStepCues(game::soundSnapshot(round, true), round, true).empty());
}

TEST_CASE("a collected crystal is one cue, also when the step collected two") {
    game::Round round = roundWithBattery(0.5F);
    const game::RoundSoundSnapshot before = game::soundSnapshot(round, true);

    round.collectedCount = 1;
    CHECK(game::roundStepCues(before, round, true) ==
          std::vector<game::SoundCue>{game::SoundCue::CrystalPickup});

    round.collectedCount = 2;
    CHECK(game::roundStepCues(before, round, true) ==
          std::vector<game::SoundCue>{game::SoundCue::CrystalPickup});
}

TEST_CASE("the gate that opens is a cue, the gate that stays open is not") {
    game::Round round = roundWithBattery(0.5F);
    const game::RoundSoundSnapshot closed = game::soundSnapshot(round, true);
    round.gateOpen = true;
    CHECK(game::roundStepCues(closed, round, true) ==
          std::vector<game::SoundCue>{game::SoundCue::GateOpen});
    // The step after: open before and open now.
    CHECK(game::roundStepCues(game::soundSnapshot(round, true), round, true).empty());
}

TEST_CASE("the crystal that opens the gate gives both cues, the pickup first") {
    game::Round round = roundWithBattery(0.5F);
    const game::RoundSoundSnapshot before = game::soundSnapshot(round, true);
    round.collectedCount = 1;
    round.gateOpen = true;
    CHECK(game::roundStepCues(before, round, true) ==
          std::vector<game::SoundCue>{game::SoundCue::CrystalPickup, game::SoundCue::GateOpen});
}

TEST_CASE("a new round fires nothing, whatever the round before looked like") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;

    // The application takes the snapshot right before every step, so the first step of
    // a new round compares the new round with itself.
    const game::Round fresh = game::startRound(world, settings);
    CHECK(game::roundStepCues(game::soundSnapshot(fresh, true), fresh, true).empty());

    // A maze without crystals starts with its gate open: that is not "the gate opens".
    const game::MazeWorld empty = game::buildMazeWorld(4, 4, 1U, game::InteractableSettings{}, 0);
    const game::Round open = game::startRound(empty, settings);
    REQUIRE(open.gateOpen);
    CHECK(game::roundStepCues(game::soundSnapshot(open, true), open, true).empty());

    // Even a snapshot of the round BEFORE the restart, compared with the new round,
    // is silent: the count went down and the gate closed, and neither is a cue.
    game::Round old = game::startRound(world, settings);
    old.collectedCount = 2;
    old.gateOpen = true;
    old.battery = 0.4F;
    CHECK(game::roundStepCues(game::soundSnapshot(old, false), fresh, true).empty());
}

TEST_CASE("the battery that runs out with the light on clicks dead, once") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    // Less charge than one step drains.
    round.battery = 0.5F * STEP / settings.batteryLifetimeSeconds;
    bool flashlightOn = true;

    game::RoundSoundSnapshot before = game::soundSnapshot(round, flashlightOn);
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    REQUIRE(round.battery == 0.0F);
    REQUIRE_FALSE(flashlightOn);
    CHECK(game::roundStepCues(before, round, flashlightOn) ==
          std::vector<game::SoundCue>{game::SoundCue::FlashlightDead});

    // The next step: empty before and empty now.
    before = game::soundSnapshot(round, flashlightOn);
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(game::roundStepCues(before, round, flashlightOn).empty());

    // The key on the empty battery: its own cue, and the switch is set. The step that
    // turns the switch off again must not click a second time.
    CHECK(game::flashlightKeyCue(round.battery, flashlightOn) == game::SoundCue::FlashlightDead);
    flashlightOn = true;
    before = game::soundSnapshot(round, flashlightOn);
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    REQUIRE_FALSE(flashlightOn);
    CHECK(game::roundStepCues(before, round, flashlightOn).empty());
}

TEST_CASE("a battery that is empty with the light off made no sound of dying") {
    // The debug UI can write 0 into the battery while the light is off.
    game::Round round = roundWithBattery(0.1F);
    const game::RoundSoundSnapshot before = game::soundSnapshot(round, false);
    round.battery = 0.0F;
    CHECK(game::roundStepCues(before, round, false).empty());
}

TEST_CASE("the low battery pulse sounds only while a played round has a low, lit battery") {
    const game::GameplaySettings settings; // threshold 0.2
    CHECK(game::lowBatteryPulseSounds(roundWithBattery(0.1F), true, settings));
    // The light is off.
    CHECK_FALSE(game::lowBatteryPulseSounds(roundWithBattery(0.1F), false, settings));
    // Exactly empty, exactly at the threshold, and above it.
    CHECK_FALSE(game::lowBatteryPulseSounds(roundWithBattery(0.0F), true, settings));
    CHECK_FALSE(game::lowBatteryPulseSounds(roundWithBattery(0.2F), true, settings));
    CHECK_FALSE(game::lowBatteryPulseSounds(roundWithBattery(0.9F), true, settings));
    // The round is won.
    game::Round won = roundWithBattery(0.1F);
    won.state = game::RoundState::Won;
    CHECK_FALSE(game::lowBatteryPulseSounds(won, true, settings));
    // A threshold of 0 switches the warning off.
    game::GameplaySettings never;
    never.lowBatteryThreshold = 0.0F;
    CHECK_FALSE(game::lowBatteryPulseSounds(roundWithBattery(0.1F), true, never));
}

TEST_CASE("the pulse gets faster as the battery runs down") {
    const game::GameplaySettings settings; // threshold 0.2
    const float nearThreshold = game::lowBatteryPulseInterval(0.199F, settings);
    const float half = game::lowBatteryPulseInterval(0.1F, settings);
    const float nearEmpty = game::lowBatteryPulseInterval(0.001F, settings);
    CHECK(nearThreshold == doctest::Approx(game::LOW_BATTERY_PULSE_SLOW_SECONDS).epsilon(0.01));
    // Half of the low range: half way between the two ends, 1.4 s.
    CHECK(half == doctest::Approx(1.4F));
    CHECK(nearEmpty == doctest::Approx(game::LOW_BATTERY_PULSE_FAST_SECONDS).epsilon(0.01));
    CHECK(nearThreshold > half);
    CHECK(half > nearEmpty);
    // Outside of the low range, and with a threshold of 0, there is no division.
    CHECK(game::lowBatteryPulseInterval(0.5F, settings) == game::LOW_BATTERY_PULSE_SLOW_SECONDS);
    game::GameplaySettings never;
    never.lowBatteryThreshold = 0.0F;
    CHECK(game::lowBatteryPulseInterval(0.1F, never) == game::LOW_BATTERY_PULSE_SLOW_SECONDS);
}

TEST_CASE("the pulse beats at once when the battery gets low and then at its interval") {
    const game::GameplaySettings settings;
    const game::Round round = roundWithBattery(0.1F); // interval 1.4 s: 168 steps
    game::LowBatteryPulse pulse;

    CHECK(game::advanceLowBatteryPulse(pulse, round, true, settings, STEP));
    // Not again in the next step, and not for a second.
    CHECK(beatsIn(pulse, round, true, settings, 120) == 0);
    // Ten seconds hold 10 / 1.4 = 7 whole intervals.
    CHECK(beatsIn(pulse, round, true, settings, 1200) == 7);
}

TEST_CASE("the pulse is silent with the light off and with a full or an empty battery") {
    const game::GameplaySettings settings;
    game::LowBatteryPulse pulse;
    CHECK(beatsIn(pulse, roundWithBattery(0.1F), false, settings, 1200) == 0);
    CHECK(beatsIn(pulse, roundWithBattery(0.9F), true, settings, 1200) == 0);
    CHECK(beatsIn(pulse, roundWithBattery(0.0F), true, settings, 1200) == 0);
}

TEST_CASE("one very long step is one beat, and no burst follows it") {
    const game::GameplaySettings settings;
    const game::Round round = roundWithBattery(0.1F);
    game::LowBatteryPulse pulse;
    CHECK(game::advanceLowBatteryPulse(pulse, round, true, settings, STEP));
    // A step of a whole minute: many intervals long, still one beat.
    CHECK(game::advanceLowBatteryPulse(pulse, round, true, settings, 60.0F));
    // The steps after it wait a full interval again.
    CHECK(beatsIn(pulse, round, true, settings, 120) == 0);
}

TEST_CASE("switching the light off holds the pulse, a charged battery and a new round reset it") {
    const game::GameplaySettings settings;
    const game::Round low = roundWithBattery(0.1F);
    game::LowBatteryPulse pulse;
    REQUIRE(game::advanceLowBatteryPulse(pulse, low, true, settings, STEP));

    // The light off for ten seconds, then on again: the clock stood still, so the wait
    // goes on where it was and there is no beat at once.
    CHECK(beatsIn(pulse, low, false, settings, 1200) == 0);
    CHECK_FALSE(game::advanceLowBatteryPulse(pulse, low, true, settings, STEP));

    // A crystal charged the battery above the threshold: the clock is reset, and the
    // next time the battery is low the first beat comes at once.
    CHECK_FALSE(game::advanceLowBatteryPulse(pulse, roundWithBattery(0.5F), true, settings, STEP));
    CHECK(game::advanceLowBatteryPulse(pulse, low, true, settings, STEP));

    // A new round: the application assigns a new clock.
    CHECK_FALSE(game::advanceLowBatteryPulse(pulse, low, true, settings, STEP));
    pulse = game::LowBatteryPulse{};
    CHECK(game::advanceLowBatteryPulse(pulse, low, true, settings, STEP));
}

TEST_CASE("a winded player breathes at once and then every 1.1 seconds") {
    game::WindedBreath breath;
    CHECK(game::advanceWindedBreath(breath, true, STEP));
    // Not again in the next step, and not for a second.
    int breaths = 0;
    for (int i = 0; i < 120; ++i) {
        breaths += game::advanceWindedBreath(breath, true, STEP) ? 1 : 0;
    }
    CHECK(breaths == 0);
    // Ten more seconds: 11 s in all hold 11 / 1.1 = 10 waits, give or take the one
    // that ends right at the edge.
    for (int i = 0; i < 1200; ++i) {
        breaths += game::advanceWindedBreath(breath, true, STEP) ? 1 : 0;
    }
    CHECK(breaths >= 9);
    CHECK(breaths <= 10);
}

TEST_CASE("a player who is not winded does not breathe, and recovering resets the clock") {
    game::WindedBreath breath;
    bool breathed = false;
    for (int i = 0; i < 1200; ++i) {
        breathed = breathed || game::advanceWindedBreath(breath, false, STEP);
    }
    CHECK_FALSE(breathed);

    // Winded, one breath, recovered half a second later: silence in between.
    REQUIRE(game::advanceWindedBreath(breath, true, STEP));
    for (int i = 0; i < 60; ++i) {
        breathed = breathed || game::advanceWindedBreath(breath, true, STEP);
    }
    CHECK_FALSE(breathed);
    CHECK_FALSE(game::advanceWindedBreath(breath, false, STEP));
    // Winded again: the first breath comes at once, not after the rest of the old wait.
    CHECK(game::advanceWindedBreath(breath, true, STEP));
}

TEST_CASE("one very long winded step is one breath, and a new round resets the clock") {
    game::WindedBreath breath;
    CHECK(game::advanceWindedBreath(breath, true, STEP));
    // A step of a whole minute: many waits long, still one breath, and no burst after.
    CHECK(game::advanceWindedBreath(breath, true, 60.0F));
    CHECK_FALSE(game::advanceWindedBreath(breath, true, STEP));

    // A new round: the application assigns a new clock.
    breath = game::WindedBreath{};
    CHECK(game::advanceWindedBreath(breath, true, STEP));
}

TEST_CASE("the breath of a winded player has its own sound file") {
    CHECK(std::string(game::soundCueFile(game::SoundCue::WindedBreath)) ==
          "audio/winded_breath.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::WindedBreath)) == "winded breath");
}

TEST_CASE("a flask that is picked up is one cue with its own sound file") {
    game::Round round = roundWithBattery(0.5F);
    const game::RoundSoundSnapshot before = game::soundSnapshot(round, true);

    round.flasksCollected = 1;
    CHECK(game::roundStepCues(before, round, true) ==
          std::vector<game::SoundCue>{game::SoundCue::FlaskPickup});
    // Two in one step are one sound, like two crystals.
    round.flasksCollected = 2;
    CHECK(game::roundStepCues(before, round, true) ==
          std::vector<game::SoundCue>{game::SoundCue::FlaskPickup});
    // The step after: as many as before, no cue.
    CHECK(game::roundStepCues(game::soundSnapshot(round, true), round, true).empty());

    // Not the chime of a crystal.
    CHECK(std::string(game::soundCueFile(game::SoundCue::FlaskPickup)) == "audio/flask_pickup.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::FlaskPickup)) == "flask pickup");
}

TEST_CASE("a crystal and a flask in one step give both cues, the crystal first") {
    game::Round round = roundWithBattery(0.5F);
    const game::RoundSoundSnapshot before = game::soundSnapshot(round, true);
    round.collectedCount = 1;
    round.flasksCollected = 1;
    CHECK(game::roundStepCues(before, round, true) ==
          std::vector<game::SoundCue>{game::SoundCue::CrystalPickup, game::SoundCue::FlaskPickup});
}

TEST_CASE("walking into a flask in a round plays its cue, and a new round is silent") {
    const game::MazeWorld world = game::buildMazeWorld(16, 16, 1U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    REQUIRE(round.flasks.size() == 1);
    bool flashlightOn = true;

    const game::RoundSoundSnapshot before = game::soundSnapshot(round, flashlightOn);
    const glm::vec3 feet = game::cellCenter(round.flasks[0].cell.x, round.flasks[0].cell.z);
    game::updateRound(round, world, settings, feet, flashlightOn, STEP);
    CHECK(game::roundStepCues(before, round, flashlightOn) ==
          std::vector<game::SoundCue>{game::SoundCue::FlaskPickup});

    // The round is started again: the count went down, and that is not a pickup.
    const game::RoundSoundSnapshot old = game::soundSnapshot(round, flashlightOn);
    const game::Round fresh = game::startRound(world, settings);
    CHECK(game::roundStepCues(old, fresh, flashlightOn).empty());
}

TEST_CASE("the hum of the shade has a file and a name, and so has the catch") {
    CHECK(std::string(game::soundCueFile(game::SoundCue::ShadeNear)) == "audio/shade_near.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::ShadeNear)) == "shade near");
    CHECK(std::string(game::soundCueFile(game::SoundCue::Caught)) == "audio/caught.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::Caught)) == "caught");
}

TEST_CASE("the alert and the banish of the shade have sounds of their own") {
    CHECK(std::string(game::soundCueFile(game::SoundCue::ShadeAlert)) == "audio/shade_alert.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::ShadeAlert)) == "shade alert");
    CHECK(std::string(game::soundCueFile(game::SoundCue::ShadeBanish)) == "audio/shade_banish.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::ShadeBanish)) == "shade banish");
    // They tell the player something, so they follow the effects volume.
    CHECK_FALSE(game::soundCueIsAmbient(game::SoundCue::ShadeAlert));
    CHECK_FALSE(game::soundCueIsAmbient(game::SoundCue::ShadeBanish));
}

TEST_CASE("a shade that is quiet after a banish does not hum, however near it stands") {
    game::Round round;
    round.shade.present = true;
    round.shade.wayMetres = 3.0F;
    REQUIRE(game::shadeHumSounds(round));
    round.shade.quietLeft = 5.0F;
    CHECK_FALSE(game::shadeHumSounds(round));
    game::ShadeHum hum;
    for (int i = 0; i < 600; ++i) {
        CHECK_FALSE(game::advanceShadeHum(hum, round, STEP));
    }
    // And it makes no step: it stands.
    game::StepClock clock;
    game::CuePlay play;
    CHECK(game::shadeStepMetres(round) == 0.0F);
    CHECK_FALSE(game::advanceShadeSteps(clock, round, play));
    // The quiet time is over: the hum is back at once.
    round.shade.quietLeft = 0.0F;
    CHECK(game::advanceShadeHum(hum, round, STEP));
}

TEST_CASE("the hum is heard from 14 m of way, and not in a round without a shade") {
    game::Round round;
    round.shade.present = true;

    CHECK(game::SHADE_HUM_DISTANCE == 14.0F);
    // A shade that has not taken a step yet knows no way: silence.
    CHECK_FALSE(game::shadeHumSounds(round));
    round.shade.wayMetres = 0.5F;
    CHECK(game::shadeHumSounds(round));
    round.shade.wayMetres = 13.9F;
    CHECK(game::shadeHumSounds(round));
    round.shade.wayMetres = 14.1F;
    CHECK_FALSE(game::shadeHumSounds(round));

    // A won round is silent, and so is a calm one.
    round.shade.wayMetres = 3.0F;
    round.state = game::RoundState::Won;
    CHECK_FALSE(game::shadeHumSounds(round));
    round.state = game::RoundState::Playing;
    round.shade.present = false;
    CHECK_FALSE(game::shadeHumSounds(round));
}

TEST_CASE("the hum comes more often the nearer the shade is") {
    CHECK(game::shadeHumInterval(14.0F) == doctest::Approx(game::SHADE_HUM_SLOW_SECONDS));
    CHECK(game::shadeHumInterval(40.0F) == doctest::Approx(game::SHADE_HUM_SLOW_SECONDS));
    CHECK(game::shadeHumInterval(0.0F) == doctest::Approx(game::SHADE_HUM_FAST_SECONDS));
    CHECK(game::shadeHumInterval(-1.0F) == doctest::Approx(game::SHADE_HUM_FAST_SECONDS));
    float before = game::shadeHumInterval(0.0F);
    for (int metres = 1; metres <= 14; ++metres) {
        const float now = game::shadeHumInterval(static_cast<float>(metres));
        CHECK(now > before);
        before = now;
    }
    // The sound of the hum is 0.62 s long: it must be over before the next one starts.
    CHECK(game::SHADE_HUM_FAST_SECONDS > 0.62F);
}

TEST_CASE("the clock of the hum starts at once, waits its interval and is silent far away") {
    game::Round round;
    round.shade.present = true;
    game::ShadeHum hum;

    // Far away: never.
    round.shade.wayMetres = 30.0F;
    for (int i = 0; i < 600; ++i) {
        CHECK_FALSE(game::advanceShadeHum(hum, round, STEP));
    }
    // Near: at once, and then again after the interval of 7 m (1.55 s), not before.
    round.shade.wayMetres = 7.0F;
    CHECK(game::advanceShadeHum(hum, round, STEP));
    const float interval = game::shadeHumInterval(7.0F);
    CHECK(interval == doctest::Approx(1.55F));
    int steps = 0;
    while (!game::advanceShadeHum(hum, round, STEP)) {
        ++steps;
        REQUIRE(steps < 1000);
    }
    CHECK(static_cast<float>(steps + 1) * STEP == doctest::Approx(interval).epsilon(0.02));

    // Twenty seconds right next to the shade: one hum every 0.7 s or so, never two in
    // one step and never faster than the sound is long.
    round.shade.wayMetres = 0.5F;
    int hums = 0;
    for (int i = 0; i < 20 * 120; ++i) {
        if (game::advanceShadeHum(hum, round, STEP)) {
            ++hums;
        }
    }
    CHECK(hums >= 25);
    CHECK(hums <= 29);

    // A calm round: the clock never fires.
    round.shade.wayMetres = 7.0F;
    round.shade.present = false;
    for (int i = 0; i < 600; ++i) {
        CHECK_FALSE(game::advanceShadeHum(hum, round, STEP));
    }
    // Back in range it hums at once again: the clock was reset.
    round.shade.present = true;
    CHECK(game::advanceShadeHum(hum, round, STEP));
}

TEST_CASE("the hum follows the way the shade has to walk, not the straight line") {
    // A real maze: at the start of the round the shade can stand a few metres away
    // behind walls and still be a long walk away.
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 168U, {}, 13);
    game::GameplaySettings settings;
    // Ears for the whole maze: the player sprints on the spot, and the shade comes.
    settings.shade.hearSprintMetres = 100000.0F;
    game::Round round = game::startRound(world, settings);
    REQUIRE(round.shade.present);
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    const glm::vec3 feet = world.startPosition;
    REQUIRE(game::shadeDistance(round.shade, feet) < game::SHADE_HUM_DISTANCE);

    // One step, still in the grace time: the way is known, and it is long.
    game::updateRoundShade(round, world, settings, feet, {}, obstacles, STEP);
    CHECK(round.shade.wayMetres == doctest::Approx(88.0F));
    CHECK_FALSE(game::shadeHumSounds(round));

    // It walks until it has caught the player. The way only gets shorter, and the hum
    // starts when 14 m of it are left.
    float before = round.shade.wayMetres;
    bool hummed = false;
    bool caught = false;
    for (int i = 0; i < 60 * 120 && !caught; ++i) {
        caught = game::updateRoundShade(round, world, settings, feet, {}, obstacles, STEP,
                                        game::Noise::Sprint)
                     .caught;
        CHECK(round.shade.wayMetres <= before + 0.001F);
        before = round.shade.wayMetres;
        CHECK(game::shadeHumSounds(round) == (round.shade.wayMetres <= game::SHADE_HUM_DISTANCE));
        hummed = hummed || game::shadeHumSounds(round);
    }
    CHECK(caught);
    CHECK(hummed);
    CHECK(round.shade.wayMetres <= settings.shade.catchDistance);
}

TEST_CASE("the wind and the bell of the intro have sound files of their own") {
    CHECK(std::string(game::soundCueFile(game::SoundCue::IntroWind)) == "audio/intro_wind.wav");
    CHECK(std::string(game::soundCueFile(game::SoundCue::IntroBell)) == "audio/intro_bell.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::IntroWind)) == "intro wind");
    CHECK(std::string(game::soundCueName(game::SoundCue::IntroBell)) == "intro bell");
}

TEST_CASE("the steps and the wind have files of their own, and only the winds are ambient") {
    CHECK(std::string(game::soundCueFile(game::SoundCue::Footstep1)) == "audio/footstep_1.wav");
    CHECK(std::string(game::soundCueFile(game::SoundCue::Footstep3)) == "audio/footstep_3.wav");
    CHECK(std::string(game::soundCueFile(game::SoundCue::ShadeStep2)) == "audio/shade_step_2.wav");
    CHECK(std::string(game::soundCueFile(game::SoundCue::MazeWind)) == "audio/maze_wind.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::MazeWind)) == "maze wind");

    // The two winds follow the ambient volume, everything else the effects volume.
    for (std::size_t i = 0; i < game::SOUND_CUE_COUNT; ++i) {
        const auto cue = static_cast<game::SoundCue>(i);
        const bool wind = cue == game::SoundCue::IntroWind || cue == game::SoundCue::MazeWind;
        CHECK(game::soundCueIsAmbient(cue) == wind);
    }
}

namespace {

// One fixed step of a player who moves at the given speed in metres per second, with
// the speeds of game::Player.
game::FootstepInput walkedAt(float metresPerSecond) {
    return {.metres = metresPerSecond * STEP,
            .stepSeconds = STEP,
            .walkSpeed = game::Player::WALK_SPEED,
            .sprintSpeed = game::Player::SPRINT_SPEED};
}

// Runs the clock of the steps for a number of fixed steps and collects what it plays.
std::vector<game::CuePlay> stepsIn(game::StepClock& clock, const game::FootstepInput& input,
                                   int steps) {
    std::vector<game::CuePlay> plays;
    for (int i = 0; i < steps; ++i) {
        game::CuePlay play;
        if (game::advanceFootsteps(clock, input, play)) {
            plays.push_back(play);
        }
    }
    return plays;
}

} // namespace

TEST_CASE("a walking player takes two steps per second, with the three sounds in turn") {
    game::StepClock clock;
    // Ten seconds at 3 m/s are 30 m: the first step after half a stride (0.75 m), then
    // one every 1.5 m. That is 20 steps, give or take the one at the edge.
    const std::vector<game::CuePlay> plays = stepsIn(clock, walkedAt(3.0F), 1200);
    CHECK(plays.size() >= 19);
    CHECK(plays.size() <= 20);
    REQUIRE(plays.size() >= 4);
    CHECK(plays[0].cue == game::SoundCue::Footstep1);
    CHECK(plays[1].cue == game::SoundCue::Footstep2);
    CHECK(plays[2].cue == game::SoundCue::Footstep3);
    CHECK(plays[3].cue == game::SoundCue::Footstep1);
    for (const game::CuePlay& play : plays) {
        CHECK(play.volume == game::FOOTSTEP_WALK_VOLUME);
    }
}

TEST_CASE("the first step of a walk comes after half a stride") {
    game::StepClock clock;
    game::CuePlay play;
    int steps = 0;
    while (!game::advanceFootsteps(clock, walkedAt(3.0F), play)) {
        ++steps;
        REQUIRE(steps < 1000);
    }
    // 0.75 m at 3 m/s: a quarter of a second, 30 fixed steps.
    CHECK(static_cast<float>(steps + 1) * STEP == doctest::Approx(0.25F).epsilon(0.05));
}

TEST_CASE("a sprinting player steps faster and louder than a walking one") {
    game::StepClock clock;
    // Ten seconds at 5.5 m/s are 55 m, one step every 2 m: 27 or 28 steps, more than
    // the 20 of a walk, and each one at the full volume.
    const std::vector<game::CuePlay> plays = stepsIn(clock, walkedAt(5.5F), 1200);
    CHECK(plays.size() >= 27);
    CHECK(plays.size() <= 28);
    for (const game::CuePlay& play : plays) {
        CHECK(play.volume == game::FOOTSTEP_SPRINT_VOLUME);
    }
    CHECK(game::FOOTSTEP_SPRINT_VOLUME > game::FOOTSTEP_WALK_VOLUME);
    // Faster in time, although the stride is longer.
    CHECK(5.5F / game::FOOTSTEP_SPRINT_METRES > 3.0F / game::FOOTSTEP_WALK_METRES);
}

TEST_CASE("sprinted or walked is decided by the speed of the feet, not by the key") {
    // A sprint that only slides slowly along a wall sounds like walking.
    game::StepClock clock;
    for (const game::CuePlay& play : stepsIn(clock, walkedAt(2.0F), 1200)) {
        CHECK(play.volume == game::FOOTSTEP_WALK_VOLUME);
    }
    // Half way between the two speeds is the border: 4.25 m/s.
    clock = {};
    for (const game::CuePlay& play : stepsIn(clock, walkedAt(4.2F), 1200)) {
        CHECK(play.volume == game::FOOTSTEP_WALK_VOLUME);
    }
    clock = {};
    for (const game::CuePlay& play : stepsIn(clock, walkedAt(4.3F), 1200)) {
        CHECK(play.volume == game::FOOTSTEP_SPRINT_VOLUME);
    }
}

TEST_CASE("a player who stands makes no step, and the way walked so far is kept") {
    game::StepClock clock;
    CHECK(stepsIn(clock, walkedAt(0.0F), 1200).empty());
    CHECK(clock.metresToNextStep == game::StepClock{}.metresToNextStep);

    // 0.6 m walked, a long stop, then on: the step comes after the 0.15 m that were
    // still missing, not after a whole new stride.
    CHECK(stepsIn(clock, walkedAt(3.0F), 24).empty());
    CHECK(stepsIn(clock, walkedAt(0.0F), 600).empty());
    CHECK(stepsIn(clock, walkedAt(3.0F), 8).size() == 1);

    // A step without a length (a clock that did not move) is no step either.
    game::CuePlay play;
    CHECK_FALSE(game::advanceFootsteps(clock, {.metres = 1.0F, .stepSeconds = 0.0F}, play));
}

TEST_CASE("a flying player is silent, and landing starts the clock anew") {
    game::StepClock clock;
    game::FootstepInput flying = walkedAt(6.0F);
    flying.flying = true;
    CHECK(stepsIn(clock, flying, 1200).empty());

    // Walk for a while, fly for a moment, walk again: the way starts from the
    // beginning, so the next step comes after half a stride and with the first sound.
    REQUIRE(stepsIn(clock, walkedAt(3.0F), 100).size() == 2);
    CHECK(stepsIn(clock, flying, 1).empty());
    CHECK(clock.metresToNextStep == game::StepClock{}.metresToNextStep);
    const std::vector<game::CuePlay> plays = stepsIn(clock, walkedAt(3.0F), 31);
    REQUIRE(plays.size() == 1);
    CHECK(plays[0].cue == game::SoundCue::Footstep1);
}

TEST_CASE("one very long way is one step, and no burst follows it") {
    game::StepClock clock;
    game::CuePlay play;
    // The debug UI puts the player 500 m away in one step: one sound.
    CHECK(game::advanceFootsteps(clock, {.metres = 500.0F, .stepSeconds = STEP}, play));
    // The steps after it wait a whole stride again.
    CHECK(stepsIn(clock, walkedAt(3.0F), 50).empty());
}

TEST_CASE("a player who pushes against a wall makes no steps, with the map or in flight neither") {
    // One closed cell: walls on the grid lines x = 0, x = 2, z = 0 and z = 2.
    const std::vector<scene::Aabb> obstacles = game::mazeColliders(game::Maze(1, 1));
    const game::Terrain flatGround;
    game::Player player;
    player.position = {1.0F, 0.0F, 1.0F};
    game::PlayerInput input;
    input.forward = true;
    input.sprint = true;
    game::StepClock clock;
    game::CuePlay play;

    // The same sum the application does in every fixed step: how far the feet moved
    // over the ground across Player::update.
    const auto walkOneStep = [&](float yawDegrees) {
        const glm::vec3 before = player.position;
        player.update(input, yawDegrees, 0.0F, STEP, obstacles, flatGround);
        const glm::vec3 moved = player.position - before;
        return game::advanceFootsteps(clock,
                                      {.metres = glm::length(glm::vec2{moved.x, moved.z}),
                                       .stepSeconds = STEP,
                                       .walkSpeed = player.walkSpeed,
                                       .sprintSpeed = player.sprintSpeed,
                                       .flying = player.noclip},
                                      play);
    };

    // Two seconds straight at the east wall: the way to it, and then ten seconds of
    // pushing with the keys still held, without one step.
    for (int i = 0; i < 240; ++i) {
        walkOneStep(90.0F);
    }
    int steps = 0;
    for (int i = 0; i < 1200; ++i) {
        steps += walkOneStep(90.0F) ? 1 : 0;
    }
    CHECK(steps == 0);

    // The map is held: the keys do not reach the player (game::movementInput), who
    // stands in the middle of the cell, and nothing sounds.
    const game::PlayerInput held = input;
    input = game::movementInput(held, true);
    player.position = {1.0F, 0.0F, 1.0F};
    for (int i = 0; i < 600; ++i) {
        steps += walkOneStep(0.0F) ? 1 : 0;
    }
    CHECK(steps == 0);

    // Noclip: the player flies through the wall, far and fast, without a sound.
    input = held;
    player.noclip = true;
    for (int i = 0; i < 600; ++i) {
        steps += walkOneStep(90.0F) ? 1 : 0;
    }
    CHECK(player.position.x > 10.0F);
    CHECK(steps == 0);
}

TEST_CASE("the shade is heard walking from 9 m of way, louder the nearer it is") {
    CHECK(game::SHADE_STEP_DISTANCE == 9.0F);
    CHECK(game::SHADE_STEP_DISTANCE < game::SHADE_HUM_DISTANCE);
    CHECK(game::shadeStepVolume(0.0F) == doctest::Approx(game::SHADE_STEP_NEAR_VOLUME));
    CHECK(game::shadeStepVolume(9.0F) == doctest::Approx(game::SHADE_STEP_FAR_VOLUME));
    CHECK(game::shadeStepVolume(30.0F) == doctest::Approx(game::SHADE_STEP_FAR_VOLUME));
    CHECK(game::shadeStepVolume(-1.0F) == doctest::Approx(game::SHADE_STEP_NEAR_VOLUME));
    CHECK(game::shadeStepVolume(4.5F) == doctest::Approx(0.65F));
    float before = game::shadeStepVolume(0.0F);
    for (int metres = 1; metres <= 9; ++metres) {
        const float now = game::shadeStepVolume(static_cast<float>(metres));
        CHECK(now < before);
        before = now;
    }
    // Its steps are slower than the ones of a walking player: fewer per second.
    CHECK(game::SHADE_SPEED / game::SHADE_STEP_METRES <
          game::Player::WALK_SPEED / game::FOOTSTEP_WALK_METRES);
}

TEST_CASE("a shade that stands makes no step, near or far, and a round without one is silent") {
    game::Round round;
    round.shade.present = true;
    round.shade.wayMetres = 3.0F;
    game::StepClock clock;
    game::CuePlay play;

    // Standing: the two positions are the same.
    CHECK(game::shadeStepMetres(round) == 0.0F);
    for (int i = 0; i < 1200; ++i) {
        CHECK_FALSE(game::advanceShadeSteps(clock, round, play));
    }

    // Walking 4 m/s, near: one step every 2.6 m, which is 0.65 s. Ten seconds hold 15,
    // give or take the one at the edge, with the two sounds in turn.
    round.shade.previousPosition = {0.0F, 0.0F, 0.0F};
    round.shade.position = {game::SHADE_SPEED * STEP, 0.5F, 0.0F};
    // The height does not count: only the way over the ground.
    CHECK(game::shadeStepMetres(round) == doctest::Approx(game::SHADE_SPEED * STEP));
    std::vector<game::CuePlay> plays;
    for (int i = 0; i < 1200; ++i) {
        if (game::advanceShadeSteps(clock, round, play)) {
            plays.push_back(play);
        }
    }
    CHECK(plays.size() >= 15);
    CHECK(plays.size() <= 16);
    REQUIRE(plays.size() >= 3);
    CHECK(plays[0].cue == game::SoundCue::ShadeStep1);
    CHECK(plays[1].cue == game::SoundCue::ShadeStep2);
    CHECK(plays[2].cue == game::SoundCue::ShadeStep1);
    CHECK(plays[0].volume == doctest::Approx(game::shadeStepVolume(3.0F)));

    // Walking, but 9.1 m of way away: not heard.
    round.shade.wayMetres = 9.1F;
    for (int i = 0; i < 1200; ++i) {
        CHECK_FALSE(game::advanceShadeSteps(clock, round, play));
    }

    // A won round and a calm one: silent, and the clock is a new one afterwards.
    round.shade.wayMetres = 3.0F;
    round.state = game::RoundState::Won;
    for (int i = 0; i < 600; ++i) {
        CHECK_FALSE(game::advanceShadeSteps(clock, round, play));
    }
    round.state = game::RoundState::Playing;
    round.shade.present = false;
    CHECK(game::shadeStepMetres(round) == 0.0F);
    for (int i = 0; i < 600; ++i) {
        CHECK_FALSE(game::advanceShadeSteps(clock, round, play));
    }
    CHECK(clock.metresToNextStep == game::StepClock{}.metresToNextStep);
    CHECK(clock.nextSound == 0);
}

TEST_CASE("the shade of a real round steps only while it walks, and only near the player") {
    // The maze of the hum test: the shade starts 88 m of way from the player, who
    // sprints on the spot and is heard across the whole maze.
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 168U, {}, 13);
    game::GameplaySettings settings;
    settings.shade.hearSprintMetres = 100000.0F;
    game::Round round = game::startRound(world, settings);
    REQUIRE(round.shade.present);
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    const glm::vec3 feet = world.startPosition;
    game::StepClock clock;
    game::CuePlay play;

    // The grace time: it stands, so there is nothing walked and nothing to hear.
    const int graceSteps = static_cast<int>(game::SHADE_GRACE_SECONDS / STEP) - 1;
    for (int i = 0; i < graceSteps; ++i) {
        game::updateRoundShade(round, world, settings, feet, {}, obstacles, STEP);
        CHECK(game::shadeStepMetres(round) == 0.0F);
        CHECK_FALSE(game::advanceShadeSteps(clock, round, play));
    }

    // Then it walks until it has caught the player. Every step that is heard lies
    // within 9 m of way, and each is louder than the one before.
    int heard = 0;
    float lastVolume = 0.0F;
    bool caught = false;
    for (int i = 0; i < 60 * 120 && !caught; ++i) {
        caught = game::updateRoundShade(round, world, settings, feet, {}, obstacles, STEP,
                                        game::Noise::Sprint)
                     .caught;
        if (game::advanceShadeSteps(clock, round, play)) {
            ++heard;
            CHECK(round.shade.wayMetres <= game::SHADE_STEP_DISTANCE);
            CHECK(play.volume >= lastVolume);
            lastVolume = play.volume;
        }
    }
    CHECK(caught);
    // 9 m of way at one step per 2.6 m: three steps, or four with the one at the edge.
    CHECK(heard >= 3);
    CHECK(heard <= 4);
}

TEST_CASE("the wind of the maze is heard while a round is played and nowhere else") {
    using game::GameMode;
    CHECK(game::mazeWindPlays({.mode = GameMode::Playing}));
    for (const GameMode mode :
         {GameMode::MainMenu, GameMode::Paused, GameMode::RoundEnd, GameMode::Quitting,
          GameMode::SettingsFromMenu, GameMode::SettingsFromPause, GameMode::Intro,
          GameMode::FreePlay, GameMode::Nights, GameMode::NewCampaign, GameMode::NightCard,
          GameMode::EndingCard, GameMode::CampaignIntro}) {
        CHECK_FALSE(game::mazeWindPlays({.mode = mode}));
    }
    // The window is not the active one, or the menu camera shows the game.
    CHECK_FALSE(game::mazeWindPlays({.mode = GameMode::Playing, .windowFocused = false}));
    CHECK_FALSE(game::mazeWindPlays({.mode = GameMode::Playing, .menuCamera = true}));
}

TEST_CASE("a sample of the wind is heard on a menu, but never over the intro") {
    using game::GameMode;
    for (const GameMode mode : {GameMode::MainMenu, GameMode::Paused, GameMode::RoundEnd,
                                GameMode::SettingsFromMenu, GameMode::SettingsFromPause,
                                GameMode::FreePlay, GameMode::Nights, GameMode::NewCampaign}) {
        CHECK(game::mazeWindPlays({.mode = mode, .sample = true}));
    }
    // Also under the menu camera: the sample is asked for by a person at the window.
    CHECK(game::mazeWindPlays({.mode = GameMode::Playing, .menuCamera = true, .sample = true}));
    // The intro has its own wind, and a window in the background is silent.
    CHECK_FALSE(game::mazeWindPlays({.mode = GameMode::Intro, .sample = true}));
    CHECK_FALSE(game::mazeWindPlays({.mode = GameMode::CampaignIntro, .sample = true}));
    // The cards of the campaign are silent as well: a sample that still runs when a night
    // is started stops under its title card.
    CHECK_FALSE(game::mazeWindPlays({.mode = GameMode::NightCard, .sample = true}));
    CHECK_FALSE(game::mazeWindPlays({.mode = GameMode::EndingCard, .sample = true}));
    CHECK_FALSE(game::mazeWindPlays(
        {.mode = GameMode::SettingsFromMenu, .windowFocused = false, .sample = true}));

    // Coming is slower than going, and a sample comes in fast.
    CHECK(game::MAZE_WIND_FADE_OUT_SECONDS < game::MAZE_WIND_FADE_IN_SECONDS);
    CHECK(game::MAZE_WIND_SAMPLE_FADE_IN_SECONDS < game::MAZE_WIND_SAMPLE_SECONDS);
}

TEST_CASE("the bell of the gate is the last cue and has a sound file of its own") {
    CHECK(game::soundCueIndex(game::SoundCue::GateBell) == game::SOUND_CUE_COUNT - 1);
    CHECK(std::string(game::soundCueFile(game::SoundCue::GateBell)) == "audio/gate_bell.wav");
    CHECK(std::string(game::soundCueName(game::SoundCue::GateBell)) == "gate bell");
    // It tells the player something, so it follows the effects volume, not the ambient one.
    CHECK_FALSE(game::soundCueIsAmbient(game::SoundCue::GateBell));
    // The cues before it kept their numbers: the audio layer loads the files by them.
    CHECK(game::soundCueIndex(game::SoundCue::ShadeBanish) == 20);
    CHECK(game::soundCueIndex(game::SoundCue::IntroBell) == 12);
}

TEST_CASE("the bell is loudest at the exit and fainter with every passage") {
    CHECK(game::gateBellVolume(0) == game::GATE_BELL_NEAR_VOLUME);
    CHECK(game::gateBellVolume(game::GATE_BELL_FAR_PASSAGES) ==
          doctest::Approx(game::GATE_BELL_FAR_VOLUME));
    CHECK(game::gateBellVolume(game::GATE_BELL_FAR_PASSAGES / 2) ==
          doctest::Approx((game::GATE_BELL_NEAR_VOLUME + game::GATE_BELL_FAR_VOLUME) / 2.0F));

    // One passage nearer is always louder, until the far distance.
    for (int passages = 1; passages <= game::GATE_BELL_FAR_PASSAGES; ++passages) {
        CHECK(game::gateBellVolume(passages) < game::gateBellVolume(passages - 1));
    }
    // Beyond it, and with no way at all, the bell is faint but never silent.
    CHECK(game::gateBellVolume(500) == doctest::Approx(game::GATE_BELL_FAR_VOLUME));
    CHECK(game::gateBellVolume(game::UNREACHABLE) == doctest::Approx(game::GATE_BELL_FAR_VOLUME));
    CHECK(game::GATE_BELL_FAR_VOLUME > 0.0F);
}

TEST_CASE("the bell is silent while the gate is closed and after the round is won") {
    game::Round round;
    CHECK_FALSE(game::gateBellTolls(round));
    round.gateOpen = true;
    CHECK(game::gateBellTolls(round));
    round.state = game::RoundState::Won;
    CHECK_FALSE(game::gateBellTolls(round));

    // A closed gate never tolls, however long it stays closed.
    game::GateBell bell;
    const game::Round closed;
    int tolls = 0;
    for (int i = 0; i < 120 * 30; ++i) {
        tolls += game::advanceGateBell(bell, closed, game::GATE_BELL_SECONDS, STEP) ? 1 : 0;
    }
    CHECK(tolls == 0);
    CHECK(bell.secondsToNextToll == game::GATE_BELL_FIRST_SECONDS);
}

TEST_CASE("the bell first tolls two seconds after the gate opens and then every six") {
    game::Round round;
    round.gateOpen = true;
    game::GateBell bell;

    // The steps at which a toll is due, over 21 seconds.
    std::vector<int> tollSteps;
    for (int i = 1; i <= 120 * 21; ++i) {
        if (game::advanceGateBell(bell, round, game::GATE_BELL_SECONDS, STEP)) {
            tollSteps.push_back(i);
        }
    }

    // At 2 s, 8 s, 14 s and 20 s, each to within a step or two of rounding.
    REQUIRE(tollSteps.size() == 4);
    CHECK(tollSteps[0] == doctest::Approx(240).epsilon(0.01));
    CHECK(tollSteps[1] - tollSteps[0] == doctest::Approx(720).epsilon(0.01));
    CHECK(tollSteps[2] - tollSteps[1] == doctest::Approx(720).epsilon(0.01));
    CHECK(tollSteps[3] - tollSteps[2] == doctest::Approx(720).epsilon(0.01));
}

TEST_CASE("the wait between two tolls is the one of the settings, and never under a second") {
    game::Round round;
    round.gateOpen = true;

    // 10 s between tolls: three tolls in 25 s (at 2, 12 and 22 s).
    game::GateBell slow;
    int tolls = 0;
    for (int i = 0; i < 120 * 25; ++i) {
        tolls += game::advanceGateBell(slow, round, 10.0F, STEP) ? 1 : 0;
    }
    CHECK(tolls == 3);

    // An interval of 0 typed into the slider does not toll in every step.
    game::GateBell typed;
    tolls = 0;
    for (int i = 0; i < 120 * 10; ++i) {
        tolls += game::advanceGateBell(typed, round, 0.0F, STEP) ? 1 : 0;
    }
    CHECK(tolls <= 10);
    CHECK(game::GATE_BELL_MIN_SECONDS >= 1.0F);
}

TEST_CASE("one very long step is one toll, and a new round resets the bell") {
    game::Round round;
    round.gateOpen = true;
    game::GateBell bell;

    CHECK(game::advanceGateBell(bell, round, game::GATE_BELL_SECONDS, 100.0F));
    CHECK_FALSE(game::advanceGateBell(bell, round, game::GATE_BELL_SECONDS, STEP));
    CHECK(bell.secondsToNextToll > game::GATE_BELL_SECONDS - 1.0F);

    // A new round has a closed gate: the clock goes back to the first wait.
    const game::Round fresh;
    CHECK_FALSE(game::advanceGateBell(bell, fresh, game::GATE_BELL_SECONDS, STEP));
    CHECK(bell.secondsToNextToll == game::GATE_BELL_FIRST_SECONDS);
    // And a new clock starts there too.
    CHECK(game::GateBell{}.secondsToNextToll == game::GATE_BELL_FIRST_SECONDS);
}

TEST_CASE("in a real round the bell starts with the gate and is louder next to the exit") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = true;
    game::GateBell bell;

    // Closed: three seconds of silence.
    int tolls = 0;
    for (int i = 0; i < 360; ++i) {
        game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
        tolls += game::advanceGateBell(bell, round, settings.gate.bellSeconds, STEP) ? 1 : 0;
    }
    CHECK(tolls == 0);

    // Every crystal is collected: the gate opens, and the bell follows two seconds later.
    for (const game::RoundCrystal& crystal : std::vector<game::RoundCrystal>(round.crystals)) {
        game::updateRound(round, world, settings, crystal.restPosition, flashlightOn, STEP);
    }
    REQUIRE(round.gateOpen);
    for (int i = 0; i < 360; ++i) {
        game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
        tolls += game::advanceGateBell(bell, round, settings.gate.bellSeconds, STEP) ? 1 : 0;
    }
    CHECK(tolls == 1);

    // The passages from the exit: 0 in the exit cell and many at the start, so the
    // bell is louder at the exit than where the night began.
    const std::vector<int> distances =
        game::passageDistances(game::roundMaze(world, round), world.exitCell);
    const auto passagesFrom = [&distances, &world](game::MazeCell cell) {
        return distances[static_cast<std::size_t>(cell.z) *
                             static_cast<std::size_t>(world.maze.width()) +
                         static_cast<std::size_t>(cell.x)];
    };
    CHECK(passagesFrom(world.exitCell) == 0);
    CHECK(game::gateBellVolume(passagesFrom(world.exitCell)) == game::GATE_BELL_NEAR_VOLUME);
    CHECK(game::gateBellVolume(passagesFrom(game::START_CELL)) <
          game::gateBellVolume(passagesFrom(world.exitCell)));

    // Walking into the exit wins the round, and the bell stops.
    game::updateRound(round, world, settings, world.exitPosition, flashlightOn, STEP);
    REQUIRE(round.state == game::RoundState::Won);
    CHECK_FALSE(game::advanceGateBell(bell, round, settings.gate.bellSeconds, 100.0F));
}
