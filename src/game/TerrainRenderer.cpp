// TerrainRenderer: draws the terrain, one large mesh with the ground texture.
// See docs/modules/renderer/terrain.md
#include "game/TerrainRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/GlCheck.hpp"
#include "core/Paths.hpp"
#include "game/ModelDraw.hpp"
#include "game/ShaderUniforms.hpp"
#include "game/Terrain.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture2D.hpp"

#include <cstdint>
#include <span>

namespace game {

namespace {

// Picture files, relative to the assets directory. Both tile: the terrain repeats them
// every GROUND_TEXTURE_SPAN metres.
constexpr const char* GROUND_TEXTURE_FILE = "textures/ground.png";
constexpr const char* GROUND_NORMAL_MAP_FILE = "textures/ground_normal.png";

// The ground has the colours of its texture: a white tint changes nothing.
constexpr glm::vec3 NO_TINT{1.0F};

// The vertices of the terrain are already in world space, so its model matrix is the
// identity matrix: it moves nothing.
constexpr glm::mat4 IDENTITY{1.0F};

// The texture of the cache, or the stand-in when the file could not be loaded.
const gfx::Texture2D* textureOr(assets::AssetCache& assets, const char* file,
                                const gfx::Texture2D& fallback) {
    const gfx::Texture2D* texture = assets.texture(core::assetPath(file));
    return texture != nullptr ? texture : &fallback;
}

} // namespace

TerrainRenderer::TerrainRenderer(assets::AssetCache& assets)
    // An empty mesh: no vertex and no index. Drawing it draws nothing.
    : m_mesh(std::span<const gfx::Vertex>{}, std::span<const std::uint32_t>{}),
      m_texture(textureOr(assets, GROUND_TEXTURE_FILE, assets.whiteTexture())),
      m_normalMap(textureOr(assets, GROUND_NORMAL_MAP_FILE, assets.flatNormalTexture())) {}

void TerrainRenderer::upload(const TerrainMeshData& mesh) {
    // A gfx::Mesh is filled once, when it is created. So a new mesh is made, and the
    // move assignment deletes the buffers of the old one.
    m_mesh = gfx::Mesh(mesh.vertices, mesh.indices);
}

void TerrainRenderer::draw(const gfx::Shader& shader, bool wireframe) const {
    setModelSamplers(shader);
    // Earth gives off no light of its own.
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});

    // Wireframe: the rasterizer draws the three edges of every triangle as lines instead
    // of filling it. Everything else (the shader, the textures, the lighting) stays the
    // same, so the lines have the colours of the ground. The core profile only knows
    // GL_FRONT_AND_BACK here: both sides of a triangle always share one mode.
    if (wireframe) {
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    }

    drawMesh(shader, m_mesh, *m_texture, *m_normalMap, NO_TINT, IDENTITY);

    // Back to filled triangles: the mode is global state, and the walls drawn after the
    // terrain must not turn into lines too.
    if (wireframe) {
        GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    }
}

} // namespace game
