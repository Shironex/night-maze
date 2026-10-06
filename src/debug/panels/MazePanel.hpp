// "Maze" debug panel: size and seed of the maze, the numbers of levers and notes,
// regeneration and a plan seen from above with the crystals, the levers, the notes, the
// gate and the exit on it.
// See docs/modules/game/maze-generator.md
#pragma once

namespace game {
struct MazeSettings;
struct MazeWorld;
struct Player;
struct Round;
} // namespace game

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

/// Draws the "Maze" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// settings is editable: the panel changes the size and the seed of the next maze and
/// the wanted numbers of levers and notes, and sets settings.regenerate, which the game
/// reads at the start of the next frame.
/// world (the maze in play), round, player and camera are read only: they are drawn as
/// a plan with the walls (a wall that a lever has opened is drawn dim), the gate, the
/// exit zone, a dot for every crystal (a dim ring once collected), a square for every
/// lever (dim once pulled) and every note, a dot for the player and a short line for
/// the direction the camera looks in. A new maze also starts a new round.
void drawMazePanel(game::MazeSettings& settings, const game::MazeWorld& world,
                   const game::Round& round, const game::Player& player,
                   const scene::Camera& camera);

} // namespace debug
