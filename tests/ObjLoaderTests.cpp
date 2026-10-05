// Tests of assets::parseObj, assets::parseMtl and assets::loadObj.
// See docs/modules/assets/obj-loader.md
#include "assets/ObjLoader.hpp"

#include "assets/Tangents.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

// The assets directory of the repository. NIGHT_MAZE_ASSETS_DIR is given to this program
// by CMake as a compile definition, so the tests read the real files of the game and do
// not depend on the directory they are started from.
std::filesystem::path assetsDirectory() {
    return {NIGHT_MAZE_ASSETS_DIR};
}

std::filesystem::path modelsDirectory() {
    return assetsDirectory() / "models";
}

// Writes a small text file for a test, creating its directory when needed.
void writeFile(const std::filesystem::path& path, std::string_view text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
    REQUIRE(file.good());
}

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

void checkVec2(const glm::vec2& actual, const glm::vec2& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
}

// Parses text that is expected to be valid and returns the model.
assets::ObjModel parseValid(std::string_view text) {
    assets::ObjModel model;
    std::string error;
    const bool ok = assets::parseObj(text, model, error);
    CHECK(ok);
    CHECK(error.empty());
    return model;
}

// Parses text that is expected to be rejected and returns the error message.
std::string parseInvalid(std::string_view text) {
    assets::ObjModel model;
    std::string error;
    const bool ok = assets::parseObj(text, model, error);
    CHECK_FALSE(ok);
    // A rejected text leaves the model as it was: empty.
    CHECK(model.vertices.empty());
    CHECK(model.indices.empty());
    CHECK(model.parts.empty());
    return error;
}

// True when message starts with prefix. The line number is at the start of a message.
bool startsWith(const std::string& message, std::string_view prefix) {
    return std::string_view(message).starts_with(prefix);
}

// A square in the XY plane made of two triangles, in the form Blender writes: 4
// positions, 4 texture coordinates and 1 normal shared by all corners.
constexpr std::string_view SQUARE = "v 0 0 0\n"
                                    "v 1 0 0\n"
                                    "v 1 1 0\n"
                                    "v 0 1 0\n"
                                    "vn 0 0 1\n"
                                    "vt 0 0\n"
                                    "vt 1 0\n"
                                    "vt 1 1\n"
                                    "vt 0 1\n"
                                    "f 1/1/1 2/2/1 3/3/1\n"
                                    "f 1/1/1 3/3/1 4/4/1\n";

// Smallest and largest coordinate of all vertices of a model.
struct Bounds {
    glm::vec3 min{0.0F};
    glm::vec3 max{0.0F};
};

Bounds boundsOf(const assets::ObjModel& model) {
    REQUIRE_FALSE(model.vertices.empty());
    Bounds bounds{.min = model.vertices.front().position, .max = model.vertices.front().position};
    for (const gfx::Vertex& vertex : model.vertices) {
        bounds.min = glm::min(bounds.min, vertex.position);
        bounds.max = glm::max(bounds.max, vertex.position);
    }
    return bounds;
}

// Loads one of the real models of the game and checks what all three have in common:
// one part with one material, unit normals, valid indices, a texture file and a normal
// map file that exist, and tangents that fit the normals and the texture coordinates.
assets::ObjModel loadGameModel(const char* fileName, const char* materialName,
                               const char* textureFileName, const char* normalMapFileName) {
    assets::ObjModel model;
    std::string error;
    const bool ok = assets::loadObj(modelsDirectory() / fileName, model, error);
    CAPTURE(error);
    REQUIRE(ok);
    CHECK(error.empty());
    CHECK(model.unknownLineCount == 0U);

    // Every index points at an existing vertex.
    for (const std::uint32_t index : model.indices) {
        CHECK(index < model.vertices.size());
    }

    // The normals come from the file unchanged, and the file has unit normals.
    for (const gfx::Vertex& vertex : model.vertices) {
        CHECK(glm::length(vertex.normal) == doctest::Approx(1.0F));
    }

    // One usemtl line, so one part that covers all indices.
    REQUIRE(model.parts.size() == 1U);
    CHECK(model.parts[0].material == materialName);
    CHECK(model.parts[0].firstIndex == 0U);
    CHECK(model.parts[0].indexCount == model.indices.size());

    REQUIRE(model.materials.size() == 1U);
    const assets::ObjMaterial& material = model.materials[0];
    CHECK(material.name == materialName);
    checkVec3(material.diffuseColor, {1.0F, 1.0F, 1.0F});
    // "../textures/x.png" next to assets/models became assets/textures/x.png.
    CHECK(material.diffuseTexture.filename() == textureFileName);
    CHECK(material.diffuseTexture ==
          (assetsDirectory() / "textures" / textureFileName).lexically_normal());
    CHECK(std::filesystem::exists(material.diffuseTexture));
    // The normal map is named by the map_Bump line and resolved the same way.
    CHECK(material.normalTexture ==
          (assetsDirectory() / "textures" / normalMapFileName).lexically_normal());
    CHECK(std::filesystem::exists(material.normalTexture));

    // The tangents are computed by the loader: length 1 and perpendicular to the normal.
    for (const gfx::Vertex& vertex : model.vertices) {
        CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
        CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));

        // The models are boxes, so every face is either vertical or horizontal. On
        // a vertical face the texture stands upright: the bitangent the shader builds,
        // cross(N, T), must point up, where v grows. A tangent the wrong way round would
        // give "down" here and turn the joints of the normal map into ridges.
        const bool vertical = std::abs(vertex.normal.y) < 0.5F;
        if (vertical) {
            checkVec3(glm::cross(vertex.normal, vertex.tangent), {0.0F, 1.0F, 0.0F});
        }
    }

    // No face has a mirrored texture, although box_project_uvs flips u on opposite sides:
    // the flip is exactly what keeps the texture readable from outside on every side.
    // This is why a vertex needs no handedness sign. Counted by the loader and again
    // here, directly.
    CHECK(model.mirroredTriangleCount == 0U);
    CHECK(assets::countMirroredTriangles(model.vertices, model.indices) == 0U);
    return model;
}

} // namespace

