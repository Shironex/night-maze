// Grass: where the tufts of grass stand, one point per tuft, chosen from the seed of the maze.
#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace game {

struct MazeWorld;

// Plain data and math without OpenGL, like the rest of the game_logic library. The blades
// themselves do not exist here: a tuft is one point, and the geometry shader grass.geom
// builds its blades on the graphics card in every frame.

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

/// One tuft of grass.
struct GrassTuft {
    /// Where its roots are: a point on the ground (y is Terrain::heightAt).
    glm::vec3 position{0.0F};

    /// A random number from 0 to 1 that belongs to this tuft. The shader derives from it
    /// everything in which one tuft differs from the next: which way its blades face,
    /// how tall it is and at what moment it sways.
    float random = 0.0F;
};

/// What can be changed about the grass while the game runs. The debug UI edits the
/// fields. The values here are the defaults.
struct GrassSettings {
    /// Whether the grass is drawn.
    bool enabled = true;

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
/// grows under a stone sheep (game::stoneSheepBareGround).
bool grassIsTrampled(const MazeWorld& world, float x, float z);

} // namespace game
