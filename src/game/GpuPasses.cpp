// GpuPasses: how long the graphics card works on every pass of a frame.
#include "game/GpuPasses.hpp"

namespace game {

namespace {

// A timer query answers in nanoseconds, the debug window shows milliseconds.
constexpr double NANOSECONDS_PER_MILLISECOND = 1'000'000.0;

} // namespace

void GpuPassTimers::beginFrame(bool publish) {
    // A frame that returned early may have left a pass running.
    end();

    for (Slot& slot : m_slots) {
        // The answer to the query of an earlier frame, if the card has it by now.
        const std::optional<std::uint64_t> nanoseconds = slot.query.takeResult();
        if (nanoseconds.has_value() && slot.answerCounts) {
            slot.reading.milliseconds.add(static_cast<double>(*nanoseconds) /
                                          NANOSECONDS_PER_MILLISECOND);
        }
        if (publish) {
            slot.reading.milliseconds.publish();
        }
        // Until begin() and end() say otherwise, the pass has not run in this frame.
        slot.reading.ran = false;
        slot.reading.drawCalls = 0;
    }
}

void GpuPassTimers::begin(GpuPass pass) {
    end();
    m_running = pass;
    m_countersAtBegin = gfx::renderCounters();
    // While the answer of an earlier frame is still on its way this starts nothing:
    // the pass is then counted in this frame, and timed again in a later one.
    m_slots[static_cast<std::size_t>(pass)].query.begin();
}

void GpuPassTimers::end() {
    if (!m_running.has_value()) {
        return;
    }
    Slot& slot = m_slots[static_cast<std::size_t>(*m_running)];
    m_running.reset();

    // What the pass drew: the counters now, minus the counters at its begin().
    const gfx::RenderCounters counted =
        gfx::countersSince(gfx::renderCounters(), m_countersAtBegin);
    slot.reading.drawCalls = counted.drawCalls;
    slot.reading.ran = counted.drawCalls > 0;

    // end() is true only when begin() really started the query in this frame, so only
    // then a new answer is on its way. Otherwise the answer that is still waited for
    // belongs to an earlier frame, and what was noted about it stays.
    if (slot.query.end()) {
        slot.answerCounts = slot.reading.ran;
    }
}

const GpuPassReading& GpuPassTimers::reading(GpuPass pass) const {
    return m_slots[static_cast<std::size_t>(pass)].reading;
}

} // namespace game
