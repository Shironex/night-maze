// ModelDraw: draws a loaded model with its textures, shared by the classes that draw models.
// See docs/modules/game/maze-rendering.md
#include "game/ModelDraw.hpp"

#include "assets/AssetCache.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "scene/Transform.hpp"

namespace game {

namespace {

// The texture units of the models: the colour pictures are bound to the first one, the
// normal maps to the second. Each sampler uniform gets the number of its unit. A shader
// can read both textures for the same fragment only because they are on different units.
constexpr GLuint TEXTURE_UNIT = 0;
constexpr GLuint NORMAL_MAP_UNIT = 1;

} // namespace

void setModelSamplers(const gfx::Shader& shader) {
    shader.setInt(TEXTURE_UNIFORM, static_cast<int>(TEXTURE_UNIT));
    shader.setInt(NORMAL_MAP_UNIFORM, static_cast<int>(NORMAL_MAP_UNIT));
}

void drawModel(const gfx::Shader& shader, const assets::LoadedModel* model,
               std::span<const glm::mat4> modelMatrices) {
    // The load error is in the log. The rest of the scene is still drawn.
    if (model == nullptr) {
        return;
    }

    // The parts are the outer loop: the textures and the tint are set once per part, and
    // only the model matrix changes from one object to the next.
    for (const assets::ModelPart& part : model->parts) {
        // The normal map first: bind() makes its unit the active one, and binding the
        // colour picture last leaves unit 0 active, as the rest of the program expects.
        // Never null: a part without a normal map has the flat one of the cache.
        part.normalMap->bind(NORMAL_MAP_UNIT);
        part.texture->bind(TEXTURE_UNIT);
        shader.setVec3(TINT_UNIFORM, part.color);

        for (const glm::mat4& modelMatrix : modelMatrices) {
            shader.setMat4(MODEL_UNIFORM, modelMatrix);
            // The lit programs turn the normals with a matrix of their own, derived
            // from the model matrix. It is computed here, on the CPU, once per object:
            // in the shader the inverse would be computed again for every vertex.
            shader.setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix));
            model->mesh.draw(part.firstIndex, part.indexCount);
        }
    }
}

} // namespace game
