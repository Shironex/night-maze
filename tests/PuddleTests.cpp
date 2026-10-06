// Tests of game::Puddles: how many puddles a maze gets, which cells they lie in, at what
// height their water stands and the disc they are drawn with.
// See docs/modules/renderer/env-mapping.md
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

// A rough heightmap, so that the water levels are worth checking: 8 by 8 values from
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

// The lowest and the highest ground under a disc, looked up where puddleWaterLevel looks:
// at its middle and at the corners of its rim.
struct GroundRange {
    float lowest = 0.0F;
    float highest = 0.0F;
};

GroundRange groundUnderDisc(const game::Terrain& terrain, float x, float z, float radius) {
    GroundRange range{.lowest = terrain.heightAt(x, z), .highest = terrain.heightAt(x, z)};
    for (int corner = 0; corner < game::PUDDLE_CORNERS; ++corner) {
        const glm::vec2 rim = radius * game::puddleRimCorner(corner);
        const float height = terrain.heightAt(x + rim.x, z + rim.y);
        range.lowest = std::min(range.lowest, height);
        range.highest = std::max(range.highest, height);
    }
    return range;
}

} // namespace

TEST_CASE("the puddle constants keep a puddle inside its cell, clear of the walls") {
    CHECK(game::PUDDLE_MIN_RADIUS > 0.0F);
    CHECK(game::PUDDLE_MAX_RADIUS > game::PUDDLE_MIN_RADIUS);
    CHECK(game::PUDDLE_DEPTH > 0.0F);
    // A fan needs at least three triangles to be a polygon.
    CHECK(game::PUDDLE_CORNERS >= 3);

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
    // cell (3, 1) and its crystals float in (0, 3) and (1, 1), so 12 cells are free, and
    // half of them get a puddle. These cells must come out on every system: the choice
    // uses std::mt19937 and randomBelow only.
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1);
    REQUIRE(world.exitCell == game::MazeCell{.x = 3, .z = 1});
    REQUIRE(world.crystals.size() == 2U);

    const std::vector<game::PuddleSpawn> puddles = spawnsOf(world, 0.5F);
    REQUIRE(puddles.size() == 6U);
    CHECK(puddles[0].cell == game::MazeCell{.x = 3, .z = 0});
    CHECK(puddles[1].cell == game::MazeCell{.x = 0, .z = 2});
    CHECK(puddles[2].cell == game::MazeCell{.x = 0, .z = 1});
    CHECK(puddles[3].cell == game::MazeCell{.x = 3, .z = 3});
    CHECK(puddles[4].cell == game::MazeCell{.x = 1, .z = 2});
    CHECK(puddles[5].cell == game::MazeCell{.x = 2, .z = 1});

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

TEST_CASE("on flat ground the water stands PUDDLE_DEPTH above it") {
    const game::Terrain flat;
    CHECK(game::puddleWaterLevel(flat, 3.0F, 5.0F, game::PUDDLE_MAX_RADIUS) ==
          doctest::Approx(game::PUDDLE_DEPTH));

    const game::MazeWorld world = game::buildMazeWorld(6, 6, 2);
    for (const game::Puddle& puddle : game::puddlesOnGround(world, 0.5F)) {
        CHECK(puddle.center.y == doctest::Approx(game::PUDDLE_DEPTH));
    }
}

TEST_CASE("on uneven ground the water stands PUDDLE_DEPTH above the lowest ground under it") {
    const game::MazeWorld world = game::buildMazeWorld(6, 5, 21, roughHeightmap(), 1.0F);
    const std::vector<game::Puddle> puddles = game::puddlesOnGround(world, 1.0F);
    REQUIRE(puddles.size() > 10U);

    for (const game::Puddle& puddle : puddles) {
        const GroundRange ground =
            groundUnderDisc(world.terrain, puddle.center.x, puddle.center.z, puddle.radius);
        CHECK(puddle.center.y == doctest::Approx(ground.lowest + game::PUDDLE_DEPTH));
        // Nowhere along its rim does the disc stand more than the depth above the
        // ground: it never hangs in the air on one side.
        for (int corner = 0; corner < game::PUDDLE_CORNERS; ++corner) {
            const glm::vec2 rim = puddle.radius * game::puddleRimCorner(corner);
            const float groundAtRim =
                world.terrain.heightAt(puddle.center.x + rim.x, puddle.center.z + rim.y);
            CHECK(puddle.center.y - groundAtRim <= game::PUDDLE_DEPTH + HEIGHT_TOLERANCE);
        }
    }
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

TEST_CASE("the disc has a middle and a rim of radius 1, flat, with every normal straight up") {
    const game::PuddleMeshData mesh = game::buildPuddleMesh();
    const auto corners = static_cast<std::size_t>(game::PUDDLE_CORNERS);
    REQUIRE(mesh.vertices.size() == corners + 1U);
    REQUIRE(mesh.indices.size() == corners * 3U);

    CHECK(mesh.vertices[0].position == glm::vec3{0.0F});
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
        const gfx::Vertex& vertex = mesh.vertices[i];
        CHECK(vertex.position.y == 0.0F);
        CHECK(vertex.normal == glm::vec3{0.0F, 1.0F, 0.0F});
        // The tangent has length 1 and lies in the disc: perpendicular to the normal.
        CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
        CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));
        // The texture coordinate stays inside the picture.
        CHECK(vertex.uv.x >= 0.0F);
        CHECK(vertex.uv.x <= 1.0F);
        CHECK(vertex.uv.y >= 0.0F);
        CHECK(vertex.uv.y <= 1.0F);
        if (i > 0) {
            CHECK(glm::length(vertex.position) == doctest::Approx(1.0F));
        }
    }
}

