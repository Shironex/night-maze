// Tests of the flasks of tea: where the flasks of a maze lie (game/Flasks.hpp) and how
// a round picks them up (game/Round.hpp). What the tea does to the stamina is tested
// in PlayerTests.cpp.
#include "game/Flasks.hpp"

#include "game/Crystals.hpp"
#include "game/Difficulty.hpp"
#include "game/Exit.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Round.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float. 120 steps are one second.
constexpr float STEP = 1.0F / 120.0F;

// A place far away from everything in the mazes used here.
constexpr glm::vec3 NOWHERE{-50.0F, 0.0F, -50.0F};

// The maze of the Normal level: 16 x 16 cells with 26 crystals.
game::MazeWorld normalWorld(std::uint32_t seed) {
    const game::DifficultyLevel& level = game::difficultyLevel(game::Difficulty::Normal);
    return game::buildMazeWorld(level.mazeWidth, level.mazeHeight, seed, {}, level.crystalCount);
}

// The flasks of a world, as a round places them.
std::vector<game::MazeCell> flasksOf(const game::MazeWorld& world, int count) {
    return game::placeFlasks(world.maze, world.seed, game::START_CELL, world.exitCell,
                             world.crystals, count);
}

bool holdsCrystal(const game::MazeWorld& world, game::MazeCell cell) {
    return std::ranges::any_of(
        world.crystals, [cell](const game::CrystalSpawn& crystal) { return crystal.cell == cell; });
}

// Where the feet of a player stand who is in the middle of a cell, on flat ground.
glm::vec3 feetIn(game::MazeCell cell) {
    return game::cellCenter(cell.x, cell.z);
}

} // namespace

TEST_CASE("a maze gets the number of flasks that was asked for, from 0 to 8") {
    const game::MazeWorld world = normalWorld(1U);
    CHECK(flasksOf(world, 0).empty());
    CHECK(flasksOf(world, 1).size() == 1);
    CHECK(flasksOf(world, 3).size() == 3);
    // Numbers outside the range are brought into it.
    CHECK(flasksOf(world, -5).empty());
    CHECK(flasksOf(world, 100).size() == static_cast<std::size_t>(game::MAX_FLASK_COUNT));
}

TEST_CASE("flasks lie in dead ends that hold no crystal, never in the start or the exit") {
    // Five crystals in a maze of 256 cells: most dead ends are empty, and there are
    // more of them than a maze can have flasks.
    constexpr int FEW_CRYSTALS = 5;
    constexpr std::uint32_t SEED_COUNT = 20;
    for (std::uint32_t seed = 1; seed <= SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::MazeWorld world = game::buildMazeWorld(16, 16, seed, {}, FEW_CRYSTALS);
        const std::vector<game::MazeCell> flasks = flasksOf(world, game::MAX_FLASK_COUNT);
        REQUIRE(flasks.size() == static_cast<std::size_t>(game::MAX_FLASK_COUNT));
        for (const game::MazeCell cell : flasks) {
            CHECK(game::isDeadEnd(world.maze, cell.x, cell.z));
            CHECK_FALSE(holdsCrystal(world, cell));
            CHECK_FALSE(cell == game::START_CELL);
            CHECK_FALSE(cell == world.exitCell);
        }
    }
}

