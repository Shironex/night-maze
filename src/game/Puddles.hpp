// Puddles: which cells of a maze get a puddle, how big it is and the mesh that lays its
// water on the ground.
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
//
// A puddle is a thin film of water that LIES ON the ground: a round patch whose every
// point is a little above the ground under it. It is not a level disc. The ground of
// the game is uneven, and a level disc on it is either cut off by the higher ground in
// a straight line or hangs in the air over the lower ground. The film follows the
// ground instead, like a wet patch. Only its normal stays straight up, so it still
// mirrors the sky like level water (see buildPuddleMesh).

// All sizes are in metres.

/// A puddle is round when seen from above. Its radius is between these two numbers,
/// chosen from the seed, so the puddles are not all the same.
constexpr float PUDDLE_MIN_RADIUS = 0.25F;
constexpr float PUDDLE_MAX_RADIUS = 0.45F;

/// The middle of a puddle lies at most this far from the centre of its cell, along
/// X and along Z, so the puddles do not stand in a straight row down a corridor. With
/// the largest radius a puddle reaches 0.75 m from the centre of its cell. The collision
/// box of a wall starts 0.85 m from it (half a cell of 2 m minus half of the 0.3 m the
/// box is thick) and the foot of a wall or a pillar 0.8 m (game::FOOTPRINT_MARGIN), so
/// no puddle ever reaches a wall, a pillar or the gate.
constexpr float PUDDLE_MAX_OFFSET = 0.3F;

/// How far the film of water lies above the ground, at every vertex of its mesh.
///
/// It has to be more than nothing for two reasons. The depth buffer cannot tell two
/// surfaces at the same place apart (they would flicker through each other), and the
/// film is only exactly parallel to the ground AT its vertices: between them it is
/// flat, while the ground under it may have a crease (an edge between two triangles of
/// the terrain, which are TERRAIN_SPACING wide). Where such a crease is a ridge, the
/// ground rises towards the film. Measured on the heightmap of the game with the mesh
/// below (PuddleTests.cpp): under the 13 puddles of the default maze the ground comes
/// at most 0.5 mm closer to the film at the default height scale and 1.2 mm at the
/// largest one, and with the largest puddle tried all over the maze 0.7 mm and 1.7 mm.
/// So 8 mm leaves more than 6 mm of air everywhere. A depth buffer of 24 bits with the
/// planes of the camera (0.1 m and 100 m) tells surfaces 6 mm apart from each other up
/// to about 30 m away, and the fog hides the ground long before that. From the height
/// of the eyes 8 mm cannot be seen as a gap.
constexpr float PUDDLE_LIFT = 0.008F;

/// The mesh of a puddle is a vertex in the middle and PUDDLE_RINGS rings around it, each
/// with PUDDLE_CORNERS vertices: like a spider web. The corners make the rim round (32
/// of them are 99.4 % of a circle). The rings are what lets the film follow the ground:
/// with 6 of them a vertex is never more than 7.5 cm from the next one along a spoke,
/// much finer than the terrain (TERRAIN_SPACING, 0.5 m).
constexpr int PUDDLE_CORNERS = 32;
constexpr int PUDDLE_RINGS = 6;

/// The numbers of vertices and of triangles of one puddle. The innermost ring is joined
/// to the middle by one triangle per corner, every other ring to the ring inside it by
/// two.
constexpr int PUDDLE_VERTEX_COUNT = 1 + PUDDLE_RINGS * PUDDLE_CORNERS;
constexpr int PUDDLE_TRIANGLE_COUNT = PUDDLE_CORNERS * (2 * PUDDLE_RINGS - 1);

/// The outer part of a puddle fades out, so that the water has no hard edge: over this
/// part of the radius, counted from the rim inwards, the film goes from fully there to
/// not there at all (reflect.frag, uniform uRimFade). The corners of the rim cannot be
/// seen then, because the rim itself is invisible.
constexpr float PUDDLE_RIM_FADE = 0.45F;

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
    /// The middle of the puddle. Its y is the height of the film there: the ground
    /// under the middle plus PUDDLE_LIFT.
    glm::vec3 center{0.0F};
    /// The radius of the puddle, measured level (along X and Z).
    float radius = PUDDLE_MIN_RADIUS;
};

/// How many puddles a maze with freeCells free cells gets: share of them, rounded to
/// the nearest whole number. A share of 0 or less gives none, a share of 1 or more one
/// per free cell.
int puddleCountFor(int freeCells, float share);

/// Chooses the puddles of a maze. The same maze, seed, start, exit, crystals and share
/// always give the same puddles, on every compiler: the choice uses std::mt19937 and
/// game::randomBelow only: the standard fixes the output of the generator but not that of
/// std::uniform_int_distribution or std::shuffle, which differ between compilers. The
/// terrain is not asked: a new height scale never moves a puddle to another cell.
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

/// The puddles of a world, lying on its terrain: placePuddles with the maze, the seed,
/// the exit and the crystals of the world, and the middle of every puddle PUDDLE_LIFT
/// above the ground there. Call it again after the terrain of the world was rebuilt:
/// the same puddles come back at their new heights.
std::vector<Puddle> puddlesOnGround(const MazeWorld& world, float share);

/// The water of all puddles as triangles, ready for gfx::Mesh: plain data, so tests can
/// check it.
struct PuddleMeshData {
    /// PUDDLE_VERTEX_COUNT vertices per puddle, puddle after puddle: the middle first,
    /// then the rings from the innermost to the rim, each starting at the corner on the
    /// +X side. The positions are in WORLD space (the mesh is drawn with the identity
    /// as its model matrix). Every normal points straight up.
    std::vector<gfx::Vertex> vertices;

    /// Three indices per triangle, PUDDLE_TRIANGLE_COUNT triangles per puddle,
    /// counter-clockwise when seen from above.
    std::vector<std::uint32_t> indices;
};

/// Builds the water of the given puddles on the terrain, as one mesh for all of them.
///
/// THE HEIGHTS. Every vertex is put PUDDLE_LIFT above the ground under it
/// (Terrain::heightAt, the same triangles the terrain is drawn with). So the film
/// follows the ground: no part of it is hidden by higher ground, and no part hangs in
/// the air. That is why the mesh is built for every puddle on its own and in world
/// space: each puddle lies on different ground. It has to be built again whenever the
/// terrain or the puddles change.
///
/// THE NORMALS. They do not follow the ground: every one is (0, 1, 0). The surface of
/// still water is level however the ground under it lies, and the normal is what
/// decides where the mirrored ray goes (reflect.frag). With the normals of the ground
/// every puddle would mirror another part of the sky and be lit like earth. With the
/// level normal it reads as water, a few millimetres thin.
///
/// THE TEXTURE COORDINATE runs from 0 to 1 across the puddle, with (0.5, 0.5) in the
/// middle, so its distance from the middle tells how far out a point lies: 0 in the
/// middle, 0.5 at the rim. reflect.frag fades the water out towards the rim with it
/// (PUDDLE_RIM_FADE), and the debug view of the texture coordinates shows it.
PuddleMeshData buildPuddleMesh(const Terrain& terrain, std::span<const Puddle> puddles);

} // namespace game
