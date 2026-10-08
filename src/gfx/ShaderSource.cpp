// Shader source text: the #include preprocessor and the file names in compile errors.
#include "gfx/ShaderSource.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string_view>

namespace gfx {

namespace {

// Names of the two preprocessor directives the loader looks at.
constexpr std::string_view INCLUDE_DIRECTIVE = "include";
constexpr std::string_view VERSION_DIRECTIVE = "version";

// The characters that may stand between the parts of a directive.
constexpr std::string_view BLANKS = " \t";

// Prefixes some drivers write in front of the source string number.
constexpr std::string_view ERROR_PREFIX = "ERROR: ";
constexpr std::string_view WARNING_PREFIX = "WARNING: ";

// A source string number with more digits than this is not one of ours. The limit also
// keeps the conversion to a number far away from an overflow.
constexpr std::size_t MAX_NUMBER_DIGITS = 6;

bool isDigit(char character) {
    return character >= '0' && character <= '9';
}

// line without the blanks at its start.
std::string_view withoutLeadingBlanks(std::string_view line) {
    const std::size_t first = line.find_first_not_of(BLANKS);
    return first == std::string_view::npos ? std::string_view{} : line.substr(first);
}

// When line is a preprocessor directive with the given name ("#include ...", also with
// blanks around the #), returns true and writes what follows the name into rest.
bool isDirective(std::string_view line, std::string_view name, std::string_view& rest) {
    std::string_view text = withoutLeadingBlanks(line);
    if (text.empty() || text.front() != '#') {
        return false;
    }
    text = withoutLeadingBlanks(text.substr(1));
    if (!text.starts_with(name)) {
        return false;
    }
    text = text.substr(name.size());
    // The name must end here: "#includes" or "#version2" are other words.
    if (!text.empty() && BLANKS.find(text.front()) == std::string_view::npos &&
        text.front() != '"') {
        return false;
    }
    rest = text;
    return true;
}

// Reads the file name out of the rest of an #include line: the text between the two
// quotes. Returns false when there are no two quotes or nothing is between them.
bool includedName(std::string_view rest, std::string& name) {
    const std::string_view text = withoutLeadingBlanks(rest);
    if (text.empty() || text.front() != '"') {
        return false;
    }
    const std::size_t closingQuote = text.find('"', 1);
    if (closingQuote == std::string_view::npos || closingQuote == 1) {
        return false;
    }
    name = std::string(text.substr(1, closingQuote - 1));
    return true;
}

// "lit.frag:12: " : the start of every error message of the preprocessor.
std::string place(const std::string& fileName, int lineNumber) {
    return fileName + ":" + std::to_string(lineNumber) + ": ";
}

// Everything one call of expandIncludes works with, so that the recursive function
// below does not need six parameters.
struct Expansion {
    const IncludeReader& readInclude;
    ShaderSource& source;
    std::string& error;
    // Numbers of the files that are being read right now: the shader file, the file it
    // includes, the file that one includes, and so on. A file that is in this list and
    // is included again would include itself for ever.
    std::vector<std::size_t> openFiles;
};

// The number of a file in ShaderSource::files. A new name is added at the end.
std::size_t fileNumberOf(ShaderSource& source, const std::string& name) {
    const auto found = std::ranges::find(source.files, name);
    if (found != source.files.end()) {
        return static_cast<std::size_t>(found - source.files.begin());
    }
    source.files.push_back(name);
    return source.files.size() - 1;
}

// Appends the text of one file to the result, line by line. An #include line is replaced
// by the file it names: the function calls itself for that file. Returns false with the
// message in expansion.error.
bool appendFile(Expansion& expansion, std::size_t fileNumber, const std::string& text) {
    // A copy of the name: the list of files may grow (and move) while this file is read.
    const std::string fileName = expansion.source.files[fileNumber];
    const bool isShaderFile = expansion.openFiles.empty();
    expansion.openFiles.push_back(fileNumber);

    bool versionSeen = false;
    int lineNumber = 0;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        ++lineNumber;
        // A file with Windows line ends that was read in binary mode keeps a carriage
        // return in front of every line feed.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string_view rest;
        if (isDirective(line, VERSION_DIRECTIVE, rest)) {
            // GLSL wants #version before everything else, so it can only stand in the
            // shader file itself. In an included file it would land in the middle.
            if (!isShaderFile) {
                expansion.error =
                    place(fileName, lineNumber) + "an included file must not have a #version line";
                return false;
            }
            versionSeen = true;
        }

        if (!isDirective(line, INCLUDE_DIRECTIVE, rest)) {
            // An ordinary line goes through unchanged. Every line gets a line end, also
            // the last line of a file that has none: the #line directive that follows an
            // included file must start on a line of its own.
            expansion.source.text += line;
            expansion.source.text += '\n';
            continue;
        }

        if (isShaderFile && !versionSeen) {
            expansion.error = place(fileName, lineNumber) +
                              "#include must come after the #version line, which has to stay "
                              "the first line of the shader";
            return false;
        }

        std::string includedFile;
        if (!includedName(rest, includedFile)) {
            expansion.error =
                place(fileName, lineNumber) + "malformed #include, expected #include \"file name\"";
            return false;
        }

        const std::size_t includedNumber = fileNumberOf(expansion.source, includedFile);
        if (std::ranges::find(expansion.openFiles, includedNumber) != expansion.openFiles.end()) {
            expansion.error = place(fileName, lineNumber) + "#include cycle: \"" + includedFile +
                              "\" is already being included, a file cannot include itself";
            return false;
        }

        std::string includedText;
        if (!expansion.readInclude(includedFile, includedText)) {
            expansion.error = place(fileName, lineNumber) + "included file cannot be opened: \"" +
                              includedFile + "\"";
            return false;
        }

        // "#line L N": the line after this directive counts as line L of source string
        // number N (GLSL 3.30 and newer). The included file starts at its line 1.
        expansion.source.text += "#line 1 " + std::to_string(includedNumber) + "\n";
        if (!appendFile(expansion, includedNumber, includedText)) {
            return false;
        }
        // Back in this file: the next line is the one after the #include line.
        expansion.source.text +=
            "#line " + std::to_string(lineNumber + 1) + " " + std::to_string(fileNumber) + "\n";
    }

