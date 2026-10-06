// Tests of game::Crystals: how many crystals a maze gets, where they are and how they move.
// See docs/modules/game/gameplay.md
#include "game/Crystals.hpp"

#include "game/Exit.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "scene/Light.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// The start cell of every maze of the game (see MazeWorld.cpp).
constexpr game::MazeCell START{.x = 0, .z = 0};

// The crystals of a generated maze, placed the way buildMazeWorld places them.
std::vector<game::CrystalSpawn> crystalsOf(const game::Maze& maze, std::uint32_t seed) {
    return game::placeCrystals(maze, seed, START, game::farthestCell(maze, START));
}

// Number of dead ends of a maze other than the two given cells.
std::size_t countFreeDeadEnds(const game::Maze& maze, game::MazeCell start, game::MazeCell exit) {
    std::size_t count = 0;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const game::MazeCell cell{.x = x, .z = z};
            if (!(cell == start) && !(cell == exit) && game::isDeadEnd(maze, x, z)) {
                ++count;
            }
        }
    }
    return count;
}

} // namespace

TEST_CASE("a maze gets one crystal for every eight cells, between 1 and 64") {
    CHECK(game::CELLS_PER_CRYSTAL == 8);

    // The default maze: 100 / 8 = 12.5, rounded to the nearest whole number.
    CHECK(game::crystalCountFor(100) == 13);
    CHECK(game::crystalCountFor(96) == 12);
    CHECK(game::crystalCountFor(16) == 2);
    // 11 / 8 = 1.375 rounds down, 12 / 8 = 1.5 rounds up.
    CHECK(game::crystalCountFor(11) == 1);
    CHECK(game::crystalCountFor(12) == 2);

    // Never fewer than one.
    CHECK(game::crystalCountFor(1) == 1);
    CHECK(game::crystalCountFor(3) == 1);

    // More than the shader has point lights for: only the nearest crystals carry
    // a light in a frame (game::nearestPointLights).
    CHECK(scene::MAX_POINT_LIGHTS == 16);
    CHECK(game::crystalCountFor(128) == 16);
    CHECK(game::crystalCountFor(16 * 16) == 32);

    // Never more than the largest number of crystals.
    CHECK(game::MAX_CRYSTAL_COUNT == 64);
    CHECK(game::crystalCountFor(40 * 40) == 64);
    CHECK(game::crystalCountFor(256 * 256) == 64);
}

TEST_CASE("a maze gets the number of crystals that was asked for") {
    const game::Maze maze = game::generateMaze(10, 10, 5U);
    const game::MazeCell exit = game::farthestCell(maze, START);

    CHECK(game::placeCrystals(maze, 5U, START, exit, 20).size() == 20U);
    CHECK(game::placeCrystals(maze, 5U, START, exit, 0).empty());
    // Left out, the size of the maze decides: 13 for 100 cells.
    CHECK(game::placeCrystals(maze, 5U, START, exit).size() == 13U);
    CHECK(game::placeCrystals(maze, 5U, START, exit, game::CRYSTAL_COUNT_FROM_SIZE).size() == 13U);

    // A number outside 0 to MAX_CRYSTAL_COUNT is brought into that range.
    CHECK(game::placeCrystals(maze, 5U, START, exit, -7).empty());
    CHECK(game::placeCrystals(maze, 5U, START, exit, 1000).size() ==
          static_cast<std::size_t>(game::MAX_CRYSTAL_COUNT));

    // Never more than the maze has free cells: 9 cells without the start and the exit.
    const game::Maze small = game::generateMaze(3, 3, 5U);
    CHECK(game::placeCrystals(small, 5U, START, game::farthestCell(small, START), 30).size() == 7U);
}

TEST_CASE("asking for more crystals keeps the first ones where they were") {
    const game::Maze maze = game::generateMaze(12, 12, 9U);
    const game::MazeCell exit = game::farthestCell(maze, START);

    const std::vector<game::CrystalSpawn> few = game::placeCrystals(maze, 9U, START, exit, 6);
    const std::vector<game::CrystalSpawn> many = game::placeCrystals(maze, 9U, START, exit, 30);

    REQUIRE(few.size() == 6U);
    REQUIRE(many.size() == 30U);
    for (std::size_t i = 0; i < few.size(); ++i) {
        CHECK(few[i].cell == many[i].cell);
    }
}

