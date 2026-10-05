// MazeGenerator: builds a random perfect maze from a seed, the same one on every system.
// See docs/modules/game/maze-generator.md
#include "game/MazeGenerator.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace game {

namespace {

// How many different numbers std::mt19937 can return: 2 to the power of 32 (every value
// from 0 to 4294967295). It does not fit in 32 bits, hence the 64 bit type.
constexpr std::uint64_t GENERATOR_RANGE = std::uint64_t{1} << 32U;

// The generator always starts in the north-west corner. Any cell would do: the result is
// a perfect maze from every start, the start only decides which one.
constexpr int START_COLUMN = 0;
constexpr int START_ROW = 0;

// A cell of the grid: column (x) and row (z).
struct Cell {
    int x;
    int z;
};

// Position of a cell in the "visited" list: rows one after another, like in Maze.
std::size_t visitedIndex(const Maze& maze, Cell cell) {
    return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
           static_cast<std::size_t>(cell.x);
}

} // namespace

std::uint32_t randomBelow(std::mt19937& generator, std::uint32_t bound) {
    if (bound == 0U) {
        throw std::invalid_argument("randomBelow: bound must be at least 1");
    }

    // The obvious way, generator() % bound, is slightly unfair (modulo bias). The 2^32
    // possible numbers rarely split into equal groups: with bound 3 the remainders 0, 1
    // and 2 would get 1431655766, 1431655765 and 1431655765 numbers. So the numbers at
    // the top of the range that do not fill a whole group are thrown away. limit is the
    // largest multiple of bound that fits in the range: below it every remainder has
    // exactly limit / bound numbers.
    const std::uint64_t limit = GENERATOR_RANGE - GENERATOR_RANGE % bound;

    // Draw again in the rare case of a number at or above the limit (for bound 3 that is
    // one number in 2^32). The loop ends: less than half of the range is ever rejected.
    // The result of generator() is at most 32 bits long even where its type is wider.
    std::uint64_t value = generator();
    while (value >= limit) {
        value = generator();
    }
    return static_cast<std::uint32_t>(value % bound);
}

Maze generateMaze(int width, int height, std::uint32_t seed) {
    // All walls are present at first. The constructor also rejects a wrong size.
    Maze maze(width, height);

    // The Mersenne Twister. Both its algorithm and the way one number seeds it are
    // written down in the C++ standard, so every compiler produces the same sequence.
    std::mt19937 generator(seed);

    // Recursive backtracker (a randomised depth first search): walk to a random
    // neighbour that has not been visited yet and knock down the wall on the way. At
    // a dead end go back along the path until a cell with an unvisited neighbour turns
    // up. When the walk is back at the start with nowhere to go, every cell was visited.
    //
    // The textbook version is a function that calls itself for every step, so the way
    // back lives on the call stack. A long corridor then means thousands of nested calls
    // and can overflow that stack. Here the way back is an ordinary vector used as
    // a stack: push when moving on, pop when going back. It grows on the heap.
    std::vector<bool> visited(
        static_cast<std::size_t>(maze.width()) * static_cast<std::size_t>(maze.height()), false);
    std::vector<Cell> path;

    const Cell start{.x = START_COLUMN, .z = START_ROW};
    visited[visitedIndex(maze, start)] = true;
    path.push_back(start);

    while (!path.empty()) {
        const Cell current = path.back();

        // Collect the directions that lead to an unvisited cell, always in the order
        // North, East, South, West. The order is part of the result: with another order
        // the same random number would pick another neighbour.
        std::array<Direction, DIRECTION_COUNT> candidates{};
        std::uint32_t candidateCount = 0;
        for (const Direction direction : ALL_DIRECTIONS) {
            const Cell neighbour{.x = current.x + columnStep(direction),
                                 .z = current.z + rowStep(direction)};
            if (maze.contains(neighbour.x, neighbour.z) &&
                !visited[visitedIndex(maze, neighbour)]) {
                candidates[candidateCount] = direction;
                ++candidateCount;
            }
        }

        // Dead end: step back to the previous cell of the path.
        if (candidateCount == 0U) {
            path.pop_back();
            continue;
        }

        // Pick one of the candidates, each with the same chance.
        const Direction chosen = candidates[randomBelow(generator, candidateCount)];
        const Cell next{.x = current.x + columnStep(chosen), .z = current.z + rowStep(chosen)};

        // Every cell except the start is entered exactly once, and each time exactly one
        // wall is removed. That gives width * height - 1 passages and no loops: a wall to
        // an already visited cell is never removed.
        maze.removeWall(current.x, current.z, chosen);
        visited[visitedIndex(maze, next)] = true;
        path.push_back(next);
    }

    return maze;
}

} // namespace game
