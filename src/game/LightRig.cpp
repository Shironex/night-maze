// LightRig: sends the lights of a frame to the graphics card and draws the light markers.
// See docs/modules/game/flashlight.md
#include "game/LightRig.hpp"

#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Vertex.hpp"
#include "scene/Light.hpp"
#include "scene/LightBlock.hpp"
#include "scene/Transform.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace game {

namespace {

// Sizes of the two arrays below. std::size_t, because that is the type of an array size.
constexpr std::size_t CORNER_COUNT = 8;
constexpr std::size_t FACE_COUNT = 6;
constexpr std::size_t INDICES_PER_FACE = 6;

// The corners of a cube with a side of 1, centred on the origin. Only the position is
// filled in: the marker is drawn in one flat colour, so it needs no normal and no
// texture coordinate, and the 8 corners can be shared by the faces. Corners 0 to 3 are
// the bottom face (y = -0.5), corners 4 to 7 the top face (y = 0.5), each going around
// the face in the same order.
constexpr std::array<gfx::Vertex, CORNER_COUNT> MARKER_CORNERS = {
    gfx::Vertex{.position = {-0.5F, -0.5F, -0.5F}}, // 0
    gfx::Vertex{.position = {0.5F, -0.5F, -0.5F}},  // 1
    gfx::Vertex{.position = {0.5F, -0.5F, 0.5F}},   // 2
    gfx::Vertex{.position = {-0.5F, -0.5F, 0.5F}},  // 3
    gfx::Vertex{.position = {-0.5F, 0.5F, -0.5F}},  // 4
    gfx::Vertex{.position = {0.5F, 0.5F, -0.5F}},   // 5
    gfx::Vertex{.position = {0.5F, 0.5F, 0.5F}},    // 6
    gfx::Vertex{.position = {-0.5F, 0.5F, 0.5F}},   // 7
};

// Every three indices are one triangle, two triangles per face. The direction the
// triangles are wound in does not matter here: the game does not remove back faces.
constexpr std::array<std::uint32_t, FACE_COUNT * INDICES_PER_FACE> MARKER_INDICES = {
    0, 1, 2, 2, 3, 0, // bottom
    4, 5, 6, 6, 7, 4, // top
    0, 1, 5, 5, 4, 0, // z = -0.5
    3, 2, 6, 6, 7, 3, // z = +0.5
    0, 3, 7, 7, 4, 0, // x = -0.5
    1, 2, 6, 6, 5, 1, // x = +0.5
};

// Side of a marker cube in metres.
constexpr float MARKER_SIZE = 0.14F;

} // namespace

LightRig::LightRig()
    : m_lightBuffer(sizeof(scene::LightBlockData), LIGHT_BLOCK_BINDING_POINT),
      m_markerCube(MARKER_CORNERS, MARKER_INDICES) {}

void LightRig::connect(gfx::Shader& shader) const {
    shader.bindUniformBlock(LIGHT_BLOCK_NAME, m_lightBuffer.bindingPoint(),
                            m_lightBuffer.sizeInBytes());
}

void LightRig::upload(const scene::LightSet& lights, const glm::vec3& cameraPosition) const {
    // The struct has exactly the bytes the block of the shader expects (the asserts in
    // scene/LightBlock.hpp), so it is copied as it is.
    const scene::LightBlockData block = scene::packLightBlock(lights, cameraPosition);
    m_lightBuffer.update(&block, sizeof(block));
}

void LightRig::drawMarkers(const gfx::Shader& shader, std::span<const glm::vec3> positions,
                           const glm::vec3& color) const {
    shader.setVec3(COLOR_UNIFORM, color);

    for (const glm::vec3& position : positions) {
        // The cube is centred on its origin, so its position is the place of the light.
        scene::Transform transform;
        transform.position = position;
        transform.scale = glm::vec3{MARKER_SIZE};

        shader.setMat4(MODEL_UNIFORM, transform.matrix());
        m_markerCube.draw();
    }
}

} // namespace game
