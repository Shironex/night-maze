// ShadowMap: the depth texture a light draws the scene into, and how the lit shaders read it.
// See docs/modules/renderer/shadows.md
#include "game/ShadowMap.hpp"

#include "core/GlCheck.hpp"
#include "game/PostProcess.hpp"
#include "game/ShaderUniforms.hpp"
#include "game/Shadows.hpp"
#include "gfx/Shader.hpp"
#include "scene/LightSpace.hpp"

#include <algorithm>

namespace game {

namespace {

// The texture unit the preview pass reads the map from.
constexpr GLuint PREVIEW_SOURCE_UNIT = 0;

// The texture unit that is left active after bindForSampling.
constexpr GLuint FIRST_TEXTURE_UNIT = 0;

// Side of the preview picture in pixels: a shadow map is a square. Small on purpose: the
// debug UI shows the picture at about this size.
constexpr int PREVIEW_SIZE = 256;

// One triangle: three vertices, starting with number 0.
constexpr GLint FIRST_VERTEX = 0;
constexpr GLsizei TRIANGLE_VERTEX_COUNT = 3;

} // namespace

bool ShadowMap::beginDepthPass(int size) {
    // A new size: the first frame, or another resolution was chosen.
    if (size != m_requestedSize) {
        m_requestedSize = size;
        // No colour texture: the pass only has to record depths. A new object instead
        // of resize(): it also covers the first frame and a framebuffer that could not
        // be created at the size before.
        m_target = gfx::Framebuffer({.width = size,
                                     .height = size,
                                     .color = gfx::ColorFormat::None,
                                     .depth = gfx::DepthFormat::Depth24});
    }
    if (!m_target.isValid()) {
        return false;
    }

    // bind() sets the viewport to the size of the map. The viewport belongs to the
    // context, so the framebuffer that is bound after this pass sets it again.
    m_target.bind();
    // The depth test is what makes the map: of everything drawn at a texel the nearest
    // depth is kept. The last pass of the frame before (the composite pass) has left
    // the test switched off.
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));
    return true;
}

void ShadowMap::bindForSampling(GLuint unit, bool linearFilter) {
    if (m_sampler.linearFilter() != linearFilter) {
        m_sampler.setLinearFilter(linearFilter);
    }
    // The texture first: bindDepthTexture unbinds whatever sampler object the unit
    // had, so the comparison sampler has to follow it.
    m_target.bindDepthTexture(unit);
    m_sampler.bind(unit);
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + FIRST_TEXTURE_UNIT));
}

void ShadowMap::drawPreview(const gfx::Shader& previewShader) {
    if (!m_target.isValid() || !previewShader.isValid()) {
        return;
    }
    if (!m_preview.isValid()) {
        // Colour only: one flat triangle needs no depth test.
        m_preview = gfx::Framebuffer({.width = PREVIEW_SIZE,
                                      .height = PREVIEW_SIZE,
                                      .color = gfx::ColorFormat::Rgba8,
                                      .depth = gfx::DepthFormat::None});
    }
    if (!m_preview.isValid()) {
        return;
    }

    // One flat triangle: nothing to test the depth against.
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    previewShader.use();
    previewShader.setInt(PREVIEW_SOURCE_UNIFORM, static_cast<int>(PREVIEW_SOURCE_UNIT));
    previewShader.setInt(PREVIEW_MODE_UNIFORM, static_cast<int>(AttachmentPreview::RawDepth));

    // Reading the depth texture of the map is allowed here because another framebuffer
    // is the target. bindDepthTexture binds it WITHOUT a sampler object, so the shader
    // gets the stored depths and not the answers of a comparison.
    m_preview.bind();
    m_target.bindDepthTexture(PREVIEW_SOURCE_UNIT);
    m_triangle.bind();
    // No buffer and no attribute: the vertex shader makes the corners from gl_VertexID.
    GL_CHECK(glDrawArrays(GL_TRIANGLES, FIRST_VERTEX, TRIANGLE_VERTEX_COUNT));
}

void setShadowUniforms(const gfx::Shader& shader, const ShadowUniformNames& names, GLuint unit,
                       bool drawn, const ShadowSettings& settings,
                       const scene::LightSpace& lightSpace) {
    // The sampler gets its unit whether or not the map is read (see the header).
    shader.setInt(names.map, static_cast<int>(unit));
    shader.setInt(names.enabled, drawn ? 1 : 0);
    shader.setMat4(names.matrix, lightSpace.matrix());

    // The depth range of the box of the light: its z extent in metres is the stored
    // range from 0 to 1.
    const float depthRange = lightSpace.extent.z;
    shader.setFloat(names.constantBias, biasInDepthUnits(settings.constantBias, depthRange));
    shader.setFloat(names.slopeBias, biasInDepthUnits(settings.slopeBias, depthRange));
    shader.setInt(names.pcfRadius, pcfRadiusInUse(settings));
    // The number comes from a slider, where anything can be typed. Above 1 a shadow
    // would take away more light than there is.
    shader.setFloat(names.strength, std::clamp(settings.strength, 0.0F, 1.0F));
}

} // namespace game
