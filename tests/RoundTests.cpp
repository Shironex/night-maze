// Tests of game::Round: collecting crystals, the gate, the battery, the flicker and the win.
// See docs/modules/game/gameplay.md
#include "game/Round.hpp"

#include "game/Crystals.hpp"
#include "game/Exit.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// The fixed step of the game: core::Time::FIXED_DT as a float. 120 steps are one second.
constexpr float STEP = 1.0F / 120.0F;

// A place far away from every crystal and from the exit of the mazes used here.
constexpr glm::vec3 NOWHERE{-50.0F, 0.0F, -50.0F};

// The golden maze: 4 x 4 cells from seed 1, with its exit in (3, 1) and two crystals.
game::MazeWorld goldenWorld() {
    return game::buildMazeWorld(4, 4, 1U);
}

// Where the feet of a player stand who is in the middle of the cell of a crystal.
glm::vec3 feetUnder(const game::RoundCrystal& crystal) {
    return {crystal.restPosition.x, 0.0F, crystal.restPosition.z};
}

// Runs the round for a number of steps with the player standing still.
void runSteps(game::Round& round, const game::MazeWorld& world,
              const game::GameplaySettings& settings, const glm::vec3& feet, bool& flashlightOn,
              int steps) {
    for (int i = 0; i < steps; ++i) {
        game::updateRound(round, world, settings, feet, flashlightOn, STEP);
    }
}

// Collects every crystal of the round by standing under each of them for one step.
void collectAll(game::Round& round, const game::MazeWorld& world,
                const game::GameplaySettings& settings, bool& flashlightOn) {
    for (std::size_t i = 0; i < round.crystals.size(); ++i) {
        game::updateRound(round, world, settings, feetUnder(round.crystals[i]), flashlightOn, STEP);
    }
}

} // namespace

TEST_CASE("the gameplay settings start with the agreed numbers") {
    const game::GameplaySettings settings;
    CHECK(settings.requiredFraction == 0.7F);
    CHECK(settings.batteryLifetimeSeconds == 180.0F);
    CHECK(settings.batteryPerCrystal == 0.25F);
    CHECK(settings.lowBatteryThreshold == 0.2F);
    CHECK(settings.pickupRadius == 0.6F);
    CHECK(settings.batteryDrains);
    CHECK_FALSE(settings.restart);
}

TEST_CASE("the gate needs 70 percent of the crystals, rounded up, and at least one") {
    constexpr float FRACTION = 0.7F;
    // The default maze: 13 crystals, 9.1 rounded up.
    CHECK(game::requiredCrystalCount(13, FRACTION) == 10);
    CHECK(game::requiredCrystalCount(16, FRACTION) == 12);
    CHECK(game::requiredCrystalCount(2, FRACTION) == 2);
    CHECK(game::requiredCrystalCount(1, FRACTION) == 1);

    // Where 70 percent is a whole number it must not be rounded up to the next one,
    // although a float cannot hold 0.7 exactly.
    CHECK(game::requiredCrystalCount(10, FRACTION) == 7);
    CHECK(game::requiredCrystalCount(20, FRACTION) == 14);
    CHECK(game::requiredCrystalCount(30, FRACTION) == 21);

    // No crystals: nothing is needed.
    CHECK(game::requiredCrystalCount(0, FRACTION) == 0);
}

TEST_CASE("the required count stays between 1 and the number of crystals for any fraction") {
    CHECK(game::requiredCrystalCount(13, 0.0F) == 1);
    CHECK(game::requiredCrystalCount(13, 0.01F) == 1);
    CHECK(game::requiredCrystalCount(13, 0.5F) == 7);
    CHECK(game::requiredCrystalCount(13, 1.0F) == 13);
    CHECK(game::requiredCrystalCount(13, 2.0F) == 13);
    CHECK(game::requiredCrystalCount(13, -1.0F) == 1);
}

