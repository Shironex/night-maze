// Framebuffer: a render target of our own, so a pass can draw into textures instead of
// the window.
// See docs/modules/gfx/framebuffers.md
#include "gfx/Framebuffer.hpp"

#include "core/GlCheck.hpp"
#include "core/Log.hpp"

#include <string>

namespace gfx {

namespace {

// The default framebuffer: the one of the window.
constexpr GLuint DEFAULT_FRAMEBUFFER = 0;

// "No sampler object": the texture bound to the unit is read with its own parameters.
constexpr GLuint NO_SAMPLER = 0;

// The attached textures have one level, the full size. A framebuffer draws into one
// level of a texture, and this is the one.
constexpr GLint BASE_LEVEL = 0;

// The three constants glTexImage2D needs for a format: how the card stores a pixel
// (internal), and what the data passed in would look like (format and type). No data is
// passed, the textures start empty, but the last two still have to be a pair OpenGL
// accepts for the internal format.
struct TextureFormat {
    GLint internal;
    GLenum format;
    GLenum type;
};

// Both colour formats are ones every OpenGL 4.1 driver can draw into (the specification
// lists them as required colour formats for render targets).
TextureFormat colorTextureFormat(ColorFormat format) {
    if (format == ColorFormat::Rgba16F) {
        return {.internal = GL_RGBA16F, .format = GL_RGBA, .type = GL_FLOAT};
    }
    return {.internal = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE};
}

// Creates an empty 2D texture of one level and leaves it bound to GL_TEXTURE_2D of the
// active unit. filter is GL_LINEAR or GL_NEAREST, for both directions.
GLuint createAttachmentTexture(const FramebufferSpec& spec, const TextureFormat& format,
                               GLint filter) {
    GLuint texture = 0;
    GL_CHECK(glGenTextures(1, &texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    // nullptr: allocate the memory, copy nothing. The pass that draws into the
    // framebuffer fills it. glTexImage2D and not glTexStorage2D, which needs OpenGL 4.2.
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, BASE_LEVEL, format.internal, spec.width, spec.height, 0,
                          format.format, format.type, nullptr));

    // The parameters are set on the texture itself, not on a sampler object: the
    // texture is read by passes that bind no sampler object (bindColorTexture).
    //
    // One level only. Without this line the texture would count as incomplete under
    // the default minification filter, which asks for mipmaps, and read as black.
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, BASE_LEVEL));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter));
    // A pass that reads next to the edge (a blur) must not get pixels of the opposite
    // edge, which is what the default GL_REPEAT would give it.
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    return texture;
}

} // namespace

const char* colorFormatName(ColorFormat format) {
    switch (format) {
    case ColorFormat::Rgba8:
        return "GL_RGBA8";
    case ColorFormat::Rgba16F:
        return "GL_RGBA16F";
    case ColorFormat::None:
        break;
    }
    return "none";
}

const char* depthFormatName(DepthFormat format) {
    return format == DepthFormat::Depth24 ? "GL_DEPTH_COMPONENT24" : "none";
}

const char* framebufferStatusText(GLenum status) {
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return "complete";
    case GL_FRAMEBUFFER_UNDEFINED:
        return "the default framebuffer does not exist";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "an attachment is incomplete (a texture of size 0 or of a format that cannot "
               "be drawn into)";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "nothing is attached";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "the draw buffer names a colour attachment that does not exist";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "the read buffer names a colour attachment that does not exist";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "the driver does not support this combination of formats";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "the attachments differ in their number of samples";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "the attachments differ in being layered";
    default:
        return "unknown status";
    }
}

Framebuffer::Framebuffer(const FramebufferSpec& spec) : m_spec(spec) {
    const bool hasAttachment = spec.color != ColorFormat::None || spec.depth != DepthFormat::None;
    if (spec.width < 1 || spec.height < 1 || !hasAttachment) {
        core::logError("Framebuffer cannot be created: it needs a size of at least 1 x 1 and "
                       "a colour or a depth attachment, but got " +
                       std::to_string(spec.width) + " x " + std::to_string(spec.height));
        m_spec = {};
        return;
    }
    create();
}

Framebuffer::~Framebuffer() {
    release();
}

// Move constructor: the new object takes the ids, and other gives them up.
Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_id(other.m_id),
      m_colorTexture(other.m_colorTexture),
      m_depthTexture(other.m_depthTexture),
      m_spec(other.m_spec) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_id = 0;
    other.m_colorTexture = 0;
    other.m_depthTexture = 0;
    other.m_spec = {};
}

