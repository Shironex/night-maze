// Tests of game::Maze and of the helpers for game::Direction.
// See docs/modules/game/maze-generator.md
#include "game/Maze.hpp"

#include <doctest/doctest.h>

#include <stdexcept>

TEST_CASE("the direction helpers follow the compass: North is -Z, East is +X") {
    CHECK(game::columnStep(game::Direction::North) == 0);
    CHECK(game::rowStep(game::Direction::North) == -1);
    CHECK(game::columnStep(game::Direction::East) == 1);
    CHECK(game::rowStep(game::Direction::East) == 0);
    CHECK(game::columnStep(game::Direction::South) == 0);
    CHECK(game::rowStep(game::Direction::South) == 1);
    CHECK(game::columnStep(game::Direction::West) == -1);
    CHECK(game::rowStep(game::Direction::West) == 0);

    CHECK(game::opposite(game::Direction::North) == game::Direction::South);
    CHECK(game::opposite(game::Direction::East) == game::Direction::West);
    CHECK(game::opposite(game::Direction::South) == game::Direction::North);
    CHECK(game::opposite(game::Direction::West) == game::Direction::East);
}

TEST_CASE("a new maze has the given size and every wall") {
    const game::Maze maze(3, 2);

    CHECK(maze.width() == 3);
    CHECK(maze.height() == 2);
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            for (const game::Direction side : game::ALL_DIRECTIONS) {
                CHECK(maze.hasWall(x, z, side));
            }
        }
    }
}

TEST_CASE("contains tells cells of the maze from everything else") {
    const game::Maze maze(3, 2);

    CHECK(maze.contains(0, 0));
    CHECK(maze.contains(2, 1));
    CHECK_FALSE(maze.contains(-1, 0));
    CHECK_FALSE(maze.contains(0, -1));
    CHECK_FALSE(maze.contains(3, 0));
    CHECK_FALSE(maze.contains(0, 2));
}

TEST_CASE("a maze with a wrong size cannot be created") {
    CHECK_THROWS_AS(game::Maze(0, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(5, 0), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(-3, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(game::Maze::MAX_SIZE + 1, 5), std::invalid_argument);
    CHECK_THROWS_AS(game::Maze(5, game::Maze::MAX_SIZE + 1), std::invalid_argument);

    // The limits themselves are fine.
    CHECK_NOTHROW(game::Maze(1, 1));
    CHECK_NOTHROW(game::Maze(game::Maze::MAX_SIZE, game::Maze::MAX_SIZE));
}

TEST_CASE("removing a wall removes it for both cells that share it") {
    game::Maze maze(3, 2);

    SUBCASE("east side of a cell is the west side of its right neighbour") {
        maze.removeWall(0, 0, game::Direction::East);
        CHECK_FALSE(maze.hasWall(0, 0, game::Direction::East));
        CHECK_FALSE(maze.hasWall(1, 0, game::Direction::West));
    }

    SUBCASE("north side of a cell is the south side of the cell above it") {
        maze.removeWall(2, 1, game::Direction::North);
        CHECK_FALSE(maze.hasWall(2, 1, game::Direction::North));
        CHECK_FALSE(maze.hasWall(2, 0, game::Direction::South));
    }

    SUBCASE("no other wall changes") {
        maze.removeWall(1, 0, game::Direction::South);
        int wallCount = 0;
        for (int z = 0; z < maze.height(); ++z) {
            for (int x = 0; x < maze.width(); ++x) {
                for (const game::Direction side : game::ALL_DIRECTIONS) {
                    if (maze.hasWall(x, z, side)) {
                        ++wallCount;
                    }
                }
            }
        }
        // 6 cells with 4 sides each, minus the two sides of the one removed wall.
        CHECK(wallCount == 22);
    }

    SUBCASE("a border wall has no second cell") {
        maze.removeWall(0, 0, game::Direction::West);
        CHECK_FALSE(maze.hasWall(0, 0, game::Direction::West));
        // The cell next to it keeps its own west wall.
        CHECK(maze.hasWall(1, 0, game::Direction::West));
    }
}

TEST_CASE("asking about a cell outside the maze throws") {
    game::Maze maze(3, 2);

    CHECK_THROWS_AS(maze.hasWall(3, 0, game::Direction::North), std::out_of_range);
    CHECK_THROWS_AS(maze.hasWall(0, 2, game::Direction::North), std::out_of_range);
    CHECK_THROWS_AS(maze.hasWall(-1, 0, game::Direction::North), std::out_of_range);
    CHECK_THROWS_AS(maze.removeWall(0, -1, game::Direction::South), std::out_of_range);
}

TEST_CASE("a cell is a dead end when it has exactly three walls") {
    // A new maze has every wall: four per cell, no dead end.
    game::Maze maze(2, 1);
    CHECK_FALSE(game::isDeadEnd(maze, 0, 0));

    // One passage between the two cells: both are closed on three sides.
    maze.removeWall(0, 0, game::Direction::East);
    CHECK(game::isDeadEnd(maze, 0, 0));
    CHECK(game::isDeadEnd(maze, 1, 0));

    // A second opening turns the cell into a corridor.
    maze.removeWall(1, 0, game::Direction::South);
    CHECK(game::isDeadEnd(maze, 0, 0));
    CHECK_FALSE(game::isDeadEnd(maze, 1, 0));

    CHECK_THROWS_AS(game::isDeadEnd(maze, 2, 0), std::out_of_range);
}

TEST_CASE("two maze cells are equal when column and row match") {
    CHECK(game::MazeCell{.x = 2, .z = 3} == game::MazeCell{.x = 2, .z = 3});
    CHECK_FALSE(game::MazeCell{.x = 2, .z = 3} == game::MazeCell{.x = 3, .z = 2});
    // A cell created without numbers is the north-west corner.
    CHECK(game::MazeCell{} == game::MazeCell{.x = 0, .z = 0});
}
