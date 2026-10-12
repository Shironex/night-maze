// Lighting of the game: the settings of the moon, the flashlight and the point lights.
#include "game/Lighting.hpp"

#include "game/SoundCues.hpp"
#include "gfx/ColorSpace.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace game {

namespace {

// The up direction of the world. The hand holds the flashlight straight below the eye.
constexpr glm::vec3 WORLD_UP{0.0F, 1.0F, 0.0F};

// A vector shorter than this cannot be brought to length 1 (see scene/LightBlock.cpp).
constexpr float MIN_DIRECTION_LENGTH = 0.0001F;

// A hand that holds the flashlight flashlightHandRight to the right of the eye and
// flashlightHandDown below it, where "below" is against the direction up, and aims it at
// the point of the line of view the settings name.
FlashlightPose poseOfHand(const LightingSettings& settings, const glm::vec3& eye,
                          const glm::vec3& forward, const glm::vec3& right, const glm::vec3& up) {
    const glm::vec3 position =
        eye + right * settings.flashlightHandRight - up * settings.flashlightHandDown;

    // The point on the line of view the beam is aimed at.
    const float convergeDistance =
        std::max(settings.flashlightConvergeDistance, MIN_FLASHLIGHT_CONVERGE_DISTANCE);
    const glm::vec3 target = eye + forward * convergeDistance;

    // From the hand to that point. The two fall together in one case only: a camera that
    // looks straight down and a hand exactly as far below the eye as the point is in
    // front of it. A vector of length 0 has no direction, so the beam then simply
    // points where the camera looks.
    const glm::vec3 toTarget = target - position;
    if (glm::length(toTarget) < MIN_DIRECTION_LENGTH) {
        return {.position = position, .direction = forward};
    }
    return {.position = position, .direction = glm::normalize(toTarget)};
}

} // namespace

SpecularModel specularModelOf(LightingMode mode) {
    return mode == LightingMode::BlinnPhong ? SpecularModel::BlinnPhong : SpecularModel::Phong;
}

bool usesNormalMap(const LightingSettings& settings) {
    return settings.normalMapping && settings.mode != LightingMode::Gouraud;
}

glm::vec3 moonDirection(const LightingSettings& settings) {
    return scene::directionFromAngles(settings.moonYawDegrees, settings.moonPitchDegrees);
}

FlashlightPose flashlightPose(const LightingSettings& settings, const glm::vec3& eye,
                              const glm::vec3& forward, const glm::vec3& right) {
    // The hand: to the right of the eye and below it. The right vector of the camera is
    // level and "below" is straight down, so the first offset is the whole horizontal
    // distance from the eye and the second the whole vertical one.
    return poseOfHand(settings, eye, forward, right, WORLD_UP);
}

glm::mat4 heldLampMatrix(const LightingSettings& settings, const glm::vec3& eye,
                         const glm::vec3& forward, const glm::vec3& right) {
    // The hand of the picture: below the eye along the up vector of the camera, so it
    // turns with the look. With a level look that is the hand of the light itself.
    const FlashlightPose hand =
        poseOfHand(settings, eye, forward, right, glm::cross(right, forward));

    // The three axes of the lamp in the world. The model points along its own -Z, so
    // its +Z is the direction against the beam. Its top is at right angles to the beam
    // and to the right vector of the camera, and its own right follows from the two.
    const glm::vec3 back = -hand.direction;
    const glm::vec3 top = glm::normalize(glm::cross(right, hand.direction));
    const glm::vec3 side = glm::cross(hand.direction, top);
    const glm::vec3 butt = hand.position + hand.direction * HELD_LAMP_REACH;

    // A matrix is filled column by column: the three axes, then the place of the origin.
    return {glm::vec4{side, 0.0F}, glm::vec4{top, 0.0F}, glm::vec4{back, 0.0F},
            glm::vec4{butt, 1.0F}};
}

