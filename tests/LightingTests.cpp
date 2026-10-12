// Tests of game::Lighting: the settings of the lighting and the lights built for one frame.
#include "game/Lighting.hpp"

#include "game/Player.hpp"
#include "gfx/ColorSpace.hpp"
#include "scene/Camera.hpp"
#include "scene/LightSpace.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// A flashlight for the tests that do not care about it: at the origin, pointing north,
// level.
constexpr game::FlashlightPose ANY_POSE{};

// The pose of the flashlight for a camera that stands at eye and looks along the two
// angles, the way the game asks for it.
game::FlashlightPose poseFor(const game::LightingSettings& settings, const glm::vec3& eye,
                             float yawDegrees, float pitchDegrees) {
    scene::Camera camera;
    camera.yawDegrees = yawDegrees;
    camera.pitchDegrees = pitchDegrees;
    return game::flashlightPose(settings, eye, camera.forward(), camera.right());
}

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

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, {});

    // The colours of the settings are sRGB values, the lights carry linear colours.
    checkVector(lights.ambient, gfx::srgbToLinear(glm::vec3{0.1F, 0.2F, 0.3F}));
    // Pitch -90: the light travels straight down.
    checkVector(lights.directional.direction, {0.0F, -1.0F, 0.0F});
    checkVector(lights.directional.color, gfx::srgbToLinear(glm::vec3{0.4F, 0.5F, 0.6F}));
    CHECK(lights.directional.intensity == 0.7F);
    CHECK(lights.pointCount == 0);
}

TEST_CASE("the flashlight sits in the hand and is aimed at a point in front of the eye") {
    game::LightingSettings settings;
    settings.flashlightRange = 10.0F;
    // The defaults: 0.2 m to the right, 0.25 m down, the beam meets the view 4 m ahead.
    CHECK(settings.flashlightHandRight == 0.2F);
    CHECK(settings.flashlightHandDown == 0.25F);
    CHECK(settings.flashlightConvergeDistance == 4.0F);

    // A camera that looks east (+X), level. Its right side is then south (+Z).
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    const glm::vec3 forward{1.0F, 0.0F, 0.0F};
    const glm::vec3 right{0.0F, 0.0F, 1.0F};

    const game::FlashlightPose pose = game::flashlightPose(settings, eye, forward, right);

    // The hand: 0.2 m south of the eye and 0.25 m below it.
    checkVector(pose.position, {3.0F, 1.45F, 5.2F});
    // The beam goes to the point 4 m east of the eye: 4 m east, 0.25 m up and 0.2 m
    // north of the hand.
    const glm::vec3 target{7.0F, 1.7F, 5.0F};
    checkVector(pose.direction, glm::normalize(target - pose.position));
    checkVector(pose.direction, glm::normalize(glm::vec3{4.0F, 0.25F, -0.2F}));

    // The spot light of the frame is built from that pose and nothing else.
    const scene::LightSet lights = game::buildLightSet(settings, pose, {});

    CHECK(lights.spotEnabled);
    checkVector(lights.spot.position, pose.position);
    checkVector(lights.spot.direction, pose.direction);
    checkVector(lights.spot.color, gfx::srgbToLinear(settings.flashlightColor));
    CHECK(lights.spot.intensity == settings.flashlightIntensity);
    CHECK(lights.spot.innerConeDegrees == settings.flashlightInnerDegrees);
    CHECK(lights.spot.outerConeDegrees == settings.flashlightOuterDegrees);
    // The range sets the attenuation: 5 % is left at 10 m.
    CHECK(scene::attenuationFactor(lights.spot.attenuation, 10.0F) ==
          doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
}

TEST_CASE("with both hand offsets at zero the flashlight is at the eye, as it used to be") {
    game::LightingSettings settings;
    settings.flashlightHandRight = 0.0F;
    settings.flashlightHandDown = 0.0F;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};

    // Whatever the camera looks at and wherever the beam is told to meet the view.
    for (const float pitch : {-89.0F, -30.0F, 0.0F, 45.0F, 89.0F}) {
        for (const float distance : {0.5F, 4.0F, 20.0F}) {
            settings.flashlightConvergeDistance = distance;
            scene::Camera camera;
            camera.yawDegrees = 140.0F;
            camera.pitchDegrees = pitch;

            const game::FlashlightPose pose =
                game::flashlightPose(settings, eye, camera.forward(), camera.right());

            checkVector(pose.position, eye);
            checkVector(pose.direction, camera.forward());
        }
    }
}

