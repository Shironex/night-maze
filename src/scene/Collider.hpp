// Collider: axis-aligned boxes and spheres, overlap tests and movement that slides along
// obstacles.
// See docs/modules/scene/collision.md
#pragma once

#include <glm/glm.hpp>

#include <span>

namespace scene {

/// Contact closer than this (in metres, 1 mm) counts as touching, not as overlapping, when
/// a box is moved by moveAndSlide. A float cannot hold every position exactly: after the
/// caller adds the allowed displacement to a position, a face that should rest exactly on
/// a wall may end up a few millionths of a metre inside it. Without the tolerance that
/// rounding error would make the wall block movement along itself.
constexpr float CONTACT_TOLERANCE = 0.001F;

/// An axis-aligned bounding box (AABB): a box whose edges are parallel to the X, Y and Z
/// axes of the world. It cannot be rotated, which is why two corners describe it fully.
///
/// Plain data, like Transform and Camera: no OpenGL and no input.
struct Aabb {
    /// The corner with the smallest x, y and z.
    glm::vec3 min{0.0F};

    /// The corner with the largest x, y and z. Every component must not be smaller than
    /// the same component of min.
    glm::vec3 max{0.0F};

    /// Builds a box from its centre and its half extents: half of the width (x), half of
    /// the height (y) and half of the depth (z).
    static Aabb fromCenter(const glm::vec3& center, const glm::vec3& halfExtents);
};

/// True when the two boxes share some volume. Boxes that only touch (a face, an edge or
/// a corner of one lies exactly on the other) do not overlap.
bool overlaps(const Aabb& a, const Aabb& b);

/// A sphere: every point within radius of center. The second kind of collision shape,
/// used for things that are picked up or entered and never block the way: walking close
/// enough is all that counts, from whatever side.
///
/// Plain data, like Aabb.
struct Sphere {
    /// The middle of the sphere, in world space.
    glm::vec3 center{0.0F};

    /// Distance from the middle to the surface, in metres. Must not be negative.
    float radius = 0.0F;
};

/// True when the two spheres share some volume: their centres are closer than the sum of
/// their radii. Spheres that only touch (the distance is exactly that sum) do not overlap.
bool overlaps(const Sphere& a, const Sphere& b);

/// True when the sphere and the box share some volume: the point of the box closest to
/// the centre of the sphere is nearer than the radius. A sphere that only touches a face,
/// an edge or a corner of the box does not overlap it.
bool overlaps(const Sphere& sphere, const Aabb& box);

/// The point of the box that is closest to point. A point inside the box is its own
/// closest point.
glm::vec3 closestPoint(const Aabb& box, const glm::vec3& point);

/// Moves a box by displacement among static obstacles and returns the part of the
/// displacement that is allowed. The caller adds the result to the position of the object.
///
/// The three axes are handled one after another: x, then z, then y. An obstacle that
/// stops the box on one axis does not take away the movement on the others, so a box
/// pushed diagonally into a wall keeps sliding along it.
///
/// The box is expected to start outside the obstacles. An obstacle that the box is
/// already inside of does not hold it: the box is free to leave.
glm::vec3 moveAndSlide(const Aabb& mover, const glm::vec3& displacement,
                       std::span<const Aabb> obstacles);

} // namespace scene
