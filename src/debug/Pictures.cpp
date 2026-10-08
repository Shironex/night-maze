// Pictures of the debug window: a framebuffer shown as an image with a caption.
#include "debug/Pictures.hpp"

#include "debug/Theme.hpp"
#include "debug/Widgets.hpp"
#include "gfx/Framebuffer.hpp"

#include <imgui.h>

namespace debug {

namespace {

// Radius of the corners of the outline of a picture, in pixels at 100 % display scaling.
constexpr float PICTURE_ROUNDING = 4.0F;

} // namespace

void drawFramebufferPicture(const char* caption, const char* tooltip,
                            const gfx::Framebuffer& picture, float width, bool drawn) {
    // A group makes the caption and the picture one widget, so one tooltip covers both.
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM_COLOR);
    ImGui::TextUnformatted(caption);
    ImGui::PopStyleColor();

    if (drawn && picture.isValid()) {
        // The picture keeps the shape of the framebuffer it shows.
        const float height =
            width * static_cast<float>(picture.height()) / static_cast<float>(picture.width());
        // ImGui identifies a texture by a number it hands to its renderer: for the OpenGL
        // backend that is the id of the texture object. uv0 is the texture coordinate of
        // the top left corner of the picture and uv1 of the bottom right one. A
        // framebuffer texture has its row v = 0 at the BOTTOM, like everything OpenGL
        // draws, so the corners are (0, 1) and (1, 0): with the defaults the picture
        // would be upside down.
        const auto textureId = static_cast<ImTextureID>(picture.colorTextureId());
        ImGui::Image(textureId, {width, height}, {0.0F, 1.0F}, {1.0F, 0.0F});
        // A thin outline, so a dark picture still has an edge on the dark card.
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                            ImGui::GetColorU32(LINE_COLOR),
                                            PICTURE_ROUNDING * displayScale());
    } else {
        // The first frame after the picture was asked for (the game draws it in its
        // next frame), or a pass that is switched off.
        ImGui::TextDisabled("%s", drawn ? "(no picture yet)" : "(not drawn)");
    }
    ImGui::EndGroup();
    tooltipCard(caption, tooltip);
}

} // namespace debug
