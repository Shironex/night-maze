// Collider: axis-aligned boxes and spheres, overlap tests and movement that slides along
// obstacles.
#include "scene/Collider.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace scene {

namespace {

// The components of a glm::vec3 by number: vector[AXIS_X] is the same float as vector.x.
// Numbers let one function do the work for any of the three axes.
constexpr int AXIS_X = 0;
constexpr int AXIS_Y = 1;
constexpr int AXIS_Z = 2;
constexpr int AXIS_COUNT = 3;

// The order in which moveAndSlide handles the axes: the two horizontal ones first, the
// vertical one last. In a maze almost all movement is horizontal.
constexpr std::array<int, AXIS_COUNT> AXIS_ORDER = {AXIS_X, AXIS_Z, AXIS_Y};

// Seen along one axis a box is just an interval from min to max. This is the length of
// the part that the intervals of two boxes have in common: it starts at the larger of the
// two starts and ends at the smaller of the two ends. Zero means the intervals touch,
// a negative value is the size of the gap between them.
float sharedLength(const Aabb& a, const Aabb& b, int axis) {
    return std::min(a.max[axis], b.max[axis]) - std::max(a.min[axis], b.min[axis]);
}

// How far the box may travel along one axis. distance is the wanted movement: its sign
// is the direction. The result has the same sign and is never longer than distance.
float allowedDistance(const Aabb& box, float distance, int axis, std::span<const Aabb> obstacles) {
    // Without this early return the code below would treat "no movement" as movement in
    // the negative direction.
    if (distance == 0.0F) {
        return 0.0F;
    }

    // The two axes the box does not move along.
    const int sideAxisA = (axis + 1) % AXIS_COUNT;
    const int sideAxisB = (axis + 2) % AXIS_COUNT;

    // The loop below works with the length of the path, the direction is put back at the
    // end. That keeps one set of comparisons for both directions.
    const bool movesForward = distance > 0.0F;
    float allowed = std::abs(distance);

    for (const Aabb& obstacle : obstacles) {
        // Moving along the axis, the box sweeps a corridor. Only an obstacle inside that
        // corridor can be hit: it has to share a part of both side axes with the box.
        // A shared part within CONTACT_TOLERANCE is a wall the box rests against (plus
        // the rounding error of a float), and such a wall must not stop the slide.
        if (sharedLength(box, obstacle, sideAxisA) <= CONTACT_TOLERANCE ||
            sharedLength(box, obstacle, sideAxisB) <= CONTACT_TOLERANCE) {
            continue;
        }

        // The free space between the face of the box that leads the movement and the
        // face of the obstacle that looks at it.
        const float gap =
            movesForward ? obstacle.min[axis] - box.max[axis] : box.min[axis] - obstacle.max[axis];

        // A clearly negative gap: the obstacle is behind the box, or the box is already
        // inside it. Neither stops this movement. In the second case that is what lets
        // a box that got into an obstacle walk out again.
        if (gap < -CONTACT_TOLERANCE) {
            continue;
        }

        // The obstacle is ahead: the box may go as far as its face and no further.
        // A gap just below zero (rounding again) is treated as zero, so the result never
        // points backwards.
        allowed = std::min(allowed, std::max(gap, 0.0F));
    }

    return movesForward ? allowed : -allowed;
}

} // namespace

Aabb Aabb::fromCenter(const glm::vec3& center, const glm::vec3& halfExtents) {
    return {.min = center - halfExtents, .max = center + halfExtents};
}

bool overlaps(const Aabb& a, const Aabb& b) {
    // Two boxes share volume only when their intervals overlap on all three axes. One
    // axis with a gap (or with mere contact) is enough to keep them apart: a plane
    // perpendicular to that axis fits between them.
    return sharedLength(a, b, AXIS_X) > 0.0F && sharedLength(a, b, AXIS_Y) > 0.0F &&
           sharedLength(a, b, AXIS_Z) > 0.0F;
}

bool overlaps(const Sphere& a, const Sphere& b) {
    // The squares are compared instead of the distances themselves: the square root that
    // a distance needs is the expensive part, and for numbers that are not negative
    // "smaller" means the same before and after squaring. dot(v, v) is the squared
    // length of v.
    const glm::vec3 offset = b.center - a.center;
    const float reach = a.radius + b.radius;
    return glm::dot(offset, offset) < reach * reach;
}

glm::vec3 closestPoint(const Aabb& box, const glm::vec3& point) {
    // Axis by axis: a coordinate between min and max stays, one outside is moved to the
    // nearer of the two. glm::clamp does that for all three components at once.
    return glm::clamp(point, box.min, box.max);
}

bool overlaps(const Sphere& sphere, const Aabb& box) {
    // The sphere reaches the box exactly when it reaches the point of the box nearest to
    // its centre. A centre inside the box is its own nearest point: the distance is 0,
    // and any radius above 0 overlaps.
    const glm::vec3 offset = closestPoint(box, sphere.center) - sphere.center;
    return glm::dot(offset, offset) < sphere.radius * sphere.radius;
}

glm::vec3 moveAndSlide(const Aabb& mover, const glm::vec3& displacement,
                       std::span<const Aabb> obstacles) {
    // Why one axis at a time gives sliding: a box pushed diagonally into a wall wants to
    // move both towards the wall and along it. Tested as one diagonal step, the whole
    // step would hit the wall and the box would stop dead. Split into axes, only the
    // part towards the wall is cut off (the gap is zero), and the part along the wall is
    // judged on its own: the wall is beside the corridor of that movement, not in it.
    //
    // The limit: the box travels along the edges of a staircase (all of x, then all of z,
    // then all of y), not along the straight diagonal. Each of the three moves is exact,
    // it measures the gap and cannot jump over an obstacle. But the staircase can go
    // around a small obstacle that the diagonal would hit, or touch a corner that the
    // diagonal would miss. The error is as large as the step, so the steps must be short
    // compared with the boxes. The game moves in fixed steps of core::Time::FIXED_DT
    // (1/120 s): at a walking speed of 3 m/s that is 2.5 cm per step, against wall boxes that
    // are 30 cm thick. A displacement taken from a whole frame time, which can be 0.25 s
    // long, would not keep that promise.
    Aabb box = mover;
    glm::vec3 allowed{0.0F};
    for (const int axis : AXIS_ORDER) {
        allowed[axis] = allowedDistance(box, displacement[axis], axis, obstacles);
        // The next axis starts from where this one ended.
        box.min[axis] += allowed[axis];
        box.max[axis] += allowed[axis];
    }
    return allowed;
}

} // namespace scene