TEST_CASE("the beam of the flashlight passes through the point the view is aimed at") {
    const game::LightingSettings settings;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};

    for (const float yaw : {0.0F, 25.0F, 90.0F, 200.0F, 315.0F}) {
        for (const float pitch : {-89.0F, -40.0F, 0.0F, 40.0F, 89.0F}) {
            scene::Camera camera;
            camera.yawDegrees = yaw;
            camera.pitchDegrees = pitch;
            const game::FlashlightPose pose = poseFor(settings, eye, yaw, pitch);

            // The direction is a unit vector.
            CHECK(glm::length(pose.direction) == doctest::Approx(1.0F));

            // Walking from the hand along the beam, as far as the point is away, ends
            // on the line of view, 4 m in front of the eye.
            const glm::vec3 target = eye + camera.forward() * settings.flashlightConvergeDistance;
            const float distance = glm::length(target - pose.position);
            checkVector(pose.position + pose.direction * distance, target);
        }
    }
}

TEST_CASE("the hand stays inside the body of the player however the camera is turned") {
    // The largest offset to the right the panel allows leaves the near plane of the
    // shadow map between the light and the side of the body.
    const float halfBody = game::Player::BODY_WIDTH / 2.0F;
    CHECK(game::MAX_FLASHLIGHT_HAND_RIGHT == doctest::Approx(halfBody - scene::SPOT_NEAR_PLANE));
    CHECK(game::LightingSettings{}.flashlightHandRight <= game::MAX_FLASHLIGHT_HAND_RIGHT);

    game::LightingSettings settings;
    settings.flashlightHandRight = game::MAX_FLASHLIGHT_HAND_RIGHT;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};

    for (const float yaw : {0.0F, 30.0F, 45.0F, 90.0F, 135.0F, 200.0F, 315.0F}) {
        for (const float pitch : {-89.0F, -45.0F, 0.0F, 45.0F, 89.0F}) {
            const game::FlashlightPose pose = poseFor(settings, eye, yaw, pitch);
            const glm::vec3 offset = pose.position - eye;

            // The body is a box along the axes of the world, with the eye above its
            // middle. Along both level axes the hand is inside it, with room to spare.
            CHECK(std::abs(offset.x) <= halfBody - scene::SPOT_NEAR_PLANE + 0.0001F);
            CHECK(std::abs(offset.z) <= halfBody - scene::SPOT_NEAR_PLANE + 0.0001F);
            // Straight down by the offset of the settings, whatever the pitch is.
            CHECK(offset.y == doctest::Approx(-settings.flashlightHandDown));
        }
    }
}

TEST_CASE("the flashlight always has a direction") {
    game::LightingSettings settings;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};

    // A distance of 0 would aim the beam at the eye itself. The smallest distance is
    // used in its place.
    settings.flashlightConvergeDistance = 0.0F;
    const game::FlashlightPose atZero = poseFor(settings, eye, 0.0F, 0.0F);
    settings.flashlightConvergeDistance = game::MIN_FLASHLIGHT_CONVERGE_DISTANCE;
    const game::FlashlightPose smallest = poseFor(settings, eye, 0.0F, 0.0F);
    checkVector(atZero.direction, smallest.direction);
    CHECK(glm::length(atZero.direction) == doctest::Approx(1.0F));

    // The one case in which the hand IS the point the beam is aimed at: a camera that
    // looks straight down and a hand as far below the eye as that point. The beam then
    // points where the camera looks instead of having no direction (NaN).
    settings.flashlightHandRight = 0.0F;
    settings.flashlightHandDown = 1.0F;
    settings.flashlightConvergeDistance = 1.0F;
    const glm::vec3 straightDown{0.0F, -1.0F, 0.0F};
    const glm::vec3 right{1.0F, 0.0F, 0.0F};
    const game::FlashlightPose pose = game::flashlightPose(settings, eye, straightDown, right);
    checkVector(pose.direction, straightDown);
}

