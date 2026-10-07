// Tests of game/Flasks.hpp: where the flasks of tea of a maze lie.
#include "game/Flasks.hpp"

#include "game/Crystals.hpp"
#include "game/Difficulty.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

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
    // 60 crystals in a maze of 100 cells: every dead end holds one, and so do many
    // other cells.
    constexpr int MANY_CRYSTALS = 60;
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 1U, {}, MANY_CRYSTALS);
    REQUIRE(world.crystals.size() == static_cast<std::size_t>(MANY_CRYSTALS));

    const std::vector<game::MazeCell> flasks = flasksOf(world, 3);
    REQUIRE(flasks.size() == 3);
    for (const game::MazeCell cell : flasks) {
        CHECK_FALSE(game::isDeadEnd(world.maze, cell.x, cell.z));
        CHECK_FALSE(holdsCrystal(world, cell));
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
