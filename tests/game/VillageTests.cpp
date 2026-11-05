// Tests of game/Village: the side the village lies on, its place in the sky, the camera
// that rises to show it and the timing of its lights.
#include "game/Village.hpp"

#include "assets/ObjLoader.hpp"
#include "game/Campaign.hpp"
#include "game/Fog.hpp"
#include "game/GateLamp.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/MenuCamera.hpp"
#include "scene/Camera.hpp"

#include <doctest/doctest.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace {

using game::Direction;

// The lane of the model as tools/blender/build_village.py builds it, in the space of the
// game: 257 m to the north of the eye and 27 m above it.
constexpr glm::vec4 LANE{0.0F, game::VILLAGE_LANE_HEIGHT, -game::VILLAGE_LANE_DISTANCE, 1.0F};

constexpr float FAR_PLANE = 100.0F;

// How far above the horizon a point lies for an eye in the origin, in degrees.
float elevationDegrees(const glm::vec3& point) {
    return glm::degrees(std::atan2(point.y, glm::length(glm::vec2{point.x, point.z})));
}

constexpr game::VillageRevealPose START{
    .eye = {4.0F, 1.7F, 9.0F}, .yawDegrees = 350.0F, .pitchDegrees = -20.0F, .fovDegrees = 60.0F};
constexpr glm::vec3 EXIT_GROUND{5.0F, 0.2F, 9.0F};
constexpr float ASIDE = game::VILLAGE_REVEAL_BESIDE_CARD_DEGREES;

} // namespace

TEST_CASE("the village lies beyond the gate: on the side opposite to it") {
    for (const std::uint32_t seed : {1U, 2U, 7U, 42U}) {
        const game::MazeWorld world = game::buildMazeWorld(6, 6, seed);
        REQUIRE(world.hasGate);
        CHECK(game::villageSide(world) == game::opposite(game::gateSide(world)));
    }
}

TEST_CASE("a camera that looks at the village has the yaw of its side") {
    CHECK(game::villageYawDegrees(Direction::North) == doctest::Approx(0.0F));
    CHECK(game::villageYawDegrees(Direction::East) == doctest::Approx(90.0F));
    CHECK(game::villageYawDegrees(Direction::South) == doctest::Approx(180.0F));
    CHECK(game::villageYawDegrees(Direction::West) == doctest::Approx(270.0F));
    // The camera agrees: with that yaw it looks at the lane of the turned model.
    for (const Direction side : game::ALL_DIRECTIONS) {
        scene::Camera camera;
        camera.yawDegrees = game::villageYawDegrees(side);
        const glm::vec3 lane{game::villageMatrix(side, FAR_PLANE) * LANE};
        const glm::vec3 level = glm::normalize(glm::vec3{lane.x, 0.0F, lane.z});
        CHECK(glm::dot(camera.forward(), level) == doctest::Approx(1.0F));
    }
}

TEST_CASE("the village matrix turns the model to its side") {
    const glm::vec3 north{game::villageMatrix(Direction::North, FAR_PLANE) * LANE};
    const glm::vec3 east{game::villageMatrix(Direction::East, FAR_PLANE) * LANE};
    const glm::vec3 south{game::villageMatrix(Direction::South, FAR_PLANE) * LANE};
    const glm::vec3 west{game::villageMatrix(Direction::West, FAR_PLANE) * LANE};
    CHECK(north.z < 0.0F);
    CHECK(north.x == doctest::Approx(0.0F).epsilon(0.001));
    CHECK(east.x > 0.0F);
    CHECK(east.z == doctest::Approx(0.0F).epsilon(0.001));
    CHECK(south.z > 0.0F);
    CHECK(west.x < 0.0F);
}

