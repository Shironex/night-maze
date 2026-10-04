// Vertex array: remembers how vertex attributes are read from buffers.
// See docs/modules/gfx/buffers-vao.md
#pragma once

#include <glad/gl.h>

#include <cstddef>

namespace gfx {

/// Owns one OpenGL vertex array object (VAO).
///
/// A vertex array stores the description of the vertex data: which attributes are
/// enabled, their format, the buffer each one reads from, and the element (index) buffer.
/// It holds no vertex data itself. The OpenGL Core profile cannot draw without one.
///
/// The constructor creates the object, the destructor deletes it (RAII). The object can
/// be moved but not copied. It needs a current OpenGL context for its whole life, so it
/// must be destroyed before the window.
class VertexArray {
public:
    /// Creates an empty vertex array (no attribute is enabled yet) and binds it, so that
    /// a Buffer created after it is recorded in this vertex array.
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    /// Takes over the vertex array of other. other is left without one.
    VertexArray(VertexArray&& other) noexcept;
    /// Deletes the vertex array this object owns, then takes over the one of other.
    VertexArray& operator=(VertexArray&& other) noexcept;

    /// Makes this vertex array the current one (glBindVertexArray). Draw calls and
    /// attribute setup always work on the current vertex array.
    void bind() const;

    /// Describes one vertex attribute made of floats and enables it. Binds this vertex
    /// array first.
    ///
    /// index is the attribute number, the same as layout(location = index) in the vertex
    /// shader. componentCount is the number of floats per vertex, from 1 to 4 (3 for a
    /// vec3). strideInBytes is the distance from one vertex to the next in the buffer.
    /// offsetInBytes is where this attribute starts inside one vertex.
    ///
    /// The attribute reads from the buffer that is bound to GL_ARRAY_BUFFER at the time
    /// of this call: the vertex array records that buffer. So bind the vertex Buffer
    /// before calling this function.
    void setFloatAttribute(GLuint index, GLint componentCount, GLsizei strideInBytes,
                           std::size_t offsetInBytes);

private:
    // Name (id) of the OpenGL vertex array object. 0 is never a real one: it means "none".
    GLuint m_id = 0;
};

} // namespace gfx
