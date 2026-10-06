// "Environment" debug panel: how the crystals and the puddles show the sky.
// See docs/modules/renderer/env-mapping.md
#include "debug/panels/EnvironmentPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/EnvironmentMapping.hpp"

#include <imgui.h>

namespace debug {

namespace {

// A share goes from nothing to everything: the sky in the colour of a surface, the
// mirrored picture in what a crystal shows, the glow a crystal keeps.
constexpr float MIN_SHARE = 0.0F;
constexpr float MAX_SHARE = 1.0F;

// The part of the free cells that gets a puddle. 0 places none. The largest one is
// game::MAX_PUDDLE_SHARE.
constexpr float MIN_PUDDLE_SHARE = 0.0F;

} // namespace

void drawEnvironmentPanel(game::EnvironmentSettings& settings, std::size_t puddleCount) {
    // First run only: the fifth row of title bars at the top edge of the window, under
    // the Shadows panel and folded like it (the constant is in PanelLayout.hpp). Later
    // ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(ENVIRONMENT_PLACEMENT);
    if (ImGui::Begin("Environment")) {
        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Environment mapping", &settings.enabled);
        ImGui::SetItemTooltip("The crystals and the puddles show the sky: the cube map of\n"
                              "the skybox, read in the direction of the mirrored or of the\n"
                              "refracted ray. Only the sky: walls are never mirrored.\n"
                              "Off: the crystals are drawn like the walls, no puddles.");

        // These are uniforms of the reflect program: a change shows in the next frame.
        ImGui::SeparatorText("Crystals");
        ImGui::SliderFloat("Sky share", &settings.crystalStrength, MIN_SHARE, MAX_SHARE, "%.2f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("How much of the lit colour of a crystal is replaced by the\n"
                              "sky it shows. Its glow is added on top.");
        ImGui::SliderFloat("Refract / reflect", &settings.crystalReflectShare, MIN_SHARE, MAX_SHARE,
                           "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("0: the sky seen through the crystal (refraction).\n"
                              "1: the sky mirrored on it (reflection).");
        ImGui::SliderFloat("Refraction ratio", &settings.crystalRefractionRatio,
                           game::MIN_REFRACTION_RATIO, game::MAX_REFRACTION_RATIO, "%.2f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("n1 / n2 of Snell's law (eta of GLSL refract). Air into\n"
                              "glass: 1 / 1.5 = 0.67. Water: 0.75. Diamond: 0.41. 1 does\n"
                              "not bend the ray. Above 1 the ray would leave a denser\n"
                              "material: flat rays are then mirrored instead (total\n"
                              "internal reflection).");
        ImGui::SliderFloat("Glow", &settings.crystalGlowShare, MIN_SHARE, MAX_SHARE, "%.2f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("How much of its own glow a crystal keeps. The glow is far\n"
                              "brighter than the night sky: turn it down to see the sky on\n"
                              "a crystal plainly. The halo of the bloom fades with it.");

        ImGui::SeparatorText("Puddles");
        ImGui::Checkbox("Puddles", &settings.puddles);
        // SliderFloat returns true in every frame in which the value changed. The panel
        // only asks: the game places the puddles at the start of its next frame.
        if (ImGui::SliderFloat("Share of cells", &settings.puddleShare, MIN_PUDDLE_SHARE,
                               game::MAX_PUDDLE_SHARE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
            settings.replacePuddles = true;
        }
        ImGui::SetItemTooltip("The part of the free cells (not the start, not the exit,\n"
                              "no crystal) that gets a puddle. The cells come from the seed\n"
                              "of the maze. A larger share keeps the puddles and adds more.");
        ImGui::SliderFloat("Reflectivity", &settings.puddleReflectivity, MIN_SHARE, MAX_SHARE,
                           "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("How much of the sky a puddle shows when looked at straight\n"
                              "from above. Real water: 0.02.");
        ImGui::Checkbox("Fresnel", &settings.puddleFresnel);
        ImGui::SetItemTooltip("On: a puddle mirrors more the flatter it is looked at\n"
                              "(Schlick's formula), up to a full mirror far ahead.\n"
                              "Off: the reflectivity above at every angle.");

        ImGui::Separator();
        ImGui::Text("Puddles: %d", static_cast<int>(puddleCount));
    }
    ImGui::End();
}

} // namespace debug
