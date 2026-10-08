// Round: the state of one play through a maze (crystals, gate, levers, battery, time) and
// its rules.
#pragma once

#include "game/Discovery.hpp"
#include "game/Flasks.hpp"
#include "game/Interactables.hpp"
#include "game/Lighting.hpp"
#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"
#include "game/Shade.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace game {

// Plain data and free functions without OpenGL, like the rest of the game_logic library,
// so tests can play a whole round without a window.

/// How long the gate takes to sink into the ground after it opens, in seconds. A wall
/// that a lever opens sinks in the same time.
constexpr float GATE_OPEN_SECONDS = 1.5F;

/// How far below its closed position the gate is when it is fully open, in metres: a
/// little more than the pillars are high (3.15 m), so nothing of it is left above the
/// ground. The closed gate already stands on the lowest ground under it. A wall that
/// a lever opens (3 m high, standing on the lowest ground under it too) sinks as far.
constexpr float GATE_SINK_DEPTH = 3.3F;

/// How long the handle of a lever takes to swing from up to down after it is pulled, in
/// seconds: a quick move of the hand, much shorter than the sinking of the wall.
constexpr float LEVER_PULL_SECONDS = 0.3F;

/// The card of a note closes by itself when the player is farther than this from the
/// note, in metres, measured on the ground: a little more than the reach of the picking
/// ray (2.5 m), so the card does not close while the player still stands at the note.
constexpr float NOTE_READ_DISTANCE = 3.0F;

/// The player reaches for crystals and for the exit with a sphere: its centre is this
/// far above the feet (the middle of the 1.8 m body), and it is as wide as the body.
constexpr float PLAYER_REACH_HEIGHT = 0.9F;
constexpr float PLAYER_REACH_RADIUS = 0.3F;

/// The numbers of the rules that can be changed while the game runs. The debug UI edits
/// them. The values here are the defaults.
struct GameplaySettings {
    /// The gate opens when this part of the crystals of the maze has been collected
    /// (see requiredCrystalCount): 0.7 means 7 of 10.
    float requiredFraction = 0.7F;

    /// How long a full battery lasts with the flashlight switched on, in seconds.
    float batteryLifetimeSeconds = 180.0F;

    /// How much of a full battery one collected crystal gives back.
    float batteryPerCrystal = 0.25F;

    /// Below this charge the flashlight flickers (see flashlightFlicker).
    float lowBatteryThreshold = 0.2F;

    /// Radius of the sphere around the middle of a crystal that the player has to reach
    /// to collect it, in metres.
    float pickupRadius = 0.6F;

    /// How many flasks of tea the maze gets when a round starts (game::placeFlasks):
    /// from 0 to MAX_FLASK_COUNT. A new game writes the number of its difficulty level
    /// here. A new number shows when the round is started again.
    int flaskCount = 1;

    /// The numbers of the shade, the enemy of the round (game/Shade.hpp), and the switch
    /// that says whether the round has one at all. A new game switches it off for a calm
    /// night.
    ShadeSettings shade;

    /// False stops the battery from draining: a switch for testing in the debug UI.
    bool batteryDrains = true;

    /// True when a new round on the same maze was asked for and has not started yet.
    /// The debug UI sets it, the application starts the round at the start of the next
    /// frame and clears it, like MazeSettings::regenerate.
    bool restart = false;

    /// True when the debug UI asked to pull every lever of the round at once: a switch
    /// for testing, handled by the application in the same way as restart.
    bool pullAllLevers = false;
};

/// Whether a round is still being played. There is no "lost": an empty battery only
/// makes the maze dark.
enum class RoundState {
    Playing = 0, ///< the player is in the maze
    Won,         ///< the player walked through the open gate into the exit
};

/// One crystal of a round.
struct RoundCrystal {
    /// Where its base rests (crystalRestPosition). The crystal that is drawn moves
    /// a little around this point. The pickup sphere stays on it.
    glm::vec3 restPosition{0.0F};

    /// Which crystal model it is drawn with (CrystalSpawn::variant).
    int variant = 0;

    /// True once the player has picked it up: it is no longer drawn and gives no light.
    bool collected = false;
};

/// One flask of tea of a round.
struct RoundFlask {
    /// The cell it lies in (game::placeFlasks). Where exactly it floats is computed
    /// from the cell and the terrain when it is needed (game::flaskRestPosition), so
    /// nothing has to be moved when the terrain is rebuilt.
    MazeCell cell;

    /// True once the player has picked it up: it is no longer drawn.
    bool collected = false;
};

/// The state of one round: everything that changes while a maze is played and starts
/// again when the round is restarted. The maze itself (MazeWorld) does not change.
struct Round {
    RoundState state = RoundState::Playing;

