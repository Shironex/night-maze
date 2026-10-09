// Heartstone: the one big splinter of a maze, which dead end it floats in and how it moves.
#include "game/Heartstone.hpp"

#include "game/Crystals.hpp"
#include "game/Exit.hpp"
#include "game/MazeLayout.hpp"

#include <cmath>
#include <cstddef>
#include <vector>

namespace game {

namespace {

constexpr float FULL_TURN_DEGREES = 360.0F;

// True when a passage leads from one cell straight into the other.
bool joinedByPassage(const Maze& maze, MazeCell from, MazeCell to) {
    for (const Direction side : ALL_DIRECTIONS) {
        const MazeCell next{.x = from.x + columnStep(side), .z = from.z + rowStep(side)};
        if (next == to && !maze.hasWall(from.x, from.z, side)) {
            return true;
        }
    }
    return false;
}

} // namespace

std::optional<MazeCell> heartstoneCell(const Maze& maze, MazeCell start, MazeCell exit) {
    const std::vector<int> distances = passageDistances(maze, start);

    // Row after row, and "greater", not "greater or equal": of two dead ends at the same
    // distance the earlier one stays. Starting at 0 leaves out the start (distance 0) and
    // every cell that cannot be reached (UNREACHABLE is below 0).
    std::optional<MazeCell> farthest;
    int farthestDistance = 0;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            const int distance =
                distances[static_cast<std::size_t>(z) * static_cast<std::size_t>(maze.width()) +
                          static_cast<std::size_t>(x)];
            if (distance <= farthestDistance || cell == exit || !isDeadEnd(maze, x, z) ||
                joinedByPassage(maze, cell, exit)) {
                continue;
            }
            farthest = cell;
            farthestDistance = distance;
        }
    }
    return farthest;
}

glm::vec3 heartstoneRestPosition(MazeCell cell, float groundHeight) {
    return cellCenter(cell.x, cell.z) +
           glm::vec3{0.0F, groundHeight + HEARTSTONE_FLOAT_HEIGHT, 0.0F};
}

glm::vec3 heartstoneCenter(const glm::vec3& basePosition) {
    return basePosition + glm::vec3{0.0F, HEARTSTONE_HEIGHT / 2.0F, 0.0F};
}

glm::vec3 heartstoneLightPosition(const glm::vec3& basePosition) {
    return basePosition + glm::vec3{0.0F, HEARTSTONE_HEIGHT + CRYSTAL_LIGHT_CLEARANCE, 0.0F};
}

float heartstoneSpinDegrees(float seconds) {
    // fmod keeps what is left after whole turns, so the angle stays between 0 and 360.
    return std::fmod(seconds * HEARTSTONE_SPIN_DEGREES_PER_SECOND, FULL_TURN_DEGREES);
}

} // namespace game
