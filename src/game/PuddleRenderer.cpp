// PuddleRenderer: draws the puddles, thin films of water that lie on the ground.
#include "game/PuddleRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/GlCheck.hpp"
#include "game/ModelDraw.hpp"
#include "game/Puddles.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture2D.hpp"

namespace game {

namespace {

// The colour of the water where it does not show the sky: a pale grey blue, the colour
// of a wet, shiny patch in weak light. It is lighter than the earth around it on
// purpose. A dark colour (the first one was 0.07, 0.09, 0.11) made a puddle in the
// beam of the flashlight a black hole in bright ground. An sRGB value, chosen by eye
// like a pixel of a texture: draw converts it to a linear colour, which is what uTint
// of the shaders expects.
constexpr glm::vec3 PUDDLE_COLOR{0.32F, 0.40F, 0.50F};

// How much the water hides of the ground in the middle of a puddle when it is looked at
// straight from above: 0 would be clear water, 1 paint. The film is thin, so the earth
// and its pebbles show through. At flat angles the cover grows with the mirror
// (reflect.frag).
constexpr float PUDDLE_OPACITY = 0.7F;

// No rim fade: what the uniform is set back to after the puddles, for the crystals.
constexpr float NO_RIM_FADE = 0.0F;

// The vertices of the puddles are in world space: the model matrix changes nothing.
constexpr glm::mat4 IDENTITY{1.0F};

} // namespace

PuddleRenderer::PuddleRenderer(assets::AssetCache& assets)
    : m_texture(&assets.whiteTexture()), m_normalMap(&assets.flatNormalTexture()) {}

void PuddleRenderer::upload(const Terrain& terrain, std::span<const Puddle> puddles) {
    m_puddleCount = puddles.size();
    // The old mesh goes first, also when no new one follows: a share of 0 has no
    // puddles, and a mesh without vertices cannot be created.
    m_mesh.reset();
    if (puddles.empty()) {
        return;
    }
    const PuddleMeshData water = buildPuddleMesh(terrain, puddles);
    m_mesh.emplace(water.vertices, water.indices);
}

void PuddleRenderer::draw(const gfx::Shader& shader) const {
    if (!m_mesh.has_value()) {
        return;
    }

    setModelSamplers(shader);
    // Water gives off no light of its own. The crystals, drawn with the same program
    // just before, set this uniform to their glow.
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});
    // The soft rim, see the header. The textured program of the debug views does not
    // have these two uniforms: setting them does nothing there.
    shader.setFloat(REFLECT_RIM_FADE_UNIFORM, PUDDLE_RIM_FADE);
    shader.setFloat(REFLECT_OPACITY_UNIFORM, PUDDLE_OPACITY);

    // Blending mixes what the shader writes with the ground that is in the framebuffer
    // already: result = water * alpha + ground * (1 - alpha). The second pair of
    // factors is for the alpha channel itself: 0 and 1 leave the alpha of the
    // framebuffer as it is, so the picture of the scene keeps an alpha of 1 everywhere.
    // The state is put back afterwards, as MinimapRenderer does: blending as it was
    // found, writing depth on again. The blend function is left set: everything else
    // that blends (the minimap, the debug UI) sets its own before it draws.
    GLboolean blendingWasOn = GL_FALSE;
    GL_CHECK(blendingWasOn = glIsEnabled(GL_BLEND));
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE));
    GL_CHECK(glDepthMask(GL_FALSE));

    drawMesh(shader, *m_mesh, *m_texture, *m_normalMap, gfx::srgbToLinear(PUDDLE_COLOR), IDENTITY);

    GL_CHECK(glDepthMask(GL_TRUE));
    if (blendingWasOn == GL_FALSE) {
        GL_CHECK(glDisable(GL_BLEND));
    }
    // The crystals are drawn with the same program in the next frame, before the
    // puddles: for them the uniform has to say "no rim" again.
    shader.setFloat(REFLECT_RIM_FADE_UNIFORM, NO_RIM_FADE);
}

} // namespace game
