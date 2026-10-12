// Lighting of the game: the settings of the moon, the flashlight and the point lights.
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
    /// Whether the lamp itself is drawn in the hand (heldLampMatrix). A switch of the
    /// debug UI: the light, its shadows and the battery do not depend on it.
    bool heldLampShown = true;

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

/// The lamp the player holds, the three models of tools/blender/build_lamp_held.py: it
/// lies along -Z with its origin in the middle of its butt, and the middle of its lens
/// is this many metres out along its axis (LENS_OUT in that script).
constexpr float HELD_LAMP_LENS_OUT = 0.219F;

/// How far the butt of the drawn lamp is from the hand, along the beam, in metres.
///
/// The light stands in the hand, 0.25 m below the eye, and no picture shows that place:
/// with a field of view of 60 degrees the lower edge of the picture reaches 0.25 m below
/// the eye only 0.43 m in front of it. So the lamp is drawn further along its own beam,
/// where the picture begins, and far enough that the window in its body is in the
/// picture too. The light is not moved (see LightingSettings::flashlightHandRight for
/// why it must stay inside the body of the player).
constexpr float HELD_LAMP_REACH = 0.35F;

/// The vertical field of view the lamp is drawn with, in degrees: the default one of
/// the game (DEFAULT_FIELD_OF_VIEW_DEGREES in game/Settings.hpp), whatever the player
/// has set. A wider view would push the lamp towards the middle of the picture and
/// a narrower one out of it.
constexpr float HELD_LAMP_FOV_DEGREES = 60.0F;

/// How bright the lens of the lamp glows, as a factor on the colour of the beam times
/// its intensity. Above 1 on purpose: the lens is the source of the light, so it has to
/// be the brightest thing of the picture and take part in the bloom.
constexpr float HELD_LAMP_LENS_GLOW = 3.0F;

/// How bright the window of the lamp glows with a full battery, as a factor on the
/// colour of the crystal lights.
constexpr float HELD_LAMP_WINDOW_GLOW = 1.6F;

/// The model matrix of the lamp the player holds, from the camera of this frame: eye,
/// forward and right as for flashlightPose.
///
/// The lamp lies on the axis of the beam of a player who looks straight ahead: its
/// origin is HELD_LAMP_REACH metres from the hand along the beam and its lens looks
/// along it, so the cone of light and the tube share one line and the beam leaves the
/// lamp through its lens, for every setting of the hand.
///
/// The one difference from flashlightPose: "below the eye" is measured along the up
/// vector of the camera here, not straight down in the world. The lamp is a part of the
/// picture, like the hand that would hold it, so it keeps its place in the lower right
/// when the camera looks up or down. With the hand of the light it would drift into the
/// middle of the picture of a player who looks at the ground, and hang there without an
/// arm. It still aims at the point the beam aims at, flashlightConvergeDistance in front
/// of the eye, so it points at the middle of the spot of light whatever the pitch is.
///
/// The top of the lamp, where its window is, stands at right angles to the beam and to
/// right, so the window faces the player and never rolls. The beam never runs along
/// right (its target is at least MIN_FLASHLIGHT_CONVERGE_DISTANCE in front of the eye),
/// so the top is always defined.
glm::mat4 heldLampMatrix(const LightingSettings& settings, const glm::vec3& eye,
                         const glm::vec3& forward, const glm::vec3& right);

/// The projection matrix the lamp is drawn with: sceneProjection, the perspective
/// projection of the frame, with its field of view replaced by HELD_LAMP_FOV_DEGREES.
/// The aspect ratio and the two planes stay. So the lamp has the same place and size in
/// the picture whatever field of view the player has chosen, and with the default one
/// the result is sceneProjection itself.
glm::mat4 heldLampProjection(const glm::mat4& sceneProjection);

/// The two glows of the lamp the player holds, as linear colours for uEmissive.
struct HeldLampGlow {
    /// The lens: black while the flashlight is off.
    glm::vec3 lens{0.0F};
    /// The window in the body, where the splinter that feeds the lamp shows.
    glm::vec3 window{0.0F};
};

/// The glows of the lamp in one frame.
///
/// frameLighting is the lighting the frame is drawn with (game::lightingForFrame): the
/// lens glows in the colour of the beam times its intensity, which already holds the
/// flicker of a low battery, and not at all while the flashlight is off or empty.
///
/// The window is the battery: it glows in the colour of the crystals, as bright as the
/// charge (1 is full, 0 is empty and dark), also while the lamp is switched off. flicker
/// is the factor of the beam at this moment (game::flashlightFlicker), so under a low
/// battery the window flickers together with the beam.
HeldLampGlow heldLampGlow(const LightingSettings& frameLighting, float charge, float flicker);

