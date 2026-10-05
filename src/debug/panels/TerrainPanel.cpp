// "Terrain" debug panel: the height scale and the wireframe switch of the terrain, and
// the size of its grid.
// See docs/modules/renderer/terrain.md
#include "debug/panels/TerrainPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/Terrain.hpp"

#include <imgui.h>

namespace debug {

namespace {

// The smallest height scale of the slider: a flat world. The largest one is
// game::MAX_HEIGHT_SCALE.
constexpr float MIN_HEIGHT_SCALE = 0.0F;

} // namespace

void drawTerrainPanel(game::TerrainSettings& settings, const game::Terrain& terrain) {
    // First run only: the second row of title bars at the top edge of the window, under
    // the Camera panel and folded like it (the constant is in PanelLayout.hpp). Later
    // ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(TERRAIN_PLACEMENT);
    if (ImGui::Begin("Terrain")) {
        // SliderFloat returns true in every frame in which the value changed, so the
        // terrain follows the slider while it is dragged. The panel only asks: the game
        // builds the terrain at the start of its next frame.
        if (ImGui::SliderFloat("Height scale", &settings.heightScale, MIN_HEIGHT_SCALE,
                               game::MAX_HEIGHT_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
            settings.rebuild = true;
        }
        ImGui::SetItemTooltip("Every height of the terrain is multiplied by this number.\n"
                              "0 is a flat world. The walls, the gate, the crystals and\n"
                              "the player are put on the new ground at once.");

        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Wireframe", &settings.wireframe);
        ImGui::SetItemTooltip("Draws the edges of the triangles of the terrain instead of\n"
                              "their faces (glPolygonMode). Everything else stays filled.");

        ImGui::Separator();
        ImGui::Text("Grid: %d x %d points, %.2f m apart", terrain.columns(), terrain.rows(),
                    terrain.spacing());
        ImGui::Text("Triangles: %d", static_cast<int>(terrain.triangleCount()));
        ImGui::Text("Height: %.2f m to %.2f m", terrain.minHeight(), terrain.maxHeight());
    }
    ImGui::End();
}

} // namespace debug
