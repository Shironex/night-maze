// Tests of the world-space layout of a maze: cells, wall segments, pillars and boxes.
// See docs/modules/game/maze-generator.md
#include "game/MazeLayout.hpp"

#include "game/MazeGenerator.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// How many segments stand at the given position and run along the given axis. The
// positions are sums and products of 0.5, 1 and 2, which a float holds exactly, so they
// can be compared with ==.
int countSegments(const std::vector<game::WallSegment>& segments, const glm::vec3& position,
                  game::WallAxis axis) {
    int count = 0;
    for (const game::WallSegment& segment : segments) {
        if (segment.position == position && segment.axis == axis) {
            ++count;
        }
    }
    return count;
}

int countPositions(const std::vector<glm::vec3>& positions, const glm::vec3& position) {
    int count = 0;
    for (const glm::vec3& candidate : positions) {
        if (candidate == position) {
            ++count;
        }
    }
    return count;
}

} // namespace

TEST_CASE("the layout constants are the agreed sizes in metres") {
    CHECK(game::CELL_SIZE == 2.0F);
    CHECK(game::WALL_LENGTH == 2.0F);
    CHECK(game::WALL_HEIGHT == 3.0F);
    CHECK(game::WALL_VISUAL_THICKNESS == 0.2F);
    CHECK(game::PILLAR_SIZE == 0.3F);
    // The collision box of a wall is as thick as a pillar, not as the visible wall.
    CHECK(game::WALL_COLLISION_THICKNESS == game::PILLAR_SIZE);
    CHECK(game::PILLAR_HEIGHT == 3.15F);
}

TEST_CASE("cellCenter is the middle of the cell at floor level") {
    checkVector(game::cellCenter(0, 0), {1.0F, 0.0F, 1.0F});
    checkVector(game::cellCenter(3, 2), {7.0F, 0.0F, 5.0F});
}

TEST_CASE("a single closed cell has four walls and four pillars") {
    const game::Maze maze(1, 1);
    const std::vector<game::WallSegment> segments = game::wallSegments(maze);
    const std::vector<glm::vec3> pillars = game::pillarPositions(maze);

    REQUIRE(segments.size() == 4U);
    // North and south walls run along X, west and east walls along Z.
    CHECK(countSegments(segments, {1.0F, 0.0F, 0.0F}, game::WallAxis::AlongX) == 1);
    CHECK(countSegments(segments, {1.0F, 0.0F, 2.0F}, game::WallAxis::AlongX) == 1);
    CHECK(countSegments(segments, {0.0F, 0.0F, 1.0F}, game::WallAxis::AlongZ) == 1);
    CHECK(countSegments(segments, {2.0F, 0.0F, 1.0F}, game::WallAxis::AlongZ) == 1);

    REQUIRE(pillars.size() == 4U);
    CHECK(countPositions(pillars, {0.0F, 0.0F, 0.0F}) == 1);
    CHECK(countPositions(pillars, {2.0F, 0.0F, 0.0F}) == 1);
    CHECK(countPositions(pillars, {0.0F, 0.0F, 2.0F}) == 1);
    CHECK(countPositions(pillars, {2.0F, 0.0F, 2.0F}) == 1);
}

TEST_CASE("the wall between two cells appears once") {
    // Two cells side by side, all walls present: 3 walls along Z and 4 along X.
    game::Maze maze(2, 1);
    const glm::vec3 sharedWall{2.0F, 0.0F, 1.0F};

    SUBCASE("closed: seven segments, the shared one among them once") {
        const std::vector<game::WallSegment> segments = game::wallSegments(maze);
        CHECK(segments.size() == 7U);
        CHECK(countSegments(segments, sharedWall, game::WallAxis::AlongZ) == 1);
        CHECK(game::pillarPositions(maze).size() == 6U);
    }

    SUBCASE("with a passage: six segments, the shared one is gone") {
        maze.removeWall(0, 0, game::Direction::East);
        const std::vector<game::WallSegment> segments = game::wallSegments(maze);
        CHECK(segments.size() == 6U);
        CHECK(countSegments(segments, sharedWall, game::WallAxis::AlongZ) == 0);
        // The two corners at the ends of the removed wall still hold border walls.
        CHECK(game::pillarPositions(maze).size() == 6U);
    }
}

