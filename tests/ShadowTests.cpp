// Tests of the parts of the shadows that need no OpenGL context: the light space of the
// moon (the box its shadow map covers) and of the flashlight (a pyramid), the bias and
// the small helpers of the settings.
// See docs/modules/renderer/shadows.md
#include "game/Shadows.hpp"

#include "game/Lighting.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Terrain.hpp"
#include "scene/Camera.hpp"
#include "scene/Collider.hpp"
#include "scene/Light.hpp"
#include "scene/LightSpace.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>

// The shadow map itself (the depth pass, the comparison in common/shadows.glsl) needs
// a window and is checked by running the game.

namespace {

// A box that is not a cube and does not sit at the origin, so that a mix-up of two axes
// or a forgotten offset shows.
constexpr scene::Aabb TEST_BOUNDS{.min = {-4.0F, 0.0F, 2.0F}, .max = {10.0F, 6.0F, 30.0F}};

// The eight corners of a box.
std::array<glm::vec3, 8> cornersOf(const scene::Aabb& box) {
    return {
        glm::vec3{box.min.x, box.min.y, box.min.z}, glm::vec3{box.max.x, box.min.y, box.min.z},
        glm::vec3{box.min.x, box.max.y, box.min.z}, glm::vec3{box.max.x, box.max.y, box.min.z},
        glm::vec3{box.min.x, box.min.y, box.max.z}, glm::vec3{box.max.x, box.min.y, box.max.z},
        glm::vec3{box.min.x, box.max.y, box.max.z}, glm::vec3{box.max.x, box.max.y, box.max.z},
    };
}

// A spot light for the tests of the perspective light space: it does not stand at the
// origin, and it has the cone and the reach of the flashlight of the game.
constexpr glm::vec3 SPOT_POSITION{3.0F, 1.45F, -7.0F};
constexpr float SPOT_OUTER_DEGREES = 21.0F;
constexpr float SPOT_RANGE = 16.0F;

// The direction the light of the moon travels in with the default settings.
glm::vec3 defaultMoonDirection() {
    return game::moonDirection(game::LightingSettings{});
}

// True when every number of the matrix is a real number (not NaN and not infinite).
bool isFinite(const glm::mat4& matrix) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (!std::isfinite(matrix[column][row])) {
                return false;
            }
        }
    }
    return true;
}

// The smallest and the largest shadow map coordinate of the corners of a box.
struct CoordinateRange {
    glm::vec3 smallest{0.0F};
    glm::vec3 largest{0.0F};
};

CoordinateRange coordinateRangeOf(const scene::Aabb& box, const scene::LightSpace& lightSpace) {
    const std::array<glm::vec3, 8> corners = cornersOf(box);
    CoordinateRange range;
    range.smallest = scene::shadowMapCoordinates(lightSpace.matrix(), corners[0]);
    range.largest = range.smallest;
    for (const glm::vec3& corner : corners) {
        const glm::vec3 coordinates = scene::shadowMapCoordinates(lightSpace.matrix(), corner);
        range.smallest = glm::min(range.smallest, coordinates);
        range.largest = glm::max(range.largest, coordinates);
    }
    return range;
}

} // namespace

TEST_CASE("the box of a directional light holds every corner of its bounds") {
    const scene::LightSpace lightSpace =
        scene::directionalLightSpace(TEST_BOUNDS, defaultMoonDirection());
    for (const glm::vec3& corner : cornersOf(TEST_BOUNDS)) {
        const glm::vec3 coordinates = scene::shadowMapCoordinates(lightSpace.matrix(), corner);
        // Inside the map in both directions, and between its near and its far plane.
        CHECK(coordinates.x > 0.0F);
        CHECK(coordinates.x < 1.0F);
        CHECK(coordinates.y > 0.0F);
        CHECK(coordinates.y < 1.0F);
        CHECK(coordinates.z > 0.0F);
        CHECK(coordinates.z < 1.0F);
    }
}

TEST_CASE("the box of a directional light fits its bounds: only the margin is left free") {
    const scene::LightSpace lightSpace =
        scene::directionalLightSpace(TEST_BOUNDS, defaultMoonDirection());
    const CoordinateRange range = coordinateRangeOf(TEST_BOUNDS, lightSpace);

    // A coordinate times the size of the box is a distance in metres from its side. On
    // every one of the six sides some corner comes as close as the margin: no texel
    // row and no part of the depth range is wasted on empty space.
    const glm::vec3 freeBefore = range.smallest * lightSpace.extent;
    const glm::vec3 freeAfter = (glm::vec3{1.0F} - range.largest) * lightSpace.extent;
    for (int axis = 0; axis < 3; ++axis) {
        CHECK(freeBefore[axis] == doctest::Approx(scene::LIGHT_BOX_MARGIN).epsilon(0.001));
        CHECK(freeAfter[axis] == doctest::Approx(scene::LIGHT_BOX_MARGIN).epsilon(0.001));
    }
}

