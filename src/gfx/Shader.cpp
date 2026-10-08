// Shader program: a vertex and a fragment shader (and optionally a geometry shader) loaded
// from files, compiled and linked.
#include "gfx/Shader.hpp"

#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "gfx/ShaderSource.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <fstream>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

namespace gfx {

namespace {

// Reads a whole text file into text. Returns false when the file cannot be opened.
bool readTextFile(const std::filesystem::path& path, std::string& text) {
    // The stream closes the file in its destructor.
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    // rdbuf() is the buffer the stream reads the file through. Sending it to another
    // stream with << copies everything up to the end of the file.
    std::ostringstream contents;
    contents << file.rdbuf();
    text = contents.str();
    return true;
}

// Text the driver wrote while compiling a shader: errors and warnings with line numbers.
std::string shaderInfoLog(GLuint shader) {
    // Length of the log in characters, including the terminating zero. 0 means no log.
    GLint length = 0;
    GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
    if (length <= 0) {
        return {};
    }

    std::string log(static_cast<std::size_t>(length), '\0');
    // written receives the number of characters copied, without the terminating zero.
    GLsizei written = 0;
    GL_CHECK(glGetShaderInfoLog(shader, length, &written, log.data()));
    log.resize(static_cast<std::size_t>(written));
    return log;
}

// Text the driver wrote while linking a program. Same steps as shaderInfoLog, but
// a program object has its own pair of functions.
std::string programInfoLog(GLuint program) {
    GLint length = 0;
    GL_CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
    if (length <= 0) {
        return {};
    }

    std::string log(static_cast<std::size_t>(length), '\0');
    GLsizei written = 0;
    GL_CHECK(glGetProgramInfoLog(program, length, &written, log.data()));
    log.resize(static_cast<std::size_t>(written));
    return log;
}

// Reads one shader file, puts the files it includes into it and compiles the result.
// type is GL_VERTEX_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER. Returns the id of
// the shader object, or 0 on failure with the message in error.
GLuint compileShader(GLenum type, const std::filesystem::path& path, std::string& error) {
    std::string fileText;
    if (!readTextFile(path, fileText)) {
        error = "Shader file cannot be opened: " + core::pathText(path);
        return 0;
    }

    // The name in an #include line is relative to the directory of the shader file:
    // "common/lighting.glsl" in assets/shaders/lit.frag is the file
    // assets/shaders/common/lighting.glsl. The lambda is called once for every #include
    // line, on every load, so a reload reads the included files again too.
    const std::filesystem::path includeDirectory = path.parent_path();
    const IncludeReader readInclude = [&includeDirectory](const std::string& name,
                                                          std::string& text) {
        return readTextFile(includeDirectory / name, text);
    };

    ShaderSource source;
    std::string includeError;
    if (!expandIncludes(core::pathText(path.filename()), fileText, readInclude, source,
                        includeError)) {
        error = "Shader include failed: " + core::pathText(path) + "\n" + includeError;
        return 0;
    }

    GLuint shader = 0;
    GL_CHECK(shader = glCreateShader(type));

    // glShaderSource takes an array of C strings. Here the array has one element.
    // nullptr as the array of lengths means that every string ends with a zero.
    const char* sourceText = source.text.c_str();
    GL_CHECK(glShaderSource(shader, 1, &sourceText, nullptr));
    GL_CHECK(glCompileShader(shader));

    // A compile error does not set an OpenGL error flag, so GL_CHECK cannot see it.
    // The result has to be asked for.
    GLint status = GL_FALSE;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (status != GL_TRUE) {
        // The driver names a file by its number in source.files. nameSourceFiles writes
        // the name instead, so an error inside an included file is reported against it.
        error = "Shader compilation failed: " + core::pathText(path) + "\n" +
                nameSourceFiles(shaderInfoLog(shader), source.files);
        GL_CHECK(glDeleteShader(shader));
        return 0;
    }
    return shader;
}

// Links the compiled shaders (one per stage) into a new program. Returns the id of the
// program object, or 0 on failure with the driver's text in infoLog.
GLuint linkProgram(std::span<const GLuint> shaders, std::string& infoLog) {
    GLuint program = 0;
    GL_CHECK(program = glCreateProgram());
    for (const GLuint shader : shaders) {
        GL_CHECK(glAttachShader(program, shader));
    }
    GL_CHECK(glLinkProgram(program));

    // A linked program keeps its own executable code, so it no longer needs the shader
    // objects. Detaching them lets glDeleteShader really free them.
    for (const GLuint shader : shaders) {
        GL_CHECK(glDetachShader(program, shader));
    }

    // Like compiling, a failed link sets no OpenGL error flag.
    GLint status = GL_FALSE;
    GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (status != GL_TRUE) {
        infoLog = programInfoLog(program);
        GL_CHECK(glDeleteProgram(program));
        return 0;
    }
    return program;
}

// One stage of a program: what kind of shader it is and the file it is read from.
struct ShaderStage {
    GLenum type;
    const std::filesystem::path* path;
};

// Deletes every shader object of the list.
void deleteShaders(std::span<const GLuint> shaders) {
    for (const GLuint shader : shaders) {
        GL_CHECK(glDeleteShader(shader));
    }
}

// Builds a complete program from its files: compile every stage, then link. An empty
// geometryPath means a program of two stages. Returns the id of the new program, or 0 on
// failure with the message in error. Whatever happens, no shader object is left behind.
GLuint buildProgram(const std::filesystem::path& vertexPath,
                    const std::filesystem::path& fragmentPath,
                    const std::filesystem::path& geometryPath, std::string& error) {
    // The stages in the order the graphics card runs them: every vertex, then (when
    // there is a geometry shader) every primitive, then every fragment.
    std::vector<ShaderStage> stages;
    stages.push_back({.type = GL_VERTEX_SHADER, .path = &vertexPath});
    if (!geometryPath.empty()) {
        stages.push_back({.type = GL_GEOMETRY_SHADER, .path = &geometryPath});
    }
    stages.push_back({.type = GL_FRAGMENT_SHADER, .path = &fragmentPath});

    // Every stage goes through the same loader: the same #include lines, the same file
    // names in its compile errors. The names of all files are collected for the message
    // of a failed link, which belongs to no single file.
    std::vector<GLuint> shaders;
    std::string fileNames;
    for (const ShaderStage& stage : stages) {
        const GLuint shader = compileShader(stage.type, *stage.path, error);
        if (shader == 0) {
            // The stages compiled so far are of no use without this one.
            deleteShaders(shaders);
            return 0;
        }
        shaders.push_back(shader);

        if (!fileNames.empty()) {
            fileNames += " + ";
        }
        fileNames += core::pathText(*stage.path);
    }

    std::string infoLog;
    const GLuint program = linkProgram(shaders, infoLog);

    // The shader objects were only an intermediate step, linked or not.
    deleteShaders(shaders);

    if (program == 0) {
        error = "Shader linking failed: " + fileNames + "\n" + infoLog;
    }
    return program;
}

// Connects one uniform block of program to its binding point and checks its size.
void applyBlockBinding(GLuint program, const UniformBlockBinding& binding) {
    // The index is the number of the block inside this program, like the location of
    // a plain uniform. GL_INVALID_INDEX means the program has no active block with this
    // name: nothing to connect. The check is needed, because glUniformBlockBinding
    // raises GL_INVALID_VALUE for that index.
    GLuint blockIndex = GL_INVALID_INDEX;
    GL_CHECK(blockIndex = glGetUniformBlockIndex(program, binding.blockName.c_str()));
    if (blockIndex == GL_INVALID_INDEX) {
        return;
    }
    GL_CHECK(glUniformBlockBinding(program, blockIndex, binding.bindingPoint));

    // How many bytes the driver laid the block out in. With std140 the layout is fixed
    // by the standard, so this must be the size of the C++ struct the buffer is filled
    // from. A difference means the two were changed apart (for example the number of
    // point lights), and every member after the first difference would be read wrong.
    GLint driverSize = 0;
    GL_CHECK(
        glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &driverSize));
    if (static_cast<std::size_t>(driverSize) != binding.sizeInBytes) {
        core::logError("Uniform block " + binding.blockName + " is " + std::to_string(driverSize) +
                       " bytes in the shader, but " + std::to_string(binding.sizeInBytes) +
                       " bytes in the C++ code");
    }
}

} // namespace

