// Tests of scene::Light: attenuation, the cone of a spot light, the light block bytes.
// See docs/modules/scene/lights.md
#include "scene/Light.hpp"

#include "scene/LightBlock.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

void checkVector(const glm::vec4& actual, const glm::vec4& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
    CHECK(actual.w == doctest::Approx(expected.w));
}

} // namespace

TEST_CASE("the default attenuation leaves a light as it is at every distance") {
    const scene::Attenuation attenuation;
    CHECK(scene::attenuationFactor(attenuation, 0.0F) == 1.0F);
    CHECK(scene::attenuationFactor(attenuation, 100.0F) == 1.0F);
}

TEST_CASE("attenuationFactor is one divided by constant plus linear plus quadratic part") {
    const scene::Attenuation attenuation{.constant = 1.0F, .linear = 0.5F, .quadratic = 0.25F};
    // 1 / (1 + 0.5 * 2 + 0.25 * 2 * 2) = 1 / 3
    CHECK(scene::attenuationFactor(attenuation, 2.0F) == doctest::Approx(1.0F / 3.0F));
}

TEST_CASE("attenuationForRadius gives the documented terms") {
    const scene::Attenuation attenuation = scene::attenuationForRadius(3.0F);
    CHECK(attenuation.constant == 1.0F);
    CHECK(attenuation.linear == doctest::Approx(2.0F / 3.0F));
    CHECK(attenuation.quadratic == doctest::Approx(17.0F / 9.0F));
}

