// MazeWorld: one generated maze with everything the game needs to draw it and walk in it.
#include "game/MazeWorld.hpp"

#include "game/Exit.hpp"
#include "game/Flasks.hpp"
#include "game/Heartstone.hpp"
#include "game/MazeGenerator.hpp"
#include "game/Stile.hpp"
#include "scene/Transform.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace game {

namespace {

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

// The number of the wall a lever opens in the list of wall segments. openedWallSegment
// gives the segment at y = 0, exactly as wallSegments lists it, so it is found by its
// place and its axis. It has to be asked before the walls are lowered to the terrain:
// that changes their y.
std::size_t wallIndexOf(const std::vector<WallSegment>& walls, const Lever& lever) {
    const WallSegment opened = openedWallSegment(lever);
    for (std::size_t i = 0; i < walls.size(); ++i) {
        if (walls[i].position == opened.position && walls[i].axis == opened.axis) {
            return i;
        }
    }
    // Cannot happen: a lever only opens a wall the maze has, and every wall of the maze
    // is in the list. An error is better than a wrong wall going down.
    throw std::logic_error("buildMazeWorld: the wall of a lever is not in the wall list");
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
    // The stone sheep stand on the ground under their middle. Their boxes come after the
    // pillars, so box number i still belongs to wall number i.
    world.sheepMatrices.clear();
    for (StoneSheep& sheep : world.sheep) {
        sheep.position.y = terrain.heightAt(sheep.position.x, sheep.position.z);
        sheep.box = stoneSheepBox(sheep.position, sheep.facing);
        world.sheepMatrices.push_back(stoneSheepMatrix(sheep));
        world.colliders.push_back(sheep.box);
    }
    // The box of the stile comes last.
    if (world.stileWall && *world.stileWall < world.walls.size()) {
        world.colliders.push_back(stileBox(world.walls[*world.stileWall]));
    }

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

    // The levers and the notes hang on the walls at a height above the ground, so they
    // follow the ground too.
    placeInteractablesOnTerrain(world.interactables, terrain);
}

MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed,
                         const InteractableSettings& interactables, int crystalCount) {
    // A new heightmap is flat, so the height scale does not matter.
    return buildMazeWorld(width, height, seed, Heightmap{}, DEFAULT_HEIGHT_SCALE, interactables,
                          crystalCount);
}

MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed, const Heightmap& heightmap,
                         float heightScale, const InteractableSettings& interactables,
                         int crystalCount) {
    MazeWorld world(generateMaze(width, height, seed));
    world.seed = seed;
    const Maze& maze = world.maze;

    // The plan of the maze: where things stand, seen from above.
    world.walls = wallSegments(maze);
    world.pillars = pillarPositions(maze);
    world.stileWall = stileWallIndex(world.walls, START_CELL);
    world.startYawDegrees = startYaw(maze);

    // The exit and its gate.
    const ExitPlacement exit = placeExit(maze, START_CELL);
    world.exitCell = exit.cell;
    world.hasGate = exit.hasGate;
    if (exit.hasGate) {
        world.gate = exit.gate;
    }

    // The crystals: never in the start cell (the player would collect one without
    // moving) and never in the exit cell (it is behind the gate). They also keep out of
    // the dead ends the flasks will use when a round starts.
    const std::vector<MazeCell> flaskOrder = flaskDeadEnds(maze, seed, START_CELL, exit.cell);
    const std::size_t reservedCount =
        std::min(flaskOrder.size(), static_cast<std::size_t>(FLASK_RESERVED_DEAD_ENDS));
    const std::vector<MazeCell> flaskCells(
        flaskOrder.begin(), flaskOrder.begin() + static_cast<std::ptrdiff_t>(reservedCount));
    world.seedCrystals = placeCrystals(maze, seed, START_CELL, exit.cell, crystalCount, flaskCells);

    // The heartstone takes its dead end before anything else, and the flasks take the
    // next ones of their order (Round.cpp skips its cell). The crystals are placed as
    // above, with the same reserved cells, so their shuffle is the same, and then kept
    // out of two cells: the cell of the heartstone, and, when that was one of the
    // reserved dead ends, the dead end the flasks get in its place. So at most two
    // crystals of a seed move, and nothing else does.
    world.heartstone = heartstoneCell(maze, START_CELL, exit.cell);
    std::vector<MazeCell> keptFree;
    if (world.heartstone) {
        keptFree.push_back(*world.heartstone);
        const bool wasReserved =
            std::ranges::find(flaskCells, *world.heartstone) != flaskCells.end();
        if (wasReserved && flaskOrder.size() > reservedCount) {
            keptFree.push_back(flaskOrder[reservedCount]);
        }
    }
    world.crystals =
        placeCrystals(maze, seed, START_CELL, exit.cell, crystalCount, flaskCells, keptFree);

    // The levers and the notes come after the crystals: a lever avoids the cells that
    // have a crystal. They are given the crystals of the seed (MazeWorld::seedCrystals),
    // so the heartstone moves no lever. For every lever the wall it opens is looked up in
    // the wall list once, here, so nothing has to search for it while the game runs.
    world.interactables =
        placeInteractables(maze, seed, START_CELL, exit.cell, world.seedCrystals, interactables);
    world.leverWalls.reserve(world.interactables.levers.size());
    for (const Lever& lever : world.interactables.levers) {
        world.leverWalls.push_back(wallIndexOf(world.walls, lever));
    }
    // The looks of the walls come last: a wall with a lever or a note on it stays plain.
    world.wallVariants = chooseWallVariants(maze, seed, START_CELL, world.interactables);
    // The stone sheep come after everything: they only take cells that are still empty.
    world.sheep = placeStoneSheep(world);

    // The heights: the terrain, and everything above standing on it.
    placeOnTerrain(world, heightmap, heightScale);
    return world;
}

} // namespace game