TEST_CASE("a new round has every crystal, a full battery and a closed gate") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    const game::Round round = game::startRound(world, settings);

    CHECK(round.state == game::RoundState::Playing);
    REQUIRE(round.crystals.size() == world.crystals.size());
    REQUIRE(round.crystals.size() == 2U);
    for (std::size_t i = 0; i < round.crystals.size(); ++i) {
        CHECK_FALSE(round.crystals[i].collected);
        CHECK(round.crystals[i].variant == world.crystals[i].variant);
        // The golden world is flat: the ground is at y = 0.
        checkVector(round.crystals[i].restPosition,
                    game::crystalRestPosition(world.crystals[i].cell, 0.0F));
    }
    CHECK(round.collectedCount == 0);
    // 70 percent of 2 crystals is 1.4: both are needed.
    CHECK(round.requiredCount == 2);
    CHECK_FALSE(round.gateOpen);
    CHECK(round.gateProgress == 0.0F);
    CHECK(round.battery == 1.0F);
    CHECK(round.elapsedSeconds == 0.0F);
    CHECK(round.animationSeconds == 0.0F);

    // The closed gate is the last obstacle of the list.
    CHECK(game::gateBlocks(world, round));
    CHECK(game::gateVisible(world, round));
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    REQUIRE(obstacles.size() == world.colliders.size() + 1U);
    checkVector(obstacles.back().min, world.gateBox.min);
    checkVector(obstacles.back().max, world.gateBox.max);
}

TEST_CASE("the reach of the player is a sphere at the middle of the body") {
    const scene::Sphere reach = game::playerReach({3.0F, 0.0F, 5.0F});
    checkVector(reach.center, {3.0F, 0.9F, 5.0F});
    CHECK(reach.radius == 0.3F);
}

TEST_CASE("a crystal is collected from the middle of its cell, not from the next cell") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;
    const glm::vec3 under = feetUnder(round.crystals[0]);

    // One cell (2 m) away, in each of the four directions: too far.
    for (const glm::vec3 offset :
         {glm::vec3{game::CELL_SIZE, 0.0F, 0.0F}, glm::vec3{-game::CELL_SIZE, 0.0F, 0.0F},
          glm::vec3{0.0F, 0.0F, game::CELL_SIZE}, glm::vec3{0.0F, 0.0F, -game::CELL_SIZE}}) {
        game::updateRound(round, world, settings, under + offset, flashlightOn, STEP);
    }
    CHECK(round.collectedCount == 0);
    CHECK_FALSE(round.crystals[0].collected);

    // 0.9 m to the side is outside too: the two radii add up to 0.3 + 0.6 = 0.9 m, and
    // the crystal hangs 0.25 m above the middle of the body, so the reach along the
    // ground is a little shorter (about 0.86 m).
    game::updateRound(round, world, settings, under + glm::vec3{0.9F, 0.0F, 0.0F}, flashlightOn,
                      STEP);
    CHECK(round.collectedCount == 0);

    // Hugging a wall of the cell: the body is 0.55 m from the middle at most (1 m to
    // the wall line, minus half a wall box and half a body). Still within reach.
    game::updateRound(round, world, settings, under + glm::vec3{0.55F, 0.0F, 0.55F}, flashlightOn,
                      STEP);
    CHECK(round.collectedCount == 1);
    CHECK(round.crystals[0].collected);
    CHECK_FALSE(round.crystals[1].collected);

    // Standing there longer does not collect it again.
    runSteps(round, world, settings, under, flashlightOn, 10);
    CHECK(round.collectedCount == 1);
}

TEST_CASE("a larger pickup radius reaches a crystal from further away") {
    const game::MazeWorld world = goldenWorld();
    game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;
    const glm::vec3 nearby = feetUnder(round.crystals[0]) + glm::vec3{1.2F, 0.0F, 0.0F};

    game::updateRound(round, world, settings, nearby, flashlightOn, STEP);
    CHECK(round.collectedCount == 0);

    settings.pickupRadius = 1.0F;
    game::updateRound(round, world, settings, nearby, flashlightOn, STEP);
    CHECK(round.collectedCount == 1);
}

TEST_CASE("the battery drains only while the flashlight is on") {
    const game::MazeWorld world = goldenWorld();
    game::GameplaySettings settings;
    settings.batteryLifetimeSeconds = 100.0F;
    game::Round round = game::startRound(world, settings);

    // Ten seconds with the light off: nothing is used.
    constexpr int TEN_SECONDS = 1200;
    bool flashlightOn = false;
    runSteps(round, world, settings, NOWHERE, flashlightOn, TEN_SECONDS);
    CHECK(round.battery == 1.0F);

    // Ten seconds with the light on: a tenth of a battery that lasts 100 seconds.
    flashlightOn = true;
    runSteps(round, world, settings, NOWHERE, flashlightOn, TEN_SECONDS);
    CHECK(round.battery == doctest::Approx(0.9F).epsilon(0.001));
    CHECK(flashlightOn);

    // The switch for testing stops the drain.
    settings.batteryDrains = false;
    const float before = round.battery;
    runSteps(round, world, settings, NOWHERE, flashlightOn, TEN_SECONDS);
    CHECK(round.battery == before);
}