// Move assignment: this object may already own a framebuffer, which has to go first.
Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    // framebuffer = std::move(framebuffer): nothing to do.
    if (this == &other) {
        return *this;
    }

    release();
    m_id = other.m_id;
    m_colorTexture = other.m_colorTexture;
    m_depthTexture = other.m_depthTexture;
    m_spec = other.m_spec;
    other.m_id = 0;
    other.m_colorTexture = 0;
    other.m_depthTexture = 0;
    other.m_spec = {};
    return *this;
}

void Framebuffer::create() {
    GL_CHECK(glGenFramebuffers(1, &m_id));
    // Attaching works on the framebuffer that is bound, like filling a texture.
    // GL_FRAMEBUFFER binds it for drawing and for reading at once.
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_id));

    if (m_spec.color != ColorFormat::None) {
        // Linear filter: a pass that reads the picture at another size (half
        // resolution, a preview) gets blended pixels instead of blocks.
        m_colorTexture =
            createAttachmentTexture(m_spec, colorTextureFormat(m_spec.color), GL_LINEAR);
        // The texture becomes colour output number 0 of the framebuffer: what a fragment
        // shader writes to its first "out" variable lands in it.
        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                        m_colorTexture, BASE_LEVEL));
    } else {
        // A framebuffer expects a colour output by default. Without a colour texture it
        // has to be told that there is none to draw to and none to read from, or some
        // drivers report it as incomplete.
        GL_CHECK(glDrawBuffer(GL_NONE));
        GL_CHECK(glReadBuffer(GL_NONE));
    }

    if (m_spec.depth != DepthFormat::None) {
        // A depth texture is filled with one number per pixel: GL_DEPTH_COMPONENT.
        // Nearest filter: the average of the depths of two surfaces is the depth of
        // neither.
        const TextureFormat depthFormat = {
            .internal = GL_DEPTH_COMPONENT24, .format = GL_DEPTH_COMPONENT, .type = GL_FLOAT};
        m_depthTexture = createAttachmentTexture(m_spec, depthFormat, GL_NEAREST);
        // The depth test of everything drawn into this framebuffer uses this texture.
        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                        m_depthTexture, BASE_LEVEL));
    }

    // The driver decides whether it can draw into this combination. Asking is the only
    // way to know: drawing into an incomplete framebuffer is an error and draws nothing.
    GLenum status = GL_FRAMEBUFFER_COMPLETE;
    GL_CHECK(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER));

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        core::logError("Framebuffer of " + std::to_string(m_spec.width) + " x " +
                       std::to_string(m_spec.height) + " (" + colorFormatName(m_spec.color) + ", " +
                       depthFormatName(m_spec.depth) +
                       ") is not complete: " + framebufferStatusText(status));
        release();
        m_spec = {};
    }
}

void Framebuffer::release() {
    // OpenGL silently ignores the id 0 in all three calls, so an object without
    // a framebuffer or without one of the textures needs no special case.
    GL_CHECK(glDeleteFramebuffers(1, &m_id));
    GL_CHECK(glDeleteTextures(1, &m_colorTexture));
    GL_CHECK(glDeleteTextures(1, &m_depthTexture));
    m_id = 0;
    m_colorTexture = 0;
    m_depthTexture = 0;
}

void Framebuffer::bind() const {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, m_id));
    GL_CHECK(glViewport(0, 0, m_spec.width, m_spec.height));
}

void Framebuffer::bindDefault(int width, int height) {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER));
    GL_CHECK(glViewport(0, 0, width, height));
}

void Framebuffer::resize(int width, int height) {
    const bool sameSize = width == m_spec.width && height == m_spec.height;
    if (!isValid() || sameSize || width < 1 || height < 1) {
        return;
    }
    // The formats stay, only the size changes. A texture cannot be given another size
    // and keep its place in the framebuffer, so everything is built again.
    const FramebufferSpec spec = {
        .width = width, .height = height, .color = m_spec.color, .depth = m_spec.depth};
    release();
    m_spec = spec;
    create();
}

void Framebuffer::bindColorTexture(GLuint unit) const {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_colorTexture));
    GL_CHECK(glBindSampler(unit, NO_SAMPLER));
}

void Framebuffer::bindDepthTexture(GLuint unit) const {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, m_depthTexture));
    GL_CHECK(glBindSampler(unit, NO_SAMPLER));
}

} // namespace gfx
