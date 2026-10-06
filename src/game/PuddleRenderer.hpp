// PuddleRenderer: draws the puddles, one flat disc per puddle.
// See docs/modules/renderer/env-mapping.md
#pragma once

#include "gfx/Mesh.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace assets {
class AssetCache;
} // namespace assets

namespace gfx {
class Shader;
class Texture2D;
} // namespace gfx

namespace game {

struct Puddle;

/// The OpenGL side of the puddles. Where they lie is plain data (game::puddlesOnGround).
/// This class owns the mesh of one disc (game::buildPuddleMesh) and a model matrix per
/// puddle: the same disc is drawn once for every puddle, like the wall model for every
/// wall.
///
/// It owns an OpenGL object, so it must be destroyed before the window. The two
/// textures belong to the asset cache, which must outlive this object.
class PuddleRenderer {
public:
    /// Uploads the disc and asks the cache for its two stand-in textures: a puddle has
    /// no picture and no relief of its own. It starts without puddles: nothing is drawn
    /// until upload was called.
    explicit PuddleRenderer(assets::AssetCache& assets);

    /// Replaces the puddles by the given ones (their model matrices, the mesh stays).
    /// Call it whenever the puddles were placed again: for a new maze, a new terrain or
    /// a new share of cells.
    void upload(std::span<const Puddle> puddles);

    /// Number of puddles that are drawn.
    std::size_t puddleCount() const { return m_matrices.size(); }

    /// Draws every puddle. shader is the reflect program, or the textured program for
    /// the debug views, prepared as for MazeRenderer::draw: in use, with uView,
    /// uProjection and its own uniforms set. The function sets the samplers, uEmissive
    /// (black: water gives off no light), uTint (the colour of dark water) and, for
    /// every puddle, uModel and uNormalMatrix.
    ///
    /// A puddle is not lifted off the ground with glPolygonOffset: its disc already
    /// lies PUDDLE_DEPTH above the lowest ground under it, and where the ground is
    /// higher than the water the depth test hides the disc, as it should.
    void draw(const gfx::Shader& shader) const;

private:
    gfx::Mesh m_disc;
    // One model matrix per puddle (game::puddleModelMatrix).
    std::vector<glm::mat4> m_matrices;

    // Not owned, never null: the white texture and the flat normal map of the cache.
    const gfx::Texture2D* m_texture;
    const gfx::Texture2D* m_normalMap;
};

} // namespace game