    /// The crystals, in the order of MazeWorld::crystals.
    std::vector<RoundCrystal> crystals;

    /// How many crystals are collected, and how many are needed to open the gate.
    int collectedCount = 0;
    int requiredCount = 0;

    /// The flasks of tea, and how many of them the player has picked up. The count only
    /// grows within a round, so comparing it before and after a step tells that a flask
    /// was picked up in that step: the application then lets the player drink it
    /// (game::drinkFlask), and the sound cues hear it (game::roundStepCues).
    std::vector<RoundFlask> flasks;
    int flasksCollected = 0;

    /// True from the moment enough crystals are collected: the gate no longer blocks
    /// the way and starts to sink. It never goes back to false within a round.
    bool gateOpen = false;

    /// How far the gate has sunk: 0 is closed, 1 is fully in the ground. It grows for
    /// GATE_OPEN_SECONDS after the gate opens.
    float gateProgress = 0.0F;

    /// Charge of the flashlight battery: 1 is full, 0 is empty.
    float battery = 1.0F;

    /// The time of the round in seconds: it counts while the round is played and stops
    /// when it is won. This is the time shown on the screen.
    float elapsedSeconds = 0.0F;

    /// The clock of everything that moves by itself (bobbing crystals, pulsing lights,
    /// the flicker of the flashlight). It keeps running after the round is won, so the
    /// scene behind the "You escaped" card does not freeze.
    float animationSeconds = 0.0F;

    /// Which cells of the maze the player has seen in this round (game/Discovery.hpp):
    /// what the minimap shows. A round that was not started has a grid without cells.
    Discovery discovery;

    /// Which levers of the maze are pulled, in the order of Interactables::levers
    /// (game::startInteractables). A pulled lever stays pulled until the round ends.
    InteractableState interactables;

    /// How far the wall of every lever has sunk, in the order of the levers: 0 is
    /// standing, 1 is fully in the ground. It grows for GATE_OPEN_SECONDS after the
    /// lever is pulled, like gateProgress.
    std::vector<float> wallProgress;

    /// The walls as they are in THIS round: a copy of the maze of the world that loses
    /// the wall of every pulled lever. The discovery and the minimap read the walls
    /// from here (game::roundMaze). MazeWorld::maze is never changed, so a new round on
    /// the same maze has every wall again. Empty in a round that was not started: Maze
    /// has no default constructor, and std::optional is a box that may hold no value.
    std::optional<Maze> maze;

    /// The note the player is reading: noteOpen is true while its card is shown, and
    /// noteIndex is its number in Interactables::notes (it means nothing otherwise).
    bool noteOpen = false;
    std::size_t noteIndex = 0;

    /// The shade of the round (game/Shade.hpp): where it is and whether it is lit. Not
    /// present in a calm round. updateRoundShade moves it.
    Shade shade;

    /// The line the player reads after being caught (game::caughtLine), or
    /// NO_CAUGHT_LINE. The application sets it in the round it starts after a catch
    /// (showCaughtLine), and caughtSeconds counts the seconds of play since then: the
    /// line is taken away after CAUGHT_LINE_SECONDS.
    int caughtLine = NO_CAUGHT_LINE;
    float caughtSeconds = 0.0F;
};

/// How many of total crystals open the gate: fraction of them, rounded up, at least 1
/// and never more than there are. With no crystals at all the answer is 0, and the gate
/// is open from the start.
int requiredCrystalCount(int total, float fraction);

/// A fresh round on the maze: every crystal in its place, a full battery, the gate
/// closed and the clocks at 0. A maze without crystals needs none, and a maze without
/// a gate has nothing to open: in both cases the round starts with the way out open.
/// Nothing of the maze is discovered except what the player sees from the start
/// (game::discoverAround of MazeWorld::startPosition). No lever is pulled, every wall
/// stands and no note is open.
///
/// The flasks are placed here, GameplaySettings::flaskCount of them, from the seed of
/// the maze (game::placeFlasks), and not when the world is built: so a new round puts
/// every flask back, and the world of a seed is the same with and without flasks.
Round startRound(const MazeWorld& world, const GameplaySettings& settings);

/// The maze with the walls of this round: Round::maze, or the maze of the world for
/// a round that was not started with startRound (it has no copy, and no pulled lever).
const Maze& roundMaze(const MazeWorld& world, const Round& round);

/// Puts every crystal of the round on the ground of the world again: its resting place
/// is CRYSTAL_FLOAT_HEIGHT above the ground at the centre of its cell. Which crystals are
/// collected does not change. Call it after the terrain of the world was rebuilt
/// (game::placeOnTerrain). The round must have been started on this world.
void restCrystalsOnGround(Round& round, const MazeWorld& world);

