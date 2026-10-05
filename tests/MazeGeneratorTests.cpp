// Tests of game::randomBelow and game::generateMaze.
// See docs/modules/game/maze-generator.md
#include "game/MazeGenerator.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// A maze size used by the tests that run over many mazes.
struct Size {
    int width;
    int height;
};

// Small and large, square and not, and the degenerate ones: a single cell, one row, one
// column.
constexpr std::array<Size, 8> SIZES = {
    Size{.width = 1, .height = 1},   Size{.width = 1, .height = 7},   Size{.width = 7, .height = 1},
    Size{.width = 2, .height = 2},   Size{.width = 5, .height = 3},   Size{.width = 3, .height = 5},
    Size{.width = 16, .height = 16}, Size{.width = 31, .height = 20},
};

// Every size above is generated with the seeds 0 to SEED_COUNT - 1.
constexpr std::uint32_t SEED_COUNT = 25;

// Number of passages: removed walls between two cells. Each one is counted once, from
// the cell on its west or north side.
int countPassages(const game::Maze& maze) {
    int passages = 0;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            if (x + 1 < maze.width() && !maze.hasWall(x, z, game::Direction::East)) {
                ++passages;
            }
            if (z + 1 < maze.height() && !maze.hasWall(x, z, game::Direction::South)) {
                ++passages;
            }
        }
    }
    return passages;
}

// Flood fill: starting in cell (0, 0), walk through every passage and count the cells
// that can be reached.
int countReachableCells(const game::Maze& maze) {
    const auto indexOf = [&maze](int x, int z) {
        return static_cast<std::size_t>(z) * static_cast<std::size_t>(maze.width()) +
               static_cast<std::size_t>(x);
    };

    std::vector<bool> reached(
        static_cast<std::size_t>(maze.width()) * static_cast<std::size_t>(maze.height()), false);
    std::vector<std::array<int, 2>> toVisit = {{0, 0}};
    reached[indexOf(0, 0)] = true;
    int count = 0;

    while (!toVisit.empty()) {
        const int x = toVisit.back()[0];
        const int z = toVisit.back()[1];
        toVisit.pop_back();
        ++count;

        for (const game::Direction direction : game::ALL_DIRECTIONS) {
            const int nextX = x + game::columnStep(direction);
            const int nextZ = z + game::rowStep(direction);
            if (maze.hasWall(x, z, direction) || !maze.contains(nextX, nextZ) ||
                reached[indexOf(nextX, nextZ)]) {
                continue;
            }
            reached[indexOf(nextX, nextZ)] = true;
            toVisit.push_back({nextX, nextZ});
        }
    }
    return count;
}

// True when both mazes have the same size and the same walls.
bool sameWalls(const game::Maze& a, const game::Maze& b) {
    if (a.width() != b.width() || a.height() != b.height()) {
        return false;
    }
    for (int z = 0; z < a.height(); ++z) {
        for (int x = 0; x < a.width(); ++x) {
            for (const game::Direction side : game::ALL_DIRECTIONS) {
                if (a.hasWall(x, z, side) != b.hasWall(x, z, side)) {
                    return false;
                }
            }
        }
    }
    return true;
}

// Draws the maze as text, one line of walls and one line of cells per row:
//
//   +--+--+
//   |     |      "+" is a grid corner, "--" a wall along X, "|" a wall along Z.
//   +  +--+      North (-Z) is at the top, East (+X) on the right.
//   |     |
//   +--+--+
std::string drawMaze(const game::Maze& maze) {
    std::string text;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            text += maze.hasWall(x, z, game::Direction::North) ? "+--" : "+  ";
        }
        text += "+\n";
        for (int x = 0; x < maze.width(); ++x) {
            text += maze.hasWall(x, z, game::Direction::West) ? "|  " : "   ";
        }
        text += maze.hasWall(maze.width() - 1, z, game::Direction::East) ? "|\n" : " \n";
    }
    for (int x = 0; x < maze.width(); ++x) {
        text += maze.hasWall(x, maze.height() - 1, game::Direction::South) ? "+--" : "+  ";
    }
    text += "+\n";
    return text;
}

} // namespace

