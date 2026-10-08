// Round: the state of one play through a maze (crystals, gate, levers, battery, time) and
// its rules.
// See docs/modules/game/gameplay.md
#include "game/Round.hpp"

#include "game/Crystals.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace game {

namespace {

// requiredCrystalCount rounds fraction * total up. A float cannot hold most fractions
// exactly, and the product can land a tiny bit above a whole number: 0.3F * 50 comes
// out as 15.000001, which would round up to 16. Taking this much off first keeps whole
// results whole. It is far smaller than the steps the product can make (the fraction
// slider moves in hundredths).
constexpr float ROUNDING_GUARD = 0.001F;

// The shortest battery lifetime the rules compute with, in seconds. It only keeps the
// division in updateRound away from zero, whatever number ends up in the settings.
constexpr float MIN_BATTERY_LIFETIME_SECONDS = 1.0F;

// The flicker of a low battery is made of two sine waves with these speeds, in radians
// per second: about 3.7 and 1.2 swings per second. Their product rises and falls at
// moments that look irregular, because the two speeds are not multiples of each other.
constexpr float FLICKER_FAST_SPEED = 23.0F;
constexpr float FLICKER_SLOW_SPEED = 7.3F;

// How much of the light the deepest dip takes away, just before the battery is empty.
constexpr float FLICKER_DEPTH = 0.85F;

// The battery loses the same share of its charge in every step the light is on: all of
// it in batteryLifetimeSeconds.
void drainBattery(Round& round, const GameplaySettings& settings, bool flashlightOn,
                  float stepSeconds) {
    if (flashlightOn && settings.batteryDrains) {
        const float lifetime =
            std::max(settings.batteryLifetimeSeconds, MIN_BATTERY_LIFETIME_SECONDS);
        round.battery -= stepSeconds / lifetime;
    }
    // The debug UI can write any number into the battery, so both ends are held here.
    round.battery = std::clamp(round.battery, 0.0F, 1.0F);
}

// Collects every crystal whose pickup sphere the reach of the player overlaps. The
// pickup sphere is centred on the resting crystal and does not bob with it, so the
// distance to walk does not depend on the moment.
void collectCrystals(Round& round, const GameplaySettings& settings, const scene::Sphere& reach) {
    for (RoundCrystal& crystal : round.crystals) {
        if (crystal.collected) {
            continue;
        }
        const scene::Sphere pickup{.center = crystalCenter(crystal.restPosition),
                                   .radius = settings.pickupRadius};
        if (scene::overlaps(reach, pickup)) {
            crystal.collected = true;
            ++round.collectedCount;
            round.battery = std::min(round.battery + settings.batteryPerCrystal, 1.0F);
        }
    }
}

// Picks up every flask whose pickup sphere the reach of the player overlaps, like
// collectCrystals. A flask is small, so the sphere is centred on the place it rests at.
void collectFlasks(Round& round, const MazeWorld& world, const GameplaySettings& settings,
                   const scene::Sphere& reach) {
    for (RoundFlask& flask : round.flasks) {
        if (flask.collected) {
            continue;
        }
        const scene::Sphere pickup{
            .center = flaskRestPosition(flask.cell, groundHeightAt(world, flask.cell)),
            .radius = settings.pickupRadius};
        if (scene::overlaps(reach, pickup)) {
            flask.collected = true;
            ++round.flasksCollected;
        }
    }
}

// Closes the card of the open note when the player has walked away from it. The
// distance is measured on the ground (x and z only), so a slope does not count.
void closeNoteFarAway(Round& round, const MazeWorld& world, const glm::vec3& feetPosition) {
    if (!round.noteOpen || round.noteIndex >= world.interactables.notes.size()) {
        return;
    }
    const glm::vec3 note = world.interactables.notes[round.noteIndex].position;
    const glm::vec2 away{feetPosition.x - note.x, feetPosition.z - note.z};
    if (glm::length(away) > NOTE_READ_DISTANCE) {
        closeNote(round);
    }
}

} // namespace

int requiredCrystalCount(int total, float fraction) {
    if (total <= 0) {
        return 0;
    }
    const auto needed =
        static_cast<int>(std::ceil(fraction * static_cast<float>(total) - ROUNDING_GUARD));
    // At least one crystal, also for a fraction of 0, and never more than there are.
    return std::clamp(needed, 1, total);
}

