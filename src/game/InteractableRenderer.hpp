// InteractableRenderer: draws the levers and the notes of a maze with their models, the
// chalk crook beside every lever and the iron ring on the wall every lever opens.
#pragma once

#include <glm/glm.hpp>

namespace assets {
class AssetCache;
struct LoadedModel;
} // namespace assets

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

struct MazeWorld;
struct PickState;
struct Round;

/// Draws what the player can point at: every lever (an oak board on the wall, a shepherd's
/// crook for a handle that is up until the lever is pulled and swings down then, and
/// a rope into the turf that hangs slack and is pulled straight then) and every note
/// (a chalk mark on the wall: a lamp for a line of the story, an arrow for a hint, turned
/// towards what the hint names). The one the picking ray points at is drawn highlighted.
/// Two things are only signs and cannot be picked: the chalk crook beside every lever,
/// and the iron ring at the foot of the wall a lever opens, on both of its faces, which
/// sinks with that wall.
///
/// It owns nothing: the models belong to the asset cache, which must outlive this
/// object, and all positions come from the MazeWorld and the Round given to draw.
class InteractableRenderer {
public:
    /// Asks the cache for the models of the lever (board, crook and the two ropes), of
    /// the slab ring and of the three chalk marks. A model that fails to load is logged
    /// by the cache and simply not drawn.
    explicit InteractableRenderer(assets::AssetCache& assets);

    /// Draws the levers and the notes. shader is the textured program, one of the two
    /// lit programs (lit, gouraud) or the depth program of the shadow pass, prepared
    /// exactly as for MazeRenderer::draw: in use, with uView, uProjection and its own
    /// uniforms set. Drawing with the program of the walls gives them the same lighting
    /// mode, the same debug views and the same shadows.
    ///
    /// pick is the picking of this frame. The lever or the note it names, when the
    /// player can use it (pick.action is PullLever or ReadNote), is drawn with uEmissive
    /// set to highlight (game::highlightGlow): that is the highlight of the selected
    /// object. The other chalk marks are drawn with game::CHALK_GLOW, the levers with
    /// uEmissive black. The shadow pass gives an
    /// empty PickState: a depth program has no colours to highlight.
    void draw(const gfx::Shader& shader, const MazeWorld& world, const Round& round,
              const PickState& pick, const glm::vec3& highlight) const;

private:
    // Not owned. nullptr when the model could not be loaded.
    const assets::LoadedModel* m_leverPost;
    const assets::LoadedModel* m_leverHandle;
    const assets::LoadedModel* m_leverRopeSlack;
    const assets::LoadedModel* m_leverRopeTaut;
    const assets::LoadedModel* m_slabRing;
    const assets::LoadedModel* m_chalkLamp;
    const assets::LoadedModel* m_chalkArrow;
    const assets::LoadedModel* m_chalkCrook;
};

} // namespace game
