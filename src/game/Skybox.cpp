// Skybox: the night sky, a cube map drawn behind everything else.
#include "game/Skybox.hpp"

#include "assets/ImageLoader.hpp"
#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "game/MazeRenderer.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Vertex.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace game {

namespace {

// The six pictures, relative to the assets directory, in the order gfx::Cubemap takes
// them: +X, -X, +Y, -Y, +Z, -Z. p stands for positive, n for negative.
constexpr std::array<const char*, gfx::Cubemap::FACE_COUNT> FACE_FILES = {
    "skybox/px.png", "skybox/nx.png", "skybox/py.png",
    "skybox/ny.png", "skybox/pz.png", "skybox/nz.png",
};

// The texture unit the cube map is bound to while the sky is drawn. Unit 0 is also the
// one the models use for their colour texture. That is fine: a unit has a separate
// binding for 2D textures and for cube maps, and every draw binds what it needs.
constexpr GLuint SKYBOX_TEXTURE_UNIT = 0;

constexpr std::size_t CORNER_COUNT = 8;
constexpr std::size_t FACE_COUNT = 6;
constexpr std::size_t INDICES_PER_FACE = 6; // two triangles

// The corners of a cube that reaches from -1 to 1 on every axis, with the camera in its
// middle. Only the position is filled in: the sky has no normal and no texture
// coordinate, the position itself is the direction the cube map is read with. The size
// of the cube does not matter: skybox.vert writes the depth of every vertex as 1.0
// (xyww), so no corner can end up in front of the near plane of the camera.
constexpr std::array<gfx::Vertex, CORNER_COUNT> CUBE_CORNERS = {
    gfx::Vertex{.position = {-1.0F, -1.0F, -1.0F}}, // 0
    gfx::Vertex{.position = {1.0F, -1.0F, -1.0F}},  // 1
    gfx::Vertex{.position = {1.0F, 1.0F, -1.0F}},   // 2
    gfx::Vertex{.position = {-1.0F, 1.0F, -1.0F}},  // 3
    gfx::Vertex{.position = {-1.0F, -1.0F, 1.0F}},  // 4
    gfx::Vertex{.position = {1.0F, -1.0F, 1.0F}},   // 5
    gfx::Vertex{.position = {1.0F, 1.0F, 1.0F}},    // 6
    gfx::Vertex{.position = {-1.0F, 1.0F, 1.0F}},   // 7
};

// Every three indices are one triangle, two triangles per face. The corners of each
// triangle go counter-clockwise when seen from INSIDE the cube, which is where the
// camera is. The game does not switch on face culling today. If it ever does, the sky
// stays visible, because its triangles face inwards.
constexpr std::array<std::uint32_t, FACE_COUNT * INDICES_PER_FACE> CUBE_TRIANGLES = {
    1, 5, 6, 1, 6, 2, // +X
    4, 0, 3, 4, 3, 7, // -X
    3, 2, 6, 3, 6, 7, // +Y
    0, 4, 5, 0, 5, 1, // -Y
    5, 4, 7, 5, 7, 6, // +Z
    0, 1, 2, 0, 2, 3, // -Z
};

// Loads the six pictures and makes the cube map from them. Returns a cube map that is
// not valid when a picture is missing or the six do not fit together.
gfx::Cubemap loadSkyCubemap() {
    std::array<assets::Image, gfx::Cubemap::FACE_COUNT> images;
    for (std::size_t face = 0; face < gfx::Cubemap::FACE_COUNT; ++face) {
        const std::filesystem::path path = core::assetPath(FACE_FILES[face]);
        std::string error;
        // RowOrder::TopFirst: no row flip. A face of a cube map has its top row first,
        // unlike a 2D texture (the reason is at assets::RowOrder).
        if (!assets::loadImage(path, images[face], error, assets::RowOrder::TopFirst)) {
            // loadImage has logged which file failed and why.
            return {};
        }
        core::logInfo("Loaded sky face: " + core::pathText(path));
    }

    // The faces of a cube map are squares of one size with one pixel format. The first
    // picture sets the size and the channel count, the others have to match it.
    const assets::Image& first = images[0];
    for (const assets::Image& image : images) {
        if (image.width != first.width || image.height != first.width ||
            image.channels != first.channels) {
            core::logError("The sky cannot be created: its six pictures must be squares of the "
                           "same size with the same number of channels");
            return {};
        }
    }

    gfx::Cubemap::FacePixels pixels{};
    for (std::size_t face = 0; face < gfx::Cubemap::FACE_COUNT; ++face) {
        pixels[face] = images[face].pixels.data();
    }
    // The graphics card takes its own copy: the six images are freed when this function
    // returns. The sky pictures are colours, painted for the screen, so they are sRGB:
    // the graphics card decodes them to linear values, which is what the HDR buffer
    // the scene is drawn into expects.
    return {first.width, first.channels, pixels, gfx::ColorSpace::Srgb};
}

} // namespace

Skybox::Skybox() : m_cubemap(loadSkyCubemap()), m_cube(CUBE_CORNERS, CUBE_TRIANGLES) {}

void Skybox::draw(const gfx::Shader& shader, const glm::mat4& view, const glm::mat4& projection,
                  const SkyboxSettings& settings, ViewMode viewMode) const {
    // Without the pictures or without the program there is no sky: the clear colour
    // stays. Both errors were logged once, when the objects were created.
    if (!m_cubemap.isValid() || !shader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    shader.use();
    // The view matrix goes in whole: the vertex shader removes its translation.
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
    shader.setFloat(SKYBOX_BRIGHTNESS_UNIFORM, settings.brightness);
    // The enum values are the numbers skybox.frag compares uViewMode with.
    shader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(viewMode));

    // The sampler of the shader gets the number of the texture unit (glUniform1i), and
    // the cube map is bound to that unit.
    shader.setInt(SKYBOX_UNIFORM, static_cast<int>(SKYBOX_TEXTURE_UNIT));
    m_cubemap.bind(SKYBOX_TEXTURE_UNIT);

    // Blend the texels of two neighbouring faces at the border between them. Without
    // this switch linear filtering stops at the edge of a face, and the borders can
    // show as thin lines. It is part of OpenGL since 3.2, is off by default and only
    // concerns cube maps, so it is left on.
    GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

    // The vertex shader gives every vertex of the sky the depth 1.0, the largest there
    // is and the very value the depth buffer is cleared to. The default test GL_LESS
    // ("nearer than what is there") would reject the sky everywhere, 1.0 is not less
    // than 1.0. GL_LEQUAL also lets a fragment through at the same depth. So the sky
    // passes on the pixels that still hold the cleared depth and fails wherever a wall,
    // the ground or a crystal was drawn, whose depth is smaller.
    GL_CHECK(glDepthFunc(GL_LEQUAL));
    // The sky must not write its depth: nothing is ever behind it.
    GL_CHECK(glDepthMask(GL_FALSE));

    m_cube.draw();

    // Back to the state the rest of the program draws with (the OpenGL defaults).
    GL_CHECK(glDepthMask(GL_TRUE));
    GL_CHECK(glDepthFunc(GL_LESS));
}

} // namespace game
