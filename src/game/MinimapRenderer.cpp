// MinimapRenderer: draws the minimap into a framebuffer of its own (offscreen rendering)
// and puts that picture into a corner of the window.
// See docs/modules/renderer/minimap.md
#include "game/MinimapRenderer.hpp"

#include "core/GlCheck.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"

#include <algorithm>
#include <cstddef>
#include <type_traits>

namespace game {

namespace {

// Attribute numbers of the two fields of game::MinimapVertex: layout(location = 0) and
// layout(location = 1) in post/minimap.vert.
constexpr GLuint POSITION_ATTRIBUTE = 0;
constexpr GLuint COLOR_ATTRIBUTE = 1;

// Stride: bytes from the start of one vertex to the start of the next one (20).
constexpr GLsizei VERTEX_STRIDE = static_cast<GLsizei>(sizeof(MinimapVertex));

// offsetof, which finds the fields below, is only guaranteed to work for
// standard-layout types (the same check as for gfx::Vertex).
static_assert(std::is_standard_layout_v<MinimapVertex>, "offsetof needs a standard-layout type");

// The texture unit the overlay pass reads the picture of the map from.
constexpr GLuint MAP_TEXTURE_UNIT = 0;

// Drawing starts with vertex number 0, and the triangle of the overlay pass has three.
constexpr GLint FIRST_VERTEX = 0;
constexpr GLsizei TRIANGLE_VERTEX_COUNT = 3;

// The alpha the picture of the map is cleared with: opaque. The opacity of the map on
// the screen is applied later, by the overlay pass.
constexpr float OPAQUE = 1.0F;

} // namespace

// m_vertexArray is not in the list: its default constructor runs first anyway, because
// it is declared first, and binds the new vertex array.
MinimapRenderer::MinimapRenderer()
    // No data yet: a buffer of 0 bytes. GL_DYNAMIC_DRAW tells the driver that the
    // contents will be replaced often (in every frame).
    : m_vertexBuffer(GL_ARRAY_BUFFER, nullptr, 0, GL_DYNAMIC_DRAW) {
    // m_triangle was created after the buffer and is the bound vertex array now. The
    // buffer is still bound to GL_ARRAY_BUFFER: that binding does not belong to
    // a vertex array. It is bound again all the same, so that the lines below do not
    // depend on the order of the members. setFloatAttribute binds m_vertexArray itself
    // and records the buffer in it for one attribute.
    m_vertexBuffer.bind();
    // offsetof(MinimapVertex, field) is the number of bytes from the start of a vertex
    // to the field: 0 for the position, 8 for the colour.
    m_vertexArray.setFloatAttribute(POSITION_ATTRIBUTE, MINIMAP_POSITION_COMPONENTS, VERTEX_STRIDE,
                                    offsetof(MinimapVertex, position));
    m_vertexArray.setFloatAttribute(COLOR_ATTRIBUTE, MINIMAP_COLOR_COMPONENTS, VERTEX_STRIDE,
                                    offsetof(MinimapVertex, color));
}

bool MinimapRenderer::drawMap(const gfx::Shader& shader, std::span<const MinimapVertex> vertices,
                              const glm::mat4& mapToClip, int pixels) {
    if (pixels < 1 || !shader.isValid()) {
        return false;
    }

    // A new size: the first frame, or the window or the size setting has changed. The
    // picture is exactly as large as the square it is shown in, so the overlay pass
    // copies it pixel for pixel and nothing is scaled or blurred.
    if (pixels != m_requestedSize) {
        m_requestedSize = pixels;
        // GL_RGBA8: one byte per channel is all a finished picture needs, the map has
        // no colours brighter than white. No depth texture: the shapes are flat and
        // are drawn in the order of the list, later ones over earlier ones.
        m_target = gfx::Framebuffer({.width = pixels,
                                     .height = pixels,
                                     .color = gfx::ColorFormat::Rgba8,
                                     .depth = gfx::DepthFormat::None});
    }
    if (!m_target.isValid()) {
        return false;
    }

    // From here on the draw calls land in the texture of the minimap. bind() also sets
    // the viewport to its size.
    m_target.bind();

    // The order of the list decides what is on top, not a depth test: this framebuffer
    // has no depth to test against. No blending either: every shape is opaque and
    // replaces what is under it. The game never switches blending on, the line says
    // that the pass relies on it.
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glDisable(GL_BLEND));

