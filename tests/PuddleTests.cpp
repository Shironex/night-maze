// Tests of game::Puddles: how many puddles a maze gets, which cells they lie in and the
// mesh that lays their water on the ground.
#include "game/Puddles.hpp"

#include "assets/ImageLoader.hpp"
#include "game/EnvironmentMapping.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Terrain.hpp"
#include "scene/Collider.hpp"

#include <doctest/doctest.h>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// How close two heights have to be to count as the same, in metres: a hundredth of
// a millimetre, for numbers that differ by rounding only.
constexpr float HEIGHT_TOLERANCE = 0.00001F;

// The assets directory of the repository, see ImageLoaderTests.cpp.
std::filesystem::path assetsDirectory() {
    return NIGHT_MAZE_ASSETS_DIR;
}

// The heightmap of the game, loaded the way the game loads it.
game::Heightmap gameHeightmap() {
    assets::Image image;
    std::string error;
    REQUIRE(assets::loadImage(assetsDirectory() / "textures" / "heightmap.png", image, error,
                              assets::RowOrder::TopFirst));
    return game::heightmapFromImage(image);
}

// A rough heightmap, so that the heights of the water are worth checking: 8 by 8 values from
// 0 to 1 made by a formula, the same in every run (the one of GrassTests.cpp).
game::Heightmap roughHeightmap() {
    constexpr int SIZE = 8;
    constexpr int LEVELS = 7;
    game::Heightmap heightmap;
    heightmap.width = SIZE;
    heightmap.height = SIZE;
    heightmap.values.clear();
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            const int level = (column * 5 + row * 3) % LEVELS;
            heightmap.values.push_back(static_cast<float>(level) / static_cast<float>(LEVELS - 1));
        }
    }
    return heightmap;
}

// The default maze of the game on the given heightmap.
game::MazeWorld defaultWorld(const game::Heightmap& heightmap, float heightScale) {
    return game::buildMazeWorld(game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT,
                                game::DEFAULT_MAZE_SEED, heightmap, heightScale);
}

// The puddles of a world as the seed chose them, placed the way puddlesOnGround does.
std::vector<game::PuddleSpawn> spawnsOf(const game::MazeWorld& world, float share) {
    return game::placePuddles(world.maze, world.seed, game::START_CELL, world.exitCell,
                              world.crystals, share);
}

// Number of cells of a world that may get a puddle: all but the start, the exit and the
// cells of the crystals.
int freeCellCount(const game::MazeWorld& world) {
    constexpr int START_AND_EXIT = 2;
    return world.maze.width() * world.maze.height() - START_AND_EXIT -
           static_cast<int>(world.crystals.size());
}

// How far the point (x, z) is from the footprint of the box, in metres: 0 inside it.
float distanceToFootprint(const scene::Aabb& box, float x, float z) {
    const float nearestX = std::clamp(x, box.min.x, box.max.x);
    const float nearestZ = std::clamp(z, box.min.z, box.max.z);
    return std::hypot(x - nearestX, z - nearestZ);
}

// One puddle of radius radius with its middle at (x, z), lying on the terrain the way
// puddlesOnGround lays it.
game::Puddle puddleAt(const game::Terrain& terrain, float x, float z, float radius) {
    return {.center = {x, terrain.heightAt(x, z) + game::PUDDLE_LIFT, z}, .radius = radius};
}

// The number of the first vertex of ring number ring (1 is the innermost) of a puddle
// whose vertices start at base.
std::size_t firstOfRing(std::size_t base, int ring) {
    return base + 1U + static_cast<std::size_t>((ring - 1) * game::PUDDLE_CORNERS);
}

