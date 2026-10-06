// "Light" category of the debug window: ambient light, the moon, the flashlight, the
// lights of the crystals and the highlight, and the shadows of the moon and of the
// flashlight with a picture of each shadow map.
// See docs/modules/scene/lights.md and docs/modules/renderer/shadows.md
#pragma once

namespace debug {

struct DebugContext;
class Page;

/// The tabs of the "Light" category, in the order of CategoryInfo::tabs.
enum class LightTab {
    Lights = 0,
    Shadows,
};

/// Draws the cards of one tab of the "Light" category onto the page, or of both while
/// the page is searching. Called by the debug window, inside the ImGui frame.
///
/// It edits, through the context: the settings of every light (game::LightingSettings)
/// and the shadow settings of the moon and of the flashlight (game::ShadowSettings).
///
/// It also sets ShadowSettings::preview of a light, but only while the card with the
/// picture of its shadow map is drawn, so the game draws that picture only when
/// somebody looks at it (DebugUI::draw clears the flags before every frame).
void drawLightCategory(Page& page, const DebugContext& context, LightTab tab);

} // namespace debug
