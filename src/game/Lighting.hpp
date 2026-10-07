// Lighting of the game: the settings of the moon, the flashlight and the point lights.
// See docs/modules/game/flashlight.md
#pragma once

#include "scene/Light.hpp"

#include <glm/glm.hpp>

#include <span>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. The class that sends the lights to the graphics card is game::LightRig.

/// How the maze is shaded. The debug UI shows the entries in this order, so the number
/// of an entry in its list is the value of the enum.
enum class LightingMode {
    Unlit = 0,  ///< no lighting: the texture as it is (the textured program)
    Gouraud,    ///< lighting computed for every vertex and blended across the triangle
    Phong,      ///< lighting computed for every fragment, highlight from the reflected ray
    BlinnPhong, ///< lighting computed for every fragment, highlight from the halfway vector
};

/// The value of the uniform uSpecularModel in common/lighting.glsl: which formula the
/// shiny highlight is computed with. The numbers are the ones the shader compares with.
enum class SpecularModel {
    Phong = 0,      ///< angle between the reflected light ray and the direction to the eye
    BlinnPhong = 1, ///< angle between the normal and the halfway vector
};

/// The highlight formula a lighting mode uses. Gouraud uses the classic Phong formula,
/// so that the modes Gouraud and Phong differ in one thing only: where the lighting is
/// computed (per vertex or per fragment).
SpecularModel specularModelOf(LightingMode mode);

/// Everything about the lighting that can be changed while the game runs. The debug UI
/// edits these fields, and the game builds the lights of a frame from them
/// (buildLightSet). The values here are the defaults of the night scene.
///
/// COLOUR SPACE: the four colours are sRGB values, the numbers a colour picker shows
/// and the screen displays. buildLightSet converts them to linear colours for the
/// shaders. The intensities multiply the linear colour and may make it brighter than 1:
/// the scene is drawn into an HDR buffer, and the composite pass (game::PostProcess)
/// brings the result back into the range of the screen. The defaults are tuned for that
/// pipeline together with PostProcessSettings::exposure and its tone mapping.
struct LightingSettings {
    /// How the maze is shaded.
    LightingMode mode = LightingMode::BlinnPhong;

    /// Light that reaches every surface: very low, so that a wall no light shines on is
    /// a dark shape that can just be told from the sky. A cold blue, like the night sky.
    /// As an sRGB value it looks like more than it is: only about a tenth of each number
    /// is left as linear light.
    glm::vec3 ambient{0.055F, 0.07F, 0.115F};

    /// The moon, a directional light. The two angles say which way its light TRAVELS,
    /// with the conventions of scene::directionFromAngles: yaw like a compass (0 towards
    /// -Z, 90 towards +X), pitch below 0 means downwards. The yaw is deliberately not
    /// a multiple of 45 degrees: the walls face four directions, and this way each of
    /// them gets a different share of the light. The moon casts shadows: its shadow map
    /// is fitted to the land from this direction (game/Shadows.hpp).
    ///
    /// COUPLING WITH THE SKY: the moon disc of the skybox is painted where this light
    /// comes from, which is the direction opposite to the one the two defaults below
    /// describe. The pictures are generated with the same two numbers
    /// (MOON_LIGHT_YAW_DEGREES and MOON_LIGHT_PITCH_DEGREES in
    /// tools/blender/make_skybox.py). When a default changes here, change it there and
    /// generate the sky again. A test in tests/SkyboxTests.cpp looks for the disc where
    /// the default light comes from, and the disc has a radius of 2.2 degrees: the test
    /// fails only when the defaults moved by more than about 2 degrees, a smaller change
    /// goes unnoticed. Known limit: the picture is fixed, so moving the moon in the
    /// Lights panel changes the light on the walls while the painted moon stays where
    /// it is.
    float moonYawDegrees = 25.0F;
    float moonPitchDegrees = -50.0F;
    /// A cool, dim blue-white. The intensity is low on purpose (it is a dark night), but
    /// high enough that a surface in the moon light is clearly brighter than one in the
    /// shadow of a wall, which only has the ambient light: about seven times on level
    /// ground.
    glm::vec3 moonColor{0.55F, 0.65F, 1.0F};
    float moonIntensity = 0.12F;

