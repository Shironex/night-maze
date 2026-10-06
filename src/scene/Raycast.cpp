// Raycast: a ray, where it hits a box or a sphere, and the ray that goes through a pixel.
// See docs/modules/scene/picking.md
#include "scene/Raycast.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace scene {

namespace {

// The number of axes of the world: a glm::vec3 has the components 0 (x), 1 (y) and 2 (z).
constexpr int AXIS_COUNT = 3;

// Normalized device coordinates (NDC) are what is left of a point after the projection:
// the visible part of the world is a cube from -1 to 1 on every axis. On the z axis -1 is
// the near clipping plane and 1 the far one (the OpenGL convention, which GLM follows).
constexpr float NDC_NEAR = -1.0F;
constexpr float NDC_FAR = 1.0F;

// The NDC cube is 2 wide (from -1 to 1), so a position from 0 to 1 is stretched by 2 and
// then moved by 1.
constexpr float NDC_WIDTH = 2.0F;
constexpr float NDC_EDGE = 1.0F;

// A point given in NDC, moved back into world space.
glm::vec3 worldPointFromNdc(const glm::vec3& ndc, const glm::mat4& inverseViewProjection) {
    // A point is written with a fourth number w = 1, so that a matrix can also move it.
    const glm::vec4 clip{ndc, 1.0F};
    const glm::vec4 world = inverseViewProjection * clip;

    // A perspective matrix leaves a w that is not 1. On the way to the screen the
    // graphics card divides x, y and z by it (that division is what makes far things
    // small). On the way back the same division by w undoes it.
    return glm::vec3{world} / world.w;
}

} // namespace

RayHit intersect(const Ray& ray, const Aabb& box) {
    // A ray without a direction goes nowhere. Without this early return the loop below
    // would call the ray "parallel to everything" and report a hit for an origin inside
    // the box.
    if (glm::dot(ray.direction, ray.direction) == 0.0F) {
        return {};
    }

    // The slab method. A box is the space shared by three slabs: the slab of the x axis
    // is everything between the planes x = min.x and x = max.x, and the same for y and
    // z. On each axis the ray is inside the slab for one stretch of distances, from the
    // distance where it crosses the first plane to the distance where it crosses the
    // second one. The ray is inside the box where it is inside all three slabs at once:
    // from the LATEST of the three entries to the EARLIEST of the three exits.
    //
    // The stretch starts as "from the origin on, for ever". Starting at 0 and not at
    // minus infinity is what cuts away everything behind the origin.
    float enterDistance = 0.0F;
    float exitDistance = std::numeric_limits<float>::infinity();

    for (int axis = 0; axis < AXIS_COUNT; ++axis) {
        const float origin = ray.origin[axis];
        const float direction = ray.direction[axis];

        if (direction == 0.0F) {
            // The ray does not move along this axis at all: it runs parallel to the two
            // planes of the slab and never crosses them. Dividing by the direction (as
            // below) would be a division by zero here, so this case has its own answer:
            // the ray stays on the side of the planes it started on. Outside the slab it
            // misses the box for good. Inside (or exactly on a plane) this axis puts no
            // limit on the stretch.
            if (origin < box.min[axis] || origin > box.max[axis]) {
                return {};
            }
            continue;
        }

        // origin + t * direction = plane, solved for t: the distances at which the ray
        // crosses the two planes of the slab. Only an exact 0 is a problem for this
        // division. A very small direction gives a very large distance, which is right:
        // the ray needs a long way to cross a plane it runs almost parallel to.
        float slabEnter = (box.min[axis] - origin) / direction;
        float slabExit = (box.max[axis] - origin) / direction;
        // A ray that goes towards the negative side crosses the max plane first.
        if (slabEnter > slabExit) {
            std::swap(slabEnter, slabExit);
        }

        enterDistance = std::max(enterDistance, slabEnter);
        exitDistance = std::min(exitDistance, slabExit);

        // The ray has left one slab before it has entered another: there is no distance
        // at which it is inside all of them. That is a miss. It is also the answer for
        // a box behind the origin: its exit lies below 0, where the stretch starts.
        // "Greater", not "greater or equal": a stretch of length 0 is a ray that touches
        // the box in one point, and that counts as a hit.
        if (enterDistance > exitDistance) {
            return {};
        }
    }

    // enterDistance is still 0 when the origin is inside the box.
    return {.hit = true, .distance = enterDistance};
}

