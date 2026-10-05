// TerrainRenderer: draws the terrain, one large mesh with the ground texture.
// See docs/modules/renderer/terrain.md
#pragma once

#include "gfx/Mesh.hpp"

namespace assets {
class AssetCache;
} // namespace assets

namespace gfx {
class Shader;
class Texture2D;
} // namespace gfx

namespace game {

struct TerrainMeshData;

/// The OpenGL side of the terrain. The heights and the triangles are plain data
/// (game::Terrain, game::buildTerrainMesh). This class owns the mesh they are copied
/// into and knows the two textures of the ground.
///
/// It owns an OpenGL object, so it must be destroyed before the window. The textures
/// belong to the asset cache, which must outlive this object.
class TerrainRenderer {
public:
    /// Asks the cache for the ground texture and its normal map. A picture that fails to
    /// load is logged by the cache and replaced by the white texture or the flat normal
    /// map. The mesh starts empty: nothing is drawn until upload was called.
    explicit TerrainRenderer(assets::AssetCache& assets);

    /// Replaces the mesh on the graphics card by the given vertices and indices. Call it
    /// whenever the terrain was built again: for a new maze and for a new height scale.
    void upload(const TerrainMeshData& mesh);

    /// Draws the terrain. shader is the textured program or one of the two lit programs
    /// (lit, gouraud), prepared exactly as for MazeRenderer::draw: in use, with uView,
    /// uProjection and its own uniforms set. The terrain is drawn like a model, so it
    /// gets the lighting mode and the debug views of the walls.
    ///
    /// wireframe draws the edges of the triangles instead of their faces
    /// (glPolygonMode), which shows the grid the terrain is made of. Only this draw is
    /// affected: the mode is set back to filled triangles before the function returns.
    void draw(const gfx::Shader& shader, bool wireframe) const;

private:
    gfx::Mesh m_mesh;

    // Not owned, never null: the pictures of the cache or its stand-ins.
    const gfx::Texture2D* m_texture;
    const gfx::Texture2D* m_normalMap;
};

} // namespace game
