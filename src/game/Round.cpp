// Round: the state of one play through a maze (crystals, gate, battery, time) and its rules.
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
            {.restPosition = crystalRestPosition(spawn.cell), .variant = spawn.variant});
    }
    round.requiredCount =
        requiredCrystalCount(static_cast<int>(round.crystals.size()), settings.requiredFraction);

    // Nothing to collect: the gate does not wait for anything.
    round.gateOpen = round.requiredCount == 0;
    // No gate at all: the way is as open as it will ever be, there is nothing to sink.
    if (!world.hasGate) {
        round.gateOpen = true;
        round.gateProgress = 1.0F;
    }
    return round;
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
        round.gateProgress = std::min(round.gateProgress + stepSeconds / GATE_OPEN_SECONDS, 1.0F);
    }

    // A won round is over: its time, its battery and its crystals stay as they were at
    // the moment of the win.
    if (round.state == RoundState::Playing) {
        round.elapsedSeconds += stepSeconds;
        drainBattery(round, settings, flashlightOn, stepSeconds);

        const scene::Sphere reach = playerReach(feetPosition);
        collectCrystals(round, settings, reach);

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
        }
    }

    // An empty battery switches the light off and keeps it off, in every state of the
    // round and whoever switched it on. It comes after the pickups on purpose: a crystal
    // collected in the very step the battery ran out saves the light.
    if (round.battery <= 0.0F) {
        flashlightOn = false;
    }
}

bool gateBlocks(const MazeWorld& world, const Round& round) {
    return world.hasGate && !round.gateOpen;
}

bool gateVisible(const MazeWorld& world, const Round& round) {
    return world.hasGate && round.gateProgress < 1.0F;
}

float gateSinkDepth(const Round& round) {
    return round.gateProgress * GATE_SINK_DEPTH;
}

std::vector<scene::Aabb> roundObstacles(const MazeWorld& world, const Round& round) {
    std::vector<scene::Aabb> obstacles = world.colliders;
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
