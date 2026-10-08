// Raycast: a ray, where it hits a box or a sphere, and the ray that goes through a pixel.
#pragma once

#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <span>

namespace scene {

/// A ray: a half line that starts in origin and goes on for ever along direction.
/// The point of the ray at the distance t (in metres, t at least 0) is
/// origin + t * direction.
///
/// Plain data, like Aabb and Sphere: no OpenGL and no input.
struct Ray {
    /// Where the ray starts, in world space.
    glm::vec3 origin{0.0F};

    /// Where it goes. It must be a unit vector (length 1): only then the t of a point is
    /// its distance from the origin in metres. The default looks along -Z, like a new
    /// Camera. A direction of length 0 is allowed and means "no ray": it hits nothing.
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
};

/// The answer of a ray test against one shape.
struct RayHit {
    /// True when the ray touches or enters the shape in front of its origin.
    bool hit = false;

    /// How far along the ray the first point of the shape is, in metres. 0 when the ray
    /// starts inside the shape. It means nothing when hit is false.
    float distance = 0.0F;
};

/// Where a ray hits a box.
///
/// The cases:
///   - the ray enters the box in front of its origin: a hit, at the distance of the point
///     where it enters,
///   - the origin is inside the box (or on its surface): a hit at the distance 0,
///   - the box lies behind the origin: no hit, a ray does not go backwards,
///   - the ray only grazes the box (it runs exactly along a face, or touches one edge or
///     one corner): a hit. This is not the rule of overlaps, where touching does not
///     count. A ray is infinitely thin, it can never "share volume" with anything, so
///     touching is the only contact it has,
///   - the ray runs parallel to two faces of the box (one component of its direction is
///     exactly 0): it hits only when its origin lies between those two faces,
///   - the direction has the length 0: no hit.
RayHit intersect(const Ray& ray, const Aabb& box);

/// Where a ray hits a sphere. The cases are the same as for a box: a hit at the point
/// where the ray enters, a hit at the distance 0 when the origin is inside the sphere or
/// on it, no hit when the sphere is behind the origin or the direction has the length 0,
/// and a ray that touches the sphere in exactly one point hits it there.
///
/// The direction of the ray must be a unit vector.
RayHit intersect(const Ray& ray, const Sphere& sphere);

/// The answer of nearestHit.
struct NearestHit {
    /// True when the ray hits at least one of the boxes within the reach.
    bool hit = false;

    /// The number of the nearest box in the list. It means nothing when hit is false.
    std::size_t index = 0;

    /// How far along the ray that box is, in metres. It means nothing when hit is false.
    float distance = 0.0F;
};

/// The first box a ray runs into: of all boxes it hits, the one at the smallest distance.
/// Of two boxes at exactly the same distance the earlier one in the list wins. A box
/// farther away than maxDistance metres is out of reach and is skipped. A box at exactly
/// maxDistance is still within reach.
///
/// This is what picking uses: the list holds everything the ray could run into, and the
/// reach is how far the player can stretch an arm.
NearestHit nearestHit(const Ray& ray, std::span<const Aabb> boxes, float maxDistance);

/// The ray that goes from the camera through one point of the picture.
///
/// position is the point, measured from the TOP left corner of the picture: x grows to
/// the right and y grows DOWN, the way a window reports its cursor. size is the width and
/// the height of the picture in the same unit as position (both in screen coordinates or
/// both in pixels, it only matters that they match). The middle of the picture is
/// size / 2. inverseViewProjection is the inverse of projection * view of the camera the
/// picture was drawn with.
///
/// The ray starts on the near clipping plane, at the point that is drawn exactly at
/// position, and not in the eye. The two differ by the distance of the near plane (0.1 m
/// with the default camera). The ray through the middle of the picture has the direction
/// Camera::forward().
///
/// Throws std::invalid_argument when the width or the height is not greater than 0 (the
/// size of a minimised window is 0: the caller must not ask for a ray then).
Ray screenPointRay(const glm::vec2& position, const glm::vec2& size,
                   const glm::mat4& inverseViewProjection);

} // namespace scene
