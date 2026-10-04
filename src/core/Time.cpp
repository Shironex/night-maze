// Frame clock: delta time, fixed-step accumulator and averaged FPS.
// See docs/modules/core/main-loop.md
#include "core/Time.hpp"

#include <algorithm>

namespace core {

Time::Time() : m_lastFrameStart(Clock::now()) {}

void Time::beginFrame() {
    const Clock::time_point now = Clock::now();
    // duration<double> converts the clock's native ticks to seconds as a double.
    const double realDelta = std::chrono::duration<double>(now - m_lastFrameStart).count();
    m_lastFrameStart = now;

    // The FPS display uses the real, unclamped time.
    m_fpsElapsed += realDelta;
    m_fpsFrameCount += 1;
    if (m_fpsElapsed >= FPS_REFRESH_INTERVAL) {
        m_fps = m_fpsFrameCount / m_fpsElapsed;
        m_frameTimeMs = 1000.0 * m_fpsElapsed / m_fpsFrameCount;
        m_fpsElapsed = 0.0;
        m_fpsFrameCount = 0;
    }

    // The simulation uses the clamped time.
    m_deltaSeconds = std::min(realDelta, MAX_FRAME_TIME);
    m_accumulator += m_deltaSeconds;
}

bool Time::consumeFixedStep() {
    if (m_accumulator < FIXED_DT) {
        return false;
    }
    m_accumulator -= FIXED_DT;
    return true;
}

double Time::alpha() const {
    return m_accumulator / FIXED_DT;
}

} // namespace core
