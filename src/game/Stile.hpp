// Stile: where the stile of the start cell stands, what of it blocks the way and where
// feet have worn the grass away in front of it.
#pragma once

#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Collider.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.
//
// The stile is where every night begins and where the shade sets the player down again:
// a step stile in the border wall of the start cell. Four flat through-stones climb to
// a notch in the wall, an oak post stands beside them and an iron hook on the post is
// empty, because the lamp that hung there is in the hand of the player. It is decoration:
// nobody climbs it and nothing on it can be used.
//
// It is two models (stile.obj and stile_post.obj, tools/blender/build_stile.py). The first
// takes the place of the plain wall model on one segment and the second is drawn with the
// matrix of the same segment. All numbers below are measured like the models: from the
// middle of the base of that segment, `along` it (the x of the model) and `out` of it into
// the start cell.

/// The side of the start cell whose wall carries the stile. North and West are the outer
/// border in every maze, so that wall is always there and no lever opens it. North, and
/// not West: the wall model is not turned on a wall along X, and its far end, where the
/// notch and the post are, then lies towards the east: a player who starts looking east
/// has the post at the left edge of the picture. A player who starts looking south has
/// the stile behind them: they came in over it.
constexpr Direction STILE_SIDE = Direction::North;

/// The one collision box of the stile, in front of the wall: from the low end of the first
/// step to the far side of the post along the wall, from the face of the collision box of
/// the wall as far out as the widest step (0.44 m from the middle of the wall, and 2 cm),
/// and as high as the top step. The stones stand 0.3 m in front of the box of the wall,
/// and without a box of their own the player would walk through them.
///
/// The far end lies exactly on the face of the pillar and of the wall or the opening on
/// the east side of the cell, so the box never reaches into the way out.
constexpr float STILE_BOX_ALONG_MIN = -0.81F;
constexpr float STILE_BOX_ALONG_MAX = WALL_LENGTH / 2.0F - WALL_COLLISION_THICKNESS / 2.0F;
constexpr float STILE_BOX_OUT_MIN = WALL_COLLISION_THICKNESS / 2.0F;
constexpr float STILE_BOX_OUT_MAX = 0.46F;
constexpr float STILE_BOX_HEIGHT = 1.92F;

/// The ground worn bare in front of the steps: an oval whose middle lies
/// STILE_WORN_OUT in front of the wall, and all the ground between that oval and the
/// wall, where the steps are.
constexpr float STILE_WORN_ALONG = -0.05F;
constexpr float STILE_WORN_OUT = 0.95F;
constexpr float STILE_WORN_HALF_LENGTH = 0.9F;
constexpr float STILE_WORN_HALF_DEPTH = 0.55F;

/// True when the segment is the wall that carries the stile: the wall on STILE_SIDE of
/// start. Only the place and the axis are compared, not the height, so it is also true
/// for that wall after it was lowered to the terrain.
bool carriesStile(const WallSegment& segment, MazeCell start);

/// The number of the wall that carries the stile in a list of wall segments
/// (game::wallSegments, MazeWorld::walls). Empty when the list has no such wall, which
/// no maze does: the border is always closed.
std::optional<std::size_t> stileWallIndex(std::span<const WallSegment> walls, MazeCell start);

/// The collision box of the stile on its wall. It stands on the base of the wall, so it
/// follows a wall that was lowered to the terrain.
scene::Aabb stileBox(const WallSegment& wall);

/// True for a point of the ground in front of the stile on this wall where no grass
/// grows. The ground on the other side of the wall, outside the maze, keeps its grass.
bool stileWornGround(const WallSegment& wall, float x, float z);

} // namespace game