TEST_CASE("an empty battery switches the flashlight off and keeps it off") {
    const game::MazeWorld world = goldenWorld();
    game::GameplaySettings settings;
    settings.batteryLifetimeSeconds = 2.0F;
    game::Round round = game::startRound(world, settings);

    // Three seconds with the light on: the battery lasts two.
    constexpr int THREE_SECONDS = 360;
    bool flashlightOn = true;
    runSteps(round, world, settings, NOWHERE, flashlightOn, THREE_SECONDS);

    // Empty means exactly 0, never below it.
    CHECK(round.battery == 0.0F);
    CHECK_FALSE(flashlightOn);
    // The round goes on: darkness is not a defeat.
    CHECK(round.state == game::RoundState::Playing);

    // Switching it on again (the F key, or a checkbox of the debug UI) does not last
    // a single step.
    flashlightOn = true;
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK_FALSE(flashlightOn);
    CHECK(round.battery == 0.0F);

    // The frame is drawn without the flashlight even if the switch were still on.
    game::LightingSettings lighting;
    lighting.flashlightOn = true;
    CHECK_FALSE(game::lightingForFrame(lighting, round, settings).flashlightOn);
}

TEST_CASE("a crystal recharges the battery by a quarter, up to full") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    // From empty: the light is not switched on by the crystal, but it can be again.
    round.battery = 0.0F;
    game::updateRound(round, world, settings, feetUnder(round.crystals[0]), flashlightOn, STEP);
    CHECK(round.battery == 0.25F);
    CHECK_FALSE(flashlightOn);
    flashlightOn = true;
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(flashlightOn);

    // Nearly full: the charge stops at 1.
    flashlightOn = false;
    round.battery = 0.9F;
    game::updateRound(round, world, settings, feetUnder(round.crystals[1]), flashlightOn, STEP);
    CHECK(round.battery == 1.0F);
}

TEST_CASE("a crystal collected in the step the battery runs out keeps the light on") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = true;

    // Less charge than one step uses.
    round.battery = 0.00001F;
    game::updateRound(round, world, settings, feetUnder(round.crystals[0]), flashlightOn, STEP);
    CHECK(round.battery == 0.25F);
    CHECK(flashlightOn);
}

TEST_CASE("a battery value from outside is brought back between 0 and 1") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    round.battery = 1.7F;
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(round.battery == 1.0F);

    round.battery = -0.4F;
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(round.battery == 0.0F);
}

TEST_CASE("the gate stops blocking when enough crystals are collected and sinks in 1.5 s") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    // One of the two needed crystals: still closed, and it does not move.
    game::updateRound(round, world, settings, feetUnder(round.crystals[0]), flashlightOn, STEP);
    CHECK_FALSE(round.gateOpen);
    runSteps(round, world, settings, NOWHERE, flashlightOn, 100);
    CHECK(round.gateProgress == 0.0F);
    CHECK(game::gateSinkDepth(round) == 0.0F);

    CHECK(game::gateBlocks(world, round));
    CHECK(game::gateVisible(world, round));

    // The second one opens it. From this step on the gate is no obstacle any more,
    // although it is still at its full height. It starts to sink in the next step.
    game::updateRound(round, world, settings, feetUnder(round.crystals[1]), flashlightOn, STEP);
    CHECK(round.collectedCount == 2);
    CHECK(round.gateOpen);
    CHECK(round.gateProgress == 0.0F);
    CHECK_FALSE(game::gateBlocks(world, round));
    CHECK(game::roundObstacles(world, round).size() == world.colliders.size());
    CHECK(game::gateVisible(world, round));

    // Half of the time: half way down, still drawn.
    constexpr int HALF_OPEN_STEPS = 90;
    runSteps(round, world, settings, NOWHERE, flashlightOn, HALF_OPEN_STEPS);
    CHECK(round.gateProgress == doctest::Approx(0.5F).epsilon(0.001));
    CHECK(game::gateSinkDepth(round) ==
          doctest::Approx(game::GATE_SINK_DEPTH / 2.0F).epsilon(0.001));
    CHECK(game::gateVisible(world, round));
    CHECK_FALSE(game::gateBlocks(world, round));

    // Well after the full time: all the way down, exactly 1, and nothing left to draw.
    runSteps(round, world, settings, NOWHERE, flashlightOn, 2 * HALF_OPEN_STEPS);
    CHECK(round.gateProgress == 1.0F);
    CHECK(game::gateSinkDepth(round) == game::GATE_SINK_DEPTH);
    CHECK_FALSE(game::gateVisible(world, round));
    CHECK_FALSE(game::gateBlocks(world, round));
    CHECK(game::roundObstacles(world, round).size() == world.colliders.size());
    // The gate ends up deeper than anything of it is high.
    CHECK(game::GATE_SINK_DEPTH > game::PILLAR_HEIGHT);
}

