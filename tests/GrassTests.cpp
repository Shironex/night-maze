// Tests of game::placeGrass: how many tufts a world gets and where they stand.
#include "game/Grass.hpp"

#include "game/GateLamp.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Terrain.hpp"
#include "scene/Collider.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

// A rough heightmap, so that the heights of the tufts are worth checking: 8 by 8 values
// from 0 to 1 made by a formula, the same in every run.
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

// The world of most tests: 6 by 5 cells on uneven ground.
game::MazeWorld testWorld() {
    constexpr int WIDTH = 6;
    constexpr int HEIGHT = 5;
    constexpr std::uint32_t SEED = 21;
    return game::buildMazeWorld(WIDTH, HEIGHT, SEED, roughHeightmap(), 1.0F);
}

// The same world with no trampled ground in front of its exit and of its stile: a maze
// without a gate and without a stile has none (game::grassIsTrampled). The tests that
// count the tufts of every wall use it, because the trampled ground takes some of those
// tufts away again.
game::MazeWorld withoutTrampledGround(game::MazeWorld world) {
    world.hasGate = false;
    world.stileWall.reset();
    return world;
}

// How many tufts the walls of a world get at a density: on both sides of every wall.
std::size_t wallTuftCount(const game::MazeWorld& world, float density) {
    const auto perSide = static_cast<std::size_t>(std::lround(density * game::WALL_LENGTH));
    return world.walls.size() * 2U * perSide;
}

// A checksum of a list of tufts: every number of every tuft, in order, as the 32 bits it
// is stored in (FNV-1a, a simple hash). Two lists have the same checksum only when they
// hold the same tufts in the same order, down to the last bit.
std::uint64_t tuftChecksum(const std::vector<game::GrassTuft>& tufts) {
    constexpr std::uint64_t FNV_START = 14695981039346656037ULL;
    constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
    std::uint64_t checksum = FNV_START;
    for (const game::GrassTuft& tuft : tufts) {
        for (const float value : {tuft.position.x, tuft.position.y, tuft.position.z, tuft.random}) {
            std::uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            checksum = (checksum ^ bits) * FNV_PRIME;
        }
    }
    return checksum;
}

// True when the point (x, z) lies inside the footprint of the box, made wider by margin
// on every side.
bool insideFootprint(const scene::Aabb& box, float x, float z, float margin) {
    return x > box.min.x - margin && x < box.max.x + margin && z > box.min.z - margin &&
           z < box.max.z + margin;
}

} // namespace

TEST_CASE("the grass constants and the default settings") {
    CHECK(game::DEFAULT_GRASS_DENSITY == 2.5F);
    CHECK(game::MAX_GRASS_DENSITY >= game::DEFAULT_GRASS_DENSITY);

    const game::GrassSettings settings;
    CHECK(settings.enabled);
    CHECK(settings.density == game::DEFAULT_GRASS_DENSITY);
    CHECK(settings.bladeHeight > 0.0F);
    CHECK(settings.windStrength == 1.0F);
    CHECK_FALSE(settings.replant);

    // The strip beside a wall ends before the pillar at the corner: the foot of
    // a pillar reaches 0.2 m along the wall.
    CHECK(game::GRASS_END_CLEARANCE > 0.2F);
}

TEST_CASE("a density of 0 or less plants nothing") {
    const game::MazeWorld world = testWorld();
    CHECK(game::placeGrass(world, 0.0F).empty());
    CHECK(game::placeGrass(world, -3.0F).empty());
}

TEST_CASE("the same world and density give the same tufts") {
    const game::MazeWorld world = testWorld();
    const std::vector<game::GrassTuft> first = game::placeGrass(world, 2.5F);
    const std::vector<game::GrassTuft> second = game::placeGrass(testWorld(), 2.5F);

    REQUIRE_FALSE(first.empty());
    REQUIRE(first.size() == second.size());
    bool same = true;
    for (std::size_t i = 0; i < first.size(); ++i) {
        same =
            same && first[i].position == second[i].position && first[i].random == second[i].random;
    }
    CHECK(same);
}

