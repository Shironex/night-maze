// Puddles: which cells of a maze get a puddle, how big it is and the mesh that lays its
// water on the ground.
#include "game/Puddles.hpp"

#include "game/Crystals.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Terrain.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>

namespace game {

namespace {

// The generator of the puddles is seeded with the seed of the maze plus this number, so
// it repeats neither the numbers the maze was carved with nor the ones of the crystals,
// the grass, the levers or the notes. Any number other than those offsets would do.
constexpr std::uint32_t PUDDLE_SEED_OFFSET = 4000037U;

// randomBetween cuts its range into this many equal steps: a radius or an offset is one
// of 33 values, which is finer than the eye can tell.
constexpr std::uint32_t RANDOM_STEPS = 32U;

// The normal of the water: straight up, the direction level water faces.
constexpr glm::vec3 UP{0.0F, 1.0F, 0.0F};

// The tangent of the water: the direction in which its texture coordinate u grows.
constexpr glm::vec3 TANGENT{1.0F, 0.0F, 0.0F};

// The vertex in the middle of a puddle is the first one of its vertices, and the rings
// follow it.
constexpr std::uint32_t CENTER_VERTEX = 0U;
constexpr std::uint32_t FIRST_RING_VERTEX = 1U;

// A triangle has three corners.
constexpr std::size_t INDICES_PER_TRIANGLE = 3U;

// The texture coordinate runs from 0 to 1 across the puddle: the middle of the puddle
// is the middle of the picture, and the rim is half a picture away from it.
constexpr glm::vec2 UV_CENTER{0.5F, 0.5F};
constexpr float UV_RADIUS = 0.5F;

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

std::vector<Puddle> puddlesOnGround(const MazeWorld& world, float share) {
    const std::vector<PuddleSpawn> spawns =
        placePuddles(world.maze, world.seed, START_CELL, world.exitCell, world.seedCrystals, share);

    std::vector<Puddle> puddles;
    puddles.reserve(spawns.size());
    for (const PuddleSpawn& spawn : spawns) {
        // The puddles were chosen around the crystals of the seed. The heartstone, and
        // a crystal that made room for it, came later: a puddle under one of them is
        // left out, so no other puddle of the seed moves and none lies under a pickup.
        if (spawn.cell == world.heartstone || hasCrystal(world.crystals, spawn.cell)) {
            continue;
        }
        // The middle of the puddle: the centre of the cell moved by the offset (its two
        // numbers are along X and along Z), as high as the film of water lies there.
        glm::vec3 center = cellCenter(spawn.cell.x, spawn.cell.z);
        center.x += spawn.offset.x;
        center.z += spawn.offset.y;
        center.y = world.terrain.heightAt(center.x, center.z) + PUDDLE_LIFT;
        puddles.push_back({.center = center, .radius = spawn.radius});
    }
    return puddles;
}

PuddleMeshData buildPuddleMesh(const Terrain& terrain, std::span<const Puddle> puddles) {
    PuddleMeshData mesh;
    mesh.vertices.reserve(puddles.size() * static_cast<std::size_t>(PUDDLE_VERTEX_COUNT));
    mesh.indices.reserve(puddles.size() * static_cast<std::size_t>(PUDDLE_TRIANGLE_COUNT) *
                         INDICES_PER_TRIANGLE);

    // One vertex of the film: at (x, z) of the world, PUDDLE_LIFT above the ground
    // there. along is where it lies in the puddle seen from above, from (-1, -1) to
    // (1, 1) with the middle at (0, 0). v grows towards -Z, like on the terrain, so
    // a picture would be seen from above the right way round. No picture is drawn on
    // a puddle (its texture is plain white): the coordinate is there for the fade of
    // the rim and for the debug view of the texture coordinates.
    const auto addVertex = [&mesh, &terrain](float x, float z, const glm::vec2& along) {
        mesh.vertices.push_back({.position = {x, terrain.heightAt(x, z) + PUDDLE_LIFT, z},
                                 .normal = UP,
                                 .uv = UV_CENTER + UV_RADIUS * glm::vec2{along.x, -along.y},
                                 .tangent = TANGENT});
    };

    for (const Puddle& puddle : puddles) {
        // The indices count from the first vertex of the whole mesh, so every puddle
        // adds the number of vertices that are there already.
        const auto base = static_cast<std::uint32_t>(mesh.vertices.size());

        // The middle, then ring after ring outwards. Ring number ring (1 is the
        // innermost) lies at that part of the radius, so the rings are equally far
        // apart and the last one is the rim.
        addVertex(puddle.center.x, puddle.center.z, glm::vec2{0.0F});
        for (int ring = 1; ring <= PUDDLE_RINGS; ++ring) {
            const float share = static_cast<float>(ring) / static_cast<float>(PUDDLE_RINGS);
            for (int corner = 0; corner < PUDDLE_CORNERS; ++corner) {
                const glm::vec2 along = share * puddleRimCorner(corner);
                addVertex(puddle.center.x + puddle.radius * along.x,
                          puddle.center.z + puddle.radius * along.y, along);
            }
        }

        // The vertex of corner number corner of ring number ring (0 is the innermost
        // here). After the last corner of a ring its first one comes again.
        const auto ringVertex = [base](int ring, int corner) {
            return base + FIRST_RING_VERTEX +
                   static_cast<std::uint32_t>(ring * PUDDLE_CORNERS + corner % PUDDLE_CORNERS);
        };

        for (int corner = 0; corner < PUDDLE_CORNERS; ++corner) {
            // The innermost ring: one triangle per corner, the middle, the NEXT corner,
            // this corner. The corners run from +X towards +Z, which is clockwise when
            // seen from above. Taking the next corner first makes the triangle
            // counter-clockwise from above, so its front face looks up, like the
            // triangles of the terrain.
            mesh.indices.insert(
                mesh.indices.end(),
                {base + CENTER_VERTEX, ringVertex(0, corner + 1), ringVertex(0, corner)});

            // Every other ring: the four-sided piece between this corner and the next
            // one, and between this ring and the one inside it, cut into two triangles
            // that turn the same way as the one above.
            for (int ring = 1; ring < PUDDLE_RINGS; ++ring) {
                const std::uint32_t inner = ringVertex(ring - 1, corner);
                const std::uint32_t innerNext = ringVertex(ring - 1, corner + 1);
                const std::uint32_t outer = ringVertex(ring, corner);
                const std::uint32_t outerNext = ringVertex(ring, corner + 1);
                mesh.indices.insert(mesh.indices.end(),
                                    {inner, outerNext, outer, inner, innerNext, outerNext});
            }
        }
    }
    return mesh;
}

} // namespace game
