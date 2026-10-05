// "Renderer" debug panel: frame statistics, OpenGL driver info, clear color, lighting mode,
// skybox.
// See docs/modules/debug-ui.md
#include "debug/panels/RendererPanel.hpp"

#include "core/Time.hpp"
#include "core/Window.hpp"
#include "debug/PanelLayout.hpp"
#include "game/Lighting.hpp"
#include "game/Skybox.hpp"

#include <imgui.h>

namespace debug {

namespace {

// The entries of the list, in the order of the enum game::LightingMode: the number of
// the chosen entry is the value of the enum. ImGui wants the entries in one string, each
// ended by a zero character.
constexpr const char* LIGHTING_MODE_ITEMS = "Unlit\0Gouraud\0Phong\0Blinn-Phong\0";

// Range of the slider of the sky brightness. 1 shows the sky pictures as they are, 0 is
// a black sky. The pictures are dark, so the range goes well above 1.
constexpr float MIN_SKY_BRIGHTNESS = 0.0F;
constexpr float MAX_SKY_BRIGHTNESS = 3.0F;

} // namespace

void drawRendererPanel(const core::Time& time, const core::Window& window,
                       std::array<float, 3>& clearColor, game::LightingMode& lightingMode,
                       game::SkyboxSettings& skybox) {
    // The place and the size of the panel the first time the program runs: the top left
    // corner of the window (the constant is in PanelLayout.hpp). The call counts only when
    // imgui.ini has no entry for this panel yet. After that the user decides where the
    // panel is.
    placePanelOnFirstUse(RENDERER_PLACEMENT);
    // Begin returns false when the panel is collapsed or hidden behind another tab.
    // End must be called in both cases.
    if (ImGui::Begin("Renderer")) {
        ImGui::Text("FPS: %.1f", time.fps());
        ImGui::Text("Frame time: %.2f ms", time.frameTimeMs());

        const core::Size framebuffer = window.framebufferSize();
        const core::Size windowSize = window.windowSize();
        ImGui::Text("Framebuffer: %d x %d px", framebuffer.width, framebuffer.height);
        ImGui::Text("Window: %d x %d", windowSize.width, windowSize.height);

        ImGui::Separator();
        ImGui::TextWrapped("OpenGL: %s", window.glVersion().c_str());
        ImGui::TextWrapped("GPU: %s", window.glRenderer().c_str());

        ImGui::Separator();
        // ColorEdit3 reads and writes three floats through the pointer.
        ImGui::ColorEdit3("Clear color", clearColor.data());

        // How the maze is shaded. Combo works on the number of the chosen entry and
        // returns true in the frame in which the user picked another one. Gouraud
        // computes the light per vertex, Phong and Blinn-Phong per fragment.
        int lightingModeIndex = static_cast<int>(lightingMode);
        if (ImGui::Combo("Lighting", &lightingModeIndex, LIGHTING_MODE_ITEMS)) {
            lightingMode = static_cast<game::LightingMode>(lightingModeIndex);
        }

        // The sky. Switched off, the clear colour above is the background again.
        // Checkbox and SliderFloat write through the pointers they are given.
        ImGui::Checkbox("Skybox", &skybox.enabled);
        ImGui::SetItemTooltip("The night sky (a cube map). The painted moon stands where the\n"
                              "default moon light comes from and does not follow the Moon\n"
                              "sliders of the Lights panel.");
        ImGui::SliderFloat("Sky brightness", &skybox.brightness, MIN_SKY_BRIGHTNESS,
                           MAX_SKY_BRIGHTNESS);
    }
    ImGui::End();
}

} // namespace debug
