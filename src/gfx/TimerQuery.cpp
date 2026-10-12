// TimerQuery: asks the graphics card how long it worked on a group of commands.
#include "gfx/TimerQuery.hpp"

#include "core/GlCheck.hpp"

namespace gfx {

TimerQuery::TimerQuery() {
    // glGenQueries writes new ids into an array. Here the array is the one member.
    GL_CHECK(glGenQueries(1, &m_id));
}

TimerQuery::~TimerQuery() {
    // OpenGL silently ignores the id 0 in glDeleteQueries, so an object that was moved
    // from needs no special case.
    GL_CHECK(glDeleteQueries(1, &m_id));
}

// Move constructor: the new object takes the query id, and other gives it up.
TimerQuery::TimerQuery(TimerQuery&& other) noexcept
    : m_id(other.m_id), m_running(other.m_running), m_waiting(other.m_waiting) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
    other.m_running = false;
    other.m_waiting = false;
}

// Move assignment: this object already owns a query object, which has to go first.
TimerQuery& TimerQuery::operator=(TimerQuery&& other) noexcept {
    // query = std::move(query): nothing to do. Without this check the query object
    // would be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the query object owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteQueries(1, &m_id));

    m_id = other.m_id;
    m_running = other.m_running;
    m_waiting = other.m_waiting;
    other.m_id = 0;
    other.m_running = false;
    other.m_waiting = false;
    return *this;
}

bool TimerQuery::begin() {
    // Beginning again would throw away the answer that is still on its way.
    if (m_running || m_waiting) {
        return false;
    }
    // From here on the card adds up the time it spends on every command, until
    // glEndQuery.
    GL_CHECK(glBeginQuery(GL_TIME_ELAPSED, m_id));
    m_running = true;
    return true;
}

bool TimerQuery::end() {
    if (!m_running) {
        return false;
    }
    // glEndQuery names the kind of query and not the object: only one of a kind runs.
    GL_CHECK(glEndQuery(GL_TIME_ELAPSED));
    m_running = false;
    m_waiting = true;
    return true;
}

std::optional<std::uint64_t> TimerQuery::takeResult() {
    if (!m_waiting) {
        return std::nullopt;
    }
    // First the question that never waits: has the answer arrived?
    GLint available = GL_FALSE;
    GL_CHECK(glGetQueryObjectiv(m_id, GL_QUERY_RESULT_AVAILABLE, &available));
    if (available == GL_FALSE) {
        return std::nullopt;
    }
    // It has, so reading it does not wait either. Nanoseconds in 64 bits: 32 bits
    // would be full after about four seconds.
    GLuint64 nanoseconds = 0;
    GL_CHECK(glGetQueryObjectui64v(m_id, GL_QUERY_RESULT, &nanoseconds));
    m_waiting = false;
    return nanoseconds;
}

} // namespace gfx
