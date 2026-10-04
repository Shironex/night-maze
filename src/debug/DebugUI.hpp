// Debug user interface: owns the Dear ImGui context and draws the debug panels.
// See docs/modules/debug-ui.md
#pragma once

#include <array>

namespace core {
class Time;
class Window;
} // namespace core

namespace debug {

/// Owns Dear ImGui for the lifetime of the object (RAII) and draws all debug panels.
///
/// The constructor sets ImGui up for the given window, the destructor shuts it down.
/// It must be destroyed before the window, because shutdown needs the OpenGL context.
class DebugUI {
public:
    /// Initializes ImGui with the GLFW and OpenGL 3 backends for this window.
    explicit DebugUI(const core::Window& window);
    ~DebugUI();

    DebugUI(const DebugUI&) = delete;
    DebugUI& operator=(const DebugUI&) = delete;

    /// Shows or hides all panels.
    void toggleVisible() { m_visible = !m_visible; }

    /// True while ImGui uses the keyboard itself: a text field is being edited or another
    /// widget is active (for example a slider being dragged).
    /// ImGui computes this at the start of each frame it builds, so it lags by a frame.
    bool wantsKeyboard() const;

    /// Builds and renders the debug UI on top of the current frame.
    /// Call it last in the frame, after the scene has been drawn.
    void draw(const core::Time& time, const core::Window& window, std::array<float, 3>& clearColor);

private:
    bool m_visible = true;
};

} // namespace debug