// How close the ground comes to the film of water BETWEEN the vertices of its mesh, as
// the smallest distance from the ground up to the film, in metres. At the vertices it
// is PUDDLE_LIFT. Inside a triangle the film is flat and the ground may not be, so
// every triangle is looked at on a fine grid of points: the height of the film there
// (blended from its three corners, as the graphics card does) minus the height of the
// ground. A number below 0 would mean that the ground pokes through the water.
float smallestClearance(const game::Terrain& terrain, const game::PuddleMeshData& mesh) {
    // Points per edge of a triangle. The triangles are at most 9 cm long, so the points
    // are about 1 cm apart.
    constexpr int STEPS = 8;
    float smallest = game::PUDDLE_LIFT;
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const glm::vec3 a = mesh.vertices[mesh.indices[i]].position;
        const glm::vec3 b = mesh.vertices[mesh.indices[i + 1]].position;
        const glm::vec3 c = mesh.vertices[mesh.indices[i + 2]].position;
        for (int u = 0; u <= STEPS; ++u) {
            for (int v = 0; u + v <= STEPS; ++v) {
                // Two of the three weights of the corners: the third is what is left.
                const float alongB = static_cast<float>(u) / static_cast<float>(STEPS);
                const float alongC = static_cast<float>(v) / static_cast<float>(STEPS);
                const glm::vec3 film = a + alongB * (b - a) + alongC * (c - a);
                smallest = std::min(smallest, film.y - terrain.heightAt(film.x, film.z));
            }
        }
    }
    return smallest;
}

} // namespace

TEST_CASE("the puddle constants keep a puddle inside its cell, clear of the walls") {
    CHECK(game::PUDDLE_MIN_RADIUS > 0.0F);
    CHECK(game::PUDDLE_MAX_RADIUS > game::PUDDLE_MIN_RADIUS);
    CHECK(game::PUDDLE_LIFT > 0.0F);
    // A ring needs at least three corners to be a polygon, and there has to be a ring.
    CHECK(game::PUDDLE_CORNERS >= 3);
    CHECK(game::PUDDLE_RINGS >= 1);
    // 32 corners and 6 rings: 193 vertices and 352 triangles per puddle.
    CHECK(game::PUDDLE_VERTEX_COUNT == 193);
    CHECK(game::PUDDLE_TRIANGLE_COUNT == 352);
    // The rim fades over a part of the radius, not over more than all of it.
    CHECK(game::PUDDLE_RIM_FADE > 0.0F);
    CHECK(game::PUDDLE_RIM_FADE <= 1.0F);

    // The farthest a puddle reaches from the centre of its cell, and where the foot of
    // a wall starts: half a cell, minus half the collision box, minus what the model is
    // wider than the box.
    const float reach = game::PUDDLE_MAX_OFFSET + game::PUDDLE_MAX_RADIUS;
    const float wallFoot =
        game::CELL_SIZE / 2.0F - game::WALL_COLLISION_THICKNESS / 2.0F - game::FOOTPRINT_MARGIN;
    CHECK(reach == doctest::Approx(0.75F));
    CHECK(wallFoot == doctest::Approx(0.8F));
    CHECK(reach < wallFoot);
}

TEST_CASE("a maze gets a puddle in the given share of its free cells") {
    // 85 free cells times 0.15 is 12.75: 13 puddles.
    CHECK(game::puddleCountFor(85, 0.15F) == 13);
    CHECK(game::puddleCountFor(10, 0.5F) == 5);
    // Rounded to the nearest whole number: 10 * 0.24 = 2.4 and 10 * 0.26 = 2.6.
    CHECK(game::puddleCountFor(10, 0.24F) == 2);
    CHECK(game::puddleCountFor(10, 0.26F) == 3);
    // No share, no puddles. A negative share is no share.
    CHECK(game::puddleCountFor(85, 0.0F) == 0);
    CHECK(game::puddleCountFor(85, -1.0F) == 0);
    // Never more puddles than cells.
    CHECK(game::puddleCountFor(85, 1.0F) == 85);
    CHECK(game::puddleCountFor(85, 3.0F) == 85);
    // No free cell, no puddle.
    CHECK(game::puddleCountFor(0, 0.5F) == 0);
    CHECK(game::puddleCountFor(-4, 0.5F) == 0);
}

TEST_CASE("the default maze has 13 puddles at the default share") {
    const game::MazeWorld world = game::buildMazeWorld(
        game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, game::DEFAULT_MAZE_SEED);
    CHECK(freeCellCount(world) == 85);
    CHECK(spawnsOf(world, game::DEFAULT_PUDDLE_SHARE).size() == 13U);
    CHECK(game::puddlesOnGround(world, game::DEFAULT_PUDDLE_SHARE).size() == 13U);
}

