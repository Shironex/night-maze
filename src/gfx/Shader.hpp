// Shader program: a vertex and a fragment shader loaded from files, compiled and linked.
// See docs/modules/gfx/shaders.md
#pragma once

#include <glad/gl.h>

#include <filesystem>
#include <string>

namespace gfx {

/// Owns one OpenGL program object built from a vertex shader file and a fragment shader file.
///
/// The constructor loads the program, the destructor deletes it (RAII). The object can be
/// moved but not copied: a copy would hold the same program id and delete it a second time.
/// It needs a current OpenGL context for its whole life, so it must be destroyed before
/// the window.
class Shader {
public:
    /// Remembers both file paths and tries to load the program. It does not throw: when
    /// loading fails the error is logged, isValid() returns false and lastError() holds
    /// the message.
    Shader(std::filesystem::path vertexPath, std::filesystem::path fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    /// Takes over the program of other. other is left without a program (not valid).
    Shader(Shader&& other) noexcept;
    /// Deletes the program this object owns, then takes over the program of other.
    Shader& operator=(Shader&& other) noexcept;

    /// Reads both files again and builds a new program. On success the new program replaces
    /// the old one and the function returns true. On failure (a file cannot be opened,
    /// a compile error, a link error) the old program stays in use, the error is logged and
    /// kept in lastError(), and the function returns false.
    bool reload();

    /// True when the object owns a linked program.
    bool isValid() const { return m_program != 0; }

    /// Makes this program the one used by the following draw calls (glUseProgram).
    /// Check isValid() first: without a program nothing useful is drawn.
    void use() const;

    /// Message of the last failed load: the file name or names and the driver's info log.
    /// Empty after a successful load.
    const std::string& lastError() const { return m_lastError; }

    /// File the vertex shader is read from, as given to the constructor.
    const std::filesystem::path& vertexPath() const { return m_vertexPath; }

    /// File the fragment shader is read from, as given to the constructor.
    const std::filesystem::path& fragmentPath() const { return m_fragmentPath; }

private:
    std::filesystem::path m_vertexPath;
    std::filesystem::path m_fragmentPath;
    // Name (id) of the OpenGL program object. 0 is never a real program: it means "none".
    GLuint m_program = 0;
    std::string m_lastError;
};

} // namespace gfx
