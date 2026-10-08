// GateLamp: the lantern of the exit gate (how bright it is and in which colour) and where
// the gatehouse, its three lanterns, its light and the milestone stand.
#pragma once

#include "game/Lighting.hpp"
#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"

#include <glm/glm.hpp>

#include <array>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.
//
// The rule of the lamp: it is never off. While the gate is closed it holds a cold ember
// in the colour of the moon, which grows a little with every crystal the player collects.
// When the gate opens it turns amber, over the time the door takes to sink. So the exit
// can be told from an ordinary corner before the gate is open, and the open gate is the
// one warm light of the maze.

/// How strongly the glass of a fully lit lantern glows, compared with its (linear)
/// colour: the same kind of number as CRYSTAL_GLOW_STRENGTH (4), a little above it, so
/// the open gate is the brightest thing of the night. The glass is the pale part of the
/// lantern texture and the iron the dark part, so this one number lights the glass and
/// leaves the frame dark.
constexpr float GATE_LAMP_GLOW_STRENGTH = 5.0F;

/// The strength of the ember while the gate is closed, as a part of the full glow: with
/// no crystal collected, and with every needed crystal collected. Both are low enough
/// that the closed lamp can never be taken for the open one.
constexpr float GATE_LAMP_EMBER_MIN = 0.2F;
constexpr float GATE_LAMP_EMBER_MAX = 0.4F;

/// How far the light of the lamp reaches on the stone around the gate, in metres: three
/// cells. Twice the reach of a crystal light (3 m).
constexpr float GATE_LAMP_RADIUS = 6.0F;

/// The intensity of the light of a fully lit lamp. It is a number of its own and not
/// LightingSettings::pointIntensity: that one pulses with the crystals, and a lamp does
/// not pulse.
constexpr float GATE_LAMP_LIGHT_INTENSITY = 1.1F;

/// The two colours of the lamp, as sRGB values: the cold blue white of the moon light
/// (LightingSettings::moonColor) while the gate is closed, and amber once it is open.
/// Amber is the one warm colour of the night: the crystals are teal and the moon is
/// blue.
constexpr glm::vec3 GATE_LAMP_COLD_COLOR{0.55F, 0.65F, 1.0F};
constexpr glm::vec3 GATE_LAMP_WARM_COLOR{1.0F, 0.72F, 0.33F};

/// How long the bell of the open gate waits between two tolls, in seconds
/// (game::advanceGateBell in game/SoundCues.hpp). It is written here, next to the other
/// numbers of the gate that the debug UI edits.
constexpr float GATE_BELL_SECONDS = 6.0F;

/// The numbers of the gate that can be changed while the game runs. The debug UI edits
/// them. The values here are the defaults.
struct GateSettings {
    /// GATE_LAMP_GLOW_STRENGTH.
    float glowStrength = GATE_LAMP_GLOW_STRENGTH;
    /// GATE_LAMP_EMBER_MIN and GATE_LAMP_EMBER_MAX.
    float emberMin = GATE_LAMP_EMBER_MIN;
    float emberMax = GATE_LAMP_EMBER_MAX;
    /// GATE_LAMP_RADIUS.
    float lightRadius = GATE_LAMP_RADIUS;
    /// GATE_BELL_SECONDS.
    float bellSeconds = GATE_BELL_SECONDS;
};

/// How brightly the lamp of the gate burns, from 0 to 1.
///
/// collected and needed are the crystals of the round (Round::collectedCount and
/// Round::requiredCount) and gateProgress is how far the gate has sunk (0 closed, 1 fully
/// open, Round::gateProgress).
///
///   - Closed: the ember. settings.emberMin with no crystal, settings.emberMax with all
///     the needed ones, and evenly in between. It never falls when a crystal is added.
///   - Opening: from the ember up to 1, evenly with gateProgress. The gate opens at the
///     moment the last needed crystal is collected, when the ember is at its maximum,
///     and gateProgress is still 0 then: so nothing jumps at that moment.
///   - Fully open: exactly 1.
///
/// A round that needs no crystal (needed is 0) has its ember at the maximum. More
/// crystals than needed count as all of them. Settings out of order (a minimum above the
/// maximum, typed into a slider) are taken in order.
float gateLampStrength(int collected, int needed, float gateProgress,
                       const GateSettings& settings = {});

/// The colour of the lamp as an sRGB value: GATE_LAMP_COLD_COLOR for a closed gate
/// (gateProgress 0), GATE_LAMP_WARM_COLOR for a fully open one (1), blended evenly in
/// between.
glm::vec3 gateLampColor(float gateProgress);

