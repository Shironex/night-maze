// Grass: where the tufts of grass stand, one point per tuft, chosen from the seed of the maze.
#pragma once

#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace game {

struct MazeWorld;

// Plain data and math without OpenGL, like the rest of the game_logic library. The blades
// themselves do not exist here: a tuft is one point, and the graphics card builds its
// blades in every frame, either in the geometry shader grass.geom or in the vertex shader
// grass_instanced.vert (see GrassPath).

// All sizes are in metres.

/// A tuft keeps this far away from both ends of its wall. A pillar stands at each end
/// (its foot is 0.4 m wide, so 0.2 m of it reach along the wall), and a wall that meets
/// this one at the corner covers 0.15 m. With the clearance a tuft is in neither.
constexpr float GRASS_END_CLEARANCE = 0.3F;

/// The strip of ground beside a wall in which the tufts stand: it starts this far from
/// the collision box of the wall (a little more than game::FOOTPRINT_MARGIN, so a tuft
/// is clear of the foot of the wall too) and is this wide. Where in the strip a tuft
/// stands is random, so the grass is not a ruler-straight line.
constexpr float GRASS_WALL_GAP = 0.06F;
constexpr float GRASS_STRIP_WIDTH = 0.22F;

/// Tufts on the land outside the maze, per square metre, at the default density. The
/// hills get a sparse scatter. The number follows the density setting in proportion.
constexpr float GRASS_HILL_TUFTS_PER_SQUARE_METRE = 0.35F;

/// No hill tuft stands closer to the maze than this: the strip next to the outer walls
/// belongs to the tufts of those walls.
constexpr float GRASS_HILL_CLEARANCE = 0.6F;

/// The density the game starts with and the largest one its slider offers, in tufts per
/// metre of wall, counted on each side of the wall separately.
constexpr float DEFAULT_GRASS_DENSITY = 2.5F;
constexpr float MAX_GRASS_DENSITY = 8.0F;

/// The blades of one tuft: BLADES_PER_TUFT in assets/shaders/common/grass_blade.glsl.
constexpr int GRASS_BLADES_PER_TUFT = 3;

/// The shape of a blade, the same numbers as in common/grass_blade.glsl (the test
/// "the blade numbers of C++ are the ones of the shader" reads them there). The C++ side
/// needs them for one thing only: how far a blade can reach from the root of its tuft,
/// which makes the box a chunk of grass is culled with (grassChunkBox).
///
/// Half of the width of a blade at its root, how far its root stands from the middle of
/// the tuft, how far its tip leans as a part of its height, and how far the wind pushes
/// a tip at wind strength 1, all in metres or parts of the height.
constexpr float GRASS_ROOT_HALF_WIDTH = 0.02F;
constexpr float GRASS_ROOT_SPREAD = 0.025F;
constexpr float GRASS_LEAN = 0.4F;
constexpr float GRASS_WIND_REACH = 0.06F;

/// The side of the square a chunk of grass covers on the ground, in metres: two cells of
/// the maze. The tufts are grouped by the square they stand in, so that a whole group can
/// be left out when its box is outside the view (chunk culling). Smaller chunks fit the
/// view more closely but need more boxes to test and more draw calls.
constexpr float GRASS_CHUNK_SIZE = 4.0F;

/// The distances of the distance LOD (level of detail) the game starts with, in metres:
/// from GRASS_LOD_NEAR on the third blade of a tuft shrinks away, from GRASS_LOD_FAR on
/// the second one. The first blade stays at every distance.
constexpr float DEFAULT_GRASS_LOD_NEAR = 12.0F;
constexpr float DEFAULT_GRASS_LOD_FAR = 24.0F;

/// The largest distance the two sliders of the LOD offer, in metres.
constexpr float MAX_GRASS_LOD_DISTANCE = 60.0F;

/// The width of the band in which a blade shrinks away, in metres. Every tuft starts to
/// shrink its blade at its own distance inside one more band of this width (it is
/// chosen by the random number of the tuft), so the blades do not vanish along one line.
/// A blade is therefore gone for good two bands behind its LOD distance.
constexpr float GRASS_LOD_FADE = 4.0F;

