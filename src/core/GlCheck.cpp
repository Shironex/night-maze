// GL_CHECK: wraps an OpenGL call and reports OpenGL errors in Debug builds.
// See docs/modules/core/gl-check.md
#include "core/GlCheck.hpp"

#include "core/Log.hpp"

#include <string>

namespace core {

namespace {

// Converts an OpenGL error code to the name used in the OpenGL documentation.
const char* glErrorName(GLenum error) {
    switch (error) {
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "unknown OpenGL error";
    }
}

} // namespace

void checkGlErrors(const char* call, const char* file, int line) {
    // OpenGL keeps a set of error flags, so one call can leave more than one error behind.
    // glGetError returns and clears one flag at a time until it reports GL_NO_ERROR.
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        logError(std::string(glErrorName(error)) + " after " + call + " (" + file + ":" +
                 std::to_string(line) + ")");
        error = glGetError();
    }
}

} // namespace core