TEST_CASE("parseObj: identical position/uv/normal triples share one vertex") {
    const assets::ObjModel model = parseValid(SQUARE);

    // 6 corners in the file, but only 4 different triples: 1/1/1 and 3/3/1 are used by
    // both triangles.
    CHECK(model.vertices.size() == 4U);
    CHECK(model.indices == std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3});

    // Vertices are numbered in the order their triples first appear.
    checkVec3(model.vertices[1].position, {1.0F, 0.0F, 0.0F});
    checkVec2(model.vertices[1].uv, {1.0F, 0.0F});
    checkVec3(model.vertices[1].normal, {0.0F, 0.0F, 1.0F});
    checkVec3(model.vertices[3].position, {0.0F, 1.0F, 0.0F});
    checkVec2(model.vertices[3].uv, {0.0F, 1.0F});
}

TEST_CASE("parseObj: the same position with another uv or normal is another vertex") {
    // One position list, but the two triangles disagree about the uv and the normal of
    // the corners they share. Nothing can be shared: 6 vertices.
    const assets::ObjModel model = parseValid("v 0 0 0\n"
                                              "v 1 0 0\n"
                                              "v 1 1 0\n"
                                              "v 0 1 0\n"
                                              "vt 0 0\n"
                                              "vt 1 1\n"
                                              "vn 0 0 1\n"
                                              "vn 0 1 0\n"
                                              "f 1/1/1 2/1/1 3/1/1\n"
                                              "f 1/2/1 3/1/2 4/1/1\n");

    CHECK(model.vertices.size() == 6U);
    CHECK(model.indices == std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5});
    // Corner 1/2/1: same position as vertex 0, different uv.
    checkVec3(model.vertices[3].position, model.vertices[0].position);
    checkVec2(model.vertices[3].uv, {1.0F, 1.0F});
    // Corner 3/1/2: same position and uv as vertex 2, different normal.
    checkVec3(model.vertices[4].position, model.vertices[2].position);
    checkVec3(model.vertices[4].normal, {0.0F, 1.0F, 0.0F});
}

TEST_CASE("parseObj: the four forms of a face corner") {
    const std::string lists = "v 0 0 0\n"
                              "v 1 0 0\n"
                              "v 0 1 0\n"
                              "vt 0.25 0.75\n"
                              "vn 0 0 1\n";

    SUBCASE("v/vt/vn: everything") {
        const assets::ObjModel model = parseValid(lists + "f 1/1/1 2/1/1 3/1/1\n");
        REQUIRE(model.vertices.size() == 3U);
        checkVec2(model.vertices[0].uv, {0.25F, 0.75F});
        checkVec3(model.vertices[0].normal, {0.0F, 0.0F, 1.0F});
    }

    SUBCASE("v//vn: no uv, so the uv is zero") {
        const assets::ObjModel model = parseValid(lists + "f 1//1 2//1 3//1\n");
        REQUIRE(model.vertices.size() == 3U);
        checkVec2(model.vertices[0].uv, {0.0F, 0.0F});
        checkVec3(model.vertices[0].normal, {0.0F, 0.0F, 1.0F});
    }

    SUBCASE("v/vt: no normal, so the normal is zero") {
        const assets::ObjModel model = parseValid(lists + "f 1/1 2/1 3/1\n");
        REQUIRE(model.vertices.size() == 3U);
        checkVec2(model.vertices[0].uv, {0.25F, 0.75F});
        checkVec3(model.vertices[0].normal, {0.0F, 0.0F, 0.0F});
    }

    SUBCASE("v: position only") {
        const assets::ObjModel model = parseValid(lists + "f 1 2 3\n");
        REQUIRE(model.vertices.size() == 3U);
        checkVec3(model.vertices[2].position, {0.0F, 1.0F, 0.0F});
        checkVec2(model.vertices[2].uv, {0.0F, 0.0F});
        checkVec3(model.vertices[2].normal, {0.0F, 0.0F, 0.0F});
    }

    SUBCASE("a corner without a uv is not the same vertex as one with a uv") {
        const assets::ObjModel model = parseValid(lists + "f 1 2 3\nf 1/1 2 3\n");
        // Only the first corner of the second face is new.
        CHECK(model.vertices.size() == 4U);
        CHECK(model.indices == std::vector<std::uint32_t>{0, 1, 2, 3, 1, 2});
    }
}

