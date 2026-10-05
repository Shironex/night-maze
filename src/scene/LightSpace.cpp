// LightSpace: the scene as a light sees it, the two matrices a shadow map is drawn and read with.
// See docs/modules/renderer/shadows.md
#include "scene/LightSpace.hpp"

#include "scene/Collider.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace scene {

namespace {

// A direction shorter than this cannot be brought to length 1 (see scene/LightBlock.cpp).
constexpr float MIN_DIRECTION_LENGTH = 0.0001F;

// What a direction of length 0 is replaced by: straight down.
constexpr glm::vec3 FALLBACK_DIRECTION{0.0F, -1.0F, 0.0F};

// The two "up" directions of the view of a light: the up of the world, and the one for
// a light that shines along the up of the world.
constexpr glm::vec3 WORLD_UP{0.0F, 1.0F, 0.0F};
constexpr glm::vec3 UP_FOR_VERTICAL_LIGHT{0.0F, 0.0F, -1.0F};

// A box has eight corners.
constexpr std::size_t CORNER_COUNT = 8;

// The eight corners of a box: every combination of the smallest and the largest x, y and z.
std::array<glm::vec3, CORNER_COUNT> cornersOf(const Aabb& box) {
    return {
        glm::vec3{box.min.x, box.min.y, box.min.z}, glm::vec3{box.max.x, box.min.y, box.min.z},
        glm::vec3{box.min.x, box.max.y, box.min.z}, glm::vec3{box.max.x, box.max.y, box.min.z},
        glm::vec3{box.min.x, box.min.y, box.max.z}, glm::vec3{box.max.x, box.min.y, box.max.z},
        glm::vec3{box.min.x, box.max.y, box.max.z}, glm::vec3{box.max.x, box.max.y, box.max.z},
    };
}

} // namespace

LightSpace directionalLightSpace(const Aabb& bounds, const glm::vec3& lightDirection) {
    const glm::vec3 direction = glm::length(lightDirection) < MIN_DIRECTION_LENGTH
                                    ? FALLBACK_DIRECTION
                                    : glm::normalize(lightDirection);

    // The view of the light: it looks at the middle of the box, along the direction its
    // rays travel. A directional light has no position, so the eye is simply put one
    // step before the middle. Where exactly it stands along the ray does not matter:
    // the near and the far plane are measured from it below.
    const glm::vec3 up =
        std::abs(direction.y) > VERTICAL_DIRECTION_LIMIT ? UP_FOR_VERTICAL_LIGHT : WORLD_UP;
    const glm::vec3 center = (bounds.min + bounds.max) * 0.5F;
    const glm::mat4 view = glm::lookAt(center - direction, center, up);

    // The box in the space of the light: the smallest and the largest coordinate of the
    // eight corners along each of its three axes.
    const std::array<glm::vec3, CORNER_COUNT> corners = cornersOf(bounds);
    glm::vec3 smallest = glm::vec3{view * glm::vec4{corners[0], 1.0F}};
    glm::vec3 largest = smallest;
    for (const glm::vec3& corner : corners) {
        const glm::vec3 inLightSpace = glm::vec3{view * glm::vec4{corner, 1.0F}};
        smallest = glm::min(smallest, inLightSpace);
        largest = glm::max(largest, inLightSpace);
    }
    smallest -= glm::vec3{LIGHT_BOX_MARGIN};
    largest += glm::vec3{LIGHT_BOX_MARGIN};

    // An orthographic projection: the box is scaled to the cube from -1 to 1, without
    // any perspective. A view looks along -Z, so the corner nearest to the light has
    // the LARGEST z, and glm::ortho wants the two planes as distances in front of the
    // eye: hence the minus signs.
    const float nearPlane = -largest.z;
    const float farPlane = -smallest.z;
    return {
        .view = view,
        .projection = glm::ortho(smallest.x, largest.x, smallest.y, largest.y, nearPlane, farPlane),
        .extent = largest - smallest,
    };
}

glm::vec3 shadowMapCoordinates(const glm::mat4& lightSpaceMatrix, const glm::vec3& worldPosition) {
    const glm::vec4 clip = lightSpaceMatrix * glm::vec4{worldPosition, 1.0F};
    // The perspective division. For an orthographic projection w is 1 and nothing
    // changes. It is done all the same, so the function is right for every light.
    const glm::vec3 ndc = glm::vec3{clip} / clip.w;
    // Normalised device coordinates run from -1 to 1, texture coordinates and stored
    // depths from 0 to 1.
    return ndc * 0.5F + 0.5F;
}

} // namespace scene
