// Heartstone: the one big splinter of a maze, which dead end it floats in and how it moves.
#pragma once

#include "game/Maze.hpp"

#include <glm/glm.hpp>

#include <optional>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.
//
// The heartstone is a pickup like a crystal, and it is NOT one of the crystals of the
// maze (MazeWorld::crystals): the number that opens the gate is counted from those alone,
// the map shows no mark for it and no note points at it. What taking it does is in
// game/Round.hpp (it counts as HEARTSTONE_WORTH crystals and fills the battery) and in
// game/Player.hpp (it is heavy: game::carryHeartstone).

/// How many crystals the heartstone counts as, for the gate and for "of N".
constexpr int HEARTSTONE_WORTH = 3;

// All sizes are in metres.

/// Height of the model, from its lower point (the origin) to the tip of its main sliver.
constexpr float HEARTSTONE_HEIGHT = 0.9F;

/// How high above the ground the lower point floats: low, so the heavy thing hangs at
/// the height of a knee and its tip at the height of a chest.
constexpr float HEARTSTONE_FLOAT_HEIGHT = 0.5F;

/// How fast it turns around its vertical axis, in degrees per second: one full turn in
/// 20 seconds, less than half as fast as a crystal (CRYSTAL_SPIN_DEGREES_PER_SECOND).
constexpr float HEARTSTONE_SPIN_DEGREES_PER_SECOND = 18.0F;

/// How much stronger the heartstone glows by itself than a crystal (game::crystalGlow
/// times this), and how much brighter and wider its point light is than the light of
/// a crystal (LightingSettings::pointIntensity and pointRadius times these).
constexpr float HEARTSTONE_GLOW_FACTOR = 1.6F;
constexpr float HEARTSTONE_LIGHT_INTENSITY_FACTOR = 1.3F;
constexpr float HEARTSTONE_LIGHT_RADIUS_FACTOR = 1.5F;

/// The cell of the heartstone: the dead end farthest from start, counted in passages
/// walked (game::passageDistances). Never the start cell, never the exit cell and never
/// the cell in front of the gate. Of several dead ends at the same distance the first one
/// in row order wins, like in game::farthestCell. No random number is drawn, so the cell
/// depends on the walls alone.
///
/// The exit is the farthest cell of all, so this is the farthest dead end after it.
/// Empty when the maze has no such dead end: a maze too small gets no heartstone.
///
/// Throws std::out_of_range when start is not a cell of the maze.
std::optional<MazeCell> heartstoneCell(const Maze& maze, MazeCell start, MazeCell exit);

/// Where the lower point of the heartstone rests: above the centre of its cell,
/// HEARTSTONE_FLOAT_HEIGHT over the ground. groundHeight is the height of the ground at
/// the centre of the cell (Terrain::heightAt).
glm::vec3 heartstoneRestPosition(MazeCell cell, float groundHeight);

/// The middle of a heartstone whose lower point is at basePosition: the centre of its
/// pickup sphere.
glm::vec3 heartstoneCenter(const glm::vec3& basePosition);

/// Where the point light of the heartstone hangs: CRYSTAL_LIGHT_CLEARANCE above its tip,
/// outside the mesh for the reason given there.
glm::vec3 heartstoneLightPosition(const glm::vec3& basePosition);

/// The angle of the heartstone around its vertical axis at a moment in time, in degrees
/// from 0 to 360.
float heartstoneSpinDegrees(float seconds);

} // namespace game
