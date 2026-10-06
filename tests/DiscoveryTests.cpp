// Tests of game::Discovery: which cells the player has seen, the line of sight rule and
// how a round starts and updates it.
// See docs/modules/renderer/minimap.md
#include "game/Discovery.hpp"

#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Round.hpp"

#include <doctest/doctest.h>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float.
constexpr float STEP = 1.0F / 120.0F;

// A corridor of length cells from west to east in one row: every wall between two
// neighbouring cells is removed, the walls around it stay.
game::Maze corridor(int length) {
    game::Maze maze(length, 1);
    for (int x = 0; x < length - 1; ++x) {
        maze.removeWall(x, 0, game::Direction::East);
    }
    return maze;
}

// A maze of 5 by 5 cells with two corridors that cross in the middle cell (2, 2): all of
// row 2 and all of column 2. Every other wall stays.
game::Maze crossroads() {
    constexpr int SIZE = 5;
    constexpr int MIDDLE = 2;
    game::Maze maze(SIZE, SIZE);
    for (int i = 0; i < SIZE - 1; ++i) {
        maze.removeWall(i, MIDDLE, game::Direction::East);
        maze.removeWall(MIDDLE, i, game::Direction::South);
    }
    return maze;
}

// True when both grids have the same size and the same cells discovered.
bool sameCells(const game::Discovery& a, const game::Discovery& b) {
    if (a.width() != b.width() || a.height() != b.height()) {
        return false;
    }
    for (int z = 0; z < a.height(); ++z) {
        for (int x = 0; x < a.width(); ++x) {
            if (a.isDiscovered(x, z) != b.isDiscovered(x, z)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

TEST_CASE("a new discovery grid has no cell discovered") {
    const game::Discovery discovery(3, 2);

    CHECK(discovery.width() == 3);
    CHECK(discovery.height() == 2);
    CHECK(discovery.count() == 0);
    for (int z = 0; z < discovery.height(); ++z) {
        for (int x = 0; x < discovery.width(); ++x) {
            CHECK_FALSE(discovery.isDiscovered(x, z));
        }
    }

    // A grid that was never given a size has no cells at all.
    const game::Discovery empty;
    CHECK(empty.width() == 0);
    CHECK(empty.height() == 0);
    CHECK_FALSE(empty.contains(0, 0));
    CHECK_FALSE(empty.isDiscovered(0, 0));

    // A size below 0 is taken as 0.
    const game::Discovery negative(-2, 3);
    CHECK(negative.width() == 0);
    CHECK_FALSE(negative.contains(0, 0));
}

TEST_CASE("discover counts a cell once and ignores cells outside the grid") {
    game::Discovery discovery(3, 2);

    discovery.discover(2, 1);
    CHECK(discovery.isDiscovered(2, 1));
    CHECK(discovery.count() == 1);

    // A second time changes nothing.
    discovery.discover(2, 1);
    CHECK(discovery.count() == 1);

    // Outside: ignored when written and "not discovered" when read.
    discovery.discover(-1, 0);
    discovery.discover(3, 0);
    discovery.discover(0, 2);
    CHECK(discovery.count() == 1);
    CHECK_FALSE(discovery.isDiscovered(-1, 0));
    CHECK_FALSE(discovery.isDiscovered(3, 0));
    CHECK_FALSE(discovery.isDiscovered(0, 2));

    // The neighbours of the discovered cell are untouched.
    CHECK_FALSE(discovery.isDiscovered(1, 1));
    CHECK_FALSE(discovery.isDiscovered(2, 0));
}

TEST_CASE("a straight corridor is revealed up to the wall and not beyond") {
    // Five cells in a row. The corridor covers the first four, a wall closes it before
    // the fifth.
    game::Maze maze(5, 1);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::East);
    maze.removeWall(2, 0, game::Direction::East);
    game::Discovery discovery(maze.width(), maze.height());

    game::discoverFrom(discovery, maze, {.x = 0, .z = 0});

    CHECK(discovery.isDiscovered(0, 0));
    CHECK(discovery.isDiscovered(1, 0));
    CHECK(discovery.isDiscovered(2, 0));
    CHECK(discovery.isDiscovered(3, 0));
    CHECK_FALSE(discovery.isDiscovered(4, 0));
    CHECK(discovery.count() == 4);
}

TEST_CASE("nothing is discovered through a wall") {
    // Nine cells with every wall: each cell is a closed room.
    const game::Maze maze(3, 3);
    game::Discovery discovery(maze.width(), maze.height());

    game::discoverFrom(discovery, maze, {.x = 1, .z = 1});

    CHECK(discovery.isDiscovered(1, 1));
    CHECK(discovery.count() == 1);
}

TEST_CASE("the view does not go around a corner: the cell behind it needs a visit") {
    // An L: from (0, 0) east to (1, 0), then south to (1, 1).
    game::Maze maze(2, 2);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::South);
    game::Discovery discovery(maze.width(), maze.height());

    game::discoverFrom(discovery, maze, {.x = 0, .z = 0});
    CHECK(discovery.isDiscovered(0, 0));
    CHECK(discovery.isDiscovered(1, 0));
    CHECK_FALSE(discovery.isDiscovered(1, 1));
    CHECK_FALSE(discovery.isDiscovered(0, 1));

    // Standing in the corner cell, the second leg of the L is in line.
    game::discoverFrom(discovery, maze, {.x = 1, .z = 0});
    CHECK(discovery.isDiscovered(1, 1));
    CHECK_FALSE(discovery.isDiscovered(0, 1));
    CHECK(discovery.count() == 3);
}

TEST_CASE("the view reaches equally far in all four directions") {
    const game::Maze maze = crossroads();
    game::Discovery discovery(maze.width(), maze.height());

    game::discoverFrom(discovery, maze, {.x = 2, .z = 2});

    // All of row 2 and all of column 2, and nothing else: 5 + 5 cells, the middle one
    // counted once.
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const bool onTheCross = x == 2 || z == 2;
            CHECK(discovery.isDiscovered(x, z) == onTheCross);
        }
    }
    CHECK(discovery.count() == 9);
}

