// "Renderer" debug panel: frame statistics and OpenGL driver info.
// See docs/modules/debug-ui.md
#pragma once

namespace core {
class Time;
class Window;
} // namespace core

namespace debug {

/// Draws the "Renderer" panel. Called by DebugUI::draw, inside the ImGui frame. It only
/// shows: its controls are in the Render category of the debug window.
void drawRendererPanel(const core::Time& time, const core::Window& window);

} // namespace debug
