// MazeRenderer: draws the walls and the pillars of a maze with their models.
#include "game/MazeRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "game/MazeWorld.hpp"
#include "game/ModelDraw.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture2D.hpp"

#include <algorithm>
#include <cstddef>

namespace game {

namespace {

// Model files, relative to the assets directory.
constexpr const char* WALL_MODEL_FILE = "models/wall_straight.obj";
constexpr const char* PILLAR_MODEL_FILE = "models/wall_pillar.obj";

// The texture files of the worn walls, relative to the assets directory: the colour
// picture and the normal map of each look, in the order of WallVariant. The plain look
// has no entry here: its two files are named by the wall model.
struct WallTextureFiles {
    const char* color;
    const char* normalMap;
};
constexpr std::array<WallTextureFiles, WALL_VARIANT_COUNT - 1> WORN_WALL_TEXTURE_FILES{{
    {.color = "textures/wall_cracked.png", .normalMap = "textures/wall_cracked_normal.png"},
    {.color = "textures/wall_mossy.png", .normalMap = "textures/wall_mossy_normal.png"},
    {.color = "textures/wall_damaged.png", .normalMap = "textures/wall_damaged_normal.png"},
}};

} // namespace

MazeRenderer::MazeRenderer(assets::AssetCache& assets)
    : m_wall(assets.model(core::assetPath(WALL_MODEL_FILE))),
      m_pillar(assets.model(core::assetPath(PILLAR_MODEL_FILE))) {
    if (m_wall == nullptr || m_wall->parts.empty()) {
        return;
    }
    // The plain look: the textures the wall model was loaded with. The model has one
    // material, so its first part is the whole wall.
    const WallTextures plain{.color = m_wall->parts.front().texture,
                             .normalMap = m_wall->parts.front().normalMap};
    m_wallTextures.fill(plain);

    // The worn looks replace the plain pair in their places. The colour picture is an
    // sRGB texture and the normal map is linear data, exactly as the model loader asks
    // for the two files of a material. A look with a missing file keeps the plain pair
    // (the cache has logged the error): its walls must still be drawn.
    for (std::size_t i = 0; i < WORN_WALL_TEXTURE_FILES.size(); ++i) {
        const WallTextures worn{
            .color = assets.texture(core::assetPath(WORN_WALL_TEXTURE_FILES[i].color),
                                    gfx::ColorSpace::Srgb),
            .normalMap = assets.texture(core::assetPath(WORN_WALL_TEXTURE_FILES[i].normalMap),
                                        gfx::ColorSpace::Linear)};
        if (worn.color != nullptr && worn.normalMap != nullptr) {
            m_wallTextures[i + 1] = worn;
        }
    }
}

void MazeRenderer::draw(const gfx::Shader& shader, const MazeWorld& world,
                        std::span<const glm::mat4> wallMatrices, bool wallVariants) const {
    setModelSamplers(shader);
    // Stone gives off no light of its own. A uniform keeps its value from one draw call
    // to the next, and the crystals set this one, so it is set back in every frame.
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});

    // The walls: one mesh, drawn once per wall with the textures of its look. The
    // matrices are the ones of the round and the looks the ones of the world. Both lists
    // are in the order of MazeWorld::walls, so the same number names the same wall, and
    // a wall a lever has lowered keeps its look on the way down.
    if (m_wall != nullptr && !m_wall->parts.empty()) {
        const glm::vec3& tint = m_wall->parts.front().color;
        // A wall without a look in the list (a matrix list of another world) is plain.
        const std::size_t withLook =
            wallVariants ? std::min(wallMatrices.size(), world.wallVariants.size()) : 0;
        for (std::size_t i = 0; i < wallMatrices.size(); ++i) {
            const WallVariant variant = i < withLook ? world.wallVariants[i] : WallVariant::Plain;
            const WallTextures& textures = m_wallTextures.at(static_cast<std::size_t>(variant));
            drawMesh(shader, m_wall->mesh, *textures.color, *textures.normalMap, tint,
                     wallMatrices[i]);
        }
    }
    drawModel(shader, m_pillar, world.pillarMatrices);
}

} // namespace game
