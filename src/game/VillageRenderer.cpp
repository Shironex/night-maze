// VillageRenderer: draws the village on the ridge and the lights of its won nights.
#include "game/VillageRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "game/GateLamp.hpp"
#include "game/ModelDraw.hpp"
#include "game/ShaderUniforms.hpp"
#include "game/Village.hpp"
#include "gfx/ColorSpace.hpp"
#include "gfx/Shader.hpp"

#include <algorithm>
#include <string>

namespace game {

namespace {

// The models, relative to the assets folder. They are written by
// tools/blender/build_village.py. The lights are one file for each night:
// village_lights_1.obj to village_lights_5.obj.
constexpr const char* VILLAGE_MODEL_FILE = "models/village.obj";
constexpr const char* LIGHTS_MODEL_PREFIX = "models/village_lights_";
constexpr const char* MODEL_SUFFIX = ".obj";

// The colour the village is multiplied by, a linear colour. The textured program knows
// no light, and the model has two tones of its own (tools/blender/build_village.py): pale
// walls, and dark roofs and ridge. With this tint the walls are stone under the moon,
// a little lighter than the sky behind them, the roofs and the ridge are black shapes,
// and the lights are what is seen first.
constexpr glm::vec3 VILLAGE_TINT{0.03F, 0.04F, 0.065F};

constexpr glm::vec3 NO_GLOW{0.0F};

// Draws every part of a model once with the given tint. drawModel would take the tint
// from the material file, which is white.
void drawTinted(const gfx::Shader& shader, const assets::LoadedModel* model, const glm::vec3& tint,
                const glm::mat4& matrix) {
    if (model == nullptr) {
        return;
    }
    for (const assets::ModelPart& part : model->parts) {
        drawMesh(shader, model->mesh, *part.texture, *part.normalMap, tint, matrix);
    }
}

} // namespace

VillageRenderer::VillageRenderer(assets::AssetCache& assets)
    : m_village(assets.model(core::assetPath(VILLAGE_MODEL_FILE))) {
    for (std::size_t night = 0; night < m_lights.size(); ++night) {
        m_lights.at(night) = assets.model(
            core::assetPath(LIGHTS_MODEL_PREFIX + std::to_string(night + 1) + MODEL_SUFFIX));
    }
}

void VillageRenderer::draw(const gfx::Shader& shader, const glm::mat4& matrix, int nightsLit,
                           float newestStrength) const {
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
    drawTinted(shader, m_village, VILLAGE_TINT, matrix);

    // The lights are amber, the colour of the open gate: the one warm colour of the
    // night. textured.frag shows tint * (1 + glow), so a light that is coming on is dimmed
    // by its tint too, and at strength 0 it is black.
    const glm::vec3 amber = gfx::srgbToLinear(GATE_LAMP_WARM_COLOR);
    const int lit = std::clamp(nightsLit, 0, CAMPAIGN_NIGHT_COUNT);
    for (int night = 1; night <= lit; ++night) {
        const float strength = night == lit ? std::clamp(newestStrength, 0.0F, 1.0F) : 1.0F;
        const float glow =
            VILLAGE_LIGHT_GLOW * (night == CAMPAIGN_NIGHT_COUNT ? VILLAGE_OWN_WINDOW_GLOW : 1.0F);
        shader.setVec3(EMISSIVE_UNIFORM, glm::vec3{glow});
        drawTinted(shader, m_lights.at(static_cast<std::size_t>(night - 1)), amber * strength,
                   matrix);
    }
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
}

} // namespace game
