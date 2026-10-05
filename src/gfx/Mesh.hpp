// Mesh: vertices and indices of one model on the graphics card, ready to draw.
// See docs/modules/gfx/mesh.md
#pragma once

#include "gfx/Buffer.hpp"
#include "gfx/Vertex.hpp"
#include "gfx/VertexArray.hpp"

#include <glad/gl.h>

#include <cstdint>
#include <span>

namespace gfx {

/// Owns everything OpenGL needs to draw one indexed mesh: a vertex array, a vertex buffer
/// and an index buffer.
///
/// The constructor uploads the data, the destructors of the three members delete the
/// OpenGL objects (RAII). The object can be moved but not copied, because its members
/// can only be moved. It needs a current OpenGL context for its whole life, so it must be
/// destroyed before the window.
///
/// A mesh knows nothing about shaders, textures or matrices: the caller selects the
/// program and sets its uniforms, then calls draw().
class Mesh {
public:
    /// Copies the vertices and the indices to the graphics card and describes the four
    /// attributes of gfx::Vertex (position, normal, uv, tangent). OpenGL takes its own
    /// copy, so both arrays may be freed right after the call.
    ///
    /// Every index must be smaller than vertices.size(): OpenGL does not check that.
    /// primitive says how the indices are grouped: GL_TRIANGLES (every 3 indices are one
    /// triangle), GL_LINES (every 2 indices are one line segment) or GL_POINTS (every
    /// index is one point: the input of a geometry shader that builds its own triangles).
    ///
    /// The new vertex array is left bound.
    Mesh(std::span<const Vertex> vertices, std::span<const std::uint32_t> indices,
         GLenum primitive = GL_TRIANGLES);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    /// Takes over the three OpenGL objects of other, member by member (each member has
    /// its own move operations). other is left without them and must not be drawn.
    Mesh(Mesh&& other) noexcept = default;
    /// Deletes the objects this mesh owns, then takes over the ones of other.
    Mesh& operator=(Mesh&& other) noexcept = default;

    /// Draws the whole mesh (glDrawElements with all indices). The shader program has to
    /// be in use already.
    void draw() const;

    /// Draws a part of the mesh: indexCount indices starting with index number firstIndex
    /// (0 is the first index of the mesh). This is how a model with several materials is
    /// drawn, one part per material. The range must lie inside the mesh: firstIndex +
    /// indexCount must not be greater than indexCount(). A range that does not is not
    /// drawn at all.
    void draw(std::uint32_t firstIndex, std::uint32_t indexCount) const;

    /// Number of indices in the mesh: 3 for every triangle, 2 for every line or 1 for
    /// every point.
    std::uint32_t indexCount() const { return m_indexCount; }

private:
    // Order matters: members are constructed top to bottom.
    //   1. m_vertexArray: its constructor binds it, so the two buffers below are created
    //      while it is the bound vertex array.
    //   2. m_vertexBuffer: stays bound to GL_ARRAY_BUFFER, and that is how the attribute
    //      setup in the constructor body tells the vertex array which buffer to read from.
    //   3. m_indexBuffer: binding it to GL_ELEMENT_ARRAY_BUFFER records it in the bound
    //      vertex array.
    VertexArray m_vertexArray;
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;

    std::uint32_t m_indexCount;
    GLenum m_primitive;
};

} // namespace gfx