TEST_CASE("a grid corner without any wall gets no pillar") {
    // Four cells with all inner walls removed: one open room of 4 by 4 metres. Nothing
    // ends at the corner in its middle.
    game::Maze maze(2, 2);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(0, 0, game::Direction::South);
    maze.removeWall(1, 1, game::Direction::West);
    maze.removeWall(1, 1, game::Direction::North);

    const std::vector<glm::vec3> pillars = game::pillarPositions(maze);
    CHECK(game::wallSegments(maze).size() == 8U);
    CHECK(pillars.size() == 8U);
    CHECK(countPositions(pillars, {2.0F, 0.0F, 2.0F}) == 0);

    // One inner wall back, and the middle corner needs its pillar again.
    game::Maze withWall(2, 2);
    withWall.removeWall(0, 0, game::Direction::East);
    withWall.removeWall(0, 0, game::Direction::South);
    withWall.removeWall(1, 1, game::Direction::West);
    CHECK(countPositions(game::pillarPositions(withWall), {2.0F, 0.0F, 2.0F}) == 1);
}

TEST_CASE("a generated maze has the expected number of walls and pillars") {
    constexpr int WIDTH = 9;
    constexpr int HEIGHT = 6;
    constexpr std::uint32_t SEED_COUNT = 10;

    // A grid of 9 by 6 cells has 9 * 7 = 63 edges along X and 6 * 10 = 60 along Z.
    // A perfect maze turns 9 * 6 - 1 = 53 of them into passages: 63 + 60 - 53 walls.
    constexpr std::size_t EXPECTED_SEGMENTS = 70;
    // In a perfect maze a wall ends at every grid corner (four passages around one
    // corner would be a loop), so there are 10 * 7 pillars. The two numbers are equal
    // for every size: w(h + 1) + h(w + 1) - (wh - 1) is the same as (w + 1)(h + 1).
    constexpr std::size_t EXPECTED_PILLARS = 70;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::Maze maze = game::generateMaze(WIDTH, HEIGHT, seed);
        CHECK(game::wallSegments(maze).size() == EXPECTED_SEGMENTS);
        CHECK(game::pillarPositions(maze).size() == EXPECTED_PILLARS);
        CHECK(game::mazeColliders(maze).size() == EXPECTED_SEGMENTS + EXPECTED_PILLARS);
    }
}

TEST_CASE("wallBox is 2 m long, 3 m high and 0.3 m thick, standing on the floor") {
    SUBCASE("a wall along X is long in x and thin in z") {
        const scene::Aabb box =
            game::wallBox({.position = {3.0F, 0.0F, 4.0F}, .axis = game::WallAxis::AlongX});
        checkVector(box.min, {2.0F, 0.0F, 3.85F});
        checkVector(box.max, {4.0F, 3.0F, 4.15F});
    }

    SUBCASE("a wall along Z is thin in x and long in z") {
        const scene::Aabb box =
            game::wallBox({.position = {4.0F, 0.0F, 3.0F}, .axis = game::WallAxis::AlongZ});
        checkVector(box.min, {3.85F, 0.0F, 2.0F});
        checkVector(box.max, {4.15F, 3.0F, 4.0F});
    }
}

TEST_CASE("pillarBox is 0.3 m square and 3.15 m high, standing on the floor") {
    const scene::Aabb box = game::pillarBox({2.0F, 0.0F, 6.0F});
    checkVector(box.min, {1.85F, 0.0F, 5.85F});
    checkVector(box.max, {2.15F, 3.15F, 6.15F});
}

