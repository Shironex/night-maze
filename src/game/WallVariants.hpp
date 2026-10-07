// WallVariants: which wall segments of a maze look cracked, mossy or damaged.
#pragma once

#include "game/Interactables.hpp"
#include "game/Maze.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.
//
// Every wall segment is drawn with the same model. What differs is the pair of textures
// on it: most walls get the plain stone, some get one of three worn looks. Which wall
// gets which look follows from the seed of the maze, like the places of the crystals.

/// The look of one wall segment. The numbers are indexes into lists of WALL_VARIANT_COUNT
/// entries (the textures of MazeRenderer, the counts of countWallVariants).
enum class WallVariant {
    Plain = 0, ///< the stone as it was built
    Cracked,   ///< deeper joints and a few long cracks
    Mossy,     ///< darker and damp, overgrown with moss
    Damaged,   ///< one or two stones are missing and leave a shallow niche
};

/// The number of looks, the plain one included.
constexpr std::size_t WALL_VARIANT_COUNT = 4;

/// How many walls out of 100 get a worn look. The outer border of the maze is weathered
/// more than the walls inside it.
constexpr std::uint32_t BORDER_WALL_VARIANT_PERCENT = 35;
constexpr std::uint32_t INNER_WALL_VARIANT_PERCENT = 20;

/// Walls this close to the start cell stay plain, so the first thing the player sees is
/// the wall as it was built. The distance is counted in cells, along columns or rows,
/// whichever is larger (a square of cells around the start), from the nearer of the two
/// cells the wall stands between.
constexpr int WALL_VARIANT_START_CLEARANCE = 2;

/// Chooses the look of every wall segment of a maze. The result has one entry per
/// segment, in the order of game::wallSegments.
///
/// The rules:
///   - a wall within WALL_VARIANT_START_CLEARANCE cells of start stays plain,
///   - a wall a lever or a note hangs on stays plain: the niche of a damaged wall is
///     only painted, and a lever in front of it would seem to float,
///   - of the other walls, BORDER_WALL_VARIANT_PERCENT out of 100 on the outer border
///     and INNER_WALL_VARIANT_PERCENT out of 100 inside get a worn look, and each of
///     the three worn looks is equally likely.
///
/// The gate needs no rule: it stands in an opening, not on a wall of the list.
///
/// The same maze, seed, start and interactables always give the same result, on every
/// compiler: the choice uses std::mt19937 and game::randomBelow only, with a generator
/// of its own. Two numbers are drawn for every wall, also for a wall that stays plain,
/// so moving a note never changes the look of another wall.
std::vector<WallVariant> chooseWallVariants(const Maze& maze, std::uint32_t seed, MazeCell start,
                                            const Interactables& interactables);

/// How many walls have each look: entry i counts the walls of the look with number i.
std::array<int, WALL_VARIANT_COUNT> countWallVariants(std::span<const WallVariant> variants);

} // namespace game