TEST_CASE("a side passage is seen only from a cell in line with it") {
    const game::Maze maze = crossroads();
    game::Discovery discovery(maze.width(), maze.height());

    // From the west end of row 2 the whole row is in sight, but of column 2 only the
    // cell where the two corridors cross.
    game::discoverFrom(discovery, maze, {.x = 0, .z = 2});

    CHECK(discovery.count() == 5);
    CHECK(discovery.isDiscovered(4, 2));
    CHECK_FALSE(discovery.isDiscovered(2, 1));
    CHECK_FALSE(discovery.isDiscovered(2, 3));
}

TEST_CASE("an open side on the border of the maze ends the view without an error") {
    game::Maze maze = corridor(2);
    // A hole in the outer wall, as an exit would be.
    maze.removeWall(0, 0, game::Direction::West);
    maze.removeWall(1, 0, game::Direction::East);
    game::Discovery discovery(maze.width(), maze.height());

    CHECK_NOTHROW(game::discoverFrom(discovery, maze, {.x = 0, .z = 0}));
    CHECK(discovery.count() == 2);
}

TEST_CASE("a cell or a position outside the maze discovers nothing") {
    const game::Maze maze = corridor(3);
    game::Discovery discovery(maze.width(), maze.height());

    game::discoverFrom(discovery, maze, {.x = -1, .z = 0});
    game::discoverFrom(discovery, maze, {.x = 3, .z = 0});
    game::discoverFrom(discovery, maze, {.x = 0, .z = 1});
    CHECK(discovery.count() == 0);

    // Half a metre west of the maze, north of it, and exactly on its east and south
    // edges, which belong to no cell.
    const float eastEdge = 3.0F * game::CELL_SIZE;
    const float southEdge = game::CELL_SIZE;
    game::discoverAround(discovery, maze, {-0.5F, 0.0F, 1.0F});
    game::discoverAround(discovery, maze, {1.0F, 0.0F, -0.5F});
    game::discoverAround(discovery, maze, {eastEdge, 0.0F, 1.0F});
    game::discoverAround(discovery, maze, {1.0F, 0.0F, southEdge});
    CHECK(discovery.count() == 0);
}

TEST_CASE("a position inside the maze discovers from its cell, whatever its height") {
    // Two rooms without a way between them.
    const game::Maze maze(2, 1);
    game::Discovery discovery(maze.width(), maze.height());

    // A point in the second cell, 40 m above the ground (a player flying in noclip).
    game::discoverAround(discovery, maze, {3.2F, 40.0F, 0.4F});

    CHECK(discovery.isDiscovered(1, 0));
    CHECK_FALSE(discovery.isDiscovered(0, 0));
}

