// FrameTexture: a texture for whole pictures that are shown as they are, and replaced
// often: the frames of a video, or one still picture in their place.
#include "gfx/FrameTexture.hpp"

#include "core/GlCheck.hpp"

namespace gfx {

namespace {

// The texture has one level: the picture in its full size.
constexpr GLint BASE_LEVEL = 0;

// Value of GL_UNPACK_ALIGNMENT that means "the rows follow each other without padding".
constexpr GLint TIGHT_ROW_ALIGNMENT = 1;

// No sampler object: the filter and the wrapping of the texture itself are used.
constexpr GLuint NO_SAMPLER = 0;

// What the bytes of a pixel are, in the words of OpenGL.
GLenum dataFormatFor(PixelOrder order) {
    switch (order) {
    case PixelOrder::Bgra:
        return GL_BGRA;
    case PixelOrder::Rgb:
        return GL_RGB;
    case PixelOrder::Rgba:
        return GL_RGBA;
    }
    return GL_RGBA;
}

} // namespace

FrameTexture::FrameTexture(int width, int height) {
    if (width < 1 || height < 1) {
        return;
    }
    m_width = width;
    m_height = height;

    GL_CHECK(glGenTextures(1, &m_id));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_id));
    // Storage without content (the pointer is null): every upload fills it. GL_RGBA8:
    // the bytes are stored as they come and read back as they are. Not GL_SRGB8_ALPHA8:
    // the pictures shown through this texture are already encoded for the screen, and
    // the shader that draws them writes them out unchanged.
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, BASE_LEVEL, GL_RGBA8, width, height, 0, GL_RGBA,
                          GL_UNSIGNED_BYTE, nullptr));
    // GL_LINEAR in both directions and no mipmaps: the picture is scaled to the window
    // by blending the four nearest texels. A texture without mipmaps must not have
    // a minification filter that names them (the default does), or it reads as black.
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    // At the edges the last texel is repeated, so the blend never reaches across to the
    // opposite edge of the picture.
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

FrameTexture::~FrameTexture() {
    // OpenGL silently ignores the id 0, so an object without a texture needs no
    // special case.
    GL_CHECK(glDeleteTextures(1, &m_id));
}

void FrameTexture::upload(const unsigned char* pixels, PixelOrder order) {
    if (!isValid() || pixels == nullptr) {
        return;
    }
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_id));

    // The rows are packed tightly, also for three bytes per pixel (see Texture2D.cpp).
    // The setting belongs to the whole context, so the previous value is put back.
    GLint previousAlignment = TIGHT_ROW_ALIGNMENT;
    GL_CHECK(glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, TIGHT_ROW_ALIGNMENT));
    // glTexSubImage2D replaces pixels of the storage that exists: the rectangle from
    // (0, 0) with the full size, which is all of it.
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, BASE_LEVEL, 0, 0, m_width, m_height,
                             dataFormatFor(order), GL_UNSIGNED_BYTE, pixels));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment));
}

void FrameTexture::bind(GLuint unit) const {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_id));
    // A gfx::Texture2D that was bound to this unit before has left its sampler object
    // there, and a sampler object wins over the parameters of the texture: the picture
    // would repeat and look for mipmaps it does not have. So the unit is cleared.
    GL_CHECK(glBindSampler(unit, NO_SAMPLER));
}

} // namespace gfx