TEST_CASE("a light that shines straight down sees the bounds from above") {
    const glm::vec3 straightDown{0.0F, -1.0F, 0.0F};
    const scene::LightSpace lightSpace = scene::directionalLightSpace(TEST_BOUNDS, straightDown);

    // lookAt with an up vector parallel to the view direction would divide by zero.
    CHECK(isFinite(lightSpace.matrix()));

    // Seen from above the box is 14 m by 28 m and 6 m deep, plus the margin on both
    // sides. Up is -Z for this light, so the width of the map lies along X.
    const float margins = 2.0F * scene::LIGHT_BOX_MARGIN;
    CHECK(lightSpace.extent.x == doctest::Approx(14.0F + margins));
    CHECK(lightSpace.extent.y == doctest::Approx(28.0F + margins));
    CHECK(lightSpace.extent.z == doctest::Approx(6.0F + margins));

    // The top of the box is nearer to the light than its bottom.
    const glm::vec3 top = scene::shadowMapCoordinates(lightSpace.matrix(), {3.0F, 6.0F, 16.0F});
    const glm::vec3 bottom = scene::shadowMapCoordinates(lightSpace.matrix(), {3.0F, 0.0F, 16.0F});
    CHECK(top.z < bottom.z);
    // The same texel: one is straight below the other.
    CHECK(top.x == doctest::Approx(bottom.x));
    CHECK(top.y == doctest::Approx(bottom.y));
}

TEST_CASE("a light that shines almost straight down still gets a usable matrix") {
    // The Lights panel offers every pitch from -90 to -5 degrees.
    for (const float pitch : {-90.0F, -89.9F, -89.0F, -87.0F, -85.0F, -5.0F}) {
        const glm::vec3 direction = scene::directionFromAngles(25.0F, pitch);
        const scene::LightSpace lightSpace = scene::directionalLightSpace(TEST_BOUNDS, direction);
        CHECK(isFinite(lightSpace.matrix()));
        const CoordinateRange range = coordinateRangeOf(TEST_BOUNDS, lightSpace);
        CHECK(glm::all(glm::greaterThan(range.smallest, glm::vec3{0.0F})));
        CHECK(glm::all(glm::lessThan(range.largest, glm::vec3{1.0F})));
    }
}

TEST_CASE("a light direction of length zero is replaced by straight down") {
    const scene::LightSpace fallback = scene::directionalLightSpace(TEST_BOUNDS, glm::vec3{0.0F});
    const scene::LightSpace down =
        scene::directionalLightSpace(TEST_BOUNDS, glm::vec3{0.0F, -1.0F, 0.0F});
    CHECK(isFinite(fallback.matrix()));
    CHECK(fallback.matrix() == down.matrix());
}

TEST_CASE("the length of the light direction does not change the box") {
    const glm::vec3 direction = defaultMoonDirection();
    const scene::LightSpace unit = scene::directionalLightSpace(TEST_BOUNDS, direction);
    const scene::LightSpace longer = scene::directionalLightSpace(TEST_BOUNDS, direction * 25.0F);
    const glm::vec3 point{2.0F, 1.0F, 9.0F};
    const glm::vec3 a = scene::shadowMapCoordinates(unit.matrix(), point);
    const glm::vec3 b = scene::shadowMapCoordinates(longer.matrix(), point);
    CHECK(a.x == doctest::Approx(b.x));
    CHECK(a.y == doctest::Approx(b.y));
    CHECK(a.z == doctest::Approx(b.z));
}

TEST_CASE("points on one ray of the light share a texel and differ in depth only") {
    const glm::vec3 direction = defaultMoonDirection();
    const scene::LightSpace lightSpace = scene::directionalLightSpace(TEST_BOUNDS, direction);

    // A point on top of a wall and the point 3 m further along the ray: the place its
    // shadow falls on.
    const glm::vec3 caster{3.0F, 3.0F, 16.0F};
    const float distance = 3.0F;
    const glm::vec3 receiver = caster + direction * distance;

    const glm::vec3 casterCoordinates = scene::shadowMapCoordinates(lightSpace.matrix(), caster);
    const glm::vec3 receiverCoordinates =
        scene::shadowMapCoordinates(lightSpace.matrix(), receiver);

    // This is what makes a shadow map work: the receiver looks up the texel the caster
    // was drawn into, and finds a depth smaller than its own.
    CHECK(receiverCoordinates.x == doctest::Approx(casterCoordinates.x));
    CHECK(receiverCoordinates.y == doctest::Approx(casterCoordinates.y));
    CHECK(receiverCoordinates.z > casterCoordinates.z);
    // The depth of an orthographic box grows evenly: 3 m of its whole depth range.
    CHECK(receiverCoordinates.z - casterCoordinates.z ==
          doctest::Approx(distance / lightSpace.extent.z));
}

