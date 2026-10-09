// WallVariants: which wall segments of a maze look cracked, mossy or damaged, and which
// carry a crown of stone twigs or have lost a coping stone.
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
// Most wall segments are drawn with the same model and the plain stone. Some get one of
// three painted looks: the same model with another pair of textures. Some get one of two
// shaped looks: the plain stone on another model, whose top edge differs. Which wall gets
// which look follows from the seed of the maze, like the places of the crystals.

/// The look of one wall segment. The numbers are indexes into lists of WALL_VARIANT_COUNT
/// entries (the models and textures of MazeRenderer, the counts of countWallVariants).
enum class WallVariant {
    Plain = 0, ///< the stone as it was built
    Cracked,   ///< deeper joints and a few long cracks
    Mossy,     ///< darker and damp, overgrown with moss
    Damaged,   ///< one or two stones are missing and leave a shallow niche
    Crowned,   ///< twigs of stone rise from the coping (a model of its own)
    Broken,    ///< a coping stone is gone and the next one sits askew (a model of its own)
};

/// The number of looks, the plain one included.
constexpr std::size_t WALL_VARIANT_COUNT = 6;

/// The number of painted looks (Cracked, Mossy, Damaged): the ones that are textures.
/// They have the numbers 1 to PAINTED_WALL_VARIANT_COUNT in the enum.
constexpr std::size_t PAINTED_WALL_VARIANT_COUNT = 3;

/// How many walls out of 100 get a painted look. The outer border of the maze is
/// weathered more than the walls inside it.
constexpr std::uint32_t BORDER_WALL_VARIANT_PERCENT = 35;
constexpr std::uint32_t INNER_WALL_VARIANT_PERCENT = 20;

/// How many out of 100 of the walls that got no painted look get a shaped one. Counted
/// over all walls of a maze, the ones that must stay plain included, that comes to about
/// one wall in six with a crown and one in twelve with a broken coping: 15 and 7 out of
/// 100 in a maze of 10 by 10 cells, 18 and 9 in one of 22 by 22 (measured over 100
/// seeds each).
constexpr std::uint32_t CROWNED_WALL_PERCENT = 24;
constexpr std::uint32_t BROKEN_WALL_PERCENT = 12;

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
///   - the wall that carries the stile (game::carriesStile) stays plain: it has a model
///     of its own, and a look would be drawn in place of it,
///   - a wall a lever or a note hangs on stays plain: the niche of a damaged wall is
///     only painted, and a lever in front of it would seem to float,
///   - of the other walls, BORDER_WALL_VARIANT_PERCENT out of 100 on the outer border
///     and INNER_WALL_VARIANT_PERCENT out of 100 inside get a painted look, and each
///     of the three painted looks is equally likely,
///   - of the walls that are still plain then, CROWNED_WALL_PERCENT out of 100 become
///     Crowned and BROKEN_WALL_PERCENT out of 100 become Broken,
///   - but a wall that a lever opens never gets a shaped look: it sinks into the ground
///     by a little more than its height, and the twigs would be left standing in the
///     opening.
///
/// The gate needs no rule: it stands in an opening, not on a wall of the list.
///
/// The same maze, seed, start and interactables always give the same result, on every
/// compiler: the choice uses std::mt19937 and game::randomBelow only, with generators
/// of its own. The numbers are drawn for every wall, also for a wall that stays plain,
/// so moving a note never changes the look of another wall. The shaped looks have
/// a generator to themselves, so they never change which wall is painted.
std::vector<WallVariant> chooseWallVariants(const Maze& maze, std::uint32_t seed, MazeCell start,
                                            const Interactables& interactables);

/// How many walls have each look: entry i counts the walls of the look with number i.
std::array<int, WALL_VARIANT_COUNT> countWallVariants(std::span<const WallVariant> variants);

} // namespace game
