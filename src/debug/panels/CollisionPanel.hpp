// "Collision" debug panel: the collision boxes as lines, their counts and the noclip mode.
// See docs/modules/scene/collision.md
#pragma once

namespace game {
struct MazeWorld;
struct Player;
} // namespace game

namespace debug {

/// Draws the "Collision" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// world is read only: the panel counts its collision boxes. player is editable: the
/// panel switches its noclip mode and shows its box. drawColliders is editable: while it
/// is true the game draws the boxes of the maze and of the player as lines.
void drawCollisionPanel(const game::MazeWorld& world, game::Player& player, bool& drawColliders);

} // namespace debug
