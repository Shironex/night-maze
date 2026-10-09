// Program entry point: joins the game with the debug UI and runs it.
#include "core/Log.hpp"
#include "debug/DebugContext.hpp"
#include "debug/DebugUI.hpp"
#include "game/NightMazeApp.hpp"

#include <GLFW/glfw3.h>

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <span>
#include <string>

namespace {

/// The game with the debug UI and the HUD drawn on top of every frame.
///
/// This is the only place where the game meets the debug UI. The panels read game types,
/// but nothing in game/ includes debug code, so the game never depends on the panels.
class DebugNightMazeApp final : public game::NightMazeApp {
public:
    /// options is what the command line asked for (game::parseStartOptions).
    explicit DebugNightMazeApp(const game::StartOptions& options) : game::NightMazeApp(options) {}

protected:
    void onRender(double alpha) override {
        // The debug panels are drawn on top of the menu, so the mouse is theirs first:
        // while one of them is under the cursor the menu ignores the mouse. Dear ImGui
        // knows that from the frame before.
        menuUi().setMouseEnabled(!m_debugUI.wantsMouse());

        // The frame of the game, with the menu of its screen on top (ui::UiLayer). The
        // debug UI follows, on top of both.
        game::NightMazeApp::onRender(alpha);

        // The menu camera shows the game alone: in the frame it is switched on the
        // panels are hidden, and in the frame it is switched off they come back as they
        // were. In between the panel key below still works, so the settings of the
        // camera can be changed while it runs. The HUD is left out by DebugUI::draw.
        // The intro and the cards of the campaign are films too: while one runs the
        // panels are hidden in the same way.
        const bool menuCameraOn = menuCameraSettings().enabled || game::isFilm(gameMode());
        if (menuCameraOn != m_menuCameraWasOn) {
            m_menuCameraWasOn = menuCameraOn;
            if (menuCameraOn) {
                m_panelsVisibleBeforeMenuCamera = m_debugUI.isVisible();
                m_debugUI.setVisible(false);
            } else {
                m_debugUI.setVisible(m_panelsVisibleBeforeMenuCamera);
            }
        }

        // The key left of 1 (` and ~ on a US keyboard) shows or hides the debug panels,
        // on every screen of the game: the keyboard is blocked only while a text field
        // is being edited. While a round is played the cursor is captured for mouse
        // look, so showing the panels gives it back for them. A click into the scene
        // captures it again (NightMazeApp::handleInteraction).
        if (input().wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)) {
            m_debugUI.toggleVisible();
            if (m_debugUI.isVisible()) {
                input().setCursorCaptured(false);
            }
        }
        // While the cursor is captured the mouse belongs to the camera. The hidden cursor
        // still has a position that moves with the mouse, so the panels must ignore it,
        // otherwise it would hover and click them unseen.
        m_debugUI.setMouseEnabled(!input().isCursorCaptured());
        // The context is rebuilt every frame: it only holds references, so it is cheap.
        m_debugUI.draw(debug::DebugContext{
            .time = time(),
            .window = window(),
            .clearColor = clearColor(),
            .camera = camera(),
            .mouseSensitivity = mouseSensitivity(),
            .texturedShader = texturedShader(),
            .colorShader = colorShader(),
            .player = player(),
            .mazeSettings = mazeSettings(),
            .mazeWorld = mazeWorld(),
            .assets = assets(),
            .viewMode = viewMode(),
            .drawColliders = drawColliders(),
            .litShader = litShader(),
            .gouraudShader = gouraudShader(),
            .lighting = lighting(),
            .gameplay = gameplaySettings(),
            .round = round(),
            .skyboxShader = skyboxShader(),
            .skybox = skyboxSettings(),
            .village = villageSettings(),
            .grassShader = grassShader(),
            .terrain = terrainSettings(),
            .grass = grassSettings(),
            .grassTuftCount = grassTuftCount(),
            .compositeShader = compositeShader(),
            .previewShader = previewShader(),
            .postProcessSettings = postProcessSettings(),
            .postProcess = postProcess(),
            .brightPassShader = brightPassShader(),
            .blurShader = blurShader(),
            .shadowDepthShader = shadowDepthShader(),
            .moonShadowSettings = moonShadowSettings(),
            .moonShadowMap = moonShadowMap(),
            .moonLightSpace = moonLightSpace(),
            .flashlightShadowSettings = flashlightShadowSettings(),
            .flashlightShadowMap = flashlightShadowMap(),
            .flashlightLightSpace = flashlightLightSpace(),
            .flashlightShadowDrawn = flashlightShadowDrawn(),
            .minimapShader = minimapShader(),
            .minimapOverlayShader = minimapOverlayShader(),
            .minimapSettings = minimapSettings(),
            .minimap = minimapRenderer(),
            .reflectShader = reflectShader(),
            .environment = environmentSettings(),
            .puddleCount = puddleCount(),
            .pick = pick(),
            .pickDebug = pickDebug(),
            .menuCamera = menuCameraSettings(),
            .hudVisible = hudVisible(),
            .mapOnScreen = mapOnScreen(),
            .menuCameraLoopSeconds = menuCameraLoopSeconds(),
            .gameMode = gameMode(),
            .audio = audio(),
            .lastCueName = lastCueName(),
            .cuesPlayed = cuesPlayed(),
            .masterVolume = masterVolumeSetting(),
            .effectsVolume = effectsVolumeSetting(),
            .ambientVolume = ambientVolumeSetting(),
            .musicVolume = musicVolumeSetting(),
            .lastCueVolume = lastCueVolume(),
            .sampleWind = windSampleRequest(),
            .playIntro = introRequest(),
            .keys = keyBindings(),
            .campaignNight = settings().campaignNight,
            .campaignSeed = settings().campaignSeed,
            .campaignBestSeconds = settings().campaignBestSeconds,
            .playedNight = playedNight(),
            .playedDaily = playedDaily(),
            .campaignRequest = campaignRequest(),
        });

