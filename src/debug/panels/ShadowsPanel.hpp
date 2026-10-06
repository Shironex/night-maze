// "Shadows" debug panel: the settings of the two shadow maps and a picture of each.
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

/// What the panel needs of one light that casts shadows: one tab shows it. A plain
/// struct of references, built for one call like debug::DebugContext.
struct ShadowMapView {
    /// Editable: the switch of the shadows, the resolution of the map, the two parts of
    /// the bias, the hardware filter, the PCF kernel and the strength. The panel also
    /// sets settings.preview: true while the tab of this light is shown, so the game
    /// draws the preview picture only when somebody looks at it.
    game::ShadowSettings& settings;
    /// Read only: the size and the format of the map, and its picture.
    const game::ShadowMap& map;
    /// Read only: how much the map covers and how large its texels are.
    const scene::LightSpace& lightSpace;
    /// Whether the map is drawn in the frames the game draws now. False: the panel
    /// shows a note in place of the picture, which would be an old one.
    bool drawn;
};

/// Draws the "Shadows" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// The panel has one tab per light that casts shadows: "Moon" and "Flashlight". Both
/// tabs have the same widgets. What differs is what they say about the map: the box of
/// the moon covers the same ground everywhere, the pyramid of the flashlight covers
/// more the farther from the light it is.
void drawShadowsPanel(const ShadowMapView& moon, const ShadowMapView& flashlight);

} // namespace debug