TEST_CASE("a point outside the bounds lands outside the shadow map") {
    const scene::LightSpace lightSpace =
        scene::directionalLightSpace(TEST_BOUNDS, glm::vec3{0.0F, -1.0F, 0.0F});
    // 100 m east of the box, seen from straight above: past the edge of the map.
    const glm::vec3 coordinates =
        scene::shadowMapCoordinates(lightSpace.matrix(), {110.0F, 1.0F, 16.0F});
    CHECK(coordinates.x > 1.0F);
    // 100 m below the box: past its far plane.
    const glm::vec3 below =
        scene::shadowMapCoordinates(lightSpace.matrix(), {3.0F, -100.0F, 16.0F});
    CHECK(below.z > 1.0F);
}

TEST_CASE("the caster bounds of the moon hold the land and everything that stands on it") {
    const game::MazeWorld world = game::buildMazeWorld(
        game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, game::DEFAULT_MAZE_SEED);
    const scene::Aabb bounds = game::shadowCasterBounds(world.terrain);

    // The whole land, the margin around the maze included.
    CHECK(bounds.min.x == doctest::Approx(world.terrain.minX()));
    CHECK(bounds.max.x == doctest::Approx(world.terrain.maxX()));
    CHECK(bounds.min.z == doctest::Approx(world.terrain.minZ()));
    CHECK(bounds.max.z == doctest::Approx(world.terrain.maxZ()));
    CHECK(bounds.min.y == doctest::Approx(world.terrain.minHeight()));
    CHECK(bounds.max.y == doctest::Approx(world.terrain.maxHeight() + game::PILLAR_HEIGHT));

    // Every wall and every pillar.
    REQUIRE_FALSE(world.colliders.empty());
    for (const scene::Aabb& box : world.colliders) {
        CHECK(box.min.x >= bounds.min.x);
        CHECK(box.max.x <= bounds.max.x);
        CHECK(box.min.z >= bounds.min.z);
        CHECK(box.max.z <= bounds.max.z);
        CHECK(box.max.y <= bounds.max.y);
    }
}

TEST_CASE("the shadow map of the moon is fine enough for the walls of the default maze") {
    const game::MazeWorld world = game::buildMazeWorld(
        game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, game::DEFAULT_MAZE_SEED);
    const scene::LightSpace lightSpace = scene::directionalLightSpace(
        game::shadowCasterBounds(world.terrain), defaultMoonDirection());

    // The land is a square of 48 m (a maze of 20 m and 14 m of margin on each side).
    // The moon looks at it from yaw 25 degrees, so the square is turned in the map and
    // is 48 * (cos 25 + sin 25) = 63.8 m wide, plus the margin on both sides.
    CHECK(lightSpace.extent.x == doctest::Approx(64.8F).epsilon(0.005));
    // A texel of the large map is then about 3.2 cm, and a wall of 0.2 m is 6 texels
    // thick. The small map has texels twice that size.
    const float fine = game::shadowTexelSize(lightSpace, game::SHADOW_MAP_SIZE_HIGH);
    const float coarse = game::shadowTexelSize(lightSpace, game::SHADOW_MAP_SIZE_LOW);
    CHECK(fine == doctest::Approx(0.0316F).epsilon(0.01));
    CHECK(coarse == doctest::Approx(2.0F * fine));
    CHECK(game::WALL_VISUAL_THICKNESS / fine > 6.0F);
}

TEST_CASE("the texel size of a shadow map follows its larger side") {
    scene::LightSpace lightSpace;
    lightSpace.extent = {40.0F, 20.0F, 10.0F};
    CHECK(game::shadowTexelSize(lightSpace, 1000) == doctest::Approx(0.04F));
    lightSpace.extent = {20.0F, 50.0F, 10.0F};
    CHECK(game::shadowTexelSize(lightSpace, 1000) == doctest::Approx(0.05F));
    // A map without texels has no texel size.
    CHECK(game::shadowTexelSize(lightSpace, 0) == doctest::Approx(0.0F));
}