TEST_CASE("another seed grows the grass in other places") {
    const game::Heightmap heightmap = roughHeightmap();
    const game::MazeWorld first = game::buildMazeWorld(6, 5, 21U, heightmap, 1.0F);
    const game::MazeWorld second = game::buildMazeWorld(6, 5, 22U, heightmap, 1.0F);

    const std::vector<game::GrassTuft> firstTufts = game::placeGrass(first, 2.5F);
    const std::vector<game::GrassTuft> secondTufts = game::placeGrass(second, 2.5F);
    REQUIRE_FALSE(firstTufts.empty());
    REQUIRE_FALSE(secondTufts.empty());
    CHECK(firstTufts[0].position != secondTufts[0].position);
}

TEST_CASE("the tufts of the walls come first: density per metre, on both sides") {
    const game::MazeWorld world = withoutTrampledGround(testWorld());
    constexpr float DENSITY = 2.5F;
    const std::vector<game::GrassTuft> tufts = game::placeGrass(world, DENSITY);

    // 2.5 tufts per metre on a wall of 2 m: 5 on each side, 10 per wall.
    constexpr std::size_t PER_SIDE = 5;
    constexpr std::size_t PER_WALL = 2 * PER_SIDE;
    const std::size_t onWalls = world.walls.size() * PER_WALL;
    CHECK(wallTuftCount(world, DENSITY) == onWalls);
    REQUIRE(tufts.size() >= onWalls);

    // The strip a tuft may stand in, measured from the middle of its wall.
    constexpr float REACH = game::WALL_LENGTH / 2.0F - game::GRASS_END_CLEARANCE;
    constexpr float NEAR = game::WALL_COLLISION_THICKNESS / 2.0F + game::GRASS_WALL_GAP;
    constexpr float FAR = NEAR + game::GRASS_STRIP_WIDTH;
    // Room for the rounding of a float.
    constexpr float SLACK = 0.0001F;

    bool inTheStrip = true;
    bool onTheRightSide = true;
    for (std::size_t wallIndex = 0; wallIndex < world.walls.size(); ++wallIndex) {
        const game::WallSegment& wall = world.walls[wallIndex];
        for (std::size_t i = 0; i < PER_WALL; ++i) {
            const glm::vec3 offset = tufts[wallIndex * PER_WALL + i].position - wall.position;
            const bool alongX = wall.axis == game::WallAxis::AlongX;
            const float along = alongX ? offset.x : offset.z;
            const float across = alongX ? offset.z : offset.x;

            inTheStrip = inTheStrip && std::abs(along) <= REACH + SLACK &&
                         std::abs(across) >= NEAR - SLACK && std::abs(across) <= FAR + SLACK;
            // The first half of the tufts of a wall stands on one side, the second half
            // on the other.
            onTheRightSide = onTheRightSide && (across < 0.0F) == (i < PER_SIDE);
        }
    }
    CHECK(inTheStrip);
    CHECK(onTheRightSide);
}

TEST_CASE("twice the density plants twice the tufts along the walls") {
    const game::MazeWorld world = withoutTrampledGround(testWorld());
    CHECK(wallTuftCount(world, 2.0F) == world.walls.size() * 8U);
    CHECK(wallTuftCount(world, 4.0F) == world.walls.size() * 16U);

    const std::size_t sparse = game::placeGrass(world, 2.0F).size();
    const std::size_t dense = game::placeGrass(world, 4.0F).size();
    CHECK(sparse >= wallTuftCount(world, 2.0F));
    CHECK(dense >= wallTuftCount(world, 4.0F));
    // The hills follow the density too, so the whole count roughly doubles.
    CHECK(dense > sparse + sparse / 2U);
    CHECK(dense < sparse * 3U);
}

TEST_CASE("no tuft stands inside a wall, a pillar or the gate") {
    const game::MazeWorld world = testWorld();
    REQUIRE(world.hasGate);
    const std::vector<game::GrassTuft> tufts = game::placeGrass(world, game::MAX_GRASS_DENSITY);
    REQUIRE(tufts.size() > 1000U);

    // The feet of the walls and the pillars are a little wider than their boxes
    // (FOOTPRINT_MARGIN), so the boxes are tested with that margin.
    bool clearOfBoxes = true;
    bool clearOfGate = true;
    for (const game::GrassTuft& tuft : tufts) {
        for (const scene::Aabb& box : world.colliders) {
            clearOfBoxes = clearOfBoxes && !insideFootprint(box, tuft.position.x, tuft.position.z,
                                                            game::FOOTPRINT_MARGIN);
        }
        clearOfGate =
            clearOfGate && !insideFootprint(world.gateBox, tuft.position.x, tuft.position.z, 0.0F);
    }
    CHECK(clearOfBoxes);
    CHECK(clearOfGate);
}

