// "Post process" category of the debug window: exposure and tone mapping of the composite
// pass, the bloom, the fog, the vignette, and previews of the scene framebuffer and of
// the bloom targets.
// See docs/modules/renderer/post-process.md
#pragma once

namespace debug {

struct DebugContext;
class Page;

/// Draws the cards of the "Post process" category onto the page. Called by the debug
/// window, inside the ImGui frame.
///
/// It edits, through the context: the exposure, the tone mapping curve, the bloom
/// (switch, blur iterations, threshold, intensity), the fog (switch, density, base
/// height, height falloff, colour), the vignette (switch, strength, radius) and the
/// range of the depth preview.
///
/// It also sets PostProcessSettings::previews, but only while the card with the four
/// preview pictures is drawn, so the game draws the pictures only when somebody looks
/// at them (DebugUI::draw clears the flag before every frame).
void drawPostProcessCategory(Page& page, const DebugContext& context);

} // namespace debug