TEST_CASE("the shadow bias is the constant part plus the slope part of a tilted surface") {
    const float constantBias = 0.02F;
    const float slopeBias = 0.1F;
    // Facing the light: the constant part only.
    CHECK(game::shadowBias(constantBias, slopeBias, 1.0F) == doctest::Approx(0.02F));
    // Grazed by the light: both parts in full.
    CHECK(game::shadowBias(constantBias, slopeBias, 0.0F) == doctest::Approx(0.12F));
    // 60 degrees between the normal and the light: cos 60 = 0.5, half of the slope part.
    CHECK(game::shadowBias(constantBias, slopeBias, 0.5F) == doctest::Approx(0.07F));
    // Facing away, or a cosine a rounding error above 1: as at the ends of the range.
    CHECK(game::shadowBias(constantBias, slopeBias, -0.7F) == doctest::Approx(0.12F));
    CHECK(game::shadowBias(constantBias, slopeBias, 1.2F) == doctest::Approx(0.02F));
    // No bias at all: the setting that shows shadow acne.
    CHECK(game::shadowBias(0.0F, 0.0F, 0.3F) == doctest::Approx(0.0F));
}

TEST_CASE("a bias in metres becomes a share of the depth range of the map") {
    // 5 cm in a box that is 50 m deep: a thousandth of the stored range.
    CHECK(game::biasInDepthUnits(0.05F, 50.0F) == doctest::Approx(0.001F));
    CHECK(game::biasInDepthUnits(0.0F, 50.0F) == doctest::Approx(0.0F));
    // No division by zero for a box without depth.
    CHECK(game::biasInDepthUnits(0.05F, 0.0F) == doctest::Approx(0.0F));
    CHECK(game::biasInDepthUnits(0.05F, -3.0F) == doctest::Approx(0.0F));
}

TEST_CASE("the shadow resolutions have their sizes") {
    CHECK(game::shadowMapSize(game::ShadowResolution::Low) == 1024);
    CHECK(game::shadowMapSize(game::ShadowResolution::High) == 2048);
    // The default is the large map.
    CHECK(game::shadowMapSize(game::ShadowSettings{}.resolution) == game::SHADOW_MAP_SIZE_HIGH);
}

TEST_CASE("the PCF kernel has an odd side and a radius inside its limits") {
    CHECK(game::pcfKernelSide(1) == 3);
    CHECK(game::pcfKernelSide(2) == 5);
    CHECK(game::pcfKernelSide(3) == 7);

    game::ShadowSettings settings;
    // The default: a kernel of 3 x 3.
    CHECK(game::pcfRadiusInUse(settings) == 1);
    settings.pcfRadius = 2;
    CHECK(game::pcfRadiusInUse(settings) == 2);
    // Numbers from a slider, where anything can be typed.
    settings.pcfRadius = 40;
    CHECK(game::pcfRadiusInUse(settings) == game::MAX_PCF_RADIUS);
    settings.pcfRadius = -2;
    CHECK(game::pcfRadiusInUse(settings) == game::MIN_PCF_RADIUS);
    // Switched off: one comparison, whatever the radius says.
    settings.pcf = false;
    CHECK(game::pcfRadiusInUse(settings) == 0);
}

TEST_CASE("the moon direction of the settings is the one the lights are built with") {
    game::LightingSettings settings;
    settings.moonYawDegrees = 140.0F;
    settings.moonPitchDegrees = -30.0F;
    const scene::LightSet lights = game::buildLightSet(settings, game::FlashlightPose{}, {});
    const glm::vec3 direction = game::moonDirection(settings);
    CHECK(glm::length(direction) == doctest::Approx(1.0F));
    CHECK(lights.directional.direction.x == doctest::Approx(direction.x));
    CHECK(lights.directional.direction.y == doctest::Approx(direction.y));
    CHECK(lights.directional.direction.z == doctest::Approx(direction.z));
}

TEST_CASE("the light space of a directional light is an orthographic box without a position") {
    const scene::LightSpace lightSpace =
        scene::directionalLightSpace(TEST_BOUNDS, defaultMoonDirection());
    CHECK(lightSpace.kind == scene::LightProjection::Orthographic);
    CHECK(lightSpace.position == glm::vec3{0.0F});
    CHECK(lightSpace.nearPlane == 0.0F);
    CHECK(lightSpace.farPlane == 0.0F);
}

TEST_CASE("a point on the axis of a spot light lands in the middle of its shadow map") {
    const glm::vec3 direction = glm::normalize(glm::vec3{1.0F, -0.2F, 0.5F});
    const scene::LightSpace lightSpace =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);

    for (const float distance : {0.5F, 2.0F, 8.0F, 15.0F}) {
        const glm::vec3 coordinates =
            scene::shadowMapCoordinates(lightSpace.matrix(), SPOT_POSITION + direction * distance);
        CHECK(coordinates.x == doctest::Approx(0.5F));
        CHECK(coordinates.y == doctest::Approx(0.5F));
        // Between the near and the far plane.
        CHECK(coordinates.z > 0.0F);
        CHECK(coordinates.z < 1.0F);
    }
}