glm::mat4 heldLampProjection(const glm::mat4& sceneProjection) {
    // The two numbers of a perspective matrix that hold the field of view: [1][1] is
    // 1 / tan(half of it) and [0][0] the same divided by the aspect ratio. Both are
    // scaled by the same factor, so the aspect ratio stays.
    const float zoom = 1.0F / std::tan(glm::radians(HELD_LAMP_FOV_DEGREES) / 2.0F);
    const float factor = zoom / sceneProjection[1][1];
    glm::mat4 projection = sceneProjection;
    projection[0][0] *= factor;
    projection[1][1] *= factor;
    return projection;
}

HeldLampGlow heldLampGlow(const LightingSettings& frameLighting, float charge, float flicker) {
    HeldLampGlow glow;
    if (frameLighting.flashlightOn) {
        glow.lens = gfx::srgbToLinear(frameLighting.flashlightColor) *
                    (frameLighting.flashlightIntensity * HELD_LAMP_LENS_GLOW);
    }
    glow.window = gfx::srgbToLinear(frameLighting.pointColor) *
                  (HELD_LAMP_WINDOW_GLOW * std::clamp(charge, 0.0F, 1.0F) * flicker);
    return glow;
}

scene::LightSet heldLampLights(const scene::LightSet& frameLights,
                               const LightingSettings& frameLighting, const glm::mat4& lampMatrix) {
    scene::LightSet lights = frameLights;
    lights.spotEnabled = false;
    if (!frameLighting.flashlightOn) {
        return lights;
    }
    // One more point light, or the last one of a full list: the list is sorted from the
    // nearest to the farthest (nearestPointLightSpots), and the last is the faintest on
    // a lamp that is a step in front of the eye.
    const int index = std::clamp(lights.pointCount, 0, scene::MAX_POINT_LIGHTS - 1);
    lights.points[static_cast<std::size_t>(index)] = {
        .position = glm::vec3{lampMatrix * glm::vec4{HELD_LAMP_SPILL_PLACE, 1.0F}},
        .color = gfx::srgbToLinear(frameLighting.flashlightColor),
        .intensity = frameLighting.flashlightIntensity * HELD_LAMP_SPILL,
        .attenuation = scene::attenuationForRadius(HELD_LAMP_SPILL_RADIUS),
    };
    lights.pointCount = index + 1;
    return lights;
}

void advanceHeldLampSway(HeldLampSway& sway, const HeldLampSwayStep& step) {
    // How far the camera turned since the frame before. The yaw is not kept inside one
    // turn, so the difference is taken the short way round. A turn too large for one
    // frame is a cut (a new round puts the camera elsewhere): nothing falls behind it.
    glm::vec2 turned{std::remainder(step.yawDegrees - sway.yawDegrees, 360.0F),
                     step.pitchDegrees - sway.pitchDegrees};
    if (glm::length(turned) > HELD_LAMP_CUT_DEGREES) {
        turned = glm::vec2{0.0F};
    }
    sway.yawDegrees = step.yawDegrees;
    sway.pitchDegrees = step.pitchDegrees;
    if (step.seconds <= 0.0F) {
        return;
    }

    // A jump of the player (a new round) is not a walk: no step is faster than a sprint.
    const float speed = std::clamp(step.speed, 0.0F, std::max(step.sprintSpeed, 0.0F));
    const float walkTarget = step.walkSpeed > 0.0F ? std::min(speed / step.walkSpeed, 1.0F) : 0.0F;
    const float sprintTarget = speed > (step.walkSpeed + step.sprintSpeed) / 2.0F ? 1.0F : 0.0F;
    const float blendStep = step.seconds / HELD_LAMP_SWAY_BLEND_SECONDS;
    sway.walkAmount += std::clamp(walkTarget - sway.walkAmount, -blendStep, blendStep);
    sway.sprintAmount += std::clamp(sprintTarget - sway.sprintAmount, -blendStep, blendStep);

    // The steps the feet took in this frame, with the stride of the sound of the steps,
    // so the lamp swings as often as a step is heard.
    const float stride = glm::mix(FOOTSTEP_WALK_METRES, FOOTSTEP_SPRINT_METRES, sway.sprintAmount);
    sway.steps = std::fmod(sway.steps + speed * step.seconds / stride, 2.0F);

    // The lamp falls behind by a share of the turn and catches up: what is left of the
    // lag shrinks by the same part in every moment, the faster the shorter
    // HELD_LAMP_LAG_SECONDS is.
    const glm::vec2 lag = (sway.lagDegrees + turned * HELD_LAMP_LAG_SHARE) *
                          std::exp(-step.seconds / HELD_LAMP_LAG_SECONDS);
    sway.lagDegrees = glm::clamp(lag, glm::vec2{-HELD_LAMP_MAX_LAG_DEGREES},
                                 glm::vec2{HELD_LAMP_MAX_LAG_DEGREES});
}

