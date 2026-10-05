// "Framebuffers" debug panel: exposure and tone mapping of the composite pass, and
// previews of the attachments of the scene framebuffer.
// See docs/modules/renderer/post-process.md
#pragma once

namespace game {
class PostProcess;
struct PostProcessSettings;
} // namespace game

namespace debug {

/// Draws the "Framebuffers" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// settings is editable: the exposure, the tone mapping curve and the range of the
/// depth preview. The panel also sets settings.previews: true while it is open, so the
/// game draws the two preview pictures only when somebody looks at them. postProcess
/// is read only: the size and the formats of the scene framebuffer, and the pictures.
void drawFramebuffersPanel(game::PostProcessSettings& settings,
                           const game::PostProcess& postProcess);

} // namespace debug