TEST_CASE("switching the flashlight off keeps its settings") {
    game::LightingSettings settings;
    settings.flashlightOn = false;

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, {});

    CHECK_FALSE(lights.spotEnabled);
    CHECK(lights.spot.intensity == settings.flashlightIntensity);
}

TEST_CASE("the inner cone of the flashlight is never wider than the outer cone") {
    game::LightingSettings settings;
    settings.flashlightInnerDegrees = 40.0F;
    settings.flashlightOuterDegrees = 15.0F;

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, {});

    CHECK(lights.spot.innerConeDegrees == 15.0F);
    CHECK(lights.spot.outerConeDegrees == 15.0F);
}

TEST_CASE("every point light gets the shared colour, intensity and radius") {
    game::LightingSettings settings;
    settings.pointColor = {0.1F, 0.8F, 0.9F};
    settings.pointIntensity = 1.5F;
    settings.pointRadius = 4.0F;
    const std::vector<game::PointLightSpot> spots = {{.position = {1.0F, 1.4F, 7.0F}},
                                                     {.position = {7.0F, 1.4F, 3.0F}}};

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, spots);

    REQUIRE(lights.pointCount == 2);
    for (std::size_t i = 0; i < spots.size(); ++i) {
        checkVector(lights.points[i].position, spots[i].position);
        checkVector(lights.points[i].color, gfx::srgbToLinear(settings.pointColor));
        CHECK(lights.points[i].intensity == 1.5F);
        CHECK(scene::attenuationFactor(lights.points[i].attenuation, 4.0F) ==
              doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
    }
}

TEST_CASE("buildLightSet ignores positions past the largest number of point lights") {
    const game::LightingSettings settings;
    const std::vector<game::PointLightSpot> spots(
        static_cast<std::size_t>(scene::MAX_POINT_LIGHTS) + 4,
        game::PointLightSpot{.position = {1.0F, 1.4F, 1.0F}});

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, spots);

    CHECK(lights.pointCount == scene::MAX_POINT_LIGHTS);
}

TEST_CASE("a point light is as bright as the settings say, times its strength") {
    game::LightingSettings settings;
    settings.pointIntensity = 0.8F;
    const std::vector<game::PointLightSpot> spots = {
        {.position = {1.0F, 1.4F, 1.0F}, .strength = 1.0F},
        {.position = {3.0F, 1.4F, 1.0F}, .strength = 0.25F},
        {.position = {5.0F, 1.4F, 1.0F}, .strength = 0.0F}};

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, spots);

    REQUIRE(lights.pointCount == 3);
    CHECK(lights.points[0].intensity == doctest::Approx(0.8F));
    CHECK(lights.points[1].intensity == doctest::Approx(0.2F));
    CHECK(lights.points[2].intensity == 0.0F);
}

TEST_CASE("with few lights every one is chosen at full strength, nearest first") {
    const std::vector<glm::vec3> positions = {
        {9.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 0.0F}, {5.0F, 1.0F, 0.0F}};
    const glm::vec3 eye{0.0F, 1.0F, 0.0F};

    const std::vector<game::PointLightSpot> chosen = game::nearestPointLights(positions, eye, 3);

    REQUIRE(chosen.size() == 3U);
    CHECK(chosen[0].position.x == 1.0F);
    CHECK(chosen[1].position.x == 5.0F);
    CHECK(chosen[2].position.x == 9.0F);
    for (const game::PointLightSpot& spot : chosen) {
        CHECK(spot.strength == 1.0F);
    }
    // No lights and no room both give an empty list.
    CHECK(game::nearestPointLights({}, eye, 3).empty());
    CHECK(game::nearestPointLights(positions, eye, 0).empty());
}

