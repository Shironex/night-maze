// "Camera" debug panel: position, angles and projection of the camera, and its controls.
// See docs/modules/scene/transforms-camera.md
#pragma once

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

/// Draws the "Camera" panel. Called by DebugUI::draw, inside the ImGui frame.
/// Everything is editable: the fields of camera, mouseSensitivity (degrees per screen
/// coordinate unit of mouse movement) and moveSpeed (metres per second).
void drawCameraPanel(scene::Camera& camera, float& mouseSensitivity, float& moveSpeed);

} // namespace debug