TEST_CASE("golden maze: 4 x 4 cells from seed 1 has exactly these puddles") {
    // The maze of the golden tests of the generator and of the crystals. Its exit is the
    // cell (3, 1) and its crystals float in (1, 1) and (1, 0), so 12 cells are free, and
    // half of them get a puddle. These cells must come out on every system: the choice
    // uses std::mt19937 and randomBelow only.
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1);
    REQUIRE(world.exitCell == game::MazeCell{.x = 3, .z = 1});
    REQUIRE(world.crystals.size() == 2U);

    const std::vector<game::PuddleSpawn> puddles = spawnsOf(world, 0.5F);
    REQUIRE(puddles.size() == 6U);
    CHECK(puddles[0].cell == game::MazeCell{.x = 0, .z = 1});
    CHECK(puddles[1].cell == game::MazeCell{.x = 1, .z = 2});
    CHECK(puddles[2].cell == game::MazeCell{.x = 2, .z = 1});
    CHECK(puddles[3].cell == game::MazeCell{.x = 3, .z = 3});
    CHECK(puddles[4].cell == game::MazeCell{.x = 2, .z = 2});
    CHECK(puddles[5].cell == game::MazeCell{.x = 0, .z = 2});

    // The first puddle in full: its place in the cell and its size.
    CHECK(puddles[0].offset.x == doctest::Approx(-0.24375F));
    CHECK(puddles[0].offset.y == doctest::Approx(0.05625F));
    CHECK(puddles[0].radius == doctest::Approx(0.275F));
}

TEST_CASE("puddles are in different cells, never at the start, at the exit or under a crystal") {
    for (const std::uint32_t seed : {1U, 2U, 3U, 40U, 500U}) {
        const game::MazeWorld world = game::buildMazeWorld(9, 7, seed);
        // Every free cell gets one: the strictest case for the three rules.
        const std::vector<game::PuddleSpawn> puddles = spawnsOf(world, 1.0F);
        REQUIRE(puddles.size() == static_cast<std::size_t>(freeCellCount(world)));

        for (std::size_t i = 0; i < puddles.size(); ++i) {
            const game::MazeCell cell = puddles[i].cell;
            CHECK(world.maze.contains(cell.x, cell.z));
            CHECK_FALSE(cell == game::START_CELL);
            CHECK_FALSE(cell == world.exitCell);
            for (const game::CrystalSpawn& crystal : world.crystals) {
                CHECK_FALSE(cell == crystal.cell);
            }
            for (std::size_t other = i + 1; other < puddles.size(); ++other) {
                CHECK_FALSE(cell == puddles[other].cell);
            }
        }
    }
}

TEST_CASE("the same maze and seed always give the same puddles, another seed gives others") {
    const game::MazeWorld world = game::buildMazeWorld(8, 8, 5);
    const std::vector<game::PuddleSpawn> first = spawnsOf(world, 0.3F);
    const std::vector<game::PuddleSpawn> second = spawnsOf(game::buildMazeWorld(8, 8, 5), 0.3F);
    REQUIRE(first.size() == second.size());
    REQUIRE_FALSE(first.empty());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].cell == second[i].cell);
        CHECK(first[i].offset == second[i].offset);
        CHECK(first[i].radius == second[i].radius);
    }

    // The same maze with the puddles of another seed: other cells. (The seed is passed
    // by itself here, so the maze, the exit and the crystals stay the same.)
    const std::vector<game::PuddleSpawn> other = game::placePuddles(
        world.maze, world.seed + 1U, game::START_CELL, world.exitCell, world.crystals, 0.3F);
    REQUIRE(other.size() == first.size());
    bool anyDifferent = false;
    for (std::size_t i = 0; i < first.size(); ++i) {
        anyDifferent = anyDifferent || !(first[i].cell == other[i].cell);
    }
    CHECK(anyDifferent);
}

