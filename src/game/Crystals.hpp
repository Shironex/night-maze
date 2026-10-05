// Crystals: how many a maze gets, which cells they float in and how they move and glow.
// See docs/modules/game/gameplay.md
#pragma once

#include "game/Maze.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.

/// One crystal for this many cells of the maze: a maze of 10 by 10 cells gets 13.
constexpr int CELLS_PER_CRYSTAL = 8;

/// The number of crystal models. Each crystal is drawn with one of them.
constexpr int CRYSTAL_VARIANT_COUNT = 2;

// All sizes are in metres.

/// Height of the crystal models, from their base (the origin of the model) to their tip.
constexpr float CRYSTAL_HEIGHT = 0.5F;

/// How high above the ground the base of a resting crystal floats: the middle of the
/// crystal is then 1.15 m above it, a little above the middle of the body of the player.
constexpr float CRYSTAL_FLOAT_HEIGHT = 0.9F;

/// The crystal moves up and down by this much around its resting height, once in
/// CRYSTAL_BOB_SECONDS.
constexpr float CRYSTAL_BOB_AMPLITUDE = 0.08F;
constexpr float CRYSTAL_BOB_SECONDS = 3.0F;

/// How fast a crystal turns around its vertical axis, in degrees per second: one full
/// turn in 9 seconds.
constexpr float CRYSTAL_SPIN_DEGREES_PER_SECOND = 40.0F;

/// The point light of a crystal hangs this far above its tip. It must be outside the
/// mesh: a light inside a closed mesh shines on the faces from behind and lights none
/// of them.
constexpr float CRYSTAL_LIGHT_CLEARANCE = 0.15F;

/// The light of a crystal pulses: its brightness goes from full down to
/// 1 - CRYSTAL_PULSE_DEPTH of it and back, once in CRYSTAL_PULSE_SECONDS.
constexpr float CRYSTAL_PULSE_DEPTH = 0.3F;
constexpr float CRYSTAL_PULSE_SECONDS = 2.4F;

/// How strongly a crystal glows by itself, compared with the (linear) colour of its
/// light (see crystalGlow). Above 1 on purpose: the scene is drawn into an HDR buffer,
/// where a colour may be brighter than white, and a glowing crystal is the one thing in
/// the maze that should be. The tone mapping of the composite pass brings it back into
/// the range of the screen without turning the crystal into one flat patch, so its
/// facets can still be told apart. An effect that looks for the pixels above 1 (bloom)
/// finds the crystals by this.
constexpr float CRYSTAL_GLOW_STRENGTH = 2.5F;

/// One crystal of a maze: the cell it floats in and which model it is drawn with.
struct CrystalSpawn {
    MazeCell cell;
    /// 0 to CRYSTAL_VARIANT_COUNT - 1.
    int variant = 0;
};

/// How many crystals a maze of cellCount cells should get: one for every
/// CELLS_PER_CRYSTAL cells, rounded to the nearest whole number, but at least 1 and at
/// most scene::MAX_POINT_LIGHTS, because every crystal carries a point light and the
/// shader has room for that many.
int crystalCountFor(int cellCount);

/// Chooses the cells of the crystals of a maze. The same maze, seed, start and exit
/// always give the same crystals, on every compiler: the choice uses std::mt19937 and
/// game::randomBelow only (see docs/decisions/deterministic-random.md).
///
/// The rules:
///   - never the start cell and never the exit cell, and at most one crystal per cell,
///   - dead ends first, in an order shuffled by the seed: a dead end is a place worth
///     walking into only if something is there,
///   - when the dead ends run out, the other cells, also shuffled by the seed,
///   - crystalCountFor(cells) crystals, or fewer when the maze has fewer free cells
///     (a maze of one or two cells gets none).
///
/// Throws std::out_of_range when start or exit is not a cell of the maze.
std::vector<CrystalSpawn> placeCrystals(const Maze& maze, std::uint32_t seed, MazeCell start,
                                        MazeCell exit);

/// Where the base of a crystal rests: above the centre of its cell, CRYSTAL_FLOAT_HEIGHT
/// over the ground. groundHeight is the height of the ground at the centre of the cell
/// (Terrain::heightAt). It is passed in, so this function needs no terrain.
glm::vec3 crystalRestPosition(MazeCell cell, float groundHeight);

/// The middle of a crystal whose base is at basePosition: half of its height further up.
glm::vec3 crystalCenter(const glm::vec3& basePosition);

/// Where the point light of a crystal hangs: CRYSTAL_LIGHT_CLEARANCE above its tip.
glm::vec3 crystalLightPosition(const glm::vec3& basePosition);

/// The base of a crystal at a moment in time: restPosition moved up or down by at most
/// CRYSTAL_BOB_AMPLITUDE. index is the number of the crystal in its list: every crystal
/// starts at another point of the movement, so they do not all rise and fall together.
glm::vec3 crystalBobPosition(const glm::vec3& restPosition, int index, float seconds);

/// The angle of a crystal around its vertical axis at a moment in time, in degrees from
/// 0 to 360. index shifts the angle, for the same reason as in crystalBobPosition.
float crystalSpinDegrees(int index, float seconds);

/// The brightness of the crystals at a moment in time, as a factor between
/// 1 - CRYSTAL_PULSE_DEPTH and 1. All crystals share it: they pulse together.
float crystalPulse(float seconds);

/// The light a crystal gives off by itself at a moment in time (the uniform uEmissive
/// of the shaders): the colour of the crystal lights times CRYSTAL_GLOW_STRENGTH, dimmed
/// by crystalPulse, so the glow of the mesh and the light around it pulse together.
/// lightColor is a LINEAR colour (the caller converts the sRGB colour of the settings),
/// and so is the result, which may be brighter than 1.
glm::vec3 crystalGlow(const glm::vec3& lightColor, float seconds);

} // namespace game