TEST_CASE("parseObj: negative indices count back from the end of the list so far") {
    SUBCASE("-1 is the element defined last") {
        const assets::ObjModel model = parseValid("v 0 0 0\n"
                                                  "v 1 0 0\n"
                                                  "v 0 1 0\n"
                                                  "vt 0.5 0.5\n"
                                                  "vn 0 0 1\n"
                                                  "f -3/-1/-1 -2/-1/-1 -1/-1/-1\n");
        REQUIRE(model.vertices.size() == 3U);
        checkVec3(model.vertices[0].position, {0.0F, 0.0F, 0.0F});
        checkVec3(model.vertices[2].position, {0.0F, 1.0F, 0.0F});
        checkVec2(model.vertices[2].uv, {0.5F, 0.5F});
    }

    SUBCASE("a negative and a positive index of the same element give the same vertex") {
        const assets::ObjModel model = parseValid("v 0 0 0\n"
                                                  "v 1 0 0\n"
                                                  "v 0 1 0\n"
                                                  "f 1 2 3\n"
                                                  "f -3 -2 -1\n");
        CHECK(model.vertices.size() == 3U);
        CHECK(model.indices == std::vector<std::uint32_t>{0, 1, 2, 0, 1, 2});
    }

    SUBCASE("the end of the list is where the face stands, not the end of the file") {
        // The second face is written after three more positions, so its -1 is another
        // position than the -1 of the first face.
        const assets::ObjModel model = parseValid("v 0 0 0\n"
                                                  "v 1 0 0\n"
                                                  "v 0 1 0\n"
                                                  "f -3 -2 -1\n"
                                                  "v 0 0 5\n"
                                                  "v 1 0 5\n"
                                                  "v 0 1 5\n"
                                                  "f -3 -2 -1\n");
        REQUIRE(model.vertices.size() == 6U);
        checkVec3(model.vertices[2].position, {0.0F, 1.0F, 0.0F});
        checkVec3(model.vertices[5].position, {0.0F, 1.0F, 5.0F});
    }
}

TEST_CASE("parseObj: a polygon is split into a fan of triangles") {
    SUBCASE("a quad becomes two triangles that keep its winding") {
        const assets::ObjModel model = parseValid("v 0 0 0\n"
                                                  "v 1 0 0\n"
                                                  "v 1 1 0\n"
                                                  "v 0 1 0\n"
                                                  "f 1 2 3 4\n");
        CHECK(model.vertices.size() == 4U);
        CHECK(model.indices == std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3});
    }

    SUBCASE("a pentagon becomes three triangles") {
        const assets::ObjModel model = parseValid("v 0 0 0\n"
                                                  "v 2 0 0\n"
                                                  "v 3 1 0\n"
                                                  "v 1 2 0\n"
                                                  "v -1 1 0\n"
                                                  "f 1 2 3 4 5\n");
        CHECK(model.vertices.size() == 5U);
        CHECK(model.indices == std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3, 0, 3, 4});
    }
}

TEST_CASE("parseObj: the tangents are computed from the positions and the uvs") {
    SUBCASE("a square with an upright texture") {
        const assets::ObjModel model = parseValid(SQUARE);

        // u grows along +X on the square, and its normal is +Z.
        for (const gfx::Vertex& vertex : model.vertices) {
            checkVec3(vertex.tangent, {1.0F, 0.0F, 0.0F});
        }
        CHECK(model.mirroredTriangleCount == 0U);
    }

    SUBCASE("a model without uvs still gets tangents of length 1") {
        const assets::ObjModel model =
            parseValid("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//1\n");

        for (const gfx::Vertex& vertex : model.vertices) {
            CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
            CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));
        }
        CHECK(model.mirroredTriangleCount == 0U);
    }

    SUBCASE("a mirrored texture is counted") {
        // The square with the u of every corner flipped (1 - u).
        const assets::ObjModel model = parseValid("v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
                                                  "vn 0 0 1\n"
                                                  "vt 1 0\nvt 0 0\nvt 0 1\nvt 1 1\n"
                                                  "f 1/1/1 2/2/1 3/3/1\n"
                                                  "f 1/1/1 3/3/1 4/4/1\n");

        CHECK(model.mirroredTriangleCount == 2U);
    }
}

TEST_CASE("parseObj: line endings, blanks and comments") {
    const std::vector<std::uint32_t> expectedIndices{0, 1, 2, 0, 2, 3};

    SUBCASE("Windows line endings (CRLF)") {
        const assets::ObjModel model = parseValid("v 0 0 0\r\n"
                                                  "v 1 0 0\r\n"
                                                  "v 1 1 0\r\n"
                                                  "v 0 1 0\r\n"
                                                  "usemtl stone\r\n"
                                                  "f 1 2 3\r\n"
                                                  "f 1 3 4\r\n");
        CHECK(model.vertices.size() == 4U);
        CHECK(model.indices == expectedIndices);
        // The '\r' did not end up in the material name.
        REQUIRE(model.parts.size() == 1U);
        CHECK(model.parts[0].material == "stone");
    }

    SUBCASE("tabs, repeated spaces, leading and trailing blanks") {
        const assets::ObjModel model = parseValid("  v   0\t0  0   \n"
                                                  "\tv 1 0 0\t\n"
                                                  "v 1   1 0\n"
                                                  "v 0 1 0\n"
                                                  "f  1   2\t3  \n"
                                                  "f 1 3 4 \t \n");
        CHECK(model.vertices.size() == 4U);
        CHECK(model.indices == expectedIndices);
    }

    SUBCASE("no line break after the last line") {
        const assets::ObjModel model = parseValid("v 0 0 0\n"
                                                  "v 1 0 0\n"
                                                  "v 1 1 0\n"
                                                  "v 0 1 0\n"
                                                  "f 1 2 3\n"
                                                  "f 1 3 4");
        CHECK(model.indices == expectedIndices);
    }

    SUBCASE("comments and blank lines") {
        const assets::ObjModel model = parseValid("# a comment\n"
                                                  "\n"
                                                  "   # an indented comment\n"
                                                  "v 0 0 0 # the origin\n"
                                                  "v 1 0 0\n"
                                                  "\r\n"
                                                  "v 1 1 0\n"
                                                  "v 0 1 0\n"
                                                  "f 1 2 3 # first half\n"
                                                  "f 1 3 4\n");
        CHECK(model.vertices.size() == 4U);
        CHECK(model.indices == expectedIndices);
        CHECK(model.unknownLineCount == 0U);
    }

    SUBCASE("an empty text is an empty model") {
        const assets::ObjModel model = parseValid("");
        CHECK(model.vertices.empty());
        CHECK(model.indices.empty());
        CHECK(model.parts.empty());
    }
}

