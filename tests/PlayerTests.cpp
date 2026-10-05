// Tests of game::Player: walking, sprinting, sliding along walls and noclip flight.
// See docs/modules/game/player.md
#include "game/Player.hpp"

#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float. 120 steps are one second.
constexpr float STEP_SECONDS = 1.0F / 120.0F;
constexpr int STEPS_PER_SECOND = 120;

// Camera yaw for the four compass directions, in degrees (see scene::Camera).
constexpr float YAW_NORTH = 0.0F;
constexpr float YAW_EAST = 90.0F;
constexpr float YAW_SOUTH = 180.0F;

// A level look.
constexpr float NO_PITCH = 0.0F;

// Where the centre of the player is when the body touches a wall box: half a wall box
// plus half a body away from the grid line the wall stands on.
constexpr float WALL_CONTACT_DISTANCE =
    game::WALL_COLLISION_THICKNESS / 2.0F + game::Player::BODY_WIDTH / 2.0F;

// Runs the same input for a number of fixed steps.
void runSteps(game::Player& player, const game::PlayerInput& input, float yawDegrees,
              float pitchDegrees, int stepCount, std::span<const scene::Aabb> obstacles) {
    for (int i = 0; i < stepCount; ++i) {
        player.update(input, yawDegrees, pitchDegrees, STEP_SECONDS, obstacles);
    }
}

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// An empty world: a list of no boxes, so there is nothing to collide with.
constexpr std::span<const scene::Aabb> NO_OBSTACLES{};

} // namespace

TEST_CASE("the player constants are the agreed sizes and speeds") {
    CHECK(game::Player::BODY_WIDTH == 0.6F);
    CHECK(game::Player::BODY_HEIGHT == 1.8F);
    CHECK(game::Player::EYE_HEIGHT == 1.7F);
    CHECK(game::Player::WALK_SPEED == 3.0F);
    CHECK(game::Player::SPRINT_SPEED == 5.5F);

    // A new player walks, at the default speeds.
    const game::Player player;
    CHECK_FALSE(player.noclip);
    CHECK(player.walkSpeed == game::Player::WALK_SPEED);
    CHECK(player.sprintSpeed == game::Player::SPRINT_SPEED);
    CHECK(player.flySpeed == game::Player::FLY_SPEED);
}

TEST_CASE("the box stands on the feet and the eyes are 1.7 m above them") {
    game::Player player;
    player.position = {1.0F, 0.0F, 5.0F};

    const scene::Aabb box = player.box();
    checkVector(box.min, {0.7F, 0.0F, 4.7F});
    checkVector(box.max, {1.3F, 1.8F, 5.3F});
    checkVector(player.eyePosition(), {1.0F, 1.7F, 5.0F});
}

TEST_CASE("walking forward covers 3 metres in one second, along the yaw") {
    const game::PlayerInput forward{.forward = true};

    SUBCASE("yaw 0 looks north: -Z") {
        game::Player player;
        runSteps(player, forward, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, 0.0F, -3.0F});
    }

    SUBCASE("yaw 90 looks east: +X") {
        game::Player player;
        runSteps(player, forward, YAW_EAST, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {3.0F, 0.0F, 0.0F});
    }

    SUBCASE("looking at the floor or at the sky changes nothing") {
        // Walking uses only the yaw: the pitch neither lifts the player nor slows it.
        game::Player player;
        runSteps(player, forward, YAW_NORTH, -60.0F, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, 0.0F, -3.0F});
    }
}

TEST_CASE("the side keys move at a right angle to the view, and opposite keys cancel") {
    SUBCASE("right of north is east") {
        game::Player player;
        runSteps(player, {.right = true}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {3.0F, 0.0F, 0.0F});
    }

    SUBCASE("left of north is west") {
        game::Player player;
        runSteps(player, {.left = true}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {-3.0F, 0.0F, 0.0F});
    }

    SUBCASE("backward is the opposite of forward") {
        game::Player player;
        runSteps(player, {.backward = true}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, 0.0F, 3.0F});
    }

    SUBCASE("forward and backward together: the player stands still") {
        game::Player player;
        const game::PlayerInput both{.forward = true, .backward = true};
        runSteps(player, both, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, 0.0F, 0.0F});
    }

    SUBCASE("no key: the player stands still") {
        game::Player player;
        runSteps(player, {}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, 0.0F, 0.0F});
    }
}

