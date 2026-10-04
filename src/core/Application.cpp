// Base class of a program: owns the window, input and clock, and runs the main loop.
// See docs/modules/core/main-loop.md
#include "core/Application.hpp"

#include <GLFW/glfw3.h>

namespace core {

Application::Application(int width, int height, const std::string& title)
    : m_window(width, height, title), m_input(m_window.nativeHandle()) {}

void Application::run() {
    while (!m_window.shouldClose()) {
        m_window.pollEvents();
        m_input.update();
        if (m_input.wasKeyPressed(GLFW_KEY_ESCAPE)) {
            m_window.requestClose();
        }

        // Simulation: as many fixed steps as fit into the time that has passed.
        m_time.beginFrame();
        while (m_time.consumeFixedStep()) {
            onUpdate(Time::FIXED_DT);
        }

        // Rendering: once per frame.
        onRender(m_time.alpha());
        m_window.swapBuffers();
    }
}

} // namespace core
