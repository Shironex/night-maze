// "Diagnostics" category of the debug window: frame statistics and driver info, the
// shader programs with a reload button and their errors, the collision boxes and the
// picking ray, and the loaded models and textures.
// See docs/modules/gfx/shader-hot-reload.md, docs/modules/scene/collision.md and
// docs/modules/assets/asset-cache.md
#pragma once

namespace debug {

struct DebugContext;
class Page;
class RawTextureSampler;

/// The tabs of the "Diagnostics" category, in the order of CategoryInfo::tabs.
enum class DiagnosticsTab {
    FrameAndShaders = 0,
    CollisionAndPicking,
    Assets,
};

/// Draws the cards of one tab of the "Diagnostics" category onto the page, or of all
/// three while the page is searching. Called by the debug window, inside the ImGui
/// frame.
///
/// Almost everything here is only shown. It edits, through the context: the switch that
/// draws the collision shapes and the two switches of the debug view of the picking. Its
/// one button reloads every shader program.
///
/// rawSampler is what the previews of the sRGB textures are read with, so that they
/// show the pictures as they are in their files (debug::RawTextureSampler).
void drawDiagnosticsCategory(Page& page, const DebugContext& context,
                             const RawTextureSampler& rawSampler, DiagnosticsTab tab);

} // namespace debug
