// Tests of the parts of the shadows that need no OpenGL context: the light space of the
// moon (the box its shadow map covers), the bias and the small helpers of the settings.
// See docs/modules/renderer/shadows.md
#include "game/Shadows.hpp"

#include "game/Lighting.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Terrain.hpp"
#include "scene/Collider.hpp"
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
    const scene::LightSet lights =
        game::buildLightSet(settings, glm::vec3{0.0F}, glm::vec3{0.0F, 0.0F, -1.0F}, {});
    const glm::vec3 direction = game::moonDirection(settings);
    CHECK(glm::length(direction) == doctest::Approx(1.0F));
    CHECK(lights.directional.direction.x == doctest::Approx(direction.x));
    CHECK(lights.directional.direction.y == doctest::Approx(direction.y));
    CHECK(lights.directional.direction.z == doctest::Approx(direction.z));
}
