// Interaction: what the picking ray of a frame points at in a round, what the player can
// do with it, and where the models of the levers and the notes hang.
// See docs/modules/scene/picking.md
#pragma once

#include "game/Interactables.hpp"
#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"
#include "game/Round.hpp"
#include "scene/Collider.hpp"
#include "scene/Raycast.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library. The
// application builds one picking ray per frame and asks the functions here what it
// hits and what the interaction key does. Nothing here reads the keyboard or the mouse,
// so tests can aim a ray and "press the key" without a window.

/// What the interaction key (and a click) does at this moment.
enum class Interaction {
    None = 0,  ///< nothing: the ray points at nothing the player can use
    PullLever, ///< the ray points at a lever that is not pulled yet
    ReadNote,  ///< the ray points at a note
    CloseNote, ///< the card of a note is open: the key closes it first
};

/// The picking of one frame: the ray, what it hits and what can be done with it. The HUD
/// and the debug UI show it, and the class that draws the levers and the notes
/// highlights what it names.
struct PickState {
    /// False when no ray could be built in this frame: the window has no size, or the
    /// free cursor is over a debug panel. ray means nothing then.
    bool hasRay = false;

    /// True when the ray goes through the middle of the picture (the cursor is
    /// captured), false when it goes through the free cursor.
    bool centered = false;

    /// The ray, starting in the EYE (see rayFromEye).
    scene::Ray ray;

    /// The lever or the note the ray points at within INTERACTION_REACH, with nothing
    /// in between (game::pickInteractable). A pulled lever is still found here.
    PickedInteractable picked;

    /// What the interaction key does now (interactionFor).
    Interaction action = Interaction::None;
};

/// The switches of the debug view of the picking, edited by the debug UI.
struct PickDebugSettings {
    /// True draws the pick boxes of the levers and the notes and the picking ray as
    /// lines on top of the scene.
    bool drawShapes = false;

    /// True keeps the ray that is drawn as it was when the switch was set, while the
    /// picking itself goes on. A picking ray leaves the eye, so seen from that eye it is
    /// a single point: only a ray that stays behind can be looked at from the side.
    bool freezeRay = false;
};

/// The ray the picking uses, made from the ray of scene::screenPointRay.
///
/// screenPointRay starts on the near clipping plane, 0.1 m to about 0.15 m in front of
/// the eye. The reach of the player (INTERACTION_REACH) is meant from the eye, so the
/// start is moved back into the eye here. The direction stays: with a perspective
/// projection every ray through a point of the picture is a straight line that leaves
/// the eye, so the ray from the eye in that direction passes the same point of the
/// near plane and shows the same pixel.
scene::Ray rayFromEye(const scene::Ray& screenRay, const glm::vec3& eye);

/// What the interaction key does for a round and a thing the ray points at:
///   - nothing unless the round is being played (a won round is over),
///   - CloseNote while the card of a note is open, whatever the ray points at,
///   - PullLever for a lever that is not pulled yet. A pulled lever gives None,
///   - ReadNote for a note.
Interaction interactionFor(const Round& round, const PickedInteractable& picked);

/// The picking of a frame that has a ray. obstacles is the obstacle list of the round
/// (game::roundObstacles): the walls, the pillars and the closed gate hide what is behind
/// them, and a wall that a lever has opened is not in it, so the ray passes the opening.
/// centered is copied into the result.
PickState pickInRound(const scene::Ray& ray, bool centered, const MazeWorld& world,
                      const Round& round, std::span<const scene::Aabb> obstacles);

/// The picking of a frame without a ray: nothing is pointed at. The action can still be
/// CloseNote, because closing a card needs no aim.
PickState pickNothing(const Round& round);

/// Does what pick.action says: pulls the lever (game::pullRoundLever), opens the card
/// of the note (game::readNote) or closes the open card (game::closeNote). The action is
/// checked against the round again, so an old PickState cannot pull a lever twice.
///
/// Returns true when a wall opened: the caller then has to build its obstacle list again
/// (game::roundObstacles).
bool interact(Round& round, const MazeWorld& world, const PickState& pick);

