// Tests of game::Exit: distances in a maze, the farthest cell, the gate and the exit zone.
#include "game/Exit.hpp"

#include "game/MazeGenerator.hpp"
#include "game/MazeWorld.hpp"

#include <doctest/doctest.h>

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

// The distance of one cell in the list passageDistances returns.
int distanceOf(const std::vector<int>& distances, const game::Maze& maze, int x, int z) {
    return distances[static_cast<std::size_t>(z) * static_cast<std::size_t>(maze.width()) +
                     static_cast<std::size_t>(x)];
}

// Number of sides of a cell without a wall.
int openSideCount(const game::Maze& maze, game::MazeCell cell) {
    int count = 0;
    for (const game::Direction side : game::ALL_DIRECTIONS) {
        if (!maze.hasWall(cell.x, cell.z, side)) {
            ++count;
        }
    }
    return count;
}

} // namespace

TEST_CASE("in a corridor the distance grows by one with every cell") {
    // Three cells in a row, joined into one corridor.
    game::Maze maze(3, 1);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::East);

    const std::vector<int> distances = game::passageDistances(maze, START);
    REQUIRE(distances.size() == 3U);
    CHECK(distances[0] == 0);
    CHECK(distances[1] == 1);
    CHECK(distances[2] == 2);

    // From the middle both ends are one passage away.
    const std::vector<int> fromMiddle = game::passageDistances(maze, {.x = 1, .z = 0});
    CHECK(fromMiddle[0] == 1);
    CHECK(fromMiddle[1] == 0);
    CHECK(fromMiddle[2] == 1);
}

TEST_CASE("a cell behind walls is unreachable, and an opening in the border leads nowhere") {
    // Two cells with the wall between them still standing.
    game::Maze maze(2, 1);
    // An exit hole in the outer border of the start cell: there is no cell behind it.
    maze.removeWall(0, 0, game::Direction::North);

    const std::vector<int> distances = game::passageDistances(maze, START);
    CHECK(distances[0] == 0);
    CHECK(distances[1] == game::UNREACHABLE);

    // Nothing can be reached, so the farthest cell is the start itself.
    CHECK(game::farthestCell(maze, START) == START);
}

TEST_CASE("the distance follows the passages, not the straight line") {
    // A 2 x 2 maze shaped like the letter U: (0,0) - (0,1) - (1,1) - (1,0). The cell
    // next to the start, (1, 0), is the farthest one: three passages away.
    game::Maze maze(2, 2);
    maze.removeWall(0, 0, game::Direction::South);
    maze.removeWall(0, 1, game::Direction::East);
    maze.removeWall(1, 1, game::Direction::North);

    const std::vector<int> distances = game::passageDistances(maze, START);
    CHECK(distanceOf(distances, maze, 0, 0) == 0);
    CHECK(distanceOf(distances, maze, 0, 1) == 1);
    CHECK(distanceOf(distances, maze, 1, 1) == 2);
    CHECK(distanceOf(distances, maze, 1, 0) == 3);
    CHECK(game::farthestCell(maze, START) == game::MazeCell{.x = 1, .z = 0});
}

TEST_CASE("of two cells at the same distance the first one in row order is the farthest") {
    // A plus sign without its north arm, started from the middle of the top row:
    // (0,0) - (1,0) - (2,0) with (1,1) below the middle. All three ends are one passage
    // away from the start (1, 0). Row 0 is scanned first, west to east: (0, 0) wins.
    game::Maze maze(3, 2);
    maze.removeWall(1, 0, game::Direction::West);
    maze.removeWall(1, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::South);

    CHECK(game::farthestCell(maze, {.x = 1, .z = 0}) == game::MazeCell{.x = 0, .z = 0});

    // The search itself reaches the east cell before the west one (the directions are
    // tried in the order North, East, South, West), so the answer does not come from
    // the order of the search. With the west arm closed the east end is the first.
    game::Maze withoutWest(3, 2);
    withoutWest.removeWall(1, 0, game::Direction::East);
    withoutWest.removeWall(1, 0, game::Direction::South);
    CHECK(game::farthestCell(withoutWest, {.x = 1, .z = 0}) == game::MazeCell{.x = 2, .z = 0});
}

