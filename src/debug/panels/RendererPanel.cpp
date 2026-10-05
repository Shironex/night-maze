// "Renderer" debug panel: frame statistics, OpenGL driver info and the clear color.
// See docs/modules/debug-ui.md
#include "debug/panels/RendererPanel.hpp"

#include "core/Time.hpp"
#include "core/Window.hpp"

#include <imgui.h>

namespace debug {

namespace {

// Where the panel appears and how big it is the first time the program runs (later ImGui
// remembers it in imgui.ini): the top of the left edge of a 1280 x 720 window.
constexpr ImVec2 FIRST_POSITION{10.0F, 10.0F};
constexpr ImVec2 FIRST_SIZE{300.0F, 190.0F};

} // namespace

void drawRendererPanel(const core::Time& time, const core::Window& window,
                       std::array<float, 3>& clearColor) {
    // FirstUseEver: the two calls count only when imgui.ini has no entry for this panel
    // yet. After that the user decides where the panel is.
    ImGui::SetNextWindowPos(FIRST_POSITION, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(FIRST_SIZE, ImGuiCond_FirstUseEver);
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
    }
    ImGui::End();
}

} // namespace debug
