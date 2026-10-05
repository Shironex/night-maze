// Texture2D: a picture on the graphics card that a fragment shader can sample.
// See docs/modules/gfx/textures.md
#include "gfx/Texture2D.hpp"

#include "core/GlCheck.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <cstring>
#include <string>

namespace gfx {

namespace {

// The two channel counts the class accepts: red, green, blue, and the same with alpha.
constexpr int RGB_CHANNELS = 3;
constexpr int RGBA_CHANNELS = 4;

// Mipmap level 0 is the picture in its full size. The smaller levels are made from it.
constexpr GLint BASE_LEVEL = 0;

// Value of GL_UNPACK_ALIGNMENT that means "the rows follow each other without padding".
constexpr GLint TIGHT_ROW_ALIGNMENT = 1;

// Anisotropic filtering is not part of OpenGL 4.1 Core. It is the extension
// GL_EXT_texture_filter_anisotropic (it became core only in OpenGL 4.6, under the name
// GL_ARB_texture_filter_anisotropic and with the same numbers). The GLAD loader of this
// project was generated without extensions, so its header does not declare the two
// constants of the extension. They are plain numbers from the extension specification
// and are passed to core functions (glSamplerParameterf, glGetFloatv), so defining them
// here is all that is needed. They may be used only after the extension was found at
// runtime.
constexpr GLenum TEXTURE_MAX_ANISOTROPY = 0x84FE;     // GL_TEXTURE_MAX_ANISOTROPY_EXT
constexpr GLenum MAX_TEXTURE_MAX_ANISOTROPY = 0x84FF; // GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
constexpr const char* ANISOTROPY_EXTENSION_EXT = "GL_EXT_texture_filter_anisotropic";
constexpr const char* ANISOTROPY_EXTENSION_ARB = "GL_ARB_texture_filter_anisotropic";

// Anisotropy level 1 means "one sample", which is the same as no anisotropic filtering.
constexpr float NO_ANISOTROPY = 1.0F;

// True when the driver lists the anisotropic filtering extension under either name.
bool hasAnisotropicFiltering() {
    // In a Core profile the extensions are not one long string any more: the driver
    // reports how many there are and hands out their names one by one.
    GLint extensionCount = 0;
    GL_CHECK(glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount));

    for (GLint index = 0; index < extensionCount; ++index) {
        const GLubyte* bytes = nullptr;
        GL_CHECK(bytes = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(index)));
        if (bytes == nullptr) {
            continue;
        }
        // OpenGL returns text as unsigned bytes (GLubyte), the C string functions want
        // char. Both are one byte per character, so the cast only changes the type.
        const char* name = reinterpret_cast<const char*>(bytes);
        if (std::strcmp(name, ANISOTROPY_EXTENSION_EXT) == 0 ||
            std::strcmp(name, ANISOTROPY_EXTENSION_ARB) == 0) {
            return true;
        }
    }
    return false;
}

// Highest anisotropy level of the driver, or NO_ANISOTROPY when the extension is missing.
float queryMaxAnisotropy() {
    if (!hasAnisotropicFiltering()) {
        return NO_ANISOTROPY;
    }
    GLfloat maximum = NO_ANISOTROPY;
    GL_CHECK(glGetFloatv(MAX_TEXTURE_MAX_ANISOTROPY, &maximum));
    return maximum;
}

// Sets the two filter parameters of a sampler object.
void applyFilter(GLuint sampler, TextureFilter filter) {
    // Minification: one pixel covers many texels (the surface is far away). Only this
    // filter may name mipmaps. Magnification: one texel covers many pixels (the surface
    // is close). The largest level is the only one that can be used then, so its filter
    // is just "nearest" or "linear".
    GLint minification = GL_LINEAR_MIPMAP_LINEAR;
    GLint magnification = GL_LINEAR;
    switch (filter) {
    case TextureFilter::Nearest:
        minification = GL_NEAREST;
        magnification = GL_NEAREST;
        break;
    case TextureFilter::Bilinear:
        minification = GL_LINEAR;
        magnification = GL_LINEAR;
        break;
    case TextureFilter::Trilinear:
        // GL_LINEAR_MIPMAP_LINEAR: linear inside a level, linear between two levels.
        minification = GL_LINEAR_MIPMAP_LINEAR;
        magnification = GL_LINEAR;
        break;
    }
    // A sampler object is changed through its id: nothing has to be bound first.
    GL_CHECK(glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, minification));
    GL_CHECK(glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, magnification));
}

} // namespace

