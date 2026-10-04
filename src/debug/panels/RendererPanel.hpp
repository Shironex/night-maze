// "Renderer" debug panel: frame statistics, OpenGL driver info and the clear color.
// See docs/modules/debug-ui.md
#pragma once

#include <array>

namespace core {
class Time;
class Window;
} // namespace core

namespace debug {

/// Draws the "Renderer" panel. Called by DebugUI::draw, inside the ImGui frame.
/// clearColor (red, green, blue in the range 0 to 1) can be edited by the user.
void drawRendererPanel(const core::Time& time, const core::Window& window,
                       std::array<float, 3>& clearColor);

} // namespace debug
