// "Player" category of the debug window: position and noclip, the angles and the
// projection of the camera, the speeds, the stamina and the menu camera.
// See docs/modules/scene/camera-controls.md
#include "debug/categories/PlayerCategory.hpp"

#include "debug/DebugContext.hpp"
#include "debug/Widgets.hpp"
#include "game/MenuCamera.hpp"
#include "game/Player.hpp"
#include "scene/Camera.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <algorithm>

namespace debug {

namespace {

// How much the position changes for one pixel of dragging, in metres.
constexpr float POSITION_DRAG_SPEED = 0.05F;

// Width of the three number fields of the position together, in pixels at 100 %
// display scaling.
constexpr float POSITION_FIELDS_WIDTH = 162.0F;

// Yaw is kept in the range from 0 to 360 by scene::Camera::rotate.
constexpr float MIN_YAW_DEGREES = 0.0F;
constexpr float MAX_YAW_DEGREES = 360.0F;

// Field of view: 20 is a strong zoom, 120 is very wide. Close to 0 or to 180 the
// projection breaks down.
constexpr float MIN_FOV_DEGREES = 20.0F;
constexpr float MAX_FOV_DEGREES = 120.0F;

// The near plane must stay above 0. The upper limit is far enough to cut into the walls
// around the player.
constexpr float MIN_NEAR_PLANE = 0.01F;
constexpr float MAX_NEAR_PLANE = 10.0F;

// The lower limit is small enough to cut off the far end of a corridor.
constexpr float MIN_FAR_PLANE = 1.0F;
constexpr float MAX_FAR_PLANE = 200.0F;

// Smallest distance between the two planes. They must never be equal: the projection
// matrix divides by (far - near).
constexpr float MIN_PLANE_DISTANCE = 0.1F;

constexpr float MIN_MOUSE_SENSITIVITY = 0.01F;
constexpr float MAX_MOUSE_SENSITIVITY = 1.0F;

// Limits of the three speeds of the player, in metres per second. At the upper limit one
// fixed step (1/120 s) is about 17 cm long, still short next to a wall box (30 cm thick).
// scene::moveAndSlide never jumps over a box whatever the step, but it moves one axis at
// a time, and that staircase path only stays close to the straight one for short steps.
constexpr float MIN_MOVE_SPEED = 0.5F;
constexpr float MAX_MOVE_SPEED = 20.0F;

// Limits of the numbers of the stamina rule. The three times are divided by or waited
// for, so they stay above 0 (the delay may be 0: refill at once). The recovery level is
// a part of the bar: at 0.05 a winded player sprints again almost at once, at 1 only
// with a full bar.
constexpr float MIN_STAMINA_SECONDS = 0.5F;
constexpr float MAX_STAMINA_SECONDS = 30.0F;
constexpr float MIN_REFILL_DELAY_SECONDS = 0.0F;
constexpr float MAX_REFILL_DELAY_SECONDS = 5.0F;
constexpr float MIN_WINDED_RECOVERY = 0.05F;
constexpr float MAX_WINDED_RECOVERY = 1.0F;

constexpr float PERCENT = 100.0F;

// The shots of the menu camera, in the order of the enum game::MenuShot: the number of
// the chosen entry is the value of the enum. One string, each entry ended by a zero
// character. A new shot is one more entry here: the check below then fails until it is
// added.
constexpr const char* MENU_SHOT_ITEMS = "Corridor walk\0High glide\0";
static_assert(game::MENU_SHOT_COUNT == 2);

// How much the time offset of the menu camera changes for one pixel of dragging, in
// seconds, and the width of its number field in pixels.
constexpr float TIME_OFFSET_DRAG_SPEED = 0.25F;
constexpr float TIME_OFFSET_FIELD_WIDTH = 84.0F;

// Where the player is and how the player moves through the walls.
void drawPosition(Page& page, const DebugContext& context) {
    game::Player& player = context.player;
    page.beginCard("Position");

    // The camera has no position of its own to edit: after every fixed step the game
    // puts it at the eyes of the player. So the field that can be dragged is the
    // position of the player (the feet), and the eye is only shown. DragFloat3 edits
    // three floats through the pointer: x, y and z. value_ptr gives the address of the
    // three floats of a glm::vec3.
    const float fieldsWidth = POSITION_FIELDS_WIDTH * displayScale();
    if (page.beginRow("Player feet",
                      "The position of the feet of the player in the world, in metres: "
                      "x, y and z. Drag a number, or double click it to type one. While "
                      "walking the game keeps y on the ground, so a change of y lasts only "
                      "in noclip mode.",
                      fieldsWidth)) {
        ImGui::SetNextItemWidth(fieldsWidth);
        ImGui::DragFloat3("##feet", glm::value_ptr(player.position), POSITION_DRAG_SPEED);
        page.endRow();
    }

    // The same switch as the N key. In noclip mode the collision boxes are ignored.
    page.toggle("Noclip (key N)", &player.noclip,
                "Free flight through the walls. W A S D fly along the view, Space goes "
                "up, Left Shift goes down.");
    page.stat("Mode", "%s", player.noclip ? "noclip (free flight)" : "walking");
    const scene::Camera& camera = context.camera;
    page.stat("Eye", "%.2f, %.2f, %.2f", camera.position.x, camera.position.y, camera.position.z);
    page.note("Click the scene to capture the mouse, Esc releases it. While captured the "
              "mouse looks around. Walking: W A S D walk level, Left Shift sprints "
              "while there is stamina.");

    page.endCard();
}

// Where the camera looks and what its projection is.
void drawView(Page& page, const DebugContext& context) {
    scene::Camera& camera = context.camera;
    page.beginCard("View");

    // A slider can also be typed into (Ctrl and click), and a typed value may be
    // outside the limits. The page forces it back between them. It matters most for
    // pitch: the field is public and only Camera::rotate clamps it, and a pitch of 90
    // degrees breaks the view matrix.
    page.slider("Yaw", &camera.yawDegrees, MIN_YAW_DEGREES, MAX_YAW_DEGREES, "%.1f deg",
                "The direction the camera looks in, around the vertical axis. 0 looks "
                "north, and the angle grows clockwise.");
    page.slider("Pitch", &camera.pitchDegrees, -scene::Camera::MAX_PITCH_DEGREES,
                scene::Camera::MAX_PITCH_DEGREES, "%.1f deg",
                "The angle of the camera up (positive) and down (negative).");
    page.slider("FOV", &camera.fovDegrees, MIN_FOV_DEGREES, MAX_FOV_DEGREES, "%.1f deg",
                "The vertical field of view: 20 is a strong zoom, 120 is very wide.");
    // Logarithmic: half of the slider covers the small values, where a change of the
    // near plane matters most.
    page.slider("Near plane", &camera.nearPlane, MIN_NEAR_PLANE, MAX_NEAR_PLANE, "%.2f m",
                "The nearest distance that is drawn. Logarithmic: half of the bar covers "
                "the small values.",
                true);
    page.slider("Far plane", &camera.farPlane, MIN_FAR_PLANE, MAX_FAR_PLANE, "%.1f m",
                "The farthest distance that is drawn. It always stays at least 0.1 m "
                "behind the near plane.");
    // The ranges of the two sliders overlap, so keep the far plane behind the near one.
    camera.farPlane = std::max(camera.farPlane, camera.nearPlane + MIN_PLANE_DISTANCE);

    page.endCard();
}

// How fast the player turns and moves.
void drawMovement(Page& page, const DebugContext& context) {
    game::Player& player = context.player;
    page.beginCard("Movement");

    page.slider("Mouse sensitivity", &context.mouseSensitivity, MIN_MOUSE_SENSITIVITY,
                MAX_MOUSE_SENSITIVITY, "%.2f",
                "Degrees the camera turns for one unit of mouse movement.");
    page.slider("Walk speed", &player.walkSpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED, "%.1f m/s",
                "The speed of the player when walking.");
    page.slider("Sprint speed", &player.sprintSpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED, "%.1f m/s",
                "The speed of the player while Left Shift is held, a direction key is "
                "held and the stamina allows it.");

    // The four numbers of the stamina rule, next to the speed they limit.
    game::StaminaSettings& rule = player.staminaSettings;
    page.slider("Stamina drain", &rule.drainSeconds, MIN_STAMINA_SECONDS, MAX_STAMINA_SECONDS,
                "%.1f s", "How long a full stamina bar lasts while sprinting.");
    page.slider("Stamina refill delay", &rule.refillDelaySeconds, MIN_REFILL_DELAY_SECONDS,
                MAX_REFILL_DELAY_SECONDS, "%.1f s",
                "How long the bar waits after the last sprinted moment before it starts "
                "to refill.");
    page.slider("Stamina refill", &rule.refillSeconds, MIN_STAMINA_SECONDS, MAX_STAMINA_SECONDS,
                "%.1f s", "How long an empty bar takes to get full once it refills.");
    page.slider("Winded recovery", &rule.windedRecovery, MIN_WINDED_RECOVERY, MAX_WINDED_RECOVERY,
                "%.2f",
                "A player who ran the bar empty is winded and cannot sprint until the bar "
                "is back at this part of it: 0.5 is half.");
    page.stat("Stamina", "%.0f%% %s", player.stamina.level * PERCENT,
              player.stamina.winded ? "(winded)" : "(can sprint)");
    page.slider("Fly speed", &player.flySpeed, MIN_MOVE_SPEED, MAX_MOVE_SPEED, "%.1f m/s",
                "The speed of the player in noclip mode.");

    page.endCard();
}

// The camera that shows the game by itself.
void drawMenuCamera(Page& page, const DebugContext& context) {
    game::MenuCameraSettings& menuCamera = context.menuCamera;
    page.beginCard("Menu camera");

    page.toggle("Menu camera (F2)", &menuCamera.enabled,
                "The game shows itself: the camera travels alone, the round stands "
                "still, and the HUD and the debug window are hidden. Key F2 switches it "
                "on and off, the debug key brings the window back while it runs.");

    int shotIndex = static_cast<int>(menuCamera.shot);
    if (page.combo("Shot", &shotIndex, MENU_SHOT_ITEMS,
                   "The path the camera travels: at eye height through the corridors, or "
                   "on a slow circle high above the maze.")) {
        menuCamera.shot = static_cast<game::MenuShot>(shotIndex);
    }
    page.slider("Speed", &menuCamera.speed, game::MIN_MENU_CAMERA_SPEED,
                game::MAX_MENU_CAMERA_SPEED, "%.2f m/s",
                "How fast the camera moves along its path.");
    page.slider("Eye height", &menuCamera.eyeHeight, game::MIN_MENU_CAMERA_EYE_HEIGHT,
                game::MAX_MENU_CAMERA_EYE_HEIGHT, "%.2f m",
                "The height of the camera above the ground on the corridor walk.");

    // Any number is a valid offset, also a negative one: the shot is a loop. So this is
    // a field that is dragged and has no limits (the two zeros), not a slider.
    const float fieldWidth = TIME_OFFSET_FIELD_WIDTH * displayScale();
    if (page.beginRow("Time offset",
                      "Moves the camera along its path in time, in seconds. Any number, "
                      "also a negative one: the shot is a loop. Drag it, or double click "
                      "it to type.",
                      fieldWidth)) {
        ImGui::SetNextItemWidth(fieldWidth);
        ImGui::DragFloat("##offset", &menuCamera.timeOffset, TIME_OFFSET_DRAG_SPEED, 0.0F, 0.0F,
                         "%.1f s");
        page.endRow();
    }
    page.stat("One loop", "%.0f s", context.menuCameraLoopSeconds);

    page.endCard();
}

} // namespace

void drawPlayerCategory(Page& page, const DebugContext& context) {
    page.setPlace("Player");
    page.beginColumns();
    drawPosition(page, context);
    drawMovement(page, context);
    page.nextColumn();
    drawView(page, context);
    drawMenuCamera(page, context);
    page.endColumns();
}

} // namespace debug