TEST_CASE("the village keeps its angles and fits inside the far plane") {
    for (const float farPlane : {100.0F, 250.0F}) {
        for (const Direction side : game::ALL_DIRECTIONS) {
            const glm::vec3 lane{game::villageMatrix(side, farPlane) * LANE};
            // Scaling about the eye changes no angle: the lane stays 6 degrees up.
            CHECK(elevationDegrees(lane) == doctest::Approx(6.0F).epsilon(0.01));
            // The furthest vertex of the model lies at nine tenths of the far plane, so
            // the lane, which is nearer, lies inside it.
            const float scale = glm::length(lane) / glm::length(glm::vec3{LANE});
            CHECK(scale * game::VILLAGE_REACH == doctest::Approx(0.9F * farPlane));
            CHECK(glm::length(lane) < 0.9F * farPlane);
        }
    }
}

TEST_CASE("the ridge closes the whole horizon and its foot lies deep under the eye") {
    // The real model. Seen from a raised camera a ridge that ended beside the village was
    // a black shape afloat in the sky, so the file is checked: in every twelfth of the
    // way round, the upper edge is above the horizon and the foot is where the header
    // says it is.
    assets::ObjModel model;
    std::string error;
    const bool ok = assets::loadObj(
        std::filesystem::path{NIGHT_MAZE_ASSETS_DIR} / "models" / "village.obj", model, error);
    CAPTURE(error);
    REQUIRE(ok);

    constexpr std::size_t SECTORS = 12;
    constexpr float SECTOR_DEGREES = 360.0F / static_cast<float>(SECTORS);
    std::array<float, SECTORS> highest{};
    std::array<float, SECTORS> lowest{};
    highest.fill(-90.0F);
    lowest.fill(90.0F);
    for (const gfx::Vertex& vertex : model.vertices) {
        CHECK(glm::length(vertex.position) <= game::VILLAGE_REACH);
        const float around = glm::degrees(std::atan2(vertex.position.x, -vertex.position.z));
        const auto sector = static_cast<std::size_t>((around + 180.0F) / SECTOR_DEGREES) % SECTORS;
        const float elevation = elevationDegrees(vertex.position);
        highest.at(sector) = std::max(highest.at(sector), elevation);
        lowest.at(sector) = std::min(lowest.at(sector), elevation);
        // Nothing lies below the foot, and what lies as deep is the foot.
        CHECK(vertex.position.y >= doctest::Approx(-game::VILLAGE_FOOT_DEPTH));
        if (vertex.position.y < 0.0F) {
            CHECK(glm::length(glm::vec2{vertex.position.x, vertex.position.z}) ==
                  doctest::Approx(game::VILLAGE_FOOT_DISTANCE).epsilon(0.001));
        }
    }
    const float foot =
        -glm::degrees(std::atan2(game::VILLAGE_FOOT_DEPTH, game::VILLAGE_FOOT_DISTANCE));
    for (std::size_t sector = 0; sector < SECTORS; ++sector) {
        CAPTURE(sector);
        CHECK(highest.at(sector) > 0.5F);
        CHECK(lowest.at(sector) == doctest::Approx(foot).epsilon(0.001));
    }
}

TEST_CASE("no camera of the game sees the foot of the ridge: it stays in the fog") {
    // What the composite pass does with the pixel of the foot (game::fogAmountAt), for
    // the far plane and the fog the game starts with, at every height of the eye up to
    // VILLAGE_CLEAR_EYE_HEIGHT.
    const game::FogSettings fog;
    const glm::vec3 foot{
        game::villageMatrix(Direction::North, scene::Camera{}.farPlane) *
        glm::vec4{0.0F, -game::VILLAGE_FOOT_DEPTH, -game::VILLAGE_FOOT_DISTANCE, 1.0F}};
    for (int half = 0; half <= static_cast<int>(game::VILLAGE_CLEAR_EYE_HEIGHT * 2.0F); ++half) {
        const glm::vec3 eye{0.0F, static_cast<float>(half) * 0.5F, 0.0F};
        CHECK(game::fogAmountAt(fog, eye, eye + foot) > 0.99F);
    }

    // The cameras of the game stay below that height: the glide, at every moment of its
    // round, over the largest maze the debug window builds (40 by 40 cells, the hard level
    // has 22 by 22), and the reveal.
    const game::MazeWorld world = game::buildMazeWorld(40, 40, 1U);
    game::MenuCameraSettings glide;
    glide.shot = game::MenuShot::HighGlide;
    for (int second = 0; second < 300; second += 5) {
        const float height =
            game::menuCameraPose({}, world, glide, static_cast<float>(second)).eye.y;
        CHECK(height > game::WALL_HEIGHT);
        CHECK(height < game::VILLAGE_CLEAR_EYE_HEIGHT / 2.0F);
    }
    CHECK(game::VILLAGE_REVEAL_HEIGHT < game::VILLAGE_CLEAR_EYE_HEIGHT / 2.0F);
}