TEST_CASE("the required count follows the fraction during a round, an open gate stays open") {
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 1U);
    game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;
    REQUIRE(round.crystals.size() == 13U);
    CHECK(round.requiredCount == 10);

    // Five crystals: not enough for 70 percent.
    for (std::size_t i = 0; i < 5; ++i) {
        game::updateRound(round, world, settings, feetUnder(round.crystals[i]), flashlightOn, STEP);
    }
    CHECK(round.collectedCount == 5);
    CHECK_FALSE(round.gateOpen);

    // The fraction is lowered in the debug UI: 30 percent of 13 is 3.9, so 4 are needed
    // and 5 are there.
    settings.requiredFraction = 0.3F;
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(round.requiredCount == 4);
    CHECK(round.gateOpen);

    // Raising it again changes the number shown, but does not close the gate.
    settings.requiredFraction = 1.0F;
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(round.requiredCount == 13);
    CHECK(round.gateOpen);
}

TEST_CASE("the round is won in the exit zone, but only while the gate is open") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    // In the exit cell with the gate closed (only noclip gets the player there).
    runSteps(round, world, settings, world.exitPosition, flashlightOn, 10);
    CHECK(round.state == game::RoundState::Playing);

    collectAll(round, world, settings, flashlightOn);
    REQUIRE(round.gateOpen);
    CHECK(round.state == game::RoundState::Playing);

    // In front of the gate, in the neighbouring cell: not yet.
    const glm::vec3 beforeGate = world.exitPosition + glm::vec3{0.0F, 0.0F, -game::CELL_SIZE};
    game::updateRound(round, world, settings, beforeGate, flashlightOn, STEP);
    CHECK(round.state == game::RoundState::Playing);

    // Just inside the exit cell, 0.15 m from the gate line: the zone starts 0.5 m in
    // and the reach is 0.3 m, so there are 5 cm missing.
    const glm::vec3 atTheEdge = world.exitPosition + glm::vec3{0.0F, 0.0F, -0.85F};
    game::updateRound(round, world, settings, atTheEdge, flashlightOn, STEP);
    CHECK(round.state == game::RoundState::Playing);

    // In the middle of the exit cell.
    game::updateRound(round, world, settings, world.exitPosition, flashlightOn, STEP);
    CHECK(round.state == game::RoundState::Won);
}

TEST_CASE("the player cannot reach the exit zone from in front of the closed gate") {
    const game::MazeWorld world = goldenWorld();

    // The gate is on the north side of the exit cell. The nearest a player can stand
    // to the exit: touching the gate box from the north.
    const float halfBody = 0.3F;
    const glm::vec3 feet{world.gate.position.x, 0.0F, world.gateBox.min.z - halfBody};
    CHECK_FALSE(scene::overlaps(game::playerReach(feet), world.exitZone));
}

TEST_CASE("the time of the round stops at the win, the animation clock goes on") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = true;

    constexpr int ONE_SECOND = 120;
    runSteps(round, world, settings, NOWHERE, flashlightOn, ONE_SECOND);
    CHECK(round.elapsedSeconds == doctest::Approx(1.0F).epsilon(0.001));
    CHECK(round.animationSeconds == doctest::Approx(1.0F).epsilon(0.001));

    // Collect both crystals and win right away, while the gate is still sinking.
    collectAll(round, world, settings, flashlightOn);
    game::updateRound(round, world, settings, world.exitPosition, flashlightOn, STEP);
    REQUIRE(round.state == game::RoundState::Won);
    const float timeAtWin = round.elapsedSeconds;
    const float batteryAtWin = round.battery;
    const float progressAtWin = round.gateProgress;
    CHECK(progressAtWin < 1.0F);

    // Two more seconds: the result stays, the scene keeps moving, the gate finishes.
    runSteps(round, world, settings, world.exitPosition, flashlightOn, 2 * ONE_SECOND);
    CHECK(round.state == game::RoundState::Won);
    CHECK(round.elapsedSeconds == timeAtWin);
    CHECK(round.battery == batteryAtWin);
    CHECK(round.animationSeconds > timeAtWin + 1.9F);
    CHECK(round.gateProgress == 1.0F);
}

