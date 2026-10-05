// PostProcess: the HDR framebuffer the scene is drawn into, the bloom passes that make
// bright things glow, and the composite pass that brings the picture to the window with
// fog, exposure, tone mapping, a vignette and gamma correction.
// See docs/modules/renderer/post-process.md
#include "game/PostProcess.hpp"

#include "core/GlCheck.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "gfx/Shader.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace game {

namespace {

// The texture unit the passes read their input from. Every pass binds what it needs.
constexpr GLuint SOURCE_TEXTURE_UNIT = 0;

// The composite pass reads the scene from the unit above and the bloom from this one.
constexpr GLuint BLOOM_TEXTURE_UNIT = 1;

// The fog of the composite pass reads a third picture, the depth of the scene, from
// this unit.
constexpr GLuint DEPTH_TEXTURE_UNIT = 2;

// The values of the uniform uHorizontal in post/blur.frag: the direction of a blur pass.
constexpr int BLUR_HORIZONTAL = 1;
constexpr int BLUR_VERTICAL = 0;

// One triangle: three vertices, starting with number 0.
constexpr GLint FIRST_VERTEX = 0;
constexpr GLsizei TRIANGLE_VERTEX_COUNT = 3;

// Height of a preview picture in pixels. The width follows from the shape of the
// window. Small on purpose: the debug UI shows the pictures at about this size.
constexpr int PREVIEW_HEIGHT = 180;

// Brings a framebuffer that one flat triangle is drawn into to the given size: created
// on first use, resized later. format is GL_RGBA8 for a preview picture and GL_RGBA16F
// for a target that holds HDR colours.
void fitTarget(gfx::Framebuffer& target, int width, int height, gfx::ColorFormat format) {
    if (!target.isValid()) {
        // Colour only: one flat triangle needs no depth test.
        target = gfx::Framebuffer(
            {.width = width, .height = height, .color = format, .depth = gfx::DepthFormat::None});
    } else {
        target.resize(width, height);
    }
}

// The width of a preview picture for a scene framebuffer: the previews have the shape
// of the scene. The casts make it a division of floats.
int previewWidthFor(const gfx::Framebuffer& scene) {
    const float aspectRatio =
        static_cast<float>(scene.width()) / static_cast<float>(scene.height());
    return std::max(
        1, static_cast<int>(std::lround(static_cast<float>(PREVIEW_HEIGHT) * aspectRatio)));
}

} // namespace

bool PostProcess::beginScene(core::Size size) {
    if (size.width < 1 || size.height < 1) {
        return false;
    }

    // A new size: the first frame, or the window was resized.
    if (size.width != m_requestedSize.width || size.height != m_requestedSize.height) {
        m_requestedSize = size;
        // GL_RGBA16F for the colour: linear values that may pass 1 (HDR). A depth
        // texture and not a renderbuffer, so that later passes can read the depth.
        // A new object instead of resize(): it also covers the first frame and a
        // framebuffer that could not be created at the size before.
        m_scene = gfx::Framebuffer({.width = size.width,
                                    .height = size.height,
                                    .color = gfx::ColorFormat::Rgba16F,
                                    .depth = gfx::DepthFormat::Depth24});
    }
    if (!m_scene.isValid()) {
        return false;
    }

    m_scene.bind();
    return true;
}

