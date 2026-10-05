// Cubemap: six square pictures on the graphics card, sampled with a direction.
// See docs/modules/gfx/cubemap.md
#pragma once

#include "gfx/ColorSpace.hpp"

#include <glad/gl.h>

#include <array>
#include <cstddef>

namespace gfx {

/// Owns one OpenGL cube map texture and the sampler object that says how it is read.
///
/// A cube map is six square pictures of the same size, the six faces of a cube around
/// the origin. A shader reads it through a samplerCube with a direction (a vec3): the
/// graphics card picks the face the direction points at and the texel it passes through.
///
/// The constructor creates both objects and uploads the pixels, the destructor deletes
/// them (RAII). The object can be moved but not copied: a copy would hold the same ids
/// and delete them a second time. It needs a current OpenGL context for its whole life,
/// so it must be destroyed before the window.
class Cubemap {
public:
    /// A cube has six faces.
    static constexpr std::size_t FACE_COUNT = 6;

    /// The pixels of the six faces, in the order OpenGL numbers them:
    /// +X, -X, +Y, -Y, +Z, -Z (right, left, top, bottom, back, front for a camera that
    /// looks along -Z).
    using FacePixels = std::array<const unsigned char*, FACE_COUNT>;

    /// An object without a texture: isValid() returns false. For the code that could
    /// not load its pictures and still has to return a Cubemap.
    Cubemap() = default;

    /// Creates a cube map whose faces are size x size pixels. channels is 3 (red, green,
    /// blue) or 4 (with alpha), one byte each. Every entry of faces points at
    /// size * size * channels bytes, row by row without gaps, TOP row first.
    ///
    /// The top row comes first here, while Texture2D wants the bottom row first: on
    /// a face of a cube map the texture coordinate t = 0 is the top of the picture
    /// (see assets::RowOrder). OpenGL takes its own copy, so the bytes may be freed right
    /// after the call.
    ///
    /// colorSpace says what the bytes mean, as for Texture2D: ColorSpace::Srgb for
    /// colour pictures like a sky (GL_SRGB8 or GL_SRGB8_ALPHA8, decoded to linear values
    /// on reading), ColorSpace::Linear for data that must arrive unchanged.
    ///
    /// Sampling is linear without mipmaps, and coordinates are clamped to the edge on
    /// all three axes (GL_CLAMP_TO_EDGE).
    ///
    /// It does not throw. When an argument is wrong (a size below 1, another channel
    /// count, a face without pixels) the error is logged, nothing is created and
    /// isValid() returns false.
    ///
    /// The texture is left bound to GL_TEXTURE_CUBE_MAP of the texture unit that is
    /// active.
    Cubemap(int size, int channels, const FacePixels& faces, ColorSpace colorSpace);
    ~Cubemap();

    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    /// Takes over the texture of other. other is left without a texture (not valid).
    Cubemap(Cubemap&& other) noexcept;
    /// Deletes the texture this object owns, then takes over the texture of other.
    Cubemap& operator=(Cubemap&& other) noexcept;

    /// True when the object owns a texture.
    bool isValid() const { return m_id != 0; }

    /// Makes texture unit number unit the active one (glActiveTexture) and binds this
    /// cube map and its sampler object to it. A samplerCube uniform set to the same
    /// number (Shader::setInt) then reads this cube map. unit counts from 0.
    ///
    /// A texture unit has one binding per kind of texture, so a 2D texture bound to the
    /// same unit stays bound. The sampler object, however, belongs to the unit as
    /// a whole: it stays there until a Texture2D or another Cubemap is bound to the unit.
    void bind(GLuint unit) const;

    /// Name (id) of the OpenGL texture object. 0 when the cube map is not valid.
    GLuint id() const { return m_id; }

    /// Width and height of every face in pixels. 0 when the cube map is not valid.
    int size() const { return m_size; }

private:
    // Name (id) of the OpenGL texture object. 0 is never a real texture: it means "none".
    GLuint m_id = 0;
    // Name (id) of the OpenGL sampler object that holds the filter and the wrapping of
    // this cube map. 0 means "none".
    GLuint m_sampler = 0;
    int m_size = 0;
};

} // namespace gfx