TEST_CASE("walking diagonally is not faster than walking straight") {
    game::Player player;
    const game::PlayerInput diagonal{.forward = true, .right = true};
    runSteps(player, diagonal, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);

    // Still 3 metres from the start, split evenly between north and east.
    CHECK(glm::length(player.position) == doctest::Approx(3.0F));
    CHECK(player.position.x == doctest::Approx(-player.position.z));
}

TEST_CASE("sprinting covers 5.5 metres in one second") {
    game::Player player;
    const game::PlayerInput sprint{.forward = true, .sprint = true};
    runSteps(player, sprint, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
    checkVector(player.position, {0.0F, 0.0F, -5.5F});
}

TEST_CASE("a walking player ignores the up and down keys") {
    game::Player player;
    runSteps(player, {.up = true}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
    checkVector(player.position, {0.0F, 0.0F, 0.0F});
    runSteps(player, {.down = true}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
    checkVector(player.position, {0.0F, 0.0F, 0.0F});
}

TEST_CASE("a wall stops the player") {
    // One closed cell: walls on the grid lines x = 0, x = 2, z = 0 and z = 2.
    const std::vector<scene::Aabb> obstacles = game::mazeColliders(game::Maze(1, 1));
    game::Player player;
    player.position = game::cellCenter(0, 0);

    // Two seconds towards the east wall, which is one metre away.
    runSteps(player, {.forward = true}, YAW_EAST, NO_PITCH, 2 * STEPS_PER_SECOND, obstacles);

    CHECK(player.position.x == doctest::Approx(game::CELL_SIZE - WALL_CONTACT_DISTANCE));
    CHECK(player.position.z == doctest::Approx(1.0F));
    CHECK(player.position.y == doctest::Approx(0.0F));
}

TEST_CASE("a player pressing into a wall slides along it and past the pillars") {
    // A maze of one column is a straight corridor along Z, whatever the seed: the east
    // wall is three segments in one line, with pillars at z = 0, 2, 4 and 6.
    const std::vector<scene::Aabb> obstacles = game::mazeColliders(game::generateMaze(1, 3, 0U));
    game::Player player;
    player.position = game::cellCenter(0, 0);

    // Looking south, "left" is east. Forward and left together push the player into the
    // east wall and along it at the same time.
    const game::PlayerInput intoTheWall{.forward = true, .left = true};
    runSteps(player, intoTheWall, YAW_SOUTH, NO_PITCH, 4 * STEPS_PER_SECOND, obstacles);

    // The east wall took the eastward part of the movement, the southward part went on:
    // past the pillars at z = 2 and z = 4, down to the south wall of the last cell.
    CHECK(player.position.x == doctest::Approx(game::CELL_SIZE - WALL_CONTACT_DISTANCE));
    CHECK(player.position.z == doctest::Approx(3.0F * game::CELL_SIZE - WALL_CONTACT_DISTANCE));
}

TEST_CASE("a player wandering through a closed maze never leaves it or enters a wall") {
    // A long walk of random key presses and random turns of the camera, in the fixed
    // steps of the game. The choices come from the seeded helper of the maze generator,
    // so the walk is the same on every run and on every system.
    constexpr int WIDTH = 6;
    constexpr int HEIGHT = 6;
    constexpr std::uint32_t MAZE_SEED = 5;
    constexpr std::uint32_t WALK_SEED = 17;
    constexpr int TURN_COUNT = 600;
    constexpr int STEPS_PER_TURN = 40;
    constexpr std::uint32_t FULL_TURN_DEGREES = 360;

    const std::vector<scene::Aabb> obstacles =
        game::mazeColliders(game::generateMaze(WIDTH, HEIGHT, MAZE_SEED));
    std::mt19937 generator(WALK_SEED);

    // True or false with equal chance.
    const auto coin = [&generator]() { return game::randomBelow(generator, 2U) == 1U; };

    // moveAndSlide lets a box sink into an obstacle by up to the contact tolerance. The
    // test therefore asks whether a box smaller by twice the tolerance on every side
    // overlaps anything: that happens only when the player is clearly inside a wall.
    const glm::vec3 margin{2.0F * scene::CONTACT_TOLERANCE};

    // The outer walls stand on the grid lines 0 and width * CELL_SIZE (or height).
    const float eastBorder = static_cast<float>(WIDTH) * game::CELL_SIZE;
    const float southBorder = static_cast<float>(HEIGHT) * game::CELL_SIZE;

    game::Player player;
    const glm::vec3 start = game::cellCenter(0, 0);
    player.position = start;
    float farthest = 0.0F;

    for (int turn = 0; turn < TURN_COUNT; ++turn) {
        // One field per line: the order of the calls to coin() is then plain to see. It
        // must not change, or the same seed would give a different walk.
        game::PlayerInput input;
        input.forward = coin();
        input.backward = coin();
        input.left = coin();
        input.right = coin();
        input.sprint = coin();
        const auto yaw = static_cast<float>(game::randomBelow(generator, FULL_TURN_DEGREES));

        // One flag for the whole turn keeps the number of assertions readable.
        bool insideObstacle = false;
        bool outsideMaze = false;
        for (int i = 0; i < STEPS_PER_TURN; ++i) {
            player.update(input, yaw, NO_PITCH, STEP_SECONDS, obstacles);

            const scene::Aabb box = player.box();
            const scene::Aabb inner{.min = box.min + margin, .max = box.max - margin};
            for (const scene::Aabb& obstacle : obstacles) {
                if (scene::overlaps(inner, obstacle)) {
                    insideObstacle = true;
                }
            }
            if (player.position.x < 0.0F || player.position.x > eastBorder ||
                player.position.z < 0.0F || player.position.z > southBorder) {
                outsideMaze = true;
            }
        }
        CAPTURE(turn);
        REQUIRE_FALSE(insideObstacle);
        REQUIRE_FALSE(outsideMaze);
        farthest = std::max(farthest, glm::distance(player.position, start));
    }

    // The player did walk: it got at least two cells away from where it started.
    CHECK(farthest > 2.0F * game::CELL_SIZE);
    // And it never left the floor.
    CHECK(player.position.y == doctest::Approx(0.0F));
}

TEST_CASE("noclip flies through walls") {
    // The same closed cell that stops a walking player.
    const std::vector<scene::Aabb> obstacles = game::mazeColliders(game::Maze(1, 1));
    game::Player player;
    player.position = game::cellCenter(0, 0);
    player.noclip = true;

    // One second east at the flight speed: far beyond the wall at x = 2.
    runSteps(player, {.forward = true}, YAW_EAST, NO_PITCH, STEPS_PER_SECOND, obstacles);

    checkVector(player.position, {1.0F + game::Player::FLY_SPEED, 0.0F, 1.0F});
}

TEST_CASE("noclip moves up and down, and forward follows the pitch") {
    SUBCASE("the up key climbs straight up") {
        game::Player player;
        player.noclip = true;
        runSteps(player, {.up = true}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, game::Player::FLY_SPEED, 0.0F});
    }

    SUBCASE("the down key sinks straight down, below the floor too") {
        game::Player player;
        player.noclip = true;
        runSteps(player, {.down = true}, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, -game::Player::FLY_SPEED, 0.0F});
    }

    SUBCASE("forward with the camera pitched up by 30 degrees climbs") {
        game::Player player;
        player.noclip = true;
        runSteps(player, {.forward = true}, YAW_NORTH, 30.0F, STEPS_PER_SECOND, NO_OBSTACLES);
        // sin(30 degrees) is one half: half of the distance is height.
        CHECK(player.position.y == doctest::Approx(game::Player::FLY_SPEED / 2.0F));
        CHECK(glm::length(player.position) == doctest::Approx(game::Player::FLY_SPEED));
    }

    SUBCASE("the sprint key does not change the flight speed") {
        game::Player player;
        player.noclip = true;
        const game::PlayerInput sprint{.forward = true, .sprint = true};
        runSteps(player, sprint, YAW_NORTH, NO_PITCH, STEPS_PER_SECOND, NO_OBSTACLES);
        checkVector(player.position, {0.0F, 0.0F, -game::Player::FLY_SPEED});
    }
}

TEST_CASE("switching noclip off brings the feet back to the floor") {
    game::Player player;
    player.position = {1.0F, 5.0F, 1.0F};
    player.noclip = false;

    // One step with no key held is enough.
    player.update({}, YAW_NORTH, NO_PITCH, STEP_SECONDS, NO_OBSTACLES);

    checkVector(player.position, {1.0F, 0.0F, 1.0F});
}
