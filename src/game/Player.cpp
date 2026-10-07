// Player: the body that walks through the maze, and its movement for one fixed step.
// See docs/modules/game/player.md
#include "game/Player.hpp"

#include "game/Terrain.hpp"
#include "scene/Camera.hpp"

#include <algorithm>

namespace game {

namespace {

// Half extents of the body, as scene::Aabb::fromCenter wants them.
constexpr glm::vec3 BODY_HALF_EXTENTS{Player::BODY_WIDTH / 2.0F, Player::BODY_HEIGHT / 2.0F,
                                      Player::BODY_WIDTH / 2.0F};

// Pitch of a level look, in degrees.
constexpr float LEVEL_PITCH_DEGREES = 0.0F;

} // namespace

PlayerInput movementInput(const PlayerInput& held, bool mapShown) {
    // A new PlayerInput has every field false: no key is held.
    return mapShown ? PlayerInput{} : held;
}

bool advanceStamina(Stamina& stamina, const StaminaSettings& settings, bool wantsSprint,
                    float stepSeconds) {
    const bool sprinting = wantsSprint && !stamina.winded && stamina.level > 0.0F;

    if (sprinting) {
        // The drain. A step is stepSeconds / drainSeconds of the whole bar, so
        // drainSeconds of sprinting empty it. max keeps the last step from going
        // below 0.
        stamina.level = std::max(stamina.level - stepSeconds / settings.drainSeconds, 0.0F);
        stamina.secondsSinceDrain = 0.0F;
        stamina.secondsFull = 0.0F;
        if (stamina.level <= 0.0F) {
            stamina.winded = true;
        }
        return true;
    }

    // No drain in this step: the wait for the refill runs, and after it the refill.
    stamina.secondsSinceDrain += stepSeconds;
    if (stamina.secondsSinceDrain >= settings.refillDelaySeconds) {
        stamina.level = std::min(stamina.level + stepSeconds / settings.refillSeconds, 1.0F);
    }
    if (stamina.winded && stamina.level >= settings.windedRecovery) {
        stamina.winded = false;
    }
    // The clock of the HUD counts only while the bar is full. The drain above sets it
    // back to 0.
    if (stamina.level >= 1.0F) {
        stamina.secondsFull += stepSeconds;
    }
    return false;
}

bool staminaBarVisible(const Stamina& stamina) {
    return stamina.level < 1.0F || stamina.secondsFull < STAMINA_BAR_LINGER_SECONDS;
}

scene::Aabb Player::box() const {
    // position is at the feet, the centre of the box is half of the body height above it.
    const glm::vec3 center = position + glm::vec3{0.0F, BODY_HEIGHT / 2.0F, 0.0F};
    return scene::Aabb::fromCenter(center, BODY_HALF_EXTENTS);
}

glm::vec3 Player::eyePosition() const {
    return position + glm::vec3{0.0F, EYE_HEIGHT, 0.0F};
}

void Player::update(const PlayerInput& input, float yawDegrees, float pitchDegrees,
                    float stepSeconds, std::span<const scene::Aabb> obstacles,
                    const Terrain& terrain) {
    // The directions come from the same math as the picture on the screen: a camera with
    // the given angles. It is used only as a calculator here, its position is not read.
    // In walking mode the pitch is replaced by 0, a level look: forward then has no
    // vertical part and keeps its length of 1, so looking at the ground does not slow the
    // player down. right() is horizontal in both modes.
    scene::Camera view;
    view.yawDegrees = yawDegrees;
    view.pitchDegrees = noclip ? pitchDegrees : LEVEL_PITCH_DEGREES;
    const glm::vec3 forward = view.forward();
    const glm::vec3 right = view.right();

    // Opposite keys cancel each other: the two vectors add up to zero.
    glm::vec3 direction{0.0F};
    if (input.forward) {
        direction += forward;
    }
    if (input.backward) {
        direction -= forward;
    }
    if (input.right) {
        direction += right;
    }
    if (input.left) {
        direction -= right;
    }
    // Straight up and down exist only in flight. A walking player does not jump.
    if (noclip && input.up) {
        direction += scene::Camera::WORLD_UP;
    }
    if (noclip && input.down) {
        direction -= scene::Camera::WORLD_UP;
    }

    // Two keys at once give a vector longer than 1 (about 1.41 for W and D), which would
    // make diagonal movement faster. Normalizing brings the length back to 1. With no key
    // held the vector is zero and must be left alone: normalizing it divides by zero.
    const bool moving = glm::length(direction) > 0.0F;
    if (moving) {
        direction = glm::normalize(direction);
    }

    // The stamina, before the two modes part: in noclip the sprint key means "down", so
    // nothing is drained there, but the bar still refills.
    const bool sprinting =
        advanceStamina(stamina, staminaSettings, !noclip && input.sprint && moving, stepSeconds);

    if (noclip) {
        // Distance of one step: metres per second times seconds. Nothing is in the way.
        position += direction * (flySpeed * stepSeconds);
        return;
    }

    // Walking. The feet belong on the ground: this matters in the first step after noclip
    // was switched off in mid-air, where the box must be among the walls before it is
    // tested against them. There is no gravity and no jump: the height is simply read
    // from the terrain.
    position.y = terrain.heightAt(position.x, position.z);

    // The keys move the player in the horizontal plane only: direction has no vertical
    // part here, so the speed over the ground is the same uphill and downhill.
    // The sprint speed only while the stamina allows it: not when winded, and not when
    // no direction key is held.
    const float speed = sprinting ? sprintSpeed : walkSpeed;
    const glm::vec3 wanted = direction * (speed * stepSeconds);

    // The walls take away the part of the movement that would go into them and leave the
    // part along them. The box is built anew from the position in every step.
    position += scene::moveAndSlide(box(), wanted, obstacles);

    // The ground is uneven, so the place the step ended at has a height of its own.
    position.y = terrain.heightAt(position.x, position.z);
}

} // namespace game
