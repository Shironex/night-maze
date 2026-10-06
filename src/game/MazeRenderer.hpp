// MazeRenderer: draws the walls and the pillars of a maze with their models.
// See docs/modules/game/maze-rendering.md
#pragma once

#include <glm/glm.hpp>

#include <span>

namespace assets {
class AssetCache;
struct LoadedModel;
} // namespace assets

namespace gfx {
class Shader;
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
/// It owns nothing: the two models belong to the asset cache, which must outlive this
/// object, and the matrices belong to the MazeWorld given to draw.
class MazeRenderer {
public:
    /// Asks the cache for the two models of the maze. A model that fails to load is
    /// logged by the cache and simply not drawn.
    explicit MazeRenderer(assets::AssetCache& assets);

    /// Draws the whole maze. shader is the textured program or one of the two lit
    /// programs (lit, gouraud): it must be in use, with uView, uProjection and its own
    /// uniforms (uViewMode, uNormalMapEnabled, or the ones of the lighting) already set.
    /// The function sets the samplers and uEmissive (black: stone does not glow), and
    /// draws every object with game::drawModel, which sets the textures, uTint, uModel
    /// and uNormalMatrix.
    ///
    /// wallMatrices are the model matrices of the walls for this frame
    /// (game::roundWallMatrices): the ones of the world, with the walls that levers have
    /// opened lowered into the ground. The pillars are drawn from the world.
    void draw(const gfx::Shader& shader, const MazeWorld& world,
              std::span<const glm::mat4> wallMatrices) const;

private:
    // Not owned. nullptr when the model could not be loaded.
    const assets::LoadedModel* m_wall;
    const assets::LoadedModel* m_pillar;
};

} // namespace game
