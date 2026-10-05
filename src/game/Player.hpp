// Player: the body that walks through the maze, and its movement for one fixed step.
// See docs/modules/game/player.md
#pragma once

#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <span>

namespace game {

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

/// The player: a box standing on the floor, plus the settings of its movement.
///
/// Plain data plus math, like scene::Camera: no OpenGL, no keyboard, no clock. The
/// application owns one and calls update once per fixed step.
///
/// Two modes. Walking: movement only in the horizontal plane, feet on the floor (y = 0),
/// stopped by the collision boxes. Noclip: free flight along the view direction, through
/// everything.
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

    /// Height of the floor: where the feet are in walking mode.
    static constexpr float FLOOR_Y = 0.0F;

    /// Position of the feet: the middle of the bottom face of the body, in world space.
    glm::vec3 position{0.0F};

    /// False: walking with collisions. True: free flight without collisions.
    bool noclip = false;

    /// Speeds in use, in metres per second. Fields and not only constants, so that the
    /// debug UI can change them live.
    float walkSpeed = WALK_SPEED;
    float sprintSpeed = SPRINT_SPEED;
    float flySpeed = FLY_SPEED;

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
    /// obstacles are the collision boxes of the world (game::mazeColliders). They are
    /// ignored in noclip mode.
    void update(const PlayerInput& input, float yawDegrees, float pitchDegrees, float stepSeconds,
                std::span<const scene::Aabb> obstacles);
};

} // namespace game