Texture2D::Texture2D(int width, int height, int channels, const unsigned char* pixels) {
    const bool channelsSupported = channels == RGB_CHANNELS || channels == RGBA_CHANNELS;
    if (width < 1 || height < 1 || !channelsSupported || pixels == nullptr) {
        core::logError("Texture2D cannot be created: it needs a size of at least 1 x 1, 3 or 4 "
                       "channels and pixel data, but got " +
                       std::to_string(width) + " x " + std::to_string(height) + " with " +
                       std::to_string(channels) + " channels");
        return;
    }
    m_width = width;
    m_height = height;

    // Two different questions. The data format says what the bytes in pixels are: three or
    // four of them per pixel. The internal format says how the graphics card should store
    // the texture: 8 bits per channel, with or without alpha. Here both match the data.
    const bool hasAlpha = channels == RGBA_CHANNELS;
    const GLenum dataFormat = hasAlpha ? GL_RGBA : GL_RGB;
    const GLint internalFormat = hasAlpha ? GL_RGBA8 : GL_RGB8;

    // glGenTextures writes new ids into an array. Here the array is the one member.
    GL_CHECK(glGenTextures(1, &m_id));
    // OpenGL 4.1 can only fill and configure the texture that is bound, so bind first.
    // The first binding also decides the kind of the texture: this one is 2D for good.
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_id));

    // By default OpenGL assumes that every row of pixels starts at an address divisible
    // by 4 and skips the bytes in between. Our rows have no such padding: a row of an RGB
    // picture is width * 3 bytes, which is divisible by 4 only for some widths. Alignment
    // 1 means "the rows are packed tightly". Without it a picture whose row size is not
    // a multiple of 4 comes out skewed, each row shifted against the one before. The
    // setting belongs to the whole context, not to the texture, so the previous value is
    // put back for the other code that uploads textures (Dear ImGui).
    GLint previousAlignment = TIGHT_ROW_ALIGNMENT;
    GL_CHECK(glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, TIGHT_ROW_ALIGNMENT));

    // Allocates level 0 on the graphics card and copies the pixels into it. The 0 after
    // the height is the border, a leftover of old OpenGL that must be 0. GL_UNSIGNED_BYTE:
    // every channel in pixels is one byte, 0 to 255. glTexImage2D is used and not
    // glTexStorage2D, which needs OpenGL 4.2 and does not exist on macOS.
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, BASE_LEVEL, internalFormat, width, height, 0, dataFormat,
                          GL_UNSIGNED_BYTE, pixels));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment));

    // Builds every smaller level from level 0: each one half as wide and half as high as
    // the one before, down to 1 x 1. They are always built, whatever the filter, so that
    // the filter can be changed later without touching the pixels again.
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

    // How the texture is read (filter, wrapping, anisotropy) is kept in a sampler object
    // of its own and not in the texture. Both places exist in OpenGL since 3.3, and a
    // sampler object bound to a texture unit wins over the parameters of the texture.
    // The reason is practical: on the Windows PC of this project the driver ignored the
    // anisotropy level set on the texture and obeyed the one set on a sampler object
    // (see the document). A sampler object also needs no binding to be changed.
    GL_CHECK(glGenSamplers(1, &m_sampler));

    // Coordinates outside 0..1 repeat the picture. S and T are the names OpenGL uses for
    // the two texture axes (u and v). The walls rely on it: their coordinates run past 1.
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_T, GL_REPEAT));
    applyFilter(m_sampler, m_filter);

    m_maxAnisotropy = queryMaxAnisotropy();
}

Texture2D::~Texture2D() {
    // OpenGL silently ignores the id 0 in both calls, so an object without a texture
    // (wrong arguments, or one that was moved from) needs no special case.
    GL_CHECK(glDeleteSamplers(1, &m_sampler));
    GL_CHECK(glDeleteTextures(1, &m_id));
}

// Move constructor: the new object takes both ids, and other gives them up.
Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_id(other.m_id),
      m_sampler(other.m_sampler),
      m_width(other.m_width),
      m_height(other.m_height),
      m_filter(other.m_filter),
      m_anisotropy(other.m_anisotropy),
      m_maxAnisotropy(other.m_maxAnisotropy) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
    other.m_sampler = 0;
}

// Move assignment: this object already owns a texture, which has to go first.
Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    // texture = std::move(texture): nothing to do. Without this check the texture would
    // be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the objects owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteSamplers(1, &m_sampler));
    GL_CHECK(glDeleteTextures(1, &m_id));

    m_id = other.m_id;
    m_sampler = other.m_sampler;
    m_width = other.m_width;
    m_height = other.m_height;
    m_filter = other.m_filter;
    m_anisotropy = other.m_anisotropy;
    m_maxAnisotropy = other.m_maxAnisotropy;
    other.m_id = 0;
    other.m_sampler = 0;
    return *this;
}

void Texture2D::bind(GLuint unit) const {
    // A context has many texture units, each with its own GL_TEXTURE_2D binding.
    // glBindTexture always works on the active unit, so the unit is chosen first. The
    // units are numbered by consecutive constants: GL_TEXTURE0 + 1 is GL_TEXTURE1.
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_id));
    // The sampler object goes to the same unit. glBindSampler takes the plain number of
    // the unit (0, 1, 2), not GL_TEXTURE0 + unit, and does not care which unit is active.
    GL_CHECK(glBindSampler(unit, m_sampler));
}

void Texture2D::setFilter(TextureFilter filter) {
    if (!isValid()) {
        return;
    }
    m_filter = filter;
    applyFilter(m_sampler, m_filter);
}

void Texture2D::setAnisotropy(float level) {
    // Without the extension the constant below means nothing to the driver and would
    // raise GL_INVALID_ENUM. m_maxAnisotropy is 1 in that case (and for a texture that is
    // not valid), and m_anisotropy stays 1.
    if (m_maxAnisotropy <= NO_ANISOTROPY) {
        return;
    }
    m_anisotropy = std::clamp(level, NO_ANISOTROPY, m_maxAnisotropy);
    // A float parameter, hence glSamplerParameterf and not glSamplerParameteri.
    GL_CHECK(glSamplerParameterf(m_sampler, TEXTURE_MAX_ANISOTROPY, m_anisotropy));
}

} // namespace gfx
