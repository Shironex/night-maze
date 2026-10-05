// MazeRenderer: draws the floor, the walls and the pillars of a maze with their models.
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

/// What the textured shader shows. The numbers are the values of the uniform uViewMode
/// in textured.frag, so the values here and the comparisons there must stay in step.
enum class ViewMode {
    Textured = 0, ///< the texture multiplied by the colour of the material
    Normals = 1,  ///< the normal used for shading as a colour (a debug view, not lighting)
    Uvs = 2,      ///< the texture coordinate as a colour (a debug view)
};

/// Draws a MazeWorld: one floor tile per cell, one wall model per wall segment and one
/// pillar model per pillar, each with its own model matrix (one draw call per object).
///
/// It owns nothing: the three models belong to the asset cache, which must outlive this
/// object, and the matrices belong to the MazeWorld given to draw.
class MazeRenderer {
public:
    /// Asks the cache for the three models of the maze. A model that fails to load is
    /// logged by the cache and simply not drawn.
    explicit MazeRenderer(assets::AssetCache& assets);

    /// Draws the whole maze. shader is the textured program or one of the two lit
    /// programs (lit, gouraud): it must be in use, with uView, uProjection and its own
    /// uniforms (uViewMode, uNormalMapEnabled, or the ones of the lighting) already set.
    /// The function sets uTexture and uNormalMap, binds the two textures and sets uTint
    /// for every part, and sets uModel and uNormalMatrix for every object. The textured
    /// program has no uNormalMatrix and the gouraud program no uNormalMap: a uniform
    /// a program does not have is ignored.
    void draw(const gfx::Shader& shader, const MazeWorld& world) const;

private:
    /// Draws one model once for every matrix in modelMatrices.
    static void drawInstances(const gfx::Shader& shader, const assets::LoadedModel* model,
                              std::span<const glm::mat4> modelMatrices);

    // Not owned. nullptr when the model could not be loaded.
    const assets::LoadedModel* m_floorTile;
    const assets::LoadedModel* m_wall;
    const assets::LoadedModel* m_pillar;
};

} // namespace game