    expansion.openFiles.pop_back();
    return true;
}

// Replaces the source string number at the start of one line of an info log by the name
// of the file, when the line has one of the two known formats.
void nameSourceFile(std::string& line, std::span<const std::string> files) {
    // Where the number starts: at the start of the line (NVIDIA, Mesa, Intel), or after
    // "ERROR: " or "WARNING: " (Apple).
    std::size_t start = 0;
    if (line.starts_with(ERROR_PREFIX)) {
        start = ERROR_PREFIX.size();
    } else if (line.starts_with(WARNING_PREFIX)) {
        start = WARNING_PREFIX.size();
    }

    // The digits of the number: from start up to (not including) end.
    std::size_t end = start;
    while (end < line.size() && isDigit(line[end])) {
        ++end;
    }
    const std::size_t digitCount = end - start;
    if (digitCount == 0 || digitCount > MAX_NUMBER_DIGITS || end + 1 >= line.size()) {
        return;
    }

    // What follows the number tells the format: "(15)" on NVIDIA, ":15" elsewhere. Only
    // a number that is followed by a line number is taken for a source string number.
    const bool lineNumberFollows = isDigit(line[end + 1]);
    const bool nvidiaFormat = start == 0 && line[end] == '(' && lineNumberFollows;
    const bool colonFormat = line[end] == ':' && lineNumberFollows;
    if (!nvidiaFormat && !colonFormat) {
        return;
    }

    const std::size_t number = std::stoul(line.substr(start, digitCount));
    if (number >= files.size()) {
        return;
    }
    line.replace(start, digitCount, files[number]);
}

} // namespace

bool expandIncludes(const std::string& rootName, const std::string& rootText,
                    const IncludeReader& readInclude, ShaderSource& source, std::string& error) {
    source.text.clear();
    source.files.clear();
    // The shader file itself is source string number 0, the number the driver uses for
    // a text without any #line directive.
    source.files.push_back(rootName);

    Expansion expansion{
        .readInclude = readInclude,
        .source = source,
        .error = error,
        .openFiles = {},
    };
    return appendFile(expansion, 0, rootText);
}

std::string nameSourceFiles(const std::string& infoLog, std::span<const std::string> files) {
    std::string result;
    std::istringstream lines(infoLog);
    std::string line;
    while (std::getline(lines, line)) {
        nameSourceFile(line, files);
        result += line;
        result += '\n';
    }

    // With a single file every number is 0 and means that file: nothing to explain.
    if (files.size() > 1) {
        result += "Source files:";
        for (std::size_t number = 0; number < files.size(); ++number) {
            result += number == 0 ? " " : ", ";
            result += std::to_string(number) + " = " + files[number];
        }
        result += '\n';
    }
    return result;
}

} // namespace gfx