TEST_CASE("parseObj: numbers") {
    SUBCASE("signs, fractions, a negative zero and an exponent") {
        const assets::ObjModel model = parseValid("v -1.500000 0.25 +2\n"
                                                  "v -0.0000 1e-3 2.5E2\n"
                                                  "v 0 0 0\n"
                                                  "f 1 2 3\n");
        REQUIRE(model.vertices.size() == 3U);
        checkVec3(model.vertices[0].position, {-1.5F, 0.25F, 2.0F});
        checkVec3(model.vertices[1].position, {0.0F, 0.001F, 250.0F});
    }

    SUBCASE("numbers after the ones that are needed are ignored") {
        // A fourth coordinate (w) on v, a third one on vt.
        const assets::ObjModel model = parseValid("v 1 2 3 1.0\n"
                                                  "v 0 0 0 1.0\n"
                                                  "v 0 1 0 1.0\n"
                                                  "vt 0.5 0.25 0.0\n"
                                                  "f 1/1 2/1 3/1\n");
        REQUIRE(model.vertices.size() == 3U);
        checkVec3(model.vertices[0].position, {1.0F, 2.0F, 3.0F});
        checkVec2(model.vertices[0].uv, {0.5F, 0.25F});
    }
}

TEST_CASE("parseObj: every run of faces after usemtl is one part") {
    const std::string lists = "v 0 0 0\n"
                              "v 1 0 0\n"
                              "v 1 1 0\n"
                              "v 0 1 0\n";

    SUBCASE("two materials give two parts with their index ranges") {
        const assets::ObjModel model = parseValid(lists + "usemtl stone\n"
                                                          "f 1 2 3\n"
                                                          "f 1 3 4\n"
                                                          "usemtl wood\n"
                                                          "f 1 2 4\n");
        REQUIRE(model.parts.size() == 2U);
        CHECK(model.parts[0].material == "stone");
        CHECK(model.parts[0].firstIndex == 0U);
        CHECK(model.parts[0].indexCount == 6U);
        CHECK(model.parts[1].material == "wood");
        CHECK(model.parts[1].firstIndex == 6U);
        CHECK(model.parts[1].indexCount == 3U);
        CHECK(model.indices.size() == 9U);
    }

    SUBCASE("a quad counts as two triangles in the range of its part") {
        const assets::ObjModel model = parseValid(lists + "usemtl stone\n"
                                                          "f 1 2 3 4\n"
                                                          "usemtl wood\n"
                                                          "f 1 2 4\n");
        REQUIRE(model.parts.size() == 2U);
        CHECK(model.parts[0].indexCount == 6U);
        CHECK(model.parts[1].firstIndex == 6U);
    }

    SUBCASE("faces before the first usemtl form a part without a material") {
        const assets::ObjModel model = parseValid(lists + "f 1 2 3\n"
                                                          "usemtl stone\n"
                                                          "f 1 3 4\n");
        REQUIRE(model.parts.size() == 2U);
        CHECK(model.parts[0].material.empty());
        CHECK(model.parts[0].indexCount == 3U);
        CHECK(model.parts[1].material == "stone");
        CHECK(model.parts[1].firstIndex == 3U);
    }

    SUBCASE("usemtl without faces after it creates no part") {
        const assets::ObjModel model = parseValid(lists + "usemtl unused\n"
                                                          "usemtl stone\n"
                                                          "f 1 2 3\n"
                                                          "usemtl trailing\n");
        REQUIRE(model.parts.size() == 1U);
        CHECK(model.parts[0].material == "stone");
        CHECK(model.parts[0].indexCount == 3U);
    }

    SUBCASE("the same material again right away continues the part") {
        const assets::ObjModel model = parseValid(lists + "usemtl stone\n"
                                                          "f 1 2 3\n"
                                                          "usemtl stone\n"
                                                          "f 1 3 4\n");
        REQUIRE(model.parts.size() == 1U);
        CHECK(model.parts[0].indexCount == 6U);
    }

    SUBCASE("a material that comes back after another one is a new part") {
        const assets::ObjModel model = parseValid(lists + "usemtl stone\n"
                                                          "f 1 2 3\n"
                                                          "usemtl wood\n"
                                                          "f 1 3 4\n"
                                                          "usemtl stone\n"
                                                          "f 1 2 4\n");
        REQUIRE(model.parts.size() == 3U);
        CHECK(model.parts[2].material == "stone");
        CHECK(model.parts[2].firstIndex == 6U);
        CHECK(model.parts[2].indexCount == 3U);
    }

    SUBCASE("a material name may contain spaces") {
        const assets::ObjModel model = parseValid(lists + "usemtl old stone  \n"
                                                          "f 1 2 3\n");
        REQUIRE(model.parts.size() == 1U);
        CHECK(model.parts[0].material == "old stone");
    }
}