/// The point light of the lamp, for the list the lights of a frame are chosen from
/// (game::nearestPointLightSpots): at position, in the colour of gateLampColor(gateProgress),
/// reaching settings.lightRadius, and as bright as GATE_LAMP_LIGHT_INTENSITY times
/// strength (gateLampStrength). So the light on the stone is cold and weak while the gate
/// is closed and amber once it is open, exactly like the glass.
PointLightSpot gateLampLight(const glm::vec3& position, float strength, float gateProgress,
                             const GateSettings& settings = {});

// Where things stand at the gate. All numbers are metres. Heights are measured from the
// base of the gate (MazeWorld::gate, the lowest ground under it), like the model of the
// gate itself.

/// The lantern model is drawn three times: the big one in the cote on top of the
/// gatehouse, and two small ones that hang on the piers and face the approach.
constexpr int GATE_LANTERN_COUNT = 3;

/// The big lantern: how high its base hangs and how many times larger than the model it
/// is drawn. The model is 0.2 m wide with the middle of its glass 0.13 m above its base,
/// so the glass of the big one is centred 5.1 m up: 2 m above the walls (WALL_HEIGHT).
constexpr float GATE_COTE_LANTERN_HEIGHT = 4.84F;
constexpr float GATE_COTE_LANTERN_SCALE = 2.0F;

/// The two bracket lanterns: how high their bases hang (above the head of the player,
/// who is 1.8 m tall), how far from the middle of the gate along it, and how far in
/// front of it on the side of the approach.
constexpr float GATE_BRACKET_LANTERN_HEIGHT = 2.22F;
constexpr float GATE_BRACKET_LANTERN_ALONG = 0.7F;
constexpr float GATE_BRACKET_LANTERN_OUT = 0.42F;

/// The point light: at the height of the door head, between the two bracket lanterns.
/// Not up in the cote: a light of 6 m that hangs 5 m up leaves under a tenth of itself
/// on the floor.
constexpr float GATE_LIGHT_HEIGHT = 2.5F;
constexpr float GATE_LIGHT_OUT = 0.45F;

/// The milestone: how far its middle stands from the middle of its cell towards the wall
/// it leans on, and towards the end of the cell that is far from the gate. With these
/// numbers the stone (0.25 m wide) is clear of the box of the wall and of the pillar.
constexpr float MILESTONE_TO_WALL = 0.7F;
constexpr float MILESTONE_TO_MOUTH = 0.55F;

/// The side of the exit cell the gate stands on: the one way out of the cell.
/// The world must have a gate (MazeWorld::hasGate).
Direction gateSide(const MazeWorld& world);

/// The cell in front of the gate: the neighbour of the exit cell through the gate, the
/// last cell the player walks before the exit. The world must have a gate.
MazeCell approachCell(const MazeWorld& world);

/// Everything that is drawn at the gate and never moves.
struct GateScenery {
    /// The model matrix of the gatehouse (the model gate_arch): the matrix of the gate
    /// as a wall segment. The model is the same seen from both sides and from both ends,
    /// so it needs no turn for the side of the approach.
    glm::mat4 arch{1.0F};

    /// The model matrices of the lanterns: the big one first, then the two on the piers.
    std::array<glm::mat4, GATE_LANTERN_COUNT> lanterns{glm::mat4{1.0F}, glm::mat4{1.0F},
                                                       glm::mat4{1.0F}};

    /// Where the point light of the lamp hangs.
    glm::vec3 lightPosition{0.0F};

    /// False when the cell in front of the gate has no wall a stone could stand against:
    /// the milestone is then left out.
    bool hasMilestone = false;
    /// The wall the milestone stands against (a side of approachCell), and its model
    /// matrix: on the ground, near the end of the cell that is far from the gate.
    Direction milestoneSide = Direction::North;
    glm::mat4 milestone{1.0F};
};

/// Where the gatehouse, the lanterns, the light and the milestone of a world stand.
///
/// The bracket lanterns and the light are on the side of the approach, the side the
/// player comes from. The milestone stands in the approach cell against the first of
/// these walls that is there: the one to the left of somebody who walks towards the
/// gate, the one to the right, the one behind them. A wall that a lever opens, or that
/// carries a lever or a note, does not count: the stone would stand in front of a plate
/// or beside a wall that sinks.
///
/// The world must have a gate. Call it again after the terrain was rebuilt
/// (game::placeOnTerrain): the heights come from the world.
GateScenery gateScenery(const MazeWorld& world);

} // namespace game
