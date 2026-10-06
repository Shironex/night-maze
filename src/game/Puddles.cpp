// Puddles: which cells of a maze get a puddle, how big it is and at what height its
// water stands.
// See docs/modules/renderer/env-mapping.md
#include "game/Puddles.hpp"

#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Terrain.hpp"
#include "scene/Transform.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>

namespace game {

namespace {

// The generator of the puddles is seeded with the seed of the maze plus this number, so
// it repeats neither the numbers the maze was carved with nor the ones of the crystals,
// the grass, the levers or the notes. Any number other than those offsets would do.
constexpr std::uint32_t PUDDLE_SEED_OFFSET = 4000037U;

// randomBetween cuts its range into this many equal steps: a radius or an offset is one
// of 33 values, which is finer than the eye can tell.
constexpr std::uint32_t RANDOM_STEPS = 32U;

// The normal of the disc: straight up, the direction level water faces.
constexpr glm::vec3 UP{0.0F, 1.0F, 0.0F};

// The tangent of the disc: the direction in which its texture coordinate u grows.
constexpr glm::vec3 TANGENT{1.0F, 0.0F, 0.0F};

// The vertex in the middle of the disc is the first one of the mesh.
constexpr std::uint32_t CENTER_VERTEX = 0U;

// A triangle has three corners.
constexpr std::size_t INDICES_PER_TRIANGLE = 3U;

// The texture coordinate runs from 0 to 1 across the disc: the middle of the disc is
// the middle of the picture, and the rim is half a picture away from it.
constexpr glm::vec2 UV_CENTER{0.5F, 0.5F};
constexpr float UV_RADIUS = 0.5F;

// Puts the cells into a random order: every order is equally likely. Fisher-Yates
// shuffle, written out by hand for the reason given at the same function in
// Crystals.cpp: std::shuffle may use the generator differently in each standard library.
void shuffleCells(std::vector<MazeCell>& cells, std::mt19937& generator) {
    // Going from the back: the last place gets one of all the cells, the place before
    // it one of the cells that are left, and so on.
    for (std::size_t last = cells.size(); last > 1; --last) {
        const auto chosen =
            static_cast<std::size_t>(randomBelow(generator, static_cast<std::uint32_t>(last)));
        std::swap(cells[last - 1], cells[chosen]);
    }
}

// A random number from low to high, both ends included, in RANDOM_STEPS equal steps.
// A whole number from randomBelow turned into a float: the same value on every system,
// which std::uniform_real_distribution does not promise.
float randomBetween(std::mt19937& generator, float low, float high) {
    const std::uint32_t step = randomBelow(generator, RANDOM_STEPS + 1U);
    return glm::mix(low, high, static_cast<float>(step) / static_cast<float>(RANDOM_STEPS));
}

// True when one of the crystals floats in the cell.
bool hasCrystal(std::span<const CrystalSpawn> crystals, MazeCell cell) {
    return std::ranges::any_of(
        crystals, [cell](const CrystalSpawn& crystal) { return crystal.cell == cell; });
}

} // namespace

int puddleCountFor(int freeCells, float share) {
    if (freeCells <= 0 || share <= 0.0F) {
        return 0;
    }
    const auto rounded = static_cast<int>(std::lround(static_cast<float>(freeCells) * share));
    // A share above 1 must not ask for more puddles than there are cells.
    return std::min(rounded, freeCells);
}

std::vector<PuddleSpawn> placePuddles(const Maze& maze, std::uint32_t seed, MazeCell start,
                                      MazeCell exit, std::span<const CrystalSpawn> crystals,
                                      float share) {
    if (!maze.contains(start.x, start.z) || !maze.contains(exit.x, exit.z)) {
        throw std::out_of_range("placePuddles: the start or the exit is outside the maze");
    }

    // The free cells, row after row: the order before the shuffle is part of the result.
    // No puddle at the start (the player would stand in it from the first moment), none
    // in the exit cell (the gate sinks into the ground there) and none under a crystal
    // (that place already has something to look at).
    std::vector<MazeCell> freeCells;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (cell == start || cell == exit || hasCrystal(crystals, cell)) {
                continue;
            }
            freeCells.push_back(cell);
        }
    }

    // The shuffle uses the generator first, and in the same way for every share. So the
    // order of the cells does not depend on the share, and a larger share only takes
    // more cells from the front of the same list.
    std::mt19937 generator(seed + PUDDLE_SEED_OFFSET);
    shuffleCells(freeCells, generator);

    const auto count =
        static_cast<std::size_t>(puddleCountFor(static_cast<int>(freeCells.size()), share));

    std::vector<PuddleSpawn> puddles;
    puddles.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        // Three random numbers per puddle, always drawn in this order: the offset along
        // X, the offset along Z, the radius.
        const float offsetX = randomBetween(generator, -PUDDLE_MAX_OFFSET, PUDDLE_MAX_OFFSET);
        const float offsetZ = randomBetween(generator, -PUDDLE_MAX_OFFSET, PUDDLE_MAX_OFFSET);
        const float radius = randomBetween(generator, PUDDLE_MIN_RADIUS, PUDDLE_MAX_RADIUS);
        puddles.push_back({.cell = freeCells[i], .offset = {offsetX, offsetZ}, .radius = radius});
    }
    return puddles;
}