    // The background: everything that is not discovered. An sRGB value, written as it
    // is, like the colours of the shapes (see post/minimap.frag). The clear colour is
    // a state of the context: the scene pass sets its own before it clears.
    GL_CHECK(glClearColor(MINIMAP_BACKGROUND_COLOR.r, MINIMAP_BACKGROUND_COLOR.g,
                          MINIMAP_BACKGROUND_COLOR.b, OPAQUE));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));

    // The uniforms belong to the program in use, so use() comes before the setter.
    shader.use();
    shader.setMat4(MINIMAP_MAP_TO_CLIP_UNIFORM, mapToClip);

    // The triangles of this frame replace the ones of the last frame in the buffer.
    // size_bytes() is the number of vertices times the size of one vertex.
    m_vertexArray.bind();
    m_vertexBuffer.setData(vertices.data(), vertices.size_bytes());
    // No index buffer: the list repeats the corners two triangles share, so the
    // vertices are drawn in the order they lie in, three per triangle.
    GL_CHECK(glDrawArrays(GL_TRIANGLES, FIRST_VERTEX, static_cast<GLsizei>(vertices.size())));
    return true;
}

void MinimapRenderer::drawOverlay(const gfx::Shader& shader, const MinimapRect& rect, float opacity,
                                  core::Size windowSize) const {
    // Back to the window, whatever follows: the debug UI is drawn after this pass and
    // must land there.
    gfx::Framebuffer::bindDefault(windowSize.width, windowSize.height);
    if (!m_target.isValid() || !shader.isValid() || rect.size < 1) {
        return;
    }

    // The viewport is the square of the minimap instead of the whole window. The
    // triangle of post/composite.vert covers "the whole target", which now is that
    // square: the picture lands in the corner without a mesh for a rectangle. All
    // three numbers are framebuffer pixels.
    GL_CHECK(glViewport(rect.x, rect.y, rect.size, rect.size));

    // The depth buffer of the window is never cleared (see PostProcess::composite), so
    // a test against it could reject the triangle.
    GL_CHECK(glDisable(GL_DEPTH_TEST));

    // Blending mixes what the shader writes with what is in the window already:
    // result = colour * alpha + window * (1 - alpha). The alpha is the opacity of the
    // map. It is switched on for this one draw call and put back afterwards, as
    // GrassRenderer does with back-face culling. The blend function is left set:
    // nothing else in the game blends, and the debug UI sets its own before it draws.
    GLboolean blendingWasOn = GL_FALSE;
    GL_CHECK(blendingWasOn = glIsEnabled(GL_BLEND));
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    shader.use();
    // The sampler gets the number of the texture unit, and the picture of the map is
    // bound to that unit. It is set in every frame: after a shader reload every
    // uniform is back at 0.
    shader.setInt(MINIMAP_OVERLAY_MAP_UNIFORM, static_cast<int>(MAP_TEXTURE_UNIT));
    shader.setFloat(MINIMAP_OVERLAY_OPACITY_UNIFORM,
                    std::clamp(opacity, MIN_MINIMAP_OPACITY, MAX_MINIMAP_OPACITY));
    // Reading the texture of the minimap is allowed here because the window is the
    // target: a pass must never read the texture it is drawing into.
    m_target.bindColorTexture(MAP_TEXTURE_UNIT);

    m_triangle.bind();
    // No buffer and no attribute: the vertex shader makes the corners from gl_VertexID.
    GL_CHECK(glDrawArrays(GL_TRIANGLES, FIRST_VERTEX, TRIANGLE_VERTEX_COUNT));

    if (blendingWasOn == GL_FALSE) {
        GL_CHECK(glDisable(GL_BLEND));
    }
    // The viewport over the whole window again, for the debug UI and the next frame.
    GL_CHECK(glViewport(0, 0, windowSize.width, windowSize.height));
}

} // namespace game