/// One tuft of grass.
struct GrassTuft {
    /// Where its roots are: a point on the ground (y is Terrain::heightAt).
    glm::vec3 position{0.0F};

    /// A random number from 0 to 1 that belongs to this tuft. The shader derives from it
    /// everything in which one tuft differs from the next: which way its blades face,
    /// how tall it is and at what moment it sways.
    float random = 0.0F;
};

/// The two ways the blades are made on the graphics card. Both draw the same blades.
enum class GrassPath {
    /// The vertex shader grass_instanced.vert: one small mesh of the three blades of
    /// a tuft is drawn once for every tuft (instancing), and each copy reads the root and
    /// the random number of its own tuft. The default.
    Instanced = 0,
    /// The geometry shader grass.geom: one point per tuft goes in, the blades come out.
    /// Kept as the example of a geometry shader. It is slow on a Mac, whose graphics
    /// cards have no geometry stage of their own: the driver imitates it.
    GeometryShader = 1,
};

/// What can be changed about the grass while the game runs. The debug UI edits the
/// fields. The values here are the defaults.
struct GrassSettings {
    /// Whether the grass is drawn.
    bool enabled = true;

    /// How the blades are made (GrassPath).
    GrassPath path = GrassPath::Instanced;

    /// Whether a chunk of grass whose box lies outside the view is left out. Only while
    /// the frustum culling of the whole game is on too (the switch in Diagnostics).
    bool chunkCulling = true;

    /// Whether far tufts have fewer blades (lodNear, lodFar).
    bool distanceLod = true;

    /// The distances of the LOD in metres: the third blade of a tuft shrinks away
    /// behind lodNear, the second one behind lodFar (see GRASS_LOD_FADE).
    float lodNear = DEFAULT_GRASS_LOD_NEAR;
    float lodFar = DEFAULT_GRASS_LOD_FAR;

    /// Tufts per metre of wall, on each side of the wall. Changing it means choosing the
    /// places again: the panel sets replant.
    float density = DEFAULT_GRASS_DENSITY;

    /// Height of the tallest blades, in metres. A tuft is between about two thirds of
    /// this and all of it.
    float bladeHeight = 0.3F;

    /// How far the wind pushes the tips of the blades: 0 is still air, 1 the default
    /// breeze.
    float windStrength = 1.0F;

    /// True when the density changed and the tufts have not been placed again yet. The
    /// debug UI sets it, the application places the tufts at the start of the next frame
    /// and clears it, like MazeSettings::regenerate.
    bool replant = false;
};

/// Chooses the places of the tufts of a world. The same world and density always give
/// the same tufts. The random numbers behind them are the same on every compiler: the
/// choice uses std::mt19937 and game::randomBelow only: the standard fixes the output of the
/// generator but not that of std::uniform_int_distribution or std::shuffle, which differ
/// between compilers.
///
/// The rules:
///   - along both sides of every wall, density tufts per metre of wall and side (the
///     number per side is rounded), each at a random place in the strip beside the wall,
///   - never inside a wall or a pillar: the strip starts outside the box of the wall and
///     ends GRASS_END_CLEARANCE before each corner,
///   - none at the gate: the open side of the exit cell has no wall, so no strip,
///   - none on the trampled ground in front of the exit and of the stile (grassIsTrampled). Those
///   tufts
///     are chosen like all the others and then left out, so the rule moves no other
///     tuft of a seed,
///   - a sparse scatter on the land outside the maze, never closer to it than
///     GRASS_HILL_CLEARANCE,
///   - every tuft stands on the ground: its y is the height of the terrain there.
///
/// A density of 0 or less gives no tufts at all.
std::vector<GrassTuft> placeGrass(const MazeWorld& world, float density);

