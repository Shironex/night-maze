// "Camera" debug panel: angles and projection of the camera, the player it follows, controls.
// See docs/modules/scene/camera-controls.md
#pragma once

namespace game {
struct Player;
} // namespace game

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

/// Draws the "Camera" panel. Called by DebugUI::draw, inside the ImGui frame.
/// Editable: the angles and the projection of camera, the position and the three speeds
/// of player, and mouseSensitivity (degrees per screen coordinate unit of mouse movement).
/// The position of the camera is only shown: it follows the eyes of the player.
void drawCameraPanel(scene::Camera& camera, game::Player& player, float& mouseSensitivity);

} // namespace debug
