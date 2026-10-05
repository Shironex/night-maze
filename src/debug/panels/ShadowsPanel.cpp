// "Shadows" debug panel: the settings of the shadow map of the moon and a picture of it.
// See docs/modules/renderer/shadows.md
#include "debug/panels/ShadowsPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/ShadowMap.hpp"
#include "game/Shadows.hpp"
#include "gfx/Framebuffer.hpp"
#include "scene/LightSpace.hpp"

#include <imgui.h>

#include <algorithm>

namespace debug {

namespace {

// The entries of the resolution list, in the order of the enum game::ShadowResolution:
// the number of the chosen entry is the value of the enum. ImGui wants the entries in
// one string, each ended by a zero character.
constexpr const char* RESOLUTION_ITEMS = "1024 x 1024\0"
                                         "2048 x 2048\0";

// The entries of the kernel list: entry number i is the radius i + game::MIN_PCF_RADIUS.
constexpr const char* PCF_KERNEL_ITEMS = "3 x 3\0"
                                         "5 x 5\0"
                                         "7 x 7\0";

// Range of the two bias sliders, in metres. At 0 the surfaces shade themselves (shadow
// acne): in stripes with both filters off, as an overall darkening with them on. The
// upper ends are far more than the 0.2 m of a wall, enough to see a shadow come loose
// from the wall that casts it (peter panning).
constexpr float MIN_BIAS = 0.0F;
constexpr float MAX_CONSTANT_BIAS = 0.5F;
constexpr float MAX_SLOPE_BIAS = 1.0F;

// Range of the strength slider: the share of the light a shadow takes away.
constexpr float MIN_STRENGTH = 0.0F;
constexpr float MAX_STRENGTH = 1.0F;

// Width of the list of kernel sizes in pixels at 100 % display scaling: enough for its
// longest entry and the arrow.
constexpr float KERNEL_LIST_WIDTH = 90.0F;

// The panel has two columns: the widgets on the left, the picture on the right.
constexpr int PANEL_COLUMNS = 2;

// Centimetres in a metre, for the size of a texel.
constexpr float CENTIMETRES_PER_METRE = 100.0F;

// The widgets of one shadow map. Every widget writes through the pointer it is given.
void drawSettings(game::ShadowSettings& settings) {
    ImGui::Checkbox("Shadows", &settings.enabled);
    ImGui::SetItemTooltip("Off: the shadow map is not drawn and nothing is in shadow.\n"
                          "The picture is then the one of the game without shadows.");

    int resolutionIndex = static_cast<int>(settings.resolution);
    if (ImGui::Combo("Resolution", &resolutionIndex, RESOLUTION_ITEMS)) {
        settings.resolution = static_cast<game::ShadowResolution>(resolutionIndex);
    }
    ImGui::SetItemTooltip("The size of the shadow map in texels. The map covers the same\n"
                          "ground at every size, so a smaller map has larger texels:\n"
                          "coarser shadow edges, and more bias is needed.");

    ImGui::SliderFloat("Constant bias", &settings.constantBias, MIN_BIAS, MAX_CONSTANT_BIAS,
                       "%.3f m", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Every surface is compared with the shadow map as if it were\n"
                          "this much nearer to the light. With both parts at 0 the\n"
                          "surfaces shade themselves (shadow acne): stripes with both\n"
                          "filters off, an overall darkening with them on.");
    ImGui::SliderFloat("Slope bias", &settings.slopeBias, MIN_BIAS, MAX_SLOPE_BIAS, "%.3f m",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Added on top for surfaces the light only grazes:\n"
                          "bias = constant + slope * (1 - cos of the angle between the\n"
                          "normal and the light). Too much bias lets a shadow come\n"
                          "loose from the wall that casts it (peter panning).");

    ImGui::Checkbox("Hardware 2 x 2 filter", &settings.hardwareFilter);
    ImGui::SetItemTooltip("The graphics card compares the four texels around a place\n"
                          "and blends the four answers (a linear comparison sampler).\n"
                          "Off: one texel, and the edges of the shadows show steps.");

    ImGui::Checkbox("PCF", &settings.pcf);
    ImGui::SetItemTooltip("Percentage closer filtering: the comparison is made for\n"
                          "a square of texels and the answers are averaged, which\n"
                          "makes the edge of a shadow soft.");
    // The list of kernel sizes stands on the same line as the switch, to keep the panel
    // short. It shows sizes and the settings hold a radius: entry 0 is radius 1.
    ImGui::SameLine();
    ImGui::SetNextItemWidth(KERNEL_LIST_WIDTH * ImGui::GetStyle().FontScaleDpi);
    int kernelIndex = std::clamp(settings.pcfRadius, game::MIN_PCF_RADIUS, game::MAX_PCF_RADIUS) -
                      game::MIN_PCF_RADIUS;
    if (ImGui::Combo("Kernel", &kernelIndex, PCF_KERNEL_ITEMS)) {
        settings.pcfRadius = kernelIndex + game::MIN_PCF_RADIUS;
    }
    ImGui::SetItemTooltip("How many texels the PCF square has. A larger kernel is\n"
                          "softer and costs more lookups per pixel: 9, 25 or 49.");

    ImGui::SliderFloat("Strength", &settings.strength, MIN_STRENGTH, MAX_STRENGTH, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("The share of the light of this lamp a shadow takes away.\n"
                          "1: none of it is left in a shadow. The ambient light and\n"
                          "the other lights are never darkened.");
}

// What the map is and how much ground it covers.
void drawFacts(const game::ShadowSettings& settings, const game::ShadowMap& map,
               const scene::LightSpace& lightSpace) {
    const gfx::Framebuffer& target = map.target();
    if (settings.enabled && target.isValid()) {
        ImGui::Text("Map: %d x %d, %s", target.width(), target.height(),
                    gfx::depthFormatName(target.depthFormat()));
    } else {
        ImGui::TextUnformatted("Map: not drawn");
    }
    ImGui::Text("Covers %.1f x %.1f m, %.1f m deep", lightSpace.extent.x, lightSpace.extent.y,
                lightSpace.extent.z);
    // The size of a texel at the resolution that is chosen, also while the map is off.
    const float texelSize =
        game::shadowTexelSize(lightSpace, game::shadowMapSize(settings.resolution));
    ImGui::Text("One texel: %.1f cm", texelSize * CENTIMETRES_PER_METRE);
}

// The picture of the map: a square as wide as the room that is left, with a caption
// above it. With drawn false the picture is not up to date, and a note stands in its
// place.
void drawPicture(const char* caption, const char* tooltip, const game::ShadowMap& map, bool drawn) {
    ImGui::BeginGroup();
    ImGui::TextUnformatted(caption);
    const gfx::Framebuffer& preview = map.preview();
    if (drawn && preview.isValid()) {
        // As large as the column is wide or the panel is high, whichever is smaller.
        const ImVec2 room = ImGui::GetContentRegionAvail();
        const float side = std::min(room.x, room.y);
        // ImGui identifies a texture by the id of the texture object. A framebuffer
        // texture has its row v = 0 at the BOTTOM, so the corners are (0, 1) and (1, 0)
        // (see the Framebuffers panel). The picture is a plain GL_RGBA8 texture: the
        // depth texture itself is never handed to ImGui.
        const auto textureId = static_cast<ImTextureID>(preview.colorTextureId());
        ImGui::Image(textureId, {side, side}, {0.0F, 1.0F}, {1.0F, 0.0F});
    } else {
        // The first frame after the panel was opened (the picture is drawn by the game
        // in its next frame), or the shadows are switched off.
        ImGui::TextUnformatted(drawn ? "(no picture yet)" : "(not drawn)");
    }
    ImGui::EndGroup();
    ImGui::SetItemTooltip("%s", tooltip);
}

// One tab: the widgets and the facts of a shadow map on the left, its picture on the
// right. A second light with a shadow map is one more call of this function.
void drawShadowMapTab(const char* pictureCaption, const char* pictureTooltip,
                      game::ShadowSettings& settings, const game::ShadowMap& map,
                      const scene::LightSpace& lightSpace) {
    // BeginTable returns false when no part of the table can be seen. Nothing is drawn
    // then, and EndTable must not be called.
    if (!ImGui::BeginTable("shadow map", PANEL_COLUMNS)) {
        return;
    }
    ImGui::TableNextColumn();
    drawSettings(settings);
    ImGui::Separator();
    drawFacts(settings, map, lightSpace);

    ImGui::TableNextColumn();
    drawPicture(pictureCaption, pictureTooltip, map, settings.enabled);
    ImGui::EndTable();
}

} // namespace

void drawShadowsPanel(game::ShadowSettings& moon, const game::ShadowMap& moonMap,
                      const scene::LightSpace& moonLightSpace) {
    // First run only: the fourth row of title bars at the top edge of the window, folded
    // (the constant is in PanelLayout.hpp). Later ImGui remembers the panel in
    // imgui.ini.
    placePanelOnFirstUse(SHADOWS_PLACEMENT);
    // Begin returns false when the panel is folded. The preview is asked for only while
    // it is open: the game reads the flag in its next frame.
    const bool open = ImGui::Begin("Shadows");
    moon.preview = open;
    // BeginTabBar returns false when the bar cannot be seen. EndTabBar must not be
    // called then. BeginTabItem returns true for the tab that is selected.
    if (open && ImGui::BeginTabBar("lights")) {
        if (ImGui::BeginTabItem("Moon")) {
            drawShadowMapTab("Depth seen from the moon",
                             "The shadow map: black is near the moon, white is far\n"
                             "from it or empty. The walls are the dark lines.",
                             moon, moonMap, moonLightSpace);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

} // namespace debug
