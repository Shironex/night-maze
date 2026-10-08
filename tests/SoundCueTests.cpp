// Tests of game/SoundCues.hpp: the table of the cues, the cue of the flashlight key, the
// cues of a fixed step (with the one of a flask), the clock of the low battery pulse and
// the clock of the breathing of a winded player.
#include "game/SoundCues.hpp"

#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Round.hpp"

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
    CHECK(game::soundCueIndex(game::SoundCue::Caught) == game::SOUND_CUE_COUNT - 1);
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
    const game::GameplaySettings settings;
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
        caught = game::updateRoundShade(round, world, settings, feet, {}, obstacles, STEP);
        CHECK(round.shade.wayMetres <= before + 0.001F);
        before = round.shade.wayMetres;
        CHECK(game::shadeHumSounds(round) == (round.shade.wayMetres <= game::SHADE_HUM_DISTANCE));
        hummed = hummed || game::shadeHumSounds(round);
    }
    CHECK(caught);
    CHECK(hummed);
    CHECK(round.shade.wayMetres <= settings.shade.catchDistance);
}
