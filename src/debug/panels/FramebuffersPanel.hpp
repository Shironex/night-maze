// "Framebuffers" debug panel: exposure and tone mapping of the composite pass, the
// settings of the bloom, the fog, the vignette and the minimap, and previews of the scene
// framebuffer, of the bloom targets and of the minimap.
// See docs/modules/renderer/post-process.md
#pragma once

namespace game {
class PostProcess;
struct PostProcessSettings;
} // namespace game

namespace debug {

/// Draws the "Framebuffers" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// settings is editable, in two tabs. "Tone and bloom": the exposure, the tone mapping
/// curve, the range of the depth preview and the bloom (switch, threshold, intensity,
/// blur iterations). "Fog and vignette": the fog (switch, density, base height, height
/// falloff, colour) and the vignette (switch, strength, radius). The panel also sets
/// settings.previews: true while it is open, so the game draws the four preview
/// pictures only when somebody looks at them. postProcess is read only: the sizes and
/// the formats of the scene framebuffer and of the bloom targets, and the pictures.
void drawFramebuffersPanel(game::PostProcessSettings& settings,
                           const game::PostProcess& postProcess);

} // namespace debug
