// "Shadows" debug panel: the settings of the shadow map of the moon and a picture of it.
// See docs/modules/renderer/shadows.md
#pragma once

namespace game {
class ShadowMap;
struct ShadowSettings;
} // namespace game

namespace scene {
struct LightSpace;
} // namespace scene

namespace debug {

/// Draws the "Shadows" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// The panel has one tab per light that casts shadows: "Moon" today. moon is editable:
/// the switch of the shadows, the resolution of the map, the two parts of the bias, the
/// hardware filter, the PCF kernel and the strength. The panel also sets moon.preview:
/// true while it is open, so the game draws the preview picture only when somebody
/// looks at it. moonMap and moonLightSpace are read only: the size and the format of
/// the map, its picture, and how much ground it covers.
void drawShadowsPanel(game::ShadowSettings& moon, const game::ShadowMap& moonMap,
                      const scene::LightSpace& moonLightSpace);

} // namespace debug
