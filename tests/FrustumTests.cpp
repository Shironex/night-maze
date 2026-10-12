// Tests of scene::Frustum: the six planes of a camera or a light, the test that tells
// whether a box can be seen, and the box of a box that a matrix has moved.
#include "scene/Frustum.hpp"

#include "scene/Camera.hpp"
#include "scene/Collider.hpp"
#include "scene/LightSpace.hpp"
#include "scene/Transform.hpp"

#include <doctest/doctest.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <random>

namespace {

// The camera of most tests stands in the origin of the world and looks along -Z, with
// the defaults of scene::Camera: 60 degrees from the top to the bottom of the picture,
// the near plane at 0.1 m and the far plane at 100 m. The picture is 16:9.
constexpr float ASPECT_RATIO = 1280.0F / 720.0F;
constexpr glm::vec3 ORIGIN{0.0F};

glm::mat4 cameraMatrix(const scene::Camera& camera, const glm::vec3& eye) {
    return camera.projectionMatrix(ASPECT_RATIO) * camera.viewMatrix(eye);
}

scene::Frustum defaultFrustum() {
    return scene::frustumFromMatrix(cameraMatrix(scene::Camera{}, ORIGIN));
}

// A cube with sides of 1 m around center.
scene::Aabb cubeAt(const glm::vec3& center) {
    return scene::Aabb::fromCenter(center, glm::vec3{0.5F});
}

// The box of the orthographic tests: 10 m wide and high around the Z axis, from z = 0
// to z = -20. With an identity view matrix glm::ortho looks along -Z like a camera.
constexpr float ORTHO_HALF_SIZE = 5.0F;
constexpr float ORTHO_DEPTH = 20.0F;

scene::Frustum orthographicFrustum() {
    return scene::frustumFromMatrix(glm::ortho(-ORTHO_HALF_SIZE, ORTHO_HALF_SIZE, -ORTHO_HALF_SIZE,
                                               ORTHO_HALF_SIZE, 0.0F, ORTHO_DEPTH));
}

// How close two numbers have to be to count as the same.
constexpr float TOLERANCE = 0.0001F;

// The places of the planes in Frustum::planes.
constexpr std::size_t LEFT_PLANE = 0;
constexpr std::size_t RIGHT_PLANE = 1;
constexpr std::size_t BOTTOM_PLANE = 2;
constexpr std::size_t TOP_PLANE = 3;
constexpr std::size_t NEAR_PLANE = 4;
constexpr std::size_t FAR_PLANE = 5;

// What the graphics card does with a point: the matrix, and then the test of the clip
// coordinates against w. This is the definition of "inside the frustum" the planes
// have to agree with.
bool insideClipVolume(const glm::mat4& viewProjection, const glm::vec3& point) {
    const glm::vec4 clip = viewProjection * glm::vec4{point, 1.0F};
    return -clip.w <= clip.x && clip.x <= clip.w && -clip.w <= clip.y && clip.y <= clip.w &&
           -clip.w <= clip.z && clip.z <= clip.w;
}

// A number from 0 to 1 out of the generator. Written by hand and not taken from
// std::uniform_real_distribution, which may give other numbers with another standard
// library: the boxes of the test below are then the same on every system.
float unitRandom(std::mt19937& generator) {
    constexpr float RANGE = 4294967296.0F; // 2 to the power of 32
    return static_cast<float>(generator()) / RANGE;
}

float randomBetween(std::mt19937& generator, float low, float high) {
    return low + (high - low) * unitRandom(generator);
}

} // namespace

