// Tests of game::Lighting: the settings of the lighting and the lights built for one frame.
// See docs/modules/game/flashlight.md
#include "game/Lighting.hpp"

#include "gfx/ColorSpace.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// A view direction for the tests that do not care about it: north, level.
constexpr glm::vec3 LOOK_NORTH{0.0F, 0.0F, -1.0F};

} // namespace

TEST_CASE("the lighting starts as a night scene shaded with Blinn-Phong") {
    const game::LightingSettings settings;
    CHECK(settings.mode == game::LightingMode::BlinnPhong);
    CHECK(settings.flashlightOn);
    CHECK(settings.flashlightInnerDegrees < settings.flashlightOuterDegrees);
    CHECK(settings.pointRadius == 3.0F);
    // The moon shines downwards.
    CHECK(scene::directionFromAngles(settings.moonYawDegrees, settings.moonPitchDegrees).y < 0.0F);
}

TEST_CASE("the numbers of the lighting modes are the entries of the list in the panel") {
    CHECK(static_cast<int>(game::LightingMode::Unlit) == 0);
    CHECK(static_cast<int>(game::LightingMode::Gouraud) == 1);
    CHECK(static_cast<int>(game::LightingMode::Phong) == 2);
    CHECK(static_cast<int>(game::LightingMode::BlinnPhong) == 3);
}

TEST_CASE("Gouraud and Phong use the Phong highlight, Blinn-Phong its own") {
    CHECK(game::specularModelOf(game::LightingMode::Gouraud) == game::SpecularModel::Phong);
    CHECK(game::specularModelOf(game::LightingMode::Phong) == game::SpecularModel::Phong);
    CHECK(game::specularModelOf(game::LightingMode::BlinnPhong) == game::SpecularModel::BlinnPhong);
    // The numbers the shader compares uSpecularModel with.
    CHECK(static_cast<int>(game::SpecularModel::Phong) == 0);
    CHECK(static_cast<int>(game::SpecularModel::BlinnPhong) == 1);
}

TEST_CASE("normal mapping is on by default and applies to every mode except Gouraud") {
    game::LightingSettings settings;
    CHECK(settings.normalMapping);

    // Phong and Blinn-Phong light per fragment: they can read a normal per texel.
    settings.mode = game::LightingMode::Phong;
    CHECK(game::usesNormalMap(settings));
    settings.mode = game::LightingMode::BlinnPhong;
    CHECK(game::usesNormalMap(settings));
    // Unlit has no lighting, but its view of the normals shows the normal maps.
    settings.mode = game::LightingMode::Unlit;
    CHECK(game::usesNormalMap(settings));
    // Gouraud lights per vertex: a normal map cannot take part.
    settings.mode = game::LightingMode::Gouraud;
    CHECK_FALSE(game::usesNormalMap(settings));

    // Switched off, it applies nowhere.
    settings.normalMapping = false;
    for (const game::LightingMode mode :
         {game::LightingMode::Unlit, game::LightingMode::Gouraud, game::LightingMode::Phong,
          game::LightingMode::BlinnPhong}) {
        settings.mode = mode;
        CHECK_FALSE(game::usesNormalMap(settings));
    }
}

TEST_CASE("buildLightSet takes the ambient light and the moon from the settings") {
    game::LightingSettings settings;
    settings.ambient = {0.1F, 0.2F, 0.3F};
    settings.moonYawDegrees = 90.0F;
    settings.moonPitchDegrees = -90.0F;
    settings.moonColor = {0.4F, 0.5F, 0.6F};
    settings.moonIntensity = 0.7F;

    const scene::LightSet lights = game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, {});

    // The colours of the settings are sRGB values, the lights carry linear colours.
    checkVector(lights.ambient, gfx::srgbToLinear(glm::vec3{0.1F, 0.2F, 0.3F}));
    // Pitch -90: the light travels straight down.
    checkVector(lights.directional.direction, {0.0F, -1.0F, 0.0F});
    checkVector(lights.directional.color, gfx::srgbToLinear(glm::vec3{0.4F, 0.5F, 0.6F}));
    CHECK(lights.directional.intensity == 0.7F);
    CHECK(lights.pointCount == 0);
}

