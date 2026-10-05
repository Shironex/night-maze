// Tests of game::MazeWorld: model matrices, collision boxes and the start of a built maze.
// See docs/modules/game/maze-rendering.md
#include "game/MazeWorld.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// Where a model matrix puts a point of the local space of a model. w = 1 marks a point,
// so the translation of the matrix applies.
glm::vec3 transformPoint(const glm::mat4& matrix, const glm::vec3& point) {
    return glm::vec3{matrix * glm::vec4{point, 1.0F}};
}

} // namespace

TEST_CASE("the default maze is 10 by 10 cells with seed 1") {
    const game::MazeSettings settings;
    CHECK(settings.width == 10);
    CHECK(settings.height == 10);
    CHECK(settings.seed == 1U);
    CHECK_FALSE(settings.regenerate);
}

TEST_CASE("yawTowards follows the compass of the camera") {
    CHECK(game::yawTowards(game::Direction::North) == 0.0F);
    CHECK(game::yawTowards(game::Direction::East) == 90.0F);
    CHECK(game::yawTowards(game::Direction::South) == 180.0F);
    CHECK(game::yawTowards(game::Direction::West) == 270.0F);
}

TEST_CASE("a maze world has one matrix and one box per wall and pillar") {
    constexpr int WIDTH = 7;
    constexpr int HEIGHT = 4;
    constexpr std::uint32_t SEED = 9;
    const game::MazeWorld world = game::buildMazeWorld(WIDTH, HEIGHT, SEED);

    CHECK(world.maze.width() == WIDTH);
    CHECK(world.maze.height() == HEIGHT);
    CHECK(world.seed == SEED);

    // The same lists as the layout functions give for the maze.
    CHECK(world.walls.size() == game::wallSegments(world.maze).size());
    CHECK(world.pillars.size() == game::pillarPositions(world.maze).size());

    CHECK(world.wallMatrices.size() == world.walls.size());
    CHECK(world.pillarMatrices.size() == world.pillars.size());
    CHECK(world.colliders.size() == world.walls.size() + world.pillars.size());
}

TEST_CASE("the same size and seed give the same maze world") {
    const game::MazeWorld first = game::buildMazeWorld(8, 8, 42U);
    const game::MazeWorld second = game::buildMazeWorld(8, 8, 42U);

    REQUIRE(first.walls.size() == second.walls.size());
    for (std::size_t i = 0; i < first.walls.size(); ++i) {
        CHECK(first.walls[i].position == second.walls[i].position);
        CHECK(first.walls[i].axis == second.walls[i].axis);
    }
    CHECK(first.startYawDegrees == second.startYawDegrees);
}

TEST_CASE("pillars are only moved to their place") {
    const game::MazeWorld world = game::buildMazeWorld(3, 2, 0U);

    REQUIRE(world.pillarMatrices.size() == world.pillars.size());
    for (std::size_t i = 0; i < world.pillars.size(); ++i) {
        checkVector(transformPoint(world.pillarMatrices[i], glm::vec3{0.0F}), world.pillars[i]);
    }
}

TEST_CASE("a wall along X keeps the model as it is, a wall along Z turns it a quarter") {
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 3U);
    REQUIRE(world.wallMatrices.size() == world.walls.size());

    // The wall model lies along its local X axis: its ends are at x = -1 and x = +1,
    // and its top at y = 3.
    const glm::vec3 modelEnd{1.0F, 0.0F, 0.0F};
    const glm::vec3 modelTop{0.0F, 3.0F, 0.0F};

    bool sawAlongX = false;
    bool sawAlongZ = false;
    for (std::size_t i = 0; i < world.walls.size(); ++i) {
        const game::WallSegment& wall = world.walls[i];
        const glm::mat4& matrix = world.wallMatrices[i];

        // The origin of the model goes to the position of the segment, and up stays up.
        checkVector(transformPoint(matrix, glm::vec3{0.0F}), wall.position);
        checkVector(transformPoint(matrix, modelTop), wall.position + modelTop);

        // Where the end of the model lands, measured from the middle of the wall.
        const glm::vec3 end = transformPoint(matrix, modelEnd) - wall.position;
        if (wall.axis == game::WallAxis::AlongX) {
            sawAlongX = true;
            checkVector(end, {1.0F, 0.0F, 0.0F});
        } else {
            sawAlongZ = true;
            // One metre along Z, to either side: the model is the same from both ends.
            CHECK(end.x == doctest::Approx(0.0F));
            CHECK(end.y == doctest::Approx(0.0F));
            CHECK(std::abs(end.z) == doctest::Approx(1.0F));
        }
    }
    // The test means nothing unless the maze has walls of both kinds.
    CHECK(sawAlongX);
    CHECK(sawAlongZ);
}

TEST_CASE("the player starts in the first cell and looks down an open passage") {
    constexpr std::uint32_t SEED_COUNT = 20;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::MazeWorld world = game::buildMazeWorld(6, 5, seed);

        checkVector(world.startPosition, game::cellCenter(0, 0));

        // The first cell is in the north-west corner: north and west are the border, so
        // the open side is east or south. The yaw must point at a side without a wall.
        const bool looksEast = world.startYawDegrees == 90.0F;
        const bool looksSouth = world.startYawDegrees == 180.0F;
        REQUIRE((looksEast || looksSouth));
        const game::Direction side = looksEast ? game::Direction::East : game::Direction::South;
        CHECK_FALSE(world.maze.hasWall(0, 0, side));
    }
}

TEST_CASE("a maze of one cell has no open side: the player looks north") {
    const game::MazeWorld world = game::buildMazeWorld(1, 1, 0U);

    CHECK(world.startYawDegrees == 0.0F);
    checkVector(world.startPosition, game::cellCenter(0, 0));
    // The only cell is the start and the exit at once, and it has no side for a gate.
    checkVector(world.exitPosition, game::cellCenter(0, 0));
    CHECK(world.exitCell == game::MazeCell{.x = 0, .z = 0});
    CHECK_FALSE(world.hasGate);
    CHECK(world.crystals.empty());
    CHECK(world.wallMatrices.size() == 4U);
    CHECK(world.pillarMatrices.size() == 4U);
}
