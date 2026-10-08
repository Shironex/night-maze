// Camera: a position and two angles, turned into the view and projection matrices.
#include "scene/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace scene {

namespace {

// One full turn. Yaw is kept below it so that the number stays readable.
constexpr float FULL_TURN_DEGREES = 360.0F;

} // namespace

glm::vec3 Camera::forward() const {
    // The standard library takes angles in radians.
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);

    // Pitch splits the unit vector into a vertical part, sin(pitch), and a horizontal
    // part of length cos(pitch). Yaw turns the horizontal part from -Z (yaw 0) towards
    // +X (yaw 90). The length of the result is always 1.
    const float horizontal = std::cos(pitch);
    return {horizontal * std::sin(yaw), std::sin(pitch), -horizontal * std::cos(yaw)};
}

glm::vec3 Camera::right() const {
    // The cross product is perpendicular to both vectors. Its length is cos(pitch), not
    // 1, so it has to be normalized. The pitch limit keeps that length above zero.
    return glm::normalize(glm::cross(forward(), WORLD_UP));
}

void Camera::rotate(float yawDeltaDegrees, float pitchDeltaDegrees) {
    yawDegrees += yawDeltaDegrees;
    // Take away the whole turns: floor gives their number, also for a negative angle.
    // 370 becomes 10 and -10 becomes 350, the direction stays the same.
    yawDegrees -= FULL_TURN_DEGREES * std::floor(yawDegrees / FULL_TURN_DEGREES);

    pitchDegrees =
        std::clamp(pitchDegrees + pitchDeltaDegrees, -MAX_PITCH_DEGREES, MAX_PITCH_DEGREES);
}

glm::mat4 Camera::viewMatrix(const glm::vec3& eye) const {
    // lookAt wants a point to look at, not a direction: one step forward from the eye.
    return glm::lookAt(eye, eye + forward(), WORLD_UP);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    // GLM takes the field of view in radians.
    return glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane);
}

} // namespace scene
