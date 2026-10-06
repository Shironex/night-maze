// Maze plan of the debug window: the maze seen from above, with the crystals, the
// levers, the notes, the gate, the exit and the player on it.
// See docs/modules/game/maze-generator.md
#pragma once

namespace game {
struct MazeWorld;
struct Player;
struct Round;
} // namespace game

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

/// Draws the plan of the maze in play at the place of the next widget, north (-Z) at
/// the top, with the draw list of the current window. The longer side of the maze is
/// width pixels long.
///
/// On the plan: the walls (a wall that a lever has opened is drawn dim), the gate, the
/// exit zone, a dot for every crystal (a dim ring once collected), a square for every
/// lever (dim once pulled) and every note, a dot for the player and a short line for
/// the direction the camera looks in. The colours are in Theme.hpp.
///
/// Everything is read only. Call it inside an ImGui window.
void drawMazePlan(const game::MazeWorld& world, const game::Round& round,
                  const game::Player& player, const scene::Camera& camera, float width);

} // namespace debug