TEST_CASE("a larger share keeps every puddle of a smaller share and adds more") {
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 7);
    const std::vector<game::PuddleSpawn> few = spawnsOf(world, 0.1F);
    const std::vector<game::PuddleSpawn> many = spawnsOf(world, 0.4F);
    REQUIRE_FALSE(few.empty());
    REQUIRE(many.size() > few.size());
    for (std::size_t i = 0; i < few.size(); ++i) {
        CHECK(few[i].cell == many[i].cell);
        CHECK(few[i].offset == many[i].offset);
        CHECK(few[i].radius == many[i].radius);
    }
}

TEST_CASE("every puddle has a size and a place in its cell within the limits, not all the same") {
    const game::MazeWorld world = game::buildMazeWorld(12, 12, 3);
    const std::vector<game::PuddleSpawn> puddles = spawnsOf(world, 1.0F);
    REQUIRE(puddles.size() > 100U);

    float smallest = puddles.front().radius;
    float largest = puddles.front().radius;
    for (const game::PuddleSpawn& puddle : puddles) {
        CHECK(puddle.radius >= game::PUDDLE_MIN_RADIUS);
        CHECK(puddle.radius <= game::PUDDLE_MAX_RADIUS);
        CHECK(std::abs(puddle.offset.x) <= game::PUDDLE_MAX_OFFSET);
        CHECK(std::abs(puddle.offset.y) <= game::PUDDLE_MAX_OFFSET);
        smallest = std::min(smallest, puddle.radius);
        largest = std::max(largest, puddle.radius);
    }
    // With more than a hundred puddles most of the range is used.
    CHECK(largest - smallest > 0.15F);
}

TEST_CASE("a share of 0 gives no puddles, and a maze without free cells gets none") {
    const game::MazeWorld world = game::buildMazeWorld(6, 6, 2);
    CHECK(spawnsOf(world, 0.0F).empty());
    CHECK(game::puddlesOnGround(world, 0.0F).empty());

    // One cell: it is the start and the exit at once.
    const game::MazeWorld single = game::buildMazeWorld(1, 1, 1);
    CHECK(spawnsOf(single, 1.0F).empty());
}

TEST_CASE("a start or an exit outside the maze is an error for the puddles too") {
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1);
    const game::MazeCell outside{.x = 4, .z = 0};
    CHECK_THROWS_AS(
        game::placePuddles(world.maze, 1, outside, world.exitCell, world.crystals, 0.5F),
        std::out_of_range);
    CHECK_THROWS_AS(
        game::placePuddles(world.maze, 1, game::START_CELL, outside, world.crystals, 0.5F),
        std::out_of_range);
}

TEST_CASE("the corners of the rim lie evenly on a circle of radius 1, the first on +X") {
    CHECK(game::puddleRimCorner(0).x == doctest::Approx(1.0F));
    CHECK(game::puddleRimCorner(0).y == doctest::Approx(0.0F));
    // A quarter of the way round: on the +Z axis.
    const glm::vec2 quarter = game::puddleRimCorner(game::PUDDLE_CORNERS / 4);
    CHECK(quarter.x == doctest::Approx(0.0F));
    CHECK(quarter.y == doctest::Approx(1.0F));

    for (int corner = 0; corner < game::PUDDLE_CORNERS; ++corner) {
        CHECK(glm::length(game::puddleRimCorner(corner)) == doctest::Approx(1.0F));
        // Every step between two corners is equally long.
        const float step =
            glm::distance(game::puddleRimCorner(corner), game::puddleRimCorner(corner + 1));
        CHECK(step ==
              doctest::Approx(glm::distance(game::puddleRimCorner(0), game::puddleRimCorner(1))));
    }
    // After the last corner the first one comes again.
    const glm::vec2 again = game::puddleRimCorner(game::PUDDLE_CORNERS);
    CHECK(again.x == doctest::Approx(1.0F));
    CHECK(again.y == doctest::Approx(0.0F));
}

