// Base class of a program: owns the window, input and clock, and runs the main loop.
#pragma once

#include "core/Input.hpp"
#include "core/PeriodicAverage.hpp"
#include "core/Time.hpp"
#include "core/Window.hpp"

#include <string>

namespace core {

/// Where the CPU time of a frame goes, in milliseconds: the three parts of the main loop,
/// each one averaged over the interval of the FPS display (Time::FPS_REFRESH_INTERVAL).
///
/// update is all fixed steps of a frame together, render is onRender and swap is
/// Window::swapBuffers. With vsync on, the wait for the screen is inside swap: a small
/// render next to a large swap then means the program waits for the screen and is not
/// slow. With vsync off, what is left in swap is the wait for the graphics card: the
/// driver holds the program there until the card has caught up. The three do not add
/// up to the frame time exactly: reading the events and the input is not measured.
struct CpuTimes {
    PeriodicAverage update;
    PeriodicAverage render;
    PeriodicAverage swap;
};

/// Owns the window, the keyboard and mouse state and the frame clock, and runs the main loop.
///
/// A concrete program derives from this class and fills in onUpdate and onRender.
/// Each frame: poll events, run zero or more fixed updates, render once, swap buffers.
/// The Escape key is handed to onEscapePressed (unless the keyboard is blocked, see Input).
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

    /// Called once when the Escape key goes down, after the events of the frame were
    /// read and before the fixed updates, so what it changes holds for the whole frame.
    /// It is not called while the keyboard is blocked (a text field is being edited).
    /// A program that does not override it closes its window: Escape quits.
    virtual void onEscapePressed();

    Window& window() { return m_window; }
    Input& input() { return m_input; }
    // Read-only: only run() may advance the clock.
    const Time& time() const { return m_time; }
    // Read-only: only run() measures.
    const CpuTimes& cpuTimes() const { return m_cpuTimes; }

private:
    // Order matters: members are constructed top to bottom, and Input needs the window.
    Window m_window;
    Input m_input;
    Time m_time;
    CpuTimes m_cpuTimes;
};

} // namespace core
