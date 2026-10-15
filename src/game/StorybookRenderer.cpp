// StorybookRenderer: loads the layers of the storybook intro and draws its pictures.
#include "game/StorybookRenderer.hpp"

#include "assets/ImageLoader.hpp"
#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "gfx/CoverFit.hpp"
#include "gfx/RenderStats.hpp"

#include <string>

namespace game {

namespace {

// The whole screen triangle of the post-processing passes and the shader of one layer.
constexpr const char* VERTEX_SHADER_FILE = "shaders/post/composite.vert";
constexpr const char* FRAGMENT_SHADER_FILE = "shaders/post/storybook_layer.frag";

constexpr const char* PICTURE_UNIFORM = "uPicture";
constexpr const char* SHOWN_LEFT_UNIFORM = "uShownLeft";
constexpr const char* SHOWN_RIGHT_UNIFORM = "uShownRight";
constexpr const char* SHOWN_TOP_UNIFORM = "uShownTop";
constexpr const char* SHOWN_BOTTOM_UNIFORM = "uShownBottom";
constexpr const char* OPACITY_UNIFORM = "uOpacity";

constexpr GLuint PICTURE_TEXTURE_UNIT = 0;
constexpr GLint FIRST_VERTEX = 0;
constexpr GLsizei TRIANGLE_VERTEX_COUNT = 3;

constexpr int RGB_CHANNELS = 3;
constexpr int RGBA_CHANNELS = 4;

} // namespace

StorybookRenderer::StorybookRenderer()
    : m_shader(core::assetPath(VERTEX_SHADER_FILE), core::assetPath(FRAGMENT_SHADER_FILE)) {}

bool StorybookRenderer::load(const Storybook& book, const std::filesystem::path& folder) {
    m_layers.clear();
    if (!m_shader.isValid()) {
        core::logError("Storybook: the shader of its layers cannot be loaded");
        return false;
    }
    for (const StorybookBeat& beat : book.beats) {
        std::vector<std::unique_ptr<gfx::FrameTexture>>& textures = m_layers.emplace_back();
        for (const StorybookLayer& layer : beat.layers) {
            const std::filesystem::path path = folder / layer.file;
            assets::Image image;
            std::string error;
            // Top row first, as the shader reads it (post/storybook_layer.frag).
            if (!assets::loadImage(path, image, error, assets::RowOrder::TopFirst)) {
                m_layers.clear();
                return false;
            }
            if (image.channels != RGB_CHANNELS && image.channels != RGBA_CHANNELS) {
                core::logError("Storybook: a layer must be an RGB or RGBA picture: " +
                               core::pathText(path));
                m_layers.clear();
                return false;
            }
            auto texture = std::make_unique<gfx::FrameTexture>(image.width, image.height);
            texture->upload(image.pixels.data(), image.channels == RGBA_CHANNELS
                                                     ? gfx::PixelOrder::Rgba
                                                     : gfx::PixelOrder::Rgb);
            textures.push_back(std::move(texture));
        }
    }
    return true;
}

void StorybookRenderer::draw(core::Size framebuffer, const Storybook& book,
                             const StorybookFrame& frame) const {
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glDisable(GL_FRAMEBUFFER_SRGB));
    GL_CHECK(glClearColor(0.0F, 0.0F, 0.0F, 1.0F));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    m_shader.use();
    m_shader.setInt(PICTURE_UNIFORM, static_cast<int>(PICTURE_TEXTURE_UNIT));
    m_vertexArray.bind();
    drawBeat(framebuffer, book, frame.beat, frame.progress, 1.0F);
    if (frame.nextOpacity > 0.0F && frame.beat + 1 < m_layers.size()) {
        drawBeat(framebuffer, book, frame.beat + 1, frame.nextProgress, frame.nextOpacity);
    }
    GL_CHECK(glDisable(GL_BLEND));
}

void StorybookRenderer::drawBeat(core::Size framebuffer, const Storybook& book, std::size_t beat,
                                 float progress, float opacity) const {
    if (beat >= m_layers.size() || beat >= book.beats.size()) {
        return;
    }
    const std::vector<StorybookLayer>& layers = book.beats.at(beat).layers;
    const std::vector<std::unique_ptr<gfx::FrameTexture>>& textures = m_layers.at(beat);
    for (std::size_t i = 0; i < textures.size() && i < layers.size(); ++i) {
        const gfx::FrameTexture& texture = *textures.at(i);
        const gfx::UvRect shown = storybookLayerUv(
            gfx::coverFit(texture.width(), texture.height(), framebuffer.width, framebuffer.height),
            layers.at(i), progress);
        texture.bind(PICTURE_TEXTURE_UNIT);
        m_shader.setFloat(SHOWN_LEFT_UNIFORM, shown.left);
        m_shader.setFloat(SHOWN_RIGHT_UNIFORM, shown.right);
        m_shader.setFloat(SHOWN_TOP_UNIFORM, shown.top);
        m_shader.setFloat(SHOWN_BOTTOM_UNIFORM, shown.bottom);
        m_shader.setFloat(OPACITY_UNIFORM, opacity);
        GL_CHECK(glDrawArrays(GL_TRIANGLES, FIRST_VERTEX, TRIANGLE_VERTEX_COUNT));
        gfx::countDraw(GL_TRIANGLES, TRIANGLE_VERTEX_COUNT);
    }
}

} // namespace game