TEST_CASE("every tuft stands on the ground and has a random number from 0 to 1") {
    const game::MazeWorld world = testWorld();
    REQUIRE(world.terrain.maxHeight() > 0.5F);
    const std::vector<game::GrassTuft> tufts = game::placeGrass(world, 3.0F);

    bool onTheGround = true;
    bool onTheLand = true;
    bool randomInRange = true;
    float smallest = 1.0F;
    float largest = 0.0F;
    for (const game::GrassTuft& tuft : tufts) {
        const glm::vec3& position = tuft.position;
        onTheGround = onTheGround && position.y == world.terrain.heightAt(position.x, position.z);
        onTheLand = onTheLand && position.x >= world.terrain.minX() &&
                    position.x <= world.terrain.maxX() && position.z >= world.terrain.minZ() &&
                    position.z <= world.terrain.maxZ();
        randomInRange = randomInRange && tuft.random >= 0.0F && tuft.random < 1.0F;
        smallest = std::min(smallest, tuft.random);
        largest = std::max(largest, tuft.random);
    }
    CHECK(onTheGround);
    CHECK(onTheLand);
    CHECK(randomInRange);
    // The numbers really are spread over the range.
    CHECK(smallest < 0.05F);
    CHECK(largest > 0.95F);
}

TEST_CASE("the hills get a sparse scatter that keeps away from the maze") {
    const game::MazeWorld world = testWorld();
    constexpr float DENSITY = game::DEFAULT_GRASS_DENSITY;
    const std::vector<game::GrassTuft> tufts = game::placeGrass(world, DENSITY);
    const std::size_t onWalls = wallTuftCount(world, DENSITY);
    REQUIRE(tufts.size() > onWalls);

    // Everything after the tufts of the walls is the scatter.
    bool awayFromTheMaze = true;
    for (std::size_t i = onWalls; i < tufts.size(); ++i) {
        const float distance = game::distanceOutsideMaze(tufts[i].position.x, tufts[i].position.z,
                                                         world.maze.width(), world.maze.height());
        awayFromTheMaze = awayFromTheMaze && distance >= game::GRASS_HILL_CLEARANCE;
    }
    CHECK(awayFromTheMaze);

    // About GRASS_HILL_TUFTS_PER_SQUARE_METRE on the land that is not maze. The places
    // are random, so the count is only near the expected one: within a fifth of it.
    const game::Terrain& terrain = world.terrain;
    const float landArea = (terrain.maxX() - terrain.minX()) * (terrain.maxZ() - terrain.minZ());
    const float mazeArea = static_cast<float>(world.maze.width() * world.maze.height()) *
                           game::CELL_SIZE * game::CELL_SIZE;
    const float expected = (landArea - mazeArea) * game::GRASS_HILL_TUFTS_PER_SQUARE_METRE;
    const auto onHills = static_cast<float>(tufts.size() - onWalls);
    CHECK(onHills > expected * 0.8F);
    CHECK(onHills < expected * 1.2F);
}

TEST_CASE("no grass grows in the exit cell and in the cell in front of its gate") {
    const game::MazeWorld world = testWorld();
    REQUIRE(world.hasGate);
    const game::MazeCell approach = game::approachCell(world);
    const std::vector<game::GrassTuft> tufts = game::placeGrass(world, game::MAX_GRASS_DENSITY);
    REQUIRE(tufts.size() > 1000U);

    bool bare = true;
    for (const game::GrassTuft& tuft : tufts) {
        const game::MazeCell cell = game::cellAt(tuft.position);
        bare = bare && cell != world.exitCell && cell != approach;
    }
    CHECK(bare);

    // The rule by itself: the middle of the two cells is trampled, the next cell is not.
    const glm::vec3 exitMiddle = game::cellCenter(world.exitCell.x, world.exitCell.z);
    const glm::vec3 approachMiddle = game::cellCenter(approach.x, approach.z);
    const glm::vec3 beyond = approachMiddle + (approachMiddle - exitMiddle);
    CHECK(game::grassIsTrampled(world, exitMiddle.x, exitMiddle.z));
    CHECK(game::grassIsTrampled(world, approachMiddle.x, approachMiddle.z));
    CHECK_FALSE(game::grassIsTrampled(world, beyond.x, beyond.z));
}

