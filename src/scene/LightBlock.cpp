// LightBlock: the bytes of the uniform block the lit shaders read the lights from.
// See docs/modules/gfx/uniform-buffers.md
#include "scene/LightBlock.hpp"

#include <algorithm>
#include <cstddef>

namespace scene {

namespace {

// A direction shorter than this cannot be brought to length 1: dividing by a length of
// (almost) 0 gives numbers that are not numbers (NaN), and one NaN in the block turns
// every lit pixel black.
constexpr float MIN_DIRECTION_LENGTH = 0.0001F;

// What a direction of length 0 is replaced by: straight down.
constexpr glm::vec3 FALLBACK_DIRECTION{0.0F, -1.0F, 0.0F};

// Value of the "on" switch of the spot light in the block. The block has no bool on
// purpose: a C++ bool is 1 byte, a std140 bool is 4.
constexpr float SWITCH_ON = 1.0F;
constexpr float SWITCH_OFF = 0.0F;

// direction with length 1.
glm::vec3 unitDirection(const glm::vec3& direction) {
    if (glm::length(direction) < MIN_DIRECTION_LENGTH) {
        return FALLBACK_DIRECTION;
    }
    return glm::normalize(direction);
}

// The three terms in the order the shader reads them: x, y, z.
glm::vec4 attenuationTerms(const Attenuation& attenuation) {
    return {attenuation.constant, attenuation.linear, attenuation.quadratic, 0.0F};
}

} // namespace

LightBlockData packLightBlock(const LightSet& lights, const glm::vec3& cameraPosition) {
    // The empty braces set every byte of the struct to zero first, also the padding and
    // the point lights that are not in use.
    LightBlockData block{};

    // glm::vec4{vec3, w}: the three floats of the vec3 followed by the fourth one.
    block.cameraPosition = glm::vec4{cameraPosition, 0.0F};
    block.ambient = glm::vec4{lights.ambient, 0.0F};

    block.directionalDirection = glm::vec4{unitDirection(lights.directional.direction), 0.0F};
    block.directionalColor = glm::vec4{lights.directional.color, lights.directional.intensity};

    const SpotLight& spot = lights.spot;
    const ConeCosines cone = coneCosines(spot.innerConeDegrees, spot.outerConeDegrees);
    block.spotPosition = glm::vec4{spot.position, 0.0F};
    block.spotDirection = glm::vec4{unitDirection(spot.direction), 0.0F};
    block.spotColor = glm::vec4{spot.color, spot.intensity};
    block.spotAttenuation = attenuationTerms(spot.attenuation);
    block.spotCone = {cone.inner, cone.outer, lights.spotEnabled ? SWITCH_ON : SWITCH_OFF, 0.0F};

    // The count comes from outside, the array has a fixed length: never read past it.
    const int pointCount = std::clamp(lights.pointCount, 0, MAX_POINT_LIGHTS);
    block.pointCount = pointCount;
    for (int i = 0; i < pointCount; ++i) {
        const auto index = static_cast<std::size_t>(i);
        const PointLight& point = lights.points[index];
        block.points[index] = {
            .position = glm::vec4{point.position, 0.0F},
            .color = glm::vec4{point.color, point.intensity},
            .attenuation = attenuationTerms(point.attenuation),
        };
    }
    return block;
}

} // namespace scene
