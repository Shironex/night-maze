// Frustum: the six planes around what a camera or a light sees, and the test that tells
// whether a box can be seen.
#include "scene/Frustum.hpp"

namespace scene {

namespace {

// The number of axes of the world: a glm::vec3 has the components 0 (x), 1 (y) and 2 (z).
constexpr int AXIS_COUNT = 3;

// The rows of a 4 x 4 matrix by number. Row 3 is the one that computes w.
constexpr int ROW_X = 0;
constexpr int ROW_Y = 1;
constexpr int ROW_Z = 2;
constexpr int ROW_W = 3;

// A box has eight corners: on each of the three axes a corner takes either the min or
// the max of the box, and 2 * 2 * 2 is 8.
constexpr int CORNER_COUNT = 8;

// One row of a matrix. GLM stores a matrix column by column: matrix[column][row]. So
// a row is collected from the four columns, one number from each.
glm::vec4 rowOf(const glm::mat4& matrix, int row) {
    return {matrix[0][row], matrix[1][row], matrix[2][row], matrix[3][row]};
}

// A plane from the four numbers of its equation a*x + b*y + c*z + d = 0, scaled so that
// the normal (a, b, c) has the length 1. Scaling all four numbers by the same factor
// does not move the plane: the equation still holds for the same points. It only
// changes the unit of the sum, which is metres afterwards.
Plane normalizedPlane(const glm::vec4& equation) {
    const glm::vec3 normal{equation};
    const float length = glm::length(normal);
    // A normal without a length has no direction, and dividing by 0 would fill the
    // plane with values that are not numbers. The plane is left with a zero normal and
    // a zero offset: every point then has the distance 0 from it, so it rejects nothing.
    if (length == 0.0F) {
        return {.normal = glm::vec3{0.0F}, .offset = 0.0F};
    }
    return {.normal = normal / length, .offset = equation.w / length};
}

// One corner of a box by its number from 0 to 7. The three lowest bits of the number
// choose, for x, y and z in turn, between the min (bit 0) and the max (bit 1).
glm::vec3 cornerOf(const Aabb& box, int corner) {
    glm::vec3 point{0.0F};
    for (int axis = 0; axis < AXIS_COUNT; ++axis) {
        const bool takesMax = ((corner >> axis) & 1) != 0;
        point[axis] = takesMax ? box.max[axis] : box.min[axis];
    }
    return point;
}

} // namespace

float signedDistance(const Plane& plane, const glm::vec3& point) {
    // The dot product with a unit normal is the length of the part of the point that
    // runs along the normal: how far the point is from the origin in that direction.
    // The offset moves the zero of that measure from the origin onto the plane.
    return glm::dot(plane.normal, point) + plane.offset;
}

Frustum frustumFromMatrix(const glm::mat4& viewProjection) {
    // The method of Gribb and Hartmann: the planes are read straight out of the matrix.
    //
    // The matrix turns a point p of the world (written with a fourth number 1) into clip
    // coordinates. Each of them is one row of the matrix times the point:
    //   x = dot(rowX, p),  y = dot(rowY, p),  z = dot(rowZ, p),  w = dot(rowW, p).
    // The graphics card keeps a point when all of these hold:
    //   -w <= x <= w,   -w <= y <= w,   -w <= z <= w.
    //
    // Take the first one, -w <= x. It is the same as 0 <= w + x, and with the rows put
    // in: 0 <= dot(rowW, p) + dot(rowX, p) = dot(rowW + rowX, p). That is the equation
    // of a plane with the four numbers rowW + rowX, and "not negative" is the side its
    // normal points to. It is the left side of the frustum. The other five follow the
    // same way: x <= w is 0 <= dot(rowW - rowX, p), the right side, and so on.
    //
    // Nothing here asks what kind of projection the matrix holds. For an orthographic
    // one rowW is (0, 0, 0, 1), so w is always 1 and the planes come out parallel in
    // pairs: a box. For a perspective one w is the distance in front of the camera, so
    // the side planes lean and meet in the eye. A point behind such a camera has
    // a negative w and cannot satisfy -w <= x and x <= w at once: it is outside.
    const glm::vec4 rowX = rowOf(viewProjection, ROW_X);
    const glm::vec4 rowY = rowOf(viewProjection, ROW_Y);
    const glm::vec4 rowZ = rowOf(viewProjection, ROW_Z);
    const glm::vec4 rowW = rowOf(viewProjection, ROW_W);

    // In the order the header promises: left, right, bottom, top, near, far. The near
    // plane is -w <= z, because the clip range of z in OpenGL is -w to w. (With the
    // range 0 to w of other systems it would be rowZ alone.)
    return {.planes = {
                normalizedPlane(rowW + rowX),
                normalizedPlane(rowW - rowX),
                normalizedPlane(rowW + rowY),
                normalizedPlane(rowW - rowY),
                normalizedPlane(rowW + rowZ),
                normalizedPlane(rowW - rowZ),
            }};
}

bool canBeSeen(const Frustum& frustum, const Aabb& box) {
    for (const Plane& plane : frustum.planes) {
        // Of the eight corners of the box, the one that reaches farthest in the
        // direction of the normal: the positive vertex. It is found axis by axis without
        // looking at all eight. Where the normal points towards larger x, the corner
        // with the largest x is farther along it, so max.x is taken, otherwise min.x.
        // The same for y and z.
        glm::vec3 positiveVertex{0.0F};
        for (int axis = 0; axis < AXIS_COUNT; ++axis) {
            positiveVertex[axis] = plane.normal[axis] >= 0.0F ? box.max[axis] : box.min[axis];
        }

        // If even that corner is behind the plane, all eight are, and with them every
        // point of the box: it lies completely outside this side of the frustum.
        // "Less than minus the margin" and not "less than 0", see FRUSTUM_MARGIN.
        // A distance that is not a number compares false, so such a box is kept.
        if (signedDistance(plane, positiveVertex) < -FRUSTUM_MARGIN) {
            return false;
        }
    }
    // No single plane has the whole box behind it.
    return true;
}

Aabb transformedBounds(const Aabb& box, const glm::mat4& matrix) {
    // The first corner starts the result, the other seven can only make it larger.
    const glm::vec3 first{matrix * glm::vec4{cornerOf(box, 0), 1.0F}};
    Aabb bounds{.min = first, .max = first};
    for (int corner = 1; corner < CORNER_COUNT; ++corner) {
        // A point is written with a fourth number w = 1, so that the matrix also moves
        // it. A model matrix leaves that w at 1, so there is nothing to divide by.
        const glm::vec3 moved{matrix * glm::vec4{cornerOf(box, corner), 1.0F}};
        // glm::min and glm::max work on the three components separately.
        bounds.min = glm::min(bounds.min, moved);
        bounds.max = glm::max(bounds.max, moved);
    }
    return bounds;
}

Aabb boundsOfBoth(const Aabb& a, const Aabb& b) {
    return {.min = glm::min(a.min, b.min), .max = glm::max(a.max, b.max)};
}

} // namespace scene
