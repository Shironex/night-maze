// RawTextureSampler: lets the debug UI show an sRGB texture as the picture in its file.
#include "debug/RawTextureSampler.hpp"

#include "core/GlCheck.hpp"
#include "gfx/Extensions.hpp"

#include <imgui.h>

namespace debug {

namespace {

// The extension and its two constants. The GLAD loader of this project was generated
// without extensions, so its header does not declare them. They are plain numbers from
// the extension specification and are passed to a core function (glSamplerParameteri).
// They may be used only after the extension was found at runtime.
constexpr const char* SRGB_DECODE_EXTENSION = "GL_EXT_texture_sRGB_decode";
constexpr GLenum TEXTURE_SRGB_DECODE = 0x8A48; // GL_TEXTURE_SRGB_DECODE_EXT
constexpr GLint SKIP_DECODE = 0x8A4A;          // GL_SKIP_DECODE_EXT

// The texture unit the OpenGL backend of ImGui draws its pictures with.
constexpr GLuint IMGUI_TEXTURE_UNIT = 0;

// Runs in the middle of the drawing of ImGui, at the place in its list of draw commands
// where begin() added it. The data of the command is the copy of the sampler id that
// begin() handed over.
void bindSampler(const ImDrawList* /*drawList*/, const ImDrawCmd* command) {
    const GLuint sampler = *static_cast<const GLuint*>(command->UserCallbackData);
    GL_CHECK(glBindSampler(IMGUI_TEXTURE_UNIT, sampler));
}

} // namespace

RawTextureSampler::RawTextureSampler() {
    if (!gfx::hasExtension(SRGB_DECODE_EXTENSION)) {
        return;
    }
    GL_CHECK(glGenSamplers(1, &m_sampler));
    // The same reading as the sampler ImGui uses for its pictures (linear filter,
    // clamped to the edge), with one difference: no sRGB decoding. The setting is
    // ignored for textures that are not sRGB.
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glSamplerParameteri(m_sampler, TEXTURE_SRGB_DECODE, SKIP_DECODE));
}

RawTextureSampler::~RawTextureSampler() {
    // OpenGL silently ignores the id 0.
    GL_CHECK(glDeleteSamplers(1, &m_sampler));
}

void RawTextureSampler::begin() const {
    if (!isSupported()) {
        return;
    }
    // ImGui does not draw while the windows are built: it collects draw commands and
    // runs them at the end of the frame. A callback is a command of our own in that
    // list. With a size as the third argument ImGui copies the bytes it is given, so
    // the callback does not depend on this object or on the local variable.
    GLuint sampler = m_sampler;
    ImGui::GetWindowDrawList()->AddCallback(bindSampler, &sampler, sizeof(sampler));
}

void RawTextureSampler::end() const {
    if (!isSupported()) {
        return;
    }
    // The backend offers a callback that binds its own linear sampler again.
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear,
                                            nullptr);
}

} // namespace debug
