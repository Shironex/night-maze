// Debug user interface: owns the Dear ImGui context and draws the debug window and the HUD.
// See docs/modules/debug-ui.md
#include "debug/DebugUI.hpp"

#include "core/Window.hpp"
#include "debug/DebugContext.hpp"
#include "debug/Hud.hpp"
#include "debug/Theme.hpp"
#include "game/Player.hpp"
#include "game/PostProcess.hpp"
#include "game/Shadows.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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

    // The look of the debug UI. The content scale is 1 at 100 % display scaling and 1.5 at
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
    // held down and the hidden cursor is at the position of the debug window. Nothing in
    // the window reacts, but the caller would block the mouse for the game, so the answer
    // is no.
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouse) != 0) {
        return false;
    }
    return io.WantCaptureMouse;
}

void DebugUI::setMouseEnabled(bool enabled) {
    // ConfigFlags is a set of bits. With the NoMouse bit set, ImGui treats no window as
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

    // The preview pictures of the framebuffer attachments are drawn by the game only
    // while the card that shows them is drawn (the Post process category). That card
    // sets the flag again below. With the debug UI hidden nobody does, and the game
    // stops drawing the pictures.
    context.postProcessSettings.previews = false;
    // The same for the preview pictures of the two shadow maps (the Light category).
    context.moonShadowSettings.preview = false;
    context.flashlightShadowSettings.preview = false;

    if (m_visible) {
        // An invisible dock area that covers the whole window, so the pinned panel of
        // the debug window can be docked to its edges. PassthruCentralNode keeps the
        // middle transparent: the scene shows through.
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                     ImGuiDockNodeFlags_PassthruCentralNode);

        // The debug window: every debug control and every readout, in seven
        // categories.
        m_window.draw(context);
    }

    // The HUD belongs to the game and not to the tools, so it is drawn whether or not
    // the debug UI is visible.
    // The game says when: not under a menu, where it would lie on top of the buttons,
    // and not in the picture of the menu camera, which shows no round.
    if (context.hudVisible) {
        drawHud(context.mazeWorld, context.round, context.gameplay, context.player, context.pick,
                context.mapOnScreen, context.keys);
    }

    // Render turns the widgets into draw lists, the backend sends them to OpenGL.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace debug