/// The sphere the player reaches with, for feet standing at the given position.
scene::Sphere playerReach(const glm::vec3& feetPosition);

/// Advances the round by one fixed step of stepSeconds seconds:
///   - the clocks run (the round time only while the round is being played),
///   - the cells the player sees from feetPosition become discovered (also after the
///     round is won),
///   - an open gate keeps sinking, and so does the wall of every pulled lever,
///   - the card of a note closes when the player walks away from the note
///     (NOTE_READ_DISTANCE) or the round is won,
///   - the battery drains while the flashlight is on, faster while its beam is on the
///     shade (game::batteryDrainFactor, with the light the shade had in the step before:
///     the shade is moved after this call),
///   - every crystal whose pickup sphere the player reaches is collected and charges
///     the battery,
///   - every flask the player reaches in the same way is picked up
///     (Round::flasksCollected). What the tea does is not decided here: the round
///     knows nothing about the stamina of the player,
///   - the gate opens when enough crystals are collected,
///   - the round is won when the player is inside the exit zone with the gate open,
///   - the seconds of a caught line count, and the line is taken away when its time is
///     over (showCaughtLine).
///
/// The shade is not moved here: it needs the flashlight of the step, so it has a call of
/// its own that follows this one (updateRoundShade).
///
/// feetPosition is the position of the player after the movement of this step.
/// flashlightOn is the switch of the flashlight (LightingSettings::flashlightOn). It is
/// read to decide whether the battery drains, and it is set to false when the battery
/// is empty: an empty battery cannot be switched on, whoever tries (the F key or
/// a checkbox of the debug UI).
void updateRound(Round& round, const MazeWorld& world, const GameplaySettings& settings,
                 const glm::vec3& feetPosition, bool& flashlightOn, float stepSeconds);

/// The flashlight as the shade of the round sees it (game::shadeLit): it gives light
/// when the switch of settings is on AND the battery of the round is not empty, it
/// stands and points as pose says (game::flashlightPose), and its cone and its range
/// are the ones of settings. The flicker of a low battery does not count: a light that
/// flickers is still on.
ShadeLamp roundShadeLamp(const LightingSettings& settings, const Round& round,
                         const FlashlightPose& pose);

/// Advances the shade of the round by one fixed step (game::advanceShade) and tells
/// what happened in it: a catch, the moment it noticed the player, a banish. Call it
/// after updateRound, with the same position of the feet. lamp is the flashlight of the
/// step (roundShadeLamp), obstacles what blocks its light (roundObstacles) and noise
/// what the player did in the step that can be heard (game::playerNoise).
///
/// The shade walks through the maze of the ROUND (roundMaze), so a wall that a lever
/// has opened is a way for it too. It only moves while the round is being played: after
/// the win it stands where it was and catches nobody. The round is not changed by
/// a catch: the caller starts it again (startRound) and shows the caught line.
ShadeEvents updateRoundShade(Round& round, const MazeWorld& world, const GameplaySettings& settings,
                             const glm::vec3& feetPosition, const ShadeLamp& lamp,
                             std::span<const scene::Aabb> obstacles, float stepSeconds,
                             Noise noise = Noise::None);

/// Shows a caught line in the round from now on (game::caughtLine of line), for
/// CAUGHT_LINE_SECONDS seconds of play.
void showCaughtLine(Round& round, int line);

/// How bright the picture of the round is drawn, from 0 (black) to 1: 1, except in the
/// first moments after the player was carried back, when the picture comes back from
/// black (game::caughtBrightness).
float roundBrightness(const Round& round);

/// True while the gate stands in the way: the maze has one and it has not opened yet.
/// The box of the gate stops being an obstacle at the moment the gate opens, while the
/// model is still at its full height: the gate opens when a crystal is collected
/// somewhere else in the maze, so it has usually sunk before the player gets to it.
bool gateBlocks(const MazeWorld& world, const Round& round);

/// True while some of the gate is above the ground and has to be drawn: the maze has one
/// and it is not fully sunk yet. That is longer than gateBlocks, by the GATE_OPEN_SECONDS
/// the sinking takes.
bool gateVisible(const MazeWorld& world, const Round& round);

/// The sinking of the gate and of a wall that a lever opens, as two small formulas that
/// both of them use. progress runs from 0 (standing) to 1 (fully in the ground).
///
/// sinkProgressAfter: the progress after one more step of stepSeconds seconds. It grows
/// evenly and reaches 1 after GATE_OPEN_SECONDS, where it stays.
/// sinkDepth: how far below its standing position the thing is drawn, in metres: 0 for
/// progress 0, GATE_SINK_DEPTH for progress 1.
float sinkProgressAfter(float progress, float stepSeconds);
float sinkDepth(float progress);