TEST_CASE("every free dead end gets a flask before any other cell does") {
    // The mazes of the game. The crystals take the dead ends first, so often few of
    // them are left, or none: the flasks then go on in other cells.
    constexpr std::uint32_t SEED_COUNT = 20;
    for (std::uint32_t seed = 1; seed <= SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::MazeWorld world = normalWorld(seed);

        // How many dead ends a flask may lie in.
        std::size_t freeDeadEnds = 0;
        for (int z = 0; z < world.maze.height(); ++z) {
            for (int x = 0; x < world.maze.width(); ++x) {
                const game::MazeCell cell{.x = x, .z = z};
                if (game::isDeadEnd(world.maze, x, z) && !holdsCrystal(world, cell) &&
                    !(cell == game::START_CELL) && !(cell == world.exitCell)) {
                    ++freeDeadEnds;
                }
            }
        }

        // The first flasks of the list are in those dead ends, as many as there are,
        // and only the ones after them are somewhere else.
        const std::vector<game::MazeCell> flasks = flasksOf(world, game::MAX_FLASK_COUNT);
        REQUIRE(flasks.size() == static_cast<std::size_t>(game::MAX_FLASK_COUNT));
        for (std::size_t i = 0; i < flasks.size(); ++i) {
            CHECK(game::isDeadEnd(world.maze, flasks[i].x, flasks[i].z) == (i < freeDeadEnds));
            CHECK_FALSE(holdsCrystal(world, flasks[i]));
            CHECK_FALSE(flasks[i] == game::START_CELL);
            CHECK_FALSE(flasks[i] == world.exitCell);
        }
    }
}

TEST_CASE("on every level the flasks lie in dead ends, apart from crystals, start and exit") {
    // The world keeps the crystals out of the dead ends the flasks use, so this holds
    // for all three levels and many seeds, and the level still has all its crystals.
    constexpr std::uint32_t SEED_COUNT = 200;
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        for (std::uint32_t seed = 1; seed <= SEED_COUNT; ++seed) {
            CAPTURE(level.name);
            CAPTURE(seed);
            const game::MazeWorld world = game::buildMazeWorld(level.mazeWidth, level.mazeHeight,
                                                               seed, {}, level.crystalCount);
            CHECK(world.crystals.size() == static_cast<std::size_t>(level.crystalCount));

            const std::vector<game::MazeCell> flasks = flasksOf(world, level.flaskCount);
            REQUIRE(flasks.size() == static_cast<std::size_t>(level.flaskCount));
            for (const game::MazeCell cell : flasks) {
                CHECK(game::isDeadEnd(world.maze, cell.x, cell.z));
                CHECK_FALSE(holdsCrystal(world, cell));
                CHECK_FALSE(cell == game::START_CELL);
                CHECK_FALSE(cell == world.exitCell);
            }
        }
    }
}

TEST_CASE("the dead ends of the flasks are far from the start first, and the same every time") {
    const game::MazeWorld world = normalWorld(3U);
    const std::vector<game::MazeCell> ends =
        game::flaskDeadEnds(world.maze, world.seed, game::START_CELL, world.exitCell);
    CHECK(ends == game::flaskDeadEnds(world.maze, world.seed, game::START_CELL, world.exitCell));
    REQUIRE(ends.size() > 3);

    const std::vector<int> distances = game::passageDistances(world.maze, game::START_CELL);
    const int farthest = *std::ranges::max_element(distances);
    // Once a near dead end has come, no far one follows.
    bool nearSeen = false;
    for (const game::MazeCell cell : ends) {
        const int index = cell.z * world.maze.width() + cell.x;
        const bool far = distances[static_cast<std::size_t>(index)] * 2 >= farthest;
        CHECK_FALSE((nearSeen && far));
        nearSeen = nearSeen || !far;
    }
    // The start and the exit are never in the list.
    CHECK(std::ranges::find(ends, game::START_CELL) == ends.end());
    CHECK(std::ranges::find(ends, world.exitCell) == ends.end());
}

TEST_CASE("no two flasks share a cell") {
    const game::MazeWorld world = normalWorld(7U);
    const std::vector<game::MazeCell> flasks = flasksOf(world, game::MAX_FLASK_COUNT);
    for (std::size_t i = 0; i < flasks.size(); ++i) {
        for (std::size_t j = i + 1; j < flasks.size(); ++j) {
            CHECK_FALSE(flasks[i] == flasks[j]);
        }
    }
}