TEST_CASE("the whole cone of a spot light is inside its shadow map, with a margin") {
    const glm::vec3 direction = glm::normalize(glm::vec3{1.0F, -0.2F, 0.5F});
    const scene::LightSpace lightSpace =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);

    // Two directions across the axis, to walk around the side of the cone with.
    const glm::vec3 across = glm::normalize(glm::cross(direction, glm::vec3{0.0F, 1.0F, 0.0F}));
    const glm::vec3 upwards = glm::cross(across, direction);
    const float outer = glm::radians(SPOT_OUTER_DEGREES);

    // The map looks 21 + 2 degrees to every side. A ray on the side of the cone is 21
    // degrees from the axis, so its distance from the middle of the map is
    // tan(21) / tan(23) of the half width of the map: 0.452 of the 0.5.
    const float expected =
        0.5F * std::tan(outer) /
        std::tan(glm::radians(SPOT_OUTER_DEGREES + scene::SPOT_CONE_MARGIN_DEGREES));
    CHECK(expected == doctest::Approx(0.452F).epsilon(0.005));

    for (const float turn : {0.0F, 45.0F, 90.0F, 135.0F, 180.0F, 225.0F, 270.0F, 315.0F}) {
        const float angle = glm::radians(turn);
        const glm::vec3 sideways = across * std::cos(angle) + upwards * std::sin(angle);
        const glm::vec3 ray = direction * std::cos(outer) + sideways * std::sin(outer);
        for (const float distance : {1.0F, 6.0F, 15.0F}) {
            const glm::vec3 coordinates =
                scene::shadowMapCoordinates(lightSpace.matrix(), SPOT_POSITION + ray * distance);
            // The same place in the map at every distance: the rays of a spot light
            // spread out exactly as its map does.
            const float fromMiddle =
                glm::length(glm::vec2{coordinates.x, coordinates.y} - glm::vec2{0.5F});
            CHECK(fromMiddle == doctest::Approx(expected).epsilon(0.001));
            CHECK(coordinates.x > 0.0F);
            CHECK(coordinates.x < 1.0F);
            CHECK(coordinates.y > 0.0F);
            CHECK(coordinates.y < 1.0F);
        }
    }
}

TEST_CASE("the depth of a spot light map runs from its near to its far plane, unevenly") {
    const glm::vec3 direction{0.0F, 0.0F, -1.0F};
    const scene::LightSpace lightSpace =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);
    CHECK(lightSpace.nearPlane == scene::SPOT_NEAR_PLANE);
    CHECK(lightSpace.farPlane == SPOT_RANGE);

    const glm::vec3 atNear = scene::shadowMapCoordinates(
        lightSpace.matrix(), SPOT_POSITION + direction * scene::SPOT_NEAR_PLANE);
    const glm::vec3 atFar =
        scene::shadowMapCoordinates(lightSpace.matrix(), SPOT_POSITION + direction * SPOT_RANGE);
    // The depth changes fastest right at the near plane, so a rounding error in the
    // place of the point shows there: hence the wider tolerance.
    CHECK(atNear.z == doctest::Approx(0.0F).epsilon(0.001));
    CHECK(atFar.z == doctest::Approx(1.0F));

    // Half way to the far plane the stored depth is not 0.5 at all: almost the whole
    // range is used up within the first metre. This is why a bias cannot be a number
    // of depth units here, and why the preview picture needs a conversion.
    const glm::vec3 halfWay = scene::shadowMapCoordinates(
        lightSpace.matrix(), SPOT_POSITION + direction * (SPOT_RANGE * 0.5F));
    CHECK(halfWay.z > 0.99F);
    const glm::vec3 oneMetre =
        scene::shadowMapCoordinates(lightSpace.matrix(), SPOT_POSITION + direction);
    CHECK(oneMetre.z > 0.95F);

    // Past the far plane and to the side of the pyramid: outside the map.
    const glm::vec3 beyond = scene::shadowMapCoordinates(
        lightSpace.matrix(), SPOT_POSITION + direction * (SPOT_RANGE + 5.0F));
    CHECK(beyond.z > 1.0F);
    const glm::vec3 aside = scene::shadowMapCoordinates(
        lightSpace.matrix(), SPOT_POSITION + direction * 4.0F + glm::vec3{4.0F, 0.0F, 0.0F});
    CHECK(aside.x > 1.0F);
}

