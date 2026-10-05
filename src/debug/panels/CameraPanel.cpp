// "Camera" debug panel: angles and projection of the camera, the player it follows, controls.
// See docs/modules/scene/camera-controls.md
#include "debug/panels/CameraPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/Player.hpp"
#include "scene/Camera.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <algorithm>

namespace debug {

namespace {

// How much the position changes for one pixel of dragging, in metres.
constexpr float POSITION_DRAG_SPEED = 0.05F;

// Yaw is kept in the range from 0 to 360 by scene::Camera::rotate.
constexpr float MIN_YAW_DEGREES = 0.0F;
constexpr float MAX_YAW_DEGREES = 360.0F;

// Field of view: 20 is a strong zoom, 120 is very wide. Close to 0 or to 180 the
// projection breaks down.
constexpr float MIN_FOV_DEGREES = 20.0F;
constexpr float MAX_FOV_DEGREES = 120.0F;

// The near plane must stay above 0. The upper limit is far enough to cut into the walls
// around the player.
constexpr float MIN_NEAR_PLANE = 0.01F;
constexpr float MAX_NEAR_PLANE = 10.0F;

// The lower limit is small enough to cut off the far end of a corridor.
constexpr float MIN_FAR_PLANE = 1.0F;
constexpr float MAX_FAR_PLANE = 200.0F;

// Smallest distance between the two planes. They must never be equal: the projection
// matrix divides by (far - near).
constexpr float MIN_PLANE_DISTANCE = 0.1F;

constexpr float MIN_MOUSE_SENSITIVITY = 0.01F;
constexpr float MAX_MOUSE_SENSITIVITY = 1.0F;

// Limits of the three speeds of the player, in metres per second. At the upper limit one
// fixed step (1/120 s) is about 17 cm long, still short next to a wall box (30 cm thick).
// scene::moveAndSlide never jumps over a box whatever the step, but it moves one axis at
// a time, and that staircase path only stays close to the straight one for short steps.
constexpr float MIN_MOVE_SPEED = 0.5F;
constexpr float MAX_MOVE_SPEED = 20.0F;

} // namespace

void drawCameraPanel(scene::Camera& camera, game::Player& player, float& mouseSensitivity) {
    // First run only: the top edge of the window, right of the left column, folded to its
    // title bar (the constant is in PanelLayout.hpp). Later ImGui remembers the panel in
    // imgui.ini.
    placePanelOnFirstUse(CAMERA_PLACEMENT);
    if (ImGui::Begin("Camera")) {
        ImGui::TextWrapped("Click the scene to capture the mouse, Esc releases it. While "
                           "captured the mouse looks around. Walking: W A S D walk level, "
                           "Left Shift sprints. Noclip (key N): W A S D fly along the view, "
                           "Space goes up, Left Shift goes down.");

        ImGui::Separator();
        ImGui::Text("Mode: %s", player.noclip ? "noclip (free flight)" : "walking");
        // The camera has no position of its own to edit: after every fixed step the game
        // puts it at the eyes of the player. So the field that can be dragged is the
        // position of the player (the feet), and the eye is only shown. DragFloat3 edits
        // three floats through the pointer: x, y and z. While walking the game keeps y at
        // the floor, so a change of y lasts only in noclip mode.
        ImGui::DragFloat3("Player feet", glm::value_ptr(player.position), POSITION_DRAG_SPEED);
        ImGui::Text("Eye: %.2f, %.2f, %.2f", camera.position.x, camera.position.y,
                    camera.position.z);

        // A slider can also be typed into (Ctrl and click), and a typed value may be
        // outside the limits. AlwaysClamp forces it back between them. It matters most
        // for pitch: the field is public and only Camera::rotate clamps it, and a pitch of
        // 90 degrees breaks the view matrix.
        ImGui::SliderFloat("Yaw", &camera.yawDegrees, MIN_YAW_DEGREES, MAX_YAW_DEGREES, "%.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Pitch", &camera.pitchDegrees, -scene::Camera::MAX_PITCH_DEGREES,
                           scene::Camera::MAX_PITCH_DEGREES, "%.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);

        ImGui::Separator();
        ImGui::SliderFloat("FOV", &camera.fovDegrees, MIN_FOV_DEGREES, MAX_FOV_DEGREES, "%.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
        // Logarithmic: half of the slider covers the small values, where a change of the
        // near plane matters most.
        ImGui::SliderFloat("Near plane", &camera.nearPlane, MIN_NEAR_PLANE, MAX_NEAR_PLANE,
                           "%.2f m", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Far plane", &camera.farPlane, MIN_FAR_PLANE, MAX_FAR_PLANE, "%.1f m",
                           ImGuiSliderFlags_AlwaysClamp);
        // The ranges of the two sliders overlap, so keep the far plane behind the near one.
        camera.farPlane = std::max(camera.farPlane, camera.nearPlane + MIN_PLANE_DISTANCE);

        ImGui::Separator();
        ImGui::SliderFloat("Mouse sensitivity", &mouseSensitivity, MIN_MOUSE_SENSITIVITY,
                           MAX_MOUSE_SENSITIVITY, "%.2f deg/unit", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Walk speed", &player.walkSpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED,
                           "%.1f m/s", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Sprint speed", &player.sprintSpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED,
                           "%.1f m/s", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Fly speed", &player.flySpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED,
                           "%.1f m/s", ImGuiSliderFlags_AlwaysClamp);
    }
    ImGui::End();
}

} // namespace debug
