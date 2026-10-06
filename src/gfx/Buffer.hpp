// Buffer: a block of memory on the graphics card, filled with vertices or indices.
// See docs/modules/gfx/buffers-vao.md
#pragma once

#include <glad/gl.h>

#include <cstddef>

namespace gfx {

/// Owns one OpenGL buffer object. Most buffers are filled once, in the constructor (the
/// vertices of a model). A buffer whose contents change while the game runs is created
/// with the usage GL_DYNAMIC_DRAW and filled again with setData (the minimap).
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
    ///
    /// usage tells the driver how the buffer will be used, so it can choose the memory
    /// for it: GL_STATIC_DRAW (the default) for data that is set once and drawn many
    /// times, GL_DYNAMIC_DRAW for data that is replaced often. It is a hint: both kinds
    /// can be drawn and filled in the same ways.
    Buffer(GLenum target, const void* data, std::size_t sizeInBytes, GLenum usage = GL_STATIC_DRAW);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /// Takes over the buffer of other. other is left without a buffer.
    Buffer(Buffer&& other) noexcept;
    /// Deletes the buffer this object owns, then takes over the buffer of other.
    Buffer& operator=(Buffer&& other) noexcept;

    /// Binds the buffer to the target it was created for (glBindBuffer).
    void bind() const;

    /// Replaces the whole contents by sizeInBytes bytes copied from data. The buffer may
    /// get another size than it had. It is left bound to its target. glBufferData gives
    /// the buffer new memory instead of writing into the old one, so the graphics card
    /// can still finish a draw call that reads the old contents. Meant for a buffer
    /// created with GL_DYNAMIC_DRAW.
    void setData(const void* data, std::size_t sizeInBytes);

private:
    // Binding point given to the constructor, remembered so that bind() needs no argument.
    GLenum m_target;
    // The usage hint given to the constructor, used again by setData.
    GLenum m_usage;
    // Name (id) of the OpenGL buffer object. 0 is never a real buffer: it means "none".
    GLuint m_id = 0;
};

} // namespace gfx
