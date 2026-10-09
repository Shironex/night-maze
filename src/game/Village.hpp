// Village: the village on the ridge beyond the gate, where it stands in the sky, and the
// camera that rises over the gate to show it after a night of the campaign is won.
#pragma once

#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"

#include <glm/glm.hpp>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can check every rule. The drawing is game::VillageRenderer, and how many nights of
// light the village shows is a rule of the campaign (game::villageNightsLit).
//
// The village is the one the lamps are for. It stands on a ridge, the far rim of the
// hollow. It is not a place in the maze: it is drawn like the sky, centred on the eye, so
// it never comes closer. Unlike the sky it has real depth: walls, hills and the gatehouse
// hide it. Its ridge is the rim of the hollow and closes the whole horizon.

/// The model (tools/blender/build_village.py) is built around the eye that looks at it,
/// and these numbers must agree with the script: the lane lies this far out and this
/// high, in metres, and no vertex is further from the eye than VILLAGE_REACH.
constexpr float VILLAGE_LANE_DISTANCE = 257.0F;
constexpr float VILLAGE_LANE_HEIGHT = 27.0F;
constexpr float VILLAGE_REACH = 290.0F;

/// The ridge is the rim of the hollow: it runs the whole way round the eye, its upper edge
/// a little above the horizon, and the hill of the village rises out of it. So it has no
/// ends. Its foot lies this far out and this far below the eye in the model, all the way
/// round, and these two numbers must agree with the script too.
constexpr float VILLAGE_FOOT_DISTANCE = 200.0F;
constexpr float VILLAGE_FOOT_DEPTH = 200.0F;

/// The model rides with the eye, so a raised camera lifts its foot out of the fog that
/// lies on the ground. Up to this height of the eye above the ground, in metres, the foot
/// stays in fog so thick that nothing of it is seen, and the ridge has no lower edge:
/// with the far plane and the fog the game starts with, which a test checks. The glide of
/// the menu camera (14 m over the hard level, 22 m over the largest maze of the debug
/// window) and the reveal are far below it. Only
/// noclip can fly higher.
constexpr float VILLAGE_CLEAR_EYE_HEIGHT = 60.0F;

/// The model is scaled about the eye until its furthest vertex lies at this part of the
/// far plane of the camera. Scaling about the eye changes no angle, so the picture is the
/// one of a village 257 m away, and nothing of it is cut off by the far plane.
constexpr float VILLAGE_FAR_SHARE = 0.9F;

/// How strongly a lit window glows, compared with its colour: the same kind of number as
/// GATE_LAMP_GLOW_STRENGTH (5). A window is two pixels wide, so the bloom has to carry it.
constexpr float VILLAGE_LIGHT_GLOW = 7.0F;

/// The last window, the one of the player, burns this much brighter than the others: it
/// is one light among 32, and its halo has to be the one that is seen coming on.
constexpr float VILLAGE_OWN_WINDOW_GLOW = 3.0F;

/// What can be changed about the village while the game runs. The debug UI edits the
/// fields.
struct VillageSettings {
    /// Whether the village is drawn.
    bool enabled = true;

    /// Lifts (or sinks) the whole ridge by this many degrees, on top of the 6 degrees
    /// the lane has by itself.
    float elevationDegrees = 0.0F;
};

/// The side of the maze the village lies on: beyond the gate, so the side opposite to the
/// one the gate stands on (game::gateSide). Who walks through the gate walks towards it.
/// The world must have a gate (MazeWorld::hasGate).
Direction villageSide(const MazeWorld& world);

/// The yaw of a camera that looks straight at the village on that side, in degrees, as
/// scene::Camera counts it: 0 north (-Z), 90 east (+X), 180 south, 270 west.
float villageYawDegrees(Direction side);

/// The model matrix of the village and of its lights, for a view matrix without its
/// translation (the one of the sky: mat4(mat3(view))). The model is turned to its side,
/// lifted by elevationDegrees and scaled about the eye so that VILLAGE_REACH lies at
/// VILLAGE_FAR_SHARE of farPlane.
glm::mat4 villageMatrix(Direction side, float farPlane, float elevationDegrees = 0.0F);

// The reveal: on the result screen of a night of the campaign the camera leaves the eyes
// of the player, rises over the exit cell and turns to the village, with a longer lens.
// The lights of the night that was just won come on once it has arrived.

/// How long the camera takes, in seconds, and how high above the ground of the exit cell
/// it ends: 2 m above the walls.
constexpr float VILLAGE_REVEAL_SECONDS = 3.5F;
constexpr float VILLAGE_REVEAL_HEIGHT = 5.0F;

/// The field of view the camera ends with, in degrees: about a third of the default one,
/// so the village is three times as large in the picture as it is from the corridors,
/// where its houses are about 110 pixels wide in a window of 1280.
constexpr float VILLAGE_REVEAL_FOV_DEGREES = 22.0F;

/// The camera ends looking this far up, so the horizon lies below the middle of the
/// picture and the village above it.
constexpr float VILLAGE_REVEAL_PITCH_DEGREES = 3.0F;

/// On the result screen the camera does not end looking straight at the village: it looks
/// this far to the right of it, so the village stands in the left half of the picture,
/// beside the card of the result. The look at the village after the last night has no
/// card and no such turn.
constexpr float VILLAGE_REVEAL_BESIDE_CARD_DEGREES = 8.5F;

/// The new lights come on over this many seconds, beginning this long after the result
/// came up: when the camera has almost arrived, so the village is first seen as the night
/// before left it.
constexpr float VILLAGE_NEW_LIGHT_DELAY_SECONDS = 3.2F;
constexpr float VILLAGE_NEW_LIGHT_SECONDS = 1.6F;

/// Where a camera stands and looks, and with which lens: the fields of scene::Camera
/// that the reveal moves.
struct VillageRevealPose {
    glm::vec3 eye{0.0F};
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
    float fovDegrees = 60.0F;
};

/// The camera of the reveal, seconds after the result came up. start is the camera of the
/// player at that moment, exitGround the middle of the exit cell on its ground
/// (MazeWorld::exitPosition) and side the side of the village. asideDegrees is how far to
/// the right of the village the camera ends looking: VILLAGE_REVEAL_BESIDE_CARD_DEGREES
/// with a card in the picture, 0 without.
///
/// At 0 seconds and before, the pose is start. From VILLAGE_REVEAL_SECONDS on it stands
/// VILLAGE_REVEAL_HEIGHT above exitGround and looks at the village, lifted by
/// VILLAGE_REVEAL_PITCH_DEGREES. In between everything moves together, slowly at both
/// ends, and the yaw turns the short way round. The yaw of the result is from 0 to 360.
VillageRevealPose villageRevealPose(const VillageRevealPose& start, const glm::vec3& exitGround,
                                    Direction side, float asideDegrees, float seconds);

/// How much of the lights of the night that was just won is there, 0 to 1, seconds after
/// the result came up. The lights of the nights before it are always all there.
float villageNewLightStrength(float seconds);

// The beat: after the last night the same camera move is shown without a card, for a few
// seconds, before the ending card (GameMode::VillageBeat). The last window lights in it.

/// How long the beat takes, and for how long at its end the picture fades to the black
/// of the ending card, in seconds.
constexpr float VILLAGE_BEAT_SECONDS = 7.5F;
constexpr float VILLAGE_BEAT_FADE_SECONDS = 1.2F;

/// How black the picture of the beat is, 0 to 1, seconds after it began: 0 until the
/// fade, 1 at its end.
float villageBeatBlack(float seconds);

} // namespace game