TEST_CASE("parseObj: mtllib names are collected, other keywords are skipped") {
    const assets::ObjModel model = parseValid("mtllib walls.mtl\n"
                                              "mtllib my floor.mtl\n"
                                              "o wall\n"
                                              "g group\n"
                                              "s 0\n"
                                              "v 0 0 0\n"
                                              "v 1 0 0\n"
                                              "v 0 1 0\n"
                                              "l 1 2\n"
                                              "curv 0 1 1 2\n"
                                              "f 1 2 3\n");

    REQUIRE(model.materialLibraries.size() == 2U);
    CHECK(model.materialLibraries[0] == "walls.mtl");
    CHECK(model.materialLibraries[1] == "my floor.mtl");
    // parseObj reads no file, so it has no materials.
    CHECK(model.materials.empty());
    // o, g and s are known and ignored. l and curv are not known: counted.
    CHECK(model.unknownLineCount == 2U);
    CHECK(model.indices.size() == 3U);
}

TEST_CASE("parseObj: a bad line is reported with its line number") {
    const std::string lists = "v 0 0 0\n"
                              "v 1 0 0\n"
                              "v 0 1 0\n"
                              "vt 0 0\n"
                              "vn 0 0 1\n";

    SUBCASE("a position index past the end of the list") {
        const std::string error = parseInvalid(lists + "f 1 2 4\n");
        CHECK(error == "line 6: face corner '4' has a bad position index (3 positions "
                       "defined so far)");
    }

    SUBCASE("index 0 does not exist: indices start at 1") {
        const std::string error = parseInvalid(lists + "f 0 1 2\n");
        CHECK(startsWith(error, "line 6: face corner '0' has a bad position index"));
    }

    SUBCASE("a negative index that reaches before the first element") {
        const std::string error = parseInvalid(lists + "f -4 -2 -1\n");
        CHECK(startsWith(error, "line 6: face corner '-4' has a bad position index"));
    }

    SUBCASE("a texture index past the end of the list") {
        const std::string error = parseInvalid(lists + "f 1/1/1 2/2/1 3/1/1\n");
        CHECK(startsWith(error, "line 6: face corner '2/2/1' has a bad texture index"));
    }

    SUBCASE("a normal index past the end of the list") {
        const std::string error = parseInvalid(lists + "f 1//1 2//1 3//2\n");
        CHECK(startsWith(error, "line 6: face corner '3//2' has a bad normal index"));
    }

    SUBCASE("a face that refers to a position defined below it") {
        const std::string error = parseInvalid("v 0 0 0\nv 1 0 0\nf 1 2 3\nv 0 1 0\n");
        CHECK(startsWith(error, "line 3: face corner '3' has a bad position index"));
    }

    SUBCASE("an index that is not an integer") {
        CHECK(startsWith(parseInvalid(lists + "f 1 2 x\n"), "line 6: face corner 'x'"));
        CHECK(startsWith(parseInvalid(lists + "f 1 2 3.5\n"), "line 6: face corner '3.5'"));
        CHECK(startsWith(parseInvalid(lists + "f 1 2 3a\n"), "line 6: face corner '3a'"));
        CHECK(startsWith(parseInvalid(lists + "f 1 2 99999999999999999999\n"),
                         "line 6: face corner '99999999999999999999'"));
    }

    SUBCASE("a corner without a position index") {
        CHECK(startsWith(parseInvalid(lists + "f /1/1 2 3\n"), "line 6: face corner '/1/1'"));
    }

    SUBCASE("a corner with four indices") {
        const std::string error = parseInvalid(lists + "f 1/1/1/1 2 3\n");
        CHECK(error == "line 6: face corner '1/1/1/1' has more than three indices");
    }

    SUBCASE("a face with fewer than three corners") {
        CHECK(parseInvalid(lists + "f 1 2\n") == "line 6: f needs at least three corners");
        CHECK(parseInvalid(lists + "f\n") == "line 6: f needs at least three corners");
    }

    SUBCASE("a number that is not a number") {
        CHECK(parseInvalid("v 0 0 0\nv 1 abc 0\n") == "line 2: v needs three numbers");
        CHECK(parseInvalid("v 0 0 1.5x\n") == "line 1: v needs three numbers");
        CHECK(parseInvalid("vn 0 0 --1\n") == "line 1: vn needs three numbers");
        CHECK(parseInvalid("vt 0.5 0,5\n") == "line 1: vt needs two numbers");
    }

    SUBCASE("too few numbers") {
        CHECK(parseInvalid("v 0 0\n") == "line 1: v needs three numbers");
        CHECK(parseInvalid("vn 0 0\n") == "line 1: vn needs three numbers");
        CHECK(parseInvalid("vt 0\n") == "line 1: vt needs two numbers");
    }

    SUBCASE("usemtl and mtllib without a name") {
        CHECK(parseInvalid("usemtl\n") == "line 1: usemtl needs a material name");
        CHECK(parseInvalid("mtllib   \n") == "line 1: mtllib needs a file name");
    }

    SUBCASE("blank lines and comments are counted as lines, also with CRLF") {
        const std::string error = parseInvalid("# comment\r\n\r\nv 0 0 0\r\nv 1 0\r\n");
        CHECK(error == "line 4: v needs three numbers");
    }
}

TEST_CASE("parseObj: a failed parse leaves the model of the caller unchanged") {
    assets::ObjModel model = parseValid(SQUARE);
    std::string error;

    CHECK_FALSE(assets::parseObj("v 0 0 0\nf 1 2 3\n", model, error));

    CHECK_FALSE(error.empty());
    CHECK(model.vertices.size() == 4U);
    CHECK(model.indices.size() == 6U);
}