TEST_CASE("with too many lights the nearest ones are chosen and fade towards the edge") {
    // Lights on a line, 1 m, 2 m, 3 m ... 10 m from the eye, given in the wrong order.
    std::vector<glm::vec3> positions;
    for (int metres = 10; metres >= 1; --metres) {
        positions.emplace_back(static_cast<float>(metres), 0.0F, 0.0F);
    }
    const glm::vec3 eye{0.0F};

    // Room for 6: the lights at 1 m to 6 m. The first one left out is 7 m away, which
    // is the edge of the set.
    const std::vector<game::PointLightSpot> chosen = game::nearestPointLights(positions, eye, 6);

    REQUIRE(chosen.size() == 6U);
    CHECK(game::POINT_LIGHT_FADE_DISTANCE == 4.0F);
    for (std::size_t i = 0; i < chosen.size(); ++i) {
        CHECK(chosen[i].position.x == static_cast<float>(i + 1));
    }
    // 6 m, 5 m and 4 m inside the edge: full strength. Then 3, 2 and 1 m inside: three
    // quarters, a half and a quarter.
    CHECK(chosen[0].strength == 1.0F);
    CHECK(chosen[1].strength == 1.0F);
    CHECK(chosen[2].strength == 1.0F);
    CHECK(chosen[3].strength == doctest::Approx(0.75F));
    CHECK(chosen[4].strength == doctest::Approx(0.5F));
    CHECK(chosen[5].strength == doctest::Approx(0.25F));
}

TEST_CASE("two lights that change places at the edge of the set are both dark") {
    // Room for one light, and two lights almost equally far from the eye: whichever is
    // chosen has almost no strength, so the change from one to the other shows no jump.
    const std::vector<glm::vec3> positions = {{5.0F, 0.0F, 0.0F}, {-5.01F, 0.0F, 0.0F}};
    const glm::vec3 eye{0.0F};

    const std::vector<game::PointLightSpot> before = game::nearestPointLights(positions, eye, 1);
    REQUIRE(before.size() == 1U);
    CHECK(before[0].position.x == 5.0F);
    CHECK(before[0].strength < 0.01F);

    // The eye moves a little towards the other light: now that one is chosen, as dark.
    const glm::vec3 movedEye{-0.01F, 0.0F, 0.0F};
    const std::vector<game::PointLightSpot> after =
        game::nearestPointLights(positions, movedEye, 1);
    REQUIRE(after.size() == 1U);
    CHECK(after[0].position.x == -5.01F);
    CHECK(after[0].strength < 0.01F);
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
    const std::vector<game::PointLightSpot> spots = {{.position = {1.0F, 1.4F, 7.0F}}};

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, spots);

    checkVector(lights.ambient, glm::vec3{0.21404F});
    checkVector(lights.directional.color, glm::vec3{0.21404F});
    // An intensity is a factor, not a colour: it is not converted.
    CHECK(lights.directional.intensity == 0.5F);
    checkVector(lights.spot.color, glm::vec3{1.0F});
    REQUIRE(lights.pointCount == 1);
    checkVector(lights.points[0].color, glm::vec3{0.0F});
}

