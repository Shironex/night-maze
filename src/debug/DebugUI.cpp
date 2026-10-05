// Debug user interface: owns the Dear ImGui context and draws the debug panels.
// See docs/modules/debug-ui.md
#include "debug/DebugUI.hpp"

#include "core/Window.hpp"
#include "debug/DebugContext.hpp"
#include "debug/Theme.hpp"
#include "debug/panels/AssetsPanel.hpp"
#include "debug/panels/CameraPanel.hpp"
#include "debug/panels/CollisionPanel.hpp"
#include "debug/panels/MazePanel.hpp"
#include "debug/panels/RendererPanel.hpp"
#include "debug/panels/ShadersPanel.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <array>

namespace debug {

DebugUI::DebugUI(const core::Window& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Platform backend: feeds GLFW input and window size into ImGui.
    // true = install GLFW callbacks (ImGui chains to callbacks that were set before).
    ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true);
    // Renderer backend: draws ImGui with OpenGL. The string is the GLSL version of its shaders.
    ImGui_ImplOpenGL3_Init("#version 410");

    // The look of the panels. The content scale is 1 at 100 % display scaling and 1.5 at
    // 150 % on Windows: the theme multiplies its sizes and the font by it. On macOS the
    // function returns 1, because a Retina display is handled by the framebuffer being
    // larger than the window, and ImGui follows that on its own.
    applyTheme(ImGui_ImplGlfw_GetContentScaleForWindow(window.nativeHandle()));
    loadFont(m_fontBytes);
}

DebugUI::~DebugUI() {
    // Reverse order of initialization.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool DebugUI::wantsKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool DebugUI::wantsMouse() const {
    const ImGuiIO& io = ImGui::GetIO();
    // With the mouse switched off ImGui still sets WantCaptureMouse while a button is
    // held down and the hidden cursor is at the position of a panel. Nothing in the panel
    // reacts, but the caller would block the mouse for the game, so the answer is no.
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouse) != 0) {
        return false;
    }
    return io.WantCaptureMouse;
}

void DebugUI::setMouseEnabled(bool enabled) {
    // ConfigFlags is a set of bits. With the NoMouse bit set, ImGui treats no panel as
    // being under the cursor when it starts a frame, so nothing is hovered or clicked.
    ImGuiIO& io = ImGui::GetIO();
    if (enabled) {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    } else {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
}

void DebugUI::draw(const DebugContext& context) {
    // An ImGui frame is started every frame, also when hidden, so that ImGui keeps
    // consuming input events and its internal timing stays correct.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (m_visible) {
        // An invisible dock area that covers the whole window, so panels can be docked to
        // its edges. PassthruCentralNode keeps the middle transparent: the scene shows through.
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                     ImGuiDockNodeFlags_PassthruCentralNode);

        // Each panel gets exactly the members it needs, so its signature still shows
        // what it reads and what it edits.
        drawRendererPanel(context.time, context.window, context.clearColor);

        // The Shaders panel takes a list, so that a new program is one more entry here
        // and no change in the panel. The array holds pointers, because a reference
        // cannot be an element of an array.
        constexpr int SHADER_COUNT = 3;
        const std::array<gfx::Shader*, SHADER_COUNT> shaders = {
            &context.shader, &context.texturedShader, &context.colorShader};
        drawShadersPanel(shaders);

        drawCameraPanel(context.camera, context.player, context.mouseSensitivity);
        drawMazePanel(context.mazeSettings, context.mazeWorld, context.player, context.camera);
        drawCollisionPanel(context.mazeWorld, context.player, context.drawColliders);
        drawAssetsPanel(context.assets, context.viewMode);
    }

    // Render turns the widgets into draw lists, the backend sends them to OpenGL.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace debug
