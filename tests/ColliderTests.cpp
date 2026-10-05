// Tests of scene::Aabb, scene::overlaps and scene::moveAndSlide.
// See docs/modules/scene/collision.md
#include "scene/Collider.hpp"

#include <doctest/doctest.h>

#include <array>
#include <vector>

namespace {

// The moving box of most tests: 0.6 m wide and deep, 1.8 m high, standing on the floor.
constexpr glm::vec3 MOVER_HALF_EXTENTS{0.3F, 0.9F, 0.3F};
constexpr float MOVER_CENTER_HEIGHT = 0.9F;

scene::Aabb moverAt(float x, float z) {
    return scene::Aabb::fromCenter({x, MOVER_CENTER_HEIGHT, z}, MOVER_HALF_EXTENTS);
}

// A wall 0.2 m thick whose face towards the mover is the plane x = 1. It is long along Z.
constexpr scene::Aabb WALL_AT_X{.min = {1.0F, 0.0F, -5.0F}, .max = {1.2F, 3.0F, 5.0F}};
// The same kind of wall turned by 90 degrees: its near face is the plane z = 1.
constexpr scene::Aabb WALL_AT_Z{.min = {-5.0F, 0.0F, 1.0F}, .max = {5.0F, 3.0F, 1.2F}};

// Free space between the mover standing at x = 0 (or z = 0) and those walls.
constexpr float GAP_TO_WALL = 0.7F;

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

} // namespace

TEST_CASE("Aabb::fromCenter puts the corners half an extent away from the centre") {
    const scene::Aabb box = scene::Aabb::fromCenter({1.0F, 2.0F, 3.0F}, {0.5F, 1.0F, 2.0F});

    checkVector(box.min, {0.5F, 1.0F, 1.0F});
    checkVector(box.max, {1.5F, 3.0F, 5.0F});
}

TEST_CASE("overlaps is true only when the boxes share volume") {
    const scene::Aabb box{.min = {0.0F, 0.0F, 0.0F}, .max = {1.0F, 1.0F, 1.0F}};

    SUBCASE("boxes that cross each other overlap") {
        const scene::Aabb other{.min = {0.5F, 0.5F, 0.5F}, .max = {2.0F, 2.0F, 2.0F}};
        CHECK(scene::overlaps(box, other));
        CHECK(scene::overlaps(other, box));
    }

    SUBCASE("a box inside another one overlaps it") {
        const scene::Aabb inner{.min = {0.25F, 0.25F, 0.25F}, .max = {0.75F, 0.75F, 0.75F}};
        CHECK(scene::overlaps(box, inner));
        CHECK(scene::overlaps(inner, box));
    }

    SUBCASE("a gap on one axis is enough to keep the boxes apart") {
        // The intervals overlap on x and y, but not on z.
        const scene::Aabb other{.min = {0.5F, 0.5F, 3.0F}, .max = {2.0F, 2.0F, 4.0F}};
        CHECK_FALSE(scene::overlaps(box, other));
        CHECK_FALSE(scene::overlaps(other, box));
    }

    SUBCASE("touching is not overlapping") {
        // A shared face, a shared edge and a shared corner.
        const scene::Aabb face{.min = {1.0F, 0.0F, 0.0F}, .max = {2.0F, 1.0F, 1.0F}};
        const scene::Aabb edge{.min = {1.0F, 1.0F, 0.0F}, .max = {2.0F, 2.0F, 1.0F}};
        const scene::Aabb corner{.min = {1.0F, 1.0F, 1.0F}, .max = {2.0F, 2.0F, 2.0F}};
        CHECK_FALSE(scene::overlaps(box, face));
        CHECK_FALSE(scene::overlaps(box, edge));
        CHECK_FALSE(scene::overlaps(box, corner));
    }
}

TEST_CASE("moveAndSlide allows the whole displacement when nothing is in the way") {
    const glm::vec3 displacement{0.4F, 0.2F, -0.3F};

    SUBCASE("without any obstacle") {
        checkVector(scene::moveAndSlide(moverAt(0.0F, 0.0F), displacement, {}), displacement);
    }

    SUBCASE("with an obstacle that the movement does not reach") {
        const std::array<scene::Aabb, 1> obstacles = {WALL_AT_X};
        checkVector(scene::moveAndSlide(moverAt(0.0F, 0.0F), displacement, obstacles),
                    displacement);
    }
}