TEST_CASE("a spot with a look of its own keeps its colour, radius and intensity") {
    game::LightingSettings settings;
    settings.pointColor = {0.1F, 0.8F, 0.9F};
    settings.pointIntensity = 1.5F;
    settings.pointRadius = 4.0F;
    // A crystal light, and the lamp of the gate at half of its strength.
    const glm::vec3 amber{1.0F, 0.72F, 0.33F};
    const std::vector<game::PointLightSpot> spots = {{.position = {1.0F, 1.4F, 7.0F}},
                                                     {.position = {7.0F, 2.5F, 3.0F},
                                                      .strength = 0.5F,
                                                      .ownLook = true,
                                                      .color = amber,
                                                      .radius = 6.0F,
                                                      .intensity = 2.0F}};

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, spots);

    REQUIRE(lights.pointCount == 2);
    // The crystal light is exactly what it was before spots could have a look.
    checkVector(lights.points[0].color, gfx::srgbToLinear(settings.pointColor));
    CHECK(lights.points[0].intensity == 1.5F);
    CHECK(scene::attenuationFactor(lights.points[0].attenuation, 4.0F) ==
          doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
    // The lamp: its own colour (converted like every colour), radius and intensity.
    checkVector(lights.points[1].position, spots[1].position);
    checkVector(lights.points[1].color, gfx::srgbToLinear(amber));
    CHECK(lights.points[1].intensity == doctest::Approx(1.0F));
    CHECK(scene::attenuationFactor(lights.points[1].attenuation, 6.0F) ==
          doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
}

TEST_CASE("the pulse of the crystal lights does not reach a spot with its own look") {
    // lightingForFrame dims pointIntensity with the pulse of the crystals. A lamp is not
    // a crystal: whatever that number is, its light stays.
    game::LightingSettings settings;
    const std::vector<game::PointLightSpot> spots = {
        {.position = {1.0F, 2.5F, 1.0F}, .ownLook = true, .radius = 6.0F, .intensity = 1.6F}};

    settings.pointIntensity = 0.9F;
    const float bright = game::buildLightSet(settings, ANY_POSE, spots).points[0].intensity;
    settings.pointIntensity = 0.1F;
    const float dim = game::buildLightSet(settings, ANY_POSE, spots).points[0].intensity;

    CHECK(bright == 1.6F);
    CHECK(dim == 1.6F);
}

TEST_CASE("a spot keeps its look and its own strength when the nearest ones are chosen") {
    const glm::vec3 eye{0.0F, 1.7F, 0.0F};
    // Three crystal lights and the lamp of the gate as the second nearest, burning at
    // 0.3 of its strength.
    const std::vector<game::PointLightSpot> lights = {
        {.position = {2.0F, 1.7F, 0.0F}},
        {.position = {20.0F, 1.7F, 0.0F}},
        {.position = {4.0F, 1.7F, 0.0F},
         .strength = 0.3F,
         .ownLook = true,
         .color = {1.0F, 0.72F, 0.33F},
         .radius = 6.0F,
         .intensity = 1.6F},
        {.position = {30.0F, 1.7F, 0.0F}},
    };

    // Room for all: every spot comes back as it went in, nearest first.
    std::vector<game::PointLightSpot> chosen = game::nearestPointLightSpots(lights, eye, 8);
    REQUIRE(chosen.size() == 4);
    CHECK(chosen[1].ownLook);
    CHECK(chosen[1].strength == 0.3F);
    CHECK(chosen[1].radius == 6.0F);
    CHECK(chosen[1].intensity == 1.6F);
    CHECK(chosen[1].color == glm::vec3{1.0F, 0.72F, 0.33F});
    CHECK_FALSE(chosen[0].ownLook);
    CHECK(chosen[0].strength == 1.0F);

    // Room for two: the lamp is 16 m inside the edge (the light at 20 m is left out),
    // so the fade is 1 and its own 0.3 is what is left.
    chosen = game::nearestPointLightSpots(lights, eye, 2);
    REQUIRE(chosen.size() == 2);
    CHECK(chosen[1].ownLook);
    CHECK(chosen[1].strength == doctest::Approx(0.3F));

    // Room for one: the lamp is the first one left out. It takes no place at all.
    chosen = game::nearestPointLightSpots(lights, eye, 1);
    REQUIRE(chosen.size() == 1);
    CHECK_FALSE(chosen[0].ownLook);
}

TEST_CASE("the lamp of the gate fades out of the set like a crystal light, from its own strength") {
    const glm::vec3 eye{0.0F, 1.7F, 0.0F};
    // The lamp 8 m away at strength 0.5, and one crystal light just behind it at 10 m:
    // with room for one, the edge is at 10 m and the lamp is half way through the fade.
    const std::vector<game::PointLightSpot> lights = {
        {.position = {8.0F, 1.7F, 0.0F}, .strength = 0.5F, .ownLook = true, .radius = 6.0F},
        {.position = {10.0F, 1.7F, 0.0F}},
    };

    const std::vector<game::PointLightSpot> chosen = game::nearestPointLightSpots(lights, eye, 1);

    REQUIRE(chosen.size() == 1);
    CHECK(chosen[0].ownLook);
    CHECK(chosen[0].strength == doctest::Approx(0.5F * 2.0F / game::POINT_LIGHT_FADE_DISTANCE));
}

TEST_CASE("choosing among positions and among plain spots gives the same lights") {
    const glm::vec3 eye{1.0F, 1.7F, 2.0F};
    std::vector<glm::vec3> positions;
    std::vector<game::PointLightSpot> spots;
    for (int i = 0; i < 24; ++i) {
        const glm::vec3 position{static_cast<float>(i * 7 % 23), 1.4F,
                                 static_cast<float>(i * 5 % 19)};
        positions.push_back(position);
        spots.push_back({.position = position});
    }

    const std::vector<game::PointLightSpot> fromPositions =
        game::nearestPointLights(positions, eye);
    const std::vector<game::PointLightSpot> fromSpots = game::nearestPointLightSpots(spots, eye);

    REQUIRE(fromPositions.size() == fromSpots.size());
    for (std::size_t i = 0; i < fromPositions.size(); ++i) {
        CHECK(fromPositions[i].position == fromSpots[i].position);
        CHECK(fromPositions[i].strength == fromSpots[i].strength);
        CHECK_FALSE(fromPositions[i].ownLook);
    }
}

TEST_CASE("the held lamp lies on the axis of the beam, with its window on top") {
    game::LightingSettings settings;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    // The lens of the model: on its axis, which is -Z.
    const glm::vec4 lens{0.0F, 0.0F, -game::HELD_LAMP_LENS_OUT, 1.0F};

    for (const float pitch : {-89.0F, -30.0F, 0.0F, 45.0F, 89.0F}) {
        for (const float handRight : {0.0F, 0.2F, game::MAX_FLASHLIGHT_HAND_RIGHT}) {
            for (const float converge : {game::MIN_FLASHLIGHT_CONVERGE_DISTANCE, 4.0F, 20.0F}) {
                CAPTURE(pitch);
                CAPTURE(handRight);
                CAPTURE(converge);
                settings.flashlightHandRight = handRight;
                settings.flashlightConvergeDistance = converge;
                scene::Camera camera;
                camera.yawDegrees = 140.0F;
                camera.pitchDegrees = pitch;
                const glm::vec3 forward = camera.forward();
                const glm::vec3 right = camera.right();

                const glm::mat4 matrix = game::heldLampMatrix(settings, eye, forward, right);
                const glm::vec3 butt{matrix[3]};
                const glm::vec3 along = glm::mat3{matrix} * glm::vec3{0.0F, 0.0F, -1.0F};

                // The lamp aims at the point the beam aims at, whatever the pitch is:
                // from its butt, the way to that point is the way its lens looks.
                const glm::vec3 target = eye + forward * converge;
                checkVector(glm::normalize(target - butt), along);

                // With a level look it lies on the beam itself: the butt is
                // HELD_LAMP_REACH from the hand of the light and the lens further along
                // the same line, so the beam leaves the lamp through its lens.
                if (pitch == 0.0F) {
                    const game::FlashlightPose pose =
                        game::flashlightPose(settings, eye, forward, right);
                    checkVector(butt, pose.position + pose.direction * game::HELD_LAMP_REACH);
                    checkVector(glm::vec3{matrix * lens},
                                pose.position + pose.direction * (game::HELD_LAMP_REACH +
                                                                  game::HELD_LAMP_LENS_OUT));
                    checkVector(along, pose.direction);
                }

                // A turn without a scale or a mirror: three axes of length 1 at right
                // angles, the third the cross product of the first two.
                const glm::vec3 side{matrix[0]};
                const glm::vec3 top{matrix[1]};
                CHECK(glm::length(side) == doctest::Approx(1.0F));
                CHECK(glm::length(top) == doctest::Approx(1.0F));
                CHECK(glm::dot(side, top) == doctest::Approx(0.0F));
                checkVector(glm::cross(side, top), glm::vec3{matrix[2]});
                // The window does not roll: the top of the lamp has no part along the
                // right vector of the camera, and it points to the upper half of the
                // picture.
                CHECK(glm::dot(top, right) == doctest::Approx(0.0F));
                CHECK(glm::dot(top, glm::cross(right, forward)) > 0.0F);
            }
        }
    }
}

TEST_CASE("the held lamp keeps its place in the lower right of the picture") {
    const game::LightingSettings settings;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    constexpr float ASPECT_RATIO = 16.0F / 9.0F;
    const glm::vec4 lens{0.0F, 0.0F, -game::HELD_LAMP_LENS_OUT, 1.0F};
    // The end of the window that is nearer to the player (tools/blender/build_lamp_held.py).
    const glm::vec4 window{0.0F, 0.0182F, -0.082F, 1.0F};

    // Looking down, level and up, with the narrowest, the default and the widest field
    // of view of the settings.
    glm::vec2 first{0.0F};
    bool hasFirst = false;
    for (const float pitch : {-60.0F, 0.0F, 60.0F}) {
        for (const float fov : {45.0F, game::HELD_LAMP_FOV_DEGREES, 90.0F}) {
            CAPTURE(pitch);
            CAPTURE(fov);
            scene::Camera camera;
            camera.yawDegrees = 220.0F;
            camera.pitchDegrees = pitch;
            camera.fovDegrees = fov;
            const glm::mat4 toClip =
                game::heldLampProjection(camera.projectionMatrix(ASPECT_RATIO)) *
                camera.viewMatrix(eye) *
                game::heldLampMatrix(settings, eye, camera.forward(), camera.right());

            // Where the lens is in the picture: -1 to 1 from the left to the right edge
            // and from the lower to the upper one. In front of the near plane.
            const glm::vec4 clip = toClip * lens;
            REQUIRE(clip.w > camera.nearPlane);
            const glm::vec2 place = glm::vec2{clip} / clip.w;

            // Right of the middle and in the lowest third, with the head inside the
            // picture.
            CHECK(place.x > 0.2F);
            CHECK(place.x < 0.8F);
            CHECK(place.y < -0.5F);
            CHECK(place.y > -0.9F);
            // The window is in the picture too.
            const glm::vec4 windowClip = toClip * window;
            CHECK(windowClip.y / windowClip.w > -1.0F);
            CHECK(windowClip.x / windowClip.w < 1.0F);

            // The same place wherever the camera looks and whatever the player has set.
            if (!hasFirst) {
                first = place;
                hasFirst = true;
            }
            CHECK(place.x == doctest::Approx(first.x));
            CHECK(place.y == doctest::Approx(first.y));
        }
    }

    // With the default field of view the projection of the frame is left as it is.
    scene::Camera camera;
    camera.fovDegrees = game::HELD_LAMP_FOV_DEGREES;
    const glm::mat4 scene = camera.projectionMatrix(ASPECT_RATIO);
    const glm::mat4 lamp = game::heldLampProjection(scene);
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            CHECK(lamp[column][row] == doctest::Approx(scene[column][row]));
        }
    }
}

