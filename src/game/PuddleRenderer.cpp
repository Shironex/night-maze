// PuddleRenderer: draws the puddles, one flat disc per puddle.
// See docs/modules/renderer/env-mapping.md
#include "game/PuddleRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "game/ModelDraw.hpp"
#include "game/Puddles.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture2D.hpp"

namespace game {

namespace {

// The colour of the water where it does not show the sky: dark, muddy water over earth.
// An sRGB value, chosen by eye like a pixel of a texture: draw converts it to a linear
// colour, which is what uTint of the shaders expects.
constexpr glm::vec3 PUDDLE_COLOR{0.07F, 0.09F, 0.11F};

// The disc as plain data, uploaded once: every puddle is drawn with this one mesh.
gfx::Mesh uploadDisc() {
    const PuddleMeshData disc = buildPuddleMesh();
    return {disc.vertices, disc.indices};
}

} // namespace

PuddleRenderer::PuddleRenderer(assets::AssetCache& assets)
    : m_disc(uploadDisc()),
      m_texture(&assets.whiteTexture()),
      m_normalMap(&assets.flatNormalTexture()) {}

void PuddleRenderer::upload(std::span<const Puddle> puddles) {
    m_matrices.clear();
    m_matrices.reserve(puddles.size());
    for (const Puddle& puddle : puddles) {
        m_matrices.push_back(puddleModelMatrix(puddle));
    }
}

void PuddleRenderer::draw(const gfx::Shader& shader) const {
    setModelSamplers(shader);
    // Water gives off no light of its own. The crystals, drawn with the same program
    // just before, set this uniform to their glow.
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});

    const glm::vec3 tint = gfx::srgbToLinear(PUDDLE_COLOR);
    for (const glm::mat4& matrix : m_matrices) {
        drawMesh(shader, m_disc, *m_texture, *m_normalMap, tint, matrix);
    }
}

} // namespace game
