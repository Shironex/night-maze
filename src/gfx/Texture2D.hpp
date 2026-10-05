// Texture2D: a picture on the graphics card that a fragment shader can sample.
// See docs/modules/gfx/textures.md
#pragma once

#include "gfx/ColorSpace.hpp"

#include <glad/gl.h>

namespace gfx {

/// How a texture is sampled when one of its texels does not cover exactly one pixel.
enum class TextureFilter {
    /// The one nearest texel, no mipmaps: sharp squares up close, shimmering far away.
    Nearest,
    /// Weighted average of the 4 nearest texels, no mipmaps: smooth up close, still
    /// shimmering far away.
    Bilinear,
    /// Bilinear inside the two nearest mipmap levels, then a blend of the two results.
    Trilinear,
};

/// Owns one OpenGL 2D texture with a full chain of mipmaps, and the sampler object that
/// says how the texture is read: filter, wrapping and anisotropy.
///
/// The constructor creates both objects and uploads the pixels, the destructor deletes
/// them (RAII). The object can be moved but not copied: a copy would hold the same ids
/// and delete them a second time. It needs a current OpenGL context for its whole life,
/// so it must be destroyed before the window.
class Texture2D {
public:
    /// Creates a texture of width x height pixels from raw bytes and builds its mipmaps.
    /// channels is 3 (red, green, blue) or 4 (with alpha), one byte each. pixels points at
    /// width * height * channels bytes, row by row without gaps, BOTTOM row first.
    /// OpenGL takes its own copy, so the bytes may be freed right after the call.
    ///
    /// colorSpace says what the bytes mean, and the caller has to know it: the class
    /// does not guess from a file name. ColorSpace::Srgb is for colour pictures: the
    /// texture gets an sRGB internal format (GL_SRGB8 or GL_SRGB8_ALPHA8) and the
    /// graphics card turns every texel into a linear value when a shader reads it
    /// (before filtering on today's cards: OpenGL 4.1 recommends that order and does
    /// not demand it). ColorSpace::Linear is for data that is not a colour, like the
    /// directions of a normal map: GL_RGB8 or GL_RGBA8, the numbers arrive unchanged.
    /// Alpha is never sRGB encoded, in either case.
    ///
    /// Sampling starts as trilinear filtering without anisotropy, and coordinates outside
    /// 0..1 repeat the picture (GL_REPEAT).
    ///
    /// It does not throw. When an argument is wrong (a size below 1, another channel
    /// count, no pixels) the error is logged, nothing is created and isValid() returns
    /// false.
    ///
    /// The texture is left bound to GL_TEXTURE_2D of the texture unit that is active.
    Texture2D(int width, int height, int channels, const unsigned char* pixels,
              ColorSpace colorSpace);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    /// Takes over the texture of other. other is left without a texture (not valid).
    Texture2D(Texture2D&& other) noexcept;
    /// Deletes the texture this object owns, then takes over the texture of other.
    Texture2D& operator=(Texture2D&& other) noexcept;

    /// True when the object owns a texture.
    bool isValid() const { return m_id != 0; }

    /// Makes texture unit number unit the active one (glActiveTexture) and binds this
    /// texture and its sampler object to it. A sampler2D uniform set to the same number
    /// (Shader::setInt) then reads this texture. unit counts from 0.
    /// The sampler object stays bound to the unit until another Texture2D is bound there.
    void bind(GLuint unit) const;

    /// Changes how the texture is filtered. It can be called at any time, bound or not:
    /// the mipmaps are always there, the filter only decides whether they are used.
    void setFilter(TextureFilter filter);

    /// Sets the level of anisotropic filtering: 1 switches it off, higher values keep the
    /// texture sharp on surfaces seen at a flat angle. The level is clamped to the range
    /// from 1 to maxAnisotropy(), and nothing happens when the graphics driver does not
    /// offer anisotropic filtering. It can be called at any time, bound or not.
    void setAnisotropy(float level);

    /// Filter in use, as set by the constructor or by setFilter.
    TextureFilter filter() const { return m_filter; }

    /// Anisotropy level in use, after clamping. 1 means off.
    float anisotropy() const { return m_anisotropy; }

    /// Highest anisotropy level the driver accepts (often 16). 1 when the driver does not
    /// offer anisotropic filtering, or when the texture is not valid.
    float maxAnisotropy() const { return m_maxAnisotropy; }

    /// Name (id) of the OpenGL texture object, for code that needs the raw number (the
    /// preview in the debug UI). 0 when the texture is not valid. Code that binds this id
    /// itself reads the texture without the sampler object, so without the filter and the
    /// anisotropy chosen here.
    GLuint id() const { return m_id; }

    /// Width of the largest mipmap level in pixels.
    int width() const { return m_width; }

    /// Height of the largest mipmap level in pixels.
    int height() const { return m_height; }

    /// What the bytes of the texture mean, as given to the constructor.
    ColorSpace colorSpace() const { return m_colorSpace; }

private:
    // Name (id) of the OpenGL texture object. 0 is never a real texture: it means "none".
    GLuint m_id = 0;
    // Name (id) of the OpenGL sampler object that holds the filter, the wrapping and the
    // anisotropy of this texture. 0 means "none".
    GLuint m_sampler = 0;
    int m_width = 0;
    int m_height = 0;
    ColorSpace m_colorSpace = ColorSpace::Linear;
    TextureFilter m_filter = TextureFilter::Trilinear;
    float m_anisotropy = 1.0F;
    // Asked from the driver once, in the constructor. 1 means "not supported".
    float m_maxAnisotropy = 1.0F;
};

} // namespace gfx
