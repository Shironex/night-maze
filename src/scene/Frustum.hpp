// Frustum: the six planes around what a camera or a light sees, and the test that tells
// whether a box can be seen.
#pragma once

#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>

namespace scene {

// Plain math without OpenGL, so tests can use it. A camera sees a piece of the world
// that is cut off on six sides: left, right, bottom, top, near and far. With
// a perspective projection that piece is a pyramid without its tip, which is what the
// word frustum means. With an orthographic projection it is a box. Whatever lies
// completely outside of it leaves no pixel in the picture, so it need not be handed to
// the graphics card at all. Leaving such things out is called view frustum culling.

/// A plane, written as the numbers of its equation: a point p lies on the plane when
/// dot(normal, p) + offset is 0. For any other point that sum is its signed distance
/// from the plane (signedDistance), as long as normal has the length 1.
///
/// Plain data, like Aabb and Ray: no OpenGL and no input.
struct Plane {
    /// The direction the plane faces. The side it points to is the "inside" for the
    /// planes of a Frustum.
    glm::vec3 normal{0.0F, 1.0F, 0.0F};

    /// How far the plane is from the origin of the world, measured against the normal:
    /// the plane y = 2 with the normal (0, 1, 0) has the offset -2.
    float offset = 0.0F;
};

/// The distance of point from plane in metres, when the normal of the plane has the
/// length 1: positive on the side the normal points to, negative on the other side and
/// 0 on the plane itself.
float signedDistance(const Plane& plane, const glm::vec3& point);

/// A frustum has six sides.
constexpr std::size_t FRUSTUM_PLANE_COUNT = 6;

/// The six planes of a frustum, in the order left, right, bottom, top, near, far. Every
/// normal points INTO the frustum and has the length 1, so a point is inside when its
/// signed distance from all six planes is not negative.
struct Frustum {
    std::array<Plane, FRUSTUM_PLANE_COUNT> planes;
};

/// A box is only left out when it lies farther than this outside a plane, in metres
/// (1 cm). The planes are computed here with floats, and the graphics card computes the
/// position of every vertex with its own floats: the two can differ in the last digits.
/// The margin is hundreds of times larger than that difference at the sizes of this
/// game (a float holds a position 200 m from the origin to about 0.02 mm), so a box
/// that touches the frustum is never left out by a rounding error. It costs nothing
/// that can be seen: a box up to 1 cm outside is drawn although no pixel of it shows.
constexpr float FRUSTUM_MARGIN = 0.01F;

/// The frustum of a matrix that takes a point of the world to clip space:
/// projection * view of a camera (Camera::projectionMatrix, Camera::viewMatrix) or
/// LightSpace::matrix() of a light. It works for a perspective and for an orthographic
/// projection alike, because it only uses the rule both share: a point is inside when
/// its clip coordinates x, y and z all lie between -w and w (the OpenGL convention,
/// which GLM follows).
///
/// The planes are in world space, so they are compared with boxes in world space, and
/// no box has to be multiplied by a matrix.
///
/// A matrix that describes no frustum (all zeros, for example) gives planes with
/// a normal of length 0. Such a plane rejects nothing: canBeSeen is then true for every
/// box, which is the safe answer.
Frustum frustumFromMatrix(const glm::mat4& viewProjection);

/// False when the box lies completely outside the frustum, by more than FRUSTUM_MARGIN:
/// nothing of it can be in the picture. True otherwise.
///
/// The answer true is not a promise that the box is visible. The test asks the six
/// planes one after another whether the whole box lies behind one of them. A box near
/// a corner of the frustum can lie outside of it and still be partly in front of every
/// single plane: it is kept. That is on purpose. A box that is kept for nothing costs
/// one draw call whose triangles the card throws away. A box that is left out by
/// mistake is a hole in the picture.
bool canBeSeen(const Frustum& frustum, const Aabb& box);

/// The smallest axis-aligned box around box after matrix has moved, turned or scaled
/// it. The eight corners are sent through the matrix and the smallest and the largest
/// x, y and z among them are the corners of the result.
///
/// This is how the bounds of a model (its box in its own local space) become the box of
/// one object in the world: matrix is the model matrix of that object. A box that is
/// turned no longer has its edges along the axes, so the result is larger than the
/// turned box. It still holds every vertex of the object, and that is all the test
/// above needs. matrix must be an affine matrix (no perspective), as every model matrix
/// is.
Aabb transformedBounds(const Aabb& box, const glm::mat4& matrix);

/// The smallest box that holds both boxes.
Aabb boundsOfBoth(const Aabb& a, const Aabb& b);

} // namespace scene
