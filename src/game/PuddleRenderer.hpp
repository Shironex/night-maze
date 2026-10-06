// PuddleRenderer: draws the puddles, thin films of water that lie on the ground.
// See docs/modules/renderer/env-mapping.md
#pragma once

#include "gfx/Mesh.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace assets {
class AssetCache;
} // namespace assets

namespace gfx {
class Shader;
class Texture2D;
} // namespace gfx

namespace game {

class Terrain;
struct Puddle;

/// The OpenGL side of the puddles. Where they lie is plain data (game::puddlesOnGround),
/// and so are their triangles (game::buildPuddleMesh). This class copies those
/// triangles to the graphics card, all puddles in ONE mesh, and draws them.
///
/// One mesh for all and not one mesh drawn once per puddle (as the wall model is drawn
/// for every wall): every puddle follows the ground it lies on, so no two puddles have
/// the same shape. Their vertices are in world space already, and the mesh is drawn
/// with the identity as its model matrix.
///
/// It owns an OpenGL object, so it must be destroyed before the window. The two
/// textures belong to the asset cache, which must outlive this object.
class PuddleRenderer {
public:
    /// Asks the cache for the two stand-in textures: a puddle has no picture and no
    /// relief of its own. It starts without puddles: nothing is drawn until upload was
    /// called.
    explicit PuddleRenderer(assets::AssetCache& assets);

    /// Replaces the puddles by the given ones: builds their mesh on terrain
    /// (game::buildPuddleMesh) and copies it to the graphics card. Call it whenever the
    /// puddles were placed again or the ground under them changed: for a new maze,
    /// a new terrain or a new share of cells.
    void upload(const Terrain& terrain, std::span<const Puddle> puddles);

    /// Number of puddles that are drawn.
    std::size_t puddleCount() const { return m_puddleCount; }

    /// Draws every puddle. shader is the reflect program, or the textured program for
    /// the debug views, prepared as for MazeRenderer::draw: in use, with uView,
    /// uProjection and its own uniforms set. The function sets the samplers, uEmissive
    /// (black: water gives off no light), uTint (the colour of the water), uModel and
    /// uNormalMatrix (the identity) and the two uniforms of the soft rim.
    ///
    /// THE SOFT RIM. The water is drawn with alpha blending: the reflect program gives
    /// every fragment an alpha, below 1 in the middle and falling to 0 at the rim
    /// (over the outer PUDDLE_RIM_FADE of the radius), and OpenGL mixes the water with the ground
    /// that is in the framebuffer already. So the ground shows through the water a little, and the
    /// puddle has no edge. Blending is switched on for this draw call only and put back afterwards,
    /// and the uniform of the rim is set back to 0, because the crystals are drawn with
    /// the same program and must stay solid. The textured program of the debug views
    /// has no such uniforms and writes an alpha of 1: there the puddles are solid.
    ///
    /// DEPTH. The depth test stays on (a wall or a blade of grass in front hides the
    /// water), but the water does not WRITE depth. A fragment that is almost invisible
    /// would otherwise claim its pixel in the depth buffer. Nothing is lost by it: the
    /// ground a few millimetres below has written its depth already, and that is the
    /// depth the fog of the composite pass should read. The puddles are drawn after
    /// the ground and the grass and before the sky, which is drawn only where nothing
    /// has written depth, so the order is right for blending.
    ///
    /// The water is not lifted off the ground with glPolygonOffset: its vertices
    /// already lie PUDDLE_LIFT above the ground.
    void draw(const gfx::Shader& shader) const;

private:
    // The triangles of all puddles. Empty while there are no puddles: a gfx::Mesh
    // cannot be created without vertices.
    std::optional<gfx::Mesh> m_mesh;
    std::size_t m_puddleCount = 0;

    // Not owned, never null: the white texture and the flat normal map of the cache.
    const gfx::Texture2D* m_texture;
    const gfx::Texture2D* m_normalMap;
};

} // namespace game