glm::vec2 puddleRimCorner(int corner) {
    // The whole circle (two pi radians) cut into PUDDLE_CORNERS equal angles. The cosine
    // and the sine of an angle are the x and the z of the point of the unit circle there.
    const float angle =
        glm::two_pi<float>() * static_cast<float>(corner) / static_cast<float>(PUDDLE_CORNERS);
    return {std::cos(angle), std::sin(angle)};
}

float puddleWaterLevel(const Terrain& terrain, float x, float z, float radius) {
    float lowest = terrain.heightAt(x, z);
    for (int corner = 0; corner < PUDDLE_CORNERS; ++corner) {
        const glm::vec2 rim = radius * puddleRimCorner(corner);
        lowest = std::min(lowest, terrain.heightAt(x + rim.x, z + rim.y));
    }
    return lowest + PUDDLE_DEPTH;
}

std::vector<Puddle> puddlesOnGround(const MazeWorld& world, float share) {
    const std::vector<PuddleSpawn> spawns =
        placePuddles(world.maze, world.seed, START_CELL, world.exitCell, world.crystals, share);

    std::vector<Puddle> puddles;
    puddles.reserve(spawns.size());
    for (const PuddleSpawn& spawn : spawns) {
        // The middle of the disc: the centre of the cell moved by the offset (its two
        // numbers are along X and along Z), at the height of the water.
        glm::vec3 center = cellCenter(spawn.cell.x, spawn.cell.z);
        center.x += spawn.offset.x;
        center.z += spawn.offset.y;
        center.y = puddleWaterLevel(world.terrain, center.x, center.z, spawn.radius);
        puddles.push_back({.center = center, .radius = spawn.radius});
    }
    return puddles;
}

PuddleMeshData buildPuddleMesh() {
    PuddleMeshData mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(PUDDLE_CORNERS) + 1U);

    // The middle, then the corners of the rim. v grows towards -Z, like on the terrain,
    // so a picture would be seen from above the right way round. No picture is drawn on
    // a puddle today (its texture is plain white): the coordinate is there for the
    // debug view of the texture coordinates.
    mesh.vertices.push_back(
        {.position = glm::vec3{0.0F}, .normal = UP, .uv = UV_CENTER, .tangent = TANGENT});
    for (int corner = 0; corner < PUDDLE_CORNERS; ++corner) {
        const glm::vec2 rim = puddleRimCorner(corner);
        mesh.vertices.push_back({.position = {rim.x, 0.0F, rim.y},
                                 .normal = UP,
                                 .uv = UV_CENTER + UV_RADIUS * glm::vec2{rim.x, -rim.y},
                                 .tangent = TANGENT});
    }

    // One triangle per corner: the middle, the NEXT corner, this corner. The corners
    // run from +X towards +Z, which is clockwise when seen from above. Taking the next
    // corner first makes the triangle counter-clockwise from above, so its front face
    // looks up, like the triangles of the terrain.
    mesh.indices.reserve(static_cast<std::size_t>(PUDDLE_CORNERS) * INDICES_PER_TRIANGLE);
    for (int corner = 0; corner < PUDDLE_CORNERS; ++corner) {
        // The rim vertices follow the middle one, so corner c is vertex c + 1. After the
        // last corner the first one comes again.
        const auto thisCorner = static_cast<std::uint32_t>(corner + 1);
        const auto nextCorner = static_cast<std::uint32_t>((corner + 1) % PUDDLE_CORNERS + 1);
        mesh.indices.insert(mesh.indices.end(), {CENTER_VERTEX, nextCorner, thisCorner});
    }
    return mesh;
}

glm::mat4 puddleModelMatrix(const Puddle& puddle) {
    scene::Transform transform;
    transform.position = puddle.center;
    // Wider along X and Z only. The disc is flat (every y is 0), so a factor along
    // Y would change nothing: it stays 1, which also keeps the matrix invertible for
    // scene::normalMatrix.
    transform.scale = {puddle.radius, 1.0F, puddle.radius};
    return transform.matrix();
}

} // namespace game
