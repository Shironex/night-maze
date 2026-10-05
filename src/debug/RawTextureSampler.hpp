// RawTextureSampler: lets the debug UI show an sRGB texture as the picture in its file.
// See docs/modules/debug-ui.md
#pragma once

#include <glad/gl.h>

namespace debug {

/// Shows sRGB textures in ImGui previews the way they look in their files.
///
/// The problem: a colour texture of the game is an sRGB texture, so reading it gives
/// LINEAR values. The scene encodes them again in its last pass, but ImGui draws
/// a preview straight into the window, and the picture would come out too dark.
///
/// The fix: for the one preview picture, ImGui is told to read the texture through
/// a sampler object of this class, on which the decoding is switched off
/// (GL_TEXTURE_SRGB_DECODE_EXT set to GL_SKIP_DECODE_EXT). The bytes of the file then
/// reach the window unchanged, as before the textures were sRGB. This is the extension
/// GL_EXT_texture_sRGB_decode, not a part of OpenGL 4.1 Core. Where the driver does not
/// offer it the object does nothing and the previews are darker than the files.
///
/// It owns an OpenGL sampler object (RAII, not copyable), so it must be destroyed
/// before the window.
class RawTextureSampler {
public:
    /// Asks the driver for the extension and creates the sampler object when it is
    /// there.
    RawTextureSampler();
    ~RawTextureSampler();

    RawTextureSampler(const RawTextureSampler&) = delete;
    RawTextureSampler& operator=(const RawTextureSampler&) = delete;

    /// True when the driver offers the extension, so begin and end have an effect.
    bool isSupported() const { return m_sampler != 0; }

    /// From here on the pictures added to the current ImGui window are read without
    /// sRGB decoding. Call it right before ImGui::Image, inside a window.
    void begin() const;

    /// Back to the sampler ImGui draws everything else with. Call it right after the
    /// picture.
    void end() const;

private:
    // Name (id) of the sampler object. 0 means "the extension is missing".
    GLuint m_sampler = 0;
};

} // namespace debug
