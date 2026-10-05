// "Renderer" debug panel: frame statistics, OpenGL driver info, clear color, lighting mode.
// See docs/modules/debug-ui.md
#pragma once

#include <array>

namespace core {
class Time;
class Window;
} // namespace core

namespace game {
enum class LightingMode;
} // namespace game

namespace debug {

/// Draws the "Renderer" panel. Called by DebugUI::draw, inside the ImGui frame.
/// clearColor (red, green, blue in the range 0 to 1) and lightingMode (how the maze is
/// shaded: unlit, Gouraud, Phong or Blinn-Phong) can be edited by the user.
void drawRendererPanel(const core::Time& time, const core::Window& window,
                       std::array<float, 3>& clearColor, game::LightingMode& lightingMode);

} // namespace debug