TEST_CASE("golden maze: 4 x 4 cells from seed 1 has exactly these two crystals") {
    // The maze of the golden test in MazeGeneratorTests.cpp. 16 cells give 2 crystals.
    //
    //   +--+--+--+--+
    //   |S |        |      S: the start cell (0, 0), E: the exit cell (3, 1). Both are
    //   +  +  +--+  +         dead ends and both stay empty.
    //   |  |2    |E |      1: the first crystal, in the only free dead end (0, 3).
    //   +  +--+  +--+      2: the second crystal: the dead ends have run out, so it is
    //   |     |     |         in one of the other cells, chosen by the seed: (1, 1).
    //   +--+  +--+  +
    //   |1          |
    //   +--+--+--+--+
    const game::Maze maze = game::generateMaze(4, 4, 1U);
    REQUIRE(game::farthestCell(maze, START) == game::MazeCell{.x = 3, .z = 1});

    const std::vector<game::CrystalSpawn> crystals = crystalsOf(maze, 1U);

    // Wrong numbers here mean the placement changed, or that it differs on this
    // compiler: the same seed must give the same crystals on macOS and on Windows.
    REQUIRE(crystals.size() == 2U);
    CHECK(crystals[0].cell == game::MazeCell{.x = 0, .z = 3});
    CHECK(crystals[0].variant == 0);
    CHECK(crystals[1].cell == game::MazeCell{.x = 1, .z = 1});
    CHECK(crystals[1].variant == 1);
}

TEST_CASE("the default maze has 13 crystals and its exit in the cell (6, 5)") {
    const game::MazeWorld world = game::buildMazeWorld(
        game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, game::DEFAULT_MAZE_SEED);
    CHECK(world.crystals.size() == 13U);
    CHECK(world.exitCell == game::MazeCell{.x = 6, .z = 5});
    // The gate closes the east side of the exit cell: the grid line x = 14 m.
    REQUIRE(world.hasGate);
    checkVector(world.gate.position, {14.0F, 0.0F, 11.0F});
    CHECK(world.gate.axis == game::WallAxis::AlongZ);
}

TEST_CASE("crystals are in different cells, never in the start or the exit cell") {
    constexpr std::uint32_t SEED_COUNT = 30;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::Maze maze = game::generateMaze(9, 7, seed);
        const game::MazeCell exit = game::farthestCell(maze, START);
        const std::vector<game::CrystalSpawn> crystals = crystalsOf(maze, seed);

        // 63 cells: 63 / 8 = 7.875, rounded to 8.
        REQUIRE(crystals.size() == 8U);

        for (std::size_t i = 0; i < crystals.size(); ++i) {
            const game::MazeCell cell = crystals[i].cell;
            CHECK(maze.contains(cell.x, cell.z));
            CHECK_FALSE(cell == START);
            CHECK_FALSE(cell == exit);
            CHECK(crystals[i].variant >= 0);
            CHECK(crystals[i].variant < game::CRYSTAL_VARIANT_COUNT);
            // No two crystals share a cell.
            for (std::size_t j = i + 1; j < crystals.size(); ++j) {
                CHECK_FALSE(cell == crystals[j].cell);
            }
        }
    }
}

TEST_CASE("the dead ends are filled before any other cell gets a crystal") {
    constexpr std::uint32_t SEED_COUNT = 30;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::Maze maze = game::generateMaze(9, 7, seed);
        const game::MazeCell exit = game::farthestCell(maze, START);
        const std::vector<game::CrystalSpawn> crystals = crystalsOf(maze, seed);
        const std::size_t freeDeadEnds = countFreeDeadEnds(maze, START, exit);

        // The first crystals are in dead ends, as many as there are dead ends (or
        // crystals, whichever is fewer). The ones after them are in other cells.
        for (std::size_t i = 0; i < crystals.size(); ++i) {
            CAPTURE(i);
            const bool inDeadEnd = game::isDeadEnd(maze, crystals[i].cell.x, crystals[i].cell.z);
            CHECK(inDeadEnd == (i < freeDeadEnds));
        }
    }
}