TEST_CASE("the planes of a frustum have unit normals that point inwards") {
    const scene::Frustum frustum = defaultFrustum();
    // A point in the middle of what the camera sees.
    const glm::vec3 inside{0.0F, 0.0F, -10.0F};
    for (const scene::Plane& plane : frustum.planes) {
        CHECK(glm::length(plane.normal) == doctest::Approx(1.0F));
        CHECK(scene::signedDistance(plane, inside) > 0.0F);
    }
    // The near plane is 0.1 m in front of the eye and the far plane 100 m, so the point
    // 10 m ahead is 9.9 m behind the one and 90 m in front of the other.
    CHECK(scene::signedDistance(frustum.planes[NEAR_PLANE], inside) == doctest::Approx(9.9F));
    CHECK(scene::signedDistance(frustum.planes[FAR_PLANE], inside) == doctest::Approx(90.0F));
    // The camera looks along -Z, so the near plane faces -Z and the far plane +Z.
    CHECK(frustum.planes[NEAR_PLANE].normal.z == doctest::Approx(-1.0F));
    CHECK(frustum.planes[FAR_PLANE].normal.z == doctest::Approx(1.0F));
    // The left plane faces right and the right plane left, the bottom one up and the
    // top one down.
    CHECK(frustum.planes[LEFT_PLANE].normal.x > 0.0F);
    CHECK(frustum.planes[RIGHT_PLANE].normal.x < 0.0F);
    CHECK(frustum.planes[BOTTOM_PLANE].normal.y > 0.0F);
    CHECK(frustum.planes[TOP_PLANE].normal.y < 0.0F);
}

TEST_CASE("a box in front of the camera can be seen") {
    const scene::Frustum frustum = defaultFrustum();
    CHECK(scene::canBeSeen(frustum, cubeAt({0.0F, 0.0F, -10.0F})));
    // Near an edge of the picture, but fully inside. At 10 m the picture is about
    // 11.5 m high (2 * 10 * tan 30 degrees) and 20.5 m wide.
    CHECK(scene::canBeSeen(frustum, cubeAt({9.0F, 4.5F, -10.0F})));
}

TEST_CASE("a box outside one of the six planes cannot be seen") {
    const scene::Frustum frustum = defaultFrustum();
    // Each of these is outside exactly the plane it is named after and inside the
    // other five.
    const scene::Aabb leftOf = cubeAt({-30.0F, 0.0F, -10.0F});
    const scene::Aabb rightOf = cubeAt({30.0F, 0.0F, -10.0F});
    const scene::Aabb below = cubeAt({0.0F, -20.0F, -10.0F});
    const scene::Aabb above = cubeAt({0.0F, 20.0F, -10.0F});
    // In front of the eye, but nearer than the near plane (0.1 m): from 2 cm to 6 cm.
    const scene::Aabb tooNear{.min = {-0.01F, -0.01F, -0.06F}, .max = {0.01F, 0.01F, -0.02F}};
    const scene::Aabb tooFar = cubeAt({0.0F, 0.0F, -150.0F});

    CHECK_FALSE(scene::canBeSeen(frustum, leftOf));
    CHECK_FALSE(scene::canBeSeen(frustum, rightOf));
    CHECK_FALSE(scene::canBeSeen(frustum, below));
    CHECK_FALSE(scene::canBeSeen(frustum, above));
    CHECK_FALSE(scene::canBeSeen(frustum, tooNear));
    CHECK_FALSE(scene::canBeSeen(frustum, tooFar));

    // The plane that rejects each one is the one it is named after: its farthest corner
    // is behind that plane and in front of the other five.
    const std::array<scene::Aabb, scene::FRUSTUM_PLANE_COUNT> boxes{leftOf, rightOf, below,
                                                                    above,  tooNear, tooFar};
    for (std::size_t rejecting = 0; rejecting < boxes.size(); ++rejecting) {
        const glm::vec3 center = (boxes[rejecting].min + boxes[rejecting].max) * 0.5F;
        for (std::size_t plane = 0; plane < frustum.planes.size(); ++plane) {
            const float distance = scene::signedDistance(frustum.planes[plane], center);
            if (plane == rejecting) {
                CHECK(distance < 0.0F);
            } else {
                CHECK(distance > 0.0F);
            }
        }
    }
}

