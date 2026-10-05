// Shader program: a vertex and a fragment shader (and optionally a geometry shader) loaded
// from files, compiled and linked.
// See docs/modules/gfx/shader-class.md
#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace gfx {

/// Which binding point a uniform block of a program reads from. A Shader keeps one of
/// these for every block it was told about (Shader::bindUniformBlock).
struct UniformBlockBinding {
    /// Name of the block as written in the shader: "uniform LightBlock { ... }".
    std::string blockName;
    /// The uniform buffer binding point the block reads from (see gfx::UniformBuffer).
    GLuint bindingPoint = 0;
    /// Size of the block in bytes as the C++ code fills it. Compared with the size the
    /// driver reports, to catch a C++ struct and a GLSL block that do not match.
    std::size_t sizeInBytes = 0;
};

/// Owns one OpenGL program object built from a vertex shader file and a fragment shader file.
///
/// A program may have a third stage between the two: a geometry shader. It runs once for
/// every primitive the vertex shader has finished (a point, a line or a triangle) and
/// writes new primitives in its place, also more or fewer than it was given. The grass
/// uses it to turn one point into a tuft of blades. It is optional: a program without
/// a geometry file is built exactly as before.
///
/// A shader file may pull in other files with lines of the form
/// #include "common/lighting.glsl". The name is relative to the directory of the shader
/// file (assets/shaders). See gfx/ShaderSource.hpp.
///
/// The constructor loads the program, the destructor deletes it (RAII). The object can be
/// moved but not copied: a copy would hold the same program id and delete it a second time.
/// It needs a current OpenGL context for its whole life, so it must be destroyed before
/// the window.
class Shader {
public:
    /// Remembers the file paths and tries to load the program. It does not throw: when
    /// loading fails the error is logged, isValid() returns false and lastError() holds
    /// the message.
    ///
    /// geometryPath is the file of the geometry shader. An empty path (the default)
    /// means that the program has no geometry stage.
    Shader(std::filesystem::path vertexPath, std::filesystem::path fragmentPath,
           std::filesystem::path geometryPath = {});
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    /// Takes over the program of other. other is left without a program (not valid).
    Shader(Shader&& other) noexcept;
    /// Deletes the program this object owns, then takes over the program of other.
    Shader& operator=(Shader&& other) noexcept;

    /// Reads the files again (two, or three with a geometry shader), together with the
    /// files they include, and builds a new program. On success the new program replaces the old
    /// one and the function returns true. On failure (a file cannot be opened, a wrong #include, a
    /// compile error, a link error) the old program stays in use, the error is logged and kept in
    /// lastError(), and the function returns false. The uniform block bindings given to
    /// bindUniformBlock are set again on the new program.
    bool reload();

    /// True when the object owns a linked program.
    bool isValid() const { return m_program != 0; }

    /// Makes this program the one used by the following draw calls (glUseProgram).
    /// Check isValid() first: without a program nothing useful is drawn.
    void use() const;

    /// Sets the uniform variable of type mat4 called name to matrix (glUniformMatrix4fv).
    /// The program must be in use: call use() first, because OpenGL writes the value into
    /// the program that is current. A name the program does not have (a typo, or a uniform
    /// the compiler removed because the shader never reads it) is ignored without an error.
    void setMat4(const char* name, const glm::mat4& matrix) const;

    /// Sets the uniform variable of type int called name to value (glUniform1i). This is
    /// also the setter for sampler uniforms (sampler2D): a sampler holds the NUMBER OF A
    /// TEXTURE UNIT, not a texture id, so setInt("uTexture", 0) together with
    /// Texture2D::bind(0) connects the sampler to that texture. GLSL 4.20 can write the
    /// unit in the shader, layout(binding = 0), but GLSL 4.10 (the newest on macOS)
    /// cannot, so it is set from C++. The rules of setMat4 apply: use() first, and an
    /// unknown name is ignored.
    void setInt(const char* name, int value) const;

    /// Sets the uniform variable of type vec3 called name to value (glUniform3fv): a
    /// color, a position or a direction. The rules of setMat4 apply: use() first, and an
    /// unknown name is ignored.
    void setVec3(const char* name, const glm::vec3& value) const;

    /// Sets the uniform variable of type mat3 called name to matrix (glUniformMatrix3fv):
    /// the normal matrix. The rules of setMat4 apply: use() first, and an unknown name is
    /// ignored.
    void setMat3(const char* name, const glm::mat3& matrix) const;

    /// Sets the uniform variable of type float called name to value (glUniform1f). The
    /// rules of setMat4 apply: use() first, and an unknown name is ignored.
    void setFloat(const char* name, float value) const;

    /// Connects the uniform block called blockName to the uniform buffer binding point
    /// number bindingPoint (glGetUniformBlockIndex and glUniformBlockBinding). From then
    /// on the block reads the bytes of the gfx::UniformBuffer attached to that point.
    ///
    /// GLSL 4.20 can write the binding point in the shader, layout(binding = 1), but
    /// GLSL 4.10 (the newest on macOS) cannot, so it is set from C++.
    ///
    /// The connection is stored in the program object, and a reload makes a new program
    /// object. So the Shader remembers the request and repeats it after every reload.
    /// A program without a block of that name is left alone. The program does not have
    /// to be in use.
    ///
    /// sizeInBytes is the size of the C++ struct the buffer is filled from. When the
    /// driver reports another size for the block, an error is logged: the struct and the
    /// block in the shader then disagree about where the members are.
    void bindUniformBlock(std::string blockName, GLuint bindingPoint, std::size_t sizeInBytes);

    /// Message of the last failed load: the file name or names and the driver's info log,
    /// with the names of included files in place of the source numbers the driver prints
    /// (gfx::nameSourceFiles). Empty after a successful load.
    const std::string& lastError() const { return m_lastError; }

    /// File the vertex shader is read from, as given to the constructor.
    const std::filesystem::path& vertexPath() const { return m_vertexPath; }

    /// File the fragment shader is read from, as given to the constructor.
    const std::filesystem::path& fragmentPath() const { return m_fragmentPath; }

    /// True when the program has a geometry stage: a geometry file was given to the
    /// constructor.
    bool hasGeometryStage() const { return !m_geometryPath.empty(); }

    /// File the geometry shader is read from, as given to the constructor. Empty when the
    /// program has no geometry stage.
    const std::filesystem::path& geometryPath() const { return m_geometryPath; }

private:
    std::filesystem::path m_vertexPath;
    std::filesystem::path m_fragmentPath;
    // Empty when the program has no geometry stage.
    std::filesystem::path m_geometryPath;
    // Name (id) of the OpenGL program object. 0 is never a real program: it means "none".
    GLuint m_program = 0;
    std::string m_lastError;
    // The requests made with bindUniformBlock, repeated on every newly built program.
    std::vector<UniformBlockBinding> m_blockBindings;
};

} // namespace gfx