TEST_CASE("the lens of the held lamp glows with the beam and its window with the battery") {
    game::LightingSettings settings;
    const float full = 1.0F;
    const float steady = 1.0F;

    // On: the colour of the beam, brighter than the light itself.
    const game::HeldLampGlow on = game::heldLampGlow(settings, full, steady);
    checkVector(on.lens, gfx::srgbToLinear(settings.flashlightColor) *
                             (settings.flashlightIntensity * game::HELD_LAMP_LENS_GLOW));
    // The window has the colour of the crystals.
    checkVector(on.window, gfx::srgbToLinear(settings.pointColor) * game::HELD_LAMP_WINDOW_GLOW);

    // A dimmed beam (the flicker of a low battery is in the intensity) dims the lens.
    game::LightingSettings dimmed = settings;
    dimmed.flashlightIntensity = settings.flashlightIntensity / 2.0F;
    checkVector(game::heldLampGlow(dimmed, full, steady).lens, on.lens / 2.0F);

    // Off: a dark lens. The window still shows the charge.
    settings.flashlightOn = false;
    const game::HeldLampGlow off = game::heldLampGlow(settings, full, steady);
    checkVector(off.lens, glm::vec3{0.0F});
    checkVector(off.window, on.window);

    // Half a charge is half the glow, the flicker takes its share, and an empty battery
    // leaves the window dark. A charge outside of 0 to 1 is taken as the nearer end.
    checkVector(game::heldLampGlow(settings, 0.5F, steady).window, on.window / 2.0F);
    checkVector(game::heldLampGlow(settings, 0.5F, 0.5F).window, on.window / 4.0F);
    checkVector(game::heldLampGlow(settings, 0.0F, steady).window, glm::vec3{0.0F});
    checkVector(game::heldLampGlow(settings, 2.0F, steady).window, on.window);
    checkVector(game::heldLampGlow(settings, -1.0F, steady).window, glm::vec3{0.0F});
}