Round startRound(const MazeWorld& world, const GameplaySettings& settings) {
    Round round;
    round.crystals.reserve(world.crystals.size());
    for (const CrystalSpawn& spawn : world.crystals) {
        round.crystals.push_back(
            {.restPosition = crystalRestPosition(spawn.cell, groundHeightAt(world, spawn.cell)),
             .variant = spawn.variant});
    }
    round.requiredCount =
        requiredCrystalCount(static_cast<int>(round.crystals.size()), settings.requiredFraction);

    // The flasks, in the cells the seed of the maze gives them. They know the crystals:
    // a flask never lies in the cell of one.
    for (const MazeCell cell : placeFlasks(world.maze, world.seed, START_CELL, world.exitCell,
                                           world.crystals, settings.flaskCount)) {
        round.flasks.push_back({.cell = cell});
    }

    // Nothing to collect: the gate does not wait for anything.
    round.gateOpen = round.requiredCount == 0;
    // No gate at all: the way is as open as it will ever be, there is nothing to sink.
    if (!world.hasGate) {
        round.gateOpen = true;
        round.gateProgress = 1.0F;
    }

    // The minimap starts empty, except for what can be seen from the start: the start
    // cell and the straight corridors that leave it. It is done here and not left to
    // the first step, so that the very first frame already shows it.
    round.discovery = Discovery(world.maze.width(), world.maze.height());
    discoverAround(round.discovery, world.maze, world.startPosition);

    // The levers: none is pulled, so every wall stands. The round gets its own copy of
    // the maze, which loses a wall when its lever is pulled (pullRoundLever).
    round.interactables = startInteractables(world.interactables);
    round.wallProgress.assign(world.interactables.levers.size(), 0.0F);
    round.maze = world.maze;

    // The shade, in the cell the seed of the maze gives it, far from the start. Placed
    // here like the flasks, so a new round puts it back. A calm round has none.
    round.shade = startShade(world.maze, world.seed, START_CELL, world.exitCell, world.terrain,
                             settings.shade);
    return round;
}

const Maze& roundMaze(const MazeWorld& world, const Round& round) {
    // has_value: whether the box holds a maze. The star takes it out.
    return round.maze.has_value() ? *round.maze : world.maze;
}

void restCrystalsOnGround(Round& round, const MazeWorld& world) {
    // The crystals of a round are in the order of MazeWorld::crystals. The smaller of
    // the two sizes guards against a round that belongs to another world.
    const std::size_t count = std::min(round.crystals.size(), world.crystals.size());
    for (std::size_t i = 0; i < count; ++i) {
        const MazeCell cell = world.crystals[i].cell;
        round.crystals[i].restPosition = crystalRestPosition(cell, groundHeightAt(world, cell));
    }
}

scene::Sphere playerReach(const glm::vec3& feetPosition) {
    return {.center = feetPosition + glm::vec3{0.0F, PLAYER_REACH_HEIGHT, 0.0F},
            .radius = PLAYER_REACH_RADIUS};
}

void updateRound(Round& round, const MazeWorld& world, const GameplaySettings& settings,
                 const glm::vec3& feetPosition, bool& flashlightOn, float stepSeconds) {
    // These two run in every step, also after the round is won: the crystals keep
    // bobbing, and a gate that was still sinking when the round ended finishes.
    round.animationSeconds += stepSeconds;
    if (round.gateOpen) {
        round.gateProgress = sinkProgressAfter(round.gateProgress, stepSeconds);
    }
    // The wall of every pulled lever sinks like the gate. The smaller of the two sizes
    // guards against a state that was written by hand.
    const std::size_t leverCount =
        std::min(round.wallProgress.size(), round.interactables.leverPulled.size());
    for (std::size_t i = 0; i < leverCount; ++i) {
        if (round.interactables.leverPulled[i]) {
            round.wallProgress[i] = sinkProgressAfter(round.wallProgress[i], stepSeconds);
        }
    }

    // What the player sees from where the feet are after this step goes onto the
    // minimap. Also after the round is won: the player can still walk around. The walls
    // are read from the maze of the ROUND in every step, so the view passes a wall from
    // the step after its lever was pulled.
    discoverAround(round.discovery, roundMaze(world, round), feetPosition);

    // A note is read standing in front of it: the card closes when the player leaves.
    closeNoteFarAway(round, world, feetPosition);

    // A won round is over: its time, its battery and its crystals stay as they were at
    // the moment of the win.
    if (round.state == RoundState::Playing) {
        round.elapsedSeconds += stepSeconds;
        drainBattery(round, settings, flashlightOn, stepSeconds);

        // The caught line stays for a few seconds of play, then it is gone.
        if (round.caughtLine != NO_CAUGHT_LINE) {
            round.caughtSeconds += stepSeconds;
            if (round.caughtSeconds >= CAUGHT_LINE_SECONDS) {
                round.caughtLine = NO_CAUGHT_LINE;
            }
        }

        const scene::Sphere reach = playerReach(feetPosition);
        collectCrystals(round, settings, reach);
        collectFlasks(round, world, settings, reach);

        // The number needed is computed again in every step, because the debug UI can
        // change the fraction in the middle of a round. An open gate stays open.
        round.requiredCount = requiredCrystalCount(static_cast<int>(round.crystals.size()),
                                                   settings.requiredFraction);
        if (round.collectedCount >= round.requiredCount) {
            round.gateOpen = true;
        }

        // The way out. The zone lies behind the gate, so with collisions the player
        // gets there only after the gate has opened. The open flag is asked for all the
        // same: in noclip mode the player flies through a closed gate, and that must
        // not win.
        if (round.gateOpen && scene::overlaps(reach, world.exitZone)) {
            round.state = RoundState::Won;
            // The card of the win takes the place of the card of a note.
            closeNote(round);
        }
    }

    // An empty battery switches the light off and keeps it off, in every state of the
    // round and whoever switched it on. It comes after the pickups on purpose: a crystal
    // collected in the very step the battery ran out saves the light.
    if (round.battery <= 0.0F) {
        flashlightOn = false;
    }
}

