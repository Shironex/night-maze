// UniformBuffer: a block of memory on the graphics card that holds a uniform block.
#include "gfx/UniformBuffer.hpp"

#include "core/GlCheck.hpp"
#include "core/Log.hpp"

namespace gfx {

UniformBuffer::UniformBuffer(std::size_t sizeInBytes, GLuint bindingPoint)
    : m_sizeInBytes(sizeInBytes), m_bindingPoint(bindingPoint) {
    GL_CHECK(glGenBuffers(1, &m_id));
    // GL_UNIFORM_BUFFER is the target a uniform buffer is filled through, like
    // GL_ARRAY_BUFFER for vertex data. OpenGL 4.1 can only fill the buffer that is bound.
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, m_id));
    // Allocates the memory. nullptr as the data means "nothing to copy yet": the
    // contents arrive with update(). GL_DYNAMIC_DRAW is a hint: the data changes often
    // (here once per frame) and is used for drawing.
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(m_sizeInBytes), nullptr,
                          GL_DYNAMIC_DRAW));
    // GL_UNIFORM_BUFFER has two kinds of binding. glBindBuffer above set the general
    // one, which only says which buffer the next glBufferData call works on.
    // glBindBufferBase puts the whole buffer into one of the numbered binding points,
    // and those are what the shader programs read their uniform blocks from. It sets
    // the general binding to the same buffer as well.
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, m_bindingPoint, m_id));
}

UniformBuffer::~UniformBuffer() {
    // OpenGL silently ignores the id 0 in glDeleteBuffers, so an object that was moved
    // from needs no special case. Deleting a buffer also takes it out of its binding point.
    GL_CHECK(glDeleteBuffers(1, &m_id));
}

// Move constructor: the new object takes the buffer id, and other gives it up.
UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : m_sizeInBytes(other.m_sizeInBytes), m_bindingPoint(other.m_bindingPoint), m_id(other.m_id) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
}

// Move assignment: this object already owns a buffer, which has to go first.
UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept {
    // buffer = std::move(buffer): nothing to do. Without this check the buffer would
    // be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the buffer owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteBuffers(1, &m_id));

    m_sizeInBytes = other.m_sizeInBytes;
    m_bindingPoint = other.m_bindingPoint;
    m_id = other.m_id;
    other.m_id = 0;
    return *this;
}

void UniformBuffer::update(const void* data, std::size_t sizeInBytes) const {
    // glBufferSubData does not grow a buffer: writing past its end is an OpenGL error
    // (GL_INVALID_VALUE). The mistake is reported here, in words.
    if (sizeInBytes > m_sizeInBytes) {
        core::logError("UniformBuffer::update: the data is larger than the buffer");
        return;
    }

    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, m_id));
    // Replaces sizeInBytes bytes starting at offset 0. Unlike glBufferData it keeps the
    // memory that was allocated in the constructor, which is cheaper every frame.
    GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(sizeInBytes), data));
}

} // namespace gfx
