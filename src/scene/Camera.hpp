// Camera: a position and two angles, turned into the view and projection matrices.
#pragma once

#include <glm/glm.hpp>

namespace scene {

/// A first person camera: where it stands, where it looks and how wide it sees.
///
/// Plain data plus math. It knows nothing about the keyboard, the mouse or time: the code
/// that owns the camera changes the fields and asks for the matrices.
///
/// Conventions: right-handed, Y up, -Z forward. With yaw 0 and pitch 0 the camera looks
/// along -Z.
struct Camera {
    /// The up direction of the world.
    static constexpr glm::vec3 WORLD_UP{0.0F, 1.0F, 0.0F};

    /// Largest pitch in either direction, in degrees. Just under 90: looking straight up
    /// or down would make the view direction parallel to WORLD_UP, and then the view
    /// matrix cannot tell which way is "right".
    static constexpr float MAX_PITCH_DEGREES = 89.0F;

    /// Position in world space. Three units in front of the origin, on the +Z side, so
    /// that the default camera looks at the origin.
    glm::vec3 position{0.0F, 0.0F, 3.0F};

    /// Turn to the left or right, in degrees, like a compass seen from above:
    /// 0 looks along -Z, 90 along +X, 180 along +Z, 270 along -X.
    float yawDegrees = 0.0F;

    /// Look up (positive) or down (negative), in degrees. 0 is level.
    float pitchDegrees = 0.0F;

    /// Vertical field of view in degrees: the angle between the top and the bottom edge
    /// of the picture.
    float fovDegrees = 60.0F;

    /// Distance to the near clipping plane. Must be greater than 0. Nothing closer to
    /// the camera is drawn.
    float nearPlane = 0.1F;

    /// Distance to the far clipping plane. Nothing farther from the camera is drawn.
    float farPlane = 100.0F;

    /// Unit vector the camera looks along, computed from yaw and pitch.
    glm::vec3 forward() const;

    /// Unit vector to the right of the camera. It is always horizontal.
    glm::vec3 right() const;

    /// Adds to yaw and pitch. Yaw is wrapped into the range from 0 to 360, pitch is kept
    /// between -MAX_PITCH_DEGREES and MAX_PITCH_DEGREES.
    void rotate(float yawDeltaDegrees, float pitchDeltaDegrees);

    /// The view matrix (world space to view space) for a camera standing at eye and
    /// looking along forward(). The eye is a parameter because a frame may be drawn from
    /// a point between two simulation steps, while position holds the simulation state.
    glm::mat4 viewMatrix(const glm::vec3& eye) const;

    /// The perspective projection matrix (view space to clip space). aspectRatio is the
    /// width of the framebuffer divided by its height.
    glm::mat4 projectionMatrix(float aspectRatio) const;
};

} // namespace scene
