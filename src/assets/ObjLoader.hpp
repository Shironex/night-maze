// OBJ loader: reads a Wavefront OBJ model and its MTL materials into plain CPU data.
#pragma once

#include "gfx/Vertex.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace assets {

/// A run of triangles that share one material: indexCount indices of the model, starting
/// with index number firstIndex. gfx::Mesh::draw(firstIndex, indexCount) draws it.
struct ObjPart {
    /// Name given by the usemtl line. Empty for faces that come before any usemtl line.
    std::string material;
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;
};

/// One material of an MTL file: only what the game uses.
struct ObjMaterial {
    /// Name given by the newmtl line. usemtl lines of the OBJ file refer to it.
    std::string name;

    /// Diffuse colour (red, green, blue) from the Kd line. White when the file has no Kd
    /// line: a white colour multiplied by a texture leaves the texture unchanged.
    glm::vec3 diffuseColor{1.0F};

    /// Diffuse texture from the map_Kd line. Empty when the material has none.
    /// parseMtl stores the path as it is written in the file. loadObj replaces it with
    /// the path of the image file: relative to the directory of the MTL file, normalized.
    std::filesystem::path diffuseTexture;

    /// Normal map from the map_Bump line (or one of its other spellings, see parseMtl).
    /// Empty when the material has none. The path is handled like diffuseTexture: as
    /// written after parseMtl, the path of the image file after loadObj.
    std::filesystem::path normalTexture;
};

/// A whole model as plain data: no OpenGL object, so it can be built without a window.
struct ObjModel {
    /// One vertex for every different position/uv/normal combination used by a face.
    /// The tangents are not in the file: they are computed from the positions and the
    /// texture coordinates of the triangles (assets::computeTangents).
    std::vector<gfx::Vertex> vertices;

    /// Three indices into vertices for every triangle, counter clockwise as in the file.
    std::vector<std::uint32_t> indices;

    /// The indices split by material, in file order. Together the parts cover all indices.
    std::vector<ObjPart> parts;

    /// File names from the mtllib lines, as written in the file.
    std::vector<std::string> materialLibraries;

    /// Materials of all material libraries. Filled by loadObj, left empty by parseObj.
    std::vector<ObjMaterial> materials;

    /// Number of lines that started with a keyword the parser does not know. Lines it
    /// knows and ignores on purpose (o, g, s, comments, blank lines) are not counted.
    std::size_t unknownLineCount = 0;

    /// Number of triangles whose texture is mirrored (assets::countMirroredTriangles).
    /// A normal map shows its relief upside down on them, because the vertices store no
    /// handedness sign. 0 for the models of the game.
    std::size_t mirroredTriangleCount = 0;
};

/// Parses the text of an OBJ file. Supported lines: v, vt, vn, f, mtllib and usemtl.
/// Faces may have any number of corners from 3 up (they are split into triangles) and the
/// corner forms v, v/vt, v//vn and v/vt/vn, also with negative (relative) indices.
/// After the last line the tangents of the vertices are computed.
///
/// Returns true and fills model on success. Returns false on the first bad line and puts
/// the reason into error, starting with "line N: ". model is then left unchanged.
/// The function reads no file, logs nothing and does not throw.
bool parseObj(std::string_view text, ObjModel& model, std::string& error);

/// Parses the text of an MTL file and appends its materials to materials. Supported
/// lines: newmtl, Kd, map_Kd and the normal map. Every other line is skipped.
///
/// The normal map line is read in the form Blender writes it,
///
///     map_Bump -bm 1.000000 ../textures/wall_stone_normal.png
///
/// and in the other common spellings of the keyword: map_bump, bump and norm. The option
/// "-bm number" (bump multiplier, the strength of the map) may be left out. Its number is
/// checked and then ignored: the game always uses a normal map at full strength.
///
/// Returns true on success. Returns false on the first bad line and puts the reason into
/// error, starting with "line N: ". materials is then left unchanged.
/// The function reads no file, logs nothing and does not throw.
bool parseMtl(std::string_view text, std::vector<ObjMaterial>& materials, std::string& error);

/// Reads an OBJ file and the MTL files it names. An mtllib name is looked up in the
/// directory of the OBJ file, the path of a texture (map_Kd, normal map) in the directory
/// of its MTL file. Whether a texture file exists is not checked: the code that opens the
/// image reports that.
///
/// Returns true and fills model on success. On failure (a file cannot be opened, a bad
/// line, a usemtl name that no MTL file defines) it logs the error once, puts the same
/// text into error, leaves model unchanged and returns false. It does not throw.
/// A model with mirrored triangles loads, with a warning in the log.
bool loadObj(const std::filesystem::path& path, ObjModel& model, std::string& error);

} // namespace assets