ShadeLamp roundShadeLamp(const LightingSettings& settings, const Round& round,
                         const FlashlightPose& pose) {
    return {.on = settings.flashlightOn && round.battery > 0.0F,
            .position = pose.position,
            .direction = pose.direction,
            .outerDegrees = settings.flashlightOuterDegrees,
            .range = settings.flashlightRange};
}

bool updateRoundShade(Round& round, const MazeWorld& world, const GameplaySettings& settings,
                      const glm::vec3& feetPosition, const ShadeLamp& lamp,
                      std::span<const scene::Aabb> obstacles, float stepSeconds) {
    if (round.state != RoundState::Playing) {
        return false;
    }
    const ShadeStep step{.playerFeet = feetPosition,
                         .lamp = lamp,
                         .obstacles = obstacles,
                         .openedWalls = pulledLeverCount(round)};
    return advanceShade(round.shade, settings.shade, roundMaze(world, round), world.terrain, step,
                        stepSeconds);
}

void showCaughtLine(Round& round, int line) {
    round.caughtLine = line;
    round.caughtSeconds = 0.0F;
}

float roundBrightness(const Round& round) {
    return round.caughtLine == NO_CAUGHT_LINE ? 1.0F : caughtBrightness(round.caughtSeconds);
}

bool gateBlocks(const MazeWorld& world, const Round& round) {
    return world.hasGate && !round.gateOpen;
}

bool gateVisible(const MazeWorld& world, const Round& round) {
    return world.hasGate && round.gateProgress < 1.0F;
}

float sinkProgressAfter(float progress, float stepSeconds) {
    // The share of the whole way that one step covers is stepSeconds / GATE_OPEN_SECONDS.
    return std::min(progress + stepSeconds / GATE_OPEN_SECONDS, 1.0F);
}

float sinkDepth(float progress) {
    return progress * GATE_SINK_DEPTH;
}

float gateSinkDepth(const Round& round) {
    return sinkDepth(round.gateProgress);
}

bool pullRoundLever(Round& round, const MazeWorld& world, std::size_t index) {
    // pullLever checks the number and the state and throws for a wrong one.
    const PullResult result = pullLever(round.interactables, world.interactables, index);
    if (!result.opened) {
        return false;
    }
    // The wall is gone for the discovery and the minimap from now on. Only the copy of
    // the round changes. Removing a wall updates both cells it stands between.
    if (round.maze.has_value()) {
        round.maze->removeWall(result.wall.cell.x, result.wall.cell.z, result.wall.side);
    }
    return true;
}

int pullAllLevers(Round& round, const MazeWorld& world) {
    int opened = 0;
    for (std::size_t i = 0; i < world.interactables.levers.size(); ++i) {
        if (pullRoundLever(round, world, i)) {
            ++opened;
        }
    }
    return opened;
}

int pulledLeverCount(const Round& round) {
    int count = 0;
    for (const bool pulled : round.interactables.leverPulled) {
        if (pulled) {
            ++count;
        }
    }
    return count;
}

std::vector<bool> openedWallFlags(const MazeWorld& world, const Round& round) {
    std::vector<bool> opened(world.walls.size(), false);
    // The smaller of the two sizes guards against a round that belongs to another world.
    const std::size_t leverCount =
        std::min(world.leverWalls.size(), round.interactables.leverPulled.size());
    for (std::size_t i = 0; i < leverCount; ++i) {
        const std::size_t wall = world.leverWalls[i];
        if (round.interactables.leverPulled[i] && wall < opened.size()) {
            opened[wall] = true;
        }
    }
    return opened;
}

