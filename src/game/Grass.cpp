// Grass: where the tufts of grass stand, one point per tuft, chosen from the seed of the maze.
#include "game/Grass.hpp"

#include "game/GateLamp.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Stile.hpp"
#include "game/Terrain.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <random>

namespace game {

namespace {

// The generator of the grass is seeded with the seed of the maze plus this number, so it
// repeats neither the numbers the maze was carved with nor the ones of the crystals. Any
// number other than those two offsets would do.
constexpr std::uint32_t GRASS_SEED_OFFSET = 2000003U;

// randomUnit cuts the range from 0 to 1 into this many equal steps.
constexpr std::uint32_t RANDOM_STEPS = 4096U;

// The two sides of a wall: towards the smaller and towards the larger coordinate.
constexpr std::array<float, 2> WALL_SIDES = {-1.0F, 1.0F};

// A random number from 0 to just below 1, in steps of 1 / RANDOM_STEPS.
//
// It replaces std::uniform_real_distribution for the reason given at randomBelow: how
// a distribution uses the generator differs between the standard libraries, and then the
// same seed would grow other grass on macOS than on Windows. A whole number from
// randomBelow divided by a power of two is the same float everywhere.
float randomUnit(std::mt19937& generator) {
    return static_cast<float>(randomBelow(generator, RANDOM_STEPS)) /
           static_cast<float>(RANDOM_STEPS);
}

// One tuft at (x, z), standing on the ground, with a random number of its own.
GrassTuft tuftAt(const Terrain& terrain, float x, float z, std::mt19937& generator) {
    return {.position = {x, terrain.heightAt(x, z), z}, .random = randomUnit(generator)};
}

// The tufts along both sides of one wall.
void plantAlongWall(const WallSegment& wall, const Terrain& terrain, int tuftsPerSide,
                    std::mt19937& generator, std::vector<GrassTuft>& tufts) {
    // How far from the middle of the wall a tuft may stand along it, to either end.
    constexpr float REACH = WALL_LENGTH / 2.0F - GRASS_END_CLEARANCE;
    // Where the strip starts, measured from the centre line of the wall.
    constexpr float STRIP_START = WALL_COLLISION_THICKNESS / 2.0F + GRASS_WALL_GAP;

    for (const float side : WALL_SIDES) {
        for (int i = 0; i < tuftsPerSide; ++i) {
            // Two random numbers per tuft, always drawn in this order: the place along
            // the wall, then the distance from it.
            const float along = glm::mix(-REACH, REACH, randomUnit(generator));
            const float across = side * (STRIP_START + GRASS_STRIP_WIDTH * randomUnit(generator));

            // A wall along X runs left and right of its middle in x, and its sides are
            // in z. For a wall along Z the two swap.
            const bool alongX = wall.axis == WallAxis::AlongX;
            const float x = wall.position.x + (alongX ? along : across);
            const float z = wall.position.z + (alongX ? across : along);
            tufts.push_back(tuftAt(terrain, x, z, generator));
        }
    }
}

// The sparse tufts on the land outside the maze.
void plantOnHills(const MazeWorld& world, float density, std::mt19937& generator,
                  std::vector<GrassTuft>& tufts) {
    const Terrain& terrain = world.terrain;
    const float landWidth = terrain.maxX() - terrain.minX();
    const float landDepth = terrain.maxZ() - terrain.minZ();

    // Places are tried all over the land, the maze included, and the ones too close to
    // the maze are thrown away. The number of tries is the number of tufts the whole
    // land would get, so what is left has the wanted density. A fixed number of tries
    // (and not "until enough tufts are found") keeps the loop short and its result the
    // same everywhere.
    const float tuftsPerSquareMetre =
        GRASS_HILL_TUFTS_PER_SQUARE_METRE * density / DEFAULT_GRASS_DENSITY;
    const int tries = static_cast<int>(std::lround(landWidth * landDepth * tuftsPerSquareMetre));

    for (int i = 0; i < tries; ++i) {
        const float x = terrain.minX() + landWidth * randomUnit(generator);
        const float z = terrain.minZ() + landDepth * randomUnit(generator);
        if (distanceOutsideMaze(x, z, world.maze.width(), world.maze.height()) <
            GRASS_HILL_CLEARANCE) {
            continue;
        }
        tufts.push_back(tuftAt(terrain, x, z, generator));
    }
}

} // namespace

std::vector<GrassTuft> placeGrass(const MazeWorld& world, float density) {
    std::vector<GrassTuft> tufts;
    if (density <= 0.0F) {
        return tufts;
    }

    std::mt19937 generator(world.seed + GRASS_SEED_OFFSET);

    // Whole tufts only: 2.5 tufts per metre on a wall of 2 m are 5 on each side.
    const int tuftsPerSide = static_cast<int>(std::lround(density * WALL_LENGTH));

    // The walls in the fixed order of the world, then the hills: the order is part of
    // the result, because every tuft takes the next numbers of the generator.
    for (const WallSegment& wall : world.walls) {
        plantAlongWall(wall, world.terrain, tuftsPerSide, generator, tufts);
    }
    plantOnHills(world, density, generator, tufts);

    // The trampled ground comes last, and it only takes tufts away. Every tuft above was
    // chosen with the next numbers of the generator, so leaving a wall out up there
    // would move all the grass that comes after it. Removing afterwards keeps every
    // other tuft of a seed exactly where it was. erase_if keeps the order of the rest.
    std::erase_if(tufts, [&world](const GrassTuft& tuft) {
        return grassIsTrampled(world, tuft.position.x, tuft.position.z);
    });
    return tufts;
}

bool grassIsTrampled(const MazeWorld& world, float x, float z) {
    // In front of the stile, where every night begins.
    if (world.stileWall && *world.stileWall < world.walls.size() &&
        stileWornGround(world.walls[*world.stileWall], x, z)) {
        return true;
    }
    // A maze of one cell has no gate, so nobody walks up to one.
    if (!world.hasGate) {
        return false;
    }
    const MazeCell cell = cellAt({x, 0.0F, z});
    return cell == world.exitCell || cell == approachCell(world);
}

} // namespace game
