// Tests of scene::Ray: where it hits boxes and spheres, the nearest hit of a list and the
// ray through a point of the picture.
#include "scene/Raycast.hpp"

#include "scene/Camera.hpp"
#include "scene/Collider.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

// The box of most tests: 2 m wide, high and deep, 4 m in front of the origin of the
// world along -Z. Its near face is the plane z = -4, its far face the plane z = -6.
constexpr scene::Aabb BOX{.min = {-1.0F, -1.0F, -6.0F}, .max = {1.0F, 1.0F, -4.0F}};

// The sphere of most tests: 1 m in radius, its centre 5 m in front of the origin of the
// world along -Z. Its nearest point is 4 m away.
constexpr scene::Sphere SPHERE{.center = {0.0F, 0.0F, -5.0F}, .radius = 1.0F};

// Unit vectors along the axes.
constexpr glm::vec3 TOWARDS_MINUS_Z{0.0F, 0.0F, -1.0F};
constexpr glm::vec3 TOWARDS_PLUS_Z{0.0F, 0.0F, 1.0F};
constexpr glm::vec3 TOWARDS_PLUS_X{1.0F, 0.0F, 0.0F};

// The picture of the camera tests: 1280 by 720, a 16:9 window.
constexpr glm::vec2 PICTURE_SIZE{1280.0F, 720.0F};
constexpr float ASPECT_RATIO = 1280.0F / 720.0F;

// How close two directions or points have to be to count as the same, per component.
// The far clipping plane is 100 m away and a float keeps about seven digits, so a point
// brought back from there is only good to some tenths of a millimetre.
constexpr float TOLERANCE = 0.001F;

void checkClose(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(std::abs(actual.x - expected.x) < TOLERANCE);
    CHECK(std::abs(actual.y - expected.y) < TOLERANCE);
    CHECK(std::abs(actual.z - expected.z) < TOLERANCE);
}

// A camera that looks somewhere that is not along an axis, and where it stands.
scene::Camera tiltedCamera() {
    scene::Camera camera;
    camera.yawDegrees = 35.0F;
    camera.pitchDegrees = -12.0F;
    return camera;
}
constexpr glm::vec3 EYE{3.0F, 1.7F, 5.0F};

// The inverse of projection * view, which screenPointRay wants.
glm::mat4 inverseViewProjection(const scene::Camera& camera) {
    return glm::inverse(camera.projectionMatrix(ASPECT_RATIO) * camera.viewMatrix(EYE));
}

// The direction that is "up" in the picture of the camera: at a right angle to both
// forward and right.
glm::vec3 cameraUp(const scene::Camera& camera) {
    return glm::cross(camera.right(), camera.forward());
}

} // namespace

TEST_CASE("a new ray starts in the origin and looks along -Z like a new camera") {
    const scene::Ray ray;
    const scene::Camera camera;

    CHECK(ray.origin == glm::vec3{0.0F});
    checkClose(ray.direction, camera.forward());
}

TEST_CASE("a ray that runs into a box hits it where it enters") {
    SUBCASE("straight at the near face") {
        const scene::RayHit hit = scene::intersect(scene::Ray{}, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(4.0F));
    }

    SUBCASE("from the other side it enters through the far face") {
        const scene::Ray ray{.origin = {0.0F, 0.0F, -10.0F}, .direction = TOWARDS_PLUS_Z};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(4.0F));
    }

    SUBCASE("at an angle the distance is measured along the ray") {
        // From 3 m to the right and 4 m in front of the middle of the near face: the
        // way to it is 5 m long (a 3-4-5 triangle).
        const glm::vec3 origin{3.0F, 0.0F, 0.0F};
        const glm::vec3 target{0.0F, 0.0F, -4.0F};
        const scene::Ray ray{.origin = origin, .direction = glm::normalize(target - origin)};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(5.0F));
    }
}