void PostProcess::drawPreviews(const gfx::Shader& shader, const PostProcessSettings& settings,
                               float nearPlane, float farPlane) {
    if (!m_scene.isValid() || !shader.isValid()) {
        return;
    }

    const int previewWidth = previewWidthFor(m_scene);
    fitTarget(m_colorPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    fitTarget(m_depthPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    if (!m_colorPreview.isValid() || !m_depthPreview.isValid()) {
        return;
    }

    // One flat triangle per picture: nothing to test the depth against.
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    shader.use();
    shader.setInt(PREVIEW_SOURCE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    shader.setFloat(PREVIEW_NEAR_UNIFORM, nearPlane);
    shader.setFloat(PREVIEW_FAR_UNIFORM, farPlane);
    shader.setFloat(PREVIEW_DEPTH_RANGE_UNIFORM, settings.depthPreviewRange);

    // The colour attachment. Reading a texture of the scene framebuffer is allowed
    // here because another framebuffer is the target: a pass must never read the
    // texture it is drawing into.
    m_colorPreview.bind();
    m_scene.bindColorTexture(SOURCE_TEXTURE_UNIT);
    shader.setInt(PREVIEW_MODE_UNIFORM, static_cast<int>(AttachmentPreview::Color));
    drawFullscreenTriangle();

    // The depth attachment.
    m_depthPreview.bind();
    m_scene.bindDepthTexture(SOURCE_TEXTURE_UNIT);
    shader.setInt(PREVIEW_MODE_UNIFORM, static_cast<int>(AttachmentPreview::Depth));
    drawFullscreenTriangle();
}

void PostProcess::drawBloom(const gfx::Shader& brightShader, const gfx::Shader& blurShader,
                            const gfx::Shader& previewShader, const PostProcessSettings& settings) {
    // Until the passes below have run, this frame has no bloom.
    m_bloomDrawn = false;
    if (!settings.bloom.enabled || !m_scene.isValid() || !brightShader.isValid() ||
        !blurShader.isValid()) {
        return;
    }

    // The three targets follow the size of the scene framebuffer, so a resized window
    // resizes them in the same frame. GL_RGBA16F like the scene: the glow of a light
    // far brighter than white must stay brighter than the glow of a white wall.
    const int width = bloomTargetExtent(m_scene.width());
    const int height = bloomTargetExtent(m_scene.height());
    fitTarget(m_brightPass, width, height, gfx::ColorFormat::Rgba16F);
    fitTarget(m_blurHorizontal, width, height, gfx::ColorFormat::Rgba16F);
    fitTarget(m_bloom, width, height, gfx::ColorFormat::Rgba16F);
    if (!m_brightPass.isValid() || !m_blurHorizontal.isValid() || !m_bloom.isValid()) {
        return;
    }

    // One flat triangle per pass: nothing to test the depth against.
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    // Step 1, the bright pass: scene colour in, the light above the threshold out.
    // bind() sets the viewport to the smaller size of the target. The triangle still
    // covers it, so the picture of the scene is shrunk to it.
    brightShader.use();
    brightShader.setInt(BRIGHT_SCENE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    brightShader.setFloat(BRIGHT_THRESHOLD_UNIFORM, settings.bloom.threshold);
    m_brightPass.bind();
    m_scene.bindColorTexture(SOURCE_TEXTURE_UNIT);
    drawFullscreenTriangle();

    // Step 2, the blur. The weights are computed on the CPU and are the same for every
    // pass, so they are set once.
    blurShader.use();
    blurShader.setInt(BLUR_SOURCE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    const std::array<float, BLOOM_BLUR_WEIGHT_COUNT> weights = bloomBlurWeights();
    blurShader.setFloatArray(BLUR_WEIGHTS_UNIFORM, weights);

    // The number comes from a slider, where anything can be typed.
    const int iterations = std::clamp(settings.bloom.blurIterations, MIN_BLOOM_BLUR_ITERATIONS,
                                      MAX_BLOOM_BLUR_ITERATIONS);
    // What the next horizontal pass reads: the bright pass first, later the result of
    // the iteration before. The bright pass itself is never drawn over, so its preview
    // shows it as it was.
    const gfx::Framebuffer* source = &m_brightPass;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        // Horizontal: every pixel becomes the weighted sum of its row neighbours.
        m_blurHorizontal.bind();
        source->bindColorTexture(SOURCE_TEXTURE_UNIT);
        blurShader.setInt(BLUR_HORIZONTAL_UNIFORM, BLUR_HORIZONTAL);
        drawFullscreenTriangle();

        // Vertical, on the result of the horizontal pass: together a round blur.
        m_bloom.bind();
        m_blurHorizontal.bindColorTexture(SOURCE_TEXTURE_UNIT);
        blurShader.setInt(BLUR_HORIZONTAL_UNIFORM, BLUR_VERTICAL);
        drawFullscreenTriangle();

        // The next iteration blurs the bloom again. It reads m_bloom while it draws
        // into m_blurHorizontal, and then the other way round: never both at once.
        source = &m_bloom;
    }
    m_bloomDrawn = true;

    // Step 3, the pictures for the debug UI: the two HDR targets, encoded like the
    // colour attachment of the scene (mode Color of post/preview.frag).
    if (!settings.previews || !previewShader.isValid()) {
        return;
    }
    const int previewWidth = previewWidthFor(m_scene);
    fitTarget(m_brightPassPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    fitTarget(m_bloomPreview, previewWidth, PREVIEW_HEIGHT, gfx::ColorFormat::Rgba8);
    if (!m_brightPassPreview.isValid() || !m_bloomPreview.isValid()) {
        return;
    }

    previewShader.use();
    previewShader.setInt(PREVIEW_SOURCE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    previewShader.setInt(PREVIEW_MODE_UNIFORM, static_cast<int>(AttachmentPreview::Color));

    m_brightPassPreview.bind();
    m_brightPass.bindColorTexture(SOURCE_TEXTURE_UNIT);
    drawFullscreenTriangle();

    m_bloomPreview.bind();
    m_bloom.bindColorTexture(SOURCE_TEXTURE_UNIT);
    drawFullscreenTriangle();
}

void PostProcess::composite(const gfx::Shader& shader, const PostProcessSettings& settings,
                            core::Size windowSize, const SceneView& view) const {
    // From here on everything lands in the window: this pass, and the debug UI after it.
    gfx::Framebuffer::bindDefault(windowSize.width, windowSize.height);
    if (!m_scene.isValid() || !shader.isValid()) {
        return;
    }

    // The triangle covers every pixel, so the window is not cleared first. The depth
    // test has to be off: the depth buffer of the window is never cleared any more and
    // holds whatever it was created with, so a test against it could reject the
    // triangle.
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    // The shader encodes to sRGB itself. With this switch on, OpenGL would encode once
    // more on writing into an sRGB capable window. It is off by default: the line says
    // that the pass relies on it.
    GL_CHECK(glDisable(GL_FRAMEBUFFER_SRGB));

    shader.use();

    // The fog. Switched off, the shader does not read the depth texture at all, so the
    // picture is exactly the one of a frame without fog. The sampler gets its unit
    // either way: a sampler that was never set reads unit 0, the picture of the scene.
    const FogSettings& fog = settings.fog;
    shader.setInt(COMPOSITE_FOG_ENABLED_UNIFORM, fog.enabled ? 1 : 0);
    shader.setInt(COMPOSITE_DEPTH_UNIFORM, static_cast<int>(DEPTH_TEXTURE_UNIT));
    shader.setFloat(COMPOSITE_FOG_DENSITY_UNIFORM, fog.density);
    shader.setFloat(COMPOSITE_FOG_BASE_HEIGHT_UNIFORM, fog.baseHeight);
    shader.setFloat(COMPOSITE_FOG_HEIGHT_FALLOFF_UNIFORM, fog.heightFalloff);
    // The colour of the fog is an sRGB value and the picture it is mixed into is
    // linear, so it is converted here, once per frame.
    shader.setVec3(COMPOSITE_FOG_COLOR_UNIFORM, gfx::srgbToLinear(fog.color));
    shader.setMat4(COMPOSITE_INVERSE_VIEW_PROJECTION_UNIFORM, view.inverseViewProjection);
    shader.setVec3(COMPOSITE_EYE_UNIFORM, view.eye);
    if (fog.enabled) {
        // Reading the depth of the scene is allowed here because the window is the
        // target: the scene framebuffer is not being drawn into.
        m_scene.bindDepthTexture(DEPTH_TEXTURE_UNIT);
    }

    // The bloom is added only when it was asked for AND drawBloom has drawn it in this
    // frame. Otherwise the shader does not read the bloom texture at all, so the
    // picture is exactly the one of a frame without bloom. The sampler gets its unit
    // either way.
    const bool addBloom = settings.bloom.enabled && m_bloomDrawn;
    shader.setInt(COMPOSITE_BLOOM_ENABLED_UNIFORM, addBloom ? 1 : 0);
    shader.setInt(COMPOSITE_BLOOM_UNIFORM, static_cast<int>(BLOOM_TEXTURE_UNIT));
    shader.setFloat(COMPOSITE_BLOOM_INTENSITY_UNIFORM, settings.bloom.intensity);
    if (addBloom) {
        m_bloom.bindColorTexture(BLOOM_TEXTURE_UNIT);
    }

    // The sampler of the shader gets the number of the texture unit (glUniform1i), and
    // the colour texture of the scene is bound to that unit.
    shader.setInt(COMPOSITE_SCENE_UNIFORM, static_cast<int>(SOURCE_TEXTURE_UNIT));
    m_scene.bindColorTexture(SOURCE_TEXTURE_UNIT);
    shader.setFloat(COMPOSITE_EXPOSURE_UNIFORM, settings.exposure);
    // The enum values are the numbers post/composite.frag compares uToneMapping with.
    shader.setInt(COMPOSITE_TONE_MAPPING_UNIFORM, static_cast<int>(settings.toneMapping));

    // The vignette. Switched off, the shader skips its line.
    const VignetteSettings& vignette = settings.vignette;
    shader.setInt(COMPOSITE_VIGNETTE_ENABLED_UNIFORM, vignette.enabled ? 1 : 0);
    shader.setFloat(COMPOSITE_VIGNETTE_STRENGTH_UNIFORM, vignette.strength);
    shader.setFloat(COMPOSITE_VIGNETTE_RADIUS_UNIFORM, vignette.radius);

    drawFullscreenTriangle();
}

void PostProcess::drawFullscreenTriangle() const {
    m_triangle.bind();
    // No buffer and no attribute: the vertex shader makes the corners from gl_VertexID.
    GL_CHECK(glDrawArrays(GL_TRIANGLES, FIRST_VERTEX, TRIANGLE_VERTEX_COUNT));
}

} // namespace game
