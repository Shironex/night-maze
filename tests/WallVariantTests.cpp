// Tests of game::WallVariants: which walls of a maze look worn, which carry a crown or
// a broken coping, and which stay plain.
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
#include <numeric>
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

// True when a lever of the world opens the wall.
bool opensForALever(const game::MazeWorld& world, const game::WallSegment& wall) {
    for (const game::Lever& lever : world.interactables.levers) {
        if (hangsOn(lever.opens, wall)) {
            return true;
        }
    }
    return false;
}

// True for the three looks that are textures: cracked, mossy and damaged.
bool isPainted(game::WallVariant variant) {
    return variant == game::WallVariant::Cracked || variant == game::WallVariant::Mossy ||
           variant == game::WallVariant::Damaged;
}

// True for the two looks that are models of their own: crowned and broken.
bool isShaped(game::WallVariant variant) {
    return variant == game::WallVariant::Crowned || variant == game::WallVariant::Broken;
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
    // again when the six notes came: a wall that carries a note stays plain. They moved once more
    // when the crystals began to leave dead ends to the flasks, which moved the notes.
    // The crowned and the broken walls came out of the plain ones: the three painted
    // counts (7, 15, 8) are the ones from before those two looks existed.
    const game::MazeWorld world = worldOf(1U);
    const std::array<int, game::WALL_VARIANT_COUNT> counts =
        game::countWallVariants(world.wallVariants);
    CHECK(std::accumulate(counts.begin(), counts.end(), 0) == static_cast<int>(world.walls.size()));
    CHECK(counts == std::array<int, game::WALL_VARIANT_COUNT>{71, 7, 15, 8, 16, 4});
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
            const bool worn = isPainted(world.wallVariants[i]);
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

    // The three painted looks share the worn walls evenly: each has about a third.
    const int wornWalls = borderWorn + innerWorn;
    for (std::size_t look = 1; look <= game::PAINTED_WALL_VARIANT_COUNT; ++look) {
        CAPTURE(look);
        const double share = static_cast<double>(looks.at(look)) / wornWalls;
        CHECK(std::abs(share - 1.0 / 3.0) < TOLERANCE);
    }
}

TEST_CASE("crowned and broken walls come out of the plain walls, in the promised shares") {
    // Counted like the shares above, among the walls that may get a shaped look: not
    // near the start, carrying nothing, opened by no lever, and not painted.
    constexpr std::uint32_t SEED_COUNT = 200;
    int candidates = 0;
    int crowned = 0;
    int broken = 0;
    int walls = 0;
    int crownedOfAll = 0;
    int brokenOfAll = 0;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        const game::MazeWorld world = worldOf(seed);
        for (std::size_t i = 0; i < world.walls.size(); ++i) {
            const game::WallSegment& wall = world.walls[i];
            const game::WallVariant variant = world.wallVariants[i];
            ++walls;
            crownedOfAll += variant == game::WallVariant::Crowned ? 1 : 0;
            brokenOfAll += variant == game::WallVariant::Broken ? 1 : 0;
            if (isNearStart(world.maze, wall) || carriesSomething(world, wall) ||
                opensForALever(world, wall) || isPainted(variant)) {
                continue;
            }
            ++candidates;
            crowned += variant == game::WallVariant::Crowned ? 1 : 0;
            broken += variant == game::WallVariant::Broken ? 1 : 0;
        }
    }

    constexpr double TOLERANCE = 0.03;
    constexpr double PERCENT = 100.0;
    REQUIRE(candidates > 1000);
    const double crownedShare = static_cast<double>(crowned) / candidates;
    const double brokenShare = static_cast<double>(broken) / candidates;
    CHECK(std::abs(crownedShare - game::CROWNED_WALL_PERCENT / PERCENT) < TOLERANCE);
    CHECK(std::abs(brokenShare - game::BROKEN_WALL_PERCENT / PERCENT) < TOLERANCE);

    // Over all walls of a maze, the plain ones near the start included, that is a modest
    // share: the top edge of most walls stays the straight line it was.
    const double crownedOfAllShare = static_cast<double>(crownedOfAll) / walls;
    const double brokenOfAllShare = static_cast<double>(brokenOfAll) / walls;
    CHECK(crownedOfAllShare > 0.10);
    CHECK(crownedOfAllShare < 1.0 / 5.0);
    CHECK(brokenOfAllShare > 0.05);
    CHECK(brokenOfAllShare < 1.0 / 10.0);
}

TEST_CASE("a wall that a lever opens keeps its top whole") {
    // Such a wall sinks by a little more than its height. The twigs of a crown would
    // be left standing in the opening.
    constexpr std::uint32_t SEED_COUNT = 200;
    int opened = 0;
    int couldBeShaped = 0;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::MazeWorld world = worldOf(seed);
        REQUIRE(world.leverWalls.size() == world.interactables.levers.size());
        for (const std::size_t wall : world.leverWalls) {
            REQUIRE(wall < world.wallVariants.size());
            ++opened;
            CHECK_FALSE(isShaped(world.wallVariants[wall]));
            CHECK(opensForALever(world, world.walls[wall]));
            couldBeShaped += world.wallVariants[wall] == game::WallVariant::Plain &&
                                     !isNearStart(world.maze, world.walls[wall]) &&
                                     !carriesSomething(world, world.walls[wall])
                                 ? 1
                                 : 0;
        }
    }

    // The loop met many such walls, and many of them had no other reason to be plain.
    CHECK(opened > 100);
    CHECK(couldBeShaped > 50);
}

TEST_CASE("countWallVariants counts every look") {
    using game::WallVariant;
    const std::vector<WallVariant> variants = {
        WallVariant::Plain,   WallVariant::Mossy,  WallVariant::Damaged,
        WallVariant::Plain,   WallVariant::Mossy,  WallVariant::Plain,
        WallVariant::Crowned, WallVariant::Broken, WallVariant::Crowned};
    CHECK(game::countWallVariants(variants) ==
          std::array<int, game::WALL_VARIANT_COUNT>{3, 0, 2, 1, 2, 1});
    CHECK(game::countWallVariants({}) ==
          std::array<int, game::WALL_VARIANT_COUNT>{0, 0, 0, 0, 0, 0});
}