TEST_CASE("the elevation of the debug window lifts the village") {
    const glm::vec3 lifted{game::villageMatrix(Direction::East, FAR_PLANE, 4.0F) * LANE};
    CHECK(elevationDegrees(lifted) == doctest::Approx(10.0F).epsilon(0.01));
    const glm::vec3 sunk{game::villageMatrix(Direction::West, FAR_PLANE, -6.0F) * LANE};
    CHECK(elevationDegrees(sunk) == doctest::Approx(0.0F).epsilon(0.01));
}

TEST_CASE("the reveal starts in the eyes of the player and ends over the exit cell") {
    for (const float seconds : {-1.0F, 0.0F}) {
        const game::VillageRevealPose pose =
            game::villageRevealPose(START, EXIT_GROUND, Direction::East, ASIDE, seconds);
        CHECK(pose.eye.x == doctest::Approx(START.eye.x));
        CHECK(pose.eye.y == doctest::Approx(START.eye.y));
        CHECK(pose.yawDegrees == doctest::Approx(START.yawDegrees));
        CHECK(pose.pitchDegrees == doctest::Approx(START.pitchDegrees));
        CHECK(pose.fovDegrees == doctest::Approx(START.fovDegrees));
    }
    for (const float seconds : {game::VILLAGE_REVEAL_SECONDS, 60.0F}) {
        const game::VillageRevealPose pose =
            game::villageRevealPose(START, EXIT_GROUND, Direction::East, ASIDE, seconds);
        CHECK(pose.eye.x == doctest::Approx(EXIT_GROUND.x));
        CHECK(pose.eye.y == doctest::Approx(EXIT_GROUND.y + game::VILLAGE_REVEAL_HEIGHT));
        CHECK(pose.eye.z == doctest::Approx(EXIT_GROUND.z));
        CHECK(pose.yawDegrees == doctest::Approx(90.0F + ASIDE));
        // Without a card in the picture the camera ends looking straight at the village.
        CHECK(game::villageRevealPose(START, EXIT_GROUND, Direction::East, 0.0F, seconds)
                  .yawDegrees == doctest::Approx(90.0F));
        CHECK(pose.pitchDegrees == doctest::Approx(game::VILLAGE_REVEAL_PITCH_DEGREES));
        CHECK(pose.fovDegrees == doctest::Approx(game::VILLAGE_REVEAL_FOV_DEGREES));
    }
}

TEST_CASE("the reveal rises without a jump and never sinks") {
    float lastHeight = START.eye.y;
    for (int step = 1; step <= 70; ++step) {
        const float seconds = static_cast<float>(step) * 0.05F;
        const float height =
            game::villageRevealPose(START, EXIT_GROUND, Direction::South, ASIDE, seconds).eye.y;
        CHECK(height >= lastHeight);
        // 3.5 m in 3.5 s, slow at both ends: never more than 0.1 m in a twentieth second.
        CHECK(height - lastHeight < 0.1F);
        lastHeight = height;
    }
}

