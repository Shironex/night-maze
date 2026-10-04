// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#include "game/NightMazeApp.hpp"

#include "core/GlCheck.hpp"

namespace game {

namespace {

constexpr int INITIAL_WIDTH = 1280;
constexpr int INITIAL_HEIGHT = 720;

} // namespace

NightMazeApp::NightMazeApp() : core::Application(INITIAL_WIDTH, INITIAL_HEIGHT, "Night Maze") {}

void NightMazeApp::onUpdate(double /*fixedDt*/) {
    // No simulation yet.
}

void NightMazeApp::onRender(double /*alpha*/) {
    // The viewport is set in pixels, so it must come from the framebuffer size, which
    // differs from the window size on Retina displays. Querying it every frame also
    // handles window resizing.
    const core::Size framebuffer = window().framebufferSize();
    GL_CHECK(glViewport(0, 0, framebuffer.width, framebuffer.height));

    GL_CHECK(glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], 1.0F));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
}

} // namespace game
