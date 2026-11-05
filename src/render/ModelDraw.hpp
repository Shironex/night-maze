// ModelDraw: draws a mesh with its textures, shared by the classes that draw models.
#pragma once

#include <glm/glm.hpp>

#include <span>

namespace assets {
struct LoadedModel;
} // namespace assets

namespace gfx {
class Mesh;
class Shader;
class Texture2D;
} // namespace gfx

namespace scene {
struct Aabb;
struct Frustum;
} // namespace scene

namespace game {

// The maze (MazeRenderer) and the things of a round (GameplayRenderer) are drawn the
// same way: a model from the asset cache, once per model matrix. The terrain
// (TerrainRenderer) is one mesh of its own with two textures. The functions here are the
// common part, so all three classes agree on the texture units and the uniforms.

/// Tells the two samplers of a program (uTexture and uNormalMap) which texture units to
/// read. shader must be in use. Call it in every frame before drawModel and not once at
/// start-up: after a shader reload all uniforms are back at 0, and both samplers would
/// read unit 0. The gouraud program has no uNormalMap: a uniform a program does not
/// have is ignored.
void setModelSamplers(const gfx::Shader& shader);

/// Draws one model once for every matrix in modelMatrices (one draw call per object and
/// part). shader is the textured program or one of the two lit programs (lit, gouraud):
/// it must be in use, with its samplers set (setModelSamplers).
///
/// The function binds the two textures and sets uTint for every part of the model, and
/// sets uModel and uNormalMatrix for every object. uTint is the Kd colour of the
/// material, used as a plain factor on the (linear) texture colour without any
/// conversion: every model of the game has a white Kd, which is 1 in any colour space. The textured
/// program has no uNormalMatrix. A model that is nullptr (it failed to load, the error is in the
/// log) is not drawn.
///
/// frustum is the frustum of the pass that is being drawn (view frustum culling): the
/// one of the very matrices the program in use has as uView and uProjection. An object
/// whose box lies outside of it is not drawn (drawnInPass). The box of an object is
/// the bounds of the model moved into the world by the matrix of the object
/// (scene::transformedBounds), computed here for every call: that fits things that
/// move, turn or are few. nullptr: every object is drawn and nothing is tested.
void drawModel(const gfx::Shader& shader, const assets::LoadedModel* model,
               std::span<const glm::mat4> modelMatrices, const scene::Frustum* frustum = nullptr);

/// The same for objects that never move: worldBoxes holds the box of every object in
/// world space, computed once when the world was built, in the order of modelMatrices.
/// An object the list has no box for is tested with the box the function above
/// computes.
void drawModel(const gfx::Shader& shader, const assets::LoadedModel* model,
               std::span<const glm::mat4> modelMatrices, std::span<const scene::Aabb> worldBoxes,
               const scene::Frustum* frustum);

/// Whether a pass draws the object with this box (in world space). frustum is the
/// frustum of the pass, or nullptr for a pass that draws everything: the answer is then
/// true and nothing is counted. Otherwise the box is tested (scene::canBeSeen) and the
/// object is counted for the Diagnostics category (gfx::countCullTest).
///
/// Only the draw call depends on the answer. Whatever a caller sets for the objects
/// that follow (a texture, a uniform) it sets all the same, so the picture is the one
/// that is drawn without the test.
bool drawnInPass(const scene::Frustum* frustum, const scene::Aabb& worldBox);

/// Draws a whole mesh once, with the given colour texture and normal map. For a mesh
/// that does not come from a model file, like the terrain, and for a model that is
/// drawn with other textures than its file names, like a worn wall. shader is prepared
/// as for drawModel. The function binds the two textures and sets uTint, uModel and
/// uNormalMatrix.
void drawMesh(const gfx::Shader& shader, const gfx::Mesh& mesh, const gfx::Texture2D& texture,
              const gfx::Texture2D& normalMap, const glm::vec3& tint, const glm::mat4& modelMatrix);

} // namespace game