TEST_CASE("the flashlight sits at the eye and points where the camera looks") {
    game::LightingSettings settings;
    settings.flashlightRange = 10.0F;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    const glm::vec3 viewDirection{1.0F, 0.0F, 0.0F};

    const scene::LightSet lights = game::buildLightSet(settings, eye, viewDirection, {});

    CHECK(lights.spotEnabled);
    checkVector(lights.spot.position, eye);
    checkVector(lights.spot.direction, viewDirection);
    checkVector(lights.spot.color, gfx::srgbToLinear(settings.flashlightColor));
    CHECK(lights.spot.intensity == settings.flashlightIntensity);
    CHECK(lights.spot.innerConeDegrees == settings.flashlightInnerDegrees);
    CHECK(lights.spot.outerConeDegrees == settings.flashlightOuterDegrees);
    // The range sets the attenuation: 5 % is left at 10 m.
    CHECK(scene::attenuationFactor(lights.spot.attenuation, 10.0F) ==
          doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
}

TEST_CASE("switching the flashlight off keeps its settings") {
    game::LightingSettings settings;
    settings.flashlightOn = false;

    const scene::LightSet lights = game::buildLightSet(settings, glm::vec3{1.0F}, LOOK_NORTH, {});

    CHECK_FALSE(lights.spotEnabled);
    CHECK(lights.spot.intensity == settings.flashlightIntensity);
}

TEST_CASE("the inner cone of the flashlight is never wider than the outer cone") {
    game::LightingSettings settings;
    settings.flashlightInnerDegrees = 40.0F;
    settings.flashlightOuterDegrees = 15.0F;

    const scene::LightSet lights = game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, {});

    CHECK(lights.spot.innerConeDegrees == 15.0F);
    CHECK(lights.spot.outerConeDegrees == 15.0F);
}

TEST_CASE("every point light gets the shared colour, intensity and radius") {
    game::LightingSettings settings;
    settings.pointColor = {0.1F, 0.8F, 0.9F};
    settings.pointIntensity = 1.5F;
    settings.pointRadius = 4.0F;
    const std::vector<glm::vec3> positions = {{1.0F, 1.4F, 7.0F}, {7.0F, 1.4F, 3.0F}};

    const scene::LightSet lights =
        game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, positions);

    REQUIRE(lights.pointCount == 2);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        checkVector(lights.points[i].position, positions[i]);
        checkVector(lights.points[i].color, gfx::srgbToLinear(settings.pointColor));
        CHECK(lights.points[i].intensity == 1.5F);
        CHECK(scene::attenuationFactor(lights.points[i].attenuation, 4.0F) ==
              doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
    }
}

TEST_CASE("buildLightSet ignores positions past the largest number of point lights") {
    const game::LightingSettings settings;
    const std::vector<glm::vec3> positions(static_cast<std::size_t>(scene::MAX_POINT_LIGHTS) + 4,
                                           glm::vec3{1.0F, 1.4F, 1.0F});

    const scene::LightSet lights =
        game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, positions);

    CHECK(lights.pointCount == scene::MAX_POINT_LIGHTS);
}

TEST_CASE("buildLightSet converts the colours from sRGB to linear and leaves the rest") {
    game::LightingSettings settings;
    // Middle grey as a colour picker shows it: about a fifth of the light of white.
    settings.ambient = glm::vec3{0.5F};
    settings.moonColor = glm::vec3{0.5F};
    settings.moonIntensity = 0.5F;
    // White and black are the same numbers in both colour spaces.
    settings.flashlightColor = glm::vec3{1.0F};
    settings.pointColor = glm::vec3{0.0F};
    const std::vector<glm::vec3> positions = {{1.0F, 1.4F, 7.0F}};

    const scene::LightSet lights =
        game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, positions);

    checkVector(lights.ambient, glm::vec3{0.21404F});
    checkVector(lights.directional.color, glm::vec3{0.21404F});
    // An intensity is a factor, not a colour: it is not converted.
    CHECK(lights.directional.intensity == 0.5F);
    checkVector(lights.spot.color, glm::vec3{1.0F});
    REQUIRE(lights.pointCount == 1);
    checkVector(lights.points[0].color, glm::vec3{0.0F});
}