TEST_CASE("a maze of one cell is discovered completely from its only cell") {
    const game::Maze maze(1, 1);
    game::Discovery discovery(maze.width(), maze.height());

    game::discoverFrom(discovery, maze, {.x = 0, .z = 0});

    CHECK(discovery.isDiscovered(0, 0));
    CHECK(discovery.count() == 1);
}

TEST_CASE("a wall that is removed later opens the view from the next call on") {
    game::Maze maze(3, 1);
    maze.removeWall(0, 0, game::Direction::East);
    game::Discovery discovery(maze.width(), maze.height());

    game::discoverFrom(discovery, maze, {.x = 0, .z = 0});
    CHECK_FALSE(discovery.isDiscovered(2, 0));

    // What a lever will do: the wall before the last cell goes away during the round.
    maze.removeWall(1, 0, game::Direction::East);
    game::discoverFrom(discovery, maze, {.x = 0, .z = 0});
    CHECK(discovery.isDiscovered(2, 0));
}

TEST_CASE("a grid of another size than the maze is filled as far as it reaches") {
    const game::Maze maze = corridor(4);
    game::Discovery discovery(2, 1);

    CHECK_NOTHROW(game::discoverFrom(discovery, maze, {.x = 0, .z = 0}));
    CHECK(discovery.count() == 2);
}

TEST_CASE("a round starts with what can be seen from the start cell") {
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 1U);
    const game::Round round = game::startRound(world, game::GameplaySettings{});

    // The same cells as the rule gives for the start cell (0, 0).
    game::Discovery expected(world.maze.width(), world.maze.height());
    game::discoverFrom(expected, world.maze, {.x = 0, .z = 0});
    CHECK(sameCells(round.discovery, expected));

    CHECK(round.discovery.isDiscovered(0, 0));
    // The start cell of a generated maze has an open side, so at least one more cell is
    // in sight. And the view from one cell covers at most one row and one column.
    CHECK(round.discovery.count() >= 2);
    CHECK(round.discovery.count() <= world.maze.width() + world.maze.height() - 1);
}

TEST_CASE("a round that was not started has nothing discovered") {
    const game::Round round;
    CHECK(round.discovery.count() == 0);
    CHECK_FALSE(round.discovery.isDiscovered(0, 0));
}

TEST_CASE("walking discovers cells, and a new round forgets them") {
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 1U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    const int atStart = round.discovery.count();
    bool flashlightOn = true;

    // One step in the middle of the exit cell: it is discovered, with what is in line.
    const game::MazeCell exitCell = world.exitCell;
    game::updateRound(round, world, settings, game::cellCenter(exitCell.x, exitCell.z),
                      flashlightOn, STEP);
    CHECK(round.discovery.isDiscovered(exitCell.x, exitCell.z));

    // One step in every cell: the whole maze is known.
    for (int z = 0; z < world.maze.height(); ++z) {
        for (int x = 0; x < world.maze.width(); ++x) {
            game::updateRound(round, world, settings, game::cellCenter(x, z), flashlightOn, STEP);
        }
    }
    CHECK(round.discovery.count() == world.maze.width() * world.maze.height());

    // A new round on the same maze starts from the start cell again.
    const game::Round restarted = game::startRound(world, settings);
    CHECK(restarted.discovery.count() == atStart);
    CHECK(atStart < world.maze.width() * world.maze.height());
}

TEST_CASE("a step outside the maze discovers nothing") {
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    const game::Discovery before = round.discovery;
    bool flashlightOn = true;

    game::updateRound(round, world, settings, {-50.0F, 0.0F, -50.0F}, flashlightOn, STEP);

    CHECK(sameCells(round.discovery, before));
}

TEST_CASE("the discovery goes on after the round is won") {
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 1U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    round.state = game::RoundState::Won;
    bool flashlightOn = true;

    // The cell in the far corner is not in line with the start cell of this maze.
    constexpr int LAST = 9;
    REQUIRE_FALSE(round.discovery.isDiscovered(LAST, LAST));
    game::updateRound(round, world, settings, game::cellCenter(LAST, LAST), flashlightOn, STEP);
    CHECK(round.discovery.isDiscovered(LAST, LAST));
}

TEST_CASE("the round of a maze of one cell knows its only cell from the start") {
    const game::MazeWorld world = game::buildMazeWorld(1, 1, 1U);
    const game::Round round = game::startRound(world, game::GameplaySettings{});

    CHECK(round.discovery.width() == 1);
    CHECK(round.discovery.height() == 1);
    CHECK(round.discovery.isDiscovered(0, 0));
    CHECK(round.discovery.count() == 1);
}
