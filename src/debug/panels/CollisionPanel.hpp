// "Collision" debug panel: the collision boxes and spheres as lines, their counts and the
// noclip mode.
// See docs/modules/scene/collision.md
#pragma once

namespace game {
struct MazeWorld;
struct Player;
struct Round;
} // namespace game

namespace debug {

/// Draws the "Collision" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// world and round are read only: the panel counts the collision boxes of the maze and
/// the pickup spheres of the crystals that are left. player is editable: the panel
/// switches its noclip mode and shows its box. drawColliders is editable: while it is
/// true the game draws the boxes and the spheres as lines.
void drawCollisionPanel(const game::MazeWorld& world, const game::Round& round,
                        game::Player& player, bool& drawColliders);

} // namespace debug
