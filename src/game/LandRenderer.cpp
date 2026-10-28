// LandRenderer: draws the land around the maze, its lit windows and the far silhouettes.
#include "game/LandRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Files.hpp"
#include "core/Paths.hpp"
#include "game/GateLamp.hpp"
#include "game/LandMeshes.hpp"
#include "game/MazeWorld.hpp"
#include "game/ModelDraw.hpp"
#include "game/ShaderUniforms.hpp"
#include "game/Village.hpp"
#include "gfx/ColorSpace.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture2D.hpp"
#include "scene/Frustum.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>

namespace game {

namespace {

constexpr const char* GROUND_TEXTURE_FILE = "textures/ground.png";
constexpr const char* GROUND_NORMAL_MAP_FILE = "textures/ground_normal.png";
constexpr const char* WALL_TEXTURE_FILE = "textures/wall_stone.png";
constexpr const char* WALL_NORMAL_MAP_FILE = "textures/wall_stone_normal.png";
constexpr const char* HEDGE_TEXTURE_FILE = "textures/land_hedge.png";
constexpr const char* HEDGE_NORMAL_MAP_FILE = "textures/land_hedge_normal.png";

constexpr glm::mat4 IDENTITY{1.0F};
constexpr glm::vec3 NO_TINT{1.0F};
constexpr glm::vec3 NO_GLOW{0.0F};

// The stream: near black water that takes a little of the colour of the sky, matte.
constexpr glm::vec3 STREAM_TINT{0.018F, 0.024F, 0.034F};

// The windows of the land glow a little more than the ones of the village: they stand low,
// in the thick of the fog, which takes most of their light, and only the bloom, which the
// fog does not dim, carries them through it.
constexpr float LAND_WINDOW_GLOW = 8.0F;

// The silhouettes of the far band have the colour of the village.
constexpr glm::vec3 SILHOUETTE_TINT{0.05F, 0.065F, 0.11F};

const gfx::Texture2D* textureOr(assets::AssetCache& assets, const char* file,
                                gfx::ColorSpace colorSpace, const gfx::Texture2D& fallback) {
    const gfx::Texture2D* texture = assets.texture(core::assetPath(file), colorSpace);
    return texture != nullptr ? texture : &fallback;
}

gfx::Mesh uploaded(const TerrainMeshData& data) {
    return gfx::Mesh(data.vertices, data.indices);
}

gfx::Mesh emptyMesh() {
    return gfx::Mesh(std::span<const gfx::Vertex>{}, std::span<const std::uint32_t>{});
}

// Draws a mesh, unless it is empty: an empty mesh would still count as a draw call.
void drawIfAny(const gfx::Shader& shader, const gfx::Mesh& mesh, const gfx::Texture2D& texture,
               const gfx::Texture2D& normalMap, const glm::vec3& tint, const glm::mat4& matrix) {
    if (mesh.indexCount() > 0) {
        drawMesh(shader, mesh, texture, normalMap, tint, matrix);
    }
}

// The strength of the lights of one night: the newest one comes on, the ones before it
// are on.
float nightStrength(int night, int nightsLit, float newestStrength) {
    return night == nightsLit ? std::clamp(newestStrength, 0.0F, 1.0F) : 1.0F;
}

} // namespace

LandRenderer::LandRenderer(assets::AssetCache& assets)
    : m_ring(emptyMesh()),
      m_walls(emptyMesh()),
      m_hedges(emptyMesh()),
      m_stream(emptyMesh()),
      m_groundTexture(
          textureOr(assets, GROUND_TEXTURE_FILE, gfx::ColorSpace::Srgb, assets.whiteTexture())),
      m_groundNormalMap(textureOr(assets, GROUND_NORMAL_MAP_FILE, gfx::ColorSpace::Linear,
                                  assets.flatNormalTexture())),
      m_wallTexture(
          textureOr(assets, WALL_TEXTURE_FILE, gfx::ColorSpace::Srgb, assets.whiteTexture())),
      m_wallNormalMap(textureOr(assets, WALL_NORMAL_MAP_FILE, gfx::ColorSpace::Linear,
                                assets.flatNormalTexture())),
      m_hedgeTexture(
          textureOr(assets, HEDGE_TEXTURE_FILE, gfx::ColorSpace::Srgb, assets.whiteTexture())),
      m_hedgeNormalMap(textureOr(assets, HEDGE_NORMAL_MAP_FILE, gfx::ColorSpace::Linear,
                                 assets.flatNormalTexture())),
      m_white(&assets.whiteTexture()),
      m_flatNormal(&assets.flatNormalTexture()) {
    for (int i = 0; i < LAND_MODEL_COUNT; ++i) {
        const auto model = static_cast<LandModel>(i);
        const std::string file = "models/" + std::string{landModelFile(model)} + ".obj";
        m_batches.at(static_cast<std::size_t>(i)).model = assets.model(core::assetPath(file));
        if (landModelShape(model).windows) {
            std::string text;
            if (core::readTextFile(core::assetPath("models/" + landWindowsFile(model)), text)) {
                m_modelWindows.at(static_cast<std::size_t>(i)) = parseLandWindows(text);
            }
        }
    }
    for (int night = 0; night < LAND_LIT_NIGHTS; ++night) {
        m_windows.push_back(emptyMesh());
    }
    // The windows of the silhouettes, in the space of their models: they are drawn with
    // the matrix of the silhouette.
    for (int i = 0; i < FAR_SILHOUETTE_COUNT; ++i) {
        const LandModel model = farSilhouetteModel(static_cast<FarSilhouette>(i));
        std::vector<PlacedWindow> placed;
        for (const LandWindow& window : m_modelWindows.at(static_cast<std::size_t>(model))) {
            placed.push_back({.window = window});
        }
        m_silhouetteWindows.push_back(uploaded(buildWindowMesh(placed)));
    }
}

void LandRenderer::upload(const MazeWorld& world) {
    const Land& land = world.land;
    m_ring = uploaded(buildLandRingMesh(land, world.terrain));
    m_walls = uploaded(buildFieldWallMesh(land, world.terrain));
    m_hedges = uploaded(buildHedgerowMesh(land, world.terrain, world.seed));
    m_stream = uploaded(buildStreamMesh(land));

    for (ModelBatch& batch : m_batches) {
        batch.matrices.clear();
        batch.boxes.clear();
        batch.nearCount = 0;
    }
    // The windows of every piece, in the order of the pieces and of the windows file.
    std::vector<PlacedWindow> windows;
    for (std::size_t i = 0; i < land.pieces.size(); ++i) {
        const LandPiece& piece = land.pieces[i];
        const auto index = static_cast<std::size_t>(piece.model);
        ModelBatch& batch = m_batches.at(index);
        const glm::mat4 matrix = landPieceMatrix(piece);
        batch.matrices.push_back(matrix);
        if (batch.model != nullptr) {
            batch.boxes.push_back(scene::transformedBounds(batch.model->bounds, matrix));
        }
        if (i < land.nearCount) {
            batch.nearCount = batch.matrices.size();
        }
        for (const LandWindow& window : m_modelWindows.at(index)) {
            windows.push_back({.window = window, .matrix = matrix});
        }
    }
    m_pieceCount = land.pieces.size();

    // The windows dealt to the nights, one mesh for each night.
    const auto nights = dealLandWindows(windows.size(), world.seed);
    for (std::size_t night = 0; night < nights.size(); ++night) {
        std::vector<PlacedWindow> ofNight;
        ofNight.reserve(nights.at(night).size());
        for (const std::size_t window : nights.at(night)) {
            ofNight.push_back(windows.at(window));
        }
        m_windows.at(night) = uploaded(buildWindowMesh(ofNight));
    }
}

void LandRenderer::draw(const gfx::Shader& shader, const scene::Frustum* frustum, int nightsLit,
                        float newestStrength, float specularStrength) const {
    setModelSamplers(shader);
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
    // The ground of the ring is earth like the terrain. It is one mesh around the whole
    // maze and always drawn.
    drawIfAny(shader, m_ring, *m_groundTexture, *m_groundNormalMap, NO_TINT, IDENTITY);
    drawIfAny(shader, m_walls, *m_wallTexture, *m_wallNormalMap, NO_TINT, IDENTITY);
    drawIfAny(shader, m_hedges, *m_hedgeTexture, *m_hedgeNormalMap, NO_TINT, IDENTITY);
    // The rim catches the outline of a crown or a roof. The ground has none: seen from the
    // maze, all of it far out is seen almost edge-on.
    shader.setVec3(RIM_LIGHT_UNIFORM, LAND_RIM_LIGHT);
    for (const ModelBatch& batch : m_batches) {
        drawModel(shader, batch.model, batch.matrices, batch.boxes, frustum);
    }
    shader.setVec3(RIM_LIGHT_UNIFORM, NO_GLOW);

    // The stream and the windows have no highlight: water this dark would show the
    // flashlight as a white spot, and a lit window shows its own light only.
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, 0.0F);
    drawIfAny(shader, m_stream, *m_white, *m_flatNormal, STREAM_TINT, IDENTITY);
    const glm::vec3 amber = gfx::srgbToLinear(GATE_LAMP_WARM_COLOR);
    const int lit = std::clamp(nightsLit, 0, LAND_LIT_NIGHTS);
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{LAND_WINDOW_GLOW});
    for (int night = 1; night <= lit; ++night) {
        drawIfAny(shader, m_windows.at(static_cast<std::size_t>(night - 1)), *m_white,
                  *m_flatNormal, amber * nightStrength(night, nightsLit, newestStrength), IDENTITY);
    }
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, specularStrength);
}

