// "Camera" debug panel: angles and projection of the camera, the player it follows, controls.
// See docs/modules/scene/camera-controls.md
#pragma once

namespace game {
struct MenuCameraSettings;
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
/// menuCamera is editable too: the switch of the menu camera, its shot, its speed, its
/// eye height and its time offset. menuCameraLoopSeconds, the time one loop of that
/// camera takes, is only shown.
void drawCameraPanel(scene::Camera& camera, game::Player& player, float& mouseSensitivity,
                     game::MenuCameraSettings& menuCamera, float menuCameraLoopSeconds);

} // namespace debug
