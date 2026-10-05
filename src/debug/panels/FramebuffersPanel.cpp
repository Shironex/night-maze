// "Framebuffers" debug panel: exposure and tone mapping of the composite pass, the
// settings of the bloom, the fog and the vignette, and previews of the scene framebuffer
// and of the bloom targets.
// See docs/modules/renderer/post-process.md
#include "debug/panels/FramebuffersPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/PostProcess.hpp"
#include "gfx/Framebuffer.hpp"

#include <imgui.h>

namespace debug {

namespace {

// The entries of the list, in the order of the enum game::ToneMapping: the number of the
// chosen entry is the value of the enum. ImGui wants the entries in one string, each
// ended by a zero character.
constexpr const char* TONE_MAPPING_ITEMS = "None (clamp)\0Reinhard\0ACES (fitted)\0";

// Range of the exposure slider. 1 leaves the scene as it is drawn. The slider is
// logarithmic, so halving and doubling the light are steps of the same length.
constexpr float MIN_EXPOSURE = 0.1F;
constexpr float MAX_EXPOSURE = 8.0F;

// Range of the bloom threshold slider, as a brightness in the HDR buffer. At 0 the
// whole picture takes part in the bloom. The upper end is above everything the scene
// draws with its default lights, so there the bloom finds nothing.
constexpr float MIN_BLOOM_THRESHOLD = 0.0F;
constexpr float MAX_BLOOM_THRESHOLD = 4.0F;

// Range of the bloom intensity slider. 0 adds no glow at all.
constexpr float MIN_BLOOM_INTENSITY = 0.0F;
constexpr float MAX_BLOOM_INTENSITY = 2.0F;

// Range of the slider of the depth preview, in metres: the distance shown as white.
constexpr float MIN_DEPTH_RANGE = 2.0F;
constexpr float MAX_DEPTH_RANGE = 100.0F;

// Range of the fog density slider, per metre. 0 is no fog. At the upper end half of
// a surface is gone after 1.4 m: the maze can hardly be seen.
constexpr float MIN_FOG_DENSITY = 0.0F;
constexpr float MAX_FOG_DENSITY = 0.5F;

// Range of the slider of the height up to which the fog has its full density, in metres
// of world height: from below the lowest ground to above the tops of the walls.
constexpr float MIN_FOG_BASE_HEIGHT = -2.0F;
constexpr float MAX_FOG_BASE_HEIGHT = 6.0F;

// Range of the slider of the height falloff of the fog, per metre. 0 gives the same
// density at every height. At the upper end the fog is a layer about a metre thick.
constexpr float MIN_FOG_HEIGHT_FALLOFF = 0.0F;
constexpr float MAX_FOG_HEIGHT_FALLOFF = 3.0F;

// Range of the vignette strength slider: the share of the light the corners lose.
constexpr float MIN_VIGNETTE_STRENGTH = 0.0F;
constexpr float MAX_VIGNETTE_STRENGTH = 1.0F;

// Range of the vignette radius slider, in texture coordinates from the middle of the
// screen. The upper end stays below the distance to a corner
// (game::VIGNETTE_CORNER_DISTANCE): the darkening needs some room to fade in.
constexpr float MIN_VIGNETTE_RADIUS = 0.0F;
constexpr float MAX_VIGNETTE_RADIUS = 0.65F;

// The widgets stand in a table of this many columns, so that the panel is short enough
// to show the pictures under them without scrolling.
constexpr int SETTING_COLUMNS = 2;

// The four previews stand side by side.
constexpr float PREVIEW_COLUMNS = 4.0F;

// One preview: a caption and the picture under it, width pixels wide. tooltip says what
// the picture shows. With drawn false the picture is not up to date (the pass that
// fills it did not run in this frame), and a note stands in its place.
void drawPreview(const char* caption, const char* tooltip, const gfx::Framebuffer& preview,
                 float width, bool drawn) {
    ImGui::BeginGroup();
    ImGui::TextUnformatted(caption);
    if (drawn && preview.isValid()) {
        // The picture keeps the shape of the framebuffer it shows.
        const float height =
            width * static_cast<float>(preview.height()) / static_cast<float>(preview.width());
        // ImGui identifies a texture by the id of the texture object. uv0 is the texture
        // coordinate of the top left corner of the picture and uv1 of the bottom right
        // one. A framebuffer texture has its row v = 0 at the BOTTOM, like everything
        // OpenGL draws, so the corners are (0, 1) and (1, 0): with the defaults the
        // picture would be upside down.
        const auto textureId = static_cast<ImTextureID>(preview.colorTextureId());
        ImGui::Image(textureId, {width, height}, {0.0F, 1.0F}, {1.0F, 0.0F});
    } else {
        // The first frame after the panel was opened (the pictures are drawn by the
        // game in its next frame), or a pass that is switched off.
        ImGui::TextUnformatted(drawn ? "(no picture yet)" : "(not drawn)");
    }
    ImGui::EndGroup();
    ImGui::SetItemTooltip("%s", tooltip);
}

// The widgets of the first tab, in a table of SETTING_COLUMNS columns: the two numbers of
// the composite pass (post/composite.frag), the switch and the three numbers of the
// bloom (game::BloomSettings) and the range of the depth preview. Every widget writes
// through the pointer it is given.
void drawToneAndBloomSettings(game::PostProcessSettings& settings) {
    // BeginTable returns false when no part of the table can be seen (it is scrolled out
    // of the panel). Nothing is drawn then, and EndTable must not be called.
    if (!ImGui::BeginTable("tone and bloom", SETTING_COLUMNS)) {
        return;
    }

    // TableNextColumn moves on to the next cell, and from the last cell of a row to the
    // first cell of a new row. So the widgets fill the table row by row.
    ImGui::TableNextColumn();
    ImGui::SliderFloat("Exposure", &settings.exposure, MIN_EXPOSURE, MAX_EXPOSURE, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
    ImGui::SetItemTooltip("The colours of the scene are multiplied by this number\n"
                          "before tone mapping. 1 changes nothing.");
    ImGui::TableNextColumn();
    int toneMappingIndex = static_cast<int>(settings.toneMapping);
    if (ImGui::Combo("Tone mapping", &toneMappingIndex, TONE_MAPPING_ITEMS)) {
        settings.toneMapping = static_cast<game::ToneMapping>(toneMappingIndex);
    }
    ImGui::SetItemTooltip("How colours brighter than 1 are brought into the range of\n"
                          "the screen. The debug views (normals, UVs) are shown\n"
                          "without exposure, tone mapping, bloom, fog and vignette.");

    game::BloomSettings& bloom = settings.bloom;
    ImGui::TableNextColumn();
    ImGui::Checkbox("Bloom", &bloom.enabled);
    ImGui::SetItemTooltip("Bright parts of the scene glow: they are copied into a smaller\n"
                          "picture, blurred and added back. The debug views (normals,\n"
                          "UVs) are shown without it.");
    ImGui::TableNextColumn();
    ImGui::SliderInt("Blur iterations", &bloom.blurIterations, game::MIN_BLOOM_BLUR_ITERATIONS,
                     game::MAX_BLOOM_BLUR_ITERATIONS, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("How many times the Gaussian blur of the bloom runs (a horizontal\n"
                          "and a vertical pass each time). More makes the glow wider.");

    ImGui::TableNextColumn();
    ImGui::SliderFloat("Threshold", &bloom.threshold, MIN_BLOOM_THRESHOLD, MAX_BLOOM_THRESHOLD,
                       "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Bloom: only light brighter than this glows. 1 is the white of\n"
                          "the screen before exposure. The bright pass preview shows\n"
                          "what is left.");
    ImGui::TableNextColumn();
    ImGui::SliderFloat("Intensity", &bloom.intensity, MIN_BLOOM_INTENSITY, MAX_BLOOM_INTENSITY,
                       "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Bloom: the blurred glow is multiplied by this number before\n"
                          "it is added to the scene.");

    ImGui::TableNextColumn();
    ImGui::SliderFloat("Depth range", &settings.depthPreviewRange, MIN_DEPTH_RANGE, MAX_DEPTH_RANGE,
                       "%.0f m", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("The depth preview shows the distance from the camera:\n"
                          "black at 0 m, white at this distance and beyond.");

    ImGui::EndTable();
}

// The widgets of the second tab, in a table like the first one: the switch, the three
// numbers and the colour of the fog (game::FogSettings), and the switch and the two
// numbers of the vignette (game::VignetteSettings).
void drawFogAndVignetteSettings(game::PostProcessSettings& settings) {
    if (!ImGui::BeginTable("fog and vignette", SETTING_COLUMNS)) {
        return;
    }

    game::FogSettings& fog = settings.fog;
    ImGui::TableNextColumn();
    ImGui::Checkbox("Fog", &fog.enabled);
    ImGui::SetItemTooltip("Far and low surfaces fade into the fog colour. Computed in\n"
                          "the composite pass from the depth of the scene. The debug\n"
                          "views (normals, UVs) are shown without it.");
    ImGui::TableNextColumn();
    ImGui::SliderFloat("Density", &fog.density, MIN_FOG_DENSITY, MAX_FOG_DENSITY, "%.3f /m",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Fog: amount = 1 - exp(-density * height factor * distance).\n"
                          "At 0.1, half of a surface on the ground is gone after 6.9 m.");

    ImGui::TableNextColumn();
    ImGui::SliderFloat("Base height", &fog.baseHeight, MIN_FOG_BASE_HEIGHT, MAX_FOG_BASE_HEIGHT,
                       "%.2f m", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Fog: up to this world height the fog has its full density.\n"
                          "The ground of the maze lies between 0 and 0.5 m.");
    ImGui::TableNextColumn();
    ImGui::SliderFloat("Height falloff", &fog.heightFalloff, MIN_FOG_HEIGHT_FALLOFF,
                       MAX_FOG_HEIGHT_FALLOFF, "%.2f /m", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Fog: how fast it thins out above the base height:\n"
                          "height factor = exp(-falloff * metres above the base).\n"
                          "0 gives the same fog at every height, the sky included.");

    ImGui::TableNextColumn();
    // &fog.color.x is the address of the first of the three floats of the vector, which
    // lie next to each other: the array of three floats ImGui asks for.
    ImGui::ColorEdit3("Fog colour", &fog.color.x);
    ImGui::SetItemTooltip("The colour surfaces fade into, as an sRGB value. It is mixed\n"
                          "in before exposure and tone mapping, so on the screen the\n"
                          "fog is darker than this swatch.");
    game::VignetteSettings& vignette = settings.vignette;
    ImGui::TableNextColumn();
    ImGui::Checkbox("Vignette", &vignette.enabled);
    ImGui::SetItemTooltip("The corners of the finished picture are darkened, after tone\n"
                          "mapping. The debug views (normals, UVs) are shown without it.");

    ImGui::TableNextColumn();
    ImGui::SliderFloat("Strength", &vignette.strength, MIN_VIGNETTE_STRENGTH, MAX_VIGNETTE_STRENGTH,
                       "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Vignette: the share of the light the corners lose.\n"
                          "0 changes nothing, 1 makes them black.");
    ImGui::TableNextColumn();
    ImGui::SliderFloat("Radius", &vignette.radius, MIN_VIGNETTE_RADIUS, MAX_VIGNETTE_RADIUS, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Vignette: the distance from the middle of the screen at which\n"
                          "the darkening starts. 0.5 is the middle of an edge, 0.71\n"
                          "a corner. Not corrected for the shape of the window.");

    ImGui::EndTable();
}

// The settings, in two tabs so that the panel stays short enough to show the pictures
// under them without scrolling: both tabs have four rows of widgets.
void drawSettings(game::PostProcessSettings& settings) {
    // BeginTabBar returns false when the bar cannot be seen. EndTabBar must not be
    // called then. BeginTabItem returns true for the tab that is selected, and only
    // that one draws its widgets.
    if (!ImGui::BeginTabBar("settings")) {
        return;
    }
    if (ImGui::BeginTabItem("Tone and bloom")) {
        drawToneAndBloomSettings(settings);
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Fog and vignette")) {
        drawFogAndVignetteSettings(settings);
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
}

} // namespace

void drawFramebuffersPanel(game::PostProcessSettings& settings,
                           const game::PostProcess& postProcess) {
    // First run only: the third row of title bars at the top edge of the window, folded
    // (the constant is in PanelLayout.hpp). Later ImGui remembers the panel in
    // imgui.ini.
    placePanelOnFirstUse(FRAMEBUFFERS_PLACEMENT);
    // Begin returns false when the panel is folded. The previews are asked for only
    // while it is open: the game reads the flag in its next frame.
    const bool open = ImGui::Begin("Framebuffers");
    settings.previews = open;
    if (open) {
        drawSettings(settings);

        // The framebuffer the scene is drawn into, and the smaller targets of the bloom.
        const gfx::Framebuffer& scene = postProcess.sceneTarget();
        const gfx::Framebuffer& bloom = postProcess.bloomTarget();
        const bool bloomDrawn = postProcess.bloomDrawn();
        ImGui::Separator();
        ImGui::Text("Scene framebuffer: %d x %d px, %s + %s", scene.width(), scene.height(),
                    gfx::colorFormatName(scene.colorFormat()),
                    gfx::depthFormatName(scene.depthFormat()));
        if (bloomDrawn) {
            ImGui::Text("Bloom targets (3): %d x %d px, %s", bloom.width(), bloom.height(),
                        gfx::colorFormatName(bloom.colorFormat()));
        } else {
            ImGui::TextUnformatted("Bloom targets: not drawn (bloom off or a debug view)");
        }

        // The four pictures, side by side, sharing the width of the panel: the two
        // attachments of the scene framebuffer, then the two steps of the bloom.
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float previewWidth =
            (ImGui::GetContentRegionAvail().x - (PREVIEW_COLUMNS - 1.0F) * spacing) /
            PREVIEW_COLUMNS;
        drawPreview("HDR colour", "The colour attachment of the scene, cut off at 1.",
                    postProcess.preview(game::AttachmentPreview::Color), previewWidth, true);
        ImGui::SameLine();
        drawPreview("Depth", "The depth attachment of the scene, as a distance.",
                    postProcess.preview(game::AttachmentPreview::Depth), previewWidth, true);
        ImGui::SameLine();
        drawPreview("Bright pass", "What the scene has above the bloom threshold.",
                    postProcess.brightPassPreview(), previewWidth, bloomDrawn);
        ImGui::SameLine();
        drawPreview("Bloom", "The bright pass after the blur, before the intensity.",
                    postProcess.bloomPreview(), previewWidth, bloomDrawn);
    }
    ImGui::End();
}

} // namespace debug