TEST_CASE("on flat ground the middle of a puddle lies PUDDLE_LIFT above it") {
    const game::MazeWorld world = game::buildMazeWorld(6, 6, 2);
    const std::vector<game::Puddle> puddles = game::puddlesOnGround(world, 0.5F);
    REQUIRE_FALSE(puddles.empty());
    for (const game::Puddle& puddle : puddles) {
        CHECK(puddle.center.y == doctest::Approx(game::PUDDLE_LIFT));
    }
}

TEST_CASE("on uneven ground the middle of a puddle lies PUDDLE_LIFT above the ground there") {
    const game::MazeWorld world = game::buildMazeWorld(6, 5, 21, roughHeightmap(), 1.0F);
    const std::vector<game::Puddle> puddles = game::puddlesOnGround(world, 1.0F);
    REQUIRE(puddles.size() > 10U);

    bool anyAboveZero = false;
    for (const game::Puddle& puddle : puddles) {
        const float ground = world.terrain.heightAt(puddle.center.x, puddle.center.z);
        CHECK(puddle.center.y == doctest::Approx(ground + game::PUDDLE_LIFT));
        anyAboveZero = anyAboveZero || ground > HEIGHT_TOLERANCE;
    }
    // The ground of this world really is uneven, so the check above says something.
    CHECK(anyAboveZero);
}

TEST_CASE("the puddles of a world lie where the seed put them, in every cell they belong to") {
    const game::MazeWorld world = game::buildMazeWorld(6, 5, 21, roughHeightmap(), 1.0F);
    const std::vector<game::PuddleSpawn> spawns = spawnsOf(world, 0.6F);
    const std::vector<game::Puddle> puddles = game::puddlesOnGround(world, 0.6F);
    REQUIRE(puddles.size() == spawns.size());
    REQUIRE_FALSE(puddles.empty());

    for (std::size_t i = 0; i < puddles.size(); ++i) {
        const glm::vec3 center = game::cellCenter(spawns[i].cell.x, spawns[i].cell.z);
        CHECK(puddles[i].center.x == doctest::Approx(center.x + spawns[i].offset.x));
        CHECK(puddles[i].center.z == doctest::Approx(center.z + spawns[i].offset.y));
        CHECK(puddles[i].radius == spawns[i].radius);
    }
}

TEST_CASE("another height scale moves the puddles up or down and nowhere else") {
    const game::Heightmap heightmap = roughHeightmap();
    game::MazeWorld world = game::buildMazeWorld(6, 5, 21, heightmap, 1.0F);
    const std::vector<game::Puddle> before = game::puddlesOnGround(world, 0.6F);

    // What the game does when the height scale slider moves.
    game::placeOnTerrain(world, heightmap, 2.0F);
    const std::vector<game::Puddle> after = game::puddlesOnGround(world, 0.6F);

    REQUIRE(after.size() == before.size());
    bool anyMoved = false;
    for (std::size_t i = 0; i < before.size(); ++i) {
        CHECK(after[i].center.x == before[i].center.x);
        CHECK(after[i].center.z == before[i].center.z);
        CHECK(after[i].radius == before[i].radius);
        anyMoved = anyMoved || after[i].center.y != before[i].center.y;
    }
    CHECK(anyMoved);
}

TEST_CASE("no puddle reaches a wall, a pillar or the gate") {
    const game::MazeWorld world = game::buildMazeWorld(9, 7, 4, roughHeightmap(), 1.0F);
    REQUIRE(world.hasGate);
    const std::vector<game::Puddle> puddles = game::puddlesOnGround(world, 1.0F);
    REQUIRE(puddles.size() > 40U);

    // The feet of the walls and the pillars are a little wider than their boxes
    // (FOOTPRINT_MARGIN), so the rim has to stay that far away from every box.
    bool clearOfBoxes = true;
    bool clearOfGate = true;
    for (const game::Puddle& puddle : puddles) {
        const float needed = puddle.radius + game::FOOTPRINT_MARGIN;
        for (const scene::Aabb& box : world.colliders) {
            clearOfBoxes = clearOfBoxes &&
                           distanceToFootprint(box, puddle.center.x, puddle.center.z) >= needed;
        }
        clearOfGate = clearOfGate && distanceToFootprint(world.gateBox, puddle.center.x,
                                                         puddle.center.z) >= needed;
    }
    CHECK(clearOfBoxes);
    CHECK(clearOfGate);
}

