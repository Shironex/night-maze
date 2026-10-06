// Tests of game::Lighting: the settings of the lighting and the lights built for one frame.
// See docs/modules/game/flashlight.md
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
    const std::vector<glm::vec3> positions = {{1.0F, 1.4F, 7.0F}, {7.0F, 1.4F, 3.0F}};

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, positions);

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

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, positions);

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

    const scene::LightSet lights = game::buildLightSet(settings, ANY_POSE, positions);

    checkVector(lights.ambient, glm::vec3{0.21404F});
    checkVector(lights.directional.color, glm::vec3{0.21404F});
    // An intensity is a factor, not a colour: it is not converted.
    CHECK(lights.directional.intensity == 0.5F);
    checkVector(lights.spot.color, glm::vec3{1.0F});
    REQUIRE(lights.pointCount == 1);
    checkVector(lights.points[0].color, glm::vec3{0.0F});
}
