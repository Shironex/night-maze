// Round: the state of one play through a maze (crystals, gate, battery, time) and its rules.
// See docs/modules/game/gameplay.md
#pragma once

#include "game/Lighting.hpp"
#include "game/MazeWorld.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace game {

// Plain data and free functions without OpenGL, like the rest of the game_logic library,
// so tests can play a whole round without a window.

/// How long the gate takes to sink into the ground after it opens, in seconds.
constexpr float GATE_OPEN_SECONDS = 1.5F;

/// How far below its closed position the gate is when it is fully open, in metres: a
/// little more than the pillars are high (3.15 m), so nothing of it is left above the
/// floor.
constexpr float GATE_SINK_DEPTH = 3.3F;

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

    /// False stops the battery from draining: a switch for testing in the debug UI.
    bool batteryDrains = true;

    /// True when a new round on the same maze was asked for and has not started yet.
    /// The debug UI sets it, the application starts the round at the start of the next
    /// frame and clears it, like MazeSettings::regenerate.
    bool restart = false;
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

/// The state of one round: everything that changes while a maze is played and starts
/// again when the round is restarted. The maze itself (MazeWorld) does not change.
struct Round {
    RoundState state = RoundState::Playing;

    /// The crystals, in the order of MazeWorld::crystals.
    std::vector<RoundCrystal> crystals;

    /// How many crystals are collected, and how many are needed to open the gate.
    int collectedCount = 0;
    int requiredCount = 0;

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
};

/// How many of total crystals open the gate: fraction of them, rounded up, at least 1
/// and never more than there are. With no crystals at all the answer is 0, and the gate
/// is open from the start.
int requiredCrystalCount(int total, float fraction);

/// A fresh round on the maze: every crystal in its place, a full battery, the gate
/// closed and the clocks at 0. A maze without crystals needs none, and a maze without
/// a gate has nothing to open: in both cases the round starts with the way out open.
Round startRound(const MazeWorld& world, const GameplaySettings& settings);

/// The sphere the player reaches with, for feet standing at the given position.
scene::Sphere playerReach(const glm::vec3& feetPosition);

/// Advances the round by one fixed step of stepSeconds seconds:
///   - the clocks run (the round time only while the round is being played),
///   - an open gate keeps sinking,
///   - the battery drains while the flashlight is on,
///   - every crystal whose pickup sphere the player reaches is collected and charges
///     the battery,
///   - the gate opens when enough crystals are collected,
///   - the round is won when the player is inside the exit zone with the gate open.
///
/// feetPosition is the position of the player after the movement of this step.
/// flashlightOn is the switch of the flashlight (LightingSettings::flashlightOn). It is
/// read to decide whether the battery drains, and it is set to false when the battery
/// is empty: an empty battery cannot be switched on, whoever tries (the F key or
/// a checkbox of the debug UI).
void updateRound(Round& round, const MazeWorld& world, const GameplaySettings& settings,
                 const glm::vec3& feetPosition, bool& flashlightOn, float stepSeconds);

/// True while the gate stands in the way: the maze has one and it has not opened yet.
/// The box of the gate stops being an obstacle at the moment the gate opens, while the
/// model is still at its full height: the gate opens when a crystal is collected
/// somewhere else in the maze, so it has usually sunk before the player gets to it.
bool gateBlocks(const MazeWorld& world, const Round& round);

/// True while some of the gate is above the floor and has to be drawn: the maze has one
/// and it is not fully sunk yet. That is longer than gateBlocks, by the GATE_OPEN_SECONDS
/// the sinking takes.
bool gateVisible(const MazeWorld& world, const Round& round);

/// How far below its closed position the gate is drawn, in metres: 0 when closed,
/// GATE_SINK_DEPTH when fully open.
float gateSinkDepth(const Round& round);

/// The obstacle list for the player: the boxes of the maze (MazeWorld::colliders), and
/// the box of the gate as the last one while gateBlocks is true (until the gate opens).
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

/// Where the point lights of this moment hang: above every crystal that is not
/// collected yet, following its bobbing. This is the list buildLightSet takes.
std::vector<glm::vec3> crystalLightPositions(const Round& round);

} // namespace game
