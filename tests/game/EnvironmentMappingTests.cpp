// Tests of the formulas of the environment mapping (the reflected ray, the refracted ray
// and the Fresnel factor), which need no OpenGL context.
#include "game/EnvironmentMapping.hpp"

#include <doctest/doctest.h>
#include <glm/glm.hpp>

#include <cmath>

// The reflections themselves are computed by assets/shaders/reflect.frag, which uses the
// GLSL functions reflect and refract and repeats refractOrReflect and fresnelSchlick.
// GLM implements reflect and refract by the GLSL specification, so comparing with it
// checks the C++ formulas against the ones the graphics card uses. The picture is
// checked by running the game.

namespace {

// A level surface that faces up, like a puddle.
constexpr glm::vec3 UP{0.0F, 1.0F, 0.0F};

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// A ray of length 1 that comes down onto a level surface, at the given angle from the
// normal (0 is straight down), travelling towards +X.
glm::vec3 rayAtAngle(float degrees) {
    const float angle = glm::radians(degrees);
    return {std::sin(angle), -std::cos(angle), 0.0F};
}

// The sine of the angle between a direction of length 1 and the vertical axis: the
// length of its level part.
float sineFromVertical(const glm::vec3& direction) {
    return glm::length(glm::vec2{direction.x, direction.z});
}

} // namespace

TEST_CASE("the defaults of the environment mapping") {
    const game::EnvironmentSettings settings;
    CHECK(settings.enabled);
    CHECK(settings.crystalStrength >= 0.0F);
    CHECK(settings.crystalStrength <= 1.0F);
    CHECK(settings.crystalReflectShare >= 0.0F);
    CHECK(settings.crystalReflectShare <= 1.0F);
    // Air into glass: 1 / 1.5.
    CHECK(settings.crystalRefractionRatio == doctest::Approx(2.0F / 3.0F));
    CHECK(settings.crystalRefractionRatio >= game::MIN_REFRACTION_RATIO);
    CHECK(settings.crystalRefractionRatio <= game::MAX_REFRACTION_RATIO);
    // The glow is whole, so the bloom finds the crystals as before.
    CHECK(settings.crystalGlowShare == 1.0F);
    CHECK(settings.puddles);
    CHECK(settings.puddleShare == game::DEFAULT_PUDDLE_SHARE);
    CHECK(game::MAX_PUDDLE_SHARE >= game::DEFAULT_PUDDLE_SHARE);
    CHECK(settings.puddleFresnel);
    CHECK_FALSE(settings.replacePuddles);
}

TEST_CASE("a ray that hits a level mirror keeps its level part and turns its vertical part") {
    // Coming down at 45 degrees towards +X: it leaves upwards at 45 degrees towards +X.
    checkVector(game::reflectDirection(rayAtAngle(45.0F), UP),
                {std::sqrt(0.5F), std::sqrt(0.5F), 0.0F});
    // Straight down comes straight back up.
    checkVector(game::reflectDirection({0.0F, -1.0F, 0.0F}, UP), {0.0F, 1.0F, 0.0F});
    // A ray along the surface is not turned at all.
    checkVector(game::reflectDirection({1.0F, 0.0F, 0.0F}, UP), {1.0F, 0.0F, 0.0F});
}

TEST_CASE("the mirrored ray leaves at the angle it came in with, and is as long") {
    const glm::vec3 normal = glm::normalize(glm::vec3{0.3F, 0.8F, -0.5F});
    const glm::vec3 incident = glm::normalize(glm::vec3{0.4F, -0.7F, 0.2F});
    const glm::vec3 reflected = game::reflectDirection(incident, normal);

    CHECK(glm::length(reflected) == doctest::Approx(1.0F));
    // The cosine against the normal has the same size and the opposite sign.
    CHECK(glm::dot(reflected, normal) == doctest::Approx(-glm::dot(incident, normal)));
    // The same result as the function of GLSL (in its GLM version).
    checkVector(reflected, glm::reflect(incident, normal));
    // Mirroring twice gives the ray back.
    checkVector(game::reflectDirection(reflected, normal), incident);
}