TEST_CASE("a ray that passes a box or looks away from it does not hit") {
    SUBCASE("it passes beside the box") {
        const scene::Ray ray{.origin = {2.0F, 0.0F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        CHECK_FALSE(scene::intersect(ray, BOX).hit);
    }

    SUBCASE("it passes above the box") {
        const scene::Ray ray{.origin = {0.0F, 1.5F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        CHECK_FALSE(scene::intersect(ray, BOX).hit);
    }

    SUBCASE("it goes off to the side") {
        const scene::Ray ray{.origin = {0.0F, 0.0F, 0.0F}, .direction = TOWARDS_PLUS_X};
        CHECK_FALSE(scene::intersect(ray, BOX).hit);
    }

    SUBCASE("the box is behind the origin") {
        // The line of the ray goes through the box, but only backwards.
        const scene::Ray ray{.origin = {0.0F, 0.0F, 0.0F}, .direction = TOWARDS_PLUS_Z};
        CHECK_FALSE(scene::intersect(ray, BOX).hit);
    }
}

TEST_CASE("a ray that starts inside a box, or on it, hits at the distance 0") {
    SUBCASE("in the middle of the box") {
        const scene::Ray ray{.origin = {0.0F, 0.0F, -5.0F}, .direction = TOWARDS_MINUS_Z};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == 0.0F);
    }

    SUBCASE("inside, looking any way") {
        const scene::Ray ray{.origin = {0.5F, -0.5F, -4.5F},
                             .direction = glm::normalize(glm::vec3{1.0F, 2.0F, 3.0F})};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == 0.0F);
    }

    SUBCASE("on the near face, looking in") {
        const scene::Ray ray{.origin = {0.0F, 0.0F, -4.0F}, .direction = TOWARDS_MINUS_Z};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == 0.0F);
    }
}

TEST_CASE("a ray parallel to faces of a box hits only from between those faces") {
    // A ray along -Z has the direction 0 on the x and on the y axis: it is parallel to
    // the four side faces. No division by zero may happen on those axes.

    SUBCASE("between the faces it hits") {
        const scene::Ray ray{.origin = {0.9F, -0.9F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(4.0F));
    }

    SUBCASE("outside the faces it misses, however long the ray is") {
        const scene::Ray ray{.origin = {1.5F, 0.0F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        CHECK_FALSE(scene::intersect(ray, BOX).hit);
    }

    SUBCASE("parallel to all faces but two, from the left") {
        // Along +X the ray is parallel to the top, the bottom, the near and the far face.
        const scene::Ray ray{.origin = {-3.0F, 0.0F, -5.0F}, .direction = TOWARDS_PLUS_X};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(2.0F));
    }
}

TEST_CASE("a ray that only grazes a box hits it") {
    SUBCASE("it runs exactly along a side face") {
        // x = 1 is the plane of the right face.
        const scene::Ray ray{.origin = {1.0F, 0.0F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(4.0F));
    }

    SUBCASE("it touches one edge and goes on outside") {
        // Seen from above the ray goes from (x 2, z -5) diagonally towards -X and +Z.
        // It meets the box in the single point (x 1, z -4), the edge between the near
        // face and the right face, after the square root of 2 metres.
        const scene::Ray ray{.origin = {2.0F, 0.0F, -5.0F},
                             .direction = glm::normalize(glm::vec3{-1.0F, 0.0F, 1.0F})};
        const scene::RayHit hit = scene::intersect(ray, BOX);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(std::sqrt(2.0F)));
    }

    SUBCASE("a hair further out it misses") {
        const scene::Ray ray{.origin = {2.01F, 0.0F, -5.0F},
                             .direction = glm::normalize(glm::vec3{-1.0F, 0.0F, 1.0F})};
        CHECK_FALSE(scene::intersect(ray, BOX).hit);
    }
}

TEST_CASE("a ray without a direction hits nothing") {
    const glm::vec3 noDirection{0.0F};

    // Not even the shape its origin is inside of.
    const scene::Ray inside{.origin = {0.0F, 0.0F, -5.0F}, .direction = noDirection};
    CHECK_FALSE(scene::intersect(inside, BOX).hit);
    CHECK_FALSE(scene::intersect(inside, SPHERE).hit);

    const scene::Ray outside{.origin = {0.0F, 0.0F, 0.0F}, .direction = noDirection};
    CHECK_FALSE(scene::intersect(outside, BOX).hit);
    CHECK_FALSE(scene::intersect(outside, SPHERE).hit);

    const std::array<scene::Aabb, 1> boxes = {BOX};
    CHECK_FALSE(scene::nearestHit(inside, boxes, 100.0F).hit);
}

TEST_CASE("a ray that runs into a sphere hits it where it enters") {
    SUBCASE("straight at the centre") {
        const scene::RayHit hit = scene::intersect(scene::Ray{}, SPHERE);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(4.0F));
    }

    SUBCASE("off the centre the surface is farther away") {
        // The ray passes the centre at 0.6 m. The half chord is then 0.8 m (a 0.6-0.8-1
        // triangle), so it enters 0.8 m before the closest point, which is 5 m away.
        const scene::Ray ray{.origin = {0.6F, 0.0F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        const scene::RayHit hit = scene::intersect(ray, SPHERE);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(4.2F));
    }

    SUBCASE("it touches the sphere in one point") {
        // The ray passes the centre at exactly the radius.
        const scene::Ray ray{.origin = {1.0F, 0.0F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        const scene::RayHit hit = scene::intersect(ray, SPHERE);
        CHECK(hit.hit);
        CHECK(hit.distance == doctest::Approx(5.0F));
    }
}

TEST_CASE("a ray that passes a sphere or looks away from it does not hit") {
    SUBCASE("it passes beside the sphere") {
        const scene::Ray ray{.origin = {1.01F, 0.0F, 0.0F}, .direction = TOWARDS_MINUS_Z};
        CHECK_FALSE(scene::intersect(ray, SPHERE).hit);
    }

    SUBCASE("the sphere is behind the origin") {
        const scene::Ray ray{.origin = {0.0F, 0.0F, 0.0F}, .direction = TOWARDS_PLUS_Z};
        CHECK_FALSE(scene::intersect(ray, SPHERE).hit);
    }

    SUBCASE("the ray has already left the sphere") {
        // The origin is 2 m past the centre, outside, and looks on: the sphere is behind.
        const scene::Ray ray{.origin = {0.0F, 0.0F, -7.0F}, .direction = TOWARDS_MINUS_Z};
        CHECK_FALSE(scene::intersect(ray, SPHERE).hit);
    }
}

TEST_CASE("a ray that starts inside a sphere, or on it, hits at the distance 0") {
    SUBCASE("inside, looking out") {
        const scene::Ray ray{.origin = {0.0F, 0.5F, -5.0F}, .direction = TOWARDS_PLUS_X};
        const scene::RayHit hit = scene::intersect(ray, SPHERE);
        CHECK(hit.hit);
        CHECK(hit.distance == 0.0F);
    }

    SUBCASE("on the surface, looking away") {
        const scene::Ray ray{.origin = {0.0F, 0.0F, -4.0F}, .direction = TOWARDS_PLUS_Z};
        const scene::RayHit hit = scene::intersect(ray, SPHERE);
        CHECK(hit.hit);
        CHECK(hit.distance == 0.0F);
    }
}

TEST_CASE("nearestHit finds the first box along the ray, whatever the order of the list") {
    // Three boxes in a row along -Z, their near faces 2 m, 4 m and 8 m away, and one
    // beside the ray. They are listed out of order on purpose.
    const scene::Aabb at8{.min = {-1.0F, -1.0F, -9.0F}, .max = {1.0F, 1.0F, -8.0F}};
    const scene::Aabb at2{.min = {-1.0F, -1.0F, -3.0F}, .max = {1.0F, 1.0F, -2.0F}};
    const scene::Aabb beside{.min = {5.0F, -1.0F, -2.0F}, .max = {6.0F, 1.0F, -1.0F}};
    const std::vector<scene::Aabb> boxes = {at8, BOX, beside, at2};
    const scene::Ray ray;

    const scene::NearestHit nearest = scene::nearestHit(ray, boxes, 100.0F);
    REQUIRE(nearest.hit);
    CHECK(nearest.index == 3U);
    CHECK(nearest.distance == doctest::Approx(2.0F));

    SUBCASE("a box behind the origin is never the nearest") {
        const scene::Ray back{.origin = {0.0F, 0.0F, -3.5F}, .direction = TOWARDS_MINUS_Z};
        const scene::NearestHit ahead = scene::nearestHit(back, boxes, 100.0F);
        REQUIRE(ahead.hit);
        CHECK(ahead.index == 1U);
        CHECK(ahead.distance == doctest::Approx(0.5F));
    }

    SUBCASE("of two boxes at the same distance the earlier one wins") {
        const std::vector<scene::Aabb> twins = {at8, BOX, BOX};
        const scene::NearestHit first = scene::nearestHit(ray, twins, 100.0F);
        REQUIRE(first.hit);
        CHECK(first.index == 1U);
    }

    SUBCASE("an empty list and a list of boxes the ray misses give no hit") {
        CHECK_FALSE(scene::nearestHit(ray, {}, 100.0F).hit);
        const std::vector<scene::Aabb> missed = {beside};
        CHECK_FALSE(scene::nearestHit(ray, missed, 100.0F).hit);
    }
}

TEST_CASE("nearestHit skips what is out of reach") {
    // The near face of BOX is 4 m away.
    const std::array<scene::Aabb, 1> boxes = {BOX};
    const scene::Ray ray;

    CHECK_FALSE(scene::nearestHit(ray, boxes, 3.9F).hit);
    // Exactly at the end of the reach still counts.
    CHECK(scene::nearestHit(ray, boxes, 4.0F).hit);
    CHECK(scene::nearestHit(ray, boxes, 4.1F).hit);

    // A box out of reach does not hide a nearer one, and a nearer one is not replaced
    // by it: only the boxes within reach take part.
    const scene::Aabb at2{.min = {-1.0F, -1.0F, -3.0F}, .max = {1.0F, 1.0F, -2.0F}};
    const std::array<scene::Aabb, 2> two = {BOX, at2};
    const scene::NearestHit nearest = scene::nearestHit(ray, two, 3.0F);
    REQUIRE(nearest.hit);
    CHECK(nearest.index == 1U);
}

TEST_CASE("the ray through the middle of the picture looks where the camera looks") {
    const scene::Camera camera = tiltedCamera();
    const scene::Ray ray =
        scene::screenPointRay(PICTURE_SIZE / 2.0F, PICTURE_SIZE, inverseViewProjection(camera));

    checkClose(ray.direction, camera.forward());
    CHECK(glm::length(ray.direction) == doctest::Approx(1.0F));
    // It starts on the near clipping plane, straight in front of the eye.
    checkClose(ray.origin, EYE + camera.forward() * camera.nearPlane);
}

TEST_CASE("the ray through a corner of the picture runs along an edge of the view frustum") {
    const scene::Camera camera = tiltedCamera();
    const glm::mat4 inverse = inverseViewProjection(camera);

    // The field of view is the angle between the top and the bottom edge of the
    // picture. One metre in front of the eye the picture therefore reaches the tangent
    // of half that angle up and down, and that times the aspect ratio to each side.
    const float halfHeight = std::tan(glm::radians(camera.fovDegrees) / 2.0F);
    const float halfWidth = halfHeight * ASPECT_RATIO;
    const glm::vec3 forward = camera.forward();
    const glm::vec3 right = camera.right();
    const glm::vec3 up = cameraUp(camera);

    SUBCASE("position (0, 0) is the TOP left corner: y grows down in a window") {
        const scene::Ray ray = scene::screenPointRay({0.0F, 0.0F}, PICTURE_SIZE, inverse);
        checkClose(ray.direction, glm::normalize(forward - right * halfWidth + up * halfHeight));
    }

    SUBCASE("position (width, height) is the bottom right corner") {
        const scene::Ray ray = scene::screenPointRay(PICTURE_SIZE, PICTURE_SIZE, inverse);
        checkClose(ray.direction, glm::normalize(forward + right * halfWidth - up * halfHeight));
    }

    SUBCASE("the middle of the top edge looks up by half of the field of view") {
        const scene::Ray ray =
            scene::screenPointRay({PICTURE_SIZE.x / 2.0F, 0.0F}, PICTURE_SIZE, inverse);
        checkClose(ray.direction, glm::normalize(forward + up * halfHeight));
    }
}

TEST_CASE("the ray through the place where a point is drawn hits that point") {
    const scene::Camera camera = tiltedCamera();
    const glm::mat4 viewProjection = camera.projectionMatrix(ASPECT_RATIO) * camera.viewMatrix(EYE);

    // A point 6 m in front of the eye, to the right of and above the middle.
    const glm::vec3 point =
        EYE + camera.forward() * 6.0F + camera.right() * 1.5F + cameraUp(camera) * 0.5F;

    // The way the graphics card takes: to clip space, the division by w, and from NDC
    // (-1 to 1, y up) to a position in the picture (0 to the size, y down).
    const glm::vec4 clip = viewProjection * glm::vec4{point, 1.0F};
    const glm::vec3 ndc = glm::vec3{clip} / clip.w;
    const glm::vec2 position{(ndc.x + 1.0F) / 2.0F * PICTURE_SIZE.x,
                             (1.0F - ndc.y) / 2.0F * PICTURE_SIZE.y};

    const scene::Ray ray =
        scene::screenPointRay(position, PICTURE_SIZE, glm::inverse(viewProjection));

    // A small sphere around the point: the ray has to go through it.
    const scene::Sphere marker{.center = point, .radius = 0.02F};
    const scene::RayHit hit = scene::intersect(ray, marker);
    REQUIRE(hit.hit);
    CHECK(hit.distance ==
          doctest::Approx(glm::length(point - ray.origin) - marker.radius).epsilon(0.001));
}

TEST_CASE("the unit of the position does not matter as long as the size uses the same") {
    const scene::Camera camera = tiltedCamera();
    const glm::mat4 inverse = inverseViewProjection(camera);

    // The same place of the picture, once in pixels of a 1280 by 720 framebuffer and
    // once in pixels of one twice as fine (a HiDPI screen).
    const scene::Ray coarse = scene::screenPointRay({320.0F, 500.0F}, PICTURE_SIZE, inverse);
    const scene::Ray fine = scene::screenPointRay({640.0F, 1000.0F}, PICTURE_SIZE * 2.0F, inverse);

    checkClose(fine.direction, coarse.direction);
    checkClose(fine.origin, coarse.origin);
}

TEST_CASE("a picture without a size is an error") {
    const glm::mat4 inverse = inverseViewProjection(scene::Camera{});

    CHECK_THROWS_AS(scene::screenPointRay({0.0F, 0.0F}, {0.0F, 720.0F}, inverse),
                    std::invalid_argument);
    CHECK_THROWS_AS(scene::screenPointRay({0.0F, 0.0F}, {1280.0F, 0.0F}, inverse),
                    std::invalid_argument);
    CHECK_THROWS_AS(scene::screenPointRay({0.0F, 0.0F}, {-1.0F, 720.0F}, inverse),
                    std::invalid_argument);
}
