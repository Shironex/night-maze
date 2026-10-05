// MazeRenderer: draws the floor, the walls and the pillars of a maze with their models.
// See docs/modules/game/maze-rendering.md
#include "game/MazeRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "game/MazeWorld.hpp"
#include "game/ModelDraw.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"

namespace game {

namespace {

// Model files, relative to the assets directory.
constexpr const char* FLOOR_TILE_MODEL_FILE = "models/floor_tile.obj";
constexpr const char* WALL_MODEL_FILE = "models/wall_straight.obj";
constexpr const char* PILLAR_MODEL_FILE = "models/wall_pillar.obj";

} // namespace

MazeRenderer::MazeRenderer(assets::AssetCache& assets)
    : m_floorTile(assets.model(core::assetPath(FLOOR_TILE_MODEL_FILE))),
      m_wall(assets.model(core::assetPath(WALL_MODEL_FILE))),
      m_pillar(assets.model(core::assetPath(PILLAR_MODEL_FILE))) {}

void MazeRenderer::draw(const gfx::Shader& shader, const MazeWorld& world) const {
    setModelSamplers(shader);
    // Stone gives off no light of its own. A uniform keeps its value from one draw call
    // to the next, and the crystals set this one, so it is set back in every frame.
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});

    drawModel(shader, m_floorTile, world.floorMatrices);
    drawModel(shader, m_wall, world.wallMatrices);
    drawModel(shader, m_pillar, world.pillarMatrices);
}

} // namespace game