// The paths arrive by value and are moved into the members, so a caller that passes
// a temporary (the result of core::assetPath) pays for no copy.
Shader::Shader(std::filesystem::path vertexPath, std::filesystem::path fragmentPath,
               std::filesystem::path geometryPath)
    : m_vertexPath(std::move(vertexPath)),
      m_fragmentPath(std::move(fragmentPath)),
      m_geometryPath(std::move(geometryPath)) {
    // The first load is the same work as a reload, starting from "no program".
    reload();
}

Shader::~Shader() {
    // OpenGL silently ignores glDeleteProgram(0), so an object without a program
    // (a failed load, or one that was moved from) needs no special case.
    GL_CHECK(glDeleteProgram(m_program));
}

// Move constructor: the new object takes the program id, and other gives it up.
Shader::Shader(Shader&& other) noexcept
    : m_vertexPath(std::move(other.m_vertexPath)),
      m_fragmentPath(std::move(other.m_fragmentPath)),
      m_geometryPath(std::move(other.m_geometryPath)),
      m_program(other.m_program),
      m_lastError(std::move(other.m_lastError)),
      m_blockBindings(std::move(other.m_blockBindings)) {
    // Two objects must never hold the same id. With 0 the destructor of other
    // deletes nothing.
    other.m_program = 0;
}

