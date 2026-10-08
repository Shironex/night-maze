// Discovery: which cells of the maze the player has seen so far in this round.
#pragma once

#include "game/Maze.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace game {

// Plain data and free functions without OpenGL, like the rest of the game_logic library.

/// One flag per cell of a maze: discovered or not. The minimap shows only the cells
/// whose flag is set.
///
/// A new grid has no cell discovered. A flag is only ever set, never cleared: a new
/// round makes a new grid (game::startRound).
class Discovery {
public:
    /// A grid without cells: nothing is discovered and nothing can be. For a round that
    /// has not been started yet.
    Discovery() = default;

    /// A grid of width columns by height rows with no cell discovered. The sizes are
    /// the ones of the maze. A size below 0 is taken as 0.
    Discovery(int width, int height);

    /// Number of columns and of rows.
    int width() const { return m_width; }
    int height() const { return m_height; }

    /// True when (x, z) is a cell of this grid.
    bool contains(int x, int z) const;

    /// True when the cell in column x and row z is discovered. False for a cell that is
    /// not in the grid.
    bool isDiscovered(int x, int z) const;

    /// Marks the cell as discovered. A cell that is not in the grid is ignored.
    void discover(int x, int z);

    /// How many cells are discovered.
    int count() const { return m_count; }

private:
    /// Position of a cell in m_cells. The cell must be in the grid.
    std::size_t cellIndex(int x, int z) const;

    int m_width = 0;
    int m_height = 0;
    int m_count = 0;

    // One entry per cell, row after row: the cell (x, z) is at index z * width + x.
    // 1 means discovered, 0 not yet. A byte per cell and not std::vector<bool>: that
    // one is a special case of the standard library that packs the flags into bits and
    // does not hand out plain references to them.
    std::vector<std::uint8_t> m_cells;
};

/// The rule of the minimap: line of sight along the corridors.
///
/// Discovers the cell, and from it every cell in a straight line to the north, the
/// east, the south and the west, cell by cell, until a wall stands in the way
/// (Maze::hasWall) or the maze ends. It does not look around corners: a side passage is
/// seen as an opening in the wall of the corridor, and the cells in it stay unknown
/// until the player stands in line with them.
///
/// The walls are asked from the maze in every call and nothing about them is kept, so
/// a wall that is removed in the middle of a round opens the view from the next call on.
/// The gate of the exit is not a wall of the maze: the view passes it, open or closed.
///
/// A cell that is not in the maze discovers nothing. The grid must have the size of the
/// maze: cells it does not have are skipped.
void discoverFrom(Discovery& discovery, const Maze& maze, MazeCell cell);

/// The same for a place in the world: discoverFrom of the cell the position lies in
/// (game::cellAt). Only x and z count, the height does not: a player who flies above
/// the maze in noclip mode discovers the cells below. A position outside the maze
/// discovers nothing.
void discoverAround(Discovery& discovery, const Maze& maze, const glm::vec3& position);

} // namespace game