TEST_CASE("the light space of a spot light keeps its position, its planes and its size") {
    const glm::vec3 direction{0.0F, 0.0F, -1.0F};
    const scene::LightSpace lightSpace =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);

    CHECK(lightSpace.kind == scene::LightProjection::Perspective);
    CHECK(lightSpace.position == SPOT_POSITION);
    // At the far plane, 16 m away, a map that opens 2 * 23 degrees covers
    // 2 * 16 * tan(23) = 13.58 m in each direction.
    CHECK(lightSpace.extent.x == doctest::Approx(13.58F).epsilon(0.001));
    CHECK(lightSpace.extent.y == doctest::Approx(lightSpace.extent.x));
    CHECK(lightSpace.extent.z == doctest::Approx(SPOT_RANGE - scene::SPOT_NEAR_PLANE));

    // The corner of that square at the far plane is the corner of the map.
    const float half = lightSpace.extent.x * 0.5F;
    const glm::vec3 corner = SPOT_POSITION + direction * SPOT_RANGE + glm::vec3{half, half, 0.0F};
    const glm::vec3 coordinates = scene::shadowMapCoordinates(lightSpace.matrix(), corner);
    CHECK(coordinates.x == doctest::Approx(1.0F));
    CHECK(coordinates.y == doctest::Approx(1.0F));
}

TEST_CASE("a spot light that points straight up or down still gets a usable matrix") {
    // The player can look almost straight up and down (scene::Camera::MAX_PITCH_DEGREES),
    // and the flashlight follows the view.
    for (const float pitch : {-90.0F, -89.0F, -87.5F, -87.0F, -60.0F, 0.0F, 87.0F, 89.0F, 90.0F}) {
        const glm::vec3 direction = scene::directionFromAngles(140.0F, pitch);
        const scene::LightSpace lightSpace =
            scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);
        // lookAt with an up vector parallel to the view direction would divide by zero.
        CHECK(isFinite(lightSpace.matrix()));
        const glm::vec3 coordinates =
            scene::shadowMapCoordinates(lightSpace.matrix(), SPOT_POSITION + direction * 3.0F);
        CHECK(coordinates.x == doctest::Approx(0.5F));
        CHECK(coordinates.y == doctest::Approx(0.5F));
    }
}

TEST_CASE("the direction of a spot light may have any length, and none means straight down") {
    const glm::vec3 direction = glm::normalize(glm::vec3{1.0F, -0.2F, 0.5F});
    const scene::LightSpace unit =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);
    const scene::LightSpace longer =
        scene::spotLightSpace(SPOT_POSITION, direction * 25.0F, SPOT_OUTER_DEGREES, SPOT_RANGE);
    const glm::vec3 point = SPOT_POSITION + glm::vec3{3.0F, -1.0F, 1.0F};
    const glm::vec3 a = scene::shadowMapCoordinates(unit.matrix(), point);
    const glm::vec3 b = scene::shadowMapCoordinates(longer.matrix(), point);
    CHECK(a.x == doctest::Approx(b.x));
    CHECK(a.y == doctest::Approx(b.y));
    CHECK(a.z == doctest::Approx(b.z));

    const scene::LightSpace fallback =
        scene::spotLightSpace(SPOT_POSITION, glm::vec3{0.0F}, SPOT_OUTER_DEGREES, SPOT_RANGE);
    const scene::LightSpace down = scene::spotLightSpace(
        SPOT_POSITION, glm::vec3{0.0F, -1.0F, 0.0F}, SPOT_OUTER_DEGREES, SPOT_RANGE);
    CHECK(isFinite(fallback.matrix()));
    CHECK(fallback.matrix() == down.matrix());
}

TEST_CASE("a spot light with a cone or a range out of bounds still gets a usable matrix") {
    const glm::vec3 direction{0.0F, 0.0F, -1.0F};

    // A cone of 120 degrees to each side would be a map that opens 244 degrees. The
    // opening angle stops at its largest value.
    const scene::LightSpace wide = scene::spotLightSpace(SPOT_POSITION, direction, 120.0F, 10.0F);
    CHECK(isFinite(wide.matrix()));
    const float widest =
        2.0F * 10.0F * std::tan(glm::radians(scene::MAX_SPOT_FIELD_OF_VIEW_DEGREES * 0.5F));
    CHECK(wide.extent.x == doctest::Approx(widest));

    // A cone without any width: the smallest opening angle.
    const scene::LightSpace thin = scene::spotLightSpace(SPOT_POSITION, direction, -30.0F, 10.0F);
    CHECK(isFinite(thin.matrix()));
    CHECK(thin.extent.x > 0.0F);

    // A range of 0 would put the far plane on the light, before the near plane.
    const scene::LightSpace noRange =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, 0.0F);
    CHECK(isFinite(noRange.matrix()));
    CHECK(noRange.farPlane > noRange.nearPlane);
}

