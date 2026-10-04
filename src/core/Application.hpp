// Base class of a program: owns the window, input and clock, and runs the main loop.
// See docs/modules/core/main-loop.md
#pragma once

#include "core/Input.hpp"
#include "core/Time.hpp"
#include "core/Window.hpp"

#include <string>

namespace core {

/// Owns the window, the keyboard state and the frame clock, and runs the main loop.
///
/// A concrete program derives from this class and fills in onUpdate and onRender.
/// Each frame: poll events, run zero or more fixed updates, render once, swap buffers.
/// Escape closes the window (unless the keyboard is blocked, see Input).
class Application {
public:
    /// Creates the window and the OpenGL context. Throws std::runtime_error on failure.
    Application(int width, int height, const std::string& title);
    virtual ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /// Runs the main loop until the window is closed.
    void run();

protected:
    /// Advances the simulation by one fixed step of fixedDt seconds.
    /// Called zero or more times per frame, depending on how long the frame took.
    virtual void onUpdate(double fixedDt) = 0;

    /// Draws one frame. alpha in [0, 1) tells how far the frame is between two fixed
    /// steps, see Time::alpha.
    virtual void onRender(double alpha) = 0;

    Window& window() { return m_window; }
    Input& input() { return m_input; }
    // Read-only: only run() may advance the clock.
    const Time& time() const { return m_time; }

private:
    // Order matters: members are constructed top to bottom, and Input needs the window.
    Window m_window;
    Input m_input;
    Time m_time;
};

} // namespace core
