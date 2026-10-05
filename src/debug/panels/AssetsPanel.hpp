// "Assets" debug panel: loaded models and textures, texture filtering, the view mode and
// the normal mapping switch.
// See docs/modules/assets/asset-cache.md
#pragma once

namespace assets {
class AssetCache;
} // namespace assets

namespace game {
enum class ViewMode;
} // namespace game

namespace debug {

class RawTextureSampler;

/// Draws the "Assets" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// assets is not const: the filter list and the anisotropy slider call setFilter and
/// setAnisotropy, which change every texture. Its models and textures are only listed.
/// viewMode is editable: what the textured shader shows (picture, normals or UVs).
/// normalMapping is editable: whether the lit shader reads its normals from the normal
/// maps (game::LightingSettings::normalMapping).
/// rawSampler is what the previews of the sRGB textures are read with, so that they show
/// the pictures as they are in their files (debug::RawTextureSampler).
void drawAssetsPanel(assets::AssetCache& assets, game::ViewMode& viewMode, bool& normalMapping,
                     const RawTextureSampler& rawSampler);

} // namespace debug