TEST_CASE("a light made for a radius has 5 % of its brightness left at that radius") {
    CHECK(scene::BRIGHTNESS_AT_RADIUS == 0.05F);
    for (const float radius : {0.5F, 3.0F, 16.0F, 60.0F}) {
        const scene::Attenuation attenuation = scene::attenuationForRadius(radius);
        CHECK(scene::attenuationFactor(attenuation, radius) ==
              doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
    }
}

TEST_CASE("a light made for a radius is full at its source, falls with distance, never to zero") {
    const scene::Attenuation attenuation = scene::attenuationForRadius(3.0F);
    CHECK(scene::attenuationFactor(attenuation, 0.0F) == 1.0F);

    float previous = 1.0F;
    for (const float distance : {0.5F, 1.0F, 1.5F, 3.0F, 6.0F, 30.0F, 300.0F}) {
        const float factor = scene::attenuationFactor(attenuation, distance);
        CHECK(factor < previous);
        CHECK(factor > 0.0F);
        previous = factor;
    }
}

TEST_CASE("a radius that is not positive gives a light without attenuation") {
    for (const float radius : {0.0F, -2.0F}) {
        const scene::Attenuation attenuation = scene::attenuationForRadius(radius);
        CHECK(attenuation.constant == 1.0F);
        CHECK(attenuation.linear == 0.0F);
        CHECK(attenuation.quadratic == 0.0F);
    }
}

TEST_CASE("coneCosines turns the two angles into their cosines") {
    const scene::ConeCosines cone = scene::coneCosines(0.0F, 60.0F);
    CHECK(cone.inner == doctest::Approx(1.0F));
    CHECK(cone.outer == doctest::Approx(0.5F));
    // The inner cone is the narrower one, and a smaller angle has a larger cosine.
    CHECK(cone.inner > cone.outer);
}

TEST_CASE("coneCosines never gives two equal cosines") {
    // Equal angles (a hard edge) and angles in the wrong order.
    for (const scene::ConeCosines cone :
         {scene::coneCosines(20.0F, 20.0F), scene::coneCosines(30.0F, 20.0F)}) {
        CHECK(cone.outer == doctest::Approx(0.9396926F));
        CHECK(cone.inner == doctest::Approx(cone.outer + scene::MIN_CONE_COSINE_GAP));
        CHECK(cone.inner > cone.outer);
    }
}

TEST_CASE("spotFactor is 1 inside the inner cone, 0 outside the outer one, a ramp between") {
    const scene::ConeCosines cone{.inner = 0.9F, .outer = 0.7F};
    CHECK(scene::spotFactor(cone, 1.0F) == 1.0F);
    CHECK(scene::spotFactor(cone, 0.9F) == doctest::Approx(1.0F));
    CHECK(scene::spotFactor(cone, 0.8F) == doctest::Approx(0.5F));
    CHECK(scene::spotFactor(cone, 0.75F) == doctest::Approx(0.25F));
    CHECK(scene::spotFactor(cone, 0.7F) == doctest::Approx(0.0F));
    CHECK(scene::spotFactor(cone, 0.2F) == 0.0F);
    // Behind the light the cosine is negative.
    CHECK(scene::spotFactor(cone, -1.0F) == 0.0F);
}

TEST_CASE("spotFactor of a cone with a hard edge is 0 or 1, never not a number") {
    const scene::ConeCosines cone = scene::coneCosines(20.0F, 20.0F);
    CHECK(scene::spotFactor(cone, 1.0F) == 1.0F);
    CHECK(scene::spotFactor(cone, 0.5F) == 0.0F);
}

TEST_CASE("directionFromAngles follows the compass of the camera") {
    checkVector(scene::directionFromAngles(0.0F, 0.0F), {0.0F, 0.0F, -1.0F});
    checkVector(scene::directionFromAngles(90.0F, 0.0F), {1.0F, 0.0F, 0.0F});
    checkVector(scene::directionFromAngles(180.0F, 0.0F), {0.0F, 0.0F, 1.0F});
    checkVector(scene::directionFromAngles(270.0F, 0.0F), {-1.0F, 0.0F, 0.0F});
    // Straight down and straight up, whatever the yaw.
    checkVector(scene::directionFromAngles(123.0F, -90.0F), {0.0F, -1.0F, 0.0F});
    checkVector(scene::directionFromAngles(123.0F, 90.0F), {0.0F, 1.0F, 0.0F});
}

TEST_CASE("directionFromAngles gives a vector of length 1") {
    for (const float yaw : {0.0F, 25.0F, 200.0F}) {
        for (const float pitch : {-80.0F, -50.0F, 0.0F, 45.0F}) {
            CHECK(glm::length(scene::directionFromAngles(yaw, pitch)) == doctest::Approx(1.0F));
        }
    }
}

TEST_CASE("an empty light set has no point lights and a spot light that is on") {
    const scene::LightSet lights;
    CHECK(lights.pointCount == 0);
    CHECK(lights.spotEnabled);
    CHECK(lights.points.size() == static_cast<std::size_t>(scene::MAX_POINT_LIGHTS));
    CHECK(scene::MAX_POINT_LIGHTS == 16);
}

TEST_CASE("packLightBlock copies the camera, the ambient light and the moon") {
    scene::LightSet lights;
    lights.ambient = {0.1F, 0.2F, 0.3F};
    lights.directional = {
        .direction = {0.0F, -4.0F, 3.0F},
        .color = {0.5F, 0.6F, 0.7F},
        .intensity = 0.8F,
    };

    const scene::LightBlockData block = scene::packLightBlock(lights, {1.0F, 2.0F, 3.0F});

    checkVector(block.cameraPosition, {1.0F, 2.0F, 3.0F, 0.0F});
    checkVector(block.ambient, {0.1F, 0.2F, 0.3F, 0.0F});
    // The direction arrives with length 1: (0, -4, 3) has length 5.
    checkVector(block.directionalDirection, {0.0F, -0.8F, 0.6F, 0.0F});
    // The intensity travels in the fourth float of the colour.
    checkVector(block.directionalColor, {0.5F, 0.6F, 0.7F, 0.8F});
}

TEST_CASE("packLightBlock packs the spot light with cosines and its switch") {
    scene::LightSet lights;
    lights.spot = {
        .position = {4.0F, 1.7F, 5.0F},
        .direction = {2.0F, 0.0F, 0.0F},
        .color = {1.0F, 0.9F, 0.8F},
        .intensity = 2.5F,
        .attenuation = {.constant = 1.0F, .linear = 0.25F, .quadratic = 0.125F},
        .innerConeDegrees = 0.0F,
        .outerConeDegrees = 60.0F,
    };
    lights.spotEnabled = true;

    scene::LightBlockData block = scene::packLightBlock(lights, glm::vec3{0.0F});

    checkVector(block.spotPosition, {4.0F, 1.7F, 5.0F, 0.0F});
    checkVector(block.spotDirection, {1.0F, 0.0F, 0.0F, 0.0F});
    checkVector(block.spotColor, {1.0F, 0.9F, 0.8F, 2.5F});
    checkVector(block.spotAttenuation, {1.0F, 0.25F, 0.125F, 0.0F});
    // x: cos(0) = 1, y: cos(60) = 0.5, z: switched on.
    checkVector(block.spotCone, {1.0F, 0.5F, 1.0F, 0.0F});

    lights.spotEnabled = false;
    block = scene::packLightBlock(lights, glm::vec3{0.0F});
    CHECK(block.spotCone.z == 0.0F);
}

TEST_CASE("packLightBlock keeps the cone cosines apart") {
    scene::LightSet lights;
    lights.spot.innerConeDegrees = 25.0F;
    lights.spot.outerConeDegrees = 25.0F;

    const scene::LightBlockData block = scene::packLightBlock(lights, glm::vec3{0.0F});

    // The shader divides by x - y.
    CHECK(block.spotCone.x - block.spotCone.y > 0.0F);
}

TEST_CASE("packLightBlock replaces a direction of length zero") {
    scene::LightSet lights;
    lights.directional.direction = glm::vec3{0.0F};
    lights.spot.direction = glm::vec3{0.0F};

    const scene::LightBlockData block = scene::packLightBlock(lights, glm::vec3{0.0F});

    // Straight down, and above all a real number in every float.
    checkVector(block.directionalDirection, {0.0F, -1.0F, 0.0F, 0.0F});
    checkVector(block.spotDirection, {0.0F, -1.0F, 0.0F, 0.0F});
}

TEST_CASE("packLightBlock packs the point lights in use and leaves the rest zero") {
    scene::LightSet lights;
    lights.points[0] = {
        .position = {1.0F, 1.4F, 7.0F},
        .color = {0.2F, 0.9F, 0.8F},
        .intensity = 2.0F,
        .attenuation = scene::attenuationForRadius(3.0F),
    };
    lights.points[1] = {
        .position = {7.0F, 1.4F, 3.0F},
        .color = {1.0F, 0.0F, 0.0F},
        .intensity = 0.5F,
        .attenuation = {},
    };
    // A third light that is filled in but not counted must not reach the shader.
    lights.points[2].position = {9.0F, 9.0F, 9.0F};
    lights.pointCount = 2;

    const scene::LightBlockData block = scene::packLightBlock(lights, glm::vec3{0.0F});

    CHECK(block.pointCount == 2);
    checkVector(block.points[0].position, {1.0F, 1.4F, 7.0F, 0.0F});
    checkVector(block.points[0].color, {0.2F, 0.9F, 0.8F, 2.0F});
    checkVector(block.points[0].attenuation, {1.0F, 2.0F / 3.0F, 17.0F / 9.0F, 0.0F});
    checkVector(block.points[1].position, {7.0F, 1.4F, 3.0F, 0.0F});
    checkVector(block.points[1].color, {1.0F, 0.0F, 0.0F, 0.5F});
    checkVector(block.points[1].attenuation, {1.0F, 0.0F, 0.0F, 0.0F});
    checkVector(block.points[2].position, glm::vec4{0.0F});
    checkVector(block.points[2].color, glm::vec4{0.0F});

    for (const std::int32_t padding : block.padding) {
        CHECK(padding == 0);
    }
}

TEST_CASE("packLightBlock limits the number of point lights to the array") {
    scene::LightSet lights;

    lights.pointCount = scene::MAX_POINT_LIGHTS + 5;
    CHECK(scene::packLightBlock(lights, glm::vec3{0.0F}).pointCount == scene::MAX_POINT_LIGHTS);

    lights.pointCount = -3;
    CHECK(scene::packLightBlock(lights, glm::vec3{0.0F}).pointCount == 0);
}

TEST_CASE("the light block has the size and the offsets of the std140 block") {
    // The same numbers are checked by static_assert in LightBlock.hpp. They are repeated
    // here so that they show up in the test list, next to the rules they follow from.
    CHECK(sizeof(scene::PointLightData) == 48);                // 3 vec4
    CHECK(offsetof(scene::LightBlockData, pointCount) == 144); // after 9 vec4
    CHECK(offsetof(scene::LightBlockData, points) == 160);     // the next multiple of 16
    CHECK(sizeof(scene::LightBlockData) == 928);               // 160 + 16 * 48
    // The size of a std140 block is a multiple of 16.
    CHECK(sizeof(scene::LightBlockData) % 16 == 0);
}
