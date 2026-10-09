// Village: the village on the ridge beyond the gate, where it stands in the sky, and the
// camera that rises over the gate to show it after a night of the campaign is won.
#include "game/Village.hpp"

#include "game/GateLamp.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace game {

namespace {

constexpr float FULL_TURN_DEGREES = 360.0F;
constexpr float HALF_TURN_DEGREES = 180.0F;
constexpr float QUARTER_TURN_DEGREES = 90.0F;

// 0 below 0, 1 above 1, and in between a curve that is level at both ends (smoothstep).
float easeInOut(float amount) {
    const float t = std::clamp(amount, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

// An angle brought into 0 to 360.
float wrapDegrees(float degrees) {
    const float wrapped = std::fmod(degrees, FULL_TURN_DEGREES);
    return wrapped < 0.0F ? wrapped + FULL_TURN_DEGREES : wrapped;
}

} // namespace

Direction villageSide(const MazeWorld& world) {
    return opposite(gateSide(world));
}

float villageYawDegrees(Direction side) {
    // The order of the enum is the order of the compass: North, East, South, West.
    return static_cast<float>(static_cast<int>(side)) * QUARTER_TURN_DEGREES;
}

glm::mat4 villageMatrix(Direction side, float farPlane, float elevationDegrees) {
    // Read from the right: the model is scaled about the eye, lifted about the axis that
    // runs along the lane, and turned to its side. The model lies to the north (-Z), and
    // a positive turn about Y would take north to the west, hence the minus sign.
    glm::mat4 matrix{1.0F};
    matrix = glm::rotate(matrix, glm::radians(-villageYawDegrees(side)), {0.0F, 1.0F, 0.0F});
    matrix = glm::rotate(matrix, glm::radians(elevationDegrees), {1.0F, 0.0F, 0.0F});
    return glm::scale(matrix, glm::vec3{VILLAGE_FAR_SHARE * farPlane / VILLAGE_REACH});
}

VillageRevealPose villageRevealPose(const VillageRevealPose& start, const glm::vec3& exitGround,
                                    Direction side, float asideDegrees, float seconds) {
    const float amount = easeInOut(seconds / VILLAGE_REVEAL_SECONDS);
    const float endYaw = villageYawDegrees(side) + asideDegrees;
    // The short way round: the difference of the two angles, brought into -180 to 180.
    const float turn =
        wrapDegrees(endYaw - start.yawDegrees + HALF_TURN_DEGREES) - HALF_TURN_DEGREES;

    VillageRevealPose pose;
    pose.eye =
        glm::mix(start.eye, exitGround + glm::vec3{0.0F, VILLAGE_REVEAL_HEIGHT, 0.0F}, amount);
    pose.yawDegrees = wrapDegrees(start.yawDegrees + turn * amount);
    pose.pitchDegrees = glm::mix(start.pitchDegrees, VILLAGE_REVEAL_PITCH_DEGREES, amount);
    pose.fovDegrees = glm::mix(start.fovDegrees, VILLAGE_REVEAL_FOV_DEGREES, amount);
    return pose;
}

float villageNewLightStrength(float seconds) {
    return easeInOut((seconds - VILLAGE_NEW_LIGHT_DELAY_SECONDS) / VILLAGE_NEW_LIGHT_SECONDS);
}

float villageBeatBlack(float seconds) {
    return easeInOut((seconds - (VILLAGE_BEAT_SECONDS - VILLAGE_BEAT_FADE_SECONDS)) /
                     VILLAGE_BEAT_FADE_SECONDS);
}

} // namespace game