TEST_CASE("every triangle of the disc starts in the middle and faces up") {
    const game::PuddleMeshData mesh = game::buildPuddleMesh();
    REQUIRE(mesh.indices.size() % 3U == 0U);

    float area = 0.0F;
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        REQUIRE(mesh.indices[i] == 0U);
        REQUIRE(mesh.indices[i + 1] < mesh.vertices.size());
        REQUIRE(mesh.indices[i + 2] < mesh.vertices.size());
        const glm::vec3 a = mesh.vertices[mesh.indices[i]].position;
        const glm::vec3 b = mesh.vertices[mesh.indices[i + 1]].position;
        const glm::vec3 c = mesh.vertices[mesh.indices[i + 2]].position;
        // The cross product of two edges points along the front face of a triangle whose
        // corners go counter-clockwise, and is twice as long as the triangle is large.
        const glm::vec3 face = glm::cross(b - a, c - a);
        CHECK(face.y > 0.0F);
        area += glm::length(face) / 2.0F;
    }
    // The triangles cover the whole polygon once: 16 corners give 97 % of the circle,
    // whose area is pi.
    CHECK(area < glm::pi<float>());
    CHECK(area > 0.97F * glm::pi<float>());
}

TEST_CASE("the model matrix makes the disc as wide as the puddle and moves it to its place") {
    const game::Puddle puddle{.center = {5.0F, 0.25F, 7.0F}, .radius = 0.4F};
    const glm::mat4 matrix = game::puddleModelMatrix(puddle);

    const glm::vec3 middle = glm::vec3(matrix * glm::vec4{0.0F, 0.0F, 0.0F, 1.0F});
    const glm::vec3 east = glm::vec3(matrix * glm::vec4{1.0F, 0.0F, 0.0F, 1.0F});
    const glm::vec3 south = glm::vec3(matrix * glm::vec4{0.0F, 0.0F, 1.0F, 1.0F});
    CHECK(middle.x == doctest::Approx(5.0F));
    CHECK(middle.y == doctest::Approx(0.25F));
    CHECK(middle.z == doctest::Approx(7.0F));
    CHECK(east.x == doctest::Approx(5.4F));
    CHECK(east.y == doctest::Approx(0.25F));
    CHECK(east.z == doctest::Approx(7.0F));
    CHECK(south.x == doctest::Approx(5.0F));
    CHECK(south.y == doctest::Approx(0.25F));
    CHECK(south.z == doctest::Approx(7.4F));
}

TEST_CASE("the ground of the game under a puddle: why the water stands on the lowest ground") {
    // The numbers behind the comment at game::puddleWaterLevel, measured on the real
    // heightmap: how much the ground differs under the largest puddle, in the middle of
    // every cell of the default maze.
    const game::Heightmap heightmap = gameHeightmap();

    const auto largestDifference = [&heightmap](float heightScale) {
        const game::MazeWorld world = defaultWorld(heightmap, heightScale);
        float largest = 0.0F;
        for (int z = 0; z < world.maze.height(); ++z) {
            for (int x = 0; x < world.maze.width(); ++x) {
                const glm::vec3 center = game::cellCenter(x, z);
                const GroundRange ground =
                    groundUnderDisc(world.terrain, center.x, center.z, game::PUDDLE_MAX_RADIUS);
                largest = std::max(largest, ground.highest - ground.lowest);
            }
        }
        return largest;
    };

    // At the default height scale up to 7 cm, at the largest one up to 17.5 cm: a disc
    // put on the HIGHEST ground would hang that far in the air on its other side.
    const float atDefaultScale = largestDifference(game::DEFAULT_HEIGHT_SCALE);
    CHECK(atDefaultScale > 0.06F);
    CHECK(atDefaultScale < 0.08F);
    const float atLargestScale = largestDifference(game::MAX_HEIGHT_SCALE);
    CHECK(atLargestScale > 0.16F);
    CHECK(atLargestScale < 0.19F);

    // On the lowest ground, the puddles of the default maze stand at most PUDDLE_DEPTH
    // above the ground at every point that was looked up, at both scales.
    for (const float heightScale : {game::DEFAULT_HEIGHT_SCALE, game::MAX_HEIGHT_SCALE}) {
        const game::MazeWorld world = defaultWorld(heightmap, heightScale);
        for (const game::Puddle& puddle :
             game::puddlesOnGround(world, game::DEFAULT_PUDDLE_SHARE)) {
            const GroundRange ground =
                groundUnderDisc(world.terrain, puddle.center.x, puddle.center.z, puddle.radius);
            CHECK(puddle.center.y - ground.lowest == doctest::Approx(game::PUDDLE_DEPTH));
        }
    }
}
