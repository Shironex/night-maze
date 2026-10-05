// Framebuffer: a render target of our own, so a pass can draw into textures instead of
// the window.
// See docs/modules/gfx/framebuffers.md
#pragma once

#include <glad/gl.h>

namespace gfx {

/// What the colour texture of a framebuffer stores per pixel.
enum class ColorFormat {
    /// No colour texture at all: a target that only records depth (a shadow map).
    None,
    /// GL_RGBA8: one byte per channel, values from 0 to 1. For pictures that are final,
    /// like a preview or a map.
    Rgba8,
    /// GL_RGBA16F: a 16 bit floating point number per channel. Values are not cut off at
    /// 1, so a bright light keeps how much brighter than white it is (HDR, high dynamic
    /// range), and dark tones keep far more steps than a byte has.
    Rgba16F,
};

/// What the depth texture of a framebuffer stores per pixel.
enum class DepthFormat {
    /// No depth texture: a target for passes that draw one flat triangle over
    /// everything and need no depth test (post-processing).
    None,
    /// GL_DEPTH_COMPONENT24: 24 bits of depth, the usual precision of a window.
    Depth24,
};

/// The size and the two formats of a framebuffer. At least one of the formats must be
/// something other than None.
struct FramebufferSpec {
    /// Size of both textures in pixels.
    int width = 0;
    int height = 0;
    ColorFormat color = ColorFormat::None;
    DepthFormat depth = DepthFormat::None;
};

/// The OpenGL name of a colour format as text ("GL_RGBA16F"), for the debug UI.
const char* colorFormatName(ColorFormat format);

/// The OpenGL name of a depth format as text ("GL_DEPTH_COMPONENT24"), for the debug UI.
const char* depthFormatName(DepthFormat format);

/// What a result of glCheckFramebufferStatus means, as a sentence for the log. status is
/// GL_FRAMEBUFFER_COMPLETE or one of the GL_FRAMEBUFFER_INCOMPLETE_... constants. It
/// calls no OpenGL function, so tests can use it.
const char* framebufferStatusText(GLenum status);

/// Owns one OpenGL framebuffer object and the textures attached to it: at most one
/// colour texture and at most one depth texture.
///
/// The window has a framebuffer of its own, the default framebuffer (number 0), and
/// everything drawn lands there. With an object of this class bound instead, the same
/// draw calls fill its textures, and a later pass can read those textures like any
/// other: that is offscreen rendering. The depth is a TEXTURE too, not a renderbuffer,
/// so that a later pass can read the depth of the scene (fog, shadow maps).
///
/// The constructor creates the objects, the destructor deletes them (RAII). The object
/// can be moved but not copied: a copy would hold the same ids and delete them a second
/// time. It needs a current OpenGL context for its whole life, so it must be destroyed
/// before the window.
class Framebuffer {
public:
    /// An object without a framebuffer: isValid() returns false. For a member that is
    /// created later, when the size is known.
    Framebuffer() = default;

    /// Creates the framebuffer and its textures and checks that OpenGL can draw into it
    /// (glCheckFramebufferStatus).
    ///
    /// It does not throw. When the spec is wrong (a size below 1, no attachment at all)
    /// or the driver reports the framebuffer as incomplete, the error is logged with
    /// the reason, nothing is kept and isValid() returns false.
    ///
    /// It leaves the DEFAULT framebuffer bound, whatever was bound before, and the
    /// texture binding of the active texture unit changed.
    explicit Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    /// Takes over the framebuffer of other. other is left without one (not valid).
    Framebuffer(Framebuffer&& other) noexcept;
    /// Deletes the framebuffer this object owns, then takes over the one of other.
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    /// True when the object owns a complete framebuffer.
    bool isValid() const { return m_id != 0; }

    /// Makes this framebuffer the target of everything drawn from now on, and sets the
    /// viewport to its whole size. The viewport belongs to the context and not to the
    /// framebuffer, so a target of another size than the window needs it set every time.
    void bind() const;

    /// Makes the default framebuffer (the window) the target again, with a viewport of
    /// width x height pixels: the framebuffer size of the window, not its size in
    /// screen coordinates.
    static void bindDefault(int width, int height);

    /// Gives the framebuffer another size. The textures cannot grow: they are deleted
    /// and created again, empty, with the same formats. Nothing happens when the size
    /// is the one it has. A size below 1 is ignored too (a minimised window reports
    /// 0 x 0): the framebuffer keeps its old size. Like the constructor it leaves the
    /// default framebuffer bound.
    void resize(int width, int height);

    /// Makes texture unit number unit the active one and binds the colour texture to
    /// it, for a sampler2D uniform set to the same number. The sampler object of the
    /// unit is unbound (glBindSampler with 0): a Texture2D drawn earlier leaves its own
    /// there, with GL_REPEAT and mipmaps, and this texture must be read with its own
    /// parameters (linear filter, clamped to the edge, one level).
    void bindColorTexture(GLuint unit) const;

    /// The same for the depth texture. A shader reads it through a sampler2D as one
    /// number in the red channel, from 0 (near plane) to 1 (far plane). Its filter is
    /// GL_NEAREST: depths of two surfaces must not be averaged.
    void bindDepthTexture(GLuint unit) const;

    /// Name (id) of the colour texture, for code that needs the raw number (the preview
    /// in the debug UI). 0 when there is none.
    GLuint colorTextureId() const { return m_colorTexture; }

    /// Name (id) of the depth texture. 0 when there is none.
    GLuint depthTextureId() const { return m_depthTexture; }

    /// Size and formats in use. The size is 0 x 0 for an object that is not valid.
    int width() const { return m_spec.width; }
    int height() const { return m_spec.height; }
    ColorFormat colorFormat() const { return m_spec.color; }
    DepthFormat depthFormat() const { return m_spec.depth; }

private:
    /// Creates the framebuffer object and the textures described by m_spec and checks
    /// completeness. On failure everything is released again and m_spec is reset.
    void create();
    /// Deletes the framebuffer object and both textures and sets the ids to 0.
    void release();

    // Name (id) of the OpenGL framebuffer object. 0 is the default framebuffer, so here
    // it means "none".
    GLuint m_id = 0;
    // Names (ids) of the two attached textures. 0 means "no such attachment".
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;
    FramebufferSpec m_spec;
};

} // namespace gfx