/// The light that spills back onto the head of the lamp while it is on: one weak point
/// light for the draw of the lamp only. It stands just outside the rim of the head, above
/// it and on the side of the player, where the model has this place: 2.5 cm to the left
/// of the axis, 6 cm above it and 1 mm in front of the rim. From there it grazes the rim
/// and the top of the head and has nearly left the tin where the head narrows to the
/// body, so the head looks touched by its own light and nothing glows.
///
/// A light in the lens itself would not do it: every face of the head the player sees
/// looks away from the lens.
constexpr glm::vec3 HELD_LAMP_SPILL_PLACE{-0.025F, 0.06F, -0.225F};

/// How strong the spill is, as a factor on the intensity of the beam, and how far it
/// reaches in metres (scene::attenuationForRadius). Both small on purpose: it is light
/// that leaks back, and at the window of the body it is gone.
constexpr float HELD_LAMP_SPILL = 0.6F;
constexpr float HELD_LAMP_SPILL_RADIUS = 0.1F;

/// The lights the lamp in the hand is drawn with: frameLights, the lights of the frame,
/// without the flashlight, and with the spill on the head of the lamp as one more point
/// light.
///
/// The flashlight is taken out because the lamp lies in its own beam, a third of a metre
/// in front of the light, and the lit programs would paint the cone on its back.
///
/// The spill has the colour of the beam and its intensity times HELD_LAMP_SPILL, so it
/// flickers with a low battery like the beam (frameLighting is the lighting of the frame,
/// game::lightingForFrame). It is left out while the flashlight is off or empty.
/// lampMatrix is the matrix the lamp is drawn with, so the spill moves with the lamp.
/// With a full list of point lights it takes the place of the last one, the farthest
/// from the eye.
scene::LightSet heldLampLights(const scene::LightSet& frameLights,
                               const LightingSettings& frameLighting, const glm::mat4& lampMatrix);

/// How the drawn lamp moves with the steps of the player and with the turns of the
/// camera. Drawing only: the light, its shadow map and flashlightPose never see it, so the
/// beam stands still and the lamp moves by centimetres around the place the beam leaves
/// from. The numbers are in metres and degrees and can be tuned here.
///
/// The bob: the lamp swings to one side with one step and to the other with the next,
/// sinks a little each time a foot comes down and rolls a little with the swing. It never
/// comes above its place at rest: its butt lies just below the lower edge of the picture
/// and must stay there, because no arm holds it. A sprint swings wider, and faster
/// because the steps come faster.
constexpr float HELD_LAMP_WALK_SIDE = 0.008F;
constexpr float HELD_LAMP_WALK_DIP = 0.005F;
constexpr float HELD_LAMP_WALK_ROLL_DEGREES = 1.0F;
constexpr float HELD_LAMP_SPRINT_SIDE = 0.014F;
constexpr float HELD_LAMP_SPRINT_DIP = 0.01F;
constexpr float HELD_LAMP_SPRINT_ROLL_DEGREES = 2.0F;

/// How long the bob takes to come in when the player sets off, to change between a walk
/// and a sprint and to settle when the player stops, in seconds.
constexpr float HELD_LAMP_SWAY_BLEND_SECONDS = 0.25F;

/// The lag: the lamp falls behind a camera that turns by this share of the turn, catches
/// up within about this many seconds, and is never further behind than the limit.
constexpr float HELD_LAMP_LAG_SHARE = 0.2F;
constexpr float HELD_LAMP_LAG_SECONDS = 0.1F;
constexpr float HELD_LAMP_MAX_LAG_DEGREES = 2.0F;

/// A camera that turned further than this in one frame, in degrees, was put somewhere
/// else and did not turn there: the start of a round. The lamp does not lag behind that.
constexpr float HELD_LAMP_CUT_DEGREES = 45.0F;

