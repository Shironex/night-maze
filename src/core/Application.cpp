// Base class of a program: owns the window, input and clock, and runs the main loop.
#include "core/Application.hpp"

#include <GLFW/glfw3.h>

#include <chrono>

namespace core {

namespace {

// The clock of the three CPU times, the same one core::Time uses: it never jumps
// backwards when the clock of the system is set.
using Clock = std::chrono::steady_clock;

// The time between two moments in milliseconds, as a double.
double millisecondsBetween(Clock::time_point from, Clock::time_point to) {
    return std::chrono::duration<double, std::milli>(to - from).count();
}

} // namespace

Application::Application(int width, int height, const std::string& title)
    : m_window(width, height, title), m_input(m_window.nativeHandle()) {}

void Application::run() {
    // Start measuring here, so the first frame does not include the start-up time.
    m_time.reset();

    while (!m_window.shouldClose()) {
        m_window.pollEvents();
        m_input.update();
        // What Escape means is up to the program: a game opens its pause menu.
        if (m_input.wasKeyPressed(GLFW_KEY_ESCAPE)) {
            onEscapePressed();
        }

        // Simulation: as many fixed steps as fit into the time that has passed.
        m_time.beginFrame();
        // The averages of the three CPU times cover the same interval as the FPS.
        if (m_time.fpsRefreshed()) {
            m_cpuTimes.update.publish();
            m_cpuTimes.render.publish();
            m_cpuTimes.swap.publish();
        }
        const Clock::time_point updateStart = Clock::now();
        while (m_time.consumeFixedStep()) {
            onUpdate(Time::FIXED_DT);
        }

        // Rendering: once per frame.
        const Clock::time_point renderStart = Clock::now();
        onRender(m_time.alpha());
        const Clock::time_point swapStart = Clock::now();
        m_window.swapBuffers();
        const Clock::time_point frameEnd = Clock::now();

        m_cpuTimes.update.add(millisecondsBetween(updateStart, renderStart));
        m_cpuTimes.render.add(millisecondsBetween(renderStart, swapStart));
        m_cpuTimes.swap.add(millisecondsBetween(swapStart, frameEnd));
    }
}

void Application::onEscapePressed() {
    m_window.requestClose();
}

} // namespace core
