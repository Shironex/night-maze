// Tests of the parts of the fog that need no OpenGL context: its two formulas and the
// way from a pixel and its depth back to a place in the world.
// See docs/modules/renderer/post-process.md
#include "game/Fog.hpp"

#include "game/Lighting.hpp"
#include "scene/Camera.hpp"
#include "scene/Light.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <numbers>

// The fog itself is computed by post/composite.frag, which repeats these formulas, and
// is checked by running the game.

namespace {

// ln(2), about 0.693: after ln(2) / k units, exp(-k * x) has fallen to one half.
constexpr float LN_2 = std::numbers::ln2_v<float>;

// Width divided by height of the window the game starts with.
constexpr float ASPECT_RATIO = 1280.0F / 720.0F;

// Where a world point lands on the screen for a camera: its texture coordinate (0 to 1
// in both directions) in x and y and the value of the depth buffer (0 to 1) in z. These
// are the steps of the graphics card: the two matrices, the perspective division, and
// from -1..1 to 0..1.
glm::vec3 screenPointOf(const glm::vec3& point, const glm::mat4& viewProjection) {
    const glm::vec4 clip = viewProjection * glm::vec4{point, 1.0F};
    const glm::vec3 ndc = glm::vec3{clip} / clip.w;
    return ndc * 0.5F + 0.5F;
}

} // namespace

TEST_CASE("the fog has its full density at and below the base height") {
    CHECK(game::fogHeightFactor(0.5F, 0.5F, 0.35F) == doctest::Approx(1.0F));
    CHECK(game::fogHeightFactor(0.0F, 0.5F, 0.35F) == doctest::Approx(1.0F));
    CHECK(game::fogHeightFactor(-10.0F, 0.5F, 0.35F) == doctest::Approx(1.0F));
}

TEST_CASE("the fog thins out above the base height") {
    const float base = 0.5F;
    const float falloff = 0.35F;
    // Halved every ln(2) / falloff metres.
    const float halfHeight = LN_2 / falloff;
    CHECK(game::fogHeightFactor(base + halfHeight, base, falloff) == doctest::Approx(0.5F));
    CHECK(game::fogHeightFactor(base + 2.0F * halfHeight, base, falloff) == doctest::Approx(0.25F));
    // Worked out by hand: 2 m above the base, exp(-0.35 * 2) = exp(-0.7) = 0.4966.
    CHECK(game::fogHeightFactor(2.5F, base, falloff) == doctest::Approx(0.4966F).epsilon(0.001));
    // It never reaches 0 and never goes below it.
    CHECK(game::fogHeightFactor(50.0F, base, falloff) >= 0.0F);
    CHECK(game::fogHeightFactor(50.0F, base, falloff) < 0.001F);
}

TEST_CASE("a height falloff of zero gives the same fog at every height") {
    CHECK(game::fogHeightFactor(0.0F, 0.5F, 0.0F) == doctest::Approx(1.0F));
    CHECK(game::fogHeightFactor(80.0F, 0.5F, 0.0F) == doctest::Approx(1.0F));
}

TEST_CASE("there is no fog at distance zero, at density zero and where the height factor is zero") {
    CHECK(game::fogAmount(0.08F, 1.0F, 0.0F) == doctest::Approx(0.0F));
    CHECK(game::fogAmount(0.0F, 1.0F, 50.0F) == doctest::Approx(0.0F));
    CHECK(game::fogAmount(0.08F, 0.0F, 50.0F) == doctest::Approx(0.0F));
}

TEST_CASE("the fog grows with the distance and never passes one") {
    const float density = 0.08F;
    float before = 0.0F;
    // Up to 100 m, the far plane of the camera. Much further a float cannot tell the
    // result from 1 any more.
    for (int metres = 1; metres <= 100; ++metres) {
        const float amount = game::fogAmount(density, 1.0F, static_cast<float>(metres));
        CHECK(amount > before);
        CHECK(amount <= 1.0F);
        before = amount;
    }
}

TEST_CASE("the fog follows the exponential law") {
    const float density = 0.08F;
    // Half of the surface is gone after ln(2) / density metres, 63 % after 1 / density.
    CHECK(game::fogAmount(density, 1.0F, LN_2 / density) == doctest::Approx(0.5F));
    CHECK(game::fogAmount(density, 1.0F, 1.0F / density) ==
          doctest::Approx(1.0F - std::exp(-1.0F)));
    // Worked out by hand: 10 m of fog, exp(-0.8) = 0.4493, so 0.5507 is fog.
    CHECK(game::fogAmount(density, 1.0F, 10.0F) == doctest::Approx(0.5507F).epsilon(0.001));
    // Two stretches one after the other let through the product of what each lets
    // through: 10 m and then 5 m are the same as 15 m.
    const float through10 = 1.0F - game::fogAmount(density, 1.0F, 10.0F);
    const float through5 = 1.0F - game::fogAmount(density, 1.0F, 5.0F);
    const float through15 = 1.0F - game::fogAmount(density, 1.0F, 15.0F);
    CHECK(through10 * through5 == doctest::Approx(through15));
    // The height factor scales the density: half the factor is half the density.
    CHECK(game::fogAmount(density, 0.5F, 10.0F) ==
          doctest::Approx(game::fogAmount(density / 2.0F, 1.0F, 10.0F)));
}

