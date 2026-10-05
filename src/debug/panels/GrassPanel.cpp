// "Grass" debug panel: the switch, the density, the blade height and the wind of the grass.
// See docs/modules/renderer/grass-geometry.md
#include "debug/panels/GrassPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/Grass.hpp"

#include <imgui.h>

namespace debug {

namespace {

// The density, in tufts per metre of wall and side. 0 plants nothing. The largest one is
// game::MAX_GRASS_DENSITY.
constexpr float MIN_DENSITY = 0.0F;

// The height of the tallest blades, in metres: from stubble to knee high.
constexpr float MIN_BLADE_HEIGHT = 0.05F;
constexpr float MAX_BLADE_HEIGHT = 0.8F;

// The strength of the wind: 0 is still air, 1 the default breeze.
constexpr float MIN_WIND_STRENGTH = 0.0F;
constexpr float MAX_WIND_STRENGTH = 3.0F;

// The blades one tuft is made of: BLADE_COUNT in assets/shaders/grass.geom. The panel
// only uses it to show a number, so a wrong value here changes nothing that is drawn.
constexpr int BLADES_PER_TUFT = 3;

} // namespace

void drawGrassPanel(game::GrassSettings& settings, std::size_t tuftCount) {
    // First run only: the second row of title bars at the top edge of the window, under
    // the Gameplay panel and folded like it (the constant is in PanelLayout.hpp). Later
    // ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(GRASS_PLACEMENT);
    if (ImGui::Begin("Grass")) {
        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Enabled", &settings.enabled);

        // SliderFloat returns true in every frame in which the value changed. The panel
        // only asks: the game places the tufts at the start of its next frame.
        if (ImGui::SliderFloat("Density", &settings.density, MIN_DENSITY, game::MAX_GRASS_DENSITY,
                               "%.1f per m", ImGuiSliderFlags_AlwaysClamp)) {
            settings.replant = true;
        }
        ImGui::SetItemTooltip("Tufts per metre of wall, on each side of the wall. The\n"
                              "scatter on the hills follows in proportion.");

        // These two are uniforms of the grass program: they change the blades the
        // geometry shader builds, and no tuft has to be placed again.
        ImGui::SliderFloat("Blade height", &settings.bladeHeight, MIN_BLADE_HEIGHT,
                           MAX_BLADE_HEIGHT, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Wind strength", &settings.windStrength, MIN_WIND_STRENGTH,
                           MAX_WIND_STRENGTH, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Separator();
        // One tuft is one point in the vertex buffer. The blades are made of it by the
        // geometry shader, so their number is not stored anywhere.
        const int tufts = static_cast<int>(tuftCount);
        ImGui::Text("Tufts: %d (%d blades)", tufts, tufts * BLADES_PER_TUFT);
    }
    ImGui::End();
}

} // namespace debug
