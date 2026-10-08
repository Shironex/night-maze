// Lights: a directional light, point lights and a spot light as plain data, plus their math.
#include "scene/Light.hpp"

#include <algorithm>
#include <cmath>

namespace scene {

namespace {

// The divisor of the attenuation formula at the radius of a light is
// 1 + RADIUS_LINEAR_PART + RADIUS_QUADRATIC_PART = 20, the inverse of BRIGHTNESS_AT_RADIUS.
// The split between the two parts shapes the curve: the linear part makes the light fall
// off quickly close to its source, the quadratic part brings it down towards the radius.
// Half way to the radius the light has 1 / (1 + 1 + 4.25) = 16 % of its brightness.
constexpr float RADIUS_LINEAR_PART = 2.0F;
constexpr float RADIUS_QUADRATIC_PART = 17.0F;

} // namespace

Attenuation attenuationForRadius(float radius) {
    // Dividing by a radius of 0 is not possible, and a negative one has no meaning.
    if (radius <= 0.0F) {
        return {};
    }
    return {
        .constant = 1.0F,
        .linear = RADIUS_LINEAR_PART / radius,
        .quadratic = RADIUS_QUADRATIC_PART / (radius * radius),
    };
}

float attenuationFactor(const Attenuation& attenuation, float distance) {
    return 1.0F / (attenuation.constant + attenuation.linear * distance +
                   attenuation.quadratic * distance * distance);
}

ConeCosines coneCosines(float innerDegrees, float outerDegrees) {
    // The standard library takes angles in radians.
    const float outer = std::cos(glm::radians(outerDegrees));
    const float inner = std::cos(glm::radians(innerDegrees));
    return {
        .inner = std::max(inner, outer + MIN_CONE_COSINE_GAP),
        .outer = outer,
    };
}

float spotFactor(const ConeCosines& cone, float cosAngle) {
    // How far cosAngle is on the way from the outer cosine (0) to the inner one (1).
    // Comparing cosines instead of angles saves the shader an acos per fragment.
    return std::clamp((cosAngle - cone.outer) / (cone.inner - cone.outer), 0.0F, 1.0F);
}

glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees) {
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);

    // The same formula as scene::Camera::forward: pitch splits the unit vector into
    // a vertical part, sin(pitch), and a horizontal part of length cos(pitch), and yaw
    // turns the horizontal part from -Z (yaw 0) towards +X (yaw 90).
    const float horizontal = std::cos(pitch);
    return {horizontal * std::sin(yaw), std::sin(pitch), -horizontal * std::cos(yaw)};
}

} // namespace scene