TEST_CASE("the fog of a point uses its distance from the eye and its own height") {
    game::FogSettings settings;
    settings.density = 0.1F;
    settings.baseHeight = 0.0F;
    settings.heightFalloff = 0.5F;
    const glm::vec3 eye{0.0F, 1.7F, 0.0F};

    // On the ground, 3 m to the side and 4 m ahead, 1.7 m below the eye: the distance
    // is the length of (3, -1.7, -4), the square root of 27.89.
    const glm::vec3 onGround{3.0F, 0.0F, -4.0F};
    const float distance = std::sqrt(27.89F);
    CHECK(game::fogAmountAt(settings, eye, onGround) ==
          doctest::Approx(1.0F - std::exp(-0.1F * distance)));

    // The same distance ahead, but high up: less fog.
    const glm::vec3 highUp{3.0F, 3.4F, -4.0F};
    CHECK(game::fogAmountAt(settings, eye, highUp) ==
          doctest::Approx(1.0F - std::exp(-0.1F * std::exp(-0.5F * 3.4F) * distance)));
    CHECK(game::fogAmountAt(settings, eye, highUp) < game::fogAmountAt(settings, eye, onGround));
}

TEST_CASE("a point of the world is found again from its pixel and its depth") {
    scene::Camera camera;
    camera.yawDegrees = 35.0F;
    camera.pitchDegrees = -12.0F;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    const glm::mat4 viewProjection = camera.projectionMatrix(ASPECT_RATIO) * camera.viewMatrix(eye);
    const glm::mat4 inverseViewProjection = glm::inverse(viewProjection);

    // A point 6 m in front of the eye, a little to the right and above the middle.
    const glm::vec3 point =
        eye + camera.forward() * 6.0F + camera.right() * 1.5F + scene::Camera::WORLD_UP * 0.5F;
    const glm::vec3 screen = screenPointOf(point, viewProjection);
    // It is on the screen and between the clipping planes.
    REQUIRE(screen.x > 0.0F);
    REQUIRE(screen.x < 1.0F);
    REQUIRE(screen.y > 0.0F);
    REQUIRE(screen.y < 1.0F);
    REQUIRE(screen.z > 0.0F);
    REQUIRE(screen.z < 1.0F);

    const glm::vec3 found = game::worldPositionFromDepth(glm::vec2{screen.x, screen.y}, screen.z,
                                                         inverseViewProjection);
    // The depth is packed close to 1 (6 m away is 0.984), so a float keeps the
    // distance to within some millimetres only.
    CHECK(found.x == doctest::Approx(point.x).epsilon(0.01));
    CHECK(found.y == doctest::Approx(point.y).epsilon(0.01));
    CHECK(found.z == doctest::Approx(point.z).epsilon(0.01));
}

TEST_CASE("the middle of the screen at depth zero and one lies on the clipping planes") {
    scene::Camera camera;
    const glm::vec3 eye{0.0F, 1.7F, 0.0F};
    const glm::mat4 inverseViewProjection =
        glm::inverse(camera.projectionMatrix(ASPECT_RATIO) * camera.viewMatrix(eye));
    const glm::vec2 middle{0.5F, 0.5F};

    // Depth 0 is the near plane and depth 1 the far plane (and the sky), both straight
    // ahead of the eye for the middle pixel.
    const glm::vec3 nearPoint = game::worldPositionFromDepth(middle, 0.0F, inverseViewProjection);
    const glm::vec3 farPoint = game::worldPositionFromDepth(middle, 1.0F, inverseViewProjection);
    CHECK(glm::length(nearPoint - eye) == doctest::Approx(camera.nearPlane).epsilon(0.001));
    CHECK(glm::length(farPoint - eye) == doctest::Approx(camera.farPlane).epsilon(0.001));
    const glm::vec3 direction = glm::normalize(farPoint - eye);
    CHECK(glm::dot(direction, camera.forward()) == doctest::Approx(1.0F).epsilon(0.0001));
}

TEST_CASE("the default fog leaves the moon clear and hides the sky below the horizon") {
    const game::FogSettings fog;
    const game::LightingSettings lighting;
    const scene::Camera camera;
    const glm::vec3 eye{10.0F, 1.7F, 10.0F};

    // A pixel of the sky is a point on the far plane. The moon stands where its light
    // comes from: against the direction the light travels in.
    const glm::vec3 towardsMoon =
        -scene::directionFromAngles(lighting.moonYawDegrees, lighting.moonPitchDegrees);
    const glm::vec3 moonPoint = eye + towardsMoon * camera.farPlane;
    CHECK(game::fogAmountAt(fog, eye, moonPoint) < 0.01F);

    // Straight down past the edge of the land the sky would show: there the fog is solid.
    const glm::vec3 belowHorizon = eye + glm::vec3{0.0F, -1.0F, 0.0F} * camera.farPlane;
    CHECK(game::fogAmountAt(fog, eye, belowHorizon) > 0.99F);
}

TEST_CASE("the default fog does not hide the maze") {
    const game::FogSettings fog;
    CHECK(fog.enabled);
    CHECK(fog.density > 0.0F);
    CHECK(fog.heightFalloff > 0.0F);
    const glm::vec3 eye{1.0F, 1.7F, 1.0F};
    // The ground four cells (8 m) ahead: a clear share of fog, but less than half.
    const float nearGround = game::fogAmountAt(fog, eye, glm::vec3{1.0F, 0.2F, 9.0F});
    CHECK(nearGround > 0.2F);
    CHECK(nearGround < 0.6F);
    // The top of a wall (3 m) at the same place has less fog than its foot.
    CHECK(game::fogAmountAt(fog, eye, glm::vec3{1.0F, 3.0F, 9.0F}) < nearGround);
}
