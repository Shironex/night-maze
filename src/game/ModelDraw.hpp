// ModelDraw: draws a mesh with its textures, shared by the classes that draw models.
// See docs/modules/game/maze-rendering.md
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
void drawModel(const gfx::Shader& shader, const assets::LoadedModel* model,
               std::span<const glm::mat4> modelMatrices);

/// Draws a whole mesh once, with the given colour texture and normal map. For a mesh
/// that does not come from a model file, like the terrain, and for a model that is
/// drawn with other textures than its file names, like a worn wall. shader is prepared
/// as for drawModel. The function binds the two textures and sets uTint, uModel and
/// uNormalMatrix.
void drawMesh(const gfx::Shader& shader, const gfx::Mesh& mesh, const gfx::Texture2D& texture,
              const gfx::Texture2D& normalMap, const glm::vec3& tint, const glm::mat4& modelMatrix);

} // namespace game
