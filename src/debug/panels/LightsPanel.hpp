// "Lights" debug panel: ambient light, the moon, the flashlight, the point lights, the highlight.
// See docs/modules/scene/lights.md
#pragma once

namespace game {
struct LightingSettings;
struct Round;
} // namespace game

namespace debug {

/// Draws the "Lights" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// lighting is editable: the colours, intensities, angles and ranges of all lights and
/// the two numbers of the highlight. The game builds the lights of the next frame from
/// it, so every change is seen at once. round (the round in play) is read only: the
/// panel shows how many crystals still carry a point light, and says so when the
/// battery of the flashlight is empty.
void drawLightsPanel(game::LightingSettings& lighting, const game::Round& round);

} // namespace debug