TEST_CASE("without a free dead end the flasks take other cells that hold no crystal") {
    // Crystals placed without reserving anything: 60 of them in a maze of 100 cells,
    // so every dead end holds one, and so do many other cells. (In a built world the
    // crystals leave the dead ends of the flasks alone.)
    constexpr int MANY_CRYSTALS = 60;
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 1U, {}, 0);
    const std::vector<game::CrystalSpawn> crystals =
        game::placeCrystals(world.maze, 1U, game::START_CELL, world.exitCell, MANY_CRYSTALS);
    REQUIRE(crystals.size() == static_cast<std::size_t>(MANY_CRYSTALS));

    const std::vector<game::MazeCell> flasks =
        game::placeFlasks(world.maze, world.seed, game::START_CELL, world.exitCell, crystals, 3);
    REQUIRE(flasks.size() == 3);
    for (const game::MazeCell cell : flasks) {
        CHECK_FALSE(game::isDeadEnd(world.maze, cell.x, cell.z));
        CHECK_FALSE(std::ranges::any_of(
            crystals, [cell](const game::CrystalSpawn& crystal) { return crystal.cell == cell; }));
        CHECK_FALSE(cell == game::START_CELL);
        CHECK_FALSE(cell == world.exitCell);
    }
}
TEST_CASE("a maze with too few free cells gets fewer flasks, down to none") {
    // Two cells: the start and the exit. Nothing is free.
    const game::MazeWorld tiny = game::buildMazeWorld(2, 1, 1U);
    CHECK(flasksOf(tiny, 3).empty());

    // Four cells with a crystal in each free one: no room either.
    const game::MazeWorld full = game::buildMazeWorld(2, 2, 1U, {}, 2);
    REQUIRE(full.crystals.size() == 2);
    CHECK(flasksOf(full, 3).empty());

    // The same maze without crystals: two free cells, so two flasks at most.
    const game::MazeWorld empty = game::buildMazeWorld(2, 2, 1U, {}, 0);
    CHECK(flasksOf(empty, 3).size() == 2);
}

TEST_CASE("the same maze and seed always give the same flasks, another seed gives others") {
    const std::vector<game::MazeCell> first = flasksOf(normalWorld(3U), 3);
    const std::vector<game::MazeCell> again = flasksOf(normalWorld(3U), 3);
    CHECK(first == again);
    CHECK_FALSE(first == flasksOf(normalWorld(4U), 3));
}

TEST_CASE("asking for more flasks keeps the first ones where they were") {
    const game::MazeWorld world = normalWorld(5U);
    const std::vector<game::MazeCell> two = flasksOf(world, 2);
    const std::vector<game::MazeCell> five = flasksOf(world, 5);
    REQUIRE(five.size() == 5);
    CHECK(two[0] == five[0]);
    CHECK(two[1] == five[1]);
}

TEST_CASE("a flask rests above the centre of its cell, low over the ground") {
    constexpr game::MazeCell CELL{.x = 2, .z = 3};
    constexpr float GROUND = 1.5F;
    const glm::vec3 rest = game::flaskRestPosition(CELL, GROUND);
    const glm::vec3 center = game::cellCenter(CELL.x, CELL.z);
    CHECK(rest.x == doctest::Approx(center.x));
    CHECK(rest.z == doctest::Approx(center.z));
    CHECK(rest.y == doctest::Approx(GROUND + game::FLASK_FLOAT_HEIGHT));
    // Lower than a crystal: something left on the ground.
    CHECK(game::FLASK_FLOAT_HEIGHT < game::CRYSTAL_FLOAT_HEIGHT);
}

TEST_CASE("the three levels have 1, 2 and 3 flasks") {
    CHECK(game::difficultyLevel(game::Difficulty::Easy).flaskCount == 1);
    CHECK(game::difficultyLevel(game::Difficulty::Normal).flaskCount == 2);
    CHECK(game::difficultyLevel(game::Difficulty::Hard).flaskCount == 3);
    // The rules start with the number of the easy level, like their other numbers.
    CHECK(game::GameplaySettings{}.flaskCount == 1);
}

