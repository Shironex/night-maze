// Exit: which cell of a maze is the exit, where its gate stands and where the round is won.
// See docs/modules/game/gameplay.md
#pragma once

#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Collider.hpp"

#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.

/// The distance of a cell that cannot be reached from the start at all. A generated maze
/// has no such cell. A maze built by hand (in a test) can have one.
constexpr int UNREACHABLE = -1;

/// Half of the side of the exit zone on the floor, in metres. The zone is a square of
/// 1 x 1 m in the middle of the 2 x 2 m exit cell: the player has to walk well into the
/// cell, past the gate, and not only brush its edge.
constexpr float EXIT_ZONE_HALF_SIZE = 0.5F;

/// For every cell, the number of passages on the shortest way from start to it: 0 for
/// start itself, UNREACHABLE for a cell with no way to it. The list holds the rows one
/// after another, so the cell (x, z) is at index z * width + x.
///
/// Throws std::out_of_range when start is not a cell of the maze.
std::vector<int> passageDistances(const Maze& maze, MazeCell start);

/// The cell that is farthest from start, counted in passages walked. Of several cells
/// at the same largest distance the first one in row order wins (row 0 from west to
/// east, then row 1 and so on), so the answer never depends on the order of the search.
///
/// In a perfect maze this is always a dead end: a cell with a second passage would lead
/// on to a cell that is farther still. A maze of one cell returns start.
///
/// Throws std::out_of_range when start is not a cell of the maze.
MazeCell farthestCell(const Maze& maze, MazeCell start);

/// Where the exit of a maze is.
struct ExitPlacement {
    /// The exit cell.
    MazeCell cell;

    /// False only when the exit cell has no open side (a maze of one cell): there is
    /// nowhere to put a gate then, and gate below is left as it is created.
    bool hasGate = false;

    /// The gate: it stands across the open side of the exit cell, on the cell border,
    /// exactly where a wall segment would stand if that side were closed.
    WallSegment gate;
};

/// Chooses the exit: the cell farthest from start (farthestCell), with a gate on its
/// first open side in the order of ALL_DIRECTIONS. The exit of a generated maze is
/// a dead end, so it has exactly one open side and the gate closes it completely.
///
/// Throws std::out_of_range when start is not a cell of the maze.
ExitPlacement placeExit(const Maze& maze, MazeCell start);

/// The exit zone of an exit cell: a box in the middle of the cell, EXIT_ZONE_HALF_SIZE
/// to each side and as high as the walls. The round is won when the player enters it
/// while the gate is open.
scene::Aabb exitZone(MazeCell cell);

} // namespace game
