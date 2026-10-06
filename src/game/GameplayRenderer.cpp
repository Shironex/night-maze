// GameplayRenderer: draws the things of a round, the crystals and the gate, with their models.
// See docs/modules/game/gameplay.md
#include "game/GameplayRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "game/MazeWorld.hpp"
#include "game/ModelDraw.hpp"
#include "game/Round.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "scene/Transform.hpp"

#include <span>

namespace game {

namespace {

// Model files, relative to the assets directory. The crystal models are about 0.5 m
// high with their origin at the base (for crystal_b the base of its main shard, so
// that model is not centred on its origin). The gate model is built like the wall
// model: along X, from x = -1 to x = +1, with its origin in the middle of its base.
constexpr const char* CRYSTAL_A_MODEL_FILE = "models/crystal_a.obj";
constexpr const char* CRYSTAL_B_MODEL_FILE = "models/crystal_b.obj";
constexpr const char* GATE_MODEL_FILE = "models/gate.obj";

} // namespace

GameplayRenderer::GameplayRenderer(assets::AssetCache& assets)
    : m_crystals{assets.model(core::assetPath(CRYSTAL_A_MODEL_FILE)),
                 assets.model(core::assetPath(CRYSTAL_B_MODEL_FILE))},
      m_gate(assets.model(core::assetPath(GATE_MODEL_FILE))) {}

void GameplayRenderer::draw(const gfx::Shader& shader, const MazeWorld& world, const Round& round,
                            const glm::vec3& crystalGlow) const {
    drawGate(shader, world, round);
    drawCrystals(shader, round, crystalGlow);
}

void GameplayRenderer::drawGate(const gfx::Shader& shader, const MazeWorld& world,
                                const Round& round) const {
    setModelSamplers(shader);

    // The gate, as long as some of it is above the ground. It is a wall segment that
    // moves: the same matrix as a wall there, with the position lowered by how far the
    // gate has sunk. The terrain hides the part that is below it.
    if (gateVisible(world, round)) {
        WallSegment loweredGate = world.gate;
        loweredGate.position.y -= gateSinkDepth(round);
        const glm::mat4 gateMatrix = wallModelMatrix(loweredGate);

        // Wood gives off no light.
        shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});
        // drawModel takes a list of matrices. A span made of a pointer and a count of
        // 1 is a list with this one matrix in it.
        drawModel(shader, m_gate, std::span<const glm::mat4>(&gateMatrix, 1));
    }
}

void GameplayRenderer::drawCrystals(const gfx::Shader& shader, const Round& round,
                                    const glm::vec3& crystalGlow) const {
    // Set here too: the crystals may be drawn with another program than the gate.
    setModelSamplers(shader);

    // The crystals glow. One uniform for all of them: they pulse together.
    shader.setVec3(EMISSIVE_UNIFORM, crystalGlow);
    for (std::size_t i = 0; i < round.crystals.size(); ++i) {
        const RoundCrystal& crystal = round.crystals[i];
        // A collected crystal is gone. A variant without a model cannot happen with the
        // crystals of placeCrystals, but an index is checked before it is used.
        if (crystal.collected || crystal.variant < 0 || crystal.variant >= CRYSTAL_VARIANT_COUNT) {
            continue;
        }

        // Where the crystal is at this moment and how far it has turned. The origin of
        // the model is at its base, so the position is the base.
        const int index = static_cast<int>(i);
        scene::Transform transform;
        transform.position =
            crystalBobPosition(crystal.restPosition, index, round.animationSeconds);
        transform.rotationDegrees = {0.0F, crystalSpinDegrees(index, round.animationSeconds), 0.0F};
        const glm::mat4 crystalMatrix = transform.matrix();

        drawModel(shader, m_crystals[static_cast<std::size_t>(crystal.variant)],
                  std::span<const glm::mat4>(&crystalMatrix, 1));
    }
}

} // namespace game