TEST_CASE("moving a point towards a spot light keeps its texel and lowers its depth") {
    // This is the bias of the flashlight in common/shadows.glsl: the fragment is moved
    // some centimetres along the straight line to the light before it is looked up.
    const glm::vec3 direction = glm::normalize(glm::vec3{1.0F, -0.2F, 0.5F});
    const scene::LightSpace lightSpace =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);

    // A point off the axis, 6 m from the light.
    const glm::vec3 point = SPOT_POSITION + direction * 5.8F + glm::vec3{0.0F, 1.5F, 0.0F};
    const glm::vec3 toLight = glm::normalize(SPOT_POSITION - point);
    const float bias = 0.1F;

    const glm::vec3 plain = scene::shadowMapCoordinates(lightSpace.matrix(), point);
    const glm::vec3 biased =
        scene::shadowMapCoordinates(lightSpace.matrix(), point + toLight * bias);

    CHECK(biased.x == doctest::Approx(plain.x));
    CHECK(biased.y == doctest::Approx(plain.y));
    CHECK(biased.z < plain.z);
}

TEST_CASE("a point behind a spot light has no place in its shadow map") {
    const glm::vec3 direction{0.0F, 0.0F, -1.0F};
    const scene::LightSpace lightSpace =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);

    // w of the clip position is the distance in front of the light. Behind the light
    // it is negative: the shader tests for that and does not divide by it.
    const glm::vec4 inFront =
        lightSpace.matrix() * glm::vec4{SPOT_POSITION + direction * 3.0F, 1.0F};
    const glm::vec4 behind =
        lightSpace.matrix() * glm::vec4{SPOT_POSITION - direction * 3.0F, 1.0F};
    CHECK(inFront.w == doctest::Approx(3.0F));
    CHECK(behind.w == doctest::Approx(-3.0F));
}

TEST_CASE("the flashlight and its shadow map stand in the same place and look the same way") {
    const game::LightingSettings settings;
    scene::Camera camera;
    camera.yawDegrees = 70.0F;
    camera.pitchDegrees = -20.0F;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};

    // One pose, used twice, as in NightMazeApp::onRender.
    const game::FlashlightPose pose =
        game::flashlightPose(settings, eye, camera.forward(), camera.right());
    const scene::LightSet lights = game::buildLightSet(settings, pose, {});
    const scene::LightSpace lightSpace = scene::spotLightSpace(
        pose.position, pose.direction, settings.flashlightOuterDegrees, settings.flashlightRange);

    CHECK(lightSpace.position == lights.spot.position);
    CHECK(lightSpace.farPlane == settings.flashlightRange);
    // A point on the axis of the cone of light is in the middle of the map.
    const glm::vec3 onAxis = lights.spot.position + lights.spot.direction * 5.0F;
    const glm::vec3 coordinates = scene::shadowMapCoordinates(lightSpace.matrix(), onAxis);
    CHECK(coordinates.x == doctest::Approx(0.5F));
    CHECK(coordinates.y == doctest::Approx(0.5F));
}

TEST_CASE("a bias goes to the shaders as depth for a box and as metres for a pyramid") {
    // The moon: a share of the depth range of its box.
    const scene::LightSpace box = scene::directionalLightSpace(TEST_BOUNDS, defaultMoonDirection());
    CHECK(game::biasForShader(0.05F, box) ==
          doctest::Approx(game::biasInDepthUnits(0.05F, box.extent.z)));
    CHECK(game::biasForShader(0.05F, box) == doctest::Approx(0.05F / box.extent.z));

    // The flashlight: the metres themselves, whatever its range is.
    const glm::vec3 direction{0.0F, 0.0F, -1.0F};
    for (const float range : {2.0F, 16.0F, 60.0F}) {
        const scene::LightSpace pyramid =
            scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, range);
        CHECK(game::biasForShader(0.05F, pyramid) == 0.05F);
        CHECK(game::biasForShader(0.0F, pyramid) == 0.0F);
    }
}

