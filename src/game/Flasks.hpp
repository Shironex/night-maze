// Flasks: the flasks of tea a maze gets, which cells they lie in and how they glow.
#pragma once

#include "game/Crystals.hpp"
#include "game/Maze.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.
//
// A flask is a pickup like a crystal: the player walks into it. It is a shepherd's
// flask of tea, and drinking it stops the stamina from draining for a while
// (game::drinkFlask in game/Player.hpp). What a flask does to the round is in
// game/Round.hpp. This file only knows where the flasks are.

/// The most flasks a maze can have, whoever asks. The three difficulty levels ask for
/// 1, 2 and 3 (game/Difficulty.hpp). The debug UI can ask for more.
constexpr int MAX_FLASK_COUNT = 8;

/// How high above the ground a flask floats, in metres: low, at the height of a knee.
/// It is something that was left on the ground, not a light in the air like a crystal.
constexpr float FLASK_FLOAT_HEIGHT = 0.35F;

/// The light a flask gives off by itself (the uniform uEmissive of the shaders), as
/// a LINEAR colour: a dim warm orange, the colour of tea against a lamp. Dim on
/// purpose. It is far below the glow of a crystal (CRYSTAL_GLOW_STRENGTH) and below
/// the threshold of the bloom, so a flask has no halo and lights nothing around it: it
/// only stays visible in a dark dead end. It has no point light either: the shaders
/// have a fixed number of those, and the crystals use them.
constexpr glm::vec3 FLASK_GLOW{1.1F, 0.5F, 0.16F};

/// Chooses the cells of the flasks of a maze. The same maze, seed, start, exit and
/// crystals always give the same flasks, on every compiler: the choice uses
/// std::mt19937 and game::randomBelow only, like placeCrystals.
///
/// The rules:
///   - never the start cell, never the exit cell and never a cell that holds a crystal,
///     and at most one flask per cell,
///   - dead ends first, in an order shuffled by the seed. The crystals fill the dead
///     ends before anything else, so the dead ends that are left are the empty ones:
///     a flask gives the player a reason to walk into them,
///   - when no such dead end is left, the other free cells, also shuffled by the seed.
///     That happens often: a maze has about one dead end for ten cells, and the Easy
///     and the Normal level have more crystals than dead ends. Over the seeds 1 to 50,
///     the Easy mazes had a free dead end in 3 of 50 cases and the Normal ones in 18.
///     Only the Hard mazes always had one (8 on average),
///   - wantedCount flasks (brought into 0 to MAX_FLASK_COUNT), or fewer when the maze
///     has fewer free cells.
///
/// The flasks have a random generator of their own, seeded with the seed of the maze
/// plus a number no other generator uses. So placing flasks never changes where the
/// crystals, the levers and the notes of a seed are: it does not use up any of their
/// random numbers.
///
/// A larger wantedCount only adds flasks at the end of the list: the first ones are
/// the same cells as with a smaller number.
std::vector<MazeCell> placeFlasks(const Maze& maze, std::uint32_t seed, MazeCell start,
                                  MazeCell exit, std::span<const CrystalSpawn> crystals,
                                  int wantedCount);

/// Where a flask rests: above the centre of its cell, FLASK_FLOAT_HEIGHT over the
/// ground. groundHeight is the height of the ground at the centre of the cell
/// (Terrain::heightAt). It is passed in, so this function needs no terrain.
glm::vec3 flaskRestPosition(MazeCell cell, float groundHeight);

} // namespace game
