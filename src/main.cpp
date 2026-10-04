// Program entry point: joins the game with the debug UI and runs it.
// See docs/modules/debug-ui.md
#include "core/Log.hpp"
#include "debug/DebugUI.hpp"
#include "game/NightMazeApp.hpp"

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <exception>
#include <string>

namespace {

/// The game with the debug UI drawn on top of every frame.
///
/// This is the only place that knows about both game/ and debug/, so the game itself
/// never depends on the debug panels.
class DebugNightMazeApp final : public game::NightMazeApp {
protected:
    void onRender(double alpha) override {
        game::NightMazeApp::onRender(alpha);

        // The key left of 1 (` and ~ on a US keyboard) shows or hides the debug panels.
        if (input().wasKeyPressed(GLFW_KEY_GRAVE_ACCENT)) {
            m_debugUI.toggleVisible();
        }
        m_debugUI.draw(time(), window(), clearColor());

        // ImGui now knows whether it is using the keyboard (a text field is being edited
        // or a widget is active). If so, block the game's keyboard from the next frame
        // on, so typing does not trigger Escape, the panel toggle or player movement.
        input().setKeyboardBlocked(m_debugUI.wantsKeyboard());
    }

private:
    // Members are destroyed before base classes, so ImGui shuts down while the window
    // and its OpenGL context (owned by core::Application) still exist.
    debug::DebugUI m_debugUI{window()};
};

} // namespace

int main() {
    try {
        DebugNightMazeApp app;
        app.run();
    } catch (const std::exception& error) {
        // Startup failures (no window, no OpenGL 4.1) arrive here as exceptions.
        core::logError(std::string("Fatal: ") + error.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
