// Buffer: a block of memory on the graphics card, filled once with vertices or indices.
// See docs/modules/gfx/buffers-vao.md
#pragma once

#include <glad/gl.h>

#include <cstddef>

namespace gfx {

/// Owns one OpenGL buffer object whose contents are set once, in the constructor.
///
/// The constructor creates and fills the buffer, the destructor deletes it (RAII). The
/// object can be moved but not copied: a copy would hold the same buffer id and delete it
/// a second time. It needs a current OpenGL context for its whole life, so it must be
/// destroyed before the window.
class Buffer {
public:
    /// Creates a buffer, binds it to target and copies sizeInBytes bytes from data into it.
    /// target is GL_ARRAY_BUFFER (vertex data) or GL_ELEMENT_ARRAY_BUFFER (indices).
    /// OpenGL takes its own copy, so data may be freed right after the call.
    ///
    /// The buffer is left bound to target. For GL_ELEMENT_ARRAY_BUFFER this matters: that
    /// binding is not global, it is stored in the vertex array that is bound at the moment.
    /// So create the element buffer while its VertexArray is the bound one, otherwise the
    /// indices are attached to no vertex array (or to the wrong one). The VertexArray
    /// constructor binds, so creating the vertex array just before the buffer is enough.
    Buffer(GLenum target, const void* data, std::size_t sizeInBytes);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /// Takes over the buffer of other. other is left without a buffer.
    Buffer(Buffer&& other) noexcept;
    /// Deletes the buffer this object owns, then takes over the buffer of other.
    Buffer& operator=(Buffer&& other) noexcept;

    /// Binds the buffer to the target it was created for (glBindBuffer).
    void bind() const;

private:
    // Binding point given to the constructor, remembered so that bind() needs no argument.
    GLenum m_target;
    // Name (id) of the OpenGL buffer object. 0 is never a real buffer: it means "none".
    GLuint m_id = 0;
};

} // namespace gfx
