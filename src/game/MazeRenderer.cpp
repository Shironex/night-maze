// MazeRenderer: draws the floor, the walls and the pillars of a maze with their models.
// See docs/modules/game/maze-rendering.md
#include "game/MazeRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "game/MazeWorld.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"

namespace game {

namespace {

// Model files, relative to the assets directory.
constexpr const char* FLOOR_TILE_MODEL_FILE = "models/floor_tile.obj";
constexpr const char* WALL_MODEL_FILE = "models/wall_straight.obj";
constexpr const char* PILLAR_MODEL_FILE = "models/wall_pillar.obj";

// The texture unit all textures of the maze are bound to. The sampler uniform gets the
// same number.
constexpr GLuint TEXTURE_UNIT = 0;

} // namespace

MazeRenderer::MazeRenderer(assets::AssetCache& assets)
    : m_floorTile(assets.model(core::assetPath(FLOOR_TILE_MODEL_FILE))),
      m_wall(assets.model(core::assetPath(WALL_MODEL_FILE))),
      m_pillar(assets.model(core::assetPath(PILLAR_MODEL_FILE))) {}

void MazeRenderer::draw(const gfx::Shader& shader, const MazeWorld& world) const {
    // The sampler reads the unit the textures are bound to below. It is set in every
    // frame and not once at start-up: after a shader reload all uniforms are back at 0.
    shader.setInt(TEXTURE_UNIFORM, static_cast<int>(TEXTURE_UNIT));

    drawInstances(shader, m_floorTile, world.floorMatrices);
    drawInstances(shader, m_wall, world.wallMatrices);
    drawInstances(shader, m_pillar, world.pillarMatrices);
}

void MazeRenderer::drawInstances(const gfx::Shader& shader, const assets::LoadedModel* model,
                                 std::span<const glm::mat4> modelMatrices) {
    // The load error is in the log. The rest of the maze is still drawn.
    if (model == nullptr) {
        return;
    }

    // The parts are the outer loop: the texture and the tint are set once per part, and
    // only the model matrix changes from one object to the next.
    for (const assets::ModelPart& part : model->parts) {
        part.texture->bind(TEXTURE_UNIT);
        shader.setVec3(TINT_UNIFORM, part.color);

        for (const glm::mat4& modelMatrix : modelMatrices) {
            shader.setMat4(MODEL_UNIFORM, modelMatrix);
            model->mesh.draw(part.firstIndex, part.indexCount);
        }
    }
}

} // namespace game
