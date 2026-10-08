// Cubemap: six square pictures on the graphics card, sampled with a direction.
#include "gfx/Cubemap.hpp"

#include "core/GlCheck.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <string>

namespace gfx {

namespace {

// The two channel counts the class accepts: red, green, blue, and the same with alpha.
constexpr int RGB_CHANNELS = 3;
constexpr int RGBA_CHANNELS = 4;

// Mipmap level 0 is the picture in its full size. A cube map of this class has no other.
constexpr GLint BASE_LEVEL = 0;

// Value of GL_UNPACK_ALIGNMENT that means "the rows follow each other without padding".
constexpr GLint TIGHT_ROW_ALIGNMENT = 1;

} // namespace

Cubemap::Cubemap(int size, int channels, const FacePixels& faces, ColorSpace colorSpace) {
    const bool channelsSupported = channels == RGB_CHANNELS || channels == RGBA_CHANNELS;
    // any_of asks the question "is there a face without pixels" of all six entries.
    const bool faceMissing =
        std::ranges::any_of(faces, [](const unsigned char* pixels) { return pixels == nullptr; });
    if (size < 1 || !channelsSupported || faceMissing) {
        core::logError("Cubemap cannot be created: it needs a face size of at least 1, 3 or 4 "
                       "channels and pixel data for all six faces, but got a size of " +
                       std::to_string(size) + " with " + std::to_string(channels) + " channels");
        return;
    }
    m_size = size;

    // The data format says what the bytes are, the internal format how the graphics card
    // stores them and what it does when a shader reads them (see Texture2D.cpp). With
    // ColorSpace::Srgb the faces get an sRGB format and are decoded to linear values on
    // reading, like the colour textures of the models.
    const bool hasAlpha = channels == RGBA_CHANNELS;
    const GLenum dataFormat = hasAlpha ? GL_RGBA : GL_RGB;
    GLint internalFormat = hasAlpha ? GL_RGBA8 : GL_RGB8;
    if (colorSpace == ColorSpace::Srgb) {
        internalFormat = hasAlpha ? GL_SRGB8_ALPHA8 : GL_SRGB8;
    }

    GL_CHECK(glGenTextures(1, &m_id));
    // The first binding decides the kind of the texture: this one is a cube map for good.
    // A cube map has a binding of its own on every texture unit, next to GL_TEXTURE_2D.
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, m_id));

    // The rows of the faces are packed tightly, like in Texture2D. The setting belongs to
    // the whole context, so the previous value is put back after the uploads.
    GLint previousAlignment = TIGHT_ROW_ALIGNMENT;
    GL_CHECK(glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment));
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, TIGHT_ROW_ALIGNMENT));

    // One glTexImage2D call per face. The cube map itself is the bound texture, and the
    // first argument names the face to fill. The six face constants are consecutive
    // numbers in the order +X, -X, +Y, -Y, +Z, -Z, so GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1
    // is GL_TEXTURE_CUBE_MAP_NEGATIVE_X, and so on: the same order as FacePixels. All
    // faces must be square and of the same size, or the cube map cannot be sampled.
    for (std::size_t face = 0; face < FACE_COUNT; ++face) {
        const GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face);
        GL_CHECK(glTexImage2D(target, BASE_LEVEL, internalFormat, size, size, 0, dataFormat,
                              GL_UNSIGNED_BYTE, faces[face]));
    }
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment));

    // Tells OpenGL that level 0 is the only level there is. Which levels exist is
    // a property of the texture itself, not of a sampler object. With it the cube map
    // is complete (readable) under any filter, also one that asks for mipmaps.
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, BASE_LEVEL));

    // How the cube map is read is kept in a sampler object, as for Texture2D. Here it is
    // also a matter of correctness: a sampler object bound to a texture unit applies to
    // every texture read through that unit. Without one of its own the cube map would be
    // read with the sampler a Texture2D left on the unit: its filter, and GL_REPEAT
    // instead of the clamping below.
    GL_CHECK(glGenSamplers(1, &m_sampler));

    // Linear filtering without mipmaps: the sky is seen at about its own resolution and
    // never from far away, so smaller levels would not be used.
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    // Clamp to the edge on all three axes. S and T are the two axes inside a face, and
    // R is the third coordinate of the direction: a cube map is addressed with three.
    // With GL_REPEAT, a direction that hits the very edge of a face would blend in
    // texels from the opposite edge of the same face, which shows as a line.
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
}

Cubemap::~Cubemap() {
    // OpenGL silently ignores the id 0 in both calls, so an object without a texture
    // (default constructed, wrong arguments, or moved from) needs no special case.
    GL_CHECK(glDeleteSamplers(1, &m_sampler));
    GL_CHECK(glDeleteTextures(1, &m_id));
}

// Move constructor: the new object takes both ids, and other gives them up.
Cubemap::Cubemap(Cubemap&& other) noexcept
    : m_id(other.m_id), m_sampler(other.m_sampler), m_size(other.m_size) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
    other.m_sampler = 0;
    other.m_size = 0;
}

// Move assignment: this object may already own a texture, which has to go first.
Cubemap& Cubemap::operator=(Cubemap&& other) noexcept {
    // cubemap = std::move(cubemap): nothing to do. Without this check the texture would
    // be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the objects owned so far (ignored by OpenGL when the id is 0).
    GL_CHECK(glDeleteSamplers(1, &m_sampler));
    GL_CHECK(glDeleteTextures(1, &m_id));

    m_id = other.m_id;
    m_sampler = other.m_sampler;
    m_size = other.m_size;
    other.m_id = 0;
    other.m_sampler = 0;
    other.m_size = 0;
    return *this;
}

void Cubemap::bind(GLuint unit) const {
    // The same three steps as Texture2D::bind, with the cube map binding of the unit.
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, m_id));
    // glBindSampler takes the plain number of the unit (0, 1, 2), not GL_TEXTURE0 + unit.
    GL_CHECK(glBindSampler(unit, m_sampler));
}

} // namespace gfx
