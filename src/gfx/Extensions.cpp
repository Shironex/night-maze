// Extensions: asks the graphics driver whether it offers an OpenGL extension.
// See docs/modules/gfx/textures.md
#include "gfx/Extensions.hpp"

#include "core/GlCheck.hpp"

#include <glad/gl.h>

#include <cstring>

namespace gfx {

bool hasExtension(const char* name) {
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
        const char* listed = reinterpret_cast<const char*>(bytes);
        if (std::strcmp(listed, name) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace gfx
