// GameplayRenderer: draws the things of a round, the crystals, the flasks and the gate with
// its gatehouse, with their models.
#pragma once

#include "game/Crystals.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>

namespace assets {
class AssetCache;
struct LoadedModel;
} // namespace assets

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

struct MazeWorld;
struct Round;

/// Draws what changes during a round: every crystal that is not collected yet, floating,
/// bobbing and turning, every flask that is not picked up yet, low over the ground, and
/// the gate of the exit, sinking into the ground once it opens, under its gatehouse with
/// the three lanterns, which never moves, and the bell in it, which swings when it tolls.
/// The maze itself is drawn by MazeRenderer.
///
/// It owns nothing: the models belong to the asset cache, which must outlive this
/// object, and all positions come from the MazeWorld and the Round given to draw.
class GameplayRenderer {
public:
    /// Asks the cache for the two crystal models, the flask model, the shade model, the
    /// gate model and the four models that stand around the gate (the gatehouse, its
    /// lantern, its bell and the milestone).
    /// A model that fails to load is logged by the cache and simply not drawn.
    explicit GameplayRenderer(assets::AssetCache& assets);

    /// Draws the gate and the crystals. shader is the textured program or one of the two
    /// lit programs (lit, gouraud), prepared exactly as for MazeRenderer::draw: in use,
    /// with uView, uProjection and its own uniforms set. Drawing with the program the
    /// maze was drawn with gives the crystals and the gate the same lighting mode and
    /// the same debug views as the walls.
    ///
    /// crystalGlow is the light the crystals give off by themselves (game::crystalGlow)
    /// and lampGlow the one of the lanterns of the gate (the colour of the lamp times its
    /// strength, as a linear colour). The function sets uEmissive to them for the
    /// crystals and the lanterns, and to black for the gate, the stone and the bell.
    /// bellSwingDegrees is how far the bell of the gatehouse is out of the middle at this
    /// moment (game::BellSwing::degrees).
    void draw(const gfx::Shader& shader, const MazeWorld& world, const Round& round,
              const glm::vec3& crystalGlow, const glm::vec3& lampGlow,
              float bellSwingDegrees) const;

    /// The two halves of draw, for a frame that draws the gate with one program and
    /// the crystals with another (the reflect program, which shows the sky on them).
    /// shader is prepared as for draw.
    ///
    /// drawGate draws the door while some of it is above the ground, and always the
    /// gatehouse, the milestone and the three lanterns (game::gateScenery): those stand
    /// still while the door sinks. The bell hangs in the gatehouse, turned by
    /// bellSwingDegrees (game::gateBellMatrix). It sets uEmissive to lampGlow for the
    /// lanterns and leaves it black. A maze without a gate gets none of this.
    /// drawCrystals sets uEmissive to crystalGlow.
    void drawGate(const gfx::Shader& shader, const MazeWorld& world, const Round& round,
                  const glm::vec3& lampGlow, float bellSwingDegrees) const;
    void drawCrystals(const gfx::Shader& shader, const Round& round,
                      const glm::vec3& crystalGlow) const;

    /// Draws every flask of the round that is not picked up yet: floating low over the
    /// ground of its cell, bobbing and turning slowly like a crystal, so it reads as
    /// something to pick up. shader is prepared as for draw. The function sets uEmissive
    /// to the dim warm glow of a flask (game::FLASK_GLOW) and back to black when it is
    /// done. It is a call of its own and not a part of draw: the shadow passes and the
    /// scene call it once each, with whatever program draws the walls. The flasks
    /// never take the reflect program of the crystals.
    void drawFlasks(const gfx::Shader& shader, const MazeWorld& world, const Round& round) const;

    /// Draws the shade once, with the given model matrix: where it stands, turned
    /// towards the player (the application builds the matrix for the frame). shader is
    /// prepared as for draw. The shade gives off no light: uEmissive is set to black.
    /// Like drawFlasks it is called once by each shadow pass and once by the scene.
    /// Whether a frame has a shade at all is decided by the caller.
    void drawShade(const gfx::Shader& shader, const glm::mat4& modelMatrix) const;

private:
    // Not owned. nullptr when the model could not be loaded. One crystal model per
    // variant: the number of a variant is its index here.
    std::array<const assets::LoadedModel*, static_cast<std::size_t>(CRYSTAL_VARIANT_COUNT)>
        m_crystals;
    const assets::LoadedModel* m_flask;
    const assets::LoadedModel* m_shade;
    const assets::LoadedModel* m_gate;
    const assets::LoadedModel* m_gateArch;
    const assets::LoadedModel* m_gateLantern;
    const assets::LoadedModel* m_gateBell;
    const assets::LoadedModel* m_milestone;
};

} // namespace game
