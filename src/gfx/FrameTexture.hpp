// FrameTexture: a texture for whole pictures that are shown as they are, and replaced
// often: the frames of a video, or one still picture in their place.
#pragma once

#include <glad/gl.h>

namespace gfx {

/// The order of the bytes of one pixel handed to FrameTexture::upload.
enum class PixelOrder {
    Bgra, ///< blue, green, red, one more byte: what a video decoder delivers
    Rgb,  ///< red, green, blue: a picture file without alpha
    Rgba, ///< red, green, blue, alpha: a picture file with alpha
};

/// Owns one OpenGL 2D texture of a fixed size without mipmaps.
///
/// It differs from gfx::Texture2D in what it is for. A Texture2D is the surface of
/// a model: it is uploaded once, has mipmaps, repeats and is decoded from sRGB. This
/// texture is a picture for the screen: its pixels are replaced up to 30 times per
/// second (glTexSubImage2D, no new storage and no mipmaps to build), it never repeats
/// (GL_CLAMP_TO_EDGE: no colour of the opposite edge bleeds in) and its bytes reach the
/// shader exactly as they were uploaded (GL_RGBA8, no sRGB decoding).
///
/// The constructor creates the texture, the destructor deletes it (RAII). It cannot be
/// copied or moved. It needs a current OpenGL context for its whole life.
class FrameTexture {
public:
    /// Creates the storage for a picture of width x height pixels. Its content is not
    /// defined until the first upload. With a size below 1 nothing is created and
    /// isValid() is false.
    FrameTexture(int width, int height);
    ~FrameTexture();

    FrameTexture(const FrameTexture&) = delete;
    FrameTexture& operator=(const FrameTexture&) = delete;

    bool isValid() const { return m_id != 0; }

    /// Replaces the whole picture. pixels points at width * height pixels in the given
    /// byte order, row by row without gaps. The row that comes first lands at texture
    /// coordinate v = 0: a picture stored top row first is read with v = 0 at its top.
    /// OpenGL takes its own copy of the bytes. The texture is left bound to
    /// GL_TEXTURE_2D of the texture unit that is active.
    void upload(const unsigned char* pixels, PixelOrder order);

    /// Makes texture unit number unit the active one and binds this texture to it.
    void bind(GLuint unit) const;

    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    // Name (id) of the OpenGL texture object. 0 is never a real texture: it means "none".
    GLuint m_id = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace gfx