TEST_CASE("both crystal models are used") {
    // 50 crystals (400 cells, one for every 8) with a variant drawn from the seed each:
    // all the same is possible in principle, but not for the seeds pinned here.
    const game::Maze maze = game::generateMaze(20, 20, 3U);
    const std::vector<game::CrystalSpawn> crystals = crystalsOf(maze, 3U);
    REQUIRE(crystals.size() == 50U);

    int firstVariant = 0;
    int secondVariant = 0;
    for (const game::CrystalSpawn& crystal : crystals) {
        if (crystal.variant == 0) {
            ++firstVariant;
        } else {
            ++secondVariant;
        }
    }
    CHECK(firstVariant > 0);
    CHECK(secondVariant > 0);
}

TEST_CASE("the same maze and seed always give the same crystals, another seed gives others") {
    const game::Maze maze = game::generateMaze(12, 12, 7U);

    const std::vector<game::CrystalSpawn> first = crystalsOf(maze, 7U);
    const std::vector<game::CrystalSpawn> second = crystalsOf(maze, 7U);
    REQUIRE(first.size() == second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].cell == second[i].cell);
        CHECK(first[i].variant == second[i].variant);
    }

    // The same walls with another seed for the crystals: the order of the dead ends is
    // shuffled differently, so at least one crystal is somewhere else.
    const std::vector<game::CrystalSpawn> other = crystalsOf(maze, 8U);
    REQUIRE(other.size() == first.size());
    bool differs = false;
    for (std::size_t i = 0; i < first.size(); ++i) {
        differs = differs || !(first[i].cell == other[i].cell);
    }
    CHECK(differs);
}

TEST_CASE("a maze with too few free cells gets fewer crystals, down to none") {
    // One cell: it is the start and the exit.
    const game::Maze oneCell(1, 1);
    CHECK(game::placeCrystals(oneCell, 0U, START, START).empty());

    // Two cells: the start and the exit.
    const game::Maze twoCells = game::generateMaze(2, 1, 0U);
    CHECK(crystalsOf(twoCells, 0U).empty());

    // Three cells in a row: the start, one free cell and the exit.
    const game::Maze threeCells = game::generateMaze(3, 1, 0U);
    const std::vector<game::CrystalSpawn> crystals = crystalsOf(threeCells, 0U);
    REQUIRE(crystals.size() == 1U);
    CHECK(crystals[0].cell == game::MazeCell{.x = 1, .z = 0});
}

TEST_CASE("a start or an exit outside the maze is an error") {
    const game::Maze maze(3, 3);
    CHECK_THROWS_AS(game::placeCrystals(maze, 0U, {.x = 3, .z = 0}, START), std::out_of_range);
    CHECK_THROWS_AS(game::placeCrystals(maze, 0U, START, {.x = 0, .z = -1}), std::out_of_range);
}

TEST_CASE("a crystal rests above the centre of its cell, its light just above its tip") {
    CHECK(game::CRYSTAL_HEIGHT == 0.5F);
    CHECK(game::CRYSTAL_FLOAT_HEIGHT == 0.9F);

    // Cell (3, 1): its centre is at x = 7 m, z = 3 m. On ground at y = 0.
    const glm::vec3 rest = game::crystalRestPosition({.x = 3, .z = 1}, 0.0F);
    checkVector(rest, {7.0F, 0.9F, 3.0F});
    // On higher ground the crystal floats just as far above it.
    checkVector(game::crystalRestPosition({.x = 3, .z = 1}, 0.25F), {7.0F, 1.15F, 3.0F});
    checkVector(game::crystalCenter(rest), {7.0F, 1.15F, 3.0F});
    // 0.9 m to the base, 0.5 m of crystal and 0.15 m of free space.
    checkVector(game::crystalLightPosition(rest), {7.0F, 1.55F, 3.0F});
    CHECK(game::crystalLightPosition(rest).y > rest.y + game::CRYSTAL_HEIGHT);
}

