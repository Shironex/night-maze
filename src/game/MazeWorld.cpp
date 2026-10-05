// MazeWorld: one generated maze with everything the game needs to draw it and walk in it.
// See docs/modules/game/maze-rendering.md
#include "game/MazeWorld.hpp"

#include "game/Exit.hpp"
#include "game/MazeGenerator.hpp"
#include "scene/Transform.hpp"

namespace game {

namespace {

// The start cell: the north-west corner of the maze.
constexpr MazeCell START_CELL{.x = 0, .z = 0};

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

// The lowest ground under a box, looked at from above: its footprint, made wider by
// FOOTPRINT_MARGIN on every side.
float lowestGroundUnder(const Terrain& terrain, const scene::Aabb& box) {
    return terrain.lowestHeightUnder(box.min.x - FOOTPRINT_MARGIN, box.min.z - FOOTPRINT_MARGIN,
                                     box.max.x + FOOTPRINT_MARGIN, box.max.z + FOOTPRINT_MARGIN);
}

// Lowers a wall segment (or the gate) to the lowest ground under it. The footprint of
// its box does not depend on its height, so the box can be asked before the height is
// known.
void lowerToGround(const Terrain& terrain, WallSegment& segment) {
    segment.position.y = lowestGroundUnder(terrain, wallBox(segment));
}

// Yaw towards the first side of the start cell that has no wall, in the fixed order of
// ALL_DIRECTIONS. In a generated maze that is East or South: North and West are the
// border. A maze of one cell has no open side, the player then looks North.
float startYaw(const Maze& maze) {
    for (const Direction direction : ALL_DIRECTIONS) {
        if (!maze.hasWall(START_CELL.x, START_CELL.z, direction)) {
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

glm::mat4 wallModelMatrix(const WallSegment& segment) {
    scene::Transform transform;
    transform.position = segment.position;
    if (segment.axis == WallAxis::AlongZ) {
        transform.rotationDegrees = WALL_ALONG_Z_ROTATION;
    }
    return transform.matrix();
}

float groundHeightAt(const MazeWorld& world, MazeCell cell) {
    const glm::vec3 center = cellCenter(cell.x, cell.z);
    return world.terrain.heightAt(center.x, center.z);
}

void placeOnTerrain(MazeWorld& world, const Heightmap& heightmap, float heightScale) {
    world.terrain = Terrain(world.maze.width(), world.maze.height(), heightmap, heightScale);
    const Terrain& terrain = world.terrain;

    // The walls and the pillars: only their height changes. The matrices and the boxes
    // are made again from the lowered positions, so the models and their collision
    // boxes stay together.
    world.wallMatrices.clear();
    for (WallSegment& segment : world.walls) {
        lowerToGround(terrain, segment);
        world.wallMatrices.push_back(wallModelMatrix(segment));
    }
    world.pillarMatrices.clear();
    for (glm::vec3& position : world.pillars) {
        position.y = lowestGroundUnder(terrain, pillarBox(position));
        world.pillarMatrices.push_back(placedAt(position));
    }
    world.colliders = colliderBoxes(world.walls, world.pillars);

    // The start and the exit stand on the ground at the centre of their cells.
    world.startPosition = cellCenter(START_CELL.x, START_CELL.z);
    world.startPosition.y = groundHeightAt(world, START_CELL);
    world.exitPosition = cellCenter(world.exitCell.x, world.exitCell.z);
    world.exitPosition.y = groundHeightAt(world, world.exitCell);
    world.exitZone = exitZone(world.exitCell, world.exitPosition.y);

    if (world.hasGate) {
        lowerToGround(terrain, world.gate);
        world.gateBox = wallBox(world.gate);
    }
}

MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed) {
    // A new heightmap is flat, so the height scale does not matter.
    return buildMazeWorld(width, height, seed, Heightmap{}, DEFAULT_HEIGHT_SCALE);
}

MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed, const Heightmap& heightmap,
                         float heightScale) {
    MazeWorld world(generateMaze(width, height, seed));
    world.seed = seed;
    const Maze& maze = world.maze;

    // The plan of the maze: where things stand, seen from above.
    world.walls = wallSegments(maze);
    world.pillars = pillarPositions(maze);
    world.startYawDegrees = startYaw(maze);

    // The exit and its gate.
    const ExitPlacement exit = placeExit(maze, START_CELL);
    world.exitCell = exit.cell;
    world.hasGate = exit.hasGate;
    if (exit.hasGate) {
        world.gate = exit.gate;
    }

    // The crystals: never in the start cell (the player would collect one without
    // moving) and never in the exit cell (it is behind the gate).
    world.crystals = placeCrystals(maze, seed, START_CELL, exit.cell);

    // The heights: the terrain, and everything above standing on it.
    placeOnTerrain(world, heightmap, heightScale);
    return world;
}

} // namespace game
