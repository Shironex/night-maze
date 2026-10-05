// Vertex: the layout of one vertex of a mesh (position, normal, texture coordinate).
// See docs/modules/gfx/mesh.md
#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <type_traits>

namespace gfx {

/// One vertex of a mesh, exactly as it lies in the vertex buffer: 8 floats, 32 bytes.
///
/// Plain data without OpenGL, so the loaders in assets/ can fill it and tests can read it
/// without a window. gfx::Mesh copies an array of these to the graphics card and tells
/// OpenGL where each field lies inside one vertex.
struct Vertex {
    /// Position in the local space of the model, in metres.
    glm::vec3 position{0.0F};

    /// Direction the surface faces at this vertex. Expected to have length 1. It is all
    /// zeros when the source had no normal.
    glm::vec3 normal{0.0F};

    /// Texture coordinate (u, v). v = 0 is the bottom row of the image. It is all zeros
    /// when the source had no texture coordinate.
    glm::vec2 uv{0.0F};
};

/// Number of floats in each field, the "size" parameter of glVertexAttribPointer.
constexpr int POSITION_COMPONENTS = 3;
constexpr int NORMAL_COMPONENTS = 3;
constexpr int UV_COMPONENTS = 2;

/// Attribute numbers of the fields. A vertex shader that reads a gfx::Mesh must declare
/// its inputs with the same numbers: layout(location = 0) in vec3 for the position,
/// location 1 for the normal and location 2 for the texture coordinate.
constexpr std::uint32_t POSITION_ATTRIBUTE = 0;
constexpr std::uint32_t NORMAL_ATTRIBUTE = 1;
constexpr std::uint32_t UV_ATTRIBUTE = 2;

// The vertex buffer is described to OpenGL as "8 floats per vertex, one after another".
// That is only true if the compiler puts no padding between or after the fields. All
// fields are made of floats, which have the same alignment (4 bytes), so there is no
// reason for padding. This line turns that expectation into a compile error on a compiler
// or with a GLM setting where it does not hold.
static_assert(sizeof(Vertex) ==
                  (POSITION_COMPONENTS + NORMAL_COMPONENTS + UV_COMPONENTS) * sizeof(float),
              "Vertex must be 8 tightly packed floats");

// offsetof, which gfx::Mesh uses to find the fields, is only guaranteed to work for
// standard-layout types (no virtual functions, all fields with the same access).
static_assert(std::is_standard_layout_v<Vertex>, "offsetof needs a standard-layout type");

} // namespace gfx
