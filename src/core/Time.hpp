// Frame clock: delta time, fixed-step accumulator and averaged FPS.
// See docs/modules/core/main-loop.md
#pragma once

#include <chrono>

namespace core {

/// Measures frame time and splits it into fixed simulation steps.
///
/// Game logic runs in steps of exactly FIXED_DT seconds, so it behaves the same at any
/// frame rate. Rendering runs once per frame, however long the frame took.
class Time {
public:
    /// Length of one simulation step in seconds (120 steps per second).
    static constexpr double FIXED_DT = 1.0 / 120.0;

    /// Longest frame time that is fed into the simulation, in seconds.
    /// Without this limit one very slow frame (a breakpoint, a window drag) would demand
    /// so many catch-up steps that the next frame is slow too: the "spiral of death".
    static constexpr double MAX_FRAME_TIME = 0.25;

    /// How often the displayed FPS value is refreshed, in seconds.
    static constexpr double FPS_REFRESH_INTERVAL = 0.5;

    Time();

    /// Call once at the start of every frame. Measures the time since the previous call.
    void beginFrame();

    /// Returns true while there is at least one whole fixed step left to simulate,
    /// and takes that step out of the accumulator. Use it as a loop condition.
    bool consumeFixedStep();

    /// How far the current frame is between two fixed steps, in the range [0, 1).
    /// Rendering can use it to blend the previous and current simulation state.
    double alpha() const;

    /// Duration of the last frame in seconds (already clamped to MAX_FRAME_TIME).
    double deltaSeconds() const { return m_deltaSeconds; }

    /// Frames per second, averaged over the last FPS_REFRESH_INTERVAL.
    double fps() const { return m_fps; }

    /// Average time of one frame in milliseconds, over the same interval as fps().
    double frameTimeMs() const { return m_frameTimeMs; }

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point m_lastFrameStart;
    double m_deltaSeconds = 0.0;
    double m_accumulator = 0.0; // simulation time that has not been consumed yet

    // FPS averaging: count frames and their total time, publish the average periodically.
    double m_fpsElapsed = 0.0;
    int m_fpsFrameCount = 0;
    double m_fps = 0.0;
    double m_frameTimeMs = 0.0;
};

} // namespace core
