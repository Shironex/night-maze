// Transform: position, rotation and scale of an object, turned into a model matrix.
#pragma once

#include <glm/glm.hpp>

namespace scene {

/// Where an object is, how it is turned and how big it is. Plain data plus one function.
///
/// The model matrix moves a vertex from the object's local space to world space. A vertex
/// is scaled first, then rotated, then translated: the object grows and turns around its
/// own origin and only then goes to its place in the world.
struct Transform {
    /// Position of the object's origin in world space.
    glm::vec3 position{0.0F};

    /// Euler angles in degrees: rotation around the x, y and z axis. Degrees, because
    /// that is what a person types into a panel. The rotations are applied to a vertex
    /// in the order z, then x, then y.
    glm::vec3 rotationDegrees{0.0F};

    /// Scale factor along each axis. 1 keeps the size.
    glm::vec3 scale{1.0F};

    /// The model matrix: translate * rotateY * rotateX * rotateZ * scale. The matrix
    /// nearest to the vertex is applied first, so the expression reads right to left.
    glm::mat4 matrix() const;
};

/// The matrix that takes a normal from the local space of an object to world space: the
/// inverse of the upper left 3 x 3 part of the model matrix, transposed.
///
/// A normal is a direction, so the translation of the model matrix must not move it:
/// that is why only the 3 x 3 part (rotation and scale) is used. That part itself,
/// mat3(model), is right as long as the scale is the same on all three axes. With an
/// unequal scale it is wrong: a normal has to stay perpendicular to its surface, and
/// stretching the object along one axis tilts the surface one way, while stretching the
/// normal along the same axis tilts it the other way. The inverse transpose undoes the
/// stretch for the normal. For a pure rotation it is the rotation itself, so nothing
/// changes for objects that are only turned and moved.
///
/// The result is not of length 1 when the model matrix scales: the shader normalizes.
/// modelMatrix must be invertible (no scale factor of 0).
glm::mat3 normalMatrix(const glm::mat4& modelMatrix);

} // namespace scene