TEST_CASE("a maze without a gate has no trampled ground") {
    game::MazeWorld world = testWorld();
    world.hasGate = false;
    const glm::vec3 exitMiddle = game::cellCenter(world.exitCell.x, world.exitCell.z);
    CHECK_FALSE(game::grassIsTrampled(world, exitMiddle.x, exitMiddle.z));
}

TEST_CASE("the trampled ground takes tufts away and moves no other tuft of a seed") {
    // The numbers on the right were printed by the game BEFORE the trampled ground
    // existed: how many tufts each of these mazes had at the default density, and
    // a checksum of all their places and random numbers, in order.
    struct Before {
        std::uint32_t seed;
        int size;
        std::size_t count;
        std::uint64_t checksum;
    };
    constexpr std::array<Before, 12> BEFORE = {{
        {.seed = 1U, .size = 6, .count = 988U, .checksum = 12846973796199066704ULL},
        {.seed = 1U, .size = 10, .count = 1843U, .checksum = 1274205875857258424ULL},
        {.seed = 1U, .size = 16, .count = 3751U, .checksum = 582719257039279581ULL},
        {.seed = 2U, .size = 6, .count = 988U, .checksum = 3280749841988392614ULL},
        {.seed = 2U, .size = 10, .count = 1855U, .checksum = 7800287520340244509ULL},
        {.seed = 2U, .size = 16, .count = 3754U, .checksum = 2751831958979162322ULL},
        {.seed = 3U, .size = 6, .count = 987U, .checksum = 3599750614337085532ULL},
        {.seed = 3U, .size = 10, .count = 1855U, .checksum = 13498469288475344034ULL},
        {.seed = 3U, .size = 16, .count = 3750U, .checksum = 6539662837113689136ULL},
        {.seed = 21U, .size = 6, .count = 989U, .checksum = 4831811329703863142ULL},
        {.seed = 21U, .size = 10, .count = 1847U, .checksum = 965332957266023376ULL},
        {.seed = 21U, .size = 16, .count = 3766U, .checksum = 1175280531423276338ULL},
    }};

    for (const Before& before : BEFORE) {
        CAPTURE(before.seed);
        CAPTURE(before.size);
        const game::MazeWorld world =
            game::buildMazeWorld(before.size, before.size, before.seed, roughHeightmap(), 1.0F);
        REQUIRE(world.hasGate);

        // The same world without a gate and without a stile has no trampled ground, so
        // its grass is the grass as it was: the same count and the same checksum as
        // before the rule.
        const game::MazeWorld ungated = withoutTrampledGround(world);
        const std::vector<game::GrassTuft> all =
            game::placeGrass(ungated, game::DEFAULT_GRASS_DENSITY);
        CHECK(all.size() == before.count);
        CHECK(tuftChecksum(all) == before.checksum);

        // With the gate: exactly those tufts, in the same order, without the ones on
        // the trampled ground. Nothing else moved and nothing was added.
        std::vector<game::GrassTuft> expected;
        for (const game::GrassTuft& tuft : all) {
            if (!game::grassIsTrampled(world, tuft.position.x, tuft.position.z)) {
                expected.push_back(tuft);
            }
        }
        const std::vector<game::GrassTuft> tufts =
            game::placeGrass(world, game::DEFAULT_GRASS_DENSITY);
        CHECK(tufts.size() < all.size());
        REQUIRE(tufts.size() == expected.size());
        bool same = true;
        for (std::size_t i = 0; i < tufts.size(); ++i) {
            same = same && tufts[i].position == expected[i].position &&
                   tufts[i].random == expected[i].random;
        }
        CHECK(same);
    }
}