void setLandLight(const gfx::Shader& shader, const MazeWorld& world) {
    const glm::vec3 to{static_cast<float>(world.maze.width()) * CELL_SIZE, 0.0F,
                       static_cast<float>(world.maze.height()) * CELL_SIZE};
    shader.setVec3(SKY_FILL_UNIFORM, LAND_SKY_FILL);
    shader.setVec3(LAND_FROM_UNIFORM, glm::vec3{0.0F});
    shader.setVec3(LAND_TO_UNIFORM, to);
    shader.setFloat(LAND_FADE_UNIFORM, TERRAIN_MARGIN);
}

void clearLandLight(const gfx::Shader& shader) {
    shader.setVec3(SKY_FILL_UNIFORM, NO_GLOW);
    shader.setVec3(RIM_LIGHT_UNIFORM, NO_GLOW);
    shader.setFloat(LAND_FADE_UNIFORM, 0.0F);
}

void LandRenderer::drawShadowCasters(const gfx::Shader& shader,
                                     const scene::Frustum* frustum) const {
    drawIfAny(shader, m_walls, *m_wallTexture, *m_wallNormalMap, NO_TINT, IDENTITY);
    drawIfAny(shader, m_hedges, *m_hedgeTexture, *m_hedgeNormalMap, NO_TINT, IDENTITY);
    for (const ModelBatch& batch : m_batches) {
        const std::span<const glm::mat4> matrices{batch.matrices.data(), batch.nearCount};
        const std::span<const scene::Aabb> boxes{batch.boxes.data(),
                                                 std::min(batch.nearCount, batch.boxes.size())};
        drawModel(shader, batch.model, matrices, boxes, frustum);
    }
}

