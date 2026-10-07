// MazeRenderer: draws the walls and the pillars of a maze with their models.
// See docs/modules/game/maze-rendering.md
#pragma once

#include "game/WallVariants.hpp"

#include <glm/glm.hpp>

#include <array>
#include <span>

namespace assets {
class AssetCache;
struct LoadedModel;
} // namespace assets

namespace gfx {
class Shader;
class Texture2D;
} // namespace gfx

namespace game {

struct MazeWorld;

/// What the scene shows: its colours or one of two debug views. The numbers are the
/// values of the uniform uViewMode in textured.frag, skybox.frag and grass.frag, so the
/// values here and the comparisons in the three files must stay in step.
enum class ViewMode {
    Textured = 0, ///< the texture multiplied by the colour of the material
    Normals = 1,  ///< the normal used for shading as a colour (a debug view, not lighting)
    Uvs = 2,      ///< the texture coordinate as a colour (a debug view)
};

/// Draws a MazeWorld: one wall model per wall segment and one pillar model per pillar,
/// each with its own model matrix (one draw call per object). The ground they stand on
/// is drawn by TerrainRenderer.
///
/// Every wall is the same model. A worn wall (game::WallVariant) is that model with
/// another pair of textures, so the three worn looks cost six textures and no geometry.
///
/// It owns nothing: the two models and the textures belong to the asset cache, which
/// must outlive this object, and the matrices belong to the MazeWorld given to draw.
class MazeRenderer {
public:
    /// Asks the cache for the two models of the maze and for the textures of the worn
    /// walls. A model that fails to load is logged by the cache and simply not drawn.
    /// A worn look whose textures fail to load is drawn with the plain stone.
    explicit MazeRenderer(assets::AssetCache& assets);

    /// Draws the whole maze. shader is the textured program or one of the two lit
    /// programs (lit, gouraud): it must be in use, with uView, uProjection and its own
    /// uniforms (uViewMode, uNormalMapEnabled, or the ones of the lighting) already set.
    /// The function sets the samplers and uEmissive (black: stone does not glow), and
    /// draws every pillar with game::drawModel and every wall with game::drawMesh (the
    /// wall mesh with the textures of the look of that wall). Both set the textures,
    /// uTint, uModel and uNormalMatrix.
    ///
    /// wallMatrices are the model matrices of the walls for this frame
    /// (game::roundWallMatrices): the ones of the world, with the walls that levers have
    /// opened lowered into the ground. The pillars are drawn from the world.
    ///
    /// wallVariants false draws every wall with the plain stone, whatever look the
    /// world chose for it (MazeWorld::wallVariants).
    void draw(const gfx::Shader& shader, const MazeWorld& world,
              std::span<const glm::mat4> wallMatrices, bool wallVariants) const;

private:
    // Not owned. nullptr when the model could not be loaded.
    const assets::LoadedModel* m_wall;
    const assets::LoadedModel* m_pillar;

    // The colour picture and the normal map of one look of the wall. Not owned.
    struct WallTextures {
        const gfx::Texture2D* color = nullptr;
        const gfx::Texture2D* normalMap = nullptr;
    };
    // One pair per look, found by the number of the WallVariant. The plain pair is the
    // one the wall model names. Left empty (two null pointers each) when the wall model
    // could not be loaded: nothing is drawn then.
    std::array<WallTextures, WALL_VARIANT_COUNT> m_wallTextures;
};

} // namespace game