        // ImGui now knows whether it is using the keyboard (a text field is being edited
        // or a widget is active) and the mouse (the cursor is over a panel or a widget is
        // being dragged). Block each device for the game from the next frame on, so typing
        // does not trigger Escape, the panel toggle or player movement, and working with
        // a panel does not click or look around in the scene.
        //
        // A menu blocks nothing here. The game itself reads no key and no mouse of the
        // round while a menu is open (game::updatesRound), and that takes effect in
        // the same frame. The one exception is a text field of a menu: while it has
        // the focus, typing belongs to it, like for a text field of a panel.
        input().setKeyboardBlocked(m_debugUI.wantsKeyboard() || menuUi().wantsKeyboard());
        input().setMouseBlocked(m_debugUI.wantsMouse());
    }

private:
    // Members are destroyed before base classes, so ImGui shuts down while the window
    // and its OpenGL context (owned by core::Application) still exist.
    debug::DebugUI m_debugUI{window()};

    // Whether the menu camera was on in the frame before, and whether the panels were
    // shown when it was switched on: that is the state they return to.
    bool m_menuCameraWasOn = false;
    bool m_panelsVisibleBeforeMenuCamera = true;
};

} // namespace

// argc is the number of words on the command line and argv the words themselves. The
// first word is the name of the program, the switches follow.
int main(int argc, char** argv) {
    // The switches are read before the window is opened: a mistyped one ends the
    // program with a line in the log and the list of switches, not with a game that
    // silently ignores it.
    const std::span<const char* const> arguments(argv + 1, static_cast<std::size_t>(argc - 1));
    const game::StartOptionsResult start = game::parseStartOptions(arguments);
    if (!start.error.empty()) {
        core::logError(start.error);
        core::logError(game::START_OPTIONS_USAGE);
        return EXIT_FAILURE;
    }

    try {
        DebugNightMazeApp app(start.options);
        app.run();
    } catch (const std::exception& error) {
        // Startup failures (no window, no OpenGL 4.1) arrive here as exceptions.
        core::logError(std::string("Fatal: ") + error.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
