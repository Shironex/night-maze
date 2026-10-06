// "Assets" debug panel: loaded models and textures.
// See docs/modules/assets/asset-cache.md
#pragma once

namespace assets {
class AssetCache;
} // namespace assets

namespace debug {

class RawTextureSampler;

/// Draws the "Assets" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// The models and textures of assets are only listed: the settings of the textures are
/// in the Render category of the debug window.
/// rawSampler is what the previews of the sRGB textures are read with, so that they show
/// the pictures as they are in their files (debug::RawTextureSampler).
void drawAssetsPanel(const assets::AssetCache& assets, const RawTextureSampler& rawSampler);

} // namespace debug