TEST_CASE("parseMtl: newmtl, Kd and map_Kd") {
    std::vector<assets::ObjMaterial> materials;
    std::string error;

    SUBCASE("a material as Blender writes it") {
        const bool ok = assets::parseMtl("# Blender 5.2.1 LTS MTL File: 'None'\n"
                                         "# www.blender.org\n"
                                         "\n"
                                         "newmtl wall_stone\n"
                                         "Ns 250.000000\n"
                                         "Ka 1.000000 1.000000 1.000000\n"
                                         "Ks 0.500000 0.500000 0.500000\n"
                                         "Ke 0.000000 0.000000 0.000000\n"
                                         "Ni 1.500000\n"
                                         "d 1.000000\n"
                                         "illum 2\n"
                                         "Kd 0.800000 0.400000 0.200000\n"
                                         "map_Kd ../textures/wall_stone.png\n",
                                         materials, error);
        REQUIRE(ok);
        REQUIRE(materials.size() == 1U);
        CHECK(materials[0].name == "wall_stone");
        checkVec3(materials[0].diffuseColor, {0.8F, 0.4F, 0.2F});
        // parseMtl keeps the path as written: resolving it is the job of loadObj.
        CHECK(materials[0].diffuseTexture == std::filesystem::path("../textures/wall_stone.png"));
        // No normal map line, so no normal map.
        CHECK(materials[0].normalTexture.empty());
    }

    SUBCASE("several materials, CRLF, no final line break") {
        const bool ok = assets::parseMtl("newmtl red\r\n"
                                         "Kd 1 0 0\r\n"
                                         "newmtl textured\r\n"
                                         "map_Kd stone.png\r\n"
                                         "newmtl plain",
                                         materials, error);
        REQUIRE(ok);
        REQUIRE(materials.size() == 3U);
        CHECK(materials[0].name == "red");
        checkVec3(materials[0].diffuseColor, {1.0F, 0.0F, 0.0F});
        CHECK(materials[0].diffuseTexture.empty());
        // No Kd line: the colour is white. The '\r' is not part of the file name.
        CHECK(materials[1].name == "textured");
        checkVec3(materials[1].diffuseColor, {1.0F, 1.0F, 1.0F});
        CHECK(materials[1].diffuseTexture == std::filesystem::path("stone.png"));
        CHECK(materials[2].name == "plain");
        CHECK(materials[2].diffuseTexture.empty());
    }

    SUBCASE("a texture path may contain spaces") {
        REQUIRE(
            assets::parseMtl("newmtl m\nmap_Kd my textures/old stone.png  \n", materials, error));
        REQUIRE(materials.size() == 1U);
        CHECK(materials[0].diffuseTexture == std::filesystem::path("my textures/old stone.png"));
    }

    SUBCASE("materials are appended to the ones already in the list") {
        REQUIRE(assets::parseMtl("newmtl first\n", materials, error));
        REQUIRE(assets::parseMtl("newmtl second\n", materials, error));
        REQUIRE(materials.size() == 2U);
        CHECK(materials[0].name == "first");
        CHECK(materials[1].name == "second");
    }

    SUBCASE("an empty text has no materials") {
        CHECK(assets::parseMtl("", materials, error));
        CHECK(materials.empty());
    }
}

TEST_CASE("parseMtl: the normal map line") {
    std::vector<assets::ObjMaterial> materials;
    std::string error;

    SUBCASE("as Blender writes it: map_Bump with the option -bm") {
        REQUIRE(assets::parseMtl("newmtl wall_stone\n"
                                 "Kd 1.000000 1.000000 1.000000\n"
                                 "map_Kd ../textures/wall_stone.png\n"
                                 "map_Bump -bm 1.000000 ../textures/wall_stone_normal.png\n",
                                 materials, error));
        REQUIRE(materials.size() == 1U);
        CHECK(materials[0].diffuseTexture == std::filesystem::path("../textures/wall_stone.png"));
        // The path is kept as written, like the one of map_Kd.
        CHECK(materials[0].normalTexture ==
              std::filesystem::path("../textures/wall_stone_normal.png"));
    }

    SUBCASE("without the option") {
        REQUIRE(assets::parseMtl("newmtl m\nmap_Bump stone_normal.png\n", materials, error));
        CHECK(materials[0].normalTexture == std::filesystem::path("stone_normal.png"));
    }

    SUBCASE("the other spellings of the keyword: map_bump, bump and norm") {
        REQUIRE(assets::parseMtl("newmtl a\nmap_bump a.png\n"
                                 "newmtl b\nbump -bm 0.5 b.png\n"
                                 "newmtl c\nnorm c.png\n",
                                 materials, error));
        REQUIRE(materials.size() == 3U);
        CHECK(materials[0].normalTexture == std::filesystem::path("a.png"));
        CHECK(materials[1].normalTexture == std::filesystem::path("b.png"));
        CHECK(materials[2].normalTexture == std::filesystem::path("c.png"));
    }

    SUBCASE("CRLF, tabs and a path with spaces") {
        REQUIRE(assets::parseMtl("newmtl m\r\nmap_Bump\t-bm  2  my maps/old stone n.png  \r\n",
                                 materials, error));
        CHECK(materials[0].normalTexture == std::filesystem::path("my maps/old stone n.png"));
    }

    SUBCASE("a material with a normal map and no colour picture") {
        REQUIRE(assets::parseMtl("newmtl m\nmap_Bump n.png\n", materials, error));
        CHECK(materials[0].diffuseTexture.empty());
        CHECK(materials[0].normalTexture == std::filesystem::path("n.png"));
    }

    SUBCASE("before the first newmtl") {
        CHECK_FALSE(assets::parseMtl("map_Bump -bm 1.0 n.png\n", materials, error));
        CHECK(error == "line 1: map_Bump before the first newmtl");
    }

    SUBCASE("without a file name") {
        CHECK_FALSE(assets::parseMtl("newmtl m\nmap_Bump\n", materials, error));
        CHECK(error == "line 2: map_Bump needs a file name");
        CHECK_FALSE(assets::parseMtl("newmtl m\nnorm -bm 1.0\n", materials, error));
        CHECK(error == "line 2: norm needs a file name");
    }

    SUBCASE("-bm without a number") {
        CHECK_FALSE(assets::parseMtl("newmtl m\nmap_Bump -bm n.png\n", materials, error));
        CHECK(error == "line 2: map_Bump -bm needs a number");
        CHECK_FALSE(assets::parseMtl("newmtl m\nbump -bm\n", materials, error));
        CHECK(error == "line 2: bump -bm needs a number");
    }
}

