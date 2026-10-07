// Tests of game::WallVariants: which walls of a maze look worn, and which stay plain.
#include "game/WallVariants.hpp"

#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace {

// The maze the tests look at: the size of the default maze.
constexpr int SIZE = 10;

// A world on flat ground, built the way the game builds it. Its walls stand at y = 0,
// exactly as game::wallSegments lists them.
game::MazeWorld worldOf(std::uint32_t seed) {
    return game::buildMazeWorld(SIZE, SIZE, seed);
}

// The two cells a wall stands between, found from its place: half a cell to each side.
std::array<game::MazeCell, 2> cellsBeside(const game::WallSegment& wall) {
    const glm::vec3 across = wall.axis == game::WallAxis::AlongX
                                 ? glm::vec3{0.0F, 0.0F, game::CELL_SIZE / 2.0F}
                                 : glm::vec3{game::CELL_SIZE / 2.0F, 0.0F, 0.0F};
    return {game::cellAt(wall.position - across), game::cellAt(wall.position + across)};
}

// True when the wall is part of the outer border: one of its two sides is no cell.
bool isBorder(const game::Maze& maze, const game::WallSegment& wall) {
    const std::array<game::MazeCell, 2> cells = cellsBeside(wall);
    return !maze.contains(cells[0].x, cells[0].z) || !maze.contains(cells[1].x, cells[1].z);
}

// True when a cell beside the wall is within the clearance of the start cell.
bool isNearStart(const game::Maze& maze, const game::WallSegment& wall) {
    for (const game::MazeCell cell : cellsBeside(wall)) {
        const int columns = std::abs(cell.x - game::START_CELL.x);
        const int rows = std::abs(cell.z - game::START_CELL.z);
        if (maze.contains(cell.x, cell.z) &&
            std::max(columns, rows) <= game::WALL_VARIANT_START_CLEARANCE) {
            return true;
        }
    }
    return false;
}

// True when the lever or note with this mount hangs on the wall.
bool hangsOn(const game::WallRef& mount, const game::WallSegment& wall) {
    const game::WallSegment mounted = game::wallSegmentOn(mount.cell.x, mount.cell.z, mount.side);
    return mounted.position == wall.position && mounted.axis == wall.axis;
}

