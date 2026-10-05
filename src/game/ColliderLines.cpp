// ColliderLines: draws collision boxes and spheres as thin lines, a debug view of the
// collisions.
// See docs/modules/scene/collision.md
#include "game/ColliderLines.hpp"

#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Vertex.hpp"
#include "scene/Transform.hpp"

#include <glm/gtc/constants.hpp>

#include <array>
#include <cmath>
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

// A circle is drawn as this many straight pieces. 32 look round at the size of a pickup
// sphere on the screen.
constexpr std::size_t CIRCLE_SEGMENTS = 32;

// The points of a circle with a radius of 1 around the origin, lying in the XY plane
// (z = 0). Point number i is at the angle i / CIRCLE_SEGMENTS of a full turn. It is
// a function and not a constant table like the cube: 32 sines and cosines are easier to
// compute than to type.
std::array<gfx::Vertex, CIRCLE_SEGMENTS> unitCirclePoints() {
    std::array<gfx::Vertex, CIRCLE_SEGMENTS> points{};
    for (std::size_t i = 0; i < CIRCLE_SEGMENTS; ++i) {
        const float angle =
            glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(CIRCLE_SEGMENTS);
        points[i].position = {std::cos(angle), std::sin(angle), 0.0F};
    }
    return points;
}

// Every two indices are one line (GL_LINES): each point is joined to the next one, and
// the last point back to the first (that is what the remainder does).
std::array<std::uint32_t, CIRCLE_SEGMENTS * INDICES_PER_LINE> unitCircleLines() {
    std::array<std::uint32_t, CIRCLE_SEGMENTS * INDICES_PER_LINE> indices{};
    for (std::size_t i = 0; i < CIRCLE_SEGMENTS; ++i) {
        indices[i * INDICES_PER_LINE] = static_cast<std::uint32_t>(i);
        indices[i * INDICES_PER_LINE + 1] = static_cast<std::uint32_t>((i + 1) % CIRCLE_SEGMENTS);
    }
    return indices;
}

// The unit circle lies in the XY plane. Turned by a quarter around the X axis it lies
// flat in the XZ plane, turned by a quarter around the Y axis it stands in the YZ plane.
constexpr float QUARTER_TURN_DEGREES = 90.0F;
constexpr std::array<glm::vec3, 3> CIRCLE_ROTATIONS = {
    glm::vec3{0.0F, 0.0F, 0.0F},
    glm::vec3{QUARTER_TURN_DEGREES, 0.0F, 0.0F},
    glm::vec3{0.0F, QUARTER_TURN_DEGREES, 0.0F},
};

} // namespace

ColliderLines::ColliderLines()
    : m_unitCube(UNIT_CUBE_CORNERS, UNIT_CUBE_EDGES, GL_LINES),
      m_unitCircle(unitCirclePoints(), unitCircleLines(), GL_LINES) {}

void ColliderLines::draw(const gfx::Shader& shader, std::span<const scene::Aabb> boxes,
                         const glm::vec3& color) const {
    // color is an sRGB value, a colour named for the screen. The scene buffer holds
    // linear colours, so it is converted here, and the composite pass shows it as given.
    shader.setVec3(COLOR_UNIFORM, gfx::srgbToLinear(color));

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

void ColliderLines::drawSpheres(const gfx::Shader& shader, std::span<const scene::Sphere> spheres,
                                const glm::vec3& color) const {
    // Converted to a linear colour, as in draw.
    shader.setVec3(COLOR_UNIFORM, gfx::srgbToLinear(color));

    for (const scene::Sphere& sphere : spheres) {
        // The unit circle has a radius of 1 around the origin, so scaling it by the
        // radius and moving it to the centre lands it on the sphere.
        scene::Transform transform;
        transform.position = sphere.center;
        transform.scale = glm::vec3{sphere.radius};

        for (const glm::vec3& rotation : CIRCLE_ROTATIONS) {
            transform.rotationDegrees = rotation;
            shader.setMat4(MODEL_UNIFORM, transform.matrix());
            m_unitCircle.draw();
        }
    }
}

} // namespace game