    /// The flashlight, a spot light in the hand of the player (flashlightPose). Key
    /// F switches it. An empty battery switches it off and keeps it off
    /// (game::updateRound). It casts shadows: its shadow map is drawn from the hand
    /// (game/Shadows.hpp).
    bool flashlightOn = true;
    /// A warm white.
    glm::vec3 flashlightColor{1.0F, 0.9F, 0.72F};
    float flashlightIntensity = 1.3F;
    /// Half angles of the cone in degrees, see scene::SpotLight.
    float flashlightInnerDegrees = 13.0F;
    float flashlightOuterDegrees = 21.0F;
    /// How far the flashlight reaches, in metres (scene::attenuationForRadius): five
    /// cells of the maze, so the end of a long corridor stays in the dark. It is also the
    /// far plane of its shadow map.
    float flashlightRange = 10.0F;
    /// Where the hand holds the flashlight, measured from the eye in metres: this far
    /// to the right of it (along the right vector of the camera, which is always level)
    /// and this far below it (straight down in the world, whatever the pitch is).
    ///
    /// Why not at the eye: a light that stands exactly where the picture is taken from
    /// casts every shadow exactly behind the thing that casts it, where the camera
    /// cannot see it. Only a light a little to the side shows its shadows.
    ///
    /// Why the light cannot end up inside a wall: the body of the player is a box 0.6
    /// m wide (Player::BODY_WIDTH) that no wall enters, and the eye is in its middle.
    /// The offset to the right is level, so the hand is 0.2 m from the middle and 0.1
    /// m inside the box, however the player is turned. Straight down keeps it that
    /// way: with "below the camera" a player who looks at the ground would hold the
    /// hand 0.25 m behind the eye, and the two offsets together (0.32 m) could leave
    /// the box. See MAX_FLASHLIGHT_HAND_RIGHT.
    float flashlightHandRight = 0.2F;
    float flashlightHandDown = 0.25F;
    /// The beam does not run parallel to the view. It points from the hand at the
    /// point this many metres in front of the eye on the line the player looks along,
    /// so the spot of light is in the middle of the screen on a wall at this distance
    /// and close to the middle on every wall that is not much nearer. 4 m is two
    /// cells of the maze: a usual distance to the wall the player walks towards.
    float flashlightConvergeDistance = 4.0F;

    /// The point lights of the crystals: one hangs just above every crystal that has not
    /// been collected yet. They all share these settings. A cyan-teal: the colour the
    /// crystals glow in (their emissive colour is derived from it).
    glm::vec3 pointColor{0.2F, 0.9F, 0.8F};
    float pointIntensity = 0.9F;
    /// How far one of them reaches, in metres: one and a half cells.
    float pointRadius = 3.0F;

    /// The shiny highlight of the stone. Strength: how bright it is compared with the
    /// light that makes it. Stone is rough, so it is weak.
    float specularStrength = 0.25F;
    /// Shininess: the exponent of the highlight formula. A larger number makes the
    /// highlight smaller and sharper. 32 is a common middle value.
    float shininess = 32.0F;

    /// Normal mapping: the normal of every fragment is read from the normal map of the
    /// material instead of being taken from the mesh, which gives the flat walls joints
    /// and the ground stones and bumps under the lights. See usesNormalMap for where it applies.
    bool normalMapping = true;
};

/// True when the normals come from the normal maps with these settings: normal mapping
/// is switched on and the lighting mode is not Gouraud.
///
/// A normal map holds a normal per texel, so it needs lighting per fragment (Phong and
/// Blinn-Phong). Gouraud computes the light at the vertices only and cannot use it. The
/// mode Unlit has no lighting at all, but the debug view "Normals as colour" shows the
/// normals of the maps in it, so the answer is true there.
bool usesNormalMap(const LightingSettings& settings);

/// The direction the light of the moon TRAVELS in, with length 1, from the two angles of
/// the settings (scene::directionFromAngles). The lights of a frame and the shadow map
/// of the moon both take it from here, so they can never disagree.
glm::vec3 moonDirection(const LightingSettings& settings);

