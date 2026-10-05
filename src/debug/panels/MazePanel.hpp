// "Maze" debug panel: size and seed of the maze, regeneration and a plan seen from above.
// See docs/modules/game/maze-generator.md
#pragma once

namespace game {
struct MazeSettings;
struct MazeWorld;
struct Player;
} // namespace game

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

/// Draws the "Maze" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// settings is editable: the panel changes the size and the seed of the next maze and
/// sets settings.regenerate, which the game reads at the start of the next frame.
/// world (the maze in play), player and camera are read only: they are drawn as a plan
/// with a dot for the player and a short line for the direction the camera looks in.
void drawMazePanel(game::MazeSettings& settings, const game::MazeWorld& world,
                   const game::Player& player, const scene::Camera& camera);

} // namespace debug