void LandRenderer::drawFarBand(const gfx::Shader& shader, Direction villageSide, float farPlane,
                               float elevationDegrees, int nightsLit, float newestStrength,
                               const glm::vec3& towardMoon) const {
    const glm::vec3 amber = gfx::srgbToLinear(GATE_LAMP_WARM_COLOR);
    shader.setVec3(MOON_TOWARD_UNIFORM, towardMoon);
    for (int i = 0; i < FAR_SILHOUETTE_COUNT; ++i) {
        const auto silhouette = static_cast<FarSilhouette>(i);
        const glm::mat4 matrix =
            farSilhouetteMatrix(silhouette, villageSide, farPlane, elevationDegrees);
        const ModelBatch& batch =
            m_batches.at(static_cast<std::size_t>(farSilhouetteModel(silhouette)));
        shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
        shader.setVec3(FAR_LIGHT_UNIFORM, VILLAGE_FAR_LIGHT);
        if (batch.model != nullptr) {
            for (const assets::ModelPart& part : batch.model->parts) {
                drawMesh(shader, batch.model->mesh, *part.texture, *part.normalMap, SILHOUETTE_TINT,
                         matrix);
            }
        }
        const int night = farSilhouetteNight(silhouette);
        shader.setVec3(FAR_LIGHT_UNIFORM, NO_GLOW);
        if (night <= nightsLit) {
            shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{VILLAGE_LIGHT_GLOW});
            drawIfAny(shader, m_silhouetteWindows.at(static_cast<std::size_t>(i)), *m_white,
                      *m_flatNormal, amber * nightStrength(night, nightsLit, newestStrength),
                      matrix);
        }
    }
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
}

} // namespace game