/// The line the HUD shows for an action: "E: pull lever", "E: read note", "E: close".
/// key is the name of the key the player has put "use" on (game::boundKeyName), E in
/// a game with the default keys. An empty text for None.
std::string interactionPrompt(Interaction action, std::string_view key);

/// The light the picked lever or note gives off by itself at a moment (seconds on the
/// animation clock of the round), as a linear colour for the uniform uEmissive: a warm
/// yellow that pulses slowly between HIGHLIGHT_MIN_GLOW and HIGHLIGHT_MAX_GLOW. This is
/// the highlight of the selected object. Every program that draws the models already
/// adds uEmissive (the glow of the crystals), so the highlight needs no new shader and
/// shows in every lighting mode.
glm::vec3 highlightGlow(float seconds);

/// The colour of the highlight at full strength (red, green, blue), a linear colour.
constexpr glm::vec3 HIGHLIGHT_COLOR{1.0F, 0.8F, 0.4F};

/// The weakest and the strongest glow of the pulse, as factors of HIGHLIGHT_COLOR. The
/// glow multiplies the colour of the surface like a light does, so 1 is as bright as
/// white light of strength 1. It never goes down to 0: the picked object always stands
/// out, also at the low point of the pulse. The strongest glow is well above 1 on
/// purpose: in the beam of the flashlight the surface is already lit with more than 1,
/// and a weaker glow could not be told from that light.
constexpr float HIGHLIGHT_MIN_GLOW = 0.8F;
constexpr float HIGHLIGHT_MAX_GLOW = 2.4F;

/// How fast the highlight pulses, in radians per second: 5 is a little less than one
/// pulse per second (a full turn is 6.28).
constexpr float HIGHLIGHT_PULSE_SPEED = 5.0F;

// The models. All three follow one convention (tools/blender/build_lever.py and
// build_note.py): the origin is the middle of the back, the point fixed to the wall,
// the back lies in the plane z = 0 and the model stands out along +Z. That is the model
// of something on the NORTH wall of a cell: it reaches south, into the cell.

/// The pivot the handle of a lever turns around lies this far in front of the wall face,
/// in metres, in the middle of the plate. It has to match the housing of the model
/// lever.obj (build_lever.py), or the handle would float beside it.
constexpr float LEVER_PIVOT_DEPTH = 0.05F;

/// The angle of the handle around the X axis of the model, in degrees, before the lever
/// is pulled and after. The handle model points along +Z (straight out of the wall).
/// A positive turn around X moves +Z towards -Y, which is down, so "up" is the negative
/// angle. 55 degrees is steep on purpose: seen from the front the rod then runs clearly
/// up (or down) from the middle of the plate and the knob stands beyond the edge of the
/// plate, so the two states differ in their outline. The knob still stays inside the
/// pick box of the lever (build_lever.py checks that with the same numbers).
constexpr float LEVER_HANDLE_UP_DEGREES = -55.0F;
constexpr float LEVER_HANDLE_DOWN_DEGREES = 55.0F;

/// Model matrix of something that hangs on a wall: the model moved to position (the
/// point on the wall face, Lever::position or Note::position) and turned around the
/// vertical axis so that its +Z points away from the wall on the given side of the
/// cell, into the cell.
glm::mat4 mountModelMatrix(const glm::vec3& position, Direction side);

/// Model matrix of the handle of a lever: the handle model with its origin in the
/// pivot (LEVER_PIVOT_DEPTH in front of lever.position), tilted between
/// LEVER_HANDLE_UP_DEGREES (handleProgress 0) and LEVER_HANDLE_DOWN_DEGREES
/// (handleProgress 1, see game::leverHandleProgress) and turned with the wall like
/// mountModelMatrix does.
glm::mat4 leverHandleMatrix(const Lever& lever, float handleProgress);

} // namespace game
