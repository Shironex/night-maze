// Crystals: how many a maze gets, which cells they float in and how they move and glow.
// See docs/modules/game/gameplay.md
#include "game/Crystals.hpp"

#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Light.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>

namespace game {

namespace {

// The generator of the crystals is seeded with the seed of the maze plus this number, so
// it does not repeat the very numbers the maze was carved with. Any number other than
// 0 would do. Adding to an unsigned number wraps around at 2^32, which is well defined.
constexpr std::uint32_t CRYSTAL_SEED_OFFSET = 1000003U;

// A full circle, in degrees and in radians.
constexpr float FULL_TURN_DEGREES = 360.0F;
constexpr float FULL_TURN_RADIANS = glm::two_pi<float>();

// How far along its movement a crystal is ahead of the crystal before it in the list,
// as a part of a full cycle. 0.382 is not a simple fraction (not a half, a third or
// a quarter), so however many crystals there are, no two of them move in step.
constexpr float PHASE_STEP = 0.382F;

// Puts the cells into a random order: every order is equally likely.
//
// Fisher-Yates shuffle, written out by hand. std::shuffle would do the same job, but
// like the distributions it may use the generator differently in each standard library,
// and then the same seed would give other crystals on macOS than on Windows.
void shuffleCells(std::vector<MazeCell>& cells, std::mt19937& generator) {
    // Going from the back: the last place gets one of all the cells, the place before
    // it one of the cells that are left, and so on. Place 0 keeps the cell that remains.
    for (std::size_t last = cells.size(); last > 1; --last) {
        const auto chosen =
            static_cast<std::size_t>(randomBelow(generator, static_cast<std::uint32_t>(last)));
        std::swap(cells[last - 1], cells[chosen]);
    }
}

// The part of a cycle that has passed at a moment in time, from 0 to 1. A cycle takes
// cycleSeconds, and offset (in cycles) moves the starting point.
float cyclePhase(float seconds, float cycleSeconds, float offset) {
    const float cycles = seconds / cycleSeconds + offset;
    // Only the part after the decimal point matters: 3.25 cycles look like 0.25.
    return cycles - std::floor(cycles);
}

} // namespace

int crystalCountFor(int cellCount) {
    // Whole number division rounds down. Adding half of the divisor first makes it round
    // to the nearest: (100 + 4) / 8 = 13 for 12.5, (96 + 4) / 8 = 12 for 12.
    const int rounded = (cellCount + CELLS_PER_CRYSTAL / 2) / CELLS_PER_CRYSTAL;
    return std::clamp(rounded, 1, scene::MAX_POINT_LIGHTS);
}

std::vector<CrystalSpawn> placeCrystals(const Maze& maze, std::uint32_t seed, MazeCell start,
                                        MazeCell exit) {
    if (!maze.contains(start.x, start.z) || !maze.contains(exit.x, exit.z)) {
        throw std::out_of_range("placeCrystals: the start or the exit is outside the maze");
    }

    // The free cells in two lists, each filled row after row: the order before the
    // shuffle is part of the result, like the order of the directions in the generator.
    std::vector<MazeCell> deadEnds;
    std::vector<MazeCell> otherCells;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (cell == start || cell == exit) {
                continue;
            }
            if (isDeadEnd(maze, x, z)) {
                deadEnds.push_back(cell);
            } else {
                otherCells.push_back(cell);
            }
        }
    }

    std::mt19937 generator(seed + CRYSTAL_SEED_OFFSET);
    shuffleCells(deadEnds, generator);
    shuffleCells(otherCells, generator);

    // One list of candidates: every dead end comes before every other cell. A cell is
    // in it once, so no cell can get two crystals.
    std::vector<MazeCell> candidates = deadEnds;
    candidates.insert(candidates.end(), otherCells.begin(), otherCells.end());

    const int cellCount = maze.width() * maze.height();
    const std::size_t count =
        std::min(static_cast<std::size_t>(crystalCountFor(cellCount)), candidates.size());

    std::vector<CrystalSpawn> crystals;
    crystals.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto variant = static_cast<int>(
            randomBelow(generator, static_cast<std::uint32_t>(CRYSTAL_VARIANT_COUNT)));
        crystals.push_back({.cell = candidates[i], .variant = variant});
    }
    return crystals;
}

glm::vec3 crystalRestPosition(MazeCell cell, float groundHeight) {
    return cellCenter(cell.x, cell.z) + glm::vec3{0.0F, groundHeight + CRYSTAL_FLOAT_HEIGHT, 0.0F};
}

glm::vec3 crystalCenter(const glm::vec3& basePosition) {
    return basePosition + glm::vec3{0.0F, CRYSTAL_HEIGHT / 2.0F, 0.0F};
}

glm::vec3 crystalLightPosition(const glm::vec3& basePosition) {
    return basePosition + glm::vec3{0.0F, CRYSTAL_HEIGHT + CRYSTAL_LIGHT_CLEARANCE, 0.0F};
}

glm::vec3 crystalBobPosition(const glm::vec3& restPosition, int index, float seconds) {
    // A sine wave: it goes smoothly from -1 to 1 and back once per full turn of its
    // angle, so the crystal slows down at the top and at the bottom like a float on water.
    const float phase =
        cyclePhase(seconds, CRYSTAL_BOB_SECONDS, static_cast<float>(index) * PHASE_STEP);
    const float lift = CRYSTAL_BOB_AMPLITUDE * std::sin(phase * FULL_TURN_RADIANS);
    return restPosition + glm::vec3{0.0F, lift, 0.0F};
}

float crystalSpinDegrees(int index, float seconds) {
    const float turnSeconds = FULL_TURN_DEGREES / CRYSTAL_SPIN_DEGREES_PER_SECOND;
    return cyclePhase(seconds, turnSeconds, static_cast<float>(index) * PHASE_STEP) *
           FULL_TURN_DEGREES;
}

float crystalPulse(float seconds) {
    // The sine runs from -1 to 1. Half of it plus one half runs from 0 to 1: how far
    // the light is dimmed at this moment.
    const float phase = cyclePhase(seconds, CRYSTAL_PULSE_SECONDS, 0.0F);
    const float dimmed = 0.5F + 0.5F * std::sin(phase * FULL_TURN_RADIANS);
    return 1.0F - CRYSTAL_PULSE_DEPTH * dimmed;
}

glm::vec3 crystalGlow(const glm::vec3& lightColor, float seconds) {
    return lightColor * CRYSTAL_GLOW_STRENGTH * crystalPulse(seconds);
}

} // namespace game