RayHit intersect(const Ray& ray, const Sphere& sphere) {
    // A ray without a direction goes nowhere.
    if (glm::dot(ray.direction, ray.direction) == 0.0F) {
        return {};
    }

    const glm::vec3 toCenter = sphere.center - ray.origin;
    const float radiusSquared = sphere.radius * sphere.radius;
    // The length of a vector squared is its dot product with itself. Comparing squares
    // saves the square root.
    const float centerDistanceSquared = glm::dot(toCenter, toCenter);

    // The origin is inside the sphere or on it: the ray is in the shape from the start.
    if (centerDistanceSquared <= radiusSquared) {
        return {.hit = true, .distance = 0.0F};
    }

    // How far along the ray the point closest to the centre lies. The dot product with
    // a unit vector is the length of the shadow of toCenter on the ray.
    const float closestDistance = glm::dot(toCenter, ray.direction);

    // The origin is outside the sphere and the centre is not in front of it: the ray
    // moves away from the sphere.
    if (closestDistance <= 0.0F) {
        return {};
    }

    // The origin, the centre and the closest point form a right triangle. Its long side
    // is toCenter and one short side is closestDistance, so by Pythagoras the other
    // short side (how far the ray passes from the centre) squared is:
    const float missDistanceSquared = centerDistanceSquared - closestDistance * closestDistance;

    // The ray passes the centre farther away than the radius: a miss. Passing at exactly
    // the radius is the ray that touches the sphere in one point, and that is a hit.
    if (missDistanceSquared > radiusSquared) {
        return {};
    }

    // A second right triangle: the centre, the closest point and the point where the ray
    // enters the sphere. Its long side is the radius, so the ray enters this far BEFORE
    // the closest point:
    const float halfChord = std::sqrt(radiusSquared - missDistanceSquared);
    return {.hit = true, .distance = closestDistance - halfChord};
}

NearestHit nearestHit(const Ray& ray, std::span<const Aabb> boxes, float maxDistance) {
    NearestHit nearest;
    for (std::size_t index = 0; index < boxes.size(); ++index) {
        const RayHit hit = intersect(ray, boxes[index]);
        // Not hit at all, or too far away to reach.
        if (!hit.hit || hit.distance > maxDistance) {
            continue;
        }
        // The first box found, or one that is nearer than the best so far. "Less", not
        // "less or equal": of two boxes at the same distance the earlier one stays.
        if (!nearest.hit || hit.distance < nearest.distance) {
            nearest = {.hit = true, .index = index, .distance = hit.distance};
        }
    }
    return nearest;
}

Ray screenPointRay(const glm::vec2& position, const glm::vec2& size,
                   const glm::mat4& inverseViewProjection) {
    if (size.x <= 0.0F || size.y <= 0.0F) {
        throw std::invalid_argument("screenPointRay: the size of the picture must be positive");
    }

    // Step 1: from the position in the picture to NDC. Dividing by the size gives a
    // number from 0 to 1, which is then stretched to the range from -1 to 1.
    const float ndcX = position.x / size.x * NDC_WIDTH - NDC_EDGE;
    // The same for y, turned upside down: in a window y = 0 is the TOP row and y grows
    // down, in NDC y = 1 is the top and y grows up.
    const float ndcY = NDC_EDGE - position.y / size.y * NDC_WIDTH;

    // Step 2: one position of the picture is a whole line of points of the world, all
    // drawn on top of each other. Two points of that line are enough to know it: the one
    // on the near clipping plane and the one on the far clipping plane.
    // Step 3: both are moved back into world space with the inverse matrix, and divided
    // by w (see worldPointFromNdc).
    const glm::vec3 nearPoint = worldPointFromNdc({ndcX, ndcY, NDC_NEAR}, inverseViewProjection);
    const glm::vec3 farPoint = worldPointFromNdc({ndcX, ndcY, NDC_FAR}, inverseViewProjection);

    // Step 4: the ray starts in the near point and goes towards the far point. The
    // direction is made a unit vector, so distances along the ray are in metres.
    return {.origin = nearPoint, .direction = glm::normalize(farPoint - nearPoint)};
}

} // namespace scene