TEST_CASE("mazeColliders lists the wall boxes first and the pillar boxes after them") {
    const game::Maze maze(1, 1);
    const std::vector<game::WallSegment> segments = game::wallSegments(maze);
    const std::vector<glm::vec3> pillars = game::pillarPositions(maze);
    const std::vector<scene::Aabb> boxes = game::mazeColliders(maze);

    REQUIRE(boxes.size() == segments.size() + pillars.size());
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const scene::Aabb expected = game::wallBox(segments[i]);
        checkVector(boxes[i].min, expected.min);
        checkVector(boxes[i].max, expected.max);
    }
    for (std::size_t i = 0; i < pillars.size(); ++i) {
        const scene::Aabb expected = game::pillarBox(pillars[i]);
        checkVector(boxes[segments.size() + i].min, expected.min);
        checkVector(boxes[segments.size() + i].max, expected.max);
    }
}

TEST_CASE("the colliders of a closed cell keep a box inside it") {
    // The layout and the collision code together: a box in the middle of a closed cell
    // is pushed east in small steps for much longer than the cell is wide.
    const std::vector<scene::Aabb> obstacles = game::mazeColliders(game::Maze(1, 1));
    constexpr glm::vec3 HALF_EXTENTS{0.3F, 0.9F, 0.3F};
    constexpr int STEP_COUNT = 400;
    const glm::vec3 step{0.025F, 0.0F, 0.0F};

    glm::vec3 position = game::cellCenter(0, 0) + glm::vec3{0.0F, 0.9F, 0.0F};
    for (int i = 0; i < STEP_COUNT; ++i) {
        const scene::Aabb box = scene::Aabb::fromCenter(position, HALF_EXTENTS);
        position += scene::moveAndSlide(box, step, obstacles);
    }

    // The inner face of the east wall box is half of its thickness before x = 2, and the
    // centre of the box stays half a box before that face.
    const float innerFace = game::CELL_SIZE - game::WALL_COLLISION_THICKNESS / 2.0F;
    CHECK(position.x == doctest::Approx(innerFace - HALF_EXTENTS.x));
    CHECK(position.z == doctest::Approx(1.0F));
}

TEST_CASE("a box wandering through a generated maze never ends up inside a wall") {
    // A long walk with many changes of direction, in the fixed steps the game will use.
    // The directions come from the same seeded helper as the maze, so the walk is the
    // same on every run and on every system.
    constexpr int WIDTH = 8;
    constexpr int HEIGHT = 8;
    constexpr std::uint32_t MAZE_SEED = 3;
    constexpr std::uint32_t WALK_SEED = 11;
    constexpr int TURN_COUNT = 400;
    constexpr int STEPS_PER_TURN = 60;
    constexpr float STEP_LENGTH = 0.025F;
    constexpr glm::vec3 HALF_EXTENTS{0.3F, 0.9F, 0.3F};

    // Sixteen directions around the compass: the eight below and their opposites. Some
    // are almost parallel to the walls, which is the hard case for sliding.
    constexpr std::array<glm::vec2, 8> DIRECTIONS = {
        glm::vec2{1.0F, 0.0F},  glm::vec2{0.0F, 1.0F},  glm::vec2{0.7F, 0.7F},
        glm::vec2{0.7F, -0.7F}, glm::vec2{1.0F, 0.02F}, glm::vec2{0.02F, 1.0F},
        glm::vec2{0.9F, 0.4F},  glm::vec2{-0.4F, 0.9F},
    };

    const std::vector<scene::Aabb> obstacles =
        game::mazeColliders(game::generateMaze(WIDTH, HEIGHT, MAZE_SEED));
    std::mt19937 generator(WALK_SEED);

    // A box smaller by twice the contact tolerance on every side. moveAndSlide lets the
    // real box sink into an obstacle by up to the tolerance, so this inner box overlaps
    // one only when the real box is clearly deeper than that. The factor of two leaves
    // room for float rounding, which differs between processors by the last digit.
    const glm::vec3 innerHalfExtents = HALF_EXTENTS - glm::vec3{2.0F * scene::CONTACT_TOLERANCE};

    const glm::vec3 start = game::cellCenter(0, 0) + glm::vec3{0.0F, 0.9F, 0.0F};
    glm::vec3 position = start;
    float farthest = 0.0F;

    for (int turn = 0; turn < TURN_COUNT; ++turn) {
        const glm::vec2 direction = DIRECTIONS[game::randomBelow(generator, 8U)];
        const float sign = game::randomBelow(generator, 2U) == 0U ? 1.0F : -1.0F;
        const glm::vec3 step{direction.x * sign * STEP_LENGTH, 0.0F,
                             direction.y * sign * STEP_LENGTH};

        // One flag for the whole turn keeps the number of assertions readable.
        bool insideObstacle = false;
        for (int i = 0; i < STEPS_PER_TURN; ++i) {
            const scene::Aabb box = scene::Aabb::fromCenter(position, HALF_EXTENTS);
            position += scene::moveAndSlide(box, step, obstacles);

            const scene::Aabb inner = scene::Aabb::fromCenter(position, innerHalfExtents);
            for (const scene::Aabb& obstacle : obstacles) {
                if (scene::overlaps(inner, obstacle)) {
                    insideObstacle = true;
                }
            }
        }
        CAPTURE(turn);
        REQUIRE_FALSE(insideObstacle);
        farthest = std::max(farthest, glm::distance(position, start));
    }

    // The box did walk: it got at least two cells away from where it started.
    CHECK(farthest > 2.0F * game::CELL_SIZE);
}

