// Puddles: which cells of a maze get a puddle, how big it is and at what height its
// water stands.
// See docs/modules/renderer/env-mapping.md
#pragma once

#include "game/Crystals.hpp"
#include "game/Maze.hpp"
#include "gfx/Vertex.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace game {

class Terrain;
struct MazeWorld;

// Plain data and math without OpenGL, like the rest of the game_logic library. A puddle
// is decoration: it is not an obstacle, the player walks through it and no rule of the
// round knows about it. It is drawn by game::PuddleRenderer with the reflect program,
// as a mirror of the sky.

// All sizes are in metres.

/// A puddle is a flat disc. Its radius is between these two numbers, chosen from the
/// seed, so the puddles are not all the same.
constexpr float PUDDLE_MIN_RADIUS = 0.25F;
constexpr float PUDDLE_MAX_RADIUS = 0.45F;

/// The middle of a puddle lies at most this far from the centre of its cell, along
/// X and along Z, so the puddles do not stand in a straight row down a corridor. With
/// the largest radius a puddle reaches 0.75 m from the centre of its cell. The collision
/// box of a wall starts 0.85 m from it (half a cell of 2 m minus half of the 0.3 m the
/// box is thick) and the foot of a wall or a pillar 0.8 m (game::FOOTPRINT_MARGIN), so
/// no puddle ever reaches a wall, a pillar or the gate.
constexpr float PUDDLE_MAX_OFFSET = 0.3F;

/// How deep the water of a puddle stands over the lowest ground under it. The water
/// level is that lowest ground plus this number, see puddleWaterLevel.
constexpr float PUDDLE_DEPTH = 0.02F;

/// The disc is drawn as a fan of this many triangles around its middle, so its rim is
/// a polygon with this many corners. 16 look round from the height of the eyes.
constexpr int PUDDLE_CORNERS = 16;

/// One puddle of a maze as the seed chose it: only the plan, no height yet.
struct PuddleSpawn {
    /// The cell the puddle lies in.
    MazeCell cell;
    /// Where its middle lies, measured from the centre of the cell along X and along Z:
    /// both between -PUDDLE_MAX_OFFSET and PUDDLE_MAX_OFFSET.
    glm::vec2 offset{0.0F};
    /// The radius of the disc, between PUDDLE_MIN_RADIUS and PUDDLE_MAX_RADIUS.
    float radius = PUDDLE_MIN_RADIUS;
};

/// One puddle in the world, ready to draw.
struct Puddle {
    /// The middle of the disc. Its y is the water level (puddleWaterLevel).
    glm::vec3 center{0.0F};
    /// The radius of the disc.
    float radius = PUDDLE_MIN_RADIUS;
};

/// How many puddles a maze with freeCells free cells gets: share of them, rounded to
/// the nearest whole number. A share of 0 or less gives none, a share of 1 or more one
/// per free cell.
int puddleCountFor(int freeCells, float share);

/// Chooses the puddles of a maze. The same maze, seed, start, exit, crystals and share
/// always give the same puddles, on every compiler: the choice uses std::mt19937 and
/// game::randomBelow only (see docs/decisions/deterministic-random.md). The terrain is
/// not asked: a new height scale never moves a puddle to another cell.
///
/// The rules:
///   - never the start cell, never the exit cell and never a cell with a crystal (the
///     free cells are all the others), and at most one puddle per cell,
///   - puddleCountFor(free cells, share) puddles, in cells picked by shuffling the free
///     cells with the seed,
///   - a larger share keeps every puddle a smaller share gave and adds more: the cells
///     are shuffled first, and the size and the place of a puddle are chosen in the
///     order of that list.
///
/// Throws std::out_of_range when start or exit is not a cell of the maze.
std::vector<PuddleSpawn> placePuddles(const Maze& maze, std::uint32_t seed, MazeCell start,
                                      MazeCell exit, std::span<const CrystalSpawn> crystals,
                                      float share);

/// Corner number corner of the rim of a disc of radius 1 around the origin, as (x, z):
/// the corners are spread evenly over the circle, PUDDLE_CORNERS of them, and corner
/// 0 lies on the +X axis. Corner numbers past the last one go around again.
glm::vec2 puddleRimCorner(int corner);

/// The height at which the water of a puddle stands: the lowest ground under the disc
/// plus PUDDLE_DEPTH. (x, z) is the middle of the disc.
///
/// WHY THE LOWEST GROUND. The ground is uneven and water is level, so the disc cannot
/// follow the ground. Put on the HIGHEST ground under it, its other side would hang in
/// the air: under the largest puddle the ground differs by up to 7 cm in the default
/// maze, and by 17 cm at the largest height scale. Put on the lowest ground, the disc
/// never stands more than PUDDLE_DEPTH above the ground anywhere along its rim.
/// Wherever the ground rises above the water level, the ground is simply drawn in front
/// of the disc (the depth test), and the shore follows the ground: on level ground
/// the whole disc shows, on a slope only the lower part of it, like water that has run
/// to the low side.
///
/// The lowest ground is looked up at the points of the disc that is drawn: its middle
/// and the PUDDLE_CORNERS corners of its rim (Terrain::heightAt).
float puddleWaterLevel(const Terrain& terrain, float x, float z, float radius);

/// The puddles of a world, standing on its terrain: placePuddles with the maze, the
/// seed, the exit and the crystals of the world, and every puddle at its water level.
/// Call it again after the terrain of the world was rebuilt: the same puddles come
/// back at their new heights.
std::vector<Puddle> puddlesOnGround(const MazeWorld& world, float share);

/// The disc of a puddle as triangles, ready for gfx::Mesh: plain data, so tests can
/// check it.
struct PuddleMeshData {
    /// The middle first, then the PUDDLE_CORNERS corners of the rim. The disc has the
    /// radius 1 and lies in the plane y = 0 around the origin: the model matrix of
    /// a puddle makes it as big as the puddle and moves it to its place. Every normal
    /// points straight up, which is what makes the disc a level mirror.
    std::vector<gfx::Vertex> vertices;

    /// Three indices per triangle, PUDDLE_CORNERS triangles, counter-clockwise when
    /// seen from above.
    std::vector<std::uint32_t> indices;
};

/// Builds the vertices and the indices of the disc.
PuddleMeshData buildPuddleMesh();

/// The model matrix of a puddle: the disc of buildPuddleMesh made puddle.radius wide
/// (not higher: it is flat) and moved to puddle.center.
glm::mat4 puddleModelMatrix(const Puddle& puddle);

} // namespace game
