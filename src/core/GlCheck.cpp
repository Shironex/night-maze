// GL_CHECK: wraps an OpenGL call and reports OpenGL errors in Debug builds.
#include "core/GlCheck.hpp"

#include "core/Log.hpp"

#include <string>

namespace core {

namespace {

// Most errors that one checkGlErrors call reads. Normally the loop ends much earlier, at
// GL_NO_ERROR. The limit exists because glGetError can keep returning an error forever when
// the OpenGL context is lost or not current, and then an unbounded loop would hang the program.
constexpr int MAX_ERRORS_PER_CHECK = 16;

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
    int errorCount = 0;
    while (error != GL_NO_ERROR && errorCount < MAX_ERRORS_PER_CHECK) {
        logError(std::string(glErrorName(error)) + " after " + call + " (" + file + ":" +
                 std::to_string(line) + ")");
        errorCount += 1;
        error = glGetError();
    }

    // Still an error after the limit: stop reading instead of spinning forever.
    if (error != GL_NO_ERROR) {
        logError("Stopped reading OpenGL errors after " + std::to_string(MAX_ERRORS_PER_CHECK) +
                 " errors (is the OpenGL context lost or not current?)");
    }
}

} // namespace core
