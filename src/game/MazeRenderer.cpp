// MazeRenderer: draws the walls, the pillars, the stile and the stone sheep of a maze with
// their models.
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

// The stile of the start cell: the wall with the notch and the steps, which is drawn in
// place of the wall model on one segment, and the post beside it (game/Stile.hpp).
constexpr const char* STILE_MODEL_FILE = "models/stile.obj";
constexpr const char* STILE_POST_MODEL_FILE = "models/stile_post.obj";

// A stone sheep: 0.9 m long, with its head along +X and its origin on the ground under
// its middle (game/StoneSheep.hpp).
constexpr const char* STONE_SHEEP_MODEL_FILE = "models/stone_sheep.obj";

// The models of the two shaped looks, Crowned and Broken, in the order of WallVariant.
// They follow the painted looks in the enum.
constexpr std::array<const char*, WALL_VARIANT_COUNT - 1 - PAINTED_WALL_VARIANT_COUNT>
    SHAPED_WALL_MODEL_FILES{"models/wall_straight_crown.obj", "models/wall_straight_broken.obj"};

// The texture files of the painted walls, relative to the assets directory: the colour
// picture and the normal map of each look, in the order of WallVariant. The plain look
// has no entry here: its two files are named by the wall model.
struct WallTextureFiles {
    const char* color;
    const char* normalMap;
};
constexpr std::array<WallTextureFiles, PAINTED_WALL_VARIANT_COUNT> WORN_WALL_TEXTURE_FILES{{
    {.color = "textures/wall_cracked.png", .normalMap = "textures/wall_cracked_normal.png"},
    {.color = "textures/wall_mossy.png", .normalMap = "textures/wall_mossy_normal.png"},
    {.color = "textures/wall_damaged.png", .normalMap = "textures/wall_damaged_normal.png"},
}};

} // namespace

MazeRenderer::MazeRenderer(assets::AssetCache& assets)
    : m_wall(assets.model(core::assetPath(WALL_MODEL_FILE))),
      m_pillar(assets.model(core::assetPath(PILLAR_MODEL_FILE))),
      m_stilePost(assets.model(core::assetPath(STILE_POST_MODEL_FILE))),
      m_stoneSheep(assets.model(core::assetPath(STONE_SHEEP_MODEL_FILE))) {
    if (m_wall == nullptr || m_wall->parts.empty()) {
        return;
    }
    // The plain look: the textures the wall model was loaded with. The model has one
    // material, so its first part is the whole wall.
    const WallLook plain{.model = m_wall,
                         .color = m_wall->parts.front().texture,
                         .normalMap = m_wall->parts.front().normalMap};
    m_wallLooks.fill(plain);

    // The painted looks replace the plain pair in their places. The colour picture is an
    // sRGB texture and the normal map is linear data, exactly as the model loader asks
    // for the two files of a material. A look with a missing file keeps the plain pair
    // (the cache has logged the error): its walls must still be drawn.
    for (std::size_t i = 0; i < WORN_WALL_TEXTURE_FILES.size(); ++i) {
        const WallLook worn{
            .model = m_wall,
            .color = assets.texture(core::assetPath(WORN_WALL_TEXTURE_FILES[i].color),
                                    gfx::ColorSpace::Srgb),
            .normalMap = assets.texture(core::assetPath(WORN_WALL_TEXTURE_FILES[i].normalMap),
                                        gfx::ColorSpace::Linear)};
        if (worn.color != nullptr && worn.normalMap != nullptr) {
            m_wallLooks[i + 1] = worn;
        }
    }

    // The shaped looks replace the model and keep the plain stone: their files name the
    // same two pictures. A look whose model is missing stays a plain wall.
    for (std::size_t i = 0; i < SHAPED_WALL_MODEL_FILES.size(); ++i) {
        const assets::LoadedModel* shaped =
            assets.model(core::assetPath(SHAPED_WALL_MODEL_FILES[i]));
        if (shaped != nullptr && !shaped->parts.empty()) {
            m_wallLooks[i + 1 + PAINTED_WALL_VARIANT_COUNT].model = shaped;
        }
    }

    // The stile is one more shape of the plain stone. Without its model the wall under
    // it is drawn like any other.
    m_stileLook = plain;
    const assets::LoadedModel* stile = assets.model(core::assetPath(STILE_MODEL_FILE));
    if (stile != nullptr && !stile->parts.empty()) {
        m_stileLook.model = stile;
    }
}

void MazeRenderer::draw(const gfx::Shader& shader, const MazeWorld& world,
                        std::span<const glm::mat4> wallMatrices, bool wallVariants) const {
    setModelSamplers(shader);
    // Stone gives off no light of its own. A uniform keeps its value from one draw call
    // to the next, and the crystals set this one, so it is set back in every frame.
    shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{0.0F});

    // The walls: one draw call per wall, with the mesh and the textures of its look. The
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
            // The stile is not a look that can be switched off: its collision box stands
            // there whatever is drawn.
            const WallLook& look = world.stileWall == i
                                       ? m_stileLook
                                       : m_wallLooks.at(static_cast<std::size_t>(variant));
            drawMesh(shader, look.model->mesh, *look.color, *look.normalMap, tint, wallMatrices[i]);
        }
    }
    drawModel(shader, m_pillar, world.pillarMatrices);
    // The post of the stile stands where its wall stands.
    if (world.stileWall && *world.stileWall < wallMatrices.size()) {
        drawModel(shader, m_stilePost, wallMatrices.subspan(*world.stileWall, 1));
    }
    drawModel(shader, m_stoneSheep, world.sheepMatrices);
}

} // namespace game
