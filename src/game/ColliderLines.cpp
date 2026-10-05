// ColliderLines: draws collision boxes as thin lines, a debug view of the collisions.
// See docs/modules/scene/collision.md
#include "game/ColliderLines.hpp"

#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Vertex.hpp"
#include "scene/Transform.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace game {

namespace {

// Sizes of the two arrays below. std::size_t, because that is the type of an array size.
constexpr std::size_t CORNER_COUNT = 8;
constexpr std::size_t EDGE_COUNT = 12;
constexpr std::size_t INDICES_PER_LINE = 2;

// The corners of a cube that reaches from (0, 0, 0) to (1, 1, 1). Only the position is
// filled in: lines have no use for a normal or a texture coordinate, and they stay zero.
// Corners 0 to 3 are the bottom face (y = 0), corners 4 to 7 the top face (y = 1), each
// going around the face.
constexpr std::array<gfx::Vertex, CORNER_COUNT> UNIT_CUBE_CORNERS = {
    gfx::Vertex{.position = {0.0F, 0.0F, 0.0F}}, // 0
    gfx::Vertex{.position = {1.0F, 0.0F, 0.0F}}, // 1
    gfx::Vertex{.position = {1.0F, 0.0F, 1.0F}}, // 2
    gfx::Vertex{.position = {0.0F, 0.0F, 1.0F}}, // 3
    gfx::Vertex{.position = {0.0F, 1.0F, 0.0F}}, // 4
    gfx::Vertex{.position = {1.0F, 1.0F, 0.0F}}, // 5
    gfx::Vertex{.position = {1.0F, 1.0F, 1.0F}}, // 6
    gfx::Vertex{.position = {0.0F, 1.0F, 1.0F}}, // 7
};

// Every two indices are one line (GL_LINES): the two corners an edge joins.
constexpr std::array<std::uint32_t, EDGE_COUNT * INDICES_PER_LINE> UNIT_CUBE_EDGES = {
    0, 1, 1, 2, 2, 3, 3, 0, // bottom face
    4, 5, 5, 6, 6, 7, 7, 4, // top face
    0, 4, 1, 5, 2, 6, 3, 7, // the four vertical edges
};

// The drawn box is this much larger than the real one on every side, in metres (1 cm).
// The box of a pillar is exactly as wide as the shaft of the pillar model, so lines at
// the true size would lie in the surface of the model and flicker in and out of it
// (z-fighting). The margin puts them just in front. Only the drawing is changed: the
// collisions use the true boxes.
constexpr float LINE_MARGIN = 0.01F;

} // namespace

ColliderLines::ColliderLines() : m_unitCube(UNIT_CUBE_CORNERS, UNIT_CUBE_EDGES, GL_LINES) {}

void ColliderLines::draw(const gfx::Shader& shader, std::span<const scene::Aabb> boxes,
                         const glm::vec3& color) const {
    shader.setVec3(COLOR_UNIFORM, color);

    // The line width is left at its default of 1 pixel on purpose: an OpenGL Core
    // profile is not required to support wider lines, and macOS does not.
    for (const scene::Aabb& box : boxes) {
        // The unit cube has its corner (0, 0, 0) in the origin, so scaling it by the
        // size of the box and then moving it to the min corner lands it on the box.
        scene::Transform transform;
        transform.position = box.min - glm::vec3{LINE_MARGIN};
        transform.scale = box.max - box.min + glm::vec3{2.0F * LINE_MARGIN};

        shader.setMat4(MODEL_UNIFORM, transform.matrix());
        m_unitCube.draw();
    }
}

} // namespace game
