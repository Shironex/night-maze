// Maze: a grid of cells with a wall on every cell edge that is not a passage.
#include "game/Maze.hpp"

#include <stdexcept>

namespace game {

namespace {

// Steps on the grid for North, East, South and West, in the order of the Direction enum.
// North is -Z, so it goes one row up the grid (row - 1). East is +X: one column further.
constexpr std::array<int, DIRECTION_COUNT> COLUMN_STEPS = {0, 1, 0, -1};
constexpr std::array<int, DIRECTION_COUNT> ROW_STEPS = {-1, 0, 1, 0};

// In the enum the opposite direction is always two places further, going around:
// North (0) and South (2), East (1) and West (3).
constexpr int OPPOSITE_OFFSET = 2;

// The number behind an enum value, as an index into the arrays above.
std::size_t indexOf(Direction direction) {
    return static_cast<std::size_t>(direction);
}

// A dead end is closed on three of its four sides.
constexpr int DEAD_END_WALL_COUNT = 3;

// A size is valid when it is between 1 and Maze::MAX_SIZE.
int checkedSize(int size) {
    if (size < 1 || size > Maze::MAX_SIZE) {
        throw std::invalid_argument("Maze: width and height must be between 1 and MAX_SIZE");
    }
    return size;
}

} // namespace

Direction opposite(Direction direction) {
    // From the enum value to its number, two places on (wrapping around after West), and
    // back to an enum value.
    const int number = (static_cast<int>(direction) + OPPOSITE_OFFSET) % DIRECTION_COUNT;
    return static_cast<Direction>(number);
}

int columnStep(Direction direction) {
    return COLUMN_STEPS[indexOf(direction)];
}

int rowStep(Direction direction) {
    return ROW_STEPS[indexOf(direction)];
}

// The sizes are checked before the vector is created: the members are initialised in the
// order of their declaration, m_width and m_height first. Every cell starts with all
// four walls (true).
Maze::Maze(int width, int height)
    : m_width(checkedSize(width)),
      m_height(checkedSize(height)),
      m_walls(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height),
              {true, true, true, true}) {}

bool Maze::contains(int x, int z) const {
    return x >= 0 && x < m_width && z >= 0 && z < m_height;
}

bool Maze::hasWall(int x, int z, Direction side) const {
    return m_walls[cellIndex(x, z)][indexOf(side)];
}

void Maze::removeWall(int x, int z, Direction side) {
    m_walls[cellIndex(x, z)][indexOf(side)] = false;

    // The same wall seen from the other cell: the neighbour on that side has it on its
    // opposite side. A border cell has no neighbour there.
    const int neighbourX = x + columnStep(side);
    const int neighbourZ = z + rowStep(side);
    if (contains(neighbourX, neighbourZ)) {
        m_walls[cellIndex(neighbourX, neighbourZ)][indexOf(opposite(side))] = false;
    }
}

std::size_t Maze::cellIndex(int x, int z) const {
    // The check has to look at both coordinates: x = width in row 0 would otherwise
    // quietly land on the first cell of row 1.
    if (!contains(x, z)) {
        throw std::out_of_range("Maze: cell is outside the maze");
    }
    // Rows are stored one after another, so row z starts after z full rows.
    return static_cast<std::size_t>(z) * static_cast<std::size_t>(m_width) +
           static_cast<std::size_t>(x);
}

bool isDeadEnd(const Maze& maze, int x, int z) {
    int wallCount = 0;
    for (const Direction side : ALL_DIRECTIONS) {
        if (maze.hasWall(x, z, side)) {
            ++wallCount;
        }
    }
    return wallCount == DEAD_END_WALL_COUNT;
}

} // namespace game