TEST_CASE("the mesh of a puddle has a middle and rings of corners around it") {
    const game::Terrain flat;
    const game::Puddle puddle = puddleAt(flat, 5.0F, 7.0F, 0.4F);
    const game::PuddleMeshData mesh = game::buildPuddleMesh(flat, {&puddle, 1});
    REQUIRE(mesh.vertices.size() == static_cast<std::size_t>(game::PUDDLE_VERTEX_COUNT));
    REQUIRE(mesh.indices.size() == static_cast<std::size_t>(game::PUDDLE_TRIANGLE_COUNT) * 3U);

    // The middle of the mesh is the middle of the puddle, in world space.
    CHECK(mesh.vertices[0].position.x == doctest::Approx(5.0F));
    CHECK(mesh.vertices[0].position.y == doctest::Approx(game::PUDDLE_LIFT));
    CHECK(mesh.vertices[0].position.z == doctest::Approx(7.0F));
    CHECK(mesh.vertices[0].uv.x == doctest::Approx(0.5F));
    CHECK(mesh.vertices[0].uv.y == doctest::Approx(0.5F));

    for (int ring = 1; ring <= game::PUDDLE_RINGS; ++ring) {
        // Ring number ring lies at that part of the radius: the rings are equally far
        // apart, and the last one is the rim, exactly as far out as the radius.
        const float share = static_cast<float>(ring) / static_cast<float>(game::PUDDLE_RINGS);
        for (int corner = 0; corner < game::PUDDLE_CORNERS; ++corner) {
            const gfx::Vertex& vertex =
                mesh.vertices[firstOfRing(0, ring) + static_cast<std::size_t>(corner)];
            const float fromMiddle = std::hypot(vertex.position.x - 5.0F, vertex.position.z - 7.0F);
            CHECK(fromMiddle == doctest::Approx(share * 0.4F));
            // The texture coordinate tells the same thing, for the fade of the rim:
            // 0 in the middle, half a picture away at the rim.
            CHECK(glm::distance(vertex.uv, glm::vec2{0.5F}) == doctest::Approx(share * 0.5F));
        }
        // Every ring starts on the +X side of the middle.
        const gfx::Vertex& first = mesh.vertices[firstOfRing(0, ring)];
        CHECK(first.position.x == doctest::Approx(5.0F + share * 0.4F));
        CHECK(first.position.z == doctest::Approx(7.0F));
    }

    for (const gfx::Vertex& vertex : mesh.vertices) {
        // Level water: every normal straight up, whatever the ground does.
        CHECK(vertex.normal == glm::vec3{0.0F, 1.0F, 0.0F});
        // The tangent has length 1 and is perpendicular to the normal.
        CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
        CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));
        // The texture coordinate stays inside the picture.
        CHECK(vertex.uv.x >= 0.0F);
        CHECK(vertex.uv.x <= 1.0F);
        CHECK(vertex.uv.y >= 0.0F);
        CHECK(vertex.uv.y <= 1.0F);
    }
}

TEST_CASE("every triangle of a puddle faces up and together they cover the puddle once") {
    const game::MazeWorld world = game::buildMazeWorld(6, 5, 21, roughHeightmap(), 1.0F);
    const game::Puddle puddle = puddleAt(world.terrain, 3.0F, 5.0F, 0.4F);
    const game::PuddleMeshData mesh = game::buildPuddleMesh(world.terrain, {&puddle, 1});
    REQUIRE(mesh.indices.size() % 3U == 0U);

    float levelArea = 0.0F;
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        REQUIRE(mesh.indices[i] < mesh.vertices.size());
        REQUIRE(mesh.indices[i + 1] < mesh.vertices.size());
        REQUIRE(mesh.indices[i + 2] < mesh.vertices.size());
        const glm::vec3 a = mesh.vertices[mesh.indices[i]].position;
        const glm::vec3 b = mesh.vertices[mesh.indices[i + 1]].position;
        const glm::vec3 c = mesh.vertices[mesh.indices[i + 2]].position;
        // The cross product of two edges points along the front face of a triangle whose
        // corners go counter-clockwise. Its y is twice the area of the triangle as seen
        // from straight above, and positive when the front face looks up.
        const glm::vec3 face = glm::cross(b - a, c - a);
        CHECK(face.y > 0.0F);
        levelArea += face.y / 2.0F;
    }
    // Seen from above the triangles cover the whole polygon once, on uneven ground too:
    // 32 corners give 99.4 % of the circle, whose area is pi times the radius squared.
    const float circle = glm::pi<float>() * 0.4F * 0.4F;
    CHECK(levelArea < circle);
    CHECK(levelArea > 0.99F * circle);
}

