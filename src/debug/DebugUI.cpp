// Debug user interface: owns the Dear ImGui context and draws the debug panels and the HUD.
// See docs/modules/debug-ui.md
#include "debug/DebugUI.hpp"

#include "core/Window.hpp"
#include "debug/DebugContext.hpp"
#include "debug/Hud.hpp"
#include "debug/Theme.hpp"
#include "debug/panels/AssetsPanel.hpp"
#include "debug/panels/CollisionPanel.hpp"
#include "debug/panels/EnvironmentPanel.hpp"
#include "debug/panels/GrassPanel.hpp"
#include "debug/panels/LightsPanel.hpp"
#include "debug/panels/MazePanel.hpp"
#include "debug/panels/RendererPanel.hpp"
#include "debug/panels/ShadersPanel.hpp"
#include "debug/panels/ShadowsPanel.hpp"
#include "debug/panels/TerrainPanel.hpp"
#include "game/Interaction.hpp"
#include "game/Lighting.hpp"
#include "game/MazeWorld.hpp"
#include "game/MenuCamera.hpp"
#include "game/Minimap.hpp"
#include "game/MinimapRenderer.hpp"
#include "game/PostProcess.hpp"
#include "game/Shadows.hpp"
#include "game/Skybox.hpp"

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

    // The preview pictures of the framebuffer attachments are drawn by the game only
    // while the card that shows them is drawn (the Post process category). That card
    // sets the flag again below. With the debug UI hidden nobody does, and the game
    // stops drawing the pictures.
    context.postProcessSettings.previews = false;
    // The same for the preview pictures of the two shadow maps and the Shadows panel.
    context.moonShadowSettings.preview = false;
    context.flashlightShadowSettings.preview = false;

    if (m_visible) {
        // An invisible dock area that covers the whole window, so panels can be docked to
        // its edges. PassthruCentralNode keeps the middle transparent: the scene shows through.
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                     ImGuiDockNodeFlags_PassthruCentralNode);

        // Each panel gets exactly the members it needs, so its signature still shows
        // what it reads and what it edits.
        drawRendererPanel(context.time, context.window);

        // The Shaders panel takes a list, so that a new program is one more entry here
        // and no change in the panel. The array holds pointers, because a reference
        // cannot be an element of an array.
        constexpr int SHADER_COUNT = 14;
        const std::array<gfx::Shader*, SHADER_COUNT> shaders = {
            &context.texturedShader,       &context.colorShader,       &context.litShader,
            &context.gouraudShader,        &context.skyboxShader,      &context.grassShader,
            &context.compositeShader,      &context.previewShader,     &context.brightPassShader,
            &context.blurShader,           &context.shadowDepthShader, &context.minimapShader,
            &context.minimapOverlayShader, &context.reflectShader};
        drawShadersPanel(shaders);

        drawTerrainPanel(context.terrain, context.mazeWorld.terrain);
        drawGrassPanel(context.grass, context.grassTuftCount);
        drawShadowsPanel({.settings = context.moonShadowSettings,
                          .map = context.moonShadowMap,
                          .lightSpace = context.moonLightSpace,
                          .drawn = context.moonShadowSettings.enabled},
                         {.settings = context.flashlightShadowSettings,
                          .map = context.flashlightShadowMap,
                          .lightSpace = context.flashlightLightSpace,
                          .drawn = context.flashlightShadowDrawn});
        drawMazePanel(context.mazeSettings, context.mazeWorld, context.round, context.player,
                      context.camera);
        drawCollisionPanel(context.mazeWorld, context.round, context.player, context.drawColliders,
                           context.pick, context.pickDebug);
        drawAssetsPanel(context.assets, m_rawTextureSampler);
        drawLightsPanel(context.lighting, context.round);
        drawEnvironmentPanel(context.environment, context.puddleCount);

        // The debug window, which takes the place of the panels above one category at
        // a time.
        m_window.draw(context);
    }

    // The HUD belongs to the game and not to the tools, so it is drawn whether or not
    // the panels are visible.
    // The game says when: not under a menu, where it would lie on top of the buttons,
    // and not in the picture of the menu camera, which shows no round.
    if (context.hudVisible) {
        drawHud(context.mazeWorld, context.round, context.gameplay, context.pick, m_visible);
    }

    // Render turns the widgets into draw lists, the backend sends them to OpenGL.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace debug
