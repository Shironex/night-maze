// Vertex array: remembers how vertex attributes are read from buffers.
// See docs/modules/gfx/buffers-vao.md
#include "gfx/VertexArray.hpp"

#include "core/GlCheck.hpp"

namespace gfx {

VertexArray::VertexArray() {
    // glGenVertexArrays writes new ids into an array. Here the array is the one member.
    GL_CHECK(glGenVertexArrays(1, &m_id));
}

VertexArray::~VertexArray() {
    // OpenGL silently ignores the id 0 in glDeleteVertexArrays, so an object that was
    // moved from needs no special case.
    GL_CHECK(glDeleteVertexArrays(1, &m_id));
}

// Move constructor: the new object takes the vertex array id, and other gives it up.
VertexArray::VertexArray(VertexArray&& other) noexcept : m_id(other.m_id) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
}

// Move assignment: this object already owns a vertex array, which has to go first.
VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    // vertexArray = std::move(vertexArray): nothing to do. Without this check the vertex
    // array would be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the vertex array owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteVertexArrays(1, &m_id));

    m_id = other.m_id;
    other.m_id = 0;
    return *this;
}

void VertexArray::bind() const {
    GL_CHECK(glBindVertexArray(m_id));
}

void VertexArray::setFloatAttribute(GLuint index, GLint componentCount, GLsizei strideInBytes,
                                    std::size_t offsetInBytes) {
    // Attribute state belongs to the vertex array that is bound, so make sure it is this one.
    bind();
    GL_CHECK(glEnableVertexAttribArray(index));

    // The last parameter of glVertexAttribPointer has the type "pointer" for historical
    // reasons: in old OpenGL it could be the address of an array in the program's memory.
    // With a buffer bound to GL_ARRAY_BUFFER it is a byte offset into that buffer, so the
    // number is passed disguised as a pointer and nothing is ever read through it.
    // clang-tidy normally forbids turning a number into a pointer. Here the API demands
    // it, so the check is switched off for this one line.
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const void* offsetAsPointer = reinterpret_cast<const void*>(offsetInBytes);

    // GL_FLOAT: each component is a 32 bit float. GL_FALSE: the values are used as they
    // are (normalization only applies to integer data).
    GL_CHECK(glVertexAttribPointer(index, componentCount, GL_FLOAT, GL_FALSE, strideInBytes,
                                   offsetAsPointer));
}

} // namespace gfx