TEST_CASE("moveAndSlide stops a box that runs straight into a wall") {
    const std::array<scene::Aabb, 1> obstacles = {WALL_AT_X};

    SUBCASE("the box stops at the face of the wall") {
        const glm::vec3 allowed =
            scene::moveAndSlide(moverAt(0.0F, 0.0F), {2.0F, 0.0F, 0.0F}, obstacles);
        checkVector(allowed, {GAP_TO_WALL, 0.0F, 0.0F});
    }

    SUBCASE("the same from the other side, moving in the negative direction") {
        // The far face of the wall is x = 1.2, the mover at x = 3 starts at 2.7.
        const glm::vec3 allowed =
            scene::moveAndSlide(moverAt(3.0F, 0.0F), {-4.0F, 0.0F, 0.0F}, obstacles);
        checkVector(allowed, {-1.5F, 0.0F, 0.0F});
    }

    SUBCASE("a step much longer than the wall is thick does not jump over it") {
        const glm::vec3 allowed =
            scene::moveAndSlide(moverAt(0.0F, 0.0F), {50.0F, 0.0F, 0.0F}, obstacles);
        checkVector(allowed, {GAP_TO_WALL, 0.0F, 0.0F});
    }

    SUBCASE("a box that already rests against the wall cannot move into it") {
        const glm::vec3 allowed =
            scene::moveAndSlide(moverAt(GAP_TO_WALL, 0.0F), {0.5F, 0.0F, 0.0F}, obstacles);
        checkVector(allowed, {0.0F, 0.0F, 0.0F});
    }
}

TEST_CASE("moveAndSlide lets a box slide along a wall") {
    const std::array<scene::Aabb, 1> obstacles = {WALL_AT_X};

    SUBCASE("a diagonal step loses only the part that points into the wall") {
        const glm::vec3 allowed =
            scene::moveAndSlide(moverAt(0.0F, 0.0F), {1.0F, 0.0F, 0.5F}, obstacles);
        checkVector(allowed, {GAP_TO_WALL, 0.0F, 0.5F});
    }

    SUBCASE("a box that touches the wall moves freely along it") {
        const glm::vec3 allowed =
            scene::moveAndSlide(moverAt(GAP_TO_WALL, 0.0F), {0.0F, 0.0F, -0.8F}, obstacles);
        checkVector(allowed, {0.0F, 0.0F, -0.8F});
    }

    SUBCASE("a box that touches the wall is free to step away from it") {
        const glm::vec3 allowed =
            scene::moveAndSlide(moverAt(GAP_TO_WALL, 0.0F), {-0.4F, 0.0F, 0.0F}, obstacles);
        checkVector(allowed, {-0.4F, 0.0F, 0.0F});
    }
}

TEST_CASE("moveAndSlide stops a box in a corner on both axes") {
    const std::array<scene::Aabb, 2> obstacles = {WALL_AT_X, WALL_AT_Z};

    const glm::vec3 allowed =
        scene::moveAndSlide(moverAt(0.0F, 0.0F), {3.0F, 0.0F, 3.0F}, obstacles);
    checkVector(allowed, {GAP_TO_WALL, 0.0F, GAP_TO_WALL});

    // Once in the corner, pushing further into it changes nothing.
    const glm::vec3 pushedAgain =
        scene::moveAndSlide(moverAt(GAP_TO_WALL, GAP_TO_WALL), {1.0F, 0.0F, 1.0F}, obstacles);
    checkVector(pushedAgain, {0.0F, 0.0F, 0.0F});
}

TEST_CASE("moveAndSlide returns zero for a zero displacement") {
    const std::array<scene::Aabb, 2> obstacles = {WALL_AT_X, WALL_AT_Z};
    const glm::vec3 none{0.0F, 0.0F, 0.0F};

    checkVector(scene::moveAndSlide(moverAt(0.0F, 0.0F), none, obstacles), none);
    // Also for a box in contact with both walls.
    checkVector(scene::moveAndSlide(moverAt(GAP_TO_WALL, GAP_TO_WALL), none, obstacles), none);
}

TEST_CASE("moveAndSlide handles the vertical axis too") {
    // A floor slab whose top is y = 0. The mover hangs 1 m above it and falls 5 m.
    const std::array<scene::Aabb, 1> obstacles = {
        scene::Aabb{.min = {-10.0F, -1.0F, -10.0F}, .max = {10.0F, 0.0F, 10.0F}}};
    const scene::Aabb mover = scene::Aabb::fromCenter({0.0F, 1.9F, 0.0F}, MOVER_HALF_EXTENTS);

    const glm::vec3 allowed = scene::moveAndSlide(mover, {0.5F, -5.0F, 0.0F}, obstacles);
    checkVector(allowed, {0.5F, -1.0F, 0.0F});

    // Standing on the floor, the box walks over it: the floor is below, not in the way.
    const glm::vec3 walking =
        scene::moveAndSlide(moverAt(0.0F, 0.0F), {0.5F, 0.0F, 0.5F}, obstacles);
    checkVector(walking, {0.5F, 0.0F, 0.5F});
}

