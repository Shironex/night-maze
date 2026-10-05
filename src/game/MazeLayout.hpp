// MazeLayout: where the cells, walls and pillars of a maze stand in the world.
// See docs/modules/game/maze-generator.md
#pragma once

#include "game/Maze.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace game {

// All sizes are in metres (one world unit is one metre).

/// Side of one square cell: the distance between two neighbouring cell centres.
constexpr float CELL_SIZE = 2.0F;

/// Length of one wall segment. A segment covers exactly one cell edge.
constexpr float WALL_LENGTH = CELL_SIZE;

/// Height of a wall, from the floor (y = 0) up.
constexpr float WALL_HEIGHT = 3.0F;

/// Side of the square footprint of a pillar. A pillar is thicker than the visible body
/// of a wall, so it covers the place where walls meet at a grid corner.
constexpr float PILLAR_SIZE = 0.3F;

/// Thickness of the visible body of a wall (the model wall_straight). The wall is centred
/// on the cell edge, so half of the thickness reaches into each of the two cells. It
/// describes the model only: collisions use WALL_COLLISION_THICKNESS.
constexpr float WALL_VISUAL_THICKNESS = 0.2F;

/// Thickness of the collision box of a wall: the same as a pillar, on purpose. The faces
/// of the wall boxes and of the pillar boxes then lie in one plane, and a player who
/// slides along a wall passes the pillars without getting caught on them. With the
/// thinner visible body (0.2 m) every pillar would stick out 5 cm in front of the wall.
constexpr float WALL_COLLISION_THICKNESS = PILLAR_SIZE;

/// Height of a pillar. It stands a little above the walls.
constexpr float PILLAR_HEIGHT = 3.15F;

/// The world axis a wall segment runs along.
enum class WallAxis {
    AlongX, ///< on the north or south edge of a cell
    AlongZ, ///< on the west or east edge of a cell
};

/// One wall segment in the world.
struct WallSegment {
    /// The middle of the wall at floor level (y = 0). This is where the origin of the
    /// wall model goes: the model is built along X, from x = -1 to x = +1, with its
    /// origin in the centre of its base.
    glm::vec3 position{0.0F};

    /// AlongX: the model is used as it is. AlongZ: the model is turned by 90 degrees
    /// around the Y axis.
    WallAxis axis = WallAxis::AlongX;
};

/// Centre of the cell in column x and row z, at floor level:
/// ((x + 0.5) * CELL_SIZE, 0, (z + 0.5) * CELL_SIZE). The maze starts in the origin of
/// the world and covers x from 0 to width * CELL_SIZE and z from 0 to height * CELL_SIZE.
glm::vec3 cellCenter(int x, int z);

/// The segment that stands on one side of the cell in column x and row z: on its north
/// or south edge it runs along X, on its west or east edge along Z. The function does
/// not ask whether the maze has a wall there: the gate of the exit uses it for a side
/// that is open.
WallSegment wallSegmentOn(int x, int z, Direction side);

/// Every wall of the maze exactly once. A wall between two cells belongs to both of
/// them, but it is one segment here. The order is fixed: row after row, cell after cell.
std::vector<WallSegment> wallSegments(const Maze& maze);

/// The grid corners at which at least one wall ends, as positions at floor level. Each
/// of them gets a pillar. The order is fixed: row of corners after row of corners.
std::vector<glm::vec3> pillarPositions(const Maze& maze);

/// The collision box of a wall segment: WALL_LENGTH long, WALL_HEIGHT high and
/// WALL_COLLISION_THICKNESS thick, standing on the floor.
scene::Aabb wallBox(const WallSegment& segment);

/// The collision box of a pillar standing at position (a result of pillarPositions):
/// PILLAR_SIZE by PILLAR_SIZE on the floor and PILLAR_HEIGHT high.
scene::Aabb pillarBox(const glm::vec3& position);

/// The collision boxes of the whole maze: the box of every wall segment, then the box of
/// every pillar. This is the obstacle list for scene::moveAndSlide.
std::vector<scene::Aabb> mazeColliders(const Maze& maze);

} // namespace game
