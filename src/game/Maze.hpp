// Maze: a grid of cells with a wall on every cell edge that is not a passage.
// See docs/modules/game/maze-generator.md
#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace game {

/// The four sides of a cell, named like a compass seen from above. They follow the same
/// convention as the yaw of scene::Camera: North is -Z, East is +X, South is +Z and West
/// is -X.
enum class Direction { North, East, South, West };

constexpr int DIRECTION_COUNT = 4;

/// All directions in one fixed order. Code that goes through the sides of a cell uses
/// this order, so that its result never depends on anything else.
constexpr std::array<Direction, DIRECTION_COUNT> ALL_DIRECTIONS = {
    Direction::North, Direction::East, Direction::South, Direction::West};

/// The direction that points the other way: North for South, East for West.
Direction opposite(Direction direction);

/// Change of the column (x) after one step in the direction: 1 for East, -1 for West,
/// 0 for North and South.
int columnStep(Direction direction);

/// Change of the row (z) after one step in the direction: 1 for South, -1 for North,
/// 0 for East and West.
int rowStep(Direction direction);

/// One cell of a maze by its place on the grid. (0, 0) is the north-west corner.
struct MazeCell {
    int x = 0; ///< column: grows towards +X (east)
    int z = 0; ///< row: grows towards +Z (south)

    /// Two cells are the same when both numbers match. "= default" lets the compiler
    /// write that comparison.
    bool operator==(const MazeCell& other) const = default;
};

/// A rectangular maze: width columns (along X) by height rows (along Z) of square cells.
///
/// A wall is not a cell. It stands on the edge between two cells, or on the outer edge
/// of a border cell. Two neighbouring cells see the wall between them as one and the
/// same wall, and the class keeps both views in step.
///
/// A new maze has every wall. Passages are made by removing walls.
class Maze {
public:
    /// Largest width and height. Keeps the number of cells far away from the limit of
    /// int and the memory small: 256 x 256 cells is already 512 by 512 metres.
    static constexpr int MAX_SIZE = 256;

    /// Creates a maze with all walls present.
    /// Throws std::invalid_argument when width or height is below 1 or above MAX_SIZE.
    Maze(int width, int height);

    /// Number of columns: cells along X.
    int width() const { return m_width; }

    /// Number of rows: cells along Z.
    int height() const { return m_height; }

    /// True when (x, z) is a cell of this maze: column 0 to width - 1, row 0 to
    /// height - 1.
    bool contains(int x, int z) const;

    /// True when the cell in column x and row z has a wall on the given side.
    /// Throws std::out_of_range when the cell is not in the maze.
    bool hasWall(int x, int z, Direction side) const;

    /// Removes the wall on one side of a cell. The neighbour on that side loses the same
    /// wall, seen from its own side. A wall on the outer border can be removed too (an
    /// exit): then there is no neighbour to update.
    /// Throws std::out_of_range when the cell is not in the maze.
    void removeWall(int x, int z, Direction side);

private:
    /// Position of a cell in m_walls. Throws std::out_of_range for a cell outside.
    std::size_t cellIndex(int x, int z) const;

    int m_width;
    int m_height;

    // One entry per cell, row after row. Each entry answers "is there a wall?" for the
    // four sides, in the order of the Direction enum.
    std::vector<std::array<bool, DIRECTION_COUNT>> m_walls;
};

/// True when the cell in column x and row z is a dead end: it has a wall on exactly
/// three of its four sides, so there is one way in and no way on.
/// Throws std::out_of_range when the cell is not in the maze.
bool isDeadEnd(const Maze& maze, int x, int z);

} // namespace game
