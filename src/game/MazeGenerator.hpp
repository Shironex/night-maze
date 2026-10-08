// MazeGenerator: builds a random perfect maze from a seed, the same one on every system.
#pragma once

#include "game/Maze.hpp"

#include <cstdint>
#include <random>

namespace game {

/// A random whole number from 0 to bound - 1, every value equally likely.
///
/// It replaces std::uniform_int_distribution on purpose. The C++ standard fixes the
/// numbers that std::mt19937 produces, but not how a distribution turns them into
/// a range: the standard libraries of clang and of MSVC do it differently, so the same
/// seed would give a different maze on macOS and on Windows. This function uses only
/// integer arithmetic that is the same everywhere.
///
/// Throws std::invalid_argument when bound is 0.
std::uint32_t randomBelow(std::mt19937& generator, std::uint32_t bound);

/// Generates a perfect maze: every cell can be reached from every other cell along
/// exactly one path. It has width * height - 1 passages and the outer border is closed.
///
/// The same width, height and seed always give the same maze, on every compiler.
/// Throws std::invalid_argument for a size that Maze does not accept.
Maze generateMaze(int width, int height, std::uint32_t seed);

} // namespace game