TEST_CASE("moveAndSlide does not hold a box that starts inside an obstacle") {
    const std::array<scene::Aabb, 1> obstacles = {WALL_AT_X};
    // The centre of the mover is in the middle of the wall.
    const glm::vec3 allowed =
        scene::moveAndSlide(moverAt(1.1F, 0.0F), {-2.0F, 0.0F, 0.0F}, obstacles);
    checkVector(allowed, {-2.0F, 0.0F, 0.0F});
}

TEST_CASE("many small steps along a wall never stick and never sink into it") {
    // This is how the game will use the function: every fixed step the position gets
    // the allowed displacement added and the box is built again from the position. The
    // numbers are deliberately awkward (far from the origin, a shallow angle), so that
    // the float rounding of "position += allowed" is not hidden by round values.
    const scene::Aabb wall{.min = {37.3F, 0.0F, -100.0F}, .max = {37.5F, 3.0F, 100.0F}};
    const std::array<scene::Aabb, 1> obstacles = {wall};
    const glm::vec3 step{0.0004F, 0.0F, 0.0251F};
    constexpr int STEP_COUNT = 2000;

    glm::vec3 position{36.9F, MOVER_CENTER_HEIGHT, -20.0F};
    for (int i = 0; i < STEP_COUNT; ++i) {
        const scene::Aabb box = scene::Aabb::fromCenter(position, MOVER_HALF_EXTENTS);
        const glm::vec3 allowed = scene::moveAndSlide(box, step, obstacles);

        // The movement along the wall is never taken away.
        REQUIRE(allowed.z == step.z);
        position += allowed;

        // The box never gets deeper into the wall than the contact tolerance.
        const float depth = position.x + MOVER_HALF_EXTENTS.x - wall.min.x;
        REQUIRE(depth <= scene::CONTACT_TOLERANCE);
    }

    // The box reached the wall (it did not stop early) and rests against it.
    CHECK(position.x + MOVER_HALF_EXTENTS.x == doctest::Approx(wall.min.x));
}

TEST_CASE("a box slides across the joint of two wall segments") {
    // Two segments in one line, like two neighbouring maze walls. The second starts
    // exactly where the first ends.
    const std::vector<scene::Aabb> obstacles = {
        scene::Aabb{.min = {1.0F, 0.0F, 0.0F}, .max = {1.2F, 3.0F, 2.0F}},
        scene::Aabb{.min = {1.0F, 0.0F, 2.0F}, .max = {1.2F, 3.0F, 4.0F}},
    };
    const glm::vec3 step{0.02F, 0.0F, 0.03F};
    constexpr int STEP_COUNT = 100;

    glm::vec3 position{0.5F, MOVER_CENTER_HEIGHT, 0.5F};
    for (int i = 0; i < STEP_COUNT; ++i) {
        const scene::Aabb box = scene::Aabb::fromCenter(position, MOVER_HALF_EXTENTS);
        position += scene::moveAndSlide(box, step, obstacles);
    }

    CHECK(position.x == doctest::Approx(GAP_TO_WALL));
    CHECK(position.z == doctest::Approx(0.5F + step.z * static_cast<float>(STEP_COUNT)));
}

TEST_CASE("documented limit: a step much longer than the boxes can go around an obstacle") {
    // A small post stands exactly on the straight line of the movement. The box does not
    // travel along that line: it moves 2 m along x first (the post is not in that
    // corridor) and then 2 m along z (the post is behind by then). The step is several
    // times longer than the boxes, which never happens with the fixed time step.
    const std::array<scene::Aabb, 1> obstacles = {
        scene::Aabb{.min = {0.9F, 0.0F, 0.9F}, .max = {1.1F, 3.0F, 1.1F}}};

    const glm::vec3 allowed =
        scene::moveAndSlide(moverAt(0.0F, 0.0F), {2.0F, 0.0F, 2.0F}, obstacles);
    checkVector(allowed, {2.0F, 0.0F, 2.0F});

    // The same distance in short steps does meet the post: the box has to slide around
    // it, which costs a part of the way along z.
    constexpr int STEP_COUNT = 80;
    const glm::vec3 step{0.025F, 0.0F, 0.025F};
    glm::vec3 position{0.0F, MOVER_CENTER_HEIGHT, 0.0F};
    for (int i = 0; i < STEP_COUNT; ++i) {
        const scene::Aabb box = scene::Aabb::fromCenter(position, MOVER_HALF_EXTENTS);
        position += scene::moveAndSlide(box, step, obstacles);
    }
    CHECK_FALSE(
        scene::overlaps(scene::Aabb::fromCenter(position, MOVER_HALF_EXTENTS), obstacles.front()));
    CHECK(position.z < 1.5F);
}