TEST_CASE("a refracted ray obeys Snell's law") {
    // Air into glass: the sine of the angle after the bend is the sine before it times
    // the ratio, so the ray is bent TOWARDS the normal.
    const float ratio = game::AIR_TO_GLASS_RATIO;
    for (const float degrees : {10.0F, 30.0F, 45.0F, 60.0F, 85.0F}) {
        const glm::vec3 incident = rayAtAngle(degrees);
        const glm::vec3 refracted = game::refractDirection(incident, UP, ratio);

        CHECK(glm::length(refracted) == doctest::Approx(1.0F));
        // It goes on downwards, into the surface.
        CHECK(refracted.y < 0.0F);
        CHECK(sineFromVertical(refracted) ==
              doctest::Approx(ratio * std::sin(glm::radians(degrees))));
        CHECK(sineFromVertical(refracted) < sineFromVertical(incident));
        // The same result as the function of GLSL (in its GLM version).
        checkVector(refracted, glm::refract(incident, UP, ratio));
    }
}

TEST_CASE("a ratio of 1 does not bend the ray, and a ray along the normal is never bent") {
    const glm::vec3 incident = rayAtAngle(40.0F);
    checkVector(game::refractDirection(incident, UP, 1.0F), incident);

    const glm::vec3 straightDown{0.0F, -1.0F, 0.0F};
    checkVector(game::refractDirection(straightDown, UP, game::AIR_TO_GLASS_RATIO), straightDown);
}

TEST_CASE("the refraction ratios of the materials") {
    CHECK(game::AIR_TO_GLASS_RATIO == doctest::Approx(1.0F / 1.5F));
    CHECK(game::WATER_REFRACTIVE_INDEX > game::AIR_REFRACTIVE_INDEX);
    CHECK(game::GLASS_REFRACTIVE_INDEX > game::WATER_REFRACTIVE_INDEX);
    // The slider reaches both sides of "no bend".
    CHECK(game::MIN_REFRACTION_RATIO < 1.0F);
    CHECK(game::MAX_REFRACTION_RATIO > 1.0F);
}

TEST_CASE("total internal reflection: no refracted ray past the critical angle") {
    // Glass into air: the ratio is 1.5. The sine after the bend would be 1.5 times the
    // sine before it, which passes 1 at the critical angle, asin(1 / 1.5) = 41.8 degrees.
    const float ratio = game::GLASS_REFRACTIVE_INDEX / game::AIR_REFRACTIVE_INDEX;
    const float criticalDegrees = glm::degrees(std::asin(1.0F / ratio));
    CHECK(criticalDegrees == doctest::Approx(41.81F).epsilon(0.001));

    // Just inside the critical angle there is a refracted ray, bent AWAY from the normal.
    const glm::vec3 inside = rayAtAngle(criticalDegrees - 1.0F);
    const glm::vec3 refracted = game::refractDirection(inside, UP, ratio);
    CHECK(glm::length(refracted) == doctest::Approx(1.0F));
    CHECK(sineFromVertical(refracted) > sineFromVertical(inside));

    // Just past it there is none: the zero vector, as GLSL refract returns it.
    const glm::vec3 outside = rayAtAngle(criticalDegrees + 1.0F);
    CHECK(game::refractDirection(outside, UP, ratio) == glm::vec3{0.0F});
    CHECK(glm::refract(outside, UP, ratio) == glm::vec3{0.0F});
}