TEST_CASE("placing flasks does not move the crystals or the notes of a seed") {
    // The flasks are placed when a round starts, with a generator of their own, after
    // the world is built. Whatever number of flasks a round asks for, the world it was
    // started on is the world of a game without flasks.
    for (const std::uint32_t seed : {1U, 2U, 42U, 2026U}) {
        CAPTURE(seed);
        const game::MazeWorld without = normalWorld(seed);
        const game::MazeWorld with = normalWorld(seed);
        game::GameplaySettings settings;
        settings.flaskCount = 0;
        const game::Round roundWithout = game::startRound(without, settings);
        settings.flaskCount = 3;
        const game::Round roundWith = game::startRound(with, settings);
        REQUIRE(roundWithout.flasks.empty());
        REQUIRE(roundWith.flasks.size() == 3);

        REQUIRE(with.crystals.size() == without.crystals.size());
        for (std::size_t i = 0; i < with.crystals.size(); ++i) {
            CHECK(with.crystals[i].cell == without.crystals[i].cell);
            CHECK(with.crystals[i].variant == without.crystals[i].variant);
        }
        REQUIRE(with.interactables.notes.size() == without.interactables.notes.size());
        for (std::size_t i = 0; i < with.interactables.notes.size(); ++i) {
            CHECK(with.interactables.notes[i].mount == without.interactables.notes[i].mount);
            CHECK(with.interactables.notes[i].kind == without.interactables.notes[i].kind);
        }
        REQUIRE(with.interactables.levers.size() == without.interactables.levers.size());
        for (std::size_t i = 0; i < with.interactables.levers.size(); ++i) {
            CHECK(with.interactables.levers[i].mount == without.interactables.levers[i].mount);
        }
        // And the crystals of the two rounds rest in the same places.
        for (std::size_t i = 0; i < roundWith.crystals.size(); ++i) {
            CHECK(roundWith.crystals[i].restPosition == roundWithout.crystals[i].restPosition);
        }
    }
}

TEST_CASE("a new round has the flasks of its settings, none of them collected") {
    const game::MazeWorld world = normalWorld(1U);
    game::GameplaySettings settings;
    settings.flaskCount = 2;
    const game::Round round = game::startRound(world, settings);

    const std::vector<game::MazeCell> cells = flasksOf(world, 2);
    REQUIRE(round.flasks.size() == 2);
    CHECK(round.flasks[0].cell == cells[0]);
    CHECK(round.flasks[1].cell == cells[1]);
    CHECK_FALSE(round.flasks[0].collected);
    CHECK_FALSE(round.flasks[1].collected);
    CHECK(round.flasksCollected == 0);
}

TEST_CASE("a flask is picked up by walking into its cell, once") {
    const game::MazeWorld world = normalWorld(1U);
    game::GameplaySettings settings;
    settings.flaskCount = 2;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = true;

    // Standing somewhere else picks up nothing.
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(round.flasksCollected == 0);

    const glm::vec3 feet = feetIn(round.flasks[0].cell);
    game::updateRound(round, world, settings, feet, flashlightOn, STEP);
    CHECK(round.flasks[0].collected);
    CHECK_FALSE(round.flasks[1].collected);
    CHECK(round.flasksCollected == 1);

    // Staying there does not pick it up a second time.
    game::updateRound(round, world, settings, feet, flashlightOn, STEP);
    CHECK(round.flasksCollected == 1);

    // A flask is no crystal: it does not count for the gate and charges no battery.
    CHECK(round.collectedCount == 0);
}

TEST_CASE("a new round puts the flasks back") {
    const game::MazeWorld world = normalWorld(1U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = true;
    REQUIRE(round.flasks.size() == 1);
    game::updateRound(round, world, settings, feetIn(round.flasks[0].cell), flashlightOn, STEP);
    REQUIRE(round.flasksCollected == 1);

    // What the application does for R, "Restart maze" and "Play again".
    round = game::startRound(world, settings);
    CHECK(round.flasksCollected == 0);
    REQUIRE(round.flasks.size() == 1);
    CHECK_FALSE(round.flasks[0].collected);
}
#include <cstdio>
