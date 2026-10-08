// Discovery: which cells of the maze the player has seen so far in this round.
#include "game/Discovery.hpp"

#include "game/MazeLayout.hpp"

#include <algorithm>
#include <cstddef>

namespace game {

namespace {

// The two values of an entry of the grid.
constexpr std::uint8_t UNKNOWN = 0;
constexpr std::uint8_t DISCOVERED = 1;

} // namespace

Discovery::Discovery(int width, int height)
    : m_width(std::max(width, 0)),
      m_height(std::max(height, 0)),
      // The casts come first: the product is then computed in std::size_t, the type of
      // a vector size.
      m_cells(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height), UNKNOWN) {}

bool Discovery::contains(int x, int z) const {
    return x >= 0 && x < m_width && z >= 0 && z < m_height;
}

std::size_t Discovery::cellIndex(int x, int z) const {
    // The rows lie one after another. The casts come first, as in the constructor.
    return static_cast<std::size_t>(z) * static_cast<std::size_t>(m_width) +
           static_cast<std::size_t>(x);
}

bool Discovery::isDiscovered(int x, int z) const {
    if (!contains(x, z)) {
        return false;
    }
    return m_cells[cellIndex(x, z)] == DISCOVERED;
}

void Discovery::discover(int x, int z) {
    if (!contains(x, z)) {
        return;
    }
    const std::size_t index = cellIndex(x, z);
    // The count grows only for a cell that was unknown, so discovering a cell twice
    // does not count it twice.
    if (m_cells[index] == UNKNOWN) {
        m_cells[index] = DISCOVERED;
        ++m_count;
    }
}

void discoverFrom(Discovery& discovery, const Maze& maze, MazeCell cell) {
    // Maze::hasWall throws for a cell outside the maze, so that case ends here.
    if (!maze.contains(cell.x, cell.z)) {
        return;
    }
    discovery.discover(cell.x, cell.z);

    for (const Direction direction : ALL_DIRECTIONS) {
        // Walk away from the cell, one cell per round of the loop. The walk goes on
        // while the side of the cell it stands in is open.
        int x = cell.x;
        int z = cell.z;
        while (!maze.hasWall(x, z, direction)) {
            x += columnStep(direction);
            z += rowStep(direction);
            // An open side on the border of the maze leads out of it. There is no cell
            // to discover there, and no cell to ask for its walls.
            if (!maze.contains(x, z)) {
                break;
            }
            discovery.discover(x, z);
        }
    }
}

void discoverAround(Discovery& discovery, const Maze& maze, const glm::vec3& position) {
    // cellAt answers for any point, also outside the maze. discoverFrom ignores a cell
    // the maze does not have.
    discoverFrom(discovery, maze, cellAt(position));
}

} // namespace game