TEST_CASE("a box that hugs a wall slides past the pillars in the middle of it") {
    // The collision box of a wall is as thick as a pillar (0.3 m), so the faces of the
    // wall boxes and of the pillar boxes lie in one plane. A box sliding along a wall with
    // its side on the wall only touches the pillar at the next grid corner, and touching
    // does not stop movement. With wall boxes as thin as the visible wall (0.2 m) every
    // pillar stuck out 5 cm and stopped the box.
    //
    // A maze of one column is a straight corridor along Z, whatever the seed: the east
    // wall is three segments in one line, with pillars at z = 0, 2, 4 and 6.
    const std::vector<scene::Aabb> obstacles = game::mazeColliders(game::generateMaze(1, 3, 0U));
    constexpr glm::vec3 HALF_EXTENTS{0.3F, 0.9F, 0.3F};
    constexpr int STEP_COUNT = 200;

    // Where the centre of the box is when its east side lies on the east wall (x = 1.85),
    // and when its south side lies on the south wall of the last cell (z = 5.85).
    const float againstEastWall =
        game::CELL_SIZE - game::WALL_COLLISION_THICKNESS / 2.0F - HALF_EXTENTS.x;
    const float againstSouthWall =
        3.0F * game::CELL_SIZE - game::WALL_COLLISION_THICKNESS / 2.0F - HALF_EXTENTS.z;

    SUBCASE("starting on the wall and walking straight south") {
        const glm::vec3 step{0.0F, 0.0F, 0.025F};
        glm::vec3 position{againstEastWall, 0.9F, 1.0F};
        for (int i = 0; i < STEP_COUNT; ++i) {
            const scene::Aabb box = scene::Aabb::fromCenter(position, HALF_EXTENTS);
            position += scene::moveAndSlide(box, step, obstacles);
        }
        // Stopped only by the south wall of the last cell, two pillars further.
        CHECK(position.x == doctest::Approx(againstEastWall));
        CHECK(position.z == doctest::Approx(againstSouthWall));
    }

    SUBCASE("pressing into the wall all the way: diagonal movement south-east") {
        const glm::vec3 step{0.02F, 0.0F, 0.02F};
        glm::vec3 position = game::cellCenter(0, 0) + glm::vec3{0.0F, 0.9F, 0.0F};
        for (int i = 0; i < 2 * STEP_COUNT; ++i) {
            const scene::Aabb box = scene::Aabb::fromCenter(position, HALF_EXTENTS);
            position += scene::moveAndSlide(box, step, obstacles);
        }
        CHECK(position.x == doctest::Approx(againstEastWall));
        CHECK(position.z == doctest::Approx(againstSouthWall));
    }
}
