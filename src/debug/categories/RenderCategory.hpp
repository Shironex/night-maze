// "Render" category of the debug window: lighting mode, sky, clear colour, the view of
// the textured shader and the texture filtering.
#pragma once

namespace debug {

struct DebugContext;
class Page;

/// Draws the cards of the "Render" category onto the page. Called by the debug window,
/// inside the ImGui frame.
///
/// It edits, through the context: the lighting mode, the switch and the brightness of
/// the sky, the clear colour, the view mode, the normal mapping switch and the filter
/// and the anisotropy level of all textures.
void drawRenderCategory(Page& page, const DebugContext& context);

} // namespace debug