TEST_CASE("a start outside the maze is an error") {
    const game::Maze maze(2, 2);
    CHECK_THROWS_AS(game::passageDistances(maze, {.x = 2, .z = 0}), std::out_of_range);
    CHECK_THROWS_AS(game::farthestCell(maze, {.x = 0, .z = -1}), std::out_of_range);
    CHECK_THROWS_AS(game::placeExit(maze, {.x = 5, .z = 5}), std::out_of_range);
}

TEST_CASE("golden maze: 4 x 4 cells from seed 1 has its exit in the dead end (3, 1)") {
    // The maze of the golden test in MazeGeneratorTests.cpp:
    //
    //   +--+--+--+--+
    //   |S |        |      S: the start cell (0, 0).
    //   +  +  +--+==+      E: the exit cell (3, 1), 14 passages from the start: down the
    //   |  |     |E |         west side, along the bottom row, up the east side, back
    //   +  +--+  +--+         west through the middle and around the top row.
    //   |     |     |      ==: the gate, across the north side of the exit cell.
    //   +--+  +--+  +
    //   |           |
    //   +--+--+--+--+
    const game::Maze maze = game::generateMaze(4, 4, 1U);

    const std::vector<int> distances = game::passageDistances(maze, START);
    CHECK(distanceOf(distances, maze, 0, 3) == 5);
    CHECK(distanceOf(distances, maze, 3, 3) == 6);
    CHECK(distanceOf(distances, maze, 1, 0) == 11);
    CHECK(distanceOf(distances, maze, 3, 1) == 14);

    const game::ExitPlacement exit = game::placeExit(maze, START);
    CHECK(exit.cell == game::MazeCell{.x = 3, .z = 1});
    REQUIRE(exit.hasGate);
    // The north edge of row 1 is the grid line z = 2 m, and the middle of column 3 is
    // x = 7 m. A segment on a north edge runs along X.
    checkVector(exit.gate.position, {7.0F, 0.0F, 2.0F});
    CHECK(exit.gate.axis == game::WallAxis::AlongX);
}

TEST_CASE(
    "the exit of a generated maze is a dead end, not the start, with a gate on its open side") {
    constexpr std::uint32_t SEED_COUNT = 25;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::Maze maze = game::generateMaze(9, 6, seed);
        const game::ExitPlacement exit = game::placeExit(maze, START);

        CHECK_FALSE(exit.cell == START);
        CHECK(game::isDeadEnd(maze, exit.cell.x, exit.cell.z));
        REQUIRE(openSideCount(maze, exit.cell) == 1);
        REQUIRE(exit.hasGate);

        // No cell is farther from the start than the exit.
        const std::vector<int> distances = game::passageDistances(maze, START);
        const int exitDistance = distanceOf(distances, maze, exit.cell.x, exit.cell.z);
        for (const int distance : distances) {
            CHECK(distance != game::UNREACHABLE);
            CHECK(distance <= exitDistance);
        }

        // The gate is one of the four segments around the exit cell, and it is the one
        // on the side without a wall.
        bool foundSide = false;
        for (const game::Direction side : game::ALL_DIRECTIONS) {
            const game::WallSegment segment = game::wallSegmentOn(exit.cell.x, exit.cell.z, side);
            if (segment.position == exit.gate.position && segment.axis == exit.gate.axis) {
                foundSide = true;
                CHECK_FALSE(maze.hasWall(exit.cell.x, exit.cell.z, side));
            }
        }
        CHECK(foundSide);

        // The same maze gives the same exit.
        const game::ExitPlacement again = game::placeExit(maze, START);
        CHECK(again.cell == exit.cell);
        CHECK(again.gate.position == exit.gate.position);
    }
}

