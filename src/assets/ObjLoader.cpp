// OBJ loader: reads a Wavefront OBJ model and its MTL materials into plain CPU data.
// See docs/modules/assets/obj-loader.md
#include "assets/ObjLoader.hpp"

#include "assets/Tangents.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"

#include <array>
#include <charconv>
#include <fstream>
#include <limits>
#include <locale>
#include <map>
#include <span>
#include <sstream>
#include <system_error>
#include <utility>

namespace assets {

namespace {

// Characters that separate the fields of a line. '\r' is here because a file saved with
// Windows line endings has "\r\n" at the end of every line: after cutting at '\n' the
// '\r' is left over and has to be treated like a trailing space.
constexpr std::string_view BLANKS = " \t\r";

// Everything from this character to the end of the line is a comment.
constexpr char COMMENT_START = '#';

// Separates the indices of one face corner: position/uv/normal.
constexpr char INDEX_SEPARATOR = '/';

// A face corner has at most three indices.
constexpr std::size_t MAX_CORNER_FIELDS = 3;

// A colour is three numbers: red, green, blue.
constexpr std::size_t COLOR_COMPONENTS = 3;

// A face needs at least three corners to have an area.
constexpr std::size_t MIN_FACE_CORNERS = 3;

// Option of a normal map line in an MTL file: "-bm 1.0" is the bump multiplier, the
// strength of the map. Blender writes it in front of the file name.
constexpr std::string_view BUMP_MULTIPLIER_OPTION = "-bm";

// Stands for "this corner has no uv" or "no normal" in a triple of indices. It is the
// largest value an index can hold. A real index could only be equal to it in a file
// with over four billion elements in one list, which does not fit in memory.
constexpr std::uint32_t NO_INDEX = std::numeric_limits<std::uint32_t>::max();

// The three indices of one face corner, already turned into 0-based positions in the
// lists of the file: {position, uv, normal}. uv and normal may be NO_INDEX.
using CornerKey = std::array<std::uint32_t, MAX_CORNER_FIELDS>;

// Cuts the first line off rest and returns it without the '\n'. The last line of a text
// needs no '\n' at its end.
std::string_view takeLine(std::string_view& rest) {
    const std::size_t end = rest.find('\n');
    if (end == std::string_view::npos) {
        // No more line breaks: everything that is left is the last line.
        const std::string_view line = rest;
        rest = {};
        return line;
    }
    const std::string_view line = rest.substr(0, end);
    rest.remove_prefix(end + 1);
    return line;
}

// The line without its comment: everything before the first '#'.
std::string_view withoutComment(std::string_view line) {
    return line.substr(0, line.find(COMMENT_START));
}

// The text without blanks at its beginning and at its end.
std::string_view trim(std::string_view text) {
    const std::size_t first = text.find_first_not_of(BLANKS);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(BLANKS);
    return text.substr(first, last - first + 1);
}

// Cuts the next field off rest and returns it: skips blanks, then takes characters up to
// the next blank. Any number of spaces and tabs may separate two fields. Returns an empty
// text when the line has no more fields.
std::string_view takeToken(std::string_view& rest) {
    const std::size_t first = rest.find_first_not_of(BLANKS);
    if (first == std::string_view::npos) {
        rest = {};
        return {};
    }
    rest.remove_prefix(first);

    // npos (no blank found) makes substr take everything that is left.
    const std::size_t length = rest.find_first_of(BLANKS);
    const std::string_view token = rest.substr(0, length);
    rest.remove_prefix(token.size());
    return token;
}

// Turns a whole field into a float. Returns false when the field is not a number.
//
// The number is read through a stream that uses the "classic" locale, the one of the C
// language: the decimal separator is always a dot. Functions like std::stof and atof
// follow the global locale of the program instead, and with a Polish locale they expect
// "0,5" and stop reading "0.5" at the dot. std::from_chars would be the direct tool, but
// libc++ (the standard library used on macOS) has its overloads for float only since
// LLVM 20, and it is not confirmed that the version shipped with Apple clang has them.
bool parseFloat(std::string_view token, float& value) {
    if (token.empty()) {
        return false;
    }
    std::istringstream stream{std::string(token)};
    stream.imbue(std::locale::classic());
    stream >> value;
    // The whole field must have been used: "1.5abc" is not a number. After a successful
    // read that stops at the end of the text, peek() finds nothing more to read.
    return !stream.fail() && stream.peek() == std::istringstream::traits_type::eof();
}

// Turns a whole field into an integer. Returns false when the field is not an integer
// (also when it is too large for the type). std::from_chars never looks at the locale,
// and its integer overloads exist in every standard library the project is built with.
bool parseInteger(std::string_view token, long long& value) {
    const char* const begin = token.data();
    const char* const end = begin + token.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    // ec is the default (no error) on success, ptr is where the reading stopped.
    return result.ec == std::errc() && result.ptr == end;
}

// Reads the next fields of rest as floats, one for every element of values. Returns false
// when a field is missing or is not a number. Fields after them are left in rest.
bool takeFloats(std::string_view& rest, std::span<float> values) {
    for (float& value : values) {
        if (!parseFloat(takeToken(rest), value)) {
            return false;
        }
    }
    return true;
}

// Message of a parse error. Line numbers start at 1, like in a text editor.
std::string lineError(std::size_t lineNumber, std::string_view message) {
    return "line " + std::to_string(lineNumber) + ": " + std::string(message);
}

// Turns one index written in a face corner into a 0-based index into a list that has
// count elements at this point of the file. Returns false when the text is not an integer
// or points outside the list.
//
// Two rules of the OBJ format: positive indices count from 1 (1 is the first element),
// and negative indices count back from the end (-1 is the element defined last).
bool resolveIndex(std::string_view text, std::size_t count, std::uint32_t& index) {
    long long written = 0;
    if (!parseInteger(text, written)) {
        return false;
    }

    // Signed arithmetic, so that a negative index needs no special cases.
    const auto size = static_cast<long long>(count);
    const long long zeroBased = written > 0 ? written - 1 : size + written;
    // written == 0 gives zeroBased == size and is rejected here too: 0 is not a valid
    // index in an OBJ file.
    if (zeroBased < 0 || zeroBased >= size) {
        return false;
    }
    index = static_cast<std::uint32_t>(zeroBased);
    return true;
}

// The state of parsing one OBJ text: the three lists of the file read so far and the
// model being built from them.
struct ObjParser {
    // Lists of the file, in file order. A face refers to their elements by number.
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    // The output vertex made for each triple of indices seen so far. This is what lets
    // two corners with the same triple share one vertex.
    std::map<CornerKey, std::uint32_t> vertexOfCorner;