/// The sway of the lamp between two frames. It belongs to the drawing and is advanced
/// with the frame time (advanceHeldLampSway), not with the fixed steps of the rules.
struct HeldLampSway {
    /// Steps walked, counted in steps and kept between 0 and 2: the bob repeats after
    /// a step to each side. It counts metres, not seconds, so the swing follows the feet
    /// at every speed and stands where it is when they stop.
    float steps = 0.0F;
    /// From 0 (the player stands) to 1 (walks at full walking speed or faster).
    float walkAmount = 0.0F;
    /// From 0 (a walk) to 1 (a sprint).
    float sprintAmount = 0.0F;
    /// How far the lamp is behind the camera, in degrees: x to the left of it after
    /// a turn to the right, y below it after a turn upwards.
    glm::vec2 lagDegrees{0.0F};
    /// The two angles of the camera in the frame before, to know how far it turned.
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
};

/// What one frame tells the sway.
struct HeldLampSwayStep {
    /// The time of the frame in seconds.
    float seconds = 0.0F;
    /// How fast the feet of the player move over the ground, in metres per second: the
    /// speed they really had, 0 for a player who stands, flies or pushes against a wall.
    float speed = 0.0F;
    /// Player::walkSpeed and Player::sprintSpeed. Faster than half way between the two
    /// is a sprint, as for the sound of the steps (game::sprintedStep).
    float walkSpeed = 0.0F;
    float sprintSpeed = 0.0F;
    /// The two angles of the camera in this frame, in degrees.
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
};

/// Advances the sway by one frame: the count of the steps by the way the feet went, the
/// two amounts towards what the speed says, each by at most the share of
/// HELD_LAMP_SWAY_BLEND_SECONDS the frame took, and the lag by the turn of the camera
/// since the frame before (none after a cut, HELD_LAMP_CUT_DEGREES). A frame without
/// time changes nothing but the remembered angles. The caller leaves it out while the round stands
/// still (a pause, the result screen), so the lamp stands still there too.
void advanceHeldLampSway(HeldLampSway& sway, const HeldLampSwayStep& step);

/// Where the sway puts the drawn lamp, measured in the axes of the lamp itself
/// (heldLampMatrix): x to its right, y to its top, z against the beam.
struct HeldLampPose {
    /// Metres from its place at rest.
    glm::vec3 offset{0.0F};
    /// Degrees about the butt of the lamp, where the wrist would be, so the butt stays
    /// below the edge of the picture: the lens up, the lens to the left, and a roll
    /// about the axis that tips the top to the left.
    float pitchDegrees = 0.0F;
    float yawDegrees = 0.0F;
    float rollDegrees = 0.0F;
};

/// The pose of a sway. Pure: the same sway always gives the same pose. A sway at rest
/// (both amounts 0, no lag) gives the pose at rest, whatever the count of the steps is.
HeldLampPose heldLampSwayPose(const HeldLampSway& sway);

/// The pose as a matrix. The lamp is drawn with heldLampMatrix(...) * this, so the pose
/// moves the lamp in its own axes. The pose at rest is the unit matrix.
glm::mat4 heldLampSwayMatrix(const HeldLampPose& pose);

/// One point light of a frame: where it hangs and how much of its brightness it has.
struct PointLightSpot {
    glm::vec3 position{0.0F};
    /// From 0 (dark) to 1 (its full intensity).
    float strength = 1.0F;

    /// False for the light of a crystal: its colour, its radius and its intensity are
    /// the ones of the settings (pointColor, pointRadius, pointIntensity), shared by all
    /// crystals. True for a light with a look of its own, the lamp of the gate
    /// (game::gateLampLight): it has the three fields below.
    bool ownLook = false;
    /// The colour as an sRGB value, like the colours of the settings.
    glm::vec3 color{1.0F};
    /// How far the light reaches, in metres (scene::attenuationForRadius).
    float radius = 0.0F;
    /// The intensity at strength 1.
    float intensity = 1.0F;
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

/// The same choice among lights that are already spots: crystal lights and lights with
/// a look of their own in one list, so the lamp of the gate takes one of the places only
/// while it is among the nearest. Every chosen spot comes back as it went in, with its
/// strength multiplied by the fade at the edge of the set.
///
/// It has a name of its own and is not a second nearestPointLights: called with an empty
/// list written as {}, the compiler could not tell the two apart.
std::vector<PointLightSpot> nearestPointLightSpots(std::span<const PointLightSpot> lights,
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
/// strength, with the colour and the radius of the settings. A spot with a look of its
/// own (PointLightSpot::ownLook) keeps its colour, its radius and its intensity. Lights
/// past scene::MAX_POINT_LIGHTS are ignored.
scene::LightSet buildLightSet(const LightingSettings& settings, const FlashlightPose& flashlight,
                              std::span<const PointLightSpot> pointLights);

} // namespace game
