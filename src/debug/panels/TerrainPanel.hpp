// "Terrain" debug panel: the height scale and the wireframe switch of the terrain, and
// the size of its grid.
// See docs/modules/renderer/terrain.md
#pragma once

namespace game {
class Terrain;
struct TerrainSettings;
} // namespace game

namespace debug {

/// Draws the "Terrain" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// settings is editable: the height scale (a change sets settings.rebuild, and the game
/// builds the terrain again at the start of its next frame) and the wireframe switch.
/// terrain is read only: the panel shows the size of its grid, the number of its
/// triangles and its lowest and highest point.
void drawTerrainPanel(game::TerrainSettings& settings, const game::Terrain& terrain);

} // namespace debug
