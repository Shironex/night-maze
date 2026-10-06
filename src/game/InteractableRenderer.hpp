// InteractableRenderer: draws the levers and the notes of a maze with their models.
// See docs/modules/game/interactables.md
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

/// Draws what the player can point at: every lever (a plate on the wall and a handle
/// that is up until the lever is pulled and swings down then) and every note (a sheet
/// of paper on the wall). The one the picking ray points at is drawn highlighted.
///
/// It owns nothing: the three models belong to the asset cache, which must outlive this
/// object, and all positions come from the MazeWorld and the Round given to draw.
class InteractableRenderer {
public:
    /// Asks the cache for the model of the lever plate, of the lever handle and of the
    /// note. A model that fails to load is logged by the cache and simply not drawn.
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
    /// object. Everything else is drawn with uEmissive black. The shadow pass gives an
    /// empty PickState: a depth program has no colours to highlight.
    void draw(const gfx::Shader& shader, const MazeWorld& world, const Round& round,
              const PickState& pick, const glm::vec3& highlight) const;

private:
    // Not owned. nullptr when the model could not be loaded.
    const assets::LoadedModel* m_leverPlate;
    const assets::LoadedModel* m_leverHandle;
    const assets::LoadedModel* m_note;
};

} // namespace game
