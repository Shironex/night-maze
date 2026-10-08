// Shader source text: the #include preprocessor and the file names in compile errors.
#pragma once

#include <functional>
#include <span>
#include <string>
#include <vector>

namespace gfx {

// GLSL has no #include. The functions here add one to the shader loader. They only work
// on text: no file is opened and no OpenGL function is called, so tests can run them.

/// Supplies the text of an included file. name is the text between the quotes of the
/// #include line, for example "common/lighting.glsl". The function writes the contents
/// into text and returns true, or returns false when there is no such file.
using IncludeReader = std::function<bool(const std::string& name, std::string& text)>;

/// One shader after all #include lines were replaced by the files they name.
struct ShaderSource {
    /// The complete text for glShaderSource.
    std::string text;

    /// The files the text was put together from. The position in this list is the
    /// "source string number" the #line directives in text use: files[0] is the shader
    /// file itself, files[1] the first included file, and so on. The driver prints that
    /// number in front of the line number of an error.
    std::vector<std::string> files;
};

/// Replaces every line of the form
///
///     #include "common/lighting.glsl"
///
/// in rootText by the contents of that file, also inside included files (nested
/// includes). rootName is the name of the shader file, used in error messages and as
/// files[0].
///
/// Around every included file two #line directives are written, so that the line numbers
/// in the compile errors of the driver stay the line numbers of the original files:
///
///     #line 1 N        before the file, N is its number in ShaderSource::files
///     #line L K        after it, L is the line after the #include, K the including file
///
/// Returns true and fills source. Returns false and writes a message with the file name
/// and the line into error when: an included file cannot be read, a file includes
/// itself (directly or through other files), an #include line is malformed, an #include
/// stands before the #version line, or an included file has a #version line of its own.
///
/// Limits: an #include inside a /* block comment */ is still carried out, and a file that
/// is included twice is inserted twice.
bool expandIncludes(const std::string& rootName, const std::string& rootText,
                    const IncludeReader& readInclude, ShaderSource& source, std::string& error);

/// Puts file names into the info log of a failed compilation. A driver starts an error
/// line with the source string number and the line number. Two formats are known, the
/// second one with and without a prefix:
///
///     1(15) : error C1503: ...     NVIDIA: number(line)
///     ERROR: 1:15: ...             Apple: number:line after "ERROR: " or "WARNING: "
///     1:15(3): error: ...          Mesa, Intel: number:line without a prefix
///
/// In such lines the number is replaced by files[number], for example
/// "common/lighting.glsl(15) : error C1503: ...". A line in another format, or with a
/// number that is not in files, is left exactly as the driver wrote it. When there is
/// more than one file, a last line lists what every number stands for, so that a log in
/// an unknown format can still be read.
std::string nameSourceFiles(const std::string& infoLog, std::span<const std::string> files);

} // namespace gfx
