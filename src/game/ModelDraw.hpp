// ModelDraw: draws a loaded model with its textures, shared by the classes that draw models.
// See docs/modules/game/maze-rendering.md
#pragma once

#include <glm/glm.hpp>

#include <span>

namespace assets {
struct LoadedModel;
} // namespace assets

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

// The maze (MazeRenderer) and the things of a round (GameplayRenderer) are drawn the
// same way: a model from the asset cache, once per model matrix. The two functions here
// are that common part, so both classes agree on the texture units and the uniforms.

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
/// sets uModel and uNormalMatrix for every object. The textured program has no
/// uNormalMatrix. A model that is nullptr (it failed to load, the error is in the log)
/// is not drawn.
void drawModel(const gfx::Shader& shader, const assets::LoadedModel* model,
               std::span<const glm::mat4> modelMatrices);

} // namespace game
