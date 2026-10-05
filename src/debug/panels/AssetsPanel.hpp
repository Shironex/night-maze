// "Assets" debug panel: loaded models and textures, texture filtering and the view mode.
// See docs/modules/assets/asset-cache.md
#pragma once

namespace assets {
class AssetCache;
} // namespace assets

namespace game {
enum class ViewMode;
} // namespace game

namespace debug {

/// Draws the "Assets" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// assets is not const: the filter list and the anisotropy slider call setFilter and
/// setAnisotropy, which change every texture. Its models and textures are only listed.
/// viewMode is editable: what the textured shader shows (picture, normals or UVs).
void drawAssetsPanel(assets::AssetCache& assets, game::ViewMode& viewMode);

} // namespace debug