// True when a lever or a note of the world hangs on the wall.
bool carriesSomething(const game::MazeWorld& world, const game::WallSegment& wall) {
    for (const game::Lever& lever : world.interactables.levers) {
        if (hangsOn(lever.mount, wall)) {
            return true;
        }
    }
    for (const game::Note& note : world.interactables.notes) {
        if (hangsOn(note.mount, wall)) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE("every wall of a world has a look") {
    const game::MazeWorld world = worldOf(1U);
    CHECK(world.wallVariants.size() == world.walls.size());
}

TEST_CASE("the same seed gives the same wall looks, another seed gives others") {
    const game::MazeWorld first = worldOf(7U);
    const game::MazeWorld again = worldOf(7U);
    CHECK(first.wallVariants == again.wallVariants);

    // Called directly, with the parts of the world, it gives what the world holds.
    CHECK(game::chooseWallVariants(first.maze, 7U, game::START_CELL, first.interactables) ==
          first.wallVariants);

    const game::MazeWorld other = worldOf(8U);
    CHECK(first.wallVariants != other.wallVariants);
}

TEST_CASE("golden wall looks: the counts of seed 1 are the same on every system") {
    // The numbers come from std::mt19937 and randomBelow alone, so these counts must be
    // the same on Windows and on macOS. They were written down from a run on Windows, and
    // again when the six notes came: a wall that carries a note stays plain.
    const game::MazeWorld world = worldOf(1U);
    const std::array<int, game::WALL_VARIANT_COUNT> counts =
        game::countWallVariants(world.wallVariants);
    CHECK(counts[0] + counts[1] + counts[2] + counts[3] == static_cast<int>(world.walls.size()));
    CHECK(counts == std::array<int, game::WALL_VARIANT_COUNT>{93, 7, 13, 8});
}

TEST_CASE("walls near the start and walls that carry a lever or a note stay plain") {
    constexpr std::uint32_t SEED_COUNT = 40;
    int nearStart = 0;
    int carrying = 0;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::MazeWorld world = worldOf(seed);
        REQUIRE(world.wallVariants.size() == world.walls.size());

        for (std::size_t i = 0; i < world.walls.size(); ++i) {
            CAPTURE(i);
            if (isNearStart(world.maze, world.walls[i])) {
                ++nearStart;
                CHECK(world.wallVariants[i] == game::WallVariant::Plain);
            }
            if (carriesSomething(world, world.walls[i])) {
                ++carrying;
                CHECK(world.wallVariants[i] == game::WallVariant::Plain);
            }
        }
    }

    // The loops above really met such walls: a test that checks nothing proves nothing.
    CHECK(nearStart > 0);
    CHECK(carrying > 0);
}

TEST_CASE("the looks of the other walls do not depend on where the notes hang") {
    // The same maze with no levers and no notes: only the walls that carried something
    // may differ, because every wall draws its numbers whether it is used or not.
    constexpr std::uint32_t SEED = 3;
    const game::MazeWorld world = worldOf(SEED);
    const game::MazeWorld bare = game::buildMazeWorld(
        SIZE, SIZE, SEED, game::InteractableSettings{.leverCount = 0, .noteCount = 0});
    REQUIRE(bare.wallVariants.size() == world.wallVariants.size());

    for (std::size_t i = 0; i < world.walls.size(); ++i) {
        CAPTURE(i);
        if (!carriesSomething(world, world.walls[i])) {
            CHECK(world.wallVariants[i] == bare.wallVariants[i]);
        }
    }
}

TEST_CASE("the border is worn more often than the inside, in the promised shares") {
    // Counted over many mazes, and only among the walls that may be worn at all: the
    // walls near the start and the ones that carry something are left out, or they
    // would pull the shares below the two constants.
    constexpr std::uint32_t SEED_COUNT = 200;
    int borderWalls = 0;
    int borderWorn = 0;
    int innerWalls = 0;
    int innerWorn = 0;
    std::array<int, game::WALL_VARIANT_COUNT> looks{};

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        const game::MazeWorld world = worldOf(seed);
        for (std::size_t i = 0; i < world.walls.size(); ++i) {
            const game::WallSegment& wall = world.walls[i];
            if (isNearStart(world.maze, wall) || carriesSomething(world, wall)) {
                continue;
            }
            const bool worn = world.wallVariants[i] != game::WallVariant::Plain;
            if (isBorder(world.maze, wall)) {
                ++borderWalls;
                borderWorn += worn ? 1 : 0;
            } else {
                ++innerWalls;
                innerWorn += worn ? 1 : 0;
            }
            ++looks.at(static_cast<std::size_t>(world.wallVariants[i]));
        }
    }

    // Thousands of walls of each kind, so the shares land within 3 walls out of 100 of
    // the constants.
    constexpr double TOLERANCE = 0.03;
    constexpr double PERCENT = 100.0;
    REQUIRE(borderWalls > 1000);
    REQUIRE(innerWalls > 1000);
    const double borderShare = static_cast<double>(borderWorn) / borderWalls;
    const double innerShare = static_cast<double>(innerWorn) / innerWalls;
    CHECK(std::abs(borderShare - game::BORDER_WALL_VARIANT_PERCENT / PERCENT) < TOLERANCE);
    CHECK(std::abs(innerShare - game::INNER_WALL_VARIANT_PERCENT / PERCENT) < TOLERANCE);
    CHECK(borderShare > innerShare);

    // The three worn looks share the worn walls evenly: each has about a third.
    const int wornWalls = borderWorn + innerWorn;
    for (std::size_t look = 1; look < game::WALL_VARIANT_COUNT; ++look) {
        CAPTURE(look);
        const double share = static_cast<double>(looks.at(look)) / wornWalls;
        CHECK(std::abs(share - 1.0 / 3.0) < TOLERANCE);
    }
}

TEST_CASE("countWallVariants counts every look") {
    using game::WallVariant;
    const std::vector<WallVariant> variants = {WallVariant::Plain,   WallVariant::Mossy,
                                               WallVariant::Damaged, WallVariant::Plain,
                                               WallVariant::Mossy,   WallVariant::Plain};
    CHECK(game::countWallVariants(variants) ==
          std::array<int, game::WALL_VARIANT_COUNT>{3, 0, 2, 1});
    CHECK(game::countWallVariants({}) == std::array<int, game::WALL_VARIANT_COUNT>{0, 0, 0, 0});
}