TEST_CASE("the texels of a spot light map grow with the distance from the light") {
    const glm::vec3 direction{0.0F, 0.0F, -1.0F};
    const scene::LightSpace pyramid =
        scene::spotLightSpace(SPOT_POSITION, direction, SPOT_OUTER_DEGREES, SPOT_RANGE);
    const int mapSize = game::SHADOW_MAP_SIZE_LOW;

    // The sizes are compared in millimetres: the numbers are then large enough for the
    // tolerance of the comparison to mean something.
    const float millimetres = 1000.0F;

    // At the far plane: the size shadowTexelSize gives, 13.58 m over 1024 texels.
    const float atFarPlane = game::shadowTexelSizeAt(pyramid, mapSize, SPOT_RANGE);
    CHECK(atFarPlane == game::shadowTexelSize(pyramid, mapSize));
    CHECK(atFarPlane * millimetres == doctest::Approx(13.265F).epsilon(0.001));
    // In proportion to the distance: 0.83 mm for every metre, 3.3 mm on a wall 4 m
    // away. A texel of the moon has 32 mm.
    CHECK(game::shadowTexelSizeAt(pyramid, mapSize, 8.0F) * millimetres ==
          doctest::Approx(atFarPlane * 0.5F * millimetres));
    CHECK(game::shadowTexelSizeAt(pyramid, mapSize, 1.0F) * millimetres ==
          doctest::Approx(0.829F).epsilon(0.001));
    CHECK(game::shadowTexelSizeAt(pyramid, mapSize, 4.0F) * millimetres ==
          doctest::Approx(3.316F).epsilon(0.001));
    CHECK(game::shadowTexelSizeAt(pyramid, mapSize, 0.0F) == doctest::Approx(0.0F));
    CHECK(game::shadowTexelSizeAt(pyramid, mapSize, -2.0F) == doctest::Approx(0.0F));
    // A map without texels has no texel size.
    CHECK(game::shadowTexelSizeAt(pyramid, 0, 4.0F) == doctest::Approx(0.0F));

    // In the box of a directional light a texel has one size everywhere.
    const scene::LightSpace box = scene::directionalLightSpace(TEST_BOUNDS, defaultMoonDirection());
    CHECK(game::shadowTexelSizeAt(box, mapSize, 1.0F) ==
          doctest::Approx(game::shadowTexelSize(box, mapSize)));
    CHECK(game::shadowTexelSizeAt(box, mapSize, 30.0F) ==
          doctest::Approx(game::shadowTexelSize(box, mapSize)));
}

TEST_CASE("the shadows of the flashlight start with the small map and a bias of their own") {
    const game::ShadowSettings settings = game::flashlightShadowDefaults();
    CHECK(settings.enabled);
    CHECK(settings.resolution == game::ShadowResolution::Low);
    CHECK(settings.constantBias == game::FLASHLIGHT_SHADOW_CONSTANT_BIAS);
    CHECK(settings.slopeBias == game::FLASHLIGHT_SHADOW_SLOPE_BIAS);
    // Everything else as for the moon.
    const game::ShadowSettings moon;
    CHECK(settings.hardwareFilter == moon.hardwareFilter);
    CHECK(settings.pcf == moon.pcf);
    CHECK(settings.pcfRadius == moon.pcfRadius);
    CHECK(settings.strength == moon.strength);

    // The largest bias a surface can get is below the thickness of a wall, so a shadow
    // does not come loose from the wall that casts it.
    CHECK(game::shadowBias(settings.constantBias, settings.slopeBias, 0.0F) <
          game::WALL_VISUAL_THICKNESS);
}

TEST_CASE("the default bias of the flashlight covers the ground up to 10 m ahead") {
    const game::LightingSettings lighting;
    const game::ShadowSettings settings = game::flashlightShadowDefaults();
    const int mapSize = game::shadowMapSize(settings.resolution);

    // The flashlight of a player who stands on level ground: in the hand, this high.
    const float height = game::Player::EYE_HEIGHT - lighting.flashlightHandDown;
    CHECK(height == doctest::Approx(1.45F));
    const scene::LightSpace lightSpace =
        scene::spotLightSpace({0.0F, height, 0.0F}, {0.0F, 0.0F, -1.0F},
                              lighting.flashlightOuterDegrees, lighting.flashlightRange);

    // The kernel of 3 x 3 with the hardware filter compares texels up to 2 away from
    // the fragment.
    const float texelReach = 2.0F;

    for (const float ahead : {1.0F, 2.0F, 4.0F, 6.0F, 8.0F, 10.0F}) {
        // A point of the ground this far ahead. The light, the point below the light
        // and that point make a right triangle.
        const float distance = std::sqrt(ahead * ahead + height * height);
        // The light meets the ground at a flat angle: the cosine between the normal of
        // the ground (straight up) and the way to the light.
        const float facing = height / distance;
        // One texel further along the ground is this much farther from the light: the
        // size of the texel times the tangent of the angle between the normal and the
        // way to the light.
        const float tangent = ahead / height;
        const float error =
            texelReach * game::shadowTexelSizeAt(lightSpace, mapSize, distance) * tangent;

        const float bias = game::shadowBias(settings.constantBias, settings.slopeBias, facing);
        CHECK(bias > error);
    }

    // The numbers of the comment in Shadows.hpp, 10 m ahead, in centimetres: 11.6 cm
    // are needed and the bias is 12.1 cm.
    const float centimetres = 100.0F;
    const float distance = std::sqrt(10.0F * 10.0F + height * height);
    const float error =
        texelReach * game::shadowTexelSizeAt(lightSpace, mapSize, distance) * (10.0F / height);
    const float bias =
        game::shadowBias(settings.constantBias, settings.slopeBias, height / distance);
    CHECK(error * centimetres == doctest::Approx(11.6F).epsilon(0.005));
    CHECK(bias * centimetres == doctest::Approx(12.1F).epsilon(0.005));
}