std::vector<glm::mat4> roundWallMatrices(const MazeWorld& world, const Round& round) {
    std::vector<glm::mat4> matrices = world.wallMatrices;
    const std::size_t leverCount =
        std::min({world.leverWalls.size(), round.interactables.leverPulled.size(),
                  round.wallProgress.size()});
    for (std::size_t i = 0; i < leverCount; ++i) {
        const std::size_t wall = world.leverWalls[i];
        if (!round.interactables.leverPulled[i] || wall >= matrices.size()) {
            continue;
        }
        // The same wall, lower: the matrix is made again from the lowered segment, the
        // way GameplayRenderer lowers the gate.
        WallSegment lowered = world.walls[wall];
        lowered.position.y -= sinkDepth(round.wallProgress[i]);
        matrices[wall] = wallModelMatrix(lowered);
    }
    return matrices;
}

float leverHandleProgress(const Round& round, std::size_t index) {
    if (index >= round.wallProgress.size()) {
        return 0.0F;
    }
    // The wall has been sinking for wallProgress * GATE_OPEN_SECONDS seconds: that is
    // the time since the pull (until the wall is down, and by then the handle is too).
    const float secondsSincePull = round.wallProgress[index] * GATE_OPEN_SECONDS;
    return std::min(secondsSincePull / LEVER_PULL_SECONDS, 1.0F);
}

void readNote(Round& round, const MazeWorld& world, std::size_t index) {
    if (index >= world.interactables.notes.size()) {
        return;
    }
    round.noteOpen = true;
    round.noteIndex = index;
}

void closeNote(Round& round) {
    round.noteOpen = false;
}

std::string openNoteText(const MazeWorld& world, const Round& round) {
    if (!round.noteOpen || round.noteIndex >= world.interactables.notes.size()) {
        return {};
    }

    // The cells of the crystals that are still there. The crystals of a round are in
    // the order of MazeWorld::crystals, which knows their cells.
    std::vector<MazeCell> crystalCells;
    const std::size_t crystalCount = std::min(round.crystals.size(), world.crystals.size());
    for (std::size_t i = 0; i < crystalCount; ++i) {
        if (!round.crystals[i].collected) {
            crystalCells.push_back(world.crystals[i].cell);
        }
    }
    return noteText(world.interactables.notes[round.noteIndex], world.exitCell, crystalCells);
}

std::vector<scene::Aabb> roundObstacles(const MazeWorld& world, const Round& round) {
    // world.colliders holds the box of every wall first, in the order of world.walls,
    // and the boxes of the pillars after them. So box number i belongs to wall number
    // i as long as i is below the number of walls. An opened wall is left out at once,
    // also while its model is still sinking (see pullRoundLever).
    const std::vector<bool> opened = openedWallFlags(world, round);
    std::vector<scene::Aabb> obstacles;
    obstacles.reserve(world.colliders.size() + 1);
    for (std::size_t i = 0; i < world.colliders.size(); ++i) {
        if (i < opened.size() && opened[i]) {
            continue;
        }
        obstacles.push_back(world.colliders[i]);
    }
    if (gateBlocks(world, round)) {
        obstacles.push_back(world.gateBox);
    }
    return obstacles;
}

float flashlightFlicker(float battery, float seconds, const GameplaySettings& settings) {
    if (battery <= 0.0F) {
        return 0.0F;
    }
    // Also covers a threshold of 0 (no flicker at all), so the division below is safe.
    if (battery >= settings.lowBatteryThreshold) {
        return 1.0F;
    }

    // How weak the battery is: 0 at the threshold, 1 when it is empty.
    const float weakness = 1.0F - battery / settings.lowBatteryThreshold;

    // Each sine is between -1 and 1, and so is their product. The negative half is cut
    // off: half of the time the light is steady, the other half it dips.
    const float wave =
        std::sin(FLICKER_FAST_SPEED * seconds) * std::sin(FLICKER_SLOW_SPEED * seconds);
    const float dip = std::max(wave, 0.0F);

    // weakness, dip and FLICKER_DEPTH are all between 0 and 1, so the result is too.
    return 1.0F - FLICKER_DEPTH * weakness * dip;
}

LightingSettings lightingForFrame(const LightingSettings& settings, const Round& round,
                                  const GameplaySettings& gameplay) {
    LightingSettings frame = settings;
    frame.flashlightOn = settings.flashlightOn && round.battery > 0.0F;
    frame.flashlightIntensity *= flashlightFlicker(round.battery, round.animationSeconds, gameplay);
    frame.pointIntensity *= crystalPulse(round.animationSeconds);
    return frame;
}

std::vector<glm::vec3> crystalLightPositions(const Round& round) {
    std::vector<glm::vec3> positions;
    for (std::size_t i = 0; i < round.crystals.size(); ++i) {
        const RoundCrystal& crystal = round.crystals[i];
        if (crystal.collected) {
            continue;
        }
        const glm::vec3 base =
            crystalBobPosition(crystal.restPosition, static_cast<int>(i), round.animationSeconds);
        positions.push_back(crystalLightPosition(base));
    }
    return positions;
}

} // namespace game