// Move assignment: this object already owns a program, which has to go first.
Shader& Shader::operator=(Shader&& other) noexcept {
    // shader = std::move(shader): nothing to do. Without this check the program would
    // be deleted here and then "taken over" from itself.
    if (this == &other) {
        return *this;
    }

    // Release the program owned so far (ignored by OpenGL when it is 0).
    GL_CHECK(glDeleteProgram(m_program));

    m_vertexPath = std::move(other.m_vertexPath);
    m_fragmentPath = std::move(other.m_fragmentPath);
    m_geometryPath = std::move(other.m_geometryPath);
    m_program = other.m_program;
    m_lastError = std::move(other.m_lastError);
    m_blockBindings = std::move(other.m_blockBindings);
    other.m_program = 0;
    return *this;
}

bool Shader::reload() {
    // Build the new program completely before touching the one in use.
    std::string error;
    const GLuint program = buildProgram(m_vertexPath, m_fragmentPath, m_geometryPath, error);
    if (program == 0) {
        // m_program is not changed: the previous program, if there is one, keeps working.
        m_lastError = error;
        core::logError(m_lastError);
        return false;
    }

    // A binding point of a uniform block is stored in the program object, and this one
    // is new: every block is back at binding point 0. Set them again.
    for (const UniformBlockBinding& binding : m_blockBindings) {
        applyBlockBinding(program, binding);
    }

    // Only now replace the old program (glDeleteProgram(0) is ignored on the first load).
    GL_CHECK(glDeleteProgram(m_program));
    m_program = program;
    m_lastError.clear();
    return true;
}

void Shader::use() const {
    GL_CHECK(glUseProgram(m_program));
}

void Shader::setMat4(const char* name, const glm::mat4& matrix) const {
    // The location is the number of the uniform inside this program. It is looked up on
    // every call: a few hundred lookups per frame (one per drawn object) are still cheap,
    // and there is no cache that could go stale after reload(). -1 means the program has
    // no active uniform with this name.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // 1: one matrix. GL_FALSE: do not transpose, GLM stores a matrix column by column,
    // which is the order OpenGL expects. value_ptr gives the address of its 16 floats.
    // OpenGL ignores location -1 without raising an error.
    GL_CHECK(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::setInt(const char* name, int value) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // glUniform1i: one value of type int. A sampler uniform must be set with exactly this
    // function: the float version (glUniform1f) raises GL_INVALID_OPERATION for a sampler.
    // OpenGL ignores location -1 without raising an error.
    GL_CHECK(glUniform1i(location, value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // 1: one vector (more only for a uniform that is an array). value_ptr gives the
    // address of its 3 floats. OpenGL ignores location -1 without raising an error.
    GL_CHECK(glUniform3fv(location, 1, glm::value_ptr(value)));
}

void Shader::setMat3(const char* name, const glm::mat3& matrix) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // The 3 x 3 version of the call in setMat4: one matrix, not transposed, 9 floats.
    GL_CHECK(glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
}

void Shader::setFloat(const char* name, float value) const {
    // Same lookup as in setMat4: no cache, -1 for a name the program does not have.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // glUniform1f: one value of type float. OpenGL ignores location -1 without an error.
    GL_CHECK(glUniform1f(location, value));
}

void Shader::setFloatArray(const char* name, std::span<const float> values) const {
    // Same lookup as in setMat4. The name of an array without an index gives the
    // location of its element 0.
    GLint location = -1;
    GL_CHECK(location = glGetUniformLocation(m_program, name));

    // glUniform1fv: count values of type float, read from the pointer and written to
    // the elements of the array, starting with the one at location.
    GL_CHECK(glUniform1fv(location, static_cast<GLsizei>(values.size()), values.data()));
}

void Shader::bindUniformBlock(std::string blockName, GLuint bindingPoint, std::size_t sizeInBytes) {
    m_blockBindings.push_back({.blockName = std::move(blockName),
                               .bindingPoint = bindingPoint,
                               .sizeInBytes = sizeInBytes});
    // The program that exists now gets the binding at once. Without a program (a failed
    // first load) the request waits for the next successful reload.
    if (isValid()) {
        applyBlockBinding(m_program, m_blockBindings.back());
    }
}

} // namespace gfx
