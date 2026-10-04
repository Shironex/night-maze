// "Shaders" debug panel: the files of the shader program, a reload button and the last error.
// See docs/modules/gfx/shader-hot-reload.md
#pragma once

namespace gfx {
class Shader;
} // namespace gfx

namespace debug {

/// Draws the "Shaders" panel. Called by DebugUI::draw, inside the ImGui frame.
/// shader is not const: the "Reload shaders" button calls shader.reload().
void drawShadersPanel(gfx::Shader& shader);

} // namespace debug
