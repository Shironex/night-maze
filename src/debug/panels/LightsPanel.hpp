// "Lights" debug panel: ambient light, the moon, the flashlight, the point lights, the highlight.
// See docs/modules/scene/lights.md
#pragma once

namespace game {
struct LightingSettings;
struct MazeWorld;
} // namespace game

namespace debug {

/// Draws the "Lights" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// lighting is editable: the colours, intensities, angles and ranges of all lights and
/// the two numbers of the highlight. The game builds the lights of the next frame from
/// it, so every change is seen at once. world (the maze in play) is read only: the
/// panel shows how many point lights it has.
void drawLightsPanel(game::LightingSettings& lighting, const game::MazeWorld& world);

} // namespace debug
