// MenuCamera: where the camera that travels through the maze by itself is at a time: the corridor
// walk and the high glide.
#include "game/MenuCamera.hpp"

#include "game/MazeLayout.hpp"
#include "game/MenuCameraShared.hpp"
#include "scene/Camera.hpp"

#include <algorithm>
#include <cmath>

namespace game {

namespace {

// The camera has the averaged yaw of the place this many metres behind it on the path.
// An average reaches as far forwards as backwards, so without this the view would
// start to turn metres before the new corridor shows, towards the wall that still
// hides it. With it the view starts to turn about when the corridor comes into sight,
// like the head of somebody who walks there.
constexpr float LOOK_LAG_METRES = 1.5F;

// The view is tilted up by this much, in degrees: the tops of the walls and the stars
// above them are part of the picture, and less of it is the ground right in front.
constexpr float LOOK_UP_DEGREES = 4.0F;

// The slope of the view follows the ground between the place this many metres behind
// the camera and the place as far ahead of it.
constexpr float SLOPE_REACH_METRES = 2.0F;

// The sway: the view drifts a little to the left and right and up and down, like the
// head of somebody who walks slowly. One swing to both sides takes about this many
// metres of the path (11 seconds at the default speed). The real number is chosen so
// that a whole number of swings fits into the loop: the sway then has no jump where the
// loop closes.
constexpr float SWAY_METRES = 9.0F;

// How far the sway turns the view to each side and tilts it up and down, in degrees.
constexpr float SWAY_YAW_DEGREES = 2.5F;
constexpr float SWAY_PITCH_DEGREES = 1.0F;

// The view tilts up and down twice while it drifts left and right once, so the sway
// draws a lying figure of eight and not a line.
constexpr float SWAY_PITCH_SWINGS = 2.0F;

// ---- the high glide -------------------------------------------------------------------

// The circle of the high glide. Its radius is this share of the distance from the
// middle of the maze to one of its corners, so the camera stays above the maze or just
// outside of it and the far corner is still in the picture.
constexpr float GLIDE_RADIUS_SHARE = 0.9F;

// The circle is never smaller than this, in metres: above a maze of one cell the camera
// would otherwise turn on the spot.
constexpr float MIN_GLIDE_RADIUS = 6.0F;

// How high the camera of the glide flies above the ground in the middle of the maze:
// the walls, and on top this share of the radius of the circle. The view then goes
// down at about 30 degrees and reaches the ground of the corridors on the far side,
// between the walls.
constexpr float GLIDE_HEIGHT_SHARE = 0.38F;

// The point the glide looks at lies this far above the ground in the middle of the
// maze, in metres: half of the height of the walls.
constexpr float GLIDE_TARGET_HEIGHT = WALL_HEIGHT / 2.0F;

// The glide is this many times faster than the walk at the same speed setting. From
// far away a movement looks slower, so the same metres per second would seem to stand
// still.
constexpr float GLIDE_SPEED_SCALE = 2.5F;

// The yaw of the view at a distance along the path, blended between the two points
// around that place. Not brought into the range 0 to 360.
float pathYawAt(const MenuCameraPath& path, float distance) {
    const PathPlace place = placeOnPath(path, distance);
    const float endYaw = path.yawDegrees[place.next] + (place.closing ? path.turnDegrees : 0.0F);
    return glm::mix(path.yawDegrees[place.before], endYaw, place.share);
}

// ---- angles ---------------------------------------------------------------------------

// The two angles of scene::Camera for a view along direction, which must have a length.
// The reverse of Camera::forward: (sin(yaw) cos(pitch), sin(pitch), -cos(yaw) cos(pitch)).
void lookAlong(const glm::vec3& direction, MenuCameraPose& pose) {
    const glm::vec3 forward = glm::normalize(direction);
    pose.yawDegrees = glm::degrees(std::atan2(forward.x, -forward.z));
    pose.pitchDegrees = glm::degrees(std::asin(std::clamp(forward.y, -1.0F, 1.0F)));
}

// Brings the angles of a pose into the ranges scene::Camera keeps them in: the yaw
// from 0 to 360 degrees and the pitch away from straight up and straight down.
void keepInRange(MenuCameraPose& pose) {
    pose.yawDegrees -= FULL_TURN_DEGREES * std::floor(pose.yawDegrees / FULL_TURN_DEGREES);
    pose.pitchDegrees = std::clamp(pose.pitchDegrees, -scene::Camera::MAX_PITCH_DEGREES,
                                   scene::Camera::MAX_PITCH_DEGREES);
}

// ---- the two shots --------------------------------------------------------------------

// The middle of the maze on the ground, and the circle the glide flies on around it.
struct GlideCircle {
    glm::vec3 center{0.0F};
    float radius = 0.0F;
    // Height of the camera above center, in metres.
    float height = 0.0F;
};

GlideCircle glideCircle(const MazeWorld& world) {
    const float width = static_cast<float>(world.maze.width()) * CELL_SIZE;
    const float depth = static_cast<float>(world.maze.height()) * CELL_SIZE;

    GlideCircle circle;
    circle.center = {width / 2.0F, 0.0F, depth / 2.0F};
    circle.center.y = world.terrain.heightAt(circle.center.x, circle.center.z);
    // Half of the diagonal of the maze: from its middle to a corner.
    const float toCorner = std::sqrt(width * width + depth * depth) / 2.0F;
    circle.radius = std::max(toCorner * GLIDE_RADIUS_SHARE, MIN_GLIDE_RADIUS);
    circle.height = WALL_HEIGHT + circle.radius * GLIDE_HEIGHT_SHARE;
    return circle;
}

MenuCameraPose corridorWalkPose(const MenuCameraPath& path, const MenuCameraSettings& settings,
                                float seconds) {
    MenuCameraPose pose;
    if (path.points.empty() || path.length < MIN_LENGTH) {
        return pose;
    }

    // Constant speed: the distance along the path grows evenly with the time.
    const float travelled = (seconds + settings.timeOffset) * settings.speed;
    const glm::vec3 ground = menuCameraPathPoint(path, travelled);
    pose.eye = ground + scene::Camera::WORLD_UP * settings.eyeHeight;

    // The view: towards the farthest place of the path in sight, with every turn
    // spread over a few metres (MenuCameraPath::yawDegrees).
    const float lag = std::min(LOOK_LAG_METRES, path.length * MAX_WINDOW_SHARE);
    pose.yawDegrees = pathYawAt(path, travelled - lag);

    // The slope: how much the ground rises between a place behind the camera and
    // a place ahead of it, over the length of path between them. The ground of the
    // maze is gentle, so this is a degree or two.
    const float reach = std::min(SLOPE_REACH_METRES, path.length * MAX_WINDOW_SHARE);
    const float rise = menuCameraPathPoint(path, travelled + reach).y -
                       menuCameraPathPoint(path, travelled - reach).y;
    pose.pitchDegrees = glm::degrees(std::asin(std::clamp(rise / (reach * 2.0F), -1.0F, 1.0F)));
    pose.pitchDegrees += LOOK_UP_DEGREES;

    // The sway, measured along the path and not in seconds: a whole number of swings
    // fits into the loop, so the view has no jump where the loop closes.
    const float swings = std::max(std::round(path.length / SWAY_METRES), 1.0F);
    const float phase = FULL_TURN * swings * travelled / path.length;
    pose.yawDegrees += SWAY_YAW_DEGREES * std::sin(phase);
    pose.pitchDegrees += SWAY_PITCH_DEGREES * std::sin(SWAY_PITCH_SWINGS * phase);
    keepInRange(pose);
    return pose;
}

MenuCameraPose highGlidePose(const MazeWorld& world, const MenuCameraSettings& settings,
                             float seconds) {
    const GlideCircle circle = glideCircle(world);

    // The angle on the circle, in radians: the way flown divided by the radius. 0 is
    // north of the middle, and it grows clockwise seen from above, like a yaw.
    const float flown = (seconds + settings.timeOffset) * settings.speed * GLIDE_SPEED_SCALE;
    const float angle = flown / circle.radius;

    MenuCameraPose pose;
    pose.eye = circle.center + glm::vec3{circle.radius * std::sin(angle), circle.height,
                                         -circle.radius * std::cos(angle)};
    const glm::vec3 target = circle.center + scene::Camera::WORLD_UP * GLIDE_TARGET_HEIGHT;
    lookAlong(target - pose.eye, pose);
    keepInRange(pose);
    return pose;
}

} // namespace

float menuCameraLoopSeconds(const MenuCameraPath& path, const MazeWorld& world,
                            const MenuCameraSettings& settings) {
    if (settings.speed <= 0.0F) {
        return 0.0F;
    }
    if (settings.shot == MenuShot::HighGlide) {
        // Once around the circle, at the speed of the glide.
        return FULL_TURN * glideCircle(world).radius / (settings.speed * GLIDE_SPEED_SCALE);
    }
    return path.length / settings.speed;
}

MenuCameraPose menuCameraPose(const MenuCameraPath& path, const MazeWorld& world,
                              const MenuCameraSettings& settings, float seconds) {
    if (settings.shot == MenuShot::HighGlide) {
        return highGlidePose(world, settings, seconds);
    }
    return corridorWalkPose(path, settings, seconds);
}

} // namespace game
