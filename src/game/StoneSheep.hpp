// StoneSheep: the flock that went to stone with the hedge. Which cells of a maze hold
// a sheep, where in its cell each one stands and which way it looks: towards the gate.
#pragma once

#include "game/Interactables.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <span>
#include <vector>

namespace game {

struct MazeWorld;

// Plain data and math without OpenGL, like the rest of the game_logic library.
//
// A stone sheep is a compass that has to be read. Every one of them looks along the
// straight line from its cell to the exit cell, whatever walls are in between, snapped to
// the eight directions of a chalk hint (game::compassTowards). Nothing else about it can
// be used: no prompt, no mark on the map, no sound. It only stands in the way, so it has
// one collision box, like the stile.
//
// It is one model (stone_sheep.obj, tools/blender/build_stone_sheep.py) with its head
// along +X, which is east, and its origin on the ground under the middle of its length.

/// How long and how wide a sheep is, from the nose to the end of the tail and across the
/// wool, in metres. The model is exactly as long and a little narrower.
constexpr float STONE_SHEEP_LENGTH = 0.9F;
constexpr float STONE_SHEEP_WIDTH = 0.4F;

/// The collision box reaches this far above the ground under the middle of the sheep and
/// this far below it: the model is 0.57 m high and its legs go 0.1 m into the ground, for
/// a sheep that stands on a slope.
constexpr float STONE_SHEEP_BOX_HEIGHT = 0.6F;
constexpr float STONE_SHEEP_BOX_DEPTH = 0.11F;

/// A sheep that looks north, east, south or west lies along a wall of its cell that runs
/// the same way. Its middle is this far from the middle of the cell towards that wall, so
/// its box ends on the face of the collision box of the wall and leaves 1.3 m of the
/// 1.7 m between the wall boxes free.
constexpr float STONE_SHEEP_TO_WALL =
    CELL_SIZE / 2.0F - WALL_COLLISION_THICKNESS / 2.0F - STONE_SHEEP_WIDTH / 2.0F;

/// Along that wall it is moved this far towards the end of the cell that is closed, away
/// from the open one. Nothing when both ends are closed, or both open.
constexpr float STONE_SHEEP_ALONG_WALL = 0.25F;

/// A sheep that looks along a diagonal stands across a corner of two walls, with its head
/// at one of them and its tail at the other: turned any other way it would reach the
/// middle of the cell. Its middle is this far from the middle of the cell towards each of
/// the two walls.
constexpr float STONE_SHEEP_TO_CORNER = 0.55F;

/// Half of the side of the square box of such a sheep. The box holds the body. The nose,
/// the tail and the ears reach up to STONE_SHEEP_DIAGONAL_OVERHANG past it, towards the
/// two walls, where the body of the player cannot follow them: a box around all of it
/// would be 0.68 m wide and would take the middle of the cell from the player.
constexpr float STONE_SHEEP_DIAGONAL_HALF = 0.24F;
constexpr float STONE_SHEEP_DIAGONAL_OVERHANG = 0.11F;

/// No grass grows this close around the box of a sheep, in metres: under the whole model
/// and a hand's width around it.
constexpr float STONE_SHEEP_BARE_MARGIN = 0.12F;

/// Two sheep are kept more than this many passages apart, as long as the maze has cells
/// for that (chooseSpreadCells).
constexpr int STONE_SHEEP_MIN_PASSAGES_APART = 4;

/// One stone sheep of a maze.
struct StoneSheep {
    /// The cell it stands in.
    MazeCell cell;

    /// The way it looks: from its cell towards the exit cell. Never Compass::Here, because
    /// no sheep stands in the exit cell.
    Compass facing = Compass::East;

    /// The middle of the sheep, on the ground. placeStoneSheep gives y for flat ground
    /// at 0.
    glm::vec3 position{0.0F};

    /// What the player cannot walk through (stoneSheepBox).
    scene::Aabb box;