/// How far below its closed position the gate is drawn, in metres: 0 when closed,
/// GATE_SINK_DEPTH when fully open (sinkDepth of Round::gateProgress).
float gateSinkDepth(const Round& round);

/// Pulls lever number index of the round (game::pullLever). The first pull opens the
/// wall of the lever, at that moment and for everything at once:
///   - the wall leaves the maze of the round (Round::maze), so the discovery sees
///     through the opening and the minimap stops drawing the wall,
///   - its box is no longer in the list of roundObstacles, so the player can walk
///     through and the picking ray passes. The caller has to build its copy of that
///     list again,
///   - the wall starts to sink (Round::wallProgress, advanced by updateRound).
/// This is the rule of the gate (gateBlocks): what opens stops being an obstacle when it
/// opens, while its model is still sinking.
///
/// Returns true when this pull opened the wall, false when the lever was pulled before.
/// Throws std::out_of_range when the maze has no lever with that number, or when the
/// round was not started on this world.
bool pullRoundLever(Round& round, const MazeWorld& world, std::size_t index);

/// Pulls every lever of the round that is not pulled yet, for the debug UI. Returns how
/// many walls that opened. The round must have been started on this world.
int pullAllLevers(Round& round, const MazeWorld& world);

/// How many levers of the round are pulled: the number of walls that are open.
int pulledLeverCount(const Round& round);

/// One flag per wall of the world, in the order of MazeWorld::walls: true for a wall
/// that a pulled lever of this round has opened.
std::vector<bool> openedWallFlags(const MazeWorld& world, const Round& round);

/// The model matrices of the walls as this round draws them: MazeWorld::wallMatrices,
/// with the wall of every pulled lever lowered by how far it has sunk (sinkDepth). The
/// list keeps the size and the order of MazeWorld::walls. A wall that is fully sunk stays
/// in it: it then lies below the lowest ground under it, and the terrain hides it.
/// The scene pass and the shadow passes draw the walls from this one list, so a sinking
/// wall casts exactly the shadow of what is still above the ground.
std::vector<glm::mat4> roundWallMatrices(const MazeWorld& world, const Round& round);

/// How far the handle of lever number index has swung: 0 is up (not pulled), 1 is down.
/// It reaches 1 LEVER_PULL_SECONDS after the pull. It is computed from the progress of
/// the wall, which started at the same moment, so the round keeps one number per lever.
/// A lever the round does not have gives 0.
float leverHandleProgress(const Round& round, std::size_t index);

/// Opens the card of note number index: from now on openNoteText gives its text. A note
/// the maze does not have is ignored. closeNote closes the card again.
void readNote(Round& round, const MazeWorld& world, std::size_t index);
void closeNote(Round& round);

/// The text of the note that is open (game::noteText), or an empty text when none is.
/// It is computed when it is asked for: a hint towards a crystal counts only the
/// crystals that are not collected yet, so it changes while the card is open.
std::string openNoteText(const MazeWorld& world, const Round& round);

/// The obstacle list for the player and for the picking ray: the boxes of the maze
/// (MazeWorld::colliders) without the box of every wall that a pulled lever has opened,
/// and the box of the gate as the last one while gateBlocks is true (until the gate
/// opens).
std::vector<scene::Aabb> roundObstacles(const MazeWorld& world, const Round& round);

/// The brightness of the flashlight as a factor from 0 to 1, for a battery charge and
/// a moment in time (seconds on the animation clock).
///
/// 1 while the charge is at or above the low-battery threshold, 0 when it is empty. In
/// between the light flickers: it dips at irregular looking moments, and the dips get
/// deeper as the battery runs down. Nothing here is random: the same charge and the
/// same moment always give the same factor.
float flashlightFlicker(float battery, float seconds, const GameplaySettings& settings);

/// The lighting settings one frame is drawn with: a copy of settings in which the
/// flashlight is off when the battery is empty and dimmed by flashlightFlicker when it
/// is low, and the point lights are dimmed by crystalPulse. settings itself, which the
/// debug UI edits, is never changed by the animation.
LightingSettings lightingForFrame(const LightingSettings& settings, const Round& round,
                                  const GameplaySettings& gameplay);

/// Where the point lights of this moment could hang: above every crystal that is not
/// collected yet, following its bobbing. A frame is drawn with the ones nearest to the
/// eye (game::nearestPointLights), because a large maze has more crystals than the
/// shaders have point lights.
std::vector<glm::vec3> crystalLightPositions(const Round& round);

} // namespace game
