// MazeRenderer: draws the walls, the pillars, the stile and the stone sheep of a maze with
// their models.
#pragma once

#include "game/MazeWorld.hpp"
#include "game/WallVariants.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <array>
#include <span>
#include <vector>

namespace assets {
class AssetCache;
struct LoadedModel;
} // namespace assets

namespace gfx {
class Shader;
class Texture2D;
} // namespace gfx

namespace scene {
struct Frustum;
} // namespace scene

namespace game {

/// The boxes around what MazeRenderer draws, in world space: what a pass asks to learn
/// whether a wall, a pillar or a stone sheep can be seen at all (view frustum culling,
/// scene::canBeSeen). They are computed once for a world (MazeRenderer::bounds) and
/// only read in every frame.
///
/// They are not the collision boxes of MazeWorld::colliders. A collision box is what
/// the player cannot walk through: 0.3 m thick and 3 m high for every wall. These boxes
/// hold everything that is DRAWN: the model is wider at its foot, a crowned wall is
/// higher, the stile has steps that stand out. They come from the vertices of the
/// models (assets::LoadedModel::bounds).
struct MazeBounds {
    /// One box per wall, in the order of MazeWorld::walls. It holds the wall in every
    /// look it can be drawn with: the plain model and the model of its own look, since
    /// the debug UI can switch the looks off at any moment, and for the wall of the
    /// stile the stile model and its post. The box of a wall that a lever lowers
    /// reaches down by GATE_SINK_DEPTH, so it holds the wall at every moment of its way
    /// into the ground.
    std::vector<scene::Aabb> walls;
    /// One box per pillar and per stone sheep, in the order of MazeWorld::pillars and
    /// MazeWorld::sheep.
    std::vector<scene::Aabb> pillars;
    std::vector<scene::Aabb> sheep;
};

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
/// A painted wall (cracked, mossy or damaged, see game::WallVariant) is the wall model
/// with another pair of textures, so those three looks cost six textures and no
/// geometry. A shaped wall (crowned or broken) is a model of its own with the plain
/// stone on it. The wall that carries the stile (MazeWorld::stileWall) is drawn with the
/// stile model, and the post of the stile with the matrix of that wall. Every stone sheep
/// (MazeWorld::sheep) is the same model with its own matrix.
///
/// It owns nothing: the models and the textures belong to the asset cache, which
/// must outlive this object, and the matrices belong to the MazeWorld given to draw.
class MazeRenderer {
public:
    /// Asks the cache for the models of the maze and for the textures of the painted
    /// walls. A model that fails to load is logged by the cache and simply not drawn.
    /// A look whose textures or model fail to load is drawn as a plain wall.
    explicit MazeRenderer(assets::AssetCache& assets);

    /// Draws the whole maze. shader is the textured program or one of the two lit
    /// programs (lit, gouraud): it must be in use, with uView, uProjection and its own
    /// uniforms (uViewMode, uNormalMapEnabled, or the ones of the lighting) already set.
    /// The function sets the samplers and uEmissive (black: stone does not glow), and
    /// draws every pillar and every stone sheep with game::drawModel and every wall with
    /// game::drawMesh (the
    /// mesh and the textures of the look of that wall). Both set the textures, uTint,
    /// uModel and uNormalMatrix.
    ///
    /// wallMatrices are the model matrices of the walls for this frame
    /// (game::roundWallMatrices): the ones of the world, with the walls that levers have
    /// opened lowered into the ground. The pillars are drawn from the world.
    ///
    /// wallVariants false draws every wall as a plain wall, whatever look the world
    /// chose for it (MazeWorld::wallVariants).
    ///
    /// parts chooses what is drawn (game::MazeParts): all of the maze, or only what
    /// never moves, or only the walls of the levers. Fixed and LeverWalls together make
    /// exactly the draw calls of All.
    ///
    /// frustum is the frustum of the pass, made from the uView and uProjection the
    /// program has (scene::frustumFromMatrix): a wall, a pillar or a sheep whose box in
    /// bounds lies outside of it is not drawn. bounds must be the boxes of this world
    /// (MazeRenderer::bounds). An object the lists have no box for is drawn. nullptr:
    /// everything is drawn and bounds is not read.
    void draw(const gfx::Shader& shader, const MazeWorld& world,
              std::span<const glm::mat4> wallMatrices, bool wallVariants, MazeParts parts,
              const MazeBounds& bounds, const scene::Frustum* frustum) const;

    /// The boxes of everything draw draws of this world, see MazeBounds. Called once
    /// when a maze is built or put on a new terrain, not in every frame: nothing in it
    /// depends on the round or on the camera.
    MazeBounds bounds(const MazeWorld& world) const;

private:
    // Not owned. nullptr when the model could not be loaded.
    const assets::LoadedModel* m_wall;
    const assets::LoadedModel* m_pillar;
    const assets::LoadedModel* m_stilePost;
    const assets::LoadedModel* m_stoneSheep;

    // What one look of the wall is drawn with: a model, and the colour picture and the
    // normal map that go on it. Not owned.
    struct WallLook {
        const assets::LoadedModel* model = nullptr;
        const gfx::Texture2D* color = nullptr;
        const gfx::Texture2D* normalMap = nullptr;
    };
    // One entry per look, found by the number of the WallVariant. The plain entry is
    // the wall model with the textures it names. Left empty (null pointers) when the
    // wall model could not be loaded: nothing is drawn then.
    std::array<WallLook, WALL_VARIANT_COUNT> m_wallLooks;
    // What the wall that carries the stile is drawn with (MazeWorld::stileWall): the
    // stile model in the plain stone, whatever look the list above has for that wall.
    WallLook m_stileLook;
};

} // namespace game