/// The largest offset of the flashlight to the right of the eye, in metres: the range
/// of the slider in the Lights panel. Half of the body of the player (0.3 m, half of
/// Player::BODY_WIDTH) minus the near plane of the shadow map of the flashlight (0.05
/// m, scene::SPOT_NEAR_PLANE). Up to here the light stays inside the body box with the
/// near plane to spare, so a wall the player stands sideways against is neither behind
/// the light nor cut off by the near plane of its shadow map. A test checks the sum.
constexpr float MAX_FLASHLIGHT_HAND_RIGHT = 0.25F;

/// The smallest distance at which the beam of the flashlight may meet the line of view,
/// in metres. Nearer than this the beam would point steeply across the picture, and at
/// 0 it would have no direction at all.
constexpr float MIN_FLASHLIGHT_CONVERGE_DISTANCE = 0.5F;

/// Where the flashlight is and where it points in one frame, in world space.
struct FlashlightPose {
    /// The place of the light: the hand of the player.
    glm::vec3 position{0.0F};
    /// The axis of the cone, with length 1.
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
};

/// The flashlight in the hand of the player, from the camera of this frame.
///
/// eye is where the camera stands, forward where it looks and right its right vector
/// (scene::Camera::forward and scene::Camera::right), both of length 1. The position is
/// the eye moved by flashlightHandRight along right and by flashlightHandDown straight
/// down. The direction points from there to the point flashlightConvergeDistance metres
/// in front of the eye (at least MIN_FLASHLIGHT_CONVERGE_DISTANCE), so the beam crosses
/// the line of view there. With both offsets at 0 the result is the eye and forward:
/// the flashlight at the eye, as it was before it cast shadows.
///
/// Pass the same eye the view matrix is built from (the blend of two fixed steps), not
/// the position of the last step, or the cone would trail behind the picture.
///
/// The spot light of the frame (buildLightSet) and the shadow map of the flashlight
/// (scene::spotLightSpace) both take the place and the direction from ONE result of
/// this function, so the light and its shadows can never disagree.
FlashlightPose flashlightPose(const LightingSettings& settings, const glm::vec3& eye,
                              const glm::vec3& forward, const glm::vec3& right);

/// One point light of a frame: where it hangs and how much of its brightness it has.
struct PointLightSpot {
    glm::vec3 position{0.0F};
    /// From 0 (dark) to 1 (the full intensity of the settings).
    float strength = 1.0F;
};

/// Over this many metres a point light fades out before it leaves the set of lights of
/// a frame (nearestPointLights): two cells of the maze.
constexpr float POINT_LIGHT_FADE_DISTANCE = 4.0F;

/// Chooses the point lights one frame is drawn with: the maxCount lights nearest to eye,
/// the place the picture is taken from.
///
/// Why: the shaders have room for scene::MAX_POINT_LIGHTS point lights (the array of
/// the uniform block), and a large maze has more crystals than that. A light reaches
/// about 3 m (LightingSettings::pointRadius), so a crystal far from the eye lights
/// ground that is small in the picture or hidden behind walls. Its own glow, which is
/// not a light but a colour of its material, is drawn for every crystal.
///
/// Without more care a light would switch on or off at full brightness in the frame in
/// which two crystals change places in the order. So the lights near the edge of the
/// set are dimmed: the distance of the nearest light that was LEFT OUT is the edge, a
/// light at the edge has strength 0, and one POINT_LIGHT_FADE_DISTANCE or more inside
/// it has strength 1. When two lights change places they are equally far away, both at
/// the edge and both dark, so nothing jumps.
///
/// With maxCount lights or fewer all of them are returned at strength 1. The result is
/// sorted from the nearest to the farthest. A maxCount below 1 gives no lights.
std::vector<PointLightSpot> nearestPointLights(std::span<const glm::vec3> positions,
                                               const glm::vec3& eye,
                                               int maxCount = scene::MAX_POINT_LIGHTS);

/// The lights of one frame, with their colours converted from sRGB to linear.
///
/// flashlight is where the spot light stands and where it points in this frame
/// (flashlightPose).
///
/// pointLights are the point lights: the ones above the crystals nearest to the eye
/// (game::crystalLightPositions, then nearestPointLights). The function does not know
/// where they come from. Each one gets the intensity of the settings times its
/// strength. Lights past scene::MAX_POINT_LIGHTS are ignored.
scene::LightSet buildLightSet(const LightingSettings& settings, const FlashlightPose& flashlight,
                              std::span<const PointLightSpot> pointLights);

} // namespace game
