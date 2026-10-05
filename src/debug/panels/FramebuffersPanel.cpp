// "Framebuffers" debug panel: exposure and tone mapping of the composite pass, and
// previews of the attachments of the scene framebuffer.
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

// Range of the slider of the depth preview, in metres: the distance shown as white.
constexpr float MIN_DEPTH_RANGE = 2.0F;
constexpr float MAX_DEPTH_RANGE = 100.0F;

// The two previews stand side by side.
constexpr float PREVIEW_COLUMNS = 2.0F;

// One preview: a caption and the picture under it, width pixels wide.
void drawPreview(const char* caption, const gfx::Framebuffer& preview, float width) {
    ImGui::BeginGroup();
    ImGui::TextUnformatted(caption);
    if (preview.isValid()) {
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
        // The first frame after the panel was opened: the pictures are drawn by the
        // game in its next frame.
        ImGui::TextUnformatted("(no picture yet)");
    }
    ImGui::EndGroup();
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
        // The two numbers of the composite pass (post/composite.frag). SliderFloat and
        // Combo write through the pointers they are given.
        ImGui::SliderFloat("Exposure", &settings.exposure, MIN_EXPOSURE, MAX_EXPOSURE, "%.2f",
                           ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
        ImGui::SetItemTooltip("The colours of the scene are multiplied by this number\n"
                              "before tone mapping. 1 changes nothing.");
        int toneMappingIndex = static_cast<int>(settings.toneMapping);
        if (ImGui::Combo("Tone mapping", &toneMappingIndex, TONE_MAPPING_ITEMS)) {
            settings.toneMapping = static_cast<game::ToneMapping>(toneMappingIndex);
        }
        ImGui::SetItemTooltip("How colours brighter than 1 are brought into the range of\n"
                              "the screen. The debug views (normals, UVs) are shown\n"
                              "without exposure and tone mapping.");

        // The framebuffer the scene is drawn into.
        const gfx::Framebuffer& scene = postProcess.sceneTarget();
        ImGui::Separator();
        ImGui::Text("Scene framebuffer: %d x %d px, %s + %s", scene.width(), scene.height(),
                    gfx::colorFormatName(scene.colorFormat()),
                    gfx::depthFormatName(scene.depthFormat()));
        ImGui::SliderFloat("Depth range", &settings.depthPreviewRange, MIN_DEPTH_RANGE,
                           MAX_DEPTH_RANGE, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("The depth preview shows the distance from the camera:\n"
                              "black at 0 m, white at this distance and beyond.");

        // The two attachments, side by side, sharing the width of the panel.
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float previewWidth = (ImGui::GetContentRegionAvail().x - spacing) / PREVIEW_COLUMNS;
        drawPreview("Colour (HDR, cut off at 1)",
                    postProcess.preview(game::AttachmentPreview::Color), previewWidth);
        ImGui::SameLine();
        drawPreview("Depth (as distance)", postProcess.preview(game::AttachmentPreview::Depth),
                    previewWidth);
    }
    ImGui::End();
}

} // namespace debug
