// Mesh: vertices and indices of one model on the graphics card, ready to draw.
// See docs/modules/gfx/mesh.md
#include "gfx/Mesh.hpp"

#include "core/GlCheck.hpp"

#include <cstddef>
#include <type_traits>

namespace gfx {

namespace {

// The indices are stored as std::uint32_t and drawn as GL_UNSIGNED_INT, the constant for
// GLuint. Both names must mean the same type, otherwise the card would read the index
// buffer with the wrong element size.
static_assert(std::is_same_v<GLuint, std::uint32_t>, "GL_UNSIGNED_INT must match uint32_t");

// Type of one index in the index buffer, as glDrawElements wants it.
constexpr GLenum INDEX_TYPE = GL_UNSIGNED_INT;

// Stride: bytes from the start of one vertex to the start of the next one (44).
constexpr GLsizei VERTEX_STRIDE = static_cast<GLsizei>(sizeof(Vertex));

} // namespace

// m_vertexArray is not in the list: its default constructor runs first anyway, because
// it is declared first, and binds the new vertex array.
Mesh::Mesh(std::span<const Vertex> vertices, std::span<const std::uint32_t> indices,
           GLenum primitive)
    // size_bytes() is the number of elements times the size of one element.
    : m_vertexBuffer(GL_ARRAY_BUFFER, vertices.data(), vertices.size_bytes()),
      m_indexBuffer(GL_ELEMENT_ARRAY_BUFFER, indices.data(), indices.size_bytes()),
      m_indexCount(static_cast<std::uint32_t>(indices.size())),
      m_primitive(primitive) {
    // m_vertexBuffer is still bound to GL_ARRAY_BUFFER (m_indexBuffer uses another binding
    // point). Each call below records that buffer in m_vertexArray for one attribute.
    // offsetof(Vertex, field) is the number of bytes from the start of a Vertex to the
    // field: 0 for position, 12 for normal, 24 for uv, 32 for tangent.
    m_vertexArray.setFloatAttribute(POSITION_ATTRIBUTE, POSITION_COMPONENTS, VERTEX_STRIDE,
                                    offsetof(Vertex, position));
    m_vertexArray.setFloatAttribute(NORMAL_ATTRIBUTE, NORMAL_COMPONENTS, VERTEX_STRIDE,
                                    offsetof(Vertex, normal));
    m_vertexArray.setFloatAttribute(UV_ATTRIBUTE, UV_COMPONENTS, VERTEX_STRIDE,
                                    offsetof(Vertex, uv));
    m_vertexArray.setFloatAttribute(TANGENT_ATTRIBUTE, TANGENT_COMPONENTS, VERTEX_STRIDE,
                                    offsetof(Vertex, tangent));
}

void Mesh::draw() const {
    draw(0, m_indexCount);
}

void Mesh::draw(std::uint32_t firstIndex, std::uint32_t indexCount) const {
    // A range that reaches past the end would make the card read outside the index
    // buffer, which OpenGL does not check. The comparison is written without firstIndex +
    // indexCount, because that sum could wrap around for huge numbers.
    if (firstIndex > m_indexCount || indexCount > m_indexCount - firstIndex) {
        return;
    }

    // One call brings back the whole description: the four attributes, the vertex buffer
    // they read from and the index buffer.
    m_vertexArray.bind();

    // The last parameter of glDrawElements has the type "pointer" for historical reasons.
    // With an index buffer recorded in the vertex array it is the byte offset of the first
    // index to draw inside that buffer: index number firstIndex starts firstIndex * 4
    // bytes from the beginning. The number is passed disguised as a pointer and nothing is
    // ever read through it. clang-tidy normally forbids turning a number into a pointer.
    // Here the API demands it, so the check is switched off for this one line.
    const std::size_t offsetInBytes = static_cast<std::size_t>(firstIndex) * sizeof(std::uint32_t);
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const void* offsetAsPointer = reinterpret_cast<const void*>(offsetInBytes);

    // The count is a number of indices, not of triangles. Its parameter is a signed type.
    GL_CHECK(
        glDrawElements(m_primitive, static_cast<GLsizei>(indexCount), INDEX_TYPE, offsetAsPointer));
}

} // namespace gfx
