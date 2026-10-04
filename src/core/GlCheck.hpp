// GL_CHECK: wraps an OpenGL call and reports OpenGL errors in Debug builds.
// OpenGL 4.1 has no debug message callback (that is 4.3+), so glGetError is the only option.
// See docs/modules/core/gl-check.md
#pragma once

#include <glad/gl.h>

namespace core {

/// Reads the pending OpenGL errors (up to a fixed limit per call) and logs each one together
/// with the call text and its location. Used by GL_CHECK, not meant to be called directly.
void checkGlErrors(const char* call, const char* file, int line);

} // namespace core

/// Runs one OpenGL call. In Debug builds it then logs any OpenGL error the call caused.
/// In Release builds (NDEBUG defined) it is just the call.
///
/// Usage:
///     GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
///     GL_CHECK(id = glCreateShader(GL_VERTEX_SHADER)); // calls that return a value
///
/// #call turns the argument into a string, __FILE__ and __LINE__ are the place of use.
/// The do { } while (false) wrapper makes the macro behave like a single statement,
/// so it is safe inside an if without braces.
#ifndef NDEBUG
#define GL_CHECK(call)                                                                             \
    do {                                                                                           \
        call;                                                                                      \
        core::checkGlErrors(#call, __FILE__, __LINE__);                                            \
    } while (false)
#else
#define GL_CHECK(call)                                                                             \
    do {                                                                                           \
        call;                                                                                      \
    } while (false)
#endif
