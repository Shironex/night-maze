// MenuCamera: a camera that travels through the maze by itself, the picture behind the menu.
#pragma once

#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"

#include <glm/glm.hpp>

#include <span>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. Nothing here is random and nothing keeps a clock: the place of the camera
// is a function of the maze, the settings and a number of seconds.

/// The two kinds of shot the menu camera knows.
enum class MenuShot {
    /// At eye height through the corridors: from the start to every crystal and to the
    /// gate, and back again.
    CorridorWalk,
    /// High above the maze on a slow circle, looking down at its middle.
    HighGlide,
};

/// How many values MenuShot has: the debug UI lists them.
constexpr int MENU_SHOT_COUNT = 2;

/// The speed the menu camera starts with, in metres per second: a slow walk, about
/// a quarter of the walking speed of the player (Player::WALK_SPEED).
constexpr float DEFAULT_MENU_CAMERA_SPEED = 0.7F;

/// The limits of the speed, in metres per second. 0 lets the camera stand still.
constexpr float MIN_MENU_CAMERA_SPEED = 0.0F;
constexpr float MAX_MENU_CAMERA_SPEED = 4.0F;

/// The height of the menu camera above the ground in the corridors, in metres. A little
/// below the eyes of the player (Player::EYE_HEIGHT), so the crystals, which float at
/// about 1 m, stand higher in the picture.
constexpr float DEFAULT_MENU_CAMERA_EYE_HEIGHT = 1.5F;

/// The limits of that height, in metres: from just above the grass to just below the
/// tops of the walls (WALL_HEIGHT is 3 m).
constexpr float MIN_MENU_CAMERA_EYE_HEIGHT = 0.3F;
constexpr float MAX_MENU_CAMERA_EYE_HEIGHT = 2.8F;

/// How far the camera of the corridor walk keeps to the right of the middle of
/// a corridor, in metres. The way out and the way back so never share a line, and
/// where the camera turns around it does so on a half circle of this radius. The face
/// of a wall is 0.85 m from the middle (half a cell less half of
/// WALL_COLLISION_THICKNESS), so 0.65 m of air stay between the camera and the wall on
/// its right, and the picture is close to the one from the middle of the corridor.
constexpr float MENU_CAMERA_LANE_OFFSET = 0.2F;

/// How far before the middle of a cell the corridor walk turns around, in metres. The
/// cells it turns around in are the ones it went to: a crystal floats in the middle of
/// such a cell, or the gate stands on its far side. Turning short of the middle keeps
/// that in front of the camera while the view swings round, and keeps the camera away
/// from the wall at the end of a corridor.
constexpr float MENU_CAMERA_TURN_SHORT = 0.6F;

/// The radius of the arc that rounds a corner of the walk, in metres. An arc is made
/// smaller where two corners are too near to each other for two arcs of this size: the
/// two corners of a turn around are twice MENU_CAMERA_LANE_OFFSET apart, so there the
/// radius is that offset.
constexpr float MENU_CAMERA_CORNER_RADIUS = 0.5F;

/// What can be changed about the menu camera. The debug UI edits the fields, and the
/// command line sets them at the start (game/StartOptions.hpp).
struct MenuCameraSettings {
    /// Whether the game shows itself: the camera travels alone, the round stands still
    /// and the HUD is hidden.
    bool enabled = false;

    /// The kind of shot.
    MenuShot shot = MenuShot::CorridorWalk;

    /// How fast the camera travels along its path, in metres per second.
    float speed = DEFAULT_MENU_CAMERA_SPEED;

    /// The height of the camera above the ground, in metres. Used by the corridor walk
    /// only: the high glide chooses its height from the size of the maze.
    float eyeHeight = DEFAULT_MENU_CAMERA_EYE_HEIGHT;

    /// Seconds added to the clock of the camera: the shot starts that far into its
    /// loop. It chooses the part of the maze a recording shows.
    float timeOffset = 0.0F;
};

/// The closed line the corridor walk follows, on the ground.
///
/// The line is stored as many points close to each other (MENU_CAMERA_SAMPLE_SPACING).
/// After the last point it goes back to the first one, which is not stored twice.
struct MenuCameraPath {
    /// The points, each one on the ground (y is the height of the terrain there).
    std::vector<glm::vec3> points;

    /// For every point the length of the line from the first point to it, in metres.
    /// The first entry is 0 and the numbers only grow.
    std::vector<float> distances;

    /// The length of the whole loop, in metres: the last entry of distances plus the
    /// piece from the last point back to the first.
    float length = 0.0F;

