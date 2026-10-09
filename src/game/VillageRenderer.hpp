// VillageRenderer: draws the village on the ridge and the lights of its won nights.
#pragma once

#include "game/Campaign.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>

namespace assets {
class AssetCache;
struct LoadedModel;
} // namespace assets

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

/// Draws the village (the model village) as a dark shape against the sky, and over it one
/// model of light for every night that is won (village_lights_1 to village_lights_5).
///
/// It owns nothing: the models belong to the asset cache, which must outlive this object.
/// Where the village stands is game::villageMatrix, and how many nights are lit is
/// game::villageNightsLit.
class VillageRenderer {
public:
    /// Asks the cache for the six models. A model that fails to load is logged by the
    /// cache and simply not drawn.
    explicit VillageRenderer(assets::AssetCache& assets);

    /// Draws the village and the lights of the first nightsLit nights: one draw call, and
    /// one more for every lit night.
    ///
    /// shader is the textured program (textured.frag): in use, with its samplers set
    /// (setModelSamplers), uProjection and uViewMode set, and uView set to the view
    /// matrix WITHOUT its translation, so the village is centred on the eye like the sky.
    /// The depth test must be on and the depth must be written: the village has real
    /// depth, so what stands nearer hides it, and the sky, drawn later, does not.
    ///
    /// matrix is game::villageMatrix. newestStrength, 0 to 1, is how much of the lights of
    /// the last lit night is there (game::villageNewLightStrength): the nights before it
    /// are all there. The function sets uEmissive and leaves it black.
    void draw(const gfx::Shader& shader, const glm::mat4& matrix, int nightsLit,
              float newestStrength) const;

private:
    // Not owned. nullptr when the model could not be loaded.
    const assets::LoadedModel* m_village;
    std::array<const assets::LoadedModel*, static_cast<std::size_t>(CAMPAIGN_NIGHT_COUNT)>
        m_lights{};
};

} // namespace game
