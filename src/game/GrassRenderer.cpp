// GrassRenderer: draws the grass, one point per tuft that the geometry shader turns into blades.
// See docs/modules/renderer/grass-geometry.md
#include "game/GrassRenderer.hpp"

#include "core/GlCheck.hpp"
#include "game/Grass.hpp"
#include "game/MazeRenderer.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Vertex.hpp"

#include <cstdint>
#include <vector>

namespace game {

namespace {

// The grass has no highlight. common/lighting.glsl still computes one, so its two
// numbers are set to values that are safe to compute with: no strength, and an exponent
// of 1 (the power of 0 with an exponent of 0 is not defined in GLSL).
constexpr float NO_SPECULAR_STRENGTH = 0.0F;
constexpr float PLAIN_SHININESS = 1.0F;

} // namespace

GrassRenderer::GrassRenderer()
    // An empty mesh: no vertex and no index. Drawing it draws nothing.
    : m_points(std::span<const gfx::Vertex>{}, std::span<const std::uint32_t>{}, GL_POINTS) {}

void GrassRenderer::upload(std::span<const GrassTuft> tufts) {
    // One gfx::Vertex per tuft. The position is the root of the tuft, in world space.
    // The random number of the tuft travels in the texture coordinate (u), which a point
    // has no other use for. The normal and the tangent stay zero: grass.vert does not
    // read them.
    std::vector<gfx::Vertex> vertices;
    std::vector<std::uint32_t> indices;
    vertices.reserve(tufts.size());
    indices.reserve(tufts.size());
    for (const GrassTuft& tuft : tufts) {
        // With GL_POINTS every index is one point, so the indices simply count up.
        indices.push_back(static_cast<std::uint32_t>(vertices.size()));
        vertices.push_back({.position = tuft.position, .uv = {tuft.random, 0.0F}});
    }

    // A gfx::Mesh is filled once, when it is created. So a new mesh is made, and the
    // move assignment deletes the buffers of the old one.
    m_points = gfx::Mesh(vertices, indices, GL_POINTS);
    m_tuftCount = tufts.size();
}

void GrassRenderer::draw(const gfx::Shader& shader, const glm::mat4& view,
                         const glm::mat4& projection, const GrassSettings& settings,
                         float timeSeconds, bool lit, ViewMode viewMode) const {
    // Nothing to draw, or nothing to draw with (the load error was logged once, when the
    // shader was created).
    if (m_tuftCount == 0 || !shader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
    shader.setFloat(GRASS_TIME_UNIFORM, timeSeconds);
    shader.setFloat(GRASS_BLADE_HEIGHT_UNIFORM, settings.bladeHeight);
    shader.setFloat(GRASS_WIND_STRENGTH_UNIFORM, settings.windStrength);
    shader.setInt(GRASS_LIT_UNIFORM, lit ? 1 : 0);
    // The enum values are the numbers grass.frag compares uViewMode with.
    shader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(viewMode));
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, NO_SPECULAR_STRENGTH);
    shader.setFloat(SHININESS_UNIFORM, PLAIN_SHININESS);

    // Back-face culling throws away triangles that are seen from behind. A blade of
    // grass is one flat strip without a back side of its own, and half of the blades
    // would vanish. The game does not switch culling on today. If it ever does, the
    // grass still has to be drawn without it, and the switch is put back afterwards.
    GLboolean cullingWasOn = GL_FALSE;
    GL_CHECK(cullingWasOn = glIsEnabled(GL_CULL_FACE));
    if (cullingWasOn == GL_TRUE) {
        GL_CHECK(glDisable(GL_CULL_FACE));
    }

    // One draw call for all tufts. The mesh was created with GL_POINTS, so the
    // geometry shader gets one point at a time.
    m_points.draw();

    if (cullingWasOn == GL_TRUE) {
        GL_CHECK(glEnable(GL_CULL_FACE));
    }
}

} // namespace game