TEST_CASE("the held lamp is drawn without the flashlight and with a spill on its head") {
    game::LightingSettings settings;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    scene::Camera camera;
    camera.yawDegrees = 140.0F;
    const glm::mat4 matrix = game::heldLampMatrix(settings, eye, camera.forward(), camera.right());

    // The lights of a frame with two crystals.
    const std::vector<game::PointLightSpot> crystals{{.position = {1.0F, 1.0F, 1.0F}},
                                                     {.position = {4.0F, 1.0F, 2.0F}}};
    const scene::LightSet frame = game::buildLightSet(settings, ANY_POSE, crystals);
    REQUIRE(frame.spotEnabled);
    REQUIRE(frame.pointCount == 2);

    // On: the beam is taken out and one point light is added behind the crystals, in the
    // colour of the beam and a part of its intensity.
    const scene::LightSet on = game::heldLampLights(frame, settings, matrix);
    CHECK_FALSE(on.spotEnabled);
    REQUIRE(on.pointCount == 3);
    checkVector(on.points[0].position, frame.points[0].position);
    checkVector(on.points[1].position, frame.points[1].position);
    const scene::PointLight& spill = on.points[2];
    checkVector(spill.color, gfx::srgbToLinear(settings.flashlightColor));
    CHECK(spill.intensity == doctest::Approx(settings.flashlightIntensity * game::HELD_LAMP_SPILL));
    // Weaker than the beam, and nearly gone at the window, a hand further back.
    CHECK(spill.intensity < settings.flashlightIntensity);
    CHECK(scene::attenuationFactor(spill.attenuation, game::HELD_LAMP_SPILL_RADIUS) ==
          doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));

    // It stands at the head: outside the tin (tools/blender/build_lamp_held.py, the head
    // is 0.0335 m wide), above the axis, on the side of the eye and within a few
    // centimetres of the lens. Behind the lens, so the lens is not lit by it.
    const glm::vec3 place = game::HELD_LAMP_SPILL_PLACE;
    CHECK(glm::length(glm::vec2{place}) > 0.0335F);
    CHECK(place.x < 0.0F);
    CHECK(place.y > 0.0F);
    CHECK(place.z > -game::HELD_LAMP_LENS_OUT - 0.01F);
    CHECK(place.z < -game::HELD_LAMP_LENS_OUT + 0.01F);
    checkVector(spill.position, glm::vec3{matrix * glm::vec4{place, 1.0F}});
    const glm::vec3 lens{matrix * glm::vec4{0.0F, 0.0F, -game::HELD_LAMP_LENS_OUT, 1.0F}};
    CHECK(glm::distance(spill.position, eye) < glm::distance(lens, eye));

    // The flicker of a low battery is in the intensity of the frame, so the spill
    // flickers with the beam.
    game::LightingSettings dimmed = settings;
    dimmed.flashlightIntensity = settings.flashlightIntensity / 2.0F;
    CHECK(game::heldLampLights(frame, dimmed, matrix).points[2].intensity ==
          doctest::Approx(spill.intensity / 2.0F));

    // Off or empty (game::lightingForFrame switches an empty lamp off): no spill, and
    // still no beam.
    settings.flashlightOn = false;
    const scene::LightSet off = game::heldLampLights(frame, settings, matrix);
    CHECK_FALSE(off.spotEnabled);
    CHECK(off.pointCount == 2);
    settings.flashlightOn = true;

    // A full list: the spill takes the place of the last light, the farthest one.
    scene::LightSet full = frame;
    full.pointCount = scene::MAX_POINT_LIGHTS;
    const scene::LightSet crowded = game::heldLampLights(full, settings, matrix);
    CHECK(crowded.pointCount == scene::MAX_POINT_LIGHTS);
    checkVector(crowded.points[scene::MAX_POINT_LIGHTS - 1].position, spill.position);
    checkVector(crowded.points[0].position, frame.points[0].position);
}
