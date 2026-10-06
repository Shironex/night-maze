// GameplayRenderer: draws the things of a round, the crystals and the gate, with their models.
// See docs/modules/game/gameplay.md
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
/// bobbing and turning, and the gate of the exit, sinking into the ground once it opens.
/// The maze itself is drawn by MazeRenderer.
///
/// It owns nothing: the models belong to the asset cache, which must outlive this
/// object, and all positions come from the MazeWorld and the Round given to draw.
class GameplayRenderer {
public:
    /// Asks the cache for the two crystal models and the gate model. A model that fails
    /// to load is logged by the cache and simply not drawn.
    explicit GameplayRenderer(assets::AssetCache& assets);

    /// Draws the gate and the crystals. shader is the textured program or one of the two
    /// lit programs (lit, gouraud), prepared exactly as for MazeRenderer::draw: in use,
    /// with uView, uProjection and its own uniforms set. Drawing with the program the
    /// maze was drawn with gives the crystals and the gate the same lighting mode and
    /// the same debug views as the walls.
    ///
    /// crystalGlow is the light the crystals give off by themselves (game::crystalGlow).
    /// The function sets uEmissive to it for the crystals and to black for the gate.
    void draw(const gfx::Shader& shader, const MazeWorld& world, const Round& round,
              const glm::vec3& crystalGlow) const;

    /// The two halves of draw, for a frame that draws the gate with one program and
    /// the crystals with another (the reflect program, which shows the sky on them).
    /// shader is prepared as for draw. drawGate sets uEmissive to black, drawCrystals
    /// to crystalGlow.
    void drawGate(const gfx::Shader& shader, const MazeWorld& world, const Round& round) const;
    void drawCrystals(const gfx::Shader& shader, const Round& round,
                      const glm::vec3& crystalGlow) const;

private:
    // Not owned. nullptr when the model could not be loaded. One crystal model per
    // variant: the number of a variant is its index here.
    std::array<const assets::LoadedModel*, static_cast<std::size_t>(CRYSTAL_VARIANT_COUNT)>
        m_crystals;
    const assets::LoadedModel* m_gate;
};

} // namespace game
