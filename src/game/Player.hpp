// Player: the body that walks through the maze, and its movement for one fixed step.
// See docs/modules/game/player.md
#pragma once

#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <span>

namespace game {

class Terrain;

/// What the player wants to do in one fixed step. Plain booleans, filled by the
/// application from the keyboard, so that the player code knows nothing about keys and
/// a test can "hold a key" by setting a field.
struct PlayerInput {
    bool forward = false;  ///< W
    bool backward = false; ///< S
    bool left = false;     ///< A
    bool right = false;    ///< D
    bool up = false;       ///< Space, used only in noclip mode
    bool down = false;     ///< Left Shift, used only in noclip mode
    bool sprint = false;   ///< Left Shift, used only in walking mode
};

/// The numbers of the stamina rule. The debug UI edits them, the values here are the
/// defaults: starting values that are meant to be tuned by playing.
struct StaminaSettings {
    /// How long a full bar lasts while sprinting, in seconds.
    float drainSeconds = 6.0F;

    /// How long the bar waits after the last drain before it starts to refill, in seconds.
    float refillDelaySeconds = 1.0F;

    /// How long an empty bar takes to get full again once it refills, in seconds.
    float refillSeconds = 4.0F;

    /// A winded player can sprint again when the bar is back at this level: 0.5 is half.
    float windedRecovery = 0.5F;
};

/// How long the stamina bar of the HUD stays on the screen after it got full again, in
/// seconds. Then it is hidden, so the screen stays calm (staminaBarVisible).
constexpr float STAMINA_BAR_LINGER_SECONDS = 2.0F;

/// The stamina of the player: what changes while playing and starts again with every
/// round. A new one (Stamina{}) is a full bar that is not winded.
struct Stamina {
    /// How full the bar is: 1 is full, 0 is empty.
    float level = 1.0F;

    /// True from the moment the bar ran empty until it is back at
    /// StaminaSettings::windedRecovery. A winded player cannot sprint.
    bool winded = false;

    /// Seconds since the bar was drained the last time. The refill waits for it to
    /// reach StaminaSettings::refillDelaySeconds.
    float secondsSinceDrain = 0.0F;

    /// Seconds the bar has been full without a break, only for the HUD. It starts at
    /// the linger time, so a new round does not show a bar nobody has used yet.
    float secondsFull = STAMINA_BAR_LINGER_SECONDS;
};

/// Advances the stamina by one fixed step of stepSeconds seconds and returns true when
/// the player sprints in this step.
///
/// wantsSprint is true when the sprint key is held AND the player is walking somewhere
/// (Player::update decides that). The player sprints when it wants to, is not winded
/// and has stamina left.
///
///   - Sprinting drains the bar: from full to empty in drainSeconds. The step that
///     empties it makes the player winded.
///   - Otherwise the bar refills, from empty to full in refillSeconds, but only after
///     refillDelaySeconds have passed since the last drain.
///   - Winded ends when the refill reaches windedRecovery. Holding the sprint key while
///     winded changes nothing: it is not a drain, so the bar refills all the same.
bool advanceStamina(Stamina& stamina, const StaminaSettings& settings, bool wantsSprint,
                    float stepSeconds);

/// True while the HUD shows the stamina bar: while it is not full, and for
/// STAMINA_BAR_LINGER_SECONDS after it got full.
bool staminaBarVisible(const Stamina& stamina);

/// The player: a box standing on the ground, plus the settings of its movement.
///
/// Plain data plus math, like scene::Camera: no OpenGL, no keyboard, no clock. The
/// application owns one and calls update once per fixed step.
///
/// Two modes. Walking: the keys move the player in the horizontal plane, stopped by the
/// collision boxes, and the feet follow the height of the terrain. Noclip: free flight
/// along the view direction, through everything.
struct Player {
    /// Width and depth of the body in metres. The box cannot rotate, so it is square.
    static constexpr float BODY_WIDTH = 0.6F;

    /// Height of the body in metres.
    static constexpr float BODY_HEIGHT = 1.8F;

    /// Height of the eyes above the feet in metres. The camera stands here.
    static constexpr float EYE_HEIGHT = 1.7F;

    /// Walking speed in metres per second.
    static constexpr float WALK_SPEED = 3.0F;

    /// Walking speed with sprint held, in metres per second.
    static constexpr float SPRINT_SPEED = 5.5F;

    /// Flight speed in noclip mode, in metres per second.
    static constexpr float FLY_SPEED = 6.0F;

    /// Position of the feet: the middle of the bottom face of the body, in world space.
    glm::vec3 position{0.0F};

    /// False: walking with collisions. True: free flight without collisions.
    bool noclip = false;

    /// Speeds in use, in metres per second. Fields and not only constants, so that the
    /// debug UI can change them live.
    float walkSpeed = WALK_SPEED;
    float sprintSpeed = SPRINT_SPEED;
    float flySpeed = FLY_SPEED;

    /// The numbers of the stamina rule, fields for the same reason as the speeds.
    StaminaSettings staminaSettings;

    /// The stamina as it is now. update drains and refills it. The application assigns
    /// a new one (Stamina{}) when a round starts.
    Stamina stamina;

    /// The collision box of the body at the current position: BODY_WIDTH x BODY_HEIGHT x
    /// BODY_WIDTH, standing on the feet.
    scene::Aabb box() const;

    /// Where the eyes are: EYE_HEIGHT above the feet.
    glm::vec3 eyePosition() const;

    /// Moves the player by one fixed step of stepSeconds seconds.
    ///
    /// yawDegrees and pitchDegrees are the angles of the camera (see scene::Camera).
    /// Walking uses only the yaw: forward is level, wherever the camera looks. Noclip
    /// uses both: forward is the view direction.
    ///
    /// obstacles are the collision boxes in the way (game::roundObstacles), and terrain
    /// is the ground the feet are put on after the movement (Terrain::heightAt). Both are
    /// ignored in noclip mode.
    ///
    /// The step also advances the stamina (advanceStamina). The player wants to sprint
    /// when the sprint key is held together with a direction key that is not cancelled
    /// by its opposite. Whether the body really gets anywhere is not asked: sprinting
    /// into a wall drains the bar too. Standing still with the key held drains nothing,
    /// and neither does noclip, where the key means "down": there the bar only refills.
    void update(const PlayerInput& input, float yawDegrees, float pitchDegrees, float stepSeconds,
                std::span<const scene::Aabb> obstacles, const Terrain& terrain);
};

} // namespace game
