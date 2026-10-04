// Buffer: a block of memory on the graphics card, filled once with vertices or indices.
// See docs/modules/gfx/buffers-vao.md
#include "gfx/Buffer.hpp"

#include "core/GlCheck.hpp"

namespace gfx {

Buffer::Buffer(GLenum target, const void* data, std::size_t sizeInBytes) : m_target(target) {
    // glGenBuffers writes new ids into an array. Here the array is the one member.
    GL_CHECK(glGenBuffers(1, &m_id));
    // OpenGL 4.1 can only fill the buffer that is bound, so bind first.
    GL_CHECK(glBindBuffer(m_target, m_id));
    // Allocates sizeInBytes bytes on the graphics card and copies the data there.
    // The size parameter is a signed type (GLsizeiptr), hence the cast.
    // GL_STATIC_DRAW is a hint: the data is set once and used for drawing many times.
    GL_CHECK(glBufferData(m_target, static_cast<GLsizeiptr>(sizeInBytes), data, GL_STATIC_DRAW));
}

Buffer::~Buffer() {
    // OpenGL silently ignores the id 0 in glDeleteBuffers, so an object that was moved
    // from needs no special case.
    GL_CHECK(glDeleteBuffers(1, &m_id));
}

// Move constructor: the new object takes the buffer id, and other gives it up.
Buffer::Buffer(Buffer&& other) noexcept : m_target(other.m_target), m_id(other.m_id) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
}

// Move assignment: this object already owns a buffer, which has to go first.
Buffer& Buffer::operator=(Buffer&& other) noexcept {
    // buffer = std::move(buffer): nothing to do. Without this check the buffer would
    // be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the buffer owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteBuffers(1, &m_id));

    m_target = other.m_target;
    m_id = other.m_id;
    other.m_id = 0;
    return *this;
}

void Buffer::bind() const {
    GL_CHECK(glBindBuffer(m_target, m_id));
}

} // namespace gfx
