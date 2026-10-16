// Tests of game::placeGrass (how many tufts a world gets and where they stand) and of the
// grouping of the tufts into chunks, the box of a chunk and the distance LOD.
#include "game/Grass.hpp"

#include "core/Files.hpp"
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
#include <filesystem>
#include <set>
#include <string>
#include <tuple>
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

// The same world with no trampled ground in front of its exit and of its stile, and no
// bare ground under a stone sheep: a maze without a gate, without a stile and without
// sheep has none (game::grassIsTrampled). The tests that count the tufts of every wall
// use it, because the bare ground takes some of those tufts away again.
game::MazeWorld withoutTrampledGround(game::MazeWorld world) {
    world.hasGate = false;
    world.stileWall.reset();
    world.sheep.clear();
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

// ---- Chunks and the distance LOD -------------------------------------------------------

namespace {

// The tufts of the world of most tests, at the default density.
std::vector<game::GrassTuft> testTufts() {
    return game::placeGrass(testWorld(), game::DEFAULT_GRASS_DENSITY);
}

// Every number of a tuft, so that two lists can be compared as sets.
using TuftNumbers = std::tuple<float, float, float, float>;
TuftNumbers numbersOf(const game::GrassTuft& tuft) {
    return {tuft.position.x, tuft.position.y, tuft.position.z, tuft.random};
}

// The square of the grid of chunks a coordinate lies in, as groupGrassIntoChunks counts.
long squareOf(float coordinate) {
    return static_cast<long>(std::floor(coordinate / game::GRASS_CHUNK_SIZE));
}

// The number written after "const float <name> = " or "const int <name> = " in
// a shader file, or -1 when the line is not there.
double shaderConstant(const std::string& source, const std::string& name) {
    for (const char* type : {"const float ", "const int "}) {
        const std::string start = type + name + " = ";
        const std::size_t at = source.find(start);
        if (at != std::string::npos) {
            return std::stod(source.substr(at + start.size()));
        }
    }
    return -1.0;
}

} // namespace

TEST_CASE("grouping into chunks keeps every tuft and only changes the order") {
    const std::vector<game::GrassTuft> tufts = testTufts();
    const std::uint64_t before = tuftChecksum(tufts);
    const game::GrassChunks grouped = game::groupGrassIntoChunks(tufts, game::GRASS_CHUNK_SIZE);

    // The list that went in is untouched: placeGrass and its checksums stay as they are.
    CHECK(tuftChecksum(tufts) == before);

    // The same tufts come out, each once.
    REQUIRE(grouped.tufts.size() == tufts.size());
    std::multiset<TuftNumbers> in;
    std::multiset<TuftNumbers> out;
    for (std::size_t i = 0; i < tufts.size(); ++i) {
        in.insert(numbersOf(tufts[i]));
        out.insert(numbersOf(grouped.tufts[i]));
    }
    CHECK(in == out);

    // The same tufts give the same chunks.
    const game::GrassChunks again = game::groupGrassIntoChunks(tufts, game::GRASS_CHUNK_SIZE);
    CHECK(tuftChecksum(again.tufts) == tuftChecksum(grouped.tufts));
    CHECK(again.chunks.size() == grouped.chunks.size());
}

TEST_CASE("the chunks cover the list without gaps, one square each, row by row") {
    const game::GrassChunks grouped =
        game::groupGrassIntoChunks(testTufts(), game::GRASS_CHUNK_SIZE);
    REQUIRE_FALSE(grouped.chunks.empty());

    std::uint32_t next = 0;
    std::set<std::pair<long, long>> squares;
    std::pair<long, long> previous{0, 0};
    for (std::size_t c = 0; c < grouped.chunks.size(); ++c) {
        const game::GrassChunk& chunk = grouped.chunks[c];
        // No empty chunk, and each one starts where the one before ended.
        CHECK(chunk.tuftCount > 0);
        CHECK(chunk.firstTuft == next);
        next += chunk.tuftCount;

        const game::GrassTuft& first = grouped.tufts[chunk.firstTuft];
        const std::pair<long, long> square{squareOf(first.position.z), squareOf(first.position.x)};
        // Every square is one chunk, and the squares come row by row (z, then x).
        CHECK(squares.insert(square).second);
        if (c > 0) {
            CHECK(previous < square);
        }
        previous = square;

        glm::vec3 low = first.position;
        glm::vec3 high = first.position;
        for (std::uint32_t i = chunk.firstTuft; i < chunk.firstTuft + chunk.tuftCount; ++i) {
            const glm::vec3& root = grouped.tufts[i].position;
            // All tufts of a chunk stand in its square.
            CHECK(squareOf(root.z) == square.first);
            CHECK(squareOf(root.x) == square.second);
            low = glm::min(low, root);
            high = glm::max(high, root);
        }
        // The box of the roots is the smallest one around them.
        CHECK(chunk.roots.min == low);
        CHECK(chunk.roots.max == high);
    }
    CHECK(next == grouped.tufts.size());
}

TEST_CASE("inside a chunk the tufts keep the order they were placed in") {
    // Four tufts in two squares, in the order a, c, b, d of the list: a and b share the
    // square at the origin, c and d the one to its right.
    const std::vector<game::GrassTuft> tufts = {
        {.position = {1.0F, 0.0F, 1.0F}, .random = 0.1F},
        {.position = {5.0F, 0.0F, 1.0F}, .random = 0.2F},
        {.position = {2.0F, 0.0F, 2.0F}, .random = 0.3F},
        {.position = {6.0F, 0.0F, 2.0F}, .random = 0.4F},
    };
    const game::GrassChunks grouped = game::groupGrassIntoChunks(tufts, 4.0F);
    REQUIRE(grouped.chunks.size() == 2U);
    REQUIRE(grouped.tufts.size() == 4U);
    CHECK(grouped.tufts[0].random == 0.1F);
    CHECK(grouped.tufts[1].random == 0.3F);
    CHECK(grouped.tufts[2].random == 0.2F);
    CHECK(grouped.tufts[3].random == 0.4F);
    CHECK(grouped.chunks[0].tuftCount == 2U);
    CHECK(grouped.chunks[1].firstTuft == 2U);
}

TEST_CASE("negative coordinates get squares of their own") {
    // -0.5 and 0.5 are in two squares: the grid is cut at 0, not folded around it.
    const std::vector<game::GrassTuft> tufts = {
        {.position = {-0.5F, 0.0F, 1.0F}, .random = 0.0F},
        {.position = {0.5F, 0.0F, 1.0F}, .random = 0.0F},
    };
    CHECK(game::groupGrassIntoChunks(tufts, 4.0F).chunks.size() == 2U);
}

TEST_CASE("no tufts give no chunks, and a size of 0 gives one chunk") {
    CHECK(game::groupGrassIntoChunks({}, game::GRASS_CHUNK_SIZE).chunks.empty());

    const std::vector<game::GrassTuft> tufts = testTufts();
    const game::GrassChunks one = game::groupGrassIntoChunks(tufts, 0.0F);
    REQUIRE(one.chunks.size() == 1U);
    CHECK(one.chunks[0].tuftCount == tufts.size());
    // With one chunk nothing is reordered.
    CHECK(tuftChecksum(one.tufts) == tuftChecksum(tufts));
}

TEST_CASE("the box of a chunk holds the farthest reach of a blade") {
    const scene::Aabb roots{.min = {1.0F, 0.5F, 2.0F}, .max = {3.0F, 0.8F, 4.0F}};
    constexpr float HEIGHT = 0.3F;
    constexpr float WIND = 1.0F;
    const scene::Aabb box = game::grassChunkBox(roots, HEIGHT, WIND);

    // Sideways: the spread of the roots, the lean of the tallest blade, the push of the
    // wind and half the width of a blade.
    const float reach = game::GRASS_ROOT_SPREAD + game::GRASS_LEAN * HEIGHT +
                        game::GRASS_WIND_REACH * WIND + game::GRASS_ROOT_HALF_WIDTH;
    CHECK(box.min.x == doctest::Approx(roots.min.x - reach));
    CHECK(box.max.z == doctest::Approx(roots.max.z + reach));
    // Up by the tallest blade, and nothing below the roots.
    CHECK(box.max.y == doctest::Approx(roots.max.y + HEIGHT));
    CHECK(box.min.y == roots.min.y);

    // More wind reaches farther, in either direction of the slider.
    CHECK(game::grassChunkBox(roots, HEIGHT, 3.0F).max.x > box.max.x);
    CHECK(game::grassChunkBox(roots, HEIGHT, -1.0F).max.x == doctest::Approx(box.max.x));
}

TEST_CASE("the distance LOD keeps three blades near, then two, then one") {
    game::GrassSettings settings;
    settings.lodNear = 10.0F;
    settings.lodFar = 20.0F;
    const float gone = 2.0F * game::GRASS_LOD_FADE;

    CHECK(game::grassBladesAt(0.0F, settings) == 3);
    // The third blade is there until two bands behind lodNear.
    CHECK(game::grassBladesAt(settings.lodNear + gone - 0.01F, settings) == 3);
    CHECK(game::grassBladesAt(settings.lodNear + gone, settings) == 2);
    CHECK(game::grassBladesAt(settings.lodFar + gone - 0.01F, settings) == 2);
    CHECK(game::grassBladesAt(settings.lodFar + gone, settings) == 1);
    // The first blade stays at every distance.
    CHECK(game::grassBladesAt(1000.0F, settings) == 1);

    // Without the LOD every tuft keeps all of its blades.
    settings.distanceLod = false;
    CHECK(game::grassBladesAt(1000.0F, settings) == game::GRASS_BLADES_PER_TUFT);
}

TEST_CASE("the second blade never goes before the third") {
    game::GrassSettings settings;
    settings.lodNear = 20.0F;
    settings.lodFar = 5.0F;
    CHECK(game::grassLodFar(settings) == 20.0F);
    const float gone = 2.0F * game::GRASS_LOD_FADE;
    // Up to two bands behind lodNear both blades are there, then both go at once.
    CHECK(game::grassBladesAt(settings.lodNear + gone - 0.01F, settings) == 3);
    CHECK(game::grassBladesAt(settings.lodNear + gone, settings) == 1);

    // The defaults are in order.
    const game::GrassSettings defaults;
    CHECK(defaults.lodNear < defaults.lodFar);
    CHECK(game::grassLodFar(defaults) == defaults.lodFar);
    CHECK(defaults.path == game::GrassPath::Instanced);
    CHECK(defaults.chunkCulling);
    CHECK(defaults.distanceLod);
}

TEST_CASE("visible chunks next to each other with the same blades are one draw call") {
    const std::vector<game::GrassChunk> chunks = {
        {.firstTuft = 0, .tuftCount = 4},  {.firstTuft = 4, .tuftCount = 2},
        {.firstTuft = 6, .tuftCount = 5},  {.firstTuft = 11, .tuftCount = 1},
        {.firstTuft = 12, .tuftCount = 3}, {.firstTuft = 15, .tuftCount = 2},
    };
    // Drawn with 3, 3, then not drawn, then 2, 2, then 1.
    const std::vector<int> blades = {3, 3, 0, 2, 2, 1};
    const std::vector<game::GrassDrawRange> ranges = game::grassDrawRanges(chunks, blades);
    REQUIRE(ranges.size() == 3U);
    CHECK(ranges[0].firstTuft == 0U);
    CHECK(ranges[0].tuftCount == 6U);
    CHECK(ranges[0].blades == 3);
    CHECK(ranges[1].firstTuft == 11U);
    CHECK(ranges[1].tuftCount == 4U);
    CHECK(ranges[1].blades == 2);
    CHECK(ranges[2].firstTuft == 15U);
    CHECK(ranges[2].tuftCount == 2U);
    CHECK(ranges[2].blades == 1);

    // A gap of a culled chunk splits two ranges with the same blades.
    const std::vector<int> gap = {3, 0, 3, 0, 0, 0};
    CHECK(game::grassDrawRanges(chunks, gap).size() == 2U);
    // Nothing visible, nothing to draw.
    CHECK(game::grassDrawRanges(chunks, std::vector<int>(chunks.size(), 0)).empty());
}

TEST_CASE("the blade numbers of C++ are the ones of the shader") {
    // The box of a chunk is made in C++ from the shape of a blade, which lives in the
    // shader. A blade that grew in the shader alone would reach out of its box and be
    // cut off at the edge of the screen.
    std::string source;
    REQUIRE(core::readTextFile(std::filesystem::path{NIGHT_MAZE_ASSETS_DIR} / "shaders" / "common" /
                                   "grass_blade.glsl",
                               source));
    CHECK(shaderConstant(source, "BLADES_PER_TUFT") == game::GRASS_BLADES_PER_TUFT);
    CHECK(shaderConstant(source, "ROOT_HALF_WIDTH") ==
          doctest::Approx(game::GRASS_ROOT_HALF_WIDTH));
    CHECK(shaderConstant(source, "ROOT_SPREAD") == doctest::Approx(game::GRASS_ROOT_SPREAD));
    CHECK(shaderConstant(source, "LEAN") == doctest::Approx(game::GRASS_LEAN));
    CHECK(shaderConstant(source, "WIND_REACH") == doctest::Approx(game::GRASS_WIND_REACH));
}