TEST_CASE("every vertex of a puddle lies exactly PUDDLE_LIFT above the ground under it") {
    // A sloped and bumpy ground: the film has to follow it, not stay level.
    const game::MazeWorld world = game::buildMazeWorld(6, 5, 21, roughHeightmap(), 2.0F);
    const std::vector<game::Puddle> puddles = game::puddlesOnGround(world, 1.0F);
    REQUIRE(puddles.size() > 10U);
    const game::PuddleMeshData mesh = game::buildPuddleMesh(world.terrain, puddles);
    REQUIRE(mesh.vertices.size() ==
            puddles.size() * static_cast<std::size_t>(game::PUDDLE_VERTEX_COUNT));

    bool everyVertexOnGround = true;
    float lowest = mesh.vertices.front().position.y;
    float highest = lowest;
    for (const gfx::Vertex& vertex : mesh.vertices) {
        const float ground = world.terrain.heightAt(vertex.position.x, vertex.position.z);
        everyVertexOnGround =
            everyVertexOnGround &&
            std::abs(vertex.position.y - (ground + game::PUDDLE_LIFT)) <= HEIGHT_TOLERANCE;
        lowest = std::min(lowest, vertex.position.y);
        highest = std::max(highest, vertex.position.y);
    }
    CHECK(everyVertexOnGround);
    // The water is not level: its vertices are at many different heights.
    CHECK(highest - lowest > 0.1F);

    // Inside one puddle too: on this ground the rim of some puddle is tilted by more
    // than the old flat disc could have been (it had one height for all of it).
    float largestTilt = 0.0F;
    for (std::size_t i = 0; i < puddles.size(); ++i) {
        const std::size_t base = i * static_cast<std::size_t>(game::PUDDLE_VERTEX_COUNT);
        float rimLowest = mesh.vertices[firstOfRing(base, game::PUDDLE_RINGS)].position.y;
        float rimHighest = rimLowest;
        for (int corner = 0; corner < game::PUDDLE_CORNERS; ++corner) {
            const float y = mesh.vertices[firstOfRing(base, game::PUDDLE_RINGS) +
                                          static_cast<std::size_t>(corner)]
                                .position.y;
            rimLowest = std::min(rimLowest, y);
            rimHighest = std::max(rimHighest, y);
        }
        largestTilt = std::max(largestTilt, rimHighest - rimLowest);
    }
    CHECK(largestTilt > 0.02F);
}

TEST_CASE("the mesh holds all puddles one after another, and none for no puddles") {
    const game::Terrain flat;
    CHECK(game::buildPuddleMesh(flat, {}).vertices.empty());
    CHECK(game::buildPuddleMesh(flat, {}).indices.empty());

    const std::vector<game::Puddle> puddles = {puddleAt(flat, 1.0F, 1.0F, 0.3F),
                                               puddleAt(flat, 3.0F, 5.0F, 0.45F)};
    const game::PuddleMeshData mesh = game::buildPuddleMesh(flat, puddles);
    const auto perPuddle = static_cast<std::size_t>(game::PUDDLE_VERTEX_COUNT);
    const auto indicesPerPuddle = static_cast<std::size_t>(game::PUDDLE_TRIANGLE_COUNT) * 3U;
    REQUIRE(mesh.vertices.size() == 2U * perPuddle);
    REQUIRE(mesh.indices.size() == 2U * indicesPerPuddle);

    // The triangles of the first puddle use its vertices only, the ones of the second
    // puddle only the vertices that follow.
    for (std::size_t i = 0; i < mesh.indices.size(); ++i) {
        if (i < indicesPerPuddle) {
            CHECK(mesh.indices[i] < perPuddle);
        } else {
            CHECK(mesh.indices[i] >= perPuddle);
            CHECK(mesh.indices[i] < 2U * perPuddle);
        }
    }
    CHECK(mesh.vertices[perPuddle].position.x == doctest::Approx(3.0F));
    CHECK(mesh.vertices[perPuddle].position.z == doctest::Approx(5.0F));
}