TEST_CASE("parseMtl: a bad line is reported with its line number") {
    std::vector<assets::ObjMaterial> materials;
    std::string error;

    SUBCASE("Kd before the first newmtl") {
        CHECK_FALSE(assets::parseMtl("# header\nKd 1 1 1\n", materials, error));
        CHECK(error == "line 2: Kd before the first newmtl");
    }

    SUBCASE("map_Kd before the first newmtl") {
        CHECK_FALSE(assets::parseMtl("map_Kd stone.png\n", materials, error));
        CHECK(error == "line 1: map_Kd before the first newmtl");
    }

    SUBCASE("Kd with a bad number") {
        CHECK_FALSE(assets::parseMtl("newmtl m\nKd 1 0,5 1\n", materials, error));
        CHECK(error == "line 2: Kd needs three numbers");
    }

    SUBCASE("Kd with too few numbers") {
        CHECK_FALSE(assets::parseMtl("newmtl m\n\nKd 1 1\n", materials, error));
        CHECK(error == "line 3: Kd needs three numbers");
    }

    SUBCASE("newmtl and map_Kd without a name") {
        CHECK_FALSE(assets::parseMtl("newmtl\n", materials, error));
        CHECK(error == "line 1: newmtl needs a material name");
        CHECK_FALSE(assets::parseMtl("newmtl m\nmap_Kd\n", materials, error));
        CHECK(error == "line 2: map_Kd needs a file name");
    }

    SUBCASE("a failed parse appends nothing") {
        REQUIRE(assets::parseMtl("newmtl kept\n", materials, error));
        CHECK_FALSE(assets::parseMtl("newmtl lost\nKd x y z\n", materials, error));
        REQUIRE(materials.size() == 1U);
        CHECK(materials[0].name == "kept");
    }
}

TEST_CASE("loadObj: wall_straight.obj") {
    const assets::ObjModel model =
        loadGameModel("wall_straight.obj", "wall_stone", "wall_stone.png", "wall_stone_normal.png");

    // 30 triangles. The file has 24 positions, 24 texture coordinates and 6 normals, and
    // its 90 corners use 60 different triples. Tangents add no vertices: they are
    // computed after the vertices exist.
    CHECK(model.indices.size() == 90U);
    CHECK(model.vertices.size() == 60U);

    const Bounds bounds = boundsOf(model);
    checkVec3(bounds.min, {-1.0F, 0.0F, -0.14F});
    checkVec3(bounds.max, {1.0F, 3.0F, 0.14F});

    // The tangent points to the right for someone who looks at a face from outside: +X
    // on the front (facing +Z), -X on the back, and on the two ends along the Z axis.
    for (const gfx::Vertex& vertex : model.vertices) {
        if (vertex.normal.z > 0.5F) {
            checkVec3(vertex.tangent, {1.0F, 0.0F, 0.0F});
        } else if (vertex.normal.z < -0.5F) {
            checkVec3(vertex.tangent, {-1.0F, 0.0F, 0.0F});
        } else if (vertex.normal.x > 0.5F) {
            checkVec3(vertex.tangent, {0.0F, 0.0F, -1.0F});
        } else if (vertex.normal.x < -0.5F) {
            checkVec3(vertex.tangent, {0.0F, 0.0F, 1.0F});
        }
    }
}

TEST_CASE("loadObj: wall_pillar.obj") {
    const assets::ObjModel model =
        loadGameModel("wall_pillar.obj", "wall_stone", "wall_stone.png", "wall_stone_normal.png");

    CHECK(model.indices.size() == 90U);
    CHECK(model.vertices.size() == 60U);

    const Bounds bounds = boundsOf(model);
    checkVec3(bounds.min, {-0.2F, 0.0F, -0.2F});
    checkVec3(bounds.max, {0.2F, 3.15F, 0.2F});
}

