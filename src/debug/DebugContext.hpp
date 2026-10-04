// Debug context: the data the debug panels may read or edit during one frame.
// See docs/modules/debug-ui.md
#pragma once

#include <array>

namespace core {
class Time;
class Window;
} // namespace core

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

/// Everything the debug panels may read or edit this frame.
///
/// A plain struct of references into objects owned by the application. main.cpp builds it
/// every frame and passes it to DebugUI::draw. It owns nothing and must not outlive the
/// frame it was built in.
///
/// A const member is read only for the panels, a non-const member can be edited by them.
/// A reference member keeps this meaning even when the struct itself is const: constness
/// of the struct does not pass through a reference to the object it refers to.
struct DebugContext {
    /// Frame clock, read only: FPS and frame time.
    const core::Time& time;
    /// Window, read only: sizes and OpenGL driver info.
    const core::Window& window;
    /// Background color (red, green, blue in the range 0 to 1), editable.
    std::array<float, 3>& clearColor;
    /// Shader program the game draws with, editable: the Shaders panel reloads it.
    gfx::Shader& shader;
    /// Camera of the game, editable: position, angles and projection.
    scene::Camera& camera;
    /// Mouse look sensitivity in degrees per screen coordinate unit, editable.
    float& mouseSensitivity;
    /// Camera movement speed in metres per second, editable.
    float& moveSpeed;
};

} // namespace debug