    bool operator==(const StoneSheep& other) const {
        return cell == other.cell && facing == other.facing && position == other.position &&
               box.min == other.box.min && box.max == other.box.max;
    }
};

/// How many sheep a maze of cellCount cells should get: 3 up to the 13 by 13 maze, 4 from
/// 16 by 16 on and 5 from 22 by 22 on. A maze gets fewer when it has fewer cells for them.
int stoneSheepCountFor(int cellCount);

/// True when the cell is a corner of a passage: exactly two of its sides are open, and
/// they meet at a right angle. Throws std::out_of_range when the cell is not in the maze.
bool isCornerCell(const Maze& maze, MazeCell cell);

/// The yaw that turns the model of a sheep to look in the given direction, in degrees,
/// for scene::Transform::rotationDegrees.y: 0 for East (the model looks along +X), 90
/// for North, 180 for West, 270 for South and the diagonals between them. 0 for Here.
float sheepYawDegrees(Compass facing);

/// Half of the width of the box of a sheep along X and along Z: long and narrow along the
/// way it looks for the four main directions, a square for the four diagonals.
glm::vec2 stoneSheepBoxHalfSize(Compass facing);

/// Where a sheep that looks in the given direction stands in a cell, as the place of its
/// middle measured from the middle of the cell along X and along Z. Nothing when the cell
/// has no place for it.
///
///   - North, East, South, West: along a closed side of the cell that runs the same way
///     (STONE_SHEEP_TO_WALL, STONE_SHEEP_ALONG_WALL). A sheep that looks east lies along
///     the north or the south wall.
///   - A diagonal: across a corner whose two sides are both closed, and only a corner it
///     can stand across (STONE_SHEEP_TO_CORNER). A sheep that looks north-east or
///     south-west fits the north-west and the south-east corner.
///
/// Where two places are possible, otherPlace chooses the second one. The place never
/// reaches into an open side of the cell: see the tests for what stays free.
/// Throws std::out_of_range when the cell is not in the maze.
std::optional<glm::vec2> stoneSheepPlace(const Maze& maze, MazeCell cell, Compass facing,
                                         bool otherPlace);

/// The collision box of a sheep whose middle is at position and that looks in the given
/// direction (stoneSheepBoxHalfSize, STONE_SHEEP_BOX_HEIGHT, STONE_SHEEP_BOX_DEPTH).
scene::Aabb stoneSheepBox(const glm::vec3& position, Compass facing);

/// The model matrix of a sheep: turned to where it looks and moved to its place.
glm::mat4 stoneSheepMatrix(const StoneSheep& sheep);

/// The dead ends that are farthest from start, counted in passages walked, the start and
/// the exit cell left out: one cell, or several at the same distance. The exit itself is
/// the farthest cell of all, so this is the far end of the maze that is not the gate.
/// No sheep stands there: that dead end is kept free for something of its own.
/// Throws std::out_of_range when start is not a cell of the maze.
std::vector<MazeCell> farthestDeadEnds(const Maze& maze, MazeCell start, MazeCell exit);

/// Takes up to count cells from candidates, in their order, so that they lie spread out:
/// a cell is taken when it is more than STONE_SHEEP_MIN_PASSAGES_APART passages from
/// every cell taken before it. When that leaves fewer than count, the cells that were
/// passed over fill the rest, again in their order. A count below 1 gives none.
std::vector<MazeCell> chooseSpreadCells(const Maze& maze, std::span<const MazeCell> candidates,
                                        int count);

/// Places the stone sheep of a world, on flat ground at 0. It is the last thing a maze
/// gets, and it only takes cells that everything else has left alone, so no wall, crystal,
/// lever, note, flask or puddle of a seed is anywhere else because of the sheep. The same
/// world always gives the same sheep, on every compiler: the choice uses std::mt19937 and
/// game::randomBelow only, with a generator of its own.
///
/// A cell can hold a sheep when all of this is true:
///   - it is a dead end or a corner (isCornerCell),
///   - it is not the start cell, the exit cell or the cell in front of the gate,
///   - it holds no crystal, no lever, no note and no slab ring (so no wall of it ever
///     opens), no puddle at the usual share (game::DEFAULT_PUDDLE_SHARE), and none of the
///     flasks of the three levels,
///   - it is not one of the dead ends the flasks have reserved, and not one of
///     farthestDeadEnds,
///   - a sheep that looks from it towards the exit has a place in it (stoneSheepPlace).
///
/// Those cells are shuffled by the seed, and stoneSheepCountFor of them are taken, spread
/// out (chooseSpreadCells). The world needs its maze, seed, exit, gate, crystals and
/// interactables: buildMazeWorld calls this after all of them.
std::vector<StoneSheep> placeStoneSheep(const MazeWorld& world);

/// True for a point of the ground under or right beside a sheep of the world, where no
/// grass grows (STONE_SHEEP_BARE_MARGIN).
bool stoneSheepBareGround(const MazeWorld& world, float x, float z);

} // namespace game