TEST_CASE("std::mt19937 gives the sequence the C++ standard promises") {
    // The standard fixes this number: the 10000th value of a default constructed
    // std::mt19937 is 4123659995 on every conforming implementation. The cross-platform
    // promise of the maze generator stands on that guarantee.
    constexpr int CALL_COUNT = 10000;
    constexpr std::uint32_t EXPECTED_VALUE = 4123659995U;

    std::mt19937 generator;
    std::uint32_t value = 0;
    for (int i = 0; i < CALL_COUNT; ++i) {
        value = static_cast<std::uint32_t>(generator());
    }
    CHECK(value == EXPECTED_VALUE);
}

TEST_CASE("randomBelow stays below the bound") {
    std::mt19937 generator(7U);

    SUBCASE("bound 1 leaves only zero") {
        for (int i = 0; i < 100; ++i) {
            CHECK(game::randomBelow(generator, 1U) == 0U);
        }
    }

    SUBCASE("every value below the bound turns up, and nothing else") {
        constexpr std::uint32_t BOUND = 3;
        constexpr int DRAW_COUNT = 3000;
        std::array<int, BOUND> hits{};
        for (int i = 0; i < DRAW_COUNT; ++i) {
            const std::uint32_t value = game::randomBelow(generator, BOUND);
            REQUIRE(value < BOUND);
            ++hits[value];
        }
        // A fair pick gives each value about a third of the draws. The margin is wide:
        // this is a check against a broken helper, not a statistics test.
        for (const int count : hits) {
            CHECK(count > DRAW_COUNT / 4);
            CHECK(count < DRAW_COUNT / 2);
        }
    }

    SUBCASE("bound 0 is rejected") {
        CHECK_THROWS_AS(game::randomBelow(generator, 0U), std::invalid_argument);
    }
}

TEST_CASE("randomBelow gives the same numbers on every system") {
    // Golden values: the first numbers below 3 for seed 1. They follow from the
    // standardised output of std::mt19937 (1791095845, 4282876139, 3093770124, ...) and
    // from plain integer arithmetic, so the same numbers have to come out on Windows
    // (MSVC) and on macOS (clang). A failure here means that the helper, and with it every
    // generated maze, depends on the compiler.
    constexpr std::uint32_t SEED = 1;
    constexpr std::uint32_t BOUND = 3;
    constexpr std::array<std::uint32_t, 12> EXPECTED = {1, 2, 0, 2, 1, 1, 2, 2, 2, 0, 2, 0};

    std::mt19937 generator(SEED);
    for (const std::uint32_t expected : EXPECTED) {
        CHECK(game::randomBelow(generator, BOUND) == expected);
    }
}

TEST_CASE("generateMaze makes a perfect maze for every size and seed") {
    for (const Size size : SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            const game::Maze maze = game::generateMaze(size.width, size.height, seed);
            const int cellCount = size.width * size.height;

            CHECK(maze.width() == size.width);
            CHECK(maze.height() == size.height);

            // Every cell can be reached from the first one.
            CHECK(countReachableCells(maze) == cellCount);

            // Exactly one passage less than cells: connected and without a loop. One
            // passage more would close a loop, one less would cut a cell off.
            CHECK(countPassages(maze) == cellCount - 1);
        }
    }
}

TEST_CASE("generateMaze keeps the outer border closed") {
    for (const Size size : SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            const game::Maze maze = game::generateMaze(size.width, size.height, seed);

            for (int x = 0; x < maze.width(); ++x) {
                CHECK(maze.hasWall(x, 0, game::Direction::North));
                CHECK(maze.hasWall(x, maze.height() - 1, game::Direction::South));
            }
            for (int z = 0; z < maze.height(); ++z) {
                CHECK(maze.hasWall(0, z, game::Direction::West));
                CHECK(maze.hasWall(maze.width() - 1, z, game::Direction::East));
            }
        }
    }
}

TEST_CASE("neighbouring cells of a generated maze agree about the wall between them") {
    for (const Size size : SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            const game::Maze maze = game::generateMaze(size.width, size.height, seed);

            for (int z = 0; z < maze.height(); ++z) {
                for (int x = 0; x < maze.width(); ++x) {
                    if (x + 1 < maze.width()) {
                        CHECK(maze.hasWall(x, z, game::Direction::East) ==
                              maze.hasWall(x + 1, z, game::Direction::West));
                    }
                    if (z + 1 < maze.height()) {
                        CHECK(maze.hasWall(x, z, game::Direction::South) ==
                              maze.hasWall(x, z + 1, game::Direction::North));
                    }
                }
            }
        }
    }
}