TEST_CASE("the ground of the game never pokes through the water of a puddle") {
    // The numbers behind the comment at game::PUDDLE_LIFT, measured on the real
    // heightmap. The film lies PUDDLE_LIFT above the ground at its vertices. Between
    // them the ground may rise towards it, where an edge of the terrain crosses
    // a triangle of the film: smallestClearance finds how close it comes.
    const game::Heightmap heightmap = gameHeightmap();

    // The puddles of the default maze, as the game draws them.
    const auto clearanceOfDefaultPuddles = [&heightmap](float heightScale) {
        const game::MazeWorld world = defaultWorld(heightmap, heightScale);
        const std::vector<game::Puddle> puddles =
            game::puddlesOnGround(world, game::DEFAULT_PUDDLE_SHARE);
        return smallestClearance(world.terrain, game::buildPuddleMesh(world.terrain, puddles));
    };

    // The worst place for a puddle: the largest one in the middle of every cell of the
    // default maze and at the four corners of the square its middle can move in.
    const auto clearanceEverywhere = [&heightmap](float heightScale) {
        const game::MazeWorld world = defaultWorld(heightmap, heightScale);
        std::vector<game::Puddle> puddles;
        for (int z = 0; z < world.maze.height(); ++z) {
            for (int x = 0; x < world.maze.width(); ++x) {
                const glm::vec3 center = game::cellCenter(x, z);
                for (const float offsetX :
                     {-game::PUDDLE_MAX_OFFSET, 0.0F, game::PUDDLE_MAX_OFFSET}) {
                    for (const float offsetZ :
                         {-game::PUDDLE_MAX_OFFSET, 0.0F, game::PUDDLE_MAX_OFFSET}) {
                        puddles.push_back(puddleAt(world.terrain, center.x + offsetX,
                                                   center.z + offsetZ, game::PUDDLE_MAX_RADIUS));
                    }
                }
            }
        }
        return smallestClearance(world.terrain, game::buildPuddleMesh(world.terrain, puddles));
    };

    // Measured (2026-10-06, PUDDLE_LIFT 8 mm, 6 rings of 32 corners): the smallest
    // distance is 7.56 mm and 6.89 mm for the puddles of the default maze (default and
    // largest height scale), and 7.35 mm and 6.37 mm with the largest puddle all over
    // the maze. So the ground comes at most 1.7 mm closer than at the vertices. The
    // checks ask for 6 mm, three quarters of the lift: a coarser mesh or a rougher
    // heightmap that eats more of it than that should be noticed here.
    constexpr float NEEDED_CLEARANCE = 0.75F * game::PUDDLE_LIFT;
    CHECK(clearanceOfDefaultPuddles(game::DEFAULT_HEIGHT_SCALE) > NEEDED_CLEARANCE);
    CHECK(clearanceOfDefaultPuddles(game::MAX_HEIGHT_SCALE) > NEEDED_CLEARANCE);
    CHECK(clearanceEverywhere(game::DEFAULT_HEIGHT_SCALE) > NEEDED_CLEARANCE);
    CHECK(clearanceEverywhere(game::MAX_HEIGHT_SCALE) > NEEDED_CLEARANCE);

    // On level ground nothing comes closer at all.
    const game::Terrain flat;
    const game::Puddle onFlat = puddleAt(flat, 3.0F, 5.0F, game::PUDDLE_MAX_RADIUS);
    CHECK(smallestClearance(flat, game::buildPuddleMesh(flat, {&onFlat, 1})) ==
          doctest::Approx(game::PUDDLE_LIFT));
}