TEST_CASE("a crystal bobs straight up and down within its amplitude") {
    const glm::vec3 rest{7.0F, 0.9F, 3.0F};
    constexpr int STEPS = 600;
    constexpr float STEP_SECONDS = 0.01F;

    float lowest = rest.y;
    float highest = rest.y;
    for (int step = 0; step < STEPS; ++step) {
        const glm::vec3 position =
            game::crystalBobPosition(rest, 0, static_cast<float>(step) * STEP_SECONDS);
        CHECK(position.x == rest.x);
        CHECK(position.z == rest.z);
        CHECK(position.y >= rest.y - game::CRYSTAL_BOB_AMPLITUDE - 0.0001F);
        CHECK(position.y <= rest.y + game::CRYSTAL_BOB_AMPLITUDE + 0.0001F);
        lowest = std::min(lowest, position.y);
        highest = std::max(highest, position.y);
    }
    // In six seconds (two cycles) it really gets near both ends.
    CHECK(highest > rest.y + 0.9F * game::CRYSTAL_BOB_AMPLITUDE);
    CHECK(lowest < rest.y - 0.9F * game::CRYSTAL_BOB_AMPLITUDE);

    // Crystal 0 starts at rest and is back there after one full cycle.
    checkVector(game::crystalBobPosition(rest, 0, 0.0F), rest);
    CHECK(game::crystalBobPosition(rest, 0, game::CRYSTAL_BOB_SECONDS).y ==
          doctest::Approx(rest.y).epsilon(0.001));
    // A quarter of a cycle later it is at the top.
    CHECK(game::crystalBobPosition(rest, 0, game::CRYSTAL_BOB_SECONDS / 4.0F).y ==
          doctest::Approx(rest.y + game::CRYSTAL_BOB_AMPLITUDE));

    // The next crystal in the list is at another point of the movement.
    CHECK(game::crystalBobPosition(rest, 1, 0.0F).y != doctest::Approx(rest.y));
}

TEST_CASE("a crystal turns 40 degrees per second and its angle stays below 360") {
    CHECK(game::crystalSpinDegrees(0, 0.0F) == doctest::Approx(0.0F));
    CHECK(game::crystalSpinDegrees(0, 1.0F) == doctest::Approx(40.0F));
    CHECK(game::crystalSpinDegrees(0, 4.5F) == doctest::Approx(180.0F));
    // 10 seconds are 400 degrees: one full turn and 40 more.
    CHECK(game::crystalSpinDegrees(0, 10.0F) == doctest::Approx(40.0F).epsilon(0.001));

    constexpr int STEPS = 500;
    for (int step = 0; step < STEPS; ++step) {
        const float angle = game::crystalSpinDegrees(3, static_cast<float>(step) * 0.37F);
        CHECK(angle >= 0.0F);
        CHECK(angle < 360.0F);
    }
}

TEST_CASE("the pulse of the crystals stays between 70 and 100 percent") {
    constexpr int STEPS = 480;
    constexpr float STEP_SECONDS = 0.01F;

    float lowest = 1.0F;
    float highest = 0.0F;
    for (int step = 0; step < STEPS; ++step) {
        const float pulse = game::crystalPulse(static_cast<float>(step) * STEP_SECONDS);
        CHECK(pulse >= 1.0F - game::CRYSTAL_PULSE_DEPTH - 0.0001F);
        CHECK(pulse <= 1.0F);
        lowest = std::min(lowest, pulse);
        highest = std::max(highest, pulse);
    }
    // In 4.8 seconds (two cycles) it gets near both ends.
    CHECK(lowest < 0.71F);
    CHECK(highest > 0.99F);

    // The same moment always gives the same brightness.
    CHECK(game::crystalPulse(1.234F) == game::crystalPulse(1.234F));
}

TEST_CASE("the glow of a crystal has the colour of its light and pulses with it") {
    const glm::vec3 lightColor{0.2F, 0.9F, 0.8F};

    // Time 0 is the middle of the pulse: 85 percent of the full glow.
    const float pulse = game::crystalPulse(0.0F);
    CHECK(pulse == doctest::Approx(0.85F));
    checkVector(game::crystalGlow(lightColor, 0.0F),
                lightColor * game::CRYSTAL_GLOW_STRENGTH * pulse);

    // A quarter of a cycle later the light is at its dimmest, three quarters at its
    // brightest. The glow follows.
    const float dimmest = game::CRYSTAL_PULSE_SECONDS / 4.0F;
    const float brightest = 3.0F * game::CRYSTAL_PULSE_SECONDS / 4.0F;
    CHECK(game::crystalPulse(dimmest) == doctest::Approx(0.7F));
    CHECK(game::crystalPulse(brightest) == doctest::Approx(1.0F));
    CHECK(game::crystalGlow(lightColor, dimmest).g < game::crystalGlow(lightColor, brightest).g);
    checkVector(game::crystalGlow(lightColor, brightest), lightColor * game::CRYSTAL_GLOW_STRENGTH);
}
