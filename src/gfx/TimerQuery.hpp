// TimerQuery: asks the graphics card how long it worked on a group of commands.
#pragma once

#include <glad/gl.h>

#include <cstdint>
#include <optional>

namespace gfx {

/// Owns one OpenGL query object that measures time (GL_TIME_ELAPSED).
///
/// A clock of the CPU cannot time a draw call: the call only puts a command into a queue
/// and returns, and the card works on it later. A timer query is measured by the card
/// itself: the time it spent on the commands between begin() and end().
///
/// The answer is not there when end() returns, because the card is still working. Asking
/// for it at once would make the program wait for the card, and that wait would slow
/// down the very frame that is measured. So takeResult() asks first whether the answer
/// has arrived, and only then reads it: usually in the next frame. Until the answer was
/// read the object is busy and begin() starts nothing: a frame is then simply not
/// measured.
///
/// OpenGL 4.1 allows one running GL_TIME_ELAPSED query at a time, so two objects of
/// this class must never be between begin() and end() together: the ranges can follow
/// each other, they cannot lie inside each other.
///
/// The constructor creates the object, the destructor deletes it (RAII). The object can
/// be moved but not copied: a copy would hold the same id and delete it a second time.
/// It needs a current OpenGL context for its whole life, so it must be destroyed before
/// the window.
class TimerQuery {
public:
    TimerQuery();
    ~TimerQuery();

    TimerQuery(const TimerQuery&) = delete;
    TimerQuery& operator=(const TimerQuery&) = delete;

    /// Takes over the query object of other. other is left without one.
    TimerQuery(TimerQuery&& other) noexcept;
    /// Deletes the query object this object owns, then takes over the one of other.
    TimerQuery& operator=(TimerQuery&& other) noexcept;

    /// Starts measuring. Returns false and starts nothing while the answer of the
    /// measurement before has not been read with takeResult().
    bool begin();

    /// Ends the measurement begin() started. Returns false and does nothing when none
    /// is running.
    bool end();

    /// The time of the last measurement in nanoseconds, when the card has the answer.
    /// Empty when no answer is waited for or when it has not arrived yet: this function
    /// never waits. An answer is handed out once.
    std::optional<std::uint64_t> takeResult();

private:
    // Name (id) of the OpenGL query object. 0 is never a real one: it means "none".
    GLuint m_id = 0;
    // True between begin() and end().
    bool m_running = false;
    // True from end() until takeResult() has read the answer.
    bool m_waiting = false;
};

} // namespace gfx
