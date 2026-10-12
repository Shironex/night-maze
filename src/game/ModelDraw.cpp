// ModelDraw: draws a mesh with its textures, shared by the classes that draw models.
#include "game/ModelDraw.hpp"

#include "assets/AssetCache.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Mesh.hpp"
#include "gfx/RenderStats.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture2D.hpp"
#include "scene/Collider.hpp"
#include "scene/Frustum.hpp"
#include "scene/Transform.hpp"

#include <cstddef>

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

bool drawnInPass(const scene::Frustum* frustum, const scene::Aabb& worldBox) {
    if (frustum == nullptr) {
        return true;
    }
    const bool seen = scene::canBeSeen(*frustum, worldBox);
    gfx::countCullTest(seen);
    return seen;
}

void drawModel(const gfx::Shader& shader, const assets::LoadedModel* model,
               std::span<const glm::mat4> modelMatrices, const scene::Frustum* frustum) {
    // No list of boxes: the box of every object is computed from its matrix.
    drawModel(shader, model, modelMatrices, std::span<const scene::Aabb>{}, frustum);
}

void drawModel(const gfx::Shader& shader, const assets::LoadedModel* model,
               std::span<const glm::mat4> modelMatrices, std::span<const scene::Aabb> worldBoxes,
               const scene::Frustum* frustum) {
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

        // An object is counted once, with its first part, however many parts it has.
        const bool counted = &part == &model->parts.front();

        for (std::size_t i = 0; i < modelMatrices.size(); ++i) {
            const glm::mat4& modelMatrix = modelMatrices[i];
            // View frustum culling: an object that lies outside of what the pass sees is
            // skipped before anything is sent to the graphics card for it. Its box is
            // the stored one, or the bounds of the model moved by its matrix.
            if (frustum != nullptr) {
                const scene::Aabb box = i < worldBoxes.size()
                                            ? worldBoxes[i]
                                            : scene::transformedBounds(model->bounds, modelMatrix);
                const bool seen = scene::canBeSeen(*frustum, box);
                if (counted) {
                    gfx::countCullTest(seen);
                }
                if (!seen) {
                    continue;
                }
            }
            shader.setMat4(MODEL_UNIFORM, modelMatrix);
            // The lit programs turn the normals with a matrix of their own, derived
            // from the model matrix. It is computed here, on the CPU, once per object:
            // in the shader the inverse would be computed again for every vertex.
            shader.setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix));
            model->mesh.draw(part.firstIndex, part.indexCount);
        }
    }
}

void drawMesh(const gfx::Shader& shader, const gfx::Mesh& mesh, const gfx::Texture2D& texture,
              const gfx::Texture2D& normalMap, const glm::vec3& tint,
              const glm::mat4& modelMatrix) {
    // The same steps as for one part of a model and one object: the two textures on
    // their units (the colour picture last, see drawModel), the tint, the two matrices.
    normalMap.bind(NORMAL_MAP_UNIT);
    texture.bind(TEXTURE_UNIT);
    shader.setVec3(TINT_UNIFORM, tint);
    shader.setMat4(MODEL_UNIFORM, modelMatrix);
    shader.setMat3(NORMAL_MATRIX_UNIFORM, scene::normalMatrix(modelMatrix));
    mesh.draw();
}

} // namespace game