    /// For every point the yaw the camera has there, in degrees as in scene::Camera.
    /// From every point the camera looks towards the farthest place of the path ahead
    /// that it can see: down the corridor, and into the next one as soon as it shows.
    /// That direction jumps when a new corridor comes into sight, so it is averaged
    /// over a few metres of the path: the view turns slowly, starts before the jump
    /// and ends after it.
    ///
    /// The numbers are not kept between 0 and 360: they run on (a turn to the right
    /// adds, a turn to the left takes away), so two neighbouring entries can be
    /// blended like any two numbers.
    std::vector<float> yawDegrees;

    /// How much the yaw has changed after one whole loop, in degrees: a whole number
    /// of full turns, because the camera looks the same way again when the loop
    /// closes. The yaw at the end of the piece that closes the loop is the first entry
    /// of yawDegrees plus this.
    float turnDegrees = 0.0F;
};

/// The largest distance between two neighbouring points of a MenuCameraPath, in metres.
/// Small against the radius of the corners, so the line of straight pieces is as good
/// as round, and small against the squares of the terrain, so the camera follows the
/// ground.
constexpr float MENU_CAMERA_SAMPLE_SPACING = 0.05F;

/// Where the menu camera stands and where it looks at one moment. The two angles mean
/// what they mean in scene::Camera.
struct MenuCameraPose {
    /// The place of the camera in world space.
    glm::vec3 eye{0.0F};
    /// 0 looks along -Z (north), 90 along +X (east). From 0 to 360.
    float yawDegrees = 0.0F;
    /// Positive looks up, negative down.
    float pitchDegrees = 0.0F;
};

/// The cells the corridor walk passes, in order: a closed walk through open passages
/// only. It starts in start, visits every cell of targets that can be reached and comes
/// back. From the last cell of the list one more step leads to the first.
///
/// The walk is the way around a tree: the tree of the shortest ways from start to the
/// targets (passageDistances). Every passage of that tree is walked twice, once in each
/// direction, and the walker always takes the first way on its right hand. With no
/// target to go to the list holds start alone.
///
/// Throws std::out_of_range when start is not a cell of the maze. A target outside the
/// maze or behind walls is left out.
std::vector<MazeCell> menuCameraRoute(const Maze& maze, MazeCell start,
                                      std::span<const MazeCell> targets);

/// The cells the corridor walk of a world goes to: the cell of every crystal of the seed
/// (MazeWorld::seedCrystals, so the heartstone changed the walk of no seed) and the
/// cell in front of the gate (the last one on the way from the start to the exit, the
/// closed gate stands between it and the exit cell). A world without a gate, a maze of
/// one cell, has no targets.
std::vector<MazeCell> menuCameraTargets(const MazeWorld& world);

/// Builds the line of the corridor walk of a world: menuCameraRoute to
/// menuCameraTargets, MENU_CAMERA_LANE_OFFSET to the right of the middle of every
/// corridor, with every corner rounded by an arc and every point put on the terrain.
/// The same world always gives the same path. In a maze of one cell the path is a small
/// circle around the middle of that cell.
MenuCameraPath buildMenuCameraPath(const MazeWorld& world);

/// The point of the path that lies distance metres along it from its first point. The
/// path is a loop: a distance past its length starts again from the beginning, and
/// a negative one counts back from the end. An empty path gives the origin.
glm::vec3 menuCameraPathPoint(const MenuCameraPath& path, float distance);

/// How long one loop of the shot takes at the speed of settings, in seconds: after that
/// time the camera is where it began, looking the same way. path must be the path of
/// world. 0 when the speed is 0, because a camera that stands still never comes round.
float menuCameraLoopSeconds(const MenuCameraPath& path, const MazeWorld& world,
                            const MenuCameraSettings& settings);

/// Where the menu camera is seconds after it was switched on. path must be the path of
/// world (buildMenuCameraPath).
///
/// CorridorWalk: the camera moves along the path at settings.speed, settings.eyeHeight
/// above the ground. It looks down the corridor, towards the farthest place of its path
/// in sight, with every turn spread over a few metres (MenuCameraPath::yawDegrees), so
/// no turn is sudden. On top comes a slow, small sway of both angles.
///
/// HighGlide: the camera flies on a circle around the middle of the maze, above the
/// tops of the walls, and looks down at that middle. path is not used.
///
/// Both shots are loops: the pose at menuCameraLoopSeconds is the pose at 0.
MenuCameraPose menuCameraPose(const MenuCameraPath& path, const MazeWorld& world,
                              const MenuCameraSettings& settings, float seconds);

} // namespace game
