// LandRenderer: draws the land around the maze, its lit windows and the far silhouettes.
#pragma once

#include "game/Land.hpp"
#include "gfx/Mesh.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
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

struct MazeWorld;

/// The light of the sky and the moonlit edges the land gets in the lit program (lit.frag),
/// linear colours. Little: on level ground far out the sky fill is about four times the
/// ambient light, under a third of the moon, and the rim only shows where a
/// crown or a roof is seen edge-on. It stays a dark night, but a crest and the crowns on it
/// read as shapes and not as one flat black cut-out.
constexpr glm::vec3 LAND_SKY_FILL{0.02F, 0.028F, 0.05F};
constexpr glm::vec3 LAND_RIM_LIGHT{0.04F, 0.06F, 0.12F};

/// Gives the lit program the light of the land (LAND_SKY_FILL, and the box of the maze it
/// grows from over TERRAIN_MARGIN): call it before the terrain and the land are drawn, and
/// clearLandLight after them, so nothing else gets it. LandRenderer::draw adds the rim.
void setLandLight(const gfx::Shader& shader, const MazeWorld& world);
void clearLandLight(const gfx::Shader& shader);

/// The OpenGL side of game::Land. The places are plain data (game::placeLand) and the
/// triangles of what is built in code too (game/LandMeshes.hpp). This class owns the
/// meshes they are copied into, the model matrices and boxes of the pieces, and the meshes
/// of the windows of every night.
///
/// It owns OpenGL objects, so it must be destroyed before the window. The models and the
/// pictures belong to the asset cache, which must outlive this object.
class LandRenderer {
public:
    /// Asks the cache for the models and the pictures. A model that fails to load is logged
    /// by the cache and not drawn, a picture is replaced by the white texture or the flat
    /// normal map. Nothing of a land is drawn until upload was called.
    explicit LandRenderer(assets::AssetCache& assets);

    /// Builds everything for the land of a world: call it whenever the world was built
    /// again, for a new maze and for a new height scale.
    void upload(const MazeWorld& world);

    /// Draws the land in the scene pass: the ring, the walls, the hedgerows, the stream,
    /// the pieces and the windows of the first nightsLit nights, the last of them at
    /// newestStrength. shader is the textured program or a lit one, prepared exactly as
    /// for TerrainRenderer::draw. frustum culls the pieces (nullptr: all are drawn).
    /// specularStrength is the strength of the stone, put back after the stream and the
    /// windows, which have none. Leaves uEmissive black.
    void draw(const gfx::Shader& shader, const scene::Frustum* frustum, int nightsLit,
              float newestStrength, float specularStrength) const;

    /// Draws what of the land casts a shadow: the pieces of the near band, the field walls
    /// and the hedgerows. Everything farther out lies outside the box of the moon.
    void drawShadowCasters(const gfx::Shader& shader, const scene::Frustum* frustum) const;

    /// Draws the mill and the church tower of the far band and the windows of those whose
    /// night is lit. shader is the textured program, prepared as for VillageRenderer::draw
    /// (uView without its translation), and gets the same light of the moon and the sky
    /// (towardMoon, VILLAGE_FAR_LIGHT). Leaves uEmissive and uFarLight black.
    void drawFarBand(const gfx::Shader& shader, Direction villageSide, float farPlane,
                     float elevationDegrees, int nightsLit, float newestStrength,
                     const glm::vec3& towardMoon) const;

    /// What was uploaded, for the perf probe and the tests of the budget.
    std::size_t pieceCount() const { return m_pieceCount; }

private:
    struct ModelBatch {
        const assets::LoadedModel* model = nullptr;
        std::vector<glm::mat4> matrices;
        std::vector<scene::Aabb> boxes;
        // The first nearCount matrices are of the near band (they cast shadows).
        std::size_t nearCount = 0;
    };

    std::array<ModelBatch, LAND_MODEL_COUNT> m_batches{};
    std::size_t m_pieceCount = 0;

    gfx::Mesh m_ring;
    gfx::Mesh m_walls;
    gfx::Mesh m_hedges;
    gfx::Mesh m_stream;
    // The windows of nights 1 to LAND_LIT_NIGHTS, one mesh each, and of the silhouettes,
    // one mesh each in the space of its model. gfx::Mesh has no empty state of its own, so
    // these are lists: an empty one is not drawn.
    std::vector<gfx::Mesh> m_windows;
    std::vector<gfx::Mesh> m_silhouetteWindows;

    // The windows of every model with windows, in its own space. Empty for the others.
    std::array<std::vector<LandWindow>, LAND_MODEL_COUNT> m_modelWindows{};

    // Not owned, never null: the pictures of the cache or its stand-ins.
    const gfx::Texture2D* m_groundTexture;
    const gfx::Texture2D* m_groundNormalMap;
    const gfx::Texture2D* m_wallTexture;
    const gfx::Texture2D* m_wallNormalMap;
    const gfx::Texture2D* m_hedgeTexture;
    const gfx::Texture2D* m_hedgeNormalMap;
    const gfx::Texture2D* m_white;
    const gfx::Texture2D* m_flatNormal;
};

} // namespace game