TEST_CASE("the reveal turns the short way round") {
    // From 350 degrees to the north (0, and the offset on top): through 360, not back
    // through 180.
    const float end = ASIDE;
    const game::VillageRevealPose half = game::villageRevealPose(
        START, EXIT_GROUND, Direction::North, ASIDE, game::VILLAGE_REVEAL_SECONDS / 2.0F);
    const float halfway = (350.0F + 360.0F + end) / 2.0F;
    CHECK(half.yawDegrees == doctest::Approx(std::fmod(halfway, 360.0F)));
    // And the other way: from 10 degrees to the west (270) the camera turns left.
    game::VillageRevealPose start = START;
    start.yawDegrees = 10.0F;
    const game::VillageRevealPose west = game::villageRevealPose(
        start, EXIT_GROUND, Direction::West, ASIDE, game::VILLAGE_REVEAL_SECONDS / 2.0F);
    CHECK(west.yawDegrees == doctest::Approx((370.0F + 270.0F + end) / 2.0F));
    // The yaw stays in 0 to 360 all the way.
    for (int step = 0; step <= 35; ++step) {
        const float yaw = game::villageRevealPose(START, EXIT_GROUND, Direction::North, ASIDE,
                                                  static_cast<float>(step) * 0.1F)
                              .yawDegrees;
        CHECK(yaw >= 0.0F);
        CHECK(yaw < 360.0F);
    }
}

TEST_CASE("the village stands in the left half of the reveal, above the middle") {
    // With the lens of the reveal a picture of 16 by 9 shows 11 degrees up and down and
    // 19 to each side. The houses are 10.4 degrees wide: all of them must lie left of the
    // middle of the picture and inside it, and the lane above the middle and the cap of
    // the tower under the upper edge.
    const float halfHeight = game::VILLAGE_REVEAL_FOV_DEGREES / 2.0F;
    const float halfWidth =
        glm::degrees(std::atan(std::tan(glm::radians(halfHeight)) * 16.0F / 9.0F));
    CHECK(halfWidth == doctest::Approx(19.06F).epsilon(0.01));
    CHECK(ASIDE > 5.2F);
    CHECK(ASIDE < halfWidth - 5.2F);
    const float lane = 6.0F - game::VILLAGE_REVEAL_PITCH_DEGREES;
    const float cap = 9.0F - game::VILLAGE_REVEAL_PITCH_DEGREES;
    CHECK(lane > 0.0F);
    CHECK(cap < halfHeight);
}

TEST_CASE("the new lights come on when the camera has almost arrived") {
    CHECK(game::villageNewLightStrength(0.0F) == doctest::Approx(0.0F));
    CHECK(game::villageNewLightStrength(game::VILLAGE_NEW_LIGHT_DELAY_SECONDS) ==
          doctest::Approx(0.0F));
    CHECK(game::VILLAGE_NEW_LIGHT_DELAY_SECONDS < game::VILLAGE_REVEAL_SECONDS);
    const float end = game::VILLAGE_NEW_LIGHT_DELAY_SECONDS + game::VILLAGE_NEW_LIGHT_SECONDS;
    CHECK(end > game::VILLAGE_REVEAL_SECONDS);
    CHECK(game::villageNewLightStrength(end) == doctest::Approx(1.0F));
    CHECK(game::villageNewLightStrength(600.0F) == doctest::Approx(1.0F));
    float last = 0.0F;
    for (int step = 0; step <= 60; ++step) {
        const float strength = game::villageNewLightStrength(static_cast<float>(step) * 0.1F);
        CHECK(strength >= last);
        last = strength;
    }
}

TEST_CASE("the look at the village shows the last window lit before it fades to black") {
    CHECK(game::villageBeatBlack(0.0F) == doctest::Approx(0.0F));
    const float fadeFrom = game::VILLAGE_BEAT_SECONDS - game::VILLAGE_BEAT_FADE_SECONDS;
    CHECK(game::villageBeatBlack(fadeFrom) == doctest::Approx(0.0F));
    CHECK(game::villageBeatBlack(game::VILLAGE_BEAT_SECONDS) == doctest::Approx(1.0F));
    // The last window is fully lit for more than a second of clear picture.
    const float lit = game::VILLAGE_NEW_LIGHT_DELAY_SECONDS + game::VILLAGE_NEW_LIGHT_SECONDS;
    CHECK(fadeFrom - lit > 1.0F);
    // The ending card that follows is black from its first frame: no jump.
    CHECK(game::villageBeatBlack(game::VILLAGE_BEAT_SECONDS + 1.0F) == doctest::Approx(1.0F));
}