    // Name from the last usemtl line. Empty until the first one.
    std::string currentMaterial;

    // Output indices of the corners of the face being read. A member, so that its memory
    // is reused from face to face.
    std::vector<std::uint32_t> faceCorners;

    ObjModel model;

    // Each function handles the fields of one kind of line (the keyword is already cut
    // off). On failure it returns false with the reason in message.
    bool readPosition(std::string_view rest, std::string& message);
    bool readUv(std::string_view rest, std::string& message);
    bool readNormal(std::string_view rest, std::string& message);
    bool readFace(std::string_view rest, std::string& message);

    // Turns the text of one face corner into the number of an output vertex, creating
    // the vertex when this combination of indices has not been seen yet.
    bool readCorner(std::string_view corner, std::uint32_t& vertexIndex, std::string& message);
};

bool ObjParser::readPosition(std::string_view rest, std::string& message) {
    // A fourth number (w) or vertex colours after the three are ignored.
    std::array<float, gfx::POSITION_COMPONENTS> values{};
    if (!takeFloats(rest, values)) {
        message = "v needs three numbers";
        return false;
    }
    positions.emplace_back(values[0], values[1], values[2]);
    return true;
}

bool ObjParser::readUv(std::string_view rest, std::string& message) {
    // A third number (w, for 3D textures) is ignored.
    std::array<float, gfx::UV_COMPONENTS> values{};
    if (!takeFloats(rest, values)) {
        message = "vt needs two numbers";
        return false;
    }
    uvs.emplace_back(values[0], values[1]);
    return true;
}

bool ObjParser::readNormal(std::string_view rest, std::string& message) {
    // Stored as written. The parser does not change the length of a normal.
    std::array<float, gfx::NORMAL_COMPONENTS> values{};
    if (!takeFloats(rest, values)) {
        message = "vn needs three numbers";
        return false;
    }
    normals.emplace_back(values[0], values[1], values[2]);
    return true;
}

bool ObjParser::readCorner(std::string_view corner, std::uint32_t& vertexIndex,
                           std::string& message) {
    // Split "position/uv/normal" at the slashes. fields[1] and fields[2] stay empty for
    // the shorter forms: "7" has one field, "7/3" two, and "7//2" three with an empty
    // one in the middle.
    std::array<std::string_view, MAX_CORNER_FIELDS> fields;
    std::size_t fieldCount = 0;
    std::string_view rest = corner;
    while (true) {
        if (fieldCount == MAX_CORNER_FIELDS) {
            message = "face corner '" + std::string(corner) + "' has more than three indices";
            return false;
        }
        const std::size_t separator = rest.find(INDEX_SEPARATOR);
        fields[fieldCount] = rest.substr(0, separator);
        ++fieldCount;
        if (separator == std::string_view::npos) {
            break;
        }
        rest.remove_prefix(separator + 1);
    }

    CornerKey key{NO_INDEX, NO_INDEX, NO_INDEX};
    // The position is required. The list sizes are the ones at this line of the file:
    // an index may only refer to an element defined above the face.
    if (!resolveIndex(fields[0], positions.size(), key[0])) {
        message = "face corner '" + std::string(corner) + "' has a bad position index (" +
                  std::to_string(positions.size()) + " positions defined so far)";
        return false;
    }
    if (!fields[1].empty() && !resolveIndex(fields[1], uvs.size(), key[1])) {
        message = "face corner '" + std::string(corner) + "' has a bad texture index (" +
                  std::to_string(uvs.size()) + " texture coordinates defined so far)";
        return false;
    }
    if (!fields[2].empty() && !resolveIndex(fields[2], normals.size(), key[2])) {
        message = "face corner '" + std::string(corner) + "' has a bad normal index (" +
                  std::to_string(normals.size()) + " normals defined so far)";
        return false;
    }

    // Has this exact combination been used before? Then the vertex exists already.
    const auto found = vertexOfCorner.find(key);
    if (found != vertexOfCorner.end()) {
        vertexIndex = found->second;
        return true;
    }

    // A new combination: build the vertex from the three lists. A missing uv or normal
    // stays at zero, the default of gfx::Vertex.
    gfx::Vertex vertex;
    vertex.position = positions[key[0]];
    if (key[1] != NO_INDEX) {
        vertex.uv = uvs[key[1]];
    }
    if (key[2] != NO_INDEX) {
        vertex.normal = normals[key[2]];
    }

    vertexIndex = static_cast<std::uint32_t>(model.vertices.size());
    model.vertices.push_back(vertex);
    vertexOfCorner.emplace(key, vertexIndex);
    return true;
}

bool ObjParser::readFace(std::string_view rest, std::string& message) {
    faceCorners.clear();
    for (std::string_view corner = takeToken(rest); !corner.empty(); corner = takeToken(rest)) {
        std::uint32_t vertexIndex = 0;
        if (!readCorner(corner, vertexIndex, message)) {
            return false;
        }
        faceCorners.push_back(vertexIndex);
    }
    if (faceCorners.size() < MIN_FACE_CORNERS) {
        message = "f needs at least three corners";
        return false;
    }

    // The first face after a change of material starts a new part. An usemtl line that
    // no face follows therefore creates nothing.
    if (model.parts.empty() || model.parts.back().material != currentMaterial) {
        ObjPart part;
        part.material = currentMaterial;
        part.firstIndex = static_cast<std::uint32_t>(model.indices.size());
        model.parts.push_back(part);
    }

    // Triangle fan: a polygon with the corners 0, 1, 2, ..., n - 1 becomes the triangles
    // (0, 1, 2), (0, 2, 3), ..., (0, n - 2, n - 1). Every triangle keeps the winding of
    // the polygon. A triangle (n = 3) goes through the loop once and is copied as it is.
    for (std::size_t i = 1; i + 1 < faceCorners.size(); ++i) {
        model.indices.push_back(faceCorners[0]);
        model.indices.push_back(faceCorners[i]);
        model.indices.push_back(faceCorners[i + 1]);
    }
    model.parts.back().indexCount =
        static_cast<std::uint32_t>(model.indices.size()) - model.parts.back().firstIndex;
    return true;
}

// Reads a whole file into text. Returns false when the file cannot be opened.
bool readFile(const std::filesystem::path& path, std::string& text) {
    // Binary mode: the bytes arrive as they are in the file, on every system. Line
    // endings are handled by the parser, not by the stream.
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    text = contents.str();
    return true;
}

// A path from text read out of a file. The text is taken as UTF-8 on every system:
// building a path from a plain std::string would use the code page of Windows instead.
std::filesystem::path pathFromText(std::string_view text) {
    const std::u8string utf8(text.begin(), text.end());
    return {utf8};
}

// True when keyword starts the normal map line of a material. The MTL format was written
// for bump maps (grey height pictures), and exporters reuse the same line for normal maps.
// Blender writes map_Bump. The other three spellings are in use too.
bool isNormalMapKeyword(std::string_view keyword) {
    return keyword == "map_Bump" || keyword == "map_bump" || keyword == "bump" || keyword == "norm";
}

// Reads the fields of a normal map line (the keyword is already cut off): an optional
// "-bm number" and then the file name. Returns false with the reason in message.
bool readNormalMap(std::string_view rest, std::filesystem::path& path, std::string& message) {
    // Look at the first field without losing rest: only "-bm" is taken away from it.
    std::string_view afterFirstField = rest;
    if (takeToken(afterFirstField) == BUMP_MULTIPLIER_OPTION) {
        float multiplier = 0.0F;
        if (!parseFloat(takeToken(afterFirstField), multiplier)) {
            message = "-bm needs a number";
            return false;
        }
        // The number is not used: the game has no setting for the strength of a map.
        rest = afterFirstField;
    }

    // The path is the rest of the line, so it may contain spaces. Other options in front
    // of the file name are not supported, as for map_Kd.
    const std::string_view fileName = trim(rest);
    if (fileName.empty()) {
        message = "needs a file name";
        return false;
    }
    path = pathFromText(fileName);
    return true;
}

// True when materials has a material with this name.
bool hasMaterial(const std::vector<ObjMaterial>& materials, std::string_view name) {
    for (const ObjMaterial& material : materials) {
        if (material.name == name) {
            return true;
        }
    }
    return false;
}

// The work of loadObj without the logging: on failure the reason is in error.
bool loadObjFiles(const std::filesystem::path& path, ObjModel& model, std::string& error) {
    std::string objText;
    if (!readFile(path, objText)) {
        error = "OBJ file cannot be opened: " + core::pathText(path);
        return false;
    }

    ObjModel loaded;
    std::string parseError;
    if (!parseObj(objText, loaded, parseError)) {
        error = core::pathText(path) + ": " + parseError;
        return false;
    }

    // mtllib names are relative to the directory of the OBJ file.
    const std::filesystem::path objDirectory = path.parent_path();
    for (const std::string& library : loaded.materialLibraries) {
        const std::filesystem::path mtlPath = objDirectory / pathFromText(library);
        std::string mtlText;
        if (!readFile(mtlPath, mtlText)) {
            error = "MTL file cannot be opened: " + core::pathText(mtlPath) + " (named by " +
                    core::pathText(path) + ")";
            return false;
        }

        std::vector<ObjMaterial> materials;
        if (!parseMtl(mtlText, materials, parseError)) {
            error = core::pathText(mtlPath) + ": " + parseError;
            return false;
        }

        // Texture paths are relative to the directory of the MTL file. lexically_normal
        // removes the ".." steps on paper, without asking the file system:
        // assets/models/../textures/wall_stone.png becomes assets/textures/wall_stone.png.
        const std::filesystem::path mtlDirectory = mtlPath.parent_path();
        for (ObjMaterial& material : materials) {
            if (!material.diffuseTexture.empty()) {
                material.diffuseTexture =
                    (mtlDirectory / material.diffuseTexture).lexically_normal();
            }
            if (!material.normalTexture.empty()) {
                material.normalTexture = (mtlDirectory / material.normalTexture).lexically_normal();
            }
            loaded.materials.push_back(std::move(material));
        }
    }

    // A part that names a material nobody defined could only be drawn with a guess.
    for (const ObjPart& part : loaded.parts) {
        if (!part.material.empty() && !hasMaterial(loaded.materials, part.material)) {
            error = core::pathText(path) + ": material '" + part.material +
                    "' is used but not defined in any material library";
            return false;
        }
    }

    model = std::move(loaded);
    return true;
}

} // namespace

bool parseObj(std::string_view text, ObjModel& model, std::string& error) {
    ObjParser parser;
    std::size_t lineNumber = 0;
    std::string_view rest = text;

    while (!rest.empty()) {
        ++lineNumber;
        // Work on the line without its comment. takeToken skips the blanks, including
        // the '\r' of a Windows line ending.
        std::string_view line = withoutComment(takeLine(rest));
        const std::string_view keyword = takeToken(line);

        std::string message;
        bool ok = true;
        if (keyword.empty() || keyword == "o" || keyword == "g" || keyword == "s") {
            // Nothing to do. An empty keyword is a blank line or a line with only a
            // comment. o, g and s (object name, group name, smoothing group) are known and
            // not needed: the whole file becomes one mesh, and normals are taken from the
            // vn lines as they are.
        } else if (keyword == "v") {
            ok = parser.readPosition(line, message);
        } else if (keyword == "vt") {
            ok = parser.readUv(line, message);
        } else if (keyword == "vn") {
            ok = parser.readNormal(line, message);
        } else if (keyword == "f") {
            ok = parser.readFace(line, message);
        } else if (keyword == "usemtl") {
            // The name is the rest of the line, so it may contain spaces.
            parser.currentMaterial = std::string(trim(line));
            if (parser.currentMaterial.empty()) {
                message = "usemtl needs a material name";
                ok = false;
            }
        } else if (keyword == "mtllib") {
            const std::string_view fileName = trim(line);
            if (fileName.empty()) {
                message = "mtllib needs a file name";
                ok = false;
            } else {
                parser.model.materialLibraries.emplace_back(fileName);
            }
        } else {
            // A keyword this parser does not know (for example "l" for lines). Skipped,
            // but counted, so that the caller can tell that something was left out.
            ++parser.model.unknownLineCount;
        }

        if (!ok) {
            error = lineError(lineNumber, message);
            return false;
        }
    }

    // An OBJ file has no tangents, so they are computed now that all triangles are known.
    // The count of mirrored triangles tells the caller whether the tangents are enough
    // for a normal map (see ObjModel::mirroredTriangleCount).
    computeTangents(parser.model.vertices, parser.model.indices);
    parser.model.mirroredTriangleCount =
        countMirroredTriangles(parser.model.vertices, parser.model.indices);

    model = std::move(parser.model);
    return true;
}

bool parseMtl(std::string_view text, std::vector<ObjMaterial>& materials, std::string& error) {
    // Collected here first, so that materials is untouched when a later line is bad.
    std::vector<ObjMaterial> parsed;
    std::size_t lineNumber = 0;
    std::string_view rest = text;

    while (!rest.empty()) {
        ++lineNumber;
        std::string_view line = withoutComment(takeLine(rest));
        const std::string_view keyword = takeToken(line);

        if (keyword == "newmtl") {
            // Starts a new material. The lines that follow describe it.
            ObjMaterial material;
            material.name = std::string(trim(line));
            if (material.name.empty()) {
                error = lineError(lineNumber, "newmtl needs a material name");
                return false;
            }
            parsed.push_back(std::move(material));
        } else if (keyword == "Kd" || keyword == "map_Kd" || isNormalMapKeyword(keyword)) {
            if (parsed.empty()) {
                error = lineError(lineNumber, std::string(keyword) + " before the first newmtl");
                return false;
            }
            ObjMaterial& material = parsed.back();
            if (isNormalMapKeyword(keyword)) {
                std::string message;
                if (!readNormalMap(line, material.normalTexture, message)) {
                    error = lineError(lineNumber, std::string(keyword) + " " + message);
                    return false;
                }
            } else if (keyword == "Kd") {
                // Three numbers from 0 to 1: red, green, blue.
                std::array<float, COLOR_COMPONENTS> values{};
                if (!takeFloats(line, values)) {
                    error = lineError(lineNumber, "Kd needs three numbers");
                    return false;
                }
                material.diffuseColor = {values[0], values[1], values[2]};
            } else {
                // The path is the rest of the line, so it may contain spaces. Options
                // in front of the file name (for example "-s 2 2 1") are not supported.
                const std::string_view fileName = trim(line);
                if (fileName.empty()) {
                    error = lineError(lineNumber, "map_Kd needs a file name");
                    return false;
                }
                material.diffuseTexture = pathFromText(fileName);
            }
        }
        // Every other line (Ns, Ka, Ks, Ke, Ni, d, illum, blank, unknown) is skipped.
    }

    for (ObjMaterial& material : parsed) {
        materials.push_back(std::move(material));
    }
    return true;
}

bool loadObj(const std::filesystem::path& path, ObjModel& model, std::string& error) {
    if (!loadObjFiles(path, model, error)) {
        // The one place that logs, so that every failure appears in the log exactly once.
        core::logError(error);
        return false;
    }

    if (model.unknownLineCount > 0) {
        core::logWarn(core::pathText(path) + ": skipped " + std::to_string(model.unknownLineCount) +
                      " line(s) with an unknown keyword");
    }
    if (model.mirroredTriangleCount > 0) {
        core::logWarn(core::pathText(path) + ": " + std::to_string(model.mirroredTriangleCount) +
                      " triangle(s) have a mirrored texture, a normal map is upside down there");
    }
    return true;
}

} // namespace assets
