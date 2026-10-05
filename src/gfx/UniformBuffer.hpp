// UniformBuffer: a block of memory on the graphics card that holds a uniform block.
// See docs/modules/gfx/uniform-buffers.md
#pragma once

#include <glad/gl.h>

#include <cstddef>

namespace gfx {

/// Owns one OpenGL buffer object used as a uniform buffer: the memory behind a uniform
/// block ("layout(std140) uniform Name { ... }") that several shader programs share.
///
/// A plain uniform belongs to one program and has to be set in every program that
/// declares it. A uniform block is filled once, here, and every program whose block is
/// connected to the same binding point reads the same bytes (see
/// Shader::bindUniformBlock).
///
/// The class only moves bytes. What the bytes mean is decided by the code that fills
/// them: the layout of the lights is in scene/LightBlock.hpp.
///
/// The constructor creates the buffer, the destructor deletes it (RAII). The object can
/// be moved but not copied: a copy would hold the same buffer id and delete it a second
/// time. It needs a current OpenGL context for its whole life, so it must be destroyed
/// before the window.
class UniformBuffer {
public:
    /// Creates a buffer of sizeInBytes bytes, all zero, and attaches it to the uniform
    /// buffer binding point number bindingPoint (glBindBufferBase). Binding points are
    /// numbered slots of the OpenGL context, like texture units: the buffer is put into
    /// a slot here, and a shader program is told to read its block from that slot.
    UniformBuffer(std::size_t sizeInBytes, GLuint bindingPoint);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    /// Takes over the buffer of other. other is left without a buffer.
    UniformBuffer(UniformBuffer&& other) noexcept;
    /// Deletes the buffer this object owns, then takes over the buffer of other.
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;

    /// Copies sizeInBytes bytes from data to the start of the buffer (glBufferSubData).
    /// The buffer keeps its size: more bytes than sizeInBytes() are not copied at all,
    /// and an error is logged.
    void update(const void* data, std::size_t sizeInBytes) const;

    /// The binding point the buffer is attached to.
    GLuint bindingPoint() const { return m_bindingPoint; }

    /// Size of the buffer in bytes, as given to the constructor.
    std::size_t sizeInBytes() const { return m_sizeInBytes; }

private:
    std::size_t m_sizeInBytes;
    GLuint m_bindingPoint;
    // Name (id) of the OpenGL buffer object. 0 is never a real buffer: it means "none".
    GLuint m_id = 0;
};

} // namespace gfx