TEST_CASE("loadObj: floor_tile.obj") {
    const assets::ObjModel model =
        loadGameModel("floor_tile.obj", "floor_stone", "floor_stone.png", "floor_stone_normal.png");

    // 2 triangles that share two corners.
    CHECK(model.indices.size() == 6U);
    CHECK(model.vertices.size() == 4U);

    const Bounds bounds = boundsOf(model);
    checkVec3(bounds.min, {-1.0F, 0.0F, -1.0F});
    checkVec3(bounds.max, {1.0F, 0.0F, 1.0F});

    // A floor faces up, and both triangles are counter clockwise seen from above: the
    // cross product of two edges points the same way as the normal.
    for (const gfx::Vertex& vertex : model.vertices) {
        checkVec3(vertex.normal, {0.0F, 1.0F, 0.0F});
        // On the floor u grows along +X and v along -Z (Blender +Y), so the tangent is
        // +X and the bitangent cross(N, T) is -Z.
        checkVec3(vertex.tangent, {1.0F, 0.0F, 0.0F});
        checkVec3(glm::cross(vertex.normal, vertex.tangent), {0.0F, 0.0F, -1.0F});
    }
    for (std::size_t i = 0; i < model.indices.size(); i += 3) {
        const glm::vec3& a = model.vertices[model.indices[i]].position;
        const glm::vec3& b = model.vertices[model.indices[i + 1]].position;
        const glm::vec3& c = model.vertices[model.indices[i + 2]].position;
        CHECK(glm::cross(b - a, c - a).y > 0.0F);
    }
}

TEST_CASE("loadObj: material libraries and texture paths of files written by the test") {
    // A scratch directory of its own, removed again at the end of the test case.
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "night_maze_obj_loader_tests";
    std::filesystem::remove_all(directory);

    const std::string triangle = "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
    const std::filesystem::path objPath = directory / "models" / "thing.obj";
    assets::ObjModel model;
    std::string error;

    SUBCASE("mtllib is found next to the OBJ file, map_Kd next to the MTL file") {
        writeFile(objPath, "mtllib materials/first.mtl\n"
                           "mtllib second.mtl\n" +
                               triangle +
                               "usemtl painted\n"
                               "f 1 2 3\n"
                               "usemtl plain\n"
                               "f 1 3 2\n");
        writeFile(directory / "models" / "materials" / "first.mtl",
                  "newmtl painted\nKd 0.5 0.25 1\nmap_Kd ../../textures/paint.png\n"
                  "map_Bump -bm 1.000000 ../../textures/paint_normal.png\n");
        writeFile(directory / "models" / "second.mtl", "newmtl plain\nKd 0 1 0\n");

        REQUIRE(assets::loadObj(objPath, model, error));
        CHECK(error.empty());
        REQUIRE(model.parts.size() == 2U);
        REQUIRE(model.materials.size() == 2U);

        CHECK(model.materials[0].name == "painted");
        checkVec3(model.materials[0].diffuseColor, {0.5F, 0.25F, 1.0F});
        // models/materials/../../textures/paint.png, with the ".." steps removed. The
        // file does not exist: loadObj does not check that.
        CHECK(model.materials[0].diffuseTexture ==
              (directory / "textures" / "paint.png").lexically_normal());
        // The normal map path goes the same way.
        CHECK(model.materials[0].normalTexture ==
              (directory / "textures" / "paint_normal.png").lexically_normal());

        CHECK(model.materials[1].name == "plain");
        checkVec3(model.materials[1].diffuseColor, {0.0F, 1.0F, 0.0F});
        CHECK(model.materials[1].diffuseTexture.empty());
        CHECK(model.materials[1].normalTexture.empty());
    }

    SUBCASE("a model without mtllib and without usemtl loads with no materials") {
        writeFile(objPath, triangle + "f 1 2 3\n");

        REQUIRE(assets::loadObj(objPath, model, error));
        CHECK(model.materials.empty());
        REQUIRE(model.parts.size() == 1U);
        CHECK(model.parts[0].material.empty());
    }

    SUBCASE("an MTL file that does not exist is an error") {
        writeFile(objPath, "mtllib missing.mtl\n" + triangle + "f 1 2 3\n");

        CHECK_FALSE(assets::loadObj(objPath, model, error));
        CHECK(startsWith(error, "MTL file cannot be opened: "));
        CHECK(error.find("missing.mtl") != std::string::npos);
        CHECK(model.vertices.empty());
    }

    SUBCASE("a material that no MTL file defines is an error") {
        writeFile(objPath, "mtllib thing.mtl\n" + triangle + "usemtl gold\nf 1 2 3\n");
        writeFile(directory / "models" / "thing.mtl", "newmtl silver\n");

        CHECK_FALSE(assets::loadObj(objPath, model, error));
        CHECK(error.find("material 'gold' is used but not defined") != std::string::npos);
        CHECK(model.vertices.empty());
    }

    SUBCASE("a bad line is reported with the file and the line number") {
        writeFile(objPath, triangle + "f 1 2 9\n");

        CHECK_FALSE(assets::loadObj(objPath, model, error));
        CHECK(error.find("thing.obj: line 4: face corner '9'") != std::string::npos);
    }

    SUBCASE("a bad line of the MTL file is reported with that file") {
        writeFile(objPath, "mtllib thing.mtl\n" + triangle + "f 1 2 3\n");
        writeFile(directory / "models" / "thing.mtl", "newmtl m\nKd 1 1\n");

        CHECK_FALSE(assets::loadObj(objPath, model, error));
        CHECK(error.find("thing.mtl: line 2: Kd needs three numbers") != std::string::npos);
    }

    std::filesystem::remove_all(directory);
}

TEST_CASE("loadObj: a file that does not exist is reported, not thrown") {
    assets::ObjModel model;
    std::string error;

    CHECK_FALSE(assets::loadObj(modelsDirectory() / "no_such_model.obj", model, error));

    CHECK(startsWith(error, "OBJ file cannot be opened: "));
    CHECK(error.find("no_such_model.obj") != std::string::npos);
    CHECK(model.vertices.empty());
}