TEST_CASE("refractOrReflect falls back to the mirrored ray and never returns no direction") {
    const float ratio = game::GLASS_REFRACTIVE_INDEX / game::AIR_REFRACTIVE_INDEX;

    // Where the ray can be refracted, it is.
    const glm::vec3 steep = rayAtAngle(20.0F);
    checkVector(game::refractOrReflect(steep, UP, ratio), game::refractDirection(steep, UP, ratio));

    // Where it cannot, all of the light is mirrored.
    const glm::vec3 flat = rayAtAngle(70.0F);
    checkVector(game::refractOrReflect(flat, UP, ratio), game::reflectDirection(flat, UP));

    // Over the whole range of the slider and every angle the result has length 1.
    for (float sliderRatio = game::MIN_REFRACTION_RATIO; sliderRatio <= game::MAX_REFRACTION_RATIO;
         sliderRatio += 0.1F) {
        for (float degrees = 0.0F; degrees < 90.0F; degrees += 5.0F) {
            const glm::vec3 direction =
                game::refractOrReflect(rayAtAngle(degrees), UP, sliderRatio);
            CHECK(glm::length(direction) == doctest::Approx(1.0F));
        }
    }

    // Air into a crystal (a ratio below 1) is never a total internal reflection: the
    // game only meets it with the slider above 1.
    for (float degrees = 0.0F; degrees < 90.0F; degrees += 5.0F) {
        CHECK(game::refractDirection(rayAtAngle(degrees), UP, game::AIR_TO_GLASS_RATIO) !=
              glm::vec3{0.0F});
    }
}

TEST_CASE("the Fresnel factor is the straight-on share from above and 1 along the surface") {
    // Looking straight down: the cosine is 1, and only the straight-on share is mirrored.
    CHECK(game::fresnelSchlick(1.0F, 0.02F) == doctest::Approx(0.02F));
    CHECK(game::fresnelSchlick(1.0F, 0.35F) == doctest::Approx(0.35F));
    // Looking along the surface: the cosine is 0, and everything is mirrored.
    CHECK(game::fresnelSchlick(0.0F, 0.02F) == doctest::Approx(1.0F));
    CHECK(game::fresnelSchlick(0.0F, 0.35F) == doctest::Approx(1.0F));
    // Half way: F0 + (1 - F0) * 0.5^5.
    CHECK(game::fresnelSchlick(0.5F, 0.2F) == doctest::Approx(0.2F + 0.8F / 32.0F));
    // A cosine outside 0..1 (a surface seen from behind) is clamped.
    CHECK(game::fresnelSchlick(-0.5F, 0.2F) == doctest::Approx(1.0F));
    CHECK(game::fresnelSchlick(1.5F, 0.2F) == doctest::Approx(0.2F));
    // A perfect mirror stays one at every angle.
    CHECK(game::fresnelSchlick(0.7F, 1.0F) == doctest::Approx(1.0F));
}

TEST_CASE("the Fresnel factor grows as the look gets flatter") {
    float previous = game::fresnelSchlick(1.0F, 0.35F);
    for (float cosine = 0.9F; cosine >= 0.0F; cosine -= 0.1F) {
        const float factor = game::fresnelSchlick(cosine, 0.35F);
        CHECK(factor > previous);
        CHECK(factor <= 1.0F);
        previous = factor;
    }
}

TEST_CASE("why the default reflectivity of a puddle is far above the one of real water") {
    // The eyes are 1.7 m above the ground (Player::EYE_HEIGHT). The cosine of the angle
    // between the normal of a puddle and the direction to the eye, for a puddle
    // distance metres ahead: the height divided by the length of the line of sight.
    constexpr float EYE_HEIGHT = 1.7F;
    const auto cosineAt = [](float distance) {
        return EYE_HEIGHT / std::sqrt(EYE_HEIGHT * EYE_HEIGHT + distance * distance);
    };

    // Real water (0.02 straight on): a puddle one or two cells ahead mirrors a few
    // percent of the sky, and the night sky is dark to begin with.
    constexpr float REAL_WATER = 0.02F;
    CHECK(game::fresnelSchlick(cosineAt(2.0F), REAL_WATER) < 0.03F);
    CHECK(game::fresnelSchlick(cosineAt(4.0F), REAL_WATER) < 0.11F);

    // The default: about a third of the sky from the next cell, more than half from
    // four cells away.
    const float straightOn = game::EnvironmentSettings{}.puddleReflectivity;
    CHECK(game::fresnelSchlick(cosineAt(2.0F), straightOn) > 0.35F);
    CHECK(game::fresnelSchlick(cosineAt(8.0F), straightOn) > 0.5F);
}