HeldLampPose heldLampSwayPose(const HeldLampSway& sway) {
    const float walk = std::clamp(sway.walkAmount, 0.0F, 1.0F);
    const float sprint = std::clamp(sway.sprintAmount, 0.0F, 1.0F);
    // Half a swing per step: to one side with one foot and to the other with the next.
    // The lamp is lowest when a foot comes down, between two swings, and back at its
    // height in the middle of a step: one smooth dip per step.
    const float swing = std::sin(glm::pi<float>() * sway.steps);
    const float side = glm::mix(HELD_LAMP_WALK_SIDE, HELD_LAMP_SPRINT_SIDE, sprint);
    const float dip = glm::mix(HELD_LAMP_WALK_DIP, HELD_LAMP_SPRINT_DIP, sprint);
    const float roll = glm::mix(HELD_LAMP_WALK_ROLL_DEGREES, HELD_LAMP_SPRINT_ROLL_DEGREES, sprint);
    return {.offset = {walk * side * swing, walk * dip * (swing * swing - 1.0F), 0.0F},
            // Behind a camera that turned upwards the lens hangs low, and behind one that
            // turned to the right it points to the left.
            .pitchDegrees = -sway.lagDegrees.y,
            .yawDegrees = sway.lagDegrees.x,
            // The top tips to the side the lamp swings away from, like a hand that
            // carries it.
            .rollDegrees = walk * roll * swing};
}

glm::mat4 heldLampSwayMatrix(const HeldLampPose& pose) {
    // Read from the right, as a point of the model meets them: the three turns about the
    // origin of the model, which is its butt, and then the move by the offset.
    glm::mat4 matrix = glm::translate(glm::mat4{1.0F}, pose.offset);
    matrix = glm::rotate(matrix, glm::radians(pose.yawDegrees), glm::vec3{0.0F, 1.0F, 0.0F});
    matrix = glm::rotate(matrix, glm::radians(pose.pitchDegrees), glm::vec3{1.0F, 0.0F, 0.0F});
    return glm::rotate(matrix, glm::radians(pose.rollDegrees), glm::vec3{0.0F, 0.0F, 1.0F});
}

std::vector<PointLightSpot> nearestPointLights(std::span<const glm::vec3> positions,
                                               const glm::vec3& eye, int maxCount) {
    // Every position becomes a spot of full strength with the look of the settings: the
    // light of a crystal. The choice itself is the one for spots.
    std::vector<PointLightSpot> lights;
    lights.reserve(positions.size());
    for (const glm::vec3& position : positions) {
        lights.push_back({.position = position});
    }
    return nearestPointLightSpots(lights, eye, maxCount);
}