TEST_CASE("a box that crosses a plane can be seen") {
    const scene::Frustum frustum = defaultFrustum();
    // At 10 m the left edge of the picture is about 10.3 m to the left. A box from
    // x = -14 to x = -8 has one part in the picture and one part outside.
    const scene::Aabb acrossLeft{.min = {-14.0F, -1.0F, -11.0F}, .max = {-8.0F, 1.0F, -9.0F}};
    CHECK(scene::canBeSeen(frustum, acrossLeft));
    // Through the near plane: it starts behind the eye and ends in front of it.
    const scene::Aabb acrossNear{.min = {-0.5F, -0.5F, -2.0F}, .max = {0.5F, 0.5F, 2.0F}};
    CHECK(scene::canBeSeen(frustum, acrossNear));
    // Through the far plane.
    const scene::Aabb acrossFar{.min = {-1.0F, -1.0F, -120.0F}, .max = {1.0F, 1.0F, -90.0F}};
    CHECK(scene::canBeSeen(frustum, acrossFar));
}

TEST_CASE("a box behind the camera cannot be seen") {
    const scene::Frustum frustum = defaultFrustum();
    // The camera looks along -Z, so +Z is behind it. A perspective matrix gives such
    // a point a negative w, and the planes have to reject it all the same.
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({0.0F, 0.0F, 10.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({3.0F, 1.0F, 40.0F})));
    // Right behind the eye, where a point mirrored through the eye would be in the
    // middle of the picture.
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({0.0F, 0.0F, 1.0F})));
}

TEST_CASE("a box that holds the whole frustum can be seen") {
    const scene::Frustum frustum = defaultFrustum();
    // None of its corners is inside the frustum, and it is seen from the inside.
    const scene::Aabb huge = scene::Aabb::fromCenter(ORIGIN, glm::vec3{1000.0F});
    CHECK(scene::canBeSeen(frustum, huge));
    // The same for a long thin box that runs through the picture from far left to far
    // right: both ends are outside, the middle is inside.
    const scene::Aabb beam{.min = {-500.0F, -0.1F, -10.1F}, .max = {500.0F, 0.1F, -9.9F}};
    CHECK(scene::canBeSeen(frustum, beam));
}

TEST_CASE("a camera that is moved and turned takes its frustum with it") {
    scene::Camera camera;
    // 90 degrees of yaw look along +X (scene::Camera::yawDegrees).
    camera.yawDegrees = 90.0F;
    const glm::vec3 eye{20.0F, 1.7F, -5.0F};
    const scene::Frustum frustum = scene::frustumFromMatrix(cameraMatrix(camera, eye));

    CHECK(scene::canBeSeen(frustum, cubeAt(eye + glm::vec3{10.0F, 0.0F, 0.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt(eye + glm::vec3{-10.0F, 0.0F, 0.0F})));
    // Along -Z, where the camera looked before it was turned: now far to its left.
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt(eye + glm::vec3{0.0F, 0.0F, -10.0F})));
}

