// Exit: which cell of a maze is the exit, where its gate stands and where the round is won.
// See docs/modules/game/gameplay.md
#include "game/Exit.hpp"

#include <cstddef>
#include <stdexcept>

namespace game {

namespace {

// Position of a cell in a list that holds the rows one after another, like in Maze.
std::size_t cellIndex(const Maze& maze, MazeCell cell) {
    return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
           static_cast<std::size_t>(cell.x);
}

} // namespace

std::vector<int> passageDistances(const Maze& maze, MazeCell start) {
    if (!maze.contains(start.x, start.z)) {
        throw std::out_of_range("passageDistances: the start cell is outside the maze");
    }

    // Breadth first search: the cells are visited in the order of their distance. First
    // the start (distance 0), then every cell one passage away, then every cell two
    // passages away and so on. A cell is given its distance the first time it is
    // reached, and that first time is along a shortest way.
    std::vector<int> distances(static_cast<std::size_t>(maze.width()) *
                                   static_cast<std::size_t>(maze.height()),
                               UNREACHABLE);

    // The cells that were reached and whose neighbours have not been looked at yet. It
    // is a queue: cells are added at the back and taken from the front. The vector only
    // grows, and next is the index of the front, so nothing is ever moved or erased.
    std::vector<MazeCell> reached;
    reached.push_back(start);
    distances[cellIndex(maze, start)] = 0;

    for (std::size_t next = 0; next < reached.size(); ++next) {
        const MazeCell current = reached[next];
        const int currentDistance = distances[cellIndex(maze, current)];

        for (const Direction direction : ALL_DIRECTIONS) {
            // A wall on this side: no passage.
            if (maze.hasWall(current.x, current.z, direction)) {
                continue;
            }
            const MazeCell neighbour{.x = current.x + columnStep(direction),
                                     .z = current.z + rowStep(direction)};
            // An opening in the outer border leads out of the maze, not to a cell.
            if (!maze.contains(neighbour.x, neighbour.z)) {
                continue;
            }
            // Already reached, along a way that was not longer.
            if (distances[cellIndex(maze, neighbour)] != UNREACHABLE) {
                continue;
            }
            distances[cellIndex(maze, neighbour)] = currentDistance + 1;
            reached.push_back(neighbour);
        }
    }
    return distances;
}

MazeCell farthestCell(const Maze& maze, MazeCell start) {
    const std::vector<int> distances = passageDistances(maze, start);

    // Row after row. "Greater", not "greater or equal": of two cells at the same
    // distance the earlier one stays. The start has distance 0 and an unreachable cell
    // -1, so the start is the answer when nothing else can be reached.
    MazeCell farthest = start;
    int farthestDistance = 0;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            const int distance = distances[cellIndex(maze, cell)];
            if (distance > farthestDistance) {
                farthest = cell;
                farthestDistance = distance;
            }
        }
    }
    return farthest;
}

ExitPlacement placeExit(const Maze& maze, MazeCell start) {
    ExitPlacement placement;
    placement.cell = farthestCell(maze, start);

    for (const Direction side : ALL_DIRECTIONS) {
        if (!maze.hasWall(placement.cell.x, placement.cell.z, side)) {
            placement.hasGate = true;
            placement.gate = wallSegmentOn(placement.cell.x, placement.cell.z, side);
            break;
        }
    }
    return placement;
}

scene::Aabb exitZone(MazeCell cell, float groundHeight) {
    // The box stands on the ground, its centre is half of its height above that.
    const glm::vec3 center =
        cellCenter(cell.x, cell.z) + glm::vec3{0.0F, groundHeight + WALL_HEIGHT / 2.0F, 0.0F};
    return scene::Aabb::fromCenter(center,
                                   {EXIT_ZONE_HALF_SIZE, WALL_HEIGHT / 2.0F, EXIT_ZONE_HALF_SIZE});
}

} // namespace game