TEST_CASE("the same seed gives the same maze, another seed another one") {
    const game::Maze first = game::generateMaze(12, 9, 2024U);
    const game::Maze again = game::generateMaze(12, 9, 2024U);
    const game::Maze other = game::generateMaze(12, 9, 2025U);

    CHECK(sameWalls(first, again));
    CHECK_FALSE(sameWalls(first, other));

    // Not just two lucky seeds: 20 seeds in a row give 20 different mazes.
    constexpr std::uint32_t SEEDS_TO_COMPARE = 20;
    std::vector<std::string> drawings;
    for (std::uint32_t seed = 0; seed < SEEDS_TO_COMPARE; ++seed) {
        const std::string drawing = drawMaze(game::generateMaze(8, 8, seed));
        for (const std::string& earlier : drawings) {
            CHECK(drawing != earlier);
        }
        drawings.push_back(drawing);
    }
}

TEST_CASE("the smallest mazes: one cell, one row, one column") {
    SUBCASE("a single cell keeps all four walls") {
        const game::Maze maze = game::generateMaze(1, 1, 5U);
        for (const game::Direction side : game::ALL_DIRECTIONS) {
            CHECK(maze.hasWall(0, 0, side));
        }
    }

    SUBCASE("one row is a straight corridor whatever the seed") {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            const game::Maze maze = game::generateMaze(6, 1, seed);
            CHECK(drawMaze(maze) == "+--+--+--+--+--+--+\n"
                                    "|                 |\n"
                                    "+--+--+--+--+--+--+\n");
        }
    }

    SUBCASE("one column is a straight corridor whatever the seed") {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            const game::Maze maze = game::generateMaze(1, 3, seed);
            CHECK(drawMaze(maze) == "+--+\n"
                                    "|  |\n"
                                    "+  +\n"
                                    "|  |\n"
                                    "+  +\n"
                                    "|  |\n"
                                    "+--+\n");
        }
    }
}

TEST_CASE("generateMaze rejects a wrong size") {
    CHECK_THROWS_AS(game::generateMaze(0, 4, 1U), std::invalid_argument);
    CHECK_THROWS_AS(game::generateMaze(4, 0, 1U), std::invalid_argument);
    CHECK_THROWS_AS(game::generateMaze(-1, -1, 1U), std::invalid_argument);
    CHECK_THROWS_AS(game::generateMaze(game::Maze::MAX_SIZE + 1, 4, 1U), std::invalid_argument);
}

TEST_CASE("golden maze: 4 x 4 cells from seed 1 has exactly these walls") {
    // This test exists to prove that macOS and Windows generate the same maze. The
    // drawing was written down from a run on Windows (MSVC, Debug and Release). The
    // generator uses nothing that may differ between compilers: std::mt19937 with
    // a standardised sequence, the integer helper randomBelow and a fixed order of the
    // neighbours. If this test fails on one system only, one of those three was broken.
    const game::Maze maze = game::generateMaze(4, 4, 1U);

    CHECK(drawMaze(maze) == "+--+--+--+--+\n"
                            "|  |        |\n"
                            "+  +  +--+  +\n"
                            "|  |     |  |\n"
                            "+  +--+  +--+\n"
                            "|     |     |\n"
                            "+--+  +--+  +\n"
                            "|           |\n"
                            "+--+--+--+--+\n");
}

TEST_CASE("the worked example of the documentation: 3 x 3 cells from seed 7") {
    // docs/modules/game/maze-generator.md walks through this maze step by step. The test
    // keeps the drawing in that document true.
    const game::Maze maze = game::generateMaze(3, 3, 7U);

    CHECK(drawMaze(maze) == "+--+--+--+\n"
                            "|  |     |\n"
                            "+  +--+  +\n"
                            "|        |\n"
                            "+--+--+  +\n"
                            "|        |\n"
                            "+--+--+--+\n");
}
