// MazeWorld: one generated maze with everything the game needs to draw it and walk in it.
// See docs/modules/game/maze-rendering.md
#include "game/MazeWorld.hpp"

#include "game/Lighting.hpp"
#include "game/MazeGenerator.hpp"
#include "scene/Transform.hpp"

namespace game {

namespace {

// The start cell: the north-west corner of the maze.
constexpr int START_COLUMN = 0;
constexpr int START_ROW = 0;

// Yaw grows by a quarter turn from one compass direction to the next one clockwise.
constexpr float QUARTER_TURN_DEGREES = 90.0F;

// The wall model lies along the X axis. A quarter turn around Y lays it along Z.
constexpr glm::vec3 WALL_ALONG_Z_ROTATION{0.0F, QUARTER_TURN_DEGREES, 0.0F};

// Model matrix of an object that only stands somewhere: no rotation, no scale.
glm::mat4 placedAt(const glm::vec3& position) {
    scene::Transform transform;
    transform.position = position;
    return transform.matrix();
}

// Model matrix of one wall segment.
glm::mat4 wallMatrix(const WallSegment& segment) {
    scene::Transform transform;
    transform.position = segment.position;
    if (segment.axis == WallAxis::AlongZ) {
        transform.rotationDegrees = WALL_ALONG_Z_ROTATION;
    }
    return transform.matrix();
}

// Yaw towards the first side of the start cell that has no wall, in the fixed order of
// ALL_DIRECTIONS. In a generated maze that is East or South: North and West are the
// border. A maze of one cell has no open side, the player then looks North.
float startYaw(const Maze& maze) {
    for (const Direction direction : ALL_DIRECTIONS) {
        if (!maze.hasWall(START_COLUMN, START_ROW, direction)) {
            return yawTowards(direction);
        }
    }
    return yawTowards(Direction::North);
}

} // namespace

float yawTowards(Direction direction) {
    // The enum lists the directions clockwise starting with North (0, 1, 2, 3), the same
    // way yaw grows, so the number of the direction times 90 is the angle.
    return static_cast<float>(static_cast<int>(direction)) * QUARTER_TURN_DEGREES;
}

MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed) {
    MazeWorld world(generateMaze(width, height, seed));
    world.seed = seed;
    const Maze& maze = world.maze;

    world.walls = wallSegments(maze);
    world.pillars = pillarPositions(maze);
    world.colliders = mazeColliders(maze);
    // The start cell gets no light of its own: the player stands there with a flashlight.
    world.pointLightPositions = deadEndLightPositions(maze, START_COLUMN, START_ROW);

    // One floor tile per cell. The tile model is 2 x 2 m with its origin in the middle,
    // exactly one cell.
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            world.floorMatrices.push_back(placedAt(cellCenter(x, z)));
        }
    }
    for (const WallSegment& segment : world.walls) {
        world.wallMatrices.push_back(wallMatrix(segment));
    }
    for (const glm::vec3& position : world.pillars) {
        world.pillarMatrices.push_back(placedAt(position));
    }

    world.startPosition = cellCenter(START_COLUMN, START_ROW);
    world.startYawDegrees = startYaw(maze);
    world.exitPosition = cellCenter(maze.width() - 1, maze.height() - 1);
    return world;
}

} // namespace game