TEST_CASE("a maze without crystals starts with its gate open") {
    // Two cells: the start and the exit, and no cell left for a crystal.
    const game::MazeWorld world = game::buildMazeWorld(2, 1, 0U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    REQUIRE(round.crystals.empty());
    REQUIRE(world.hasGate);
    CHECK(round.requiredCount == 0);
    CHECK(round.gateOpen);
    CHECK(round.state == game::RoundState::Playing);
    // The gate is in nobody's way from the first step, and sinks while the round runs.
    CHECK_FALSE(game::gateBlocks(world, round));
    CHECK(game::roundObstacles(world, round).size() == world.colliders.size());
    CHECK(game::gateVisible(world, round));

    // The player still has to walk there.
    game::updateRound(round, world, settings, world.startPosition, flashlightOn, STEP);
    CHECK(round.state == game::RoundState::Playing);
    game::updateRound(round, world, settings, world.exitPosition, flashlightOn, STEP);
    CHECK(round.state == game::RoundState::Won);
}

TEST_CASE("a maze of one cell has no gate, and standing in it wins the round") {
    const game::MazeWorld world = game::buildMazeWorld(1, 1, 0U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    CHECK(round.gateOpen);
    CHECK(round.gateProgress == 1.0F);
    CHECK_FALSE(game::gateBlocks(world, round));
    CHECK_FALSE(game::gateVisible(world, round));
    CHECK(game::roundObstacles(world, round).size() == world.colliders.size());

    game::updateRound(round, world, settings, world.startPosition, flashlightOn, STEP);
    CHECK(round.state == game::RoundState::Won);
}

TEST_CASE("the flashlight is steady above the low-battery threshold and dark when empty") {
    const game::GameplaySettings settings;
    constexpr int MOMENTS = 200;
    for (int i = 0; i < MOMENTS; ++i) {
        const float seconds = static_cast<float>(i) * 0.013F;
        CHECK(game::flashlightFlicker(1.0F, seconds, settings) == 1.0F);
        CHECK(game::flashlightFlicker(0.5F, seconds, settings) == 1.0F);
        CHECK(game::flashlightFlicker(settings.lowBatteryThreshold, seconds, settings) == 1.0F);
        CHECK(game::flashlightFlicker(0.0F, seconds, settings) == 0.0F);
        CHECK(game::flashlightFlicker(-0.1F, seconds, settings) == 0.0F);
    }
}

TEST_CASE("a low battery flickers: the factor stays in 0 to 1, dips, and repeats exactly") {
    const game::GameplaySettings settings;
    constexpr int MOMENTS = 2000;
    constexpr float MOMENT_SECONDS = 0.005F;

    // Deepest dip of ten seconds for two charges below the threshold of 0.2.
    float lowestAtHalf = 1.0F;
    float lowestNearEmpty = 1.0F;
    int steadyMoments = 0;
    for (int i = 0; i < MOMENTS; ++i) {
        const float seconds = static_cast<float>(i) * MOMENT_SECONDS;
        const float atHalf = game::flashlightFlicker(0.1F, seconds, settings);
        const float nearEmpty = game::flashlightFlicker(0.01F, seconds, settings);

        CHECK(atHalf >= 0.0F);
        CHECK(atHalf <= 1.0F);
        CHECK(nearEmpty >= 0.0F);
        CHECK(nearEmpty <= 1.0F);
        // The weaker battery is never the brighter one at the same moment.
        CHECK(nearEmpty <= atHalf);

        lowestAtHalf = std::min(lowestAtHalf, atHalf);
        lowestNearEmpty = std::min(lowestNearEmpty, nearEmpty);
        if (nearEmpty == 1.0F) {
            ++steadyMoments;
        }

        // No random numbers: the same charge and moment give the same factor.
        CHECK(game::flashlightFlicker(0.1F, seconds, settings) == atHalf);
    }

    // It really dips, deeper the weaker the battery, but the light never goes out
    // completely while there is charge left.
    CHECK(lowestAtHalf < 0.7F);
    CHECK(lowestNearEmpty < 0.3F);
    CHECK(lowestNearEmpty < lowestAtHalf);
    CHECK(lowestNearEmpty > 0.0F);
    // Between the dips the light is at full brightness, a good part of the time.
    CHECK(steadyMoments > MOMENTS / 4);
}

TEST_CASE("a threshold of zero means no flicker at all") {
    game::GameplaySettings settings;
    settings.lowBatteryThreshold = 0.0F;
    CHECK(game::flashlightFlicker(0.001F, 1.0F, settings) == 1.0F);
    CHECK(game::flashlightFlicker(0.0F, 1.0F, settings) == 0.0F);
}

TEST_CASE("the lighting of a frame dims the flashlight and the crystals, not the settings") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    const game::LightingSettings settings;

    // A full battery at time 0: the flashlight as set, the crystals at the pulse of
    // that moment.
    const game::LightingSettings fresh = game::lightingForFrame(settings, round, gameplay);
    CHECK(fresh.flashlightOn);
    CHECK(fresh.flashlightIntensity == settings.flashlightIntensity);
    CHECK(fresh.pointIntensity ==
          doctest::Approx(settings.pointIntensity * game::crystalPulse(0.0F)));
    // Everything else is the copy.
    CHECK(fresh.mode == settings.mode);
    CHECK(fresh.pointRadius == settings.pointRadius);
    CHECK(fresh.moonIntensity == settings.moonIntensity);

    // A low battery in the middle of a dip.
    round.battery = 0.02F;
    round.animationSeconds = 0.3F;
    const float flicker = game::flashlightFlicker(0.02F, 0.3F, gameplay);
    REQUIRE(flicker < 1.0F);
    const game::LightingSettings low = game::lightingForFrame(settings, round, gameplay);
    CHECK(low.flashlightOn);
    CHECK(low.flashlightIntensity == doctest::Approx(settings.flashlightIntensity * flicker));

    // A switch that is off stays off.
    game::LightingSettings switchedOff = settings;
    switchedOff.flashlightOn = false;
    CHECK_FALSE(game::lightingForFrame(switchedOff, round, gameplay).flashlightOn);
}

TEST_CASE("every crystal that is left carries a light, a collected one does not") {
    const game::MazeWorld world = goldenWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    // At time 0 crystal 0 is at rest: its light is 1.55 m above the centre of its cell.
    std::vector<glm::vec3> lights = game::crystalLightPositions(round);
    REQUIRE(lights.size() == 2U);
    checkVector(lights[0], game::crystalLightPosition(round.crystals[0].restPosition));
    // Each light stays above its own crystal, within the bobbing.
    for (std::size_t i = 0; i < lights.size(); ++i) {
        CHECK(lights[i].x == round.crystals[i].restPosition.x);
        CHECK(lights[i].z == round.crystals[i].restPosition.z);
        const float restHeight = game::crystalLightPosition(round.crystals[i].restPosition).y;
        CHECK(lights[i].y >= restHeight - game::CRYSTAL_BOB_AMPLITUDE - 0.0001F);
        CHECK(lights[i].y <= restHeight + game::CRYSTAL_BOB_AMPLITUDE + 0.0001F);
    }

    // The first crystal is collected: only the light of the second one is left.
    game::updateRound(round, world, settings, feetUnder(round.crystals[0]), flashlightOn, STEP);
    lights = game::crystalLightPositions(round);
    REQUIRE(lights.size() == 1U);
    CHECK(lights[0].x == round.crystals[1].restPosition.x);
    CHECK(lights[0].z == round.crystals[1].restPosition.z);

    game::updateRound(round, world, settings, feetUnder(round.crystals[1]), flashlightOn, STEP);
    CHECK(game::crystalLightPositions(round).empty());
}

TEST_CASE("a large maze has more crystals than lights, and a frame takes the nearest ones") {
    const game::MazeWorld world = game::buildMazeWorld(40, 40, 11U);
    const game::Round round = game::startRound(world, game::GameplaySettings{});
    REQUIRE(round.crystals.size() == static_cast<std::size_t>(game::MAX_CRYSTAL_COUNT));

    // Every crystal that is still there has a place for a light.
    const std::vector<glm::vec3> positions = game::crystalLightPositions(round);
    CHECK(positions.size() == round.crystals.size());

    // The frame is drawn with as many as the shader has room for, however many there are.
    const std::vector<game::PointLightSpot> chosen =
        game::nearestPointLights(positions, world.startPosition);
    CHECK(chosen.size() == static_cast<std::size_t>(scene::MAX_POINT_LIGHTS));
    const scene::LightSet lights =
        game::buildLightSet(game::LightingSettings{}, game::FlashlightPose{}, chosen);
    CHECK(lights.pointCount == scene::MAX_POINT_LIGHTS);
}
