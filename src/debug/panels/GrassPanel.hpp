// "Grass" debug panel: the switch, the density, the blade height and the wind of the grass.
// See docs/modules/renderer/grass-geometry.md
#pragma once

#include <cstddef>

namespace game {
struct GrassSettings;
} // namespace game

namespace debug {

/// Draws the "Grass" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// settings is editable: the switch, the density (a change sets settings.replant, and
/// the game places the tufts again at the start of its next frame), the height of the
/// blades and the strength of the wind. tuftCount is the number of tufts that are drawn:
/// the panel only shows it.
void drawGrassPanel(game::GrassSettings& settings, std::size_t tuftCount);

} // namespace debug
