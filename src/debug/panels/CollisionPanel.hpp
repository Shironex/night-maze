// "Collision" debug panel: the collision boxes and spheres as lines, their counts, the
// noclip mode and the result of the last picking ray.
// See docs/modules/scene/collision.md
#pragma once

namespace game {
struct MazeWorld;
struct PickDebugSettings;
struct PickState;
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
///
/// pick is read only: the panel shows the picking ray of the last frame (where it
/// starts and where it points), what it hit (a lever or a note, its number and its
/// distance) and what the interaction key does. pickDebug is editable: the switches
/// that draw the pick boxes and the ray and that freeze the drawn ray.
void drawCollisionPanel(const game::MazeWorld& world, const game::Round& round,
                        game::Player& player, bool& drawColliders, const game::PickState& pick,
                        game::PickDebugSettings& pickDebug);

} // namespace debug