std::vector<PointLightSpot> nearestPointLightSpots(std::span<const PointLightSpot> lights,
                                                   const glm::vec3& eye, int maxCount) {
    if (maxCount < 1) {
        return {};
    }

    // Every light with its distance from the eye, sorted from the nearest to the
    // farthest. stable_sort keeps two lights at the same distance in the order they came
    // in, so the same input always gives the same result.
    struct Candidate {
        PointLightSpot light;
        float distance;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(lights.size());
    for (const PointLightSpot& light : lights) {
        candidates.push_back({.light = light, .distance = glm::distance(light.position, eye)});
    }
    std::ranges::stable_sort(
        candidates, [](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });

    // The edge of the set: the distance of the first light that is left out. With no
    // light left out there is no edge, and every light keeps its full strength.
    const auto count = static_cast<std::size_t>(maxCount);
    const bool someLeftOut = candidates.size() > count;
    const float edge = someLeftOut ? candidates[count].distance : 0.0F;

    std::vector<PointLightSpot> chosen;
    chosen.reserve(std::min(candidates.size(), count));
    for (std::size_t i = 0; i < candidates.size() && i < count; ++i) {
        float fade = 1.0F;
        if (someLeftOut) {
            // 0 at the edge, 1 from POINT_LIGHT_FADE_DISTANCE inside of it.
            fade =
                std::clamp((edge - candidates[i].distance) / POINT_LIGHT_FADE_DISTANCE, 0.0F, 1.0F);
        }
        // The spot as it came in. Only its strength changes: a light that was already
        // dim (the ember of the gate) fades from there.
        PointLightSpot light = candidates[i].light;
        light.strength *= fade;
        chosen.push_back(light);
    }
    return chosen;
}

scene::LightSet buildLightSet(const LightingSettings& settings, const FlashlightPose& flashlight,
                              std::span<const PointLightSpot> pointLights) {
    // The colours of the settings are sRGB values: they are picked on the screen. The
    // shaders compute with linear light, so this function is the one place where the
    // four colours are converted. The intensities are plain factors and stay as they
    // are.
    scene::LightSet lights;
    lights.ambient = gfx::srgbToLinear(settings.ambient);

    lights.directional = {
        .direction = moonDirection(settings),
        .color = gfx::srgbToLinear(settings.moonColor),
        .intensity = settings.moonIntensity,
    };

    // The flashlight is held in the hand and aimed at a point in front of the eye: the
    // caller has computed both (flashlightPose), once for the light and its shadow map.
    lights.spot = {
        .position = flashlight.position,
        .direction = flashlight.direction,
        .color = gfx::srgbToLinear(settings.flashlightColor),
        .intensity = settings.flashlightIntensity,
        .attenuation = scene::attenuationForRadius(settings.flashlightRange),
        // A cone cannot be wider inside than outside. The panel keeps the two angles in
        // order, this line keeps them in order whoever sets them.
        .innerConeDegrees =
            std::min(settings.flashlightInnerDegrees, settings.flashlightOuterDegrees),
        .outerConeDegrees = settings.flashlightOuterDegrees,
    };
    lights.spotEnabled = settings.flashlightOn;

    // The lights of the crystals share one colour and one radius. The place differs, and
    // the intensity where a light is fading out of the set (nearestPointLights). A spot
    // with a look of its own (the lamp of the gate) brings its three numbers with it.
    const scene::Attenuation pointAttenuation = scene::attenuationForRadius(settings.pointRadius);
    const glm::vec3 pointColor = gfx::srgbToLinear(settings.pointColor);
    const std::size_t pointCount =
        std::min(pointLights.size(), static_cast<std::size_t>(scene::MAX_POINT_LIGHTS));
    for (std::size_t i = 0; i < pointCount; ++i) {
        const PointLightSpot& spot = pointLights[i];
        if (spot.ownLook) {
            lights.points[i] = {
                .position = spot.position,
                .color = gfx::srgbToLinear(spot.color),
                .intensity = spot.intensity * spot.strength,
                .attenuation = scene::attenuationForRadius(spot.radius),
            };
            continue;
        }
        lights.points[i] = {
            .position = spot.position,
            .color = pointColor,
            .intensity = settings.pointIntensity * spot.strength,
            .attenuation = pointAttenuation,
        };
    }
    lights.pointCount = static_cast<int>(pointCount);
    return lights;
}

} // namespace game