/// True for a point of the ground where no grass grows because feet have worn it bare:
/// inside the exit cell, and inside the cell in front of its gate (game::approachCell),
/// the last one every lamplighter walked through. The bare ground is one of the signs
/// that tell the exit from an ordinary dead end. A maze without a gate has none of that.
/// The other bare place is the ground in front of the stile of the start cell
/// (game::stileWornGround), in a world that has one (MazeWorld::stileWall). And no grass
/// grows under a stone sheep (game::stoneSheepBareGround), nor under the field walls, the
/// hedgerows and the buildings of the land around the maze (game::landCoversGround).
bool grassIsTrampled(const MazeWorld& world, float x, float z);

/// The tufts that stand in one square of GRASS_CHUNK_SIZE: a range of GrassChunks::tufts.
struct GrassChunk {
    /// The first tuft of the chunk in GrassChunks::tufts, and how many follow it.
    std::uint32_t firstTuft = 0;
    std::uint32_t tuftCount = 0;

    /// The smallest box around the roots of its tufts. The blades reach out of it:
    /// grassChunkBox makes it large enough for them.
    scene::Aabb roots;
};

/// The tufts of a world grouped into chunks: tufts holds every tuft once, ordered so
/// that the tufts of one chunk follow each other, and chunks says where each chunk
/// begins.
struct GrassChunks {
    std::vector<GrassTuft> tufts;
    std::vector<GrassChunk> chunks;
};

/// Groups tufts into squares of chunkSize metres on the ground. The squares are a fixed
/// grid of the world with a corner in its origin, so a square of the maze is the same
/// square in every world. The chunks come row by row (z, then x), so
/// chunks that stand side by side in a row are neighbours in the list too, and a row of
/// visible chunks becomes one range of tufts that one draw call can draw. Inside
/// a chunk the tufts keep their order. No chunk is empty.
///
/// This only reorders: the tufts that come out are the tufts that went in. placeGrass
/// itself is untouched by it, so the places of the grass of a seed stay what they are.
/// A chunkSize of 0 or less puts every tuft into one chunk.
GrassChunks groupGrassIntoChunks(std::span<const GrassTuft> tufts, float chunkSize);

/// The box that holds every blade of a chunk: the box of the roots, wider on every side
/// by how far a blade can reach sideways (its spread, its lean, its width and the wind)
/// and higher by the height of the tallest blades. The frustum test needs nothing more
/// precise: a box that is a little too large only keeps a chunk that shows nothing.
scene::Aabb grassChunkBox(const scene::Aabb& roots, float bladeHeight, float windStrength);

/// The distance behind which the second blade of a tuft shrinks away: settings.lodFar,
/// but never less than settings.lodNear, so the blades always go in the order third,
/// second. The renderer gives the shader this number, not lodFar itself.
float grassLodFar(const GrassSettings& settings);

/// How many blades a tuft at this distance from the eye still shows with the distance
/// LOD of settings: 3 near, then 2, then 1. A blade counts while any part of it can
/// still be seen: up to two bands of GRASS_LOD_FADE behind its LOD distance (the shader
/// starts to shrink it somewhere in the first band and needs the second one to finish).
/// Without the LOD (settings.distanceLod false) every tuft shows all of its blades.
///
/// The renderer asks it with the distance of the nearest point of a chunk: no tuft of
/// the chunk is nearer, so none of them needs more blades.
int grassBladesAt(float distance, const GrassSettings& settings);

/// One draw call of the grass: a range of GrassChunks::tufts and how many blades each of
/// its tufts gets.
struct GrassDrawRange {
    std::uint32_t firstTuft = 0;
    std::uint32_t tuftCount = 0;
    int blades = 0;
};

/// The draw calls for a list of chunks. bladesPerChunk holds one number for every chunk:
/// 0 for a chunk that is not drawn (it is outside the view), otherwise how many blades
/// its tufts get. Chunks that follow each other in the list and get the same number are
/// joined into one range. Both spans must have the same length.
std::vector<GrassDrawRange> grassDrawRanges(std::span<const GrassChunk> chunks,
                                            std::span<const int> bladesPerChunk);

} // namespace game