TEST_CASE("a maze of one cell has its exit in the start cell and no gate") {
    const game::Maze maze(1, 1);
    const game::ExitPlacement exit = game::placeExit(maze, START);
    CHECK(exit.cell == START);
    CHECK_FALSE(exit.hasGate);
}

TEST_CASE("wallSegmentOn gives the segment on each of the four sides of a cell") {
    // Cell (2, 1): x from 4 to 6 m, z from 2 to 4 m.
    const game::WallSegment north = game::wallSegmentOn(2, 1, game::Direction::North);
    checkVector(north.position, {5.0F, 0.0F, 2.0F});
    CHECK(north.axis == game::WallAxis::AlongX);

    const game::WallSegment south = game::wallSegmentOn(2, 1, game::Direction::South);
    checkVector(south.position, {5.0F, 0.0F, 4.0F});
    CHECK(south.axis == game::WallAxis::AlongX);

    const game::WallSegment west = game::wallSegmentOn(2, 1, game::Direction::West);
    checkVector(west.position, {4.0F, 0.0F, 3.0F});
    CHECK(west.axis == game::WallAxis::AlongZ);

    const game::WallSegment east = game::wallSegmentOn(2, 1, game::Direction::East);
    checkVector(east.position, {6.0F, 0.0F, 3.0F});
    CHECK(east.axis == game::WallAxis::AlongZ);
}

TEST_CASE("the exit zone is a 1 m square in the middle of the exit cell, as high as the walls") {
    // Cell (3, 1): its centre is at x = 7 m, z = 3 m. On ground at y = 0.
    const scene::Aabb zone = game::exitZone({.x = 3, .z = 1}, 0.0F);
    checkVector(zone.min, {6.5F, 0.0F, 2.5F});
    checkVector(zone.max, {7.5F, 3.0F, 3.5F});

    // On higher ground the whole box stands higher.
    const scene::Aabb raised = game::exitZone({.x = 3, .z = 1}, 0.25F);
    checkVector(raised.min, {6.5F, 0.25F, 2.5F});
    checkVector(raised.max, {7.5F, 3.25F, 3.5F});
}

TEST_CASE("a maze world carries the exit, the gate box and the exit zone of its maze") {
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1U);

    CHECK(world.exitCell == game::MazeCell{.x = 3, .z = 1});
    checkVector(world.exitPosition, {7.0F, 0.0F, 3.0F});
    checkVector(world.exitZone.min, {6.5F, 0.0F, 2.5F});
    checkVector(world.exitZone.max, {7.5F, 3.0F, 3.5F});

    REQUIRE(world.hasGate);
    checkVector(world.gate.position, {7.0F, 0.0F, 2.0F});
    // The box of a wall along X standing there: 2 m long, 0.3 m thick, 3 m high.
    checkVector(world.gateBox.min, {6.0F, 0.0F, 1.85F});
    checkVector(world.gateBox.max, {8.0F, 3.0F, 2.15F});

    // The gate is not one of the walls and not one of the fixed obstacles: those are
    // the walls, the pillars, the stone sheep and the one box of the stile.
    for (const game::WallSegment& wall : world.walls) {
        CHECK(wall.position != world.gate.position);
    }
    CHECK(world.colliders.size() ==
          world.walls.size() + world.pillars.size() + world.sheep.size() + 1U);

    // A pillar stands at each end of the gate: the walls on the two sides of the dead
    // end stop there.
    bool westPillar = false;
    bool eastPillar = false;
    for (const glm::vec3& pillar : world.pillars) {
        westPillar = westPillar || pillar == glm::vec3{6.0F, 0.0F, 2.0F};
        eastPillar = eastPillar || pillar == glm::vec3{8.0F, 0.0F, 2.0F};
    }
    CHECK(westPillar);
    CHECK(eastPillar);
}
