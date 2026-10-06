// Environment mapping: the settings of the reflections of the sky on the crystals and
// the puddles, and the formulas of a reflected and of a refracted ray.
// See docs/modules/renderer/env-mapping.md
#include "game/EnvironmentMapping.hpp"

#include <algorithm>
#include <cmath>

namespace game {

glm::vec3 reflectDirection(const glm::vec3& incident, const glm::vec3& normal) {
    return incident - 2.0F * glm::dot(normal, incident) * normal;
}

glm::vec3 refractDirection(const glm::vec3& incident, const glm::vec3& normal, float ratio) {
    // The cosine of the angle between the ray and the normal, negative because the ray
    // travels against the normal.
    const float cosine = glm::dot(normal, incident);
    // 1 - cosine * cosine is the squared sine of the angle before the bend. Snell's law
    // multiplies the sine by the ratio, so the squared sine after the bend is that
    // times ratio * ratio, and k is the squared cosine after the bend.
    const float k = 1.0F - ratio * ratio * (1.0F - cosine * cosine);
    if (k < 0.0F) {
        // A sine above 1 does not exist: total internal reflection.
        return glm::vec3{0.0F};
    }
    return ratio * incident - (ratio * cosine + std::sqrt(k)) * normal;
}

glm::vec3 refractOrReflect(const glm::vec3& incident, const glm::vec3& normal, float ratio) {
    const glm::vec3 refracted = refractDirection(incident, normal, ratio);
    // A refracted ray of two unit vectors has length 1. Only the "no ray" answer is
    // the zero vector, so comparing with it exactly is safe.
    if (refracted == glm::vec3{0.0F}) {
        return reflectDirection(incident, normal);
    }
    return refracted;
}

float fresnelSchlick(float cosine, float straightOn) {
    const float facing = std::clamp(cosine, 0.0F, 1.0F);
    return straightOn + (1.0F - straightOn) * std::pow(1.0F - facing, SCHLICK_EXPONENT);
}

} // namespace game