TEST_CASE("the frustum of an orthographic matrix is a box") {
    const scene::Frustum frustum = orthographicFrustum();
    // The planes stand in pairs, 10 m apart to the sides and 20 m in depth. A point in
    // the middle is 5 m from each side plane and 10 m from the near and the far plane.
    const glm::vec3 middle{0.0F, 0.0F, -10.0F};
    CHECK(scene::signedDistance(frustum.planes[LEFT_PLANE], middle) == doctest::Approx(5.0F));
    CHECK(scene::signedDistance(frustum.planes[RIGHT_PLANE], middle) == doctest::Approx(5.0F));
    CHECK(scene::signedDistance(frustum.planes[BOTTOM_PLANE], middle) == doctest::Approx(5.0F));
    CHECK(scene::signedDistance(frustum.planes[TOP_PLANE], middle) == doctest::Approx(5.0F));
    CHECK(scene::signedDistance(frustum.planes[NEAR_PLANE], middle) == doctest::Approx(10.0F));
    CHECK(scene::signedDistance(frustum.planes[FAR_PLANE], middle) == doctest::Approx(10.0F));
    // The side planes do not lean: the left one faces straight along +X.
    CHECK(std::abs(frustum.planes[LEFT_PLANE].normal.x - 1.0F) < TOLERANCE);

    CHECK(scene::canBeSeen(frustum, cubeAt(middle)));
    // Nothing gets smaller with distance: 4 m to the side is inside near and far.
    CHECK(scene::canBeSeen(frustum, cubeAt({4.0F, 0.0F, -1.0F})));
    CHECK(scene::canBeSeen(frustum, cubeAt({4.0F, 0.0F, -19.0F})));

    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({-7.0F, 0.0F, -10.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({7.0F, 0.0F, -10.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({0.0F, -7.0F, -10.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({0.0F, 7.0F, -10.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({0.0F, 0.0F, 2.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt({0.0F, 0.0F, -22.0F})));
    // Across the right side.
    CHECK(scene::canBeSeen(frustum, cubeAt({5.2F, 0.0F, -10.0F})));
}

TEST_CASE("a box is only left out when it is farther outside than the margin") {
    const scene::Frustum frustum = orthographicFrustum();
    // The right side of the frustum is the plane x = 5. A box that starts half the
    // margin to the right of it is kept, one that starts five margins away is not.
    const float justOutside = ORTHO_HALF_SIZE + scene::FRUSTUM_MARGIN * 0.5F;
    const float clearlyOutside = ORTHO_HALF_SIZE + scene::FRUSTUM_MARGIN * 5.0F;
    const scene::Aabb kept{.min = {justOutside, -1.0F, -11.0F},
                           .max = {justOutside + 1.0F, 1.0F, -9.0F}};
    const scene::Aabb leftOut{.min = {clearlyOutside, -1.0F, -11.0F},
                              .max = {clearlyOutside + 1.0F, 1.0F, -9.0F}};
    CHECK(scene::canBeSeen(frustum, kept));
    CHECK_FALSE(scene::canBeSeen(frustum, leftOut));
    // A box that only touches the plane from outside is kept too.
    const scene::Aabb touching{.min = {ORTHO_HALF_SIZE, -1.0F, -11.0F},
                               .max = {ORTHO_HALF_SIZE + 1.0F, 1.0F, -9.0F}};
    CHECK(scene::canBeSeen(frustum, touching));
}

TEST_CASE("the light space of a spot light gives the pyramid of that light") {
    // A lamp 1.5 m above the ground that shines along +X, with a cone of 20 degrees to
    // each side and a reach of 10 m.
    const glm::vec3 lamp{0.0F, 1.5F, 0.0F};
    const scene::LightSpace lightSpace =
        scene::spotLightSpace(lamp, glm::vec3{1.0F, 0.0F, 0.0F}, 20.0F, 10.0F);
    const scene::Frustum frustum = scene::frustumFromMatrix(lightSpace.matrix());

    // In the beam.
    CHECK(scene::canBeSeen(frustum, cubeAt(lamp + glm::vec3{5.0F, 0.0F, 0.0F})));
    // Behind the lamp, beside the beam and beyond its reach.
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt(lamp + glm::vec3{-5.0F, 0.0F, 0.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt(lamp + glm::vec3{5.0F, 0.0F, 6.0F})));
    CHECK_FALSE(scene::canBeSeen(frustum, cubeAt(lamp + glm::vec3{14.0F, 0.0F, 0.0F})));
    // The far plane is the reach of the light: 10 m from it.
    CHECK(scene::signedDistance(frustum.planes[FAR_PLANE], lamp) == doctest::Approx(10.0F));
}

TEST_CASE("a matrix without a frustum rejects nothing") {
    // All zeros: every plane gets a normal of length 0.
    const scene::Frustum frustum = scene::frustumFromMatrix(glm::mat4{0.0F});
    for (const scene::Plane& plane : frustum.planes) {
        CHECK(plane.normal == glm::vec3{0.0F});
        CHECK(plane.offset == 0.0F);
    }
    CHECK(scene::canBeSeen(frustum, cubeAt({0.0F, 0.0F, -10.0F})));
    CHECK(scene::canBeSeen(frustum, cubeAt({500.0F, -300.0F, 80.0F})));
}

TEST_CASE("no box with a visible point is ever left out, for random boxes") {
    // Three matrices of the kinds the game culls with: a camera that stands somewhere
    // and looks down a little, the pyramid of a spot light, and the orthographic box of
    // a directional light.
    scene::Camera camera;
    camera.yawDegrees = 35.0F;
    camera.pitchDegrees = -12.0F;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    const scene::Aabb land{.min = {-40.0F, -2.0F, -40.0F}, .max = {40.0F, 6.0F, 40.0F}};
    // spread is how far from the eye the boxes of a matrix are scattered, in metres to
    // each side: about as far as its frustum reaches, so that many boxes fall inside
    // and many outside.
    struct Case {
        glm::mat4 matrix;
        float spread;
    };
    const std::array<Case, 3> cases{{
        {.matrix = cameraMatrix(camera, eye), .spread = 60.0F},
        {.matrix = scene::spotLightSpace(eye, glm::vec3{0.6F, -0.2F, -0.7F}, 22.0F, 10.0F).matrix(),
         .spread = 12.0F},
        {.matrix = scene::directionalLightSpace(land, glm::vec3{-0.4F, -1.0F, 0.3F}).matrix(),
         .spread = 90.0F},
    }};

    // The same boxes in every run and on every system.
    constexpr std::uint32_t SEED = 20261012;
    std::mt19937 generator(SEED);
    constexpr int BOX_COUNT = 3000;
    // Points per axis of the grid a box is sampled with: 5 * 5 * 5 points, the eight
    // corners among them.
    constexpr int SAMPLES_PER_AXIS = 5;

    for (const Case& testCase : cases) {
        const glm::mat4& matrix = testCase.matrix;
        const float spread = testCase.spread;
        const scene::Frustum frustum = scene::frustumFromMatrix(matrix);
        int kept = 0;
        int leftOut = 0;
        for (int i = 0; i < BOX_COUNT; ++i) {
            // Boxes of every size from a pebble to a house, all around the frustum.
            const glm::vec3 center = eye + glm::vec3{randomBetween(generator, -spread, spread),
                                                     randomBetween(generator, -spread, spread),
                                                     randomBetween(generator, -spread, spread)};
            const glm::vec3 halfExtents{randomBetween(generator, 0.05F, spread / 8.0F),
                                        randomBetween(generator, 0.05F, spread / 8.0F),
                                        randomBetween(generator, 0.05F, spread / 8.0F)};
            const scene::Aabb box = scene::Aabb::fromCenter(center, halfExtents);

            // The reference, by brute force: the points of the grid are sent through
            // the matrix one by one, as the graphics card would do with vertices.
            bool pointVisible = false;
            // Whether one plane has all eight corners clearly behind it.
            std::array<bool, scene::FRUSTUM_PLANE_COUNT> allBehind{};
            allBehind.fill(true);
            for (int x = 0; x < SAMPLES_PER_AXIS; ++x) {
                for (int y = 0; y < SAMPLES_PER_AXIS; ++y) {
                    for (int z = 0; z < SAMPLES_PER_AXIS; ++z) {
                        // From 0 to 1 along each axis of the box.
                        const glm::vec3 share =
                            glm::vec3{x, y, z} / static_cast<float>(SAMPLES_PER_AXIS - 1);
                        const glm::vec3 point = glm::mix(box.min, box.max, share);
                        pointVisible = pointVisible || insideClipVolume(matrix, point);
                        for (std::size_t plane = 0; plane < frustum.planes.size(); ++plane) {
                            if (scene::signedDistance(frustum.planes[plane], point) >=
                                -2.0F * scene::FRUSTUM_MARGIN) {
                                allBehind[plane] = false;
                            }
                        }
                    }
                }
            }
            bool behindOnePlane = false;
            for (const bool behind : allBehind) {
                behindOnePlane = behindOnePlane || behind;
            }

            const bool seen = scene::canBeSeen(frustum, box);
            // The rule that must never break: a box with a point in the picture is drawn.
            if (pointVisible) {
                CHECK(seen);
            }
            // And the test is not lazy: a box that is wholly behind one plane is left out.
            if (behindOnePlane) {
                CHECK_FALSE(seen);
            }
            kept += seen ? 1 : 0;
            leftOut += seen ? 0 : 1;
        }
        // The boxes were spread so that both answers come up many times: a test that
        // only ever saw one of them would prove nothing.
        MESSAGE("frustum: " << kept << " boxes kept, " << leftOut << " left out");
        CHECK(kept > BOX_COUNT / 30);
        CHECK(leftOut > BOX_COUNT / 30);
    }
}

TEST_CASE("the bounds of a moved box hold all eight of its corners") {
    const scene::Aabb box{.min = {0.0F, 0.0F, 0.0F}, .max = {2.0F, 1.0F, 4.0F}};

    // Only moved: the same box somewhere else.
    scene::Transform moved;
    moved.position = {10.0F, -3.0F, 5.0F};
    const scene::Aabb movedBounds = scene::transformedBounds(box, moved.matrix());
    CHECK(glm::length(movedBounds.min - glm::vec3{10.0F, -3.0F, 5.0F}) < TOLERANCE);
    CHECK(glm::length(movedBounds.max - glm::vec3{12.0F, -2.0F, 9.0F}) < TOLERANCE);

    // A quarter turn around the vertical axis swaps its width and its depth: +Z of the
    // box goes to +X and +X goes to -Z, as for a wall that is laid along Z.
    scene::Transform quarterTurn;
    quarterTurn.rotationDegrees = {0.0F, 90.0F, 0.0F};
    const scene::Aabb turnedBounds = scene::transformedBounds(box, quarterTurn.matrix());
    CHECK(glm::length(turnedBounds.min - glm::vec3{0.0F, 0.0F, -2.0F}) < TOLERANCE);
    CHECK(glm::length(turnedBounds.max - glm::vec3{4.0F, 1.0F, 0.0F}) < TOLERANCE);

    // Turned by an angle in between, the box around it is larger than the box itself,
    // and every turned corner lies inside it.
    scene::Transform tilted;
    tilted.position = {1.0F, 2.0F, 3.0F};
    tilted.rotationDegrees = {0.0F, 30.0F, 0.0F};
    tilted.scale = {1.0F, 2.0F, 1.0F};
    const glm::mat4 matrix = tilted.matrix();
    const scene::Aabb tiltedBounds = scene::transformedBounds(box, matrix);
    for (const float x : {box.min.x, box.max.x}) {
        for (const float y : {box.min.y, box.max.y}) {
            for (const float z : {box.min.z, box.max.z}) {
                const glm::vec3 corner{matrix * glm::vec4{x, y, z, 1.0F}};
                CHECK(corner.x >= tiltedBounds.min.x - TOLERANCE);
                CHECK(corner.y >= tiltedBounds.min.y - TOLERANCE);
                CHECK(corner.z >= tiltedBounds.min.z - TOLERANCE);
                CHECK(corner.x <= tiltedBounds.max.x + TOLERANCE);
                CHECK(corner.y <= tiltedBounds.max.y + TOLERANCE);
                CHECK(corner.z <= tiltedBounds.max.z + TOLERANCE);
            }
        }
    }
    // The height was doubled and the turn is around the vertical axis.
    CHECK(tiltedBounds.max.y - tiltedBounds.min.y == doctest::Approx(2.0F));
    const glm::vec3 size = tiltedBounds.max - tiltedBounds.min;
    CHECK(size.x * size.z > 2.0F * 4.0F);
}

TEST_CASE("the bounds of two boxes hold both") {
    const scene::Aabb low{.min = {-1.0F, 0.0F, -0.2F}, .max = {1.0F, 3.0F, 0.2F}};
    const scene::Aabb crown{.min = {-1.1F, 2.9F, -0.3F}, .max = {0.9F, 3.4F, 0.1F}};
    const scene::Aabb both = scene::boundsOfBoth(low, crown);
    CHECK(both.min == glm::vec3{-1.1F, 0.0F, -0.3F});
    CHECK(both.max == glm::vec3{1.0F, 3.4F, 0.2F});
}
