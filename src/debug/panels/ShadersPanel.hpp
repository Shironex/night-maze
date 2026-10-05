// "Shaders" debug panel: the files of every shader program, a reload button and the errors.
// See docs/modules/gfx/shader-hot-reload.md
#pragma once

#include <span>

namespace gfx {
class Shader;
} // namespace gfx

namespace debug {

/// Draws the "Shaders" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// shaders is the list of the programs of the game. The pointers are const (the list
/// cannot be changed), the shaders they point at are not: the "Reload shaders" button
/// calls reload() on every one of them. No pointer may be null.
void drawShadersPanel(std::span<gfx::Shader* const> shaders);

} // namespace debug
