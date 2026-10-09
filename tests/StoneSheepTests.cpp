// Tests of game::StoneSheep: how many sheep a maze gets, which cells they take, where in
// its cell each one stands, which way it looks and what it must never be in the way of.
#include "game/StoneSheep.hpp"

#include "assets/ObjLoader.hpp"
#include "game/Campaign.hpp"
#include "game/Crystals.hpp"
#include "game/Difficulty.hpp"
#include "game/EnvironmentMapping.hpp"
#include "game/Exit.hpp"
#include "game/Flasks.hpp"
#include "game/GateLamp.hpp"
#include "game/Grass.hpp"
#include "game/Interactables.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Puddles.hpp"
#include "game/Round.hpp"
#include "game/Shade.hpp"
#include "game/Terrain.hpp"
#include "scene/Collider.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace {

using game::Compass;
using game::Direction;
using game::MazeCell;

// The five sizes the game builds: the three levels and the two nights between them.
constexpr std::array<int, 5> GAME_SIZES = {10, 13, 16, 19, 22};

// How many crystals the game asks for in a maze of each of those sizes (the five nights
// of the campaign: the first, the third and the fifth are also the three levels). The
// crystals fill the dead ends first, so their number decides how many dead ends are left
// for the sheep: none in the two small mazes, a few in the large ones.
constexpr std::array<int, 5> GAME_CRYSTALS = {13, 19, 26, 33, 40};

// The maze of the game with the given number in GAME_SIZES.
game::MazeWorld gameWorld(std::size_t level, std::uint32_t seed) {
    return game::buildMazeWorld(GAME_SIZES.at(level), GAME_SIZES.at(level), seed, {},
                                GAME_CRYSTALS.at(level));
}

// Small and odd mazes, down to the smallest there is: the rules must hold there too.
struct Size {
    int width;
    int height;
};
constexpr std::array<Size, 8> SMALL_SIZES = {{
    {.width = 1, .height = 1},
    {.width = 2, .height = 1},
    {.width = 2, .height = 2},
    {.width = 3, .height = 2},
    {.width = 2, .height = 5},
    {.width = 4, .height = 4},
    {.width = 7, .height = 3},
    {.width = 6, .height = 6},
}};

// Every size is built with the seeds 0 to SEED_COUNT - 1.
constexpr std::uint32_t SEED_COUNT = 60;

// Room for the rounding of a float.
constexpr float SLACK = 0.0001F;

// Half of the room between the collision boxes of two walls across a cell: 0.85 m.
constexpr float CELL_HALF_ROOM = game::CELL_SIZE / 2.0F - game::WALL_COLLISION_THICKNESS / 2.0F;

// What has to stay free beside a sheep towards every open side of its cell, in metres,
// and half of the width of the shade (the figure is about 0.8 m wide).
constexpr float FREE_WIDTH = 1.2F;
constexpr float SHADE_HALF_WIDTH = 0.4F;

constexpr std::array<Compass, 8> EIGHT_WAYS = {
    Compass::North, Compass::NorthEast, Compass::East, Compass::SouthEast,
    Compass::South, Compass::SouthWest, Compass::West, Compass::NorthWest};

// Calls check(world) for every maze of the game and seed, and for the small sizes when
// asked.
template <typename Check>
void forEveryWorld(Check check, bool smallOnesToo = true) {
    for (std::size_t level = 0; level < GAME_SIZES.size(); ++level) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(GAME_SIZES.at(level));
            CAPTURE(seed);
            check(gameWorld(level, seed));
        }
    }
    if (!smallOnesToo) {
        return;
    }
    for (const Size& size : SMALL_SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            check(game::buildMazeWorld(size.width, size.height, seed));
        }
    }
}

// The box made smaller by SLACK on every side: what it must not share with anything.
scene::Aabb inside(const scene::Aabb& box) {
    return {.min = box.min + glm::vec3{SLACK}, .max = box.max - glm::vec3{SLACK}};
}

bool contains(const std::vector<MazeCell>& cells, MazeCell cell) {
    return std::ranges::find(cells, cell) != cells.end();
}

// The step on the ground that belongs to a compass direction: x east, z south.
glm::vec3 compassStep(Compass way) {
    switch (way) {
    case Compass::North:
        return {0.0F, 0.0F, -1.0F};
    case Compass::NorthEast:
        return {1.0F, 0.0F, -1.0F};
    case Compass::East:
        return {1.0F, 0.0F, 0.0F};
    case Compass::SouthEast:
        return {1.0F, 0.0F, 1.0F};
    case Compass::South:
        return {0.0F, 0.0F, 1.0F};
    case Compass::SouthWest:
        return {-1.0F, 0.0F, 1.0F};
    case Compass::West:
        return {-1.0F, 0.0F, 0.0F};
    case Compass::NorthWest:
        return {-1.0F, 0.0F, -1.0F};
    case Compass::Here:
        break;
    }
    return glm::vec3{0.0F};
}

glm::vec3 sideStep(Direction side) {
    return {static_cast<float>(game::columnStep(side)), 0.0F,
            static_cast<float>(game::rowStep(side))};
}

// A maze of one row: a corridor of `length` cells from west to east.
game::Maze corridor(int length) {
    game::Maze maze(length, 1);
    for (int x = 0; x + 1 < length; ++x) {
        maze.removeWall(x, 0, Direction::East);
    }
    return maze;
}

// A maze of 3 by 3 cells whose middle cell has exactly the given open sides.
game::Maze roomWithOpenSides(std::initializer_list<Direction> openSides) {
    game::Maze maze(3, 3);
    for (const Direction side : openSides) {
        maze.removeWall(1, 1, side);
    }
    return maze;
}
constexpr MazeCell ROOM{.x = 1, .z = 1};

// How far the box is from the straight line that runs from `from` one cell far towards
// `side`, measured on the ground.
float groundDistanceToWay(const scene::Aabb& box, const glm::vec3& from, Direction side) {
    const glm::vec3 to = from + sideStep(side) * (game::CELL_SIZE / 2.0F);
    const float lowX = std::min(from.x, to.x);
    const float highX = std::max(from.x, to.x);
    const float lowZ = std::min(from.z, to.z);
    const float highZ = std::max(from.z, to.z);
    const float gapX = std::max({box.min.x - highX, lowX - box.max.x, 0.0F});
    const float gapZ = std::max({box.min.z - highZ, lowZ - box.max.z, 0.0F});
    return std::sqrt(gapX * gapX + gapZ * gapZ);
}

// The widest free stretch across the way from the middle of a cell to its open side,
// where the box narrows that way the most. The way is the half of the cell on that side,
// between the collision boxes of its walls.
float freeWidthTowards(const scene::Aabb& box, const glm::vec3& centre, Direction side) {
    const bool alongX = game::columnStep(side) != 0;
    // The box in the frame of the cell: `along` the way and `across` it.
    const float lowAlong = (alongX ? box.min.x - centre.x : box.min.z - centre.z);
    const float highAlong = (alongX ? box.max.x - centre.x : box.max.z - centre.z);
    const float lowAcross = (alongX ? box.min.z - centre.z : box.min.x - centre.x);
    const float highAcross = (alongX ? box.max.z - centre.z : box.max.x - centre.x);
    const auto sign = static_cast<float>(alongX ? game::columnStep(side) : game::rowStep(side));
    // The way runs from 0 to half a cell on the side of `side`.
    const float nearEnd = sign > 0.0F ? lowAlong : -highAlong;
    const float farEnd = sign > 0.0F ? highAlong : -lowAlong;
    if (farEnd <= SLACK || nearEnd >= game::CELL_SIZE / 2.0F) {
        return 2.0F * CELL_HALF_ROOM;
    }
    return std::max(lowAcross + CELL_HALF_ROOM, CELL_HALF_ROOM - highAcross);
}

// Checks one sheep in its cell: what stays free towards every open side.
void checkLeavesTheWayFree(const game::Maze& maze, MazeCell cell, const scene::Aabb& box) {
    const glm::vec3 centre = game::cellCenter(cell.x, cell.z);
    // Inside the room between the wall boxes of the cell.
    CHECK(box.min.x >= centre.x - CELL_HALF_ROOM - SLACK);
    CHECK(box.max.x <= centre.x + CELL_HALF_ROOM + SLACK);
    CHECK(box.min.z >= centre.z - CELL_HALF_ROOM - SLACK);
    CHECK(box.max.z <= centre.z + CELL_HALF_ROOM + SLACK);

    // The body of the player fits in the middle of the cell.
    game::Player player;
    player.position = centre;
    CHECK_FALSE(scene::overlaps(player.box(), box));

    for (const Direction side : game::ALL_DIRECTIONS) {
        if (maze.hasWall(cell.x, cell.z, side)) {
            continue;
        }
        CAPTURE(static_cast<int>(side));
        CHECK(freeWidthTowards(box, centre, side) >= FREE_WIDTH - SLACK);
        // The shade walks from the middle of a cell to the middle of the next one.
        CHECK(groundDistanceToWay(box, centre, side) >= SHADE_HALF_WIDTH - SLACK);
    }
}

// True when the player, as wide as the body is, can get from the start of the world to
// the middle of every cell with all its fixed obstacles in the way. A flood fill over
// a grid of 10 cm: a point of the grid is free when the body, standing there, touches no
// box.
bool playerReachesEveryCell(const game::MazeWorld& world) {
    constexpr int PER_METRE = 10;
    constexpr float HALF_BODY = game::Player::BODY_WIDTH / 2.0F;
    const int columns = world.maze.width() * 2 * PER_METRE + 1;
    const int rows = world.maze.height() * 2 * PER_METRE + 1;
    const auto at = [columns](int column, int row) {
        return static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) +
               static_cast<std::size_t>(column);
    };
    std::vector<bool> blocked(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows),
                              false);
    for (const scene::Aabb& box : world.colliders) {
        // Every point of the grid closer to the box than half a body, along X and Z.
        const int firstColumn = static_cast<int>(
            std::ceil((box.min.x - HALF_BODY + SLACK) * static_cast<float>(PER_METRE)));
        const int lastColumn = static_cast<int>(
            std::floor((box.max.x + HALF_BODY - SLACK) * static_cast<float>(PER_METRE)));
        const int firstRow = static_cast<int>(
            std::ceil((box.min.z - HALF_BODY + SLACK) * static_cast<float>(PER_METRE)));
        const int lastRow = static_cast<int>(
            std::floor((box.max.z + HALF_BODY - SLACK) * static_cast<float>(PER_METRE)));
        for (int row = std::max(firstRow, 0); row <= std::min(lastRow, rows - 1); ++row) {
            for (int column = std::max(firstColumn, 0); column <= std::min(lastColumn, columns - 1);
                 ++column) {
                blocked[at(column, row)] = true;
            }
        }
    }

    std::vector<bool> reached(blocked.size(), false);
    std::deque<std::pair<int, int>> open;
    // The middle of the cell (x, z) is the point (2x + 1, 2z + 1) metres.
    open.emplace_back(PER_METRE, PER_METRE);
    reached[at(PER_METRE, PER_METRE)] = true;
    while (!open.empty()) {
        const auto [column, row] = open.front();
        open.pop_front();
        constexpr std::array<std::pair<int, int>, 4> STEPS = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        for (const auto& [columnStep, rowStep] : STEPS) {
            const int nextColumn = column + columnStep;
            const int nextRow = row + rowStep;
            if (nextColumn < 0 || nextRow < 0 || nextColumn >= columns || nextRow >= rows ||
                blocked[at(nextColumn, nextRow)] || reached[at(nextColumn, nextRow)]) {
                continue;
            }
            reached[at(nextColumn, nextRow)] = true;
            open.emplace_back(nextColumn, nextRow);
        }
    }
    for (int z = 0; z < world.maze.height(); ++z) {
        for (int x = 0; x < world.maze.width(); ++x) {
            if (!reached[at((2 * x + 1) * PER_METRE, (2 * z + 1) * PER_METRE)]) {
                return false;
            }
        }
    }
    return true;
}

// A heightmap that is not flat: 8 by 8 values from 0 to 1 made by a formula.
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

} // namespace

TEST_CASE("a maze gets three, four or five stone sheep by its size") {
    CHECK(game::stoneSheepCountFor(10 * 10) == 3);
    CHECK(game::stoneSheepCountFor(13 * 13) == 3);
    CHECK(game::stoneSheepCountFor(16 * 16) == 4);
    CHECK(game::stoneSheepCountFor(19 * 19) == 4);
    CHECK(game::stoneSheepCountFor(22 * 22) == 5);
    CHECK(game::stoneSheepCountFor(1) == 3);
    CHECK(game::stoneSheepCountFor(256 * 256) == 5);
}

TEST_CASE("the model of a sheep is turned to each of the eight directions") {
    CHECK(game::sheepYawDegrees(Compass::East) == 0.0F);
    CHECK(game::sheepYawDegrees(Compass::NorthEast) == 45.0F);
    CHECK(game::sheepYawDegrees(Compass::North) == 90.0F);
    CHECK(game::sheepYawDegrees(Compass::NorthWest) == 135.0F);
    CHECK(game::sheepYawDegrees(Compass::West) == 180.0F);
    CHECK(game::sheepYawDegrees(Compass::SouthWest) == 225.0F);
    CHECK(game::sheepYawDegrees(Compass::South) == 270.0F);
    CHECK(game::sheepYawDegrees(Compass::SouthEast) == 315.0F);
    CHECK(game::sheepYawDegrees(Compass::Here) == 0.0F);

    // The angle is only a number: what counts is where the head of the model ends up.
    // It lies on the +X axis of the model.
    for (const Compass way : EIGHT_WAYS) {
        CAPTURE(static_cast<int>(way));
        game::StoneSheep sheep;
        sheep.facing = way;
        sheep.position = {7.0F, 0.5F, 3.0F};
        const glm::mat4 matrix = game::stoneSheepMatrix(sheep);
        const glm::vec3 head{matrix * glm::vec4{1.0F, 0.0F, 0.0F, 0.0F}};
        const glm::vec3 expected = glm::normalize(compassStep(way));
        CHECK(head.x == doctest::Approx(expected.x).epsilon(0.0001));
        CHECK(head.y == doctest::Approx(0.0F));
        CHECK(head.z == doctest::Approx(expected.z).epsilon(0.0001));
        // And the sheep is where it was put.
        CHECK(glm::vec3{matrix[3]} == sheep.position);
    }
}

TEST_CASE("a sheep looks along the straight line to the gate, snapped like a chalk hint") {
    const MazeCell exit{.x = 10, .z = 10};
    // Straight along a row or a column.
    CHECK(game::compassTowards({.x = 10, .z = 14}, exit) == Compass::North);
    CHECK(game::compassTowards({.x = 3, .z = 10}, exit) == Compass::East);
    CHECK(game::compassTowards({.x = 10, .z = 2}, exit) == Compass::South);
    CHECK(game::compassTowards({.x = 15, .z = 10}, exit) == Compass::West);
    // The four diagonals.
    CHECK(game::compassTowards({.x = 7, .z = 13}, exit) == Compass::NorthEast);
    CHECK(game::compassTowards({.x = 7, .z = 7}, exit) == Compass::SouthEast);
    CHECK(game::compassTowards({.x = 13, .z = 7}, exit) == Compass::SouthWest);
    CHECK(game::compassTowards({.x = 13, .z = 13}, exit) == Compass::NorthWest);
    // The ties: exactly twice as far one way as the other is still a diagonal, one step
    // more is not. The same on every side of the gate.
    CHECK(game::compassTowards({.x = 8, .z = 11}, exit) == Compass::NorthEast);
    CHECK(game::compassTowards({.x = 7, .z = 11}, exit) == Compass::East);
    CHECK(game::compassTowards({.x = 9, .z = 12}, exit) == Compass::NorthEast);
    CHECK(game::compassTowards({.x = 9, .z = 13}, exit) == Compass::North);
    CHECK(game::compassTowards({.x = 12, .z = 9}, exit) == Compass::SouthWest);
    CHECK(game::compassTowards({.x = 13, .z = 9}, exit) == Compass::West);
    CHECK(game::compassTowards({.x = 11, .z = 8}, exit) == Compass::SouthWest);
    CHECK(game::compassTowards({.x = 11, .z = 7}, exit) == Compass::South);
    CHECK(game::compassTowards({.x = 12, .z = 11}, exit) == Compass::NorthWest);
    CHECK(game::compassTowards({.x = 9, .z = 8}, exit) == Compass::SouthEast);

    // Every sheep of a maze looks where an exit hint chalked in its cell would point.
    forEveryWorld([](const game::MazeWorld& world) {
        for (const game::StoneSheep& sheep : world.sheep) {
            CHECK(sheep.facing != Compass::Here);
            CHECK(sheep.facing == game::compassTowards(sheep.cell, world.exitCell));
            const game::Note hint{.mount = {.cell = sheep.cell, .side = Direction::North},
                                  .kind = game::NoteKind::ExitHint};
            CHECK(game::noteLean(hint, world.exitCell, {}) == sheep.facing);
        }
    });
}

TEST_CASE("a corner is a cell with two open sides that meet at a right angle") {
    CHECK(game::isCornerCell(roomWithOpenSides({Direction::North, Direction::East}), ROOM));
    CHECK(game::isCornerCell(roomWithOpenSides({Direction::South, Direction::West}), ROOM));
    CHECK_FALSE(game::isCornerCell(roomWithOpenSides({Direction::North, Direction::South}), ROOM));
    CHECK_FALSE(game::isCornerCell(roomWithOpenSides({Direction::East, Direction::West}), ROOM));
    CHECK_FALSE(game::isCornerCell(roomWithOpenSides({Direction::East}), ROOM));
    CHECK_FALSE(game::isCornerCell(
        roomWithOpenSides({Direction::North, Direction::East, Direction::South}), ROOM));
    CHECK_FALSE(game::isCornerCell(roomWithOpenSides({}), ROOM));
}

TEST_CASE("a sheep that looks along the grid lies along a wall that runs the same way") {
    // A dead end that is open to the south.
    const game::Maze deadEnd = roomWithOpenSides({Direction::South});
    // East and west: along the back wall, in the middle of it.
    for (const Compass way : {Compass::East, Compass::West}) {
        const auto place = game::stoneSheepPlace(deadEnd, ROOM, way, false);
        REQUIRE(place.has_value());
        CHECK(place.value_or(glm::vec2{}).x == doctest::Approx(0.0F));
        CHECK(place.value_or(glm::vec2{}).y == doctest::Approx(-game::STONE_SHEEP_TO_WALL));
        // There is only that one wall: the other choice is the same place.
        CHECK(game::stoneSheepPlace(deadEnd, ROOM, way, true) == place);
    }
    // North and south: along the west or the east wall, towards the closed end.
    for (const Compass way : {Compass::North, Compass::South}) {
        const glm::vec2 first =
            game::stoneSheepPlace(deadEnd, ROOM, way, false).value_or(glm::vec2{});
        const glm::vec2 second =
            game::stoneSheepPlace(deadEnd, ROOM, way, true).value_or(glm::vec2{});
        CHECK(first.x == doctest::Approx(-game::STONE_SHEEP_TO_WALL));
        CHECK(second.x == doctest::Approx(game::STONE_SHEEP_TO_WALL));
        CHECK(first.y == doctest::Approx(-game::STONE_SHEEP_ALONG_WALL));
        CHECK(second.y == doctest::Approx(-game::STONE_SHEEP_ALONG_WALL));
    }
    CHECK(game::STONE_SHEEP_TO_WALL == doctest::Approx(0.65F));

    // A corner that is open to the south and the east: only its two walls can be used.
    const game::Maze corner = roomWithOpenSides({Direction::South, Direction::East});
    const glm::vec2 east =
        game::stoneSheepPlace(corner, ROOM, Compass::East, false).value_or(glm::vec2{});
    CHECK(east.x == doctest::Approx(-game::STONE_SHEEP_ALONG_WALL));
    CHECK(east.y == doctest::Approx(-game::STONE_SHEEP_TO_WALL));
    const glm::vec2 south =
        game::stoneSheepPlace(corner, ROOM, Compass::South, true).value_or(glm::vec2{});
    CHECK(south.x == doctest::Approx(-game::STONE_SHEEP_TO_WALL));
    CHECK(south.y == doctest::Approx(-game::STONE_SHEEP_ALONG_WALL));

    // A straight passage from west to east has no wall for a sheep that looks north.
    const game::Maze passage = roomWithOpenSides({Direction::West, Direction::East});
    CHECK_FALSE(game::stoneSheepPlace(passage, ROOM, Compass::North, false).has_value());
    CHECK(game::stoneSheepPlace(passage, ROOM, Compass::East, false).has_value());
    // Nothing looks "here".
    CHECK_FALSE(game::stoneSheepPlace(deadEnd, ROOM, Compass::Here, false).has_value());
}

TEST_CASE("a sheep that looks along a diagonal stands across a closed corner") {
    constexpr float TO_CORNER = game::STONE_SHEEP_TO_CORNER;
    // A dead end open to the south has two closed corners, and each diagonal fits one.
    const game::Maze deadEnd = roomWithOpenSides({Direction::South});
    for (const Compass way : {Compass::NorthEast, Compass::SouthWest}) {
        const glm::vec2 place =
            game::stoneSheepPlace(deadEnd, ROOM, way, false).value_or(glm::vec2{});
        CHECK(place.x == doctest::Approx(-TO_CORNER));
        CHECK(place.y == doctest::Approx(-TO_CORNER));
    }
    for (const Compass way : {Compass::NorthWest, Compass::SouthEast}) {
        const glm::vec2 place =
            game::stoneSheepPlace(deadEnd, ROOM, way, true).value_or(glm::vec2{});
        CHECK(place.x == doctest::Approx(TO_CORNER));
        CHECK(place.y == doctest::Approx(-TO_CORNER));
    }

    // A corner open to the south and the east has one closed corner, the north-west
    // one. Only two of the four diagonals stand across it.
    const game::Maze corner = roomWithOpenSides({Direction::South, Direction::East});
    CHECK(game::stoneSheepPlace(corner, ROOM, Compass::NorthEast, false).has_value());
    CHECK(game::stoneSheepPlace(corner, ROOM, Compass::SouthWest, false).has_value());
    CHECK_FALSE(game::stoneSheepPlace(corner, ROOM, Compass::NorthWest, false).has_value());
    CHECK_FALSE(game::stoneSheepPlace(corner, ROOM, Compass::SouthEast, false).has_value());

    // The head and the tail of a sheep across a corner point along the two walls: the
    // line from the middle of the cell into the corner is square to the way it looks.
    for (const Compass way : EIGHT_WAYS) {
        const auto place = game::stoneSheepPlace(deadEnd, ROOM, way, false);
        // Every direction has a place in a dead end.
        REQUIRE(place.has_value());
        if (std::abs(place.value_or(glm::vec2{}).x) == doctest::Approx(TO_CORNER)) {
            const glm::vec3 looks = compassStep(way);
            const glm::vec2 into = place.value_or(glm::vec2{});
            CHECK(looks.x * into.x + looks.z * into.y == doctest::Approx(0.0F));
        }
    }
}

TEST_CASE("in every kind of cell a sheep leaves the way and the middle of the cell free") {
    // Every dead end and every corner, with a sheep that looks each of the eight ways,
    // in both of its places.
    const std::array<std::vector<Direction>, 8> openSides = {{
        {Direction::North},
        {Direction::East},
        {Direction::South},
        {Direction::West},
        {Direction::North, Direction::East},
        {Direction::East, Direction::South},
        {Direction::South, Direction::West},
        {Direction::West, Direction::North},
    }};
    int placed = 0;
    for (const std::vector<Direction>& open : openSides) {
        game::Maze maze(3, 3);
        for (const Direction side : open) {
            maze.removeWall(ROOM.x, ROOM.z, side);
        }
        for (const Compass way : EIGHT_WAYS) {
            for (const bool otherPlace : {false, true}) {
                CAPTURE(open.size());
                CAPTURE(static_cast<int>(open.front()));
                CAPTURE(static_cast<int>(way));
                const auto place = game::stoneSheepPlace(maze, ROOM, way, otherPlace);
                // A dead end has a place for every direction.
                CHECK((place.has_value() || open.size() == 2));
                if (!place) {
                    continue;
                }
                ++placed;
                const glm::vec3 position =
                    game::cellCenter(ROOM.x, ROOM.z) +
                    glm::vec3{place.value_or(glm::vec2{}).x, 0.0F, place.value_or(glm::vec2{}).y};
                checkLeavesTheWayFree(maze, ROOM, game::stoneSheepBox(position, way));
            }
        }
    }
    // 4 dead ends with 8 directions and 4 corners with 6, each asked twice.
    CHECK(placed == (4 * 8 + 4 * 6) * 2);
}

TEST_CASE("the sheep of a maze stand in free dead ends and corners, one per cell") {
    forEveryWorld([](const game::MazeWorld& world) {
        const game::Maze& maze = world.maze;
        const int wanted = game::stoneSheepCountFor(maze.width() * maze.height());
        CHECK(static_cast<int>(world.sheep.size()) <= wanted);
        CHECK(world.sheepMatrices.size() == world.sheep.size());

        const std::vector<MazeCell> flaskDeadEnds =
            game::flaskDeadEnds(maze, world.seed, game::START_CELL, world.exitCell);
        const std::vector<MazeCell> flasks =
            game::placeFlasks(maze, world.seed, game::START_CELL, world.exitCell, world.crystals,
                              game::FLASK_RESERVED_DEAD_ENDS);
        const std::vector<game::PuddleSpawn> puddles =
            game::placePuddles(maze, world.seed, game::START_CELL, world.exitCell,
                               world.seedCrystals, game::DEFAULT_PUDDLE_SHARE);

        for (std::size_t i = 0; i < world.sheep.size(); ++i) {
            const MazeCell cell = world.sheep[i].cell;
            CAPTURE(cell.x);
            CAPTURE(cell.z);
            REQUIRE(maze.contains(cell.x, cell.z));
            CHECK((game::isDeadEnd(maze, cell.x, cell.z) || game::isCornerCell(maze, cell)));
            CHECK(game::cellAt(world.sheep[i].position) == cell);
            for (std::size_t other = 0; other < i; ++other) {
                CHECK(world.sheep[other].cell != cell);
            }

            // Not where a round begins or ends.
            CHECK(cell != game::START_CELL);
            CHECK(cell != world.exitCell);
            if (world.hasGate) {
                CHECK(cell != game::approachCell(world));
            }
            // Not where anything else is.
            for (const game::CrystalSpawn& crystal : world.crystals) {
                CHECK(crystal.cell != cell);
            }
            for (const game::Lever& lever : world.interactables.levers) {
                CHECK(lever.mount.cell != cell);
                for (const game::WallRef& ring : game::slabRingMounts(lever)) {
                    CHECK(ring.cell != cell);
                }
            }
            for (const game::Note& note : world.interactables.notes) {
                CHECK(note.mount.cell != cell);
            }
            for (const game::PuddleSpawn& puddle : puddles) {
                CHECK(puddle.cell != cell);
            }
            // The flasks had the first choice of the dead ends, and the dead end of the
            // heartstone is left alone.
            for (std::size_t reserved = 0;
                 reserved < flaskDeadEnds.size() &&
                 reserved < static_cast<std::size_t>(game::FLASK_RESERVED_DEAD_ENDS);
                 ++reserved) {
                CHECK(flaskDeadEnds[reserved] != cell);
            }
            CHECK_FALSE(contains(flasks, cell));
            CHECK(world.heartstone != cell);
        }
    });
}

// One game the player can start: a level of free play or a night of the campaign.
struct GameConfig {
    int width;
    int height;
    int crystalCount;
    int flaskCount;
    game::InteractableSettings interactables;
};

// The three levels, then the five nights, with the notes and levers each one asks for.
std::vector<GameConfig> everyGameConfig() {
    std::vector<GameConfig> configs;
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        configs.push_back({.width = level.mazeWidth,
                           .height = level.mazeHeight,
                           .crystalCount = level.crystalCount,
                           .flaskCount = level.flaskCount,
                           .interactables = {}});
    }
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        const game::CampaignNight& level = game::campaignNight(night);
        configs.push_back({.width = level.mazeWidth,
                           .height = level.mazeHeight,
                           .crystalCount = level.crystalCount,
                           .flaskCount = level.flaskCount,
                           .interactables = game::campaignInteractables(night, {})});
    }
    return configs;
}

// The cells of the flasks of a round with this many flasks: asked of the round itself.
std::vector<MazeCell> roundFlaskCells(const game::MazeWorld& world, int flaskCount) {
    game::GameplaySettings settings;
    settings.flaskCount = flaskCount;
    std::vector<MazeCell> cells;
    for (const game::RoundFlask& flask : game::startRound(world, settings).flasks) {
        cells.push_back(flask.cell);
    }
    return cells;
}

TEST_CASE("no sheep shares a cell with anything a game puts into its maze") {
    // Every level and every night, 300 seeds each. What is in a cell is asked of the
    // functions the game itself calls (startRound, puddlesOnGround) and not worked out
    // again the way placeStoneSheep does it.
    constexpr std::uint32_t GAME_SEED_COUNT = 300;
    std::vector<int> sizesSeen;
    int flocks = 0;
    for (const GameConfig& config : everyGameConfig()) {
        if (std::ranges::find(sizesSeen, config.width) == sizesSeen.end()) {
            sizesSeen.push_back(config.width);
        }
        for (std::uint32_t seed = 0; seed < GAME_SEED_COUNT; ++seed) {
            CAPTURE(config.width);
            CAPTURE(config.crystalCount);
            CAPTURE(seed);
            const game::MazeWorld world = game::buildMazeWorld(
                config.width, config.height, seed, config.interactables, config.crystalCount);
            flocks += world.sheep.empty() ? 0 : 1;

            std::vector<MazeCell> held = {game::START_CELL, world.exitCell};
            if (world.hasGate) {
                held.push_back(game::approachCell(world));
            }
            REQUIRE(world.heartstone.has_value());
            held.push_back(world.heartstone.value_or(MazeCell{}));
            for (const game::CrystalSpawn& crystal : world.crystals) {
                held.push_back(crystal.cell);
            }
            for (const game::Lever& lever : world.interactables.levers) {
                held.push_back(lever.mount.cell);
                for (const game::WallRef& ring : game::slabRingMounts(lever)) {
                    held.push_back(ring.cell);
                }
            }
            for (const game::Note& note : world.interactables.notes) {
                held.push_back(note.mount.cell);
            }
            const std::vector<game::Puddle> puddles =
                game::puddlesOnGround(world, game::DEFAULT_PUDDLE_SHARE);
            for (const game::Puddle& puddle : puddles) {
                held.push_back(game::cellAt(puddle.center));
            }
            // The flasks of this game, and the flasks of a round with as many as a
            // round can have: the sheep are the ones that yield, to every one of them.
            const std::vector<MazeCell> flasks = roundFlaskCells(world, config.flaskCount);
            CHECK(static_cast<int>(flasks.size()) == config.flaskCount);
            const std::vector<MazeCell> mostFlasks = roundFlaskCells(world, game::MAX_FLASK_COUNT);
            held.insert(held.end(), mostFlasks.begin(), mostFlasks.end());
            for (const MazeCell flask : flasks) {
                CHECK(contains(mostFlasks, flask));
            }

            for (const game::StoneSheep& sheep : world.sheep) {
                CAPTURE(sheep.cell.x);
                CAPTURE(sheep.cell.z);
                CHECK_FALSE(contains(held, sheep.cell));
            }

            // The sheep move nothing: the same world without them gives the same flasks
            // and the same puddles. (Its crystals, heartstone, levers and notes are the
            // very same objects: placeStoneSheep only reads them.)
            game::MazeWorld bare = world;
            bare.sheep.clear();
            CHECK(roundFlaskCells(bare, game::MAX_FLASK_COUNT) == mostFlasks);
            const std::vector<game::Puddle> barePuddles =
                game::puddlesOnGround(bare, game::DEFAULT_PUDDLE_SHARE);
            REQUIRE(barePuddles.size() == puddles.size());
            for (std::size_t i = 0; i < puddles.size(); ++i) {
                CHECK(barePuddles[i].center == puddles[i].center);
            }
        }
    }
    // All five sizes were in the sweep, and nearly every maze had sheep to check.
    CHECK(sizesSeen.size() == GAME_SIZES.size());
    CHECK(flocks > 0);
}

TEST_CASE("every maze of the game has its whole flock, and the same seed the same flock") {
    int inDeadEnds = 0;
    int inCorners = 0;
    int onDiagonals = 0;
    for (std::size_t level = 0; level < GAME_SIZES.size(); ++level) {
        const int size = GAME_SIZES.at(level);
        const auto wanted = static_cast<std::size_t>(game::stoneSheepCountFor(size * size));
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size);
            CAPTURE(seed);
            // With the crystals of the game, and with the number the size alone gives.
            const game::MazeWorld world = gameWorld(level, seed);
            CHECK(world.sheep.size() == wanted);
            CHECK(game::buildMazeWorld(size, size, seed).sheep.size() == wanted);

            const game::MazeWorld again = gameWorld(level, seed);
            CHECK(again.sheep == world.sheep);
            CHECK(game::placeStoneSheep(world) == world.sheep);

            for (const game::StoneSheep& sheep : world.sheep) {
                const bool deadEnd = game::isDeadEnd(world.maze, sheep.cell.x, sheep.cell.z);
                inDeadEnds += deadEnd ? 1 : 0;
                inCorners += deadEnd ? 0 : 1;
                onDiagonals += static_cast<int>(compassStep(sheep.facing).x != 0.0F &&
                                                compassStep(sheep.facing).z != 0.0F);
            }
        }
    }
    // Both kinds of cell and both kinds of direction are in the sweep, so the tests
    // that go through every world have seen each of them.
    CHECK(inDeadEnds > 0);
    CHECK(inCorners > inDeadEnds);
    CHECK(onDiagonals * 5 > inDeadEnds + inCorners);
    // A maze of one cell has no room for a sheep.
    CHECK(game::buildMazeWorld(1, 1, 0U).sheep.empty());
    CHECK(game::buildMazeWorld(2, 1, 0U).sheep.empty());
}

TEST_CASE("the sheep change nothing else of a maze") {
    // Everything else of a world is what its own function gives for the maze and the
    // seed: the sheep come last and draw from a generator of their own.
    for (const int size : GAME_SIZES) {
        for (std::uint32_t seed = 0; seed < 10; ++seed) {
            CAPTURE(size);
            CAPTURE(seed);
            const game::MazeWorld world = game::buildMazeWorld(size, size, seed);
            const game::Maze& maze = world.maze;
            CHECK(world.exitCell == game::farthestCell(maze, game::START_CELL));

            std::vector<MazeCell> flaskCells =
                game::flaskDeadEnds(maze, seed, game::START_CELL, world.exitCell);
            flaskCells.resize(std::min(flaskCells.size(),
                                       static_cast<std::size_t>(game::FLASK_RESERVED_DEAD_ENDS)));
            // The crystals of the seed: the heartstone moves at most two of them
            // afterwards (HeartstoneTests), the sheep none.
            const std::vector<game::CrystalSpawn> crystals =
                game::placeCrystals(maze, seed, game::START_CELL, world.exitCell,
                                    game::CRYSTAL_COUNT_FROM_SIZE, flaskCells);
            REQUIRE(crystals.size() == world.seedCrystals.size());
            for (std::size_t i = 0; i < crystals.size(); ++i) {
                CHECK(crystals[i].cell == world.seedCrystals[i].cell);
            }
            const game::Interactables interactables = game::placeInteractables(
                maze, seed, game::START_CELL, world.exitCell, crystals, {});
            REQUIRE(interactables.levers.size() == world.interactables.levers.size());
            for (std::size_t i = 0; i < interactables.levers.size(); ++i) {
                CHECK(interactables.levers[i].mount == world.interactables.levers[i].mount);
                CHECK(interactables.levers[i].opens == world.interactables.levers[i].opens);
            }
            REQUIRE(interactables.notes.size() == world.interactables.notes.size());
            for (std::size_t i = 0; i < interactables.notes.size(); ++i) {
                CHECK(interactables.notes[i].mount == world.interactables.notes[i].mount);
            }
            // The walls and the pillars are the ones of the plan, sheep or no sheep.
            CHECK(world.walls.size() == game::wallSegments(maze).size());
            CHECK(world.pillars.size() == game::pillarPositions(maze).size());
        }
    }
}

TEST_CASE("cells for sheep are taken spread out, and close ones only when nothing else is left") {
    const game::Maze maze = corridor(12);
    const auto cells = [](std::initializer_list<int> columns) {
        std::vector<MazeCell> list;
        for (const int column : columns) {
            list.push_back({.x = column, .z = 0});
        }
        return list;
    };

    // The second and the third candidate are too close to the first one.
    CHECK(game::chooseSpreadCells(maze, cells({0, 2, 4, 5, 11}), 3) == cells({0, 5, 11}));
    // Exactly four passages apart is still too close, five is not.
    CHECK(game::chooseSpreadCells(maze, cells({3, 7, 8}), 2) == cells({3, 8}));
    // Nothing far enough is left: the cells that were passed over fill up, in order.
    CHECK(game::chooseSpreadCells(maze, cells({0, 2, 4, 5}), 3) == cells({0, 5, 2}));
    // Fewer candidates than wanted, none wanted, and no candidates.
    CHECK(game::chooseSpreadCells(maze, cells({6, 7}), 5) == cells({6, 7}));
    CHECK(game::chooseSpreadCells(maze, cells({0, 5, 11}), 0).empty());
    CHECK(game::chooseSpreadCells(maze, {}, 3).empty());

    // In the mazes of the game the flock is spread out nearly always.
    int worlds = 0;
    int spread = 0;
    forEveryWorld(
        [&](const game::MazeWorld& world) {
            ++worlds;
            bool apart = true;
            for (std::size_t i = 0; i < world.sheep.size(); ++i) {
                const std::vector<int> ways =
                    game::passageDistances(world.maze, world.sheep[i].cell);
                for (std::size_t other = 0; other < i; ++other) {
                    const MazeCell cell = world.sheep[other].cell;
                    const int index = cell.z * world.maze.width() + cell.x;
                    apart = apart && ways[static_cast<std::size_t>(index)] >
                                         game::STONE_SHEEP_MIN_PASSAGES_APART;
                }
            }
            spread += apart ? 1 : 0;
        },
        false);
    CHECK(spread * 10 >= worlds * 9);
}

TEST_CASE("the box of a sheep is long along the way it looks, or a square on a diagonal") {
    const glm::vec3 position{5.0F, 2.0F, 9.0F};
    const scene::Aabb east = game::stoneSheepBox(position, Compass::East);
    CHECK(east.max.x - east.min.x == doctest::Approx(game::STONE_SHEEP_LENGTH));
    CHECK(east.max.z - east.min.z == doctest::Approx(game::STONE_SHEEP_WIDTH));
    CHECK(east.min.y == doctest::Approx(2.0F - game::STONE_SHEEP_BOX_DEPTH));
    CHECK(east.max.y == doctest::Approx(2.0F + game::STONE_SHEEP_BOX_HEIGHT));
    const scene::Aabb north = game::stoneSheepBox(position, Compass::North);
    CHECK(north.max.x - north.min.x == doctest::Approx(game::STONE_SHEEP_WIDTH));
    CHECK(north.max.z - north.min.z == doctest::Approx(game::STONE_SHEEP_LENGTH));
    const scene::Aabb diagonal = game::stoneSheepBox(position, Compass::SouthWest);
    CHECK(diagonal.max.x - diagonal.min.x ==
          doctest::Approx(2.0F * game::STONE_SHEEP_DIAGONAL_HALF));
    CHECK(diagonal.max.z - diagonal.min.z ==
          doctest::Approx(2.0F * game::STONE_SHEEP_DIAGONAL_HALF));
    // The plate of the sketchbook: a box of 0.9 by 0.4 m.
    CHECK(game::STONE_SHEEP_LENGTH == 0.9F);
    CHECK(game::STONE_SHEEP_WIDTH == 0.4F);
}

TEST_CASE("the boxes of the sheep are obstacles of the world, before the box of the stile") {
    forEveryWorld([](const game::MazeWorld& world) {
        const std::size_t first = world.walls.size() + world.pillars.size();
        REQUIRE(world.colliders.size() == first + world.sheep.size() + 1U);
        for (std::size_t i = 0; i < world.sheep.size(); ++i) {
            const scene::Aabb& box = world.colliders[first + i];
            CHECK(box.min == world.sheep[i].box.min);
            CHECK(box.max == world.sheep[i].box.max);
            CHECK(world.sheepMatrices[i] == game::stoneSheepMatrix(world.sheep[i]));

            // It shares no room with any other obstacle, the gate or the exit.
            const scene::Aabb own = inside(box);
            for (std::size_t other = 0; other < world.colliders.size(); ++other) {
                if (other != first + i) {
                    CHECK_FALSE(scene::overlaps(own, world.colliders[other]));
                }
            }
            if (world.hasGate) {
                CHECK_FALSE(scene::overlaps(own, world.gateBox));
                CHECK_FALSE(scene::overlaps(own, world.exitZone));
            }
            // Nor with anything that is picked: no lever, no note.
            for (const game::Lever& lever : world.interactables.levers) {
                CHECK_FALSE(scene::overlaps(own, lever.box));
            }
            for (const game::Note& note : world.interactables.notes) {
                CHECK_FALSE(scene::overlaps(own, note.box));
            }
            // Lower than the hand that holds the lamp, so the beam goes over it.
            CHECK(box.max.y - world.sheep[i].position.y < game::LEVER_MOUNT_HEIGHT);
        }
    });
}

TEST_CASE("no sheep blocks a passage: the way past it stays 1.2 m wide") {
    forEveryWorld([](const game::MazeWorld& world) {
        for (const game::StoneSheep& sheep : world.sheep) {
            CAPTURE(sheep.cell.x);
            CAPTURE(sheep.cell.z);
            checkLeavesTheWayFree(world.maze, sheep.cell, sheep.box);

            // No lever ever opens a wall of its cell, so its closed sides stay closed.
            for (const game::Lever& lever : world.interactables.levers) {
                for (const Direction side : game::ALL_DIRECTIONS) {
                    CHECK_FALSE(game::sameWall(lever.opens, {.cell = sheep.cell, .side = side}));
                }
            }

            // The player walks straight from the middle of its cell through every open
            // side to the middle of the next cell, and back, with every obstacle in play.
            for (const Direction side : game::ALL_DIRECTIONS) {
                if (world.maze.hasWall(sheep.cell.x, sheep.cell.z, side)) {
                    continue;
                }
                const glm::vec3 step = sideStep(side) * game::CELL_SIZE;
                game::Player player;
                player.position = game::cellCenter(sheep.cell.x, sheep.cell.z);
                const glm::vec3 out = scene::moveAndSlide(player.box(), step, world.colliders);
                CHECK(out.x == doctest::Approx(step.x));
                CHECK(out.z == doctest::Approx(step.z));
                player.position += step;
                const glm::vec3 back = scene::moveAndSlide(player.box(), -step, world.colliders);
                CHECK(back.x == doctest::Approx(-step.x));
                CHECK(back.z == doctest::Approx(-step.z));
            }
        }
    });
}

TEST_CASE("the player still reaches the middle of every cell of a maze with sheep") {
    constexpr std::uint32_t SEEDS = 30;
    for (std::size_t level = 0; level < GAME_SIZES.size(); ++level) {
        for (std::uint32_t seed = 0; seed < SEEDS; ++seed) {
            CAPTURE(GAME_SIZES.at(level));
            CAPTURE(seed);
            const game::MazeWorld world = gameWorld(level, seed);
            REQUIRE_FALSE(world.sheep.empty());
            CHECK(playerReachesEveryCell(world));
        }
    }
    // The fill does find a blocked passage: a sheep box across a corridor closes it.
    game::MazeWorld world = game::buildMazeWorld(10, 10, 1U);
    REQUIRE(playerReachesEveryCell(world));
    const glm::vec3 middle =
        game::cellCenter(world.sheep.front().cell.x, world.sheep.front().cell.z);
    world.colliders.push_back(scene::Aabb::fromCenter(middle, glm::vec3{1.0F}));
    CHECK_FALSE(playerReachesEveryCell(world));
}

TEST_CASE("the shade walks through the cell of a sheep and is never held up by it") {
    constexpr float STEP = 1.0F / 120.0F;
    // Four metres at the slowest of its speeds, and a little more.
    constexpr int STEP_LIMIT = 6 * 120;
    const game::Terrain flat;
    int walks = 0;
    for (std::uint32_t seed = 0; seed < 20; ++seed) {
        // The largest maze: its flock has sheep in dead ends too.
        const game::MazeWorld world = gameWorld(GAME_SIZES.size() - 1, seed);
        for (const game::StoneSheep& sheep : world.sheep) {
            std::vector<MazeCell> neighbours;
            for (const Direction side : game::ALL_DIRECTIONS) {
                if (!world.maze.hasWall(sheep.cell.x, sheep.cell.z, side)) {
                    neighbours.push_back({.x = sheep.cell.x + game::columnStep(side),
                                          .z = sheep.cell.z + game::rowStep(side)});
                }
            }
            REQUIRE_FALSE(neighbours.empty());
            // The shade comes in through one open side. The player stands in the cell
            // behind the other one, or in the dead end itself, beside the sheep, and
            // sprints on the spot.
            const MazeCell from = neighbours.front();
            const MazeCell playerCell = neighbours.size() > 1 ? neighbours.back() : sheep.cell;
            const glm::vec3 playerFeet = game::cellCenter(playerCell.x, playerCell.z);
            CAPTURE(seed);
            CAPTURE(sheep.cell.x);
            CAPTURE(sheep.cell.z);

            game::ShadeSettings settings;
            settings.graceSeconds = 0.0F;
            game::Shade shade;
            shade.present = true;
            shade.position = game::cellCenter(from.x, from.z);
            shade.previousPosition = shade.position;
            shade.target = from;
            const game::ShadeStep step{.playerFeet = playerFeet,
                                       .obstacles = world.colliders,
                                       .noise = game::Noise::Sprint};
            bool caught = false;
            bool crossed = false;
            for (int steps = 0; steps < STEP_LIMIT && !caught; ++steps) {
                caught = game::advanceShade(shade, settings, world.maze, flat, step, STEP).caught;
                crossed = crossed || game::cellAt(shade.position) == sheep.cell;
            }
            CHECK(caught);
            CHECK((crossed || playerCell == sheep.cell));
            ++walks;
        }
    }
    CHECK(walks == 20 * 5);
}

TEST_CASE("the sheep stand on the terrain, and their boxes and matrices follow") {
    game::MazeWorld world = game::buildMazeWorld(16, 16, 3U, roughHeightmap(), 1.0F);
    const game::MazeWorld flat = game::buildMazeWorld(16, 16, 3U);
    REQUIRE(world.sheep.size() == flat.sheep.size());
    REQUIRE_FALSE(world.sheep.empty());
    bool raised = false;
    for (const float scale : {1.0F, 3.0F}) {
        // Another height scale: every list is built again, with each sheep in it once.
        game::placeOnTerrain(world, roughHeightmap(), scale);
        REQUIRE(world.colliders.size() ==
                world.walls.size() + world.pillars.size() + world.sheep.size() + 1U);
        REQUIRE(world.sheepMatrices.size() == world.sheep.size());
        for (std::size_t i = 0; i < world.sheep.size(); ++i) {
            const game::StoneSheep& sheep = world.sheep[i];
            // Nothing moves sideways.
            CHECK(sheep.cell == flat.sheep[i].cell);
            CHECK(sheep.facing == flat.sheep[i].facing);
            CHECK(sheep.position.x == flat.sheep[i].position.x);
            CHECK(sheep.position.z == flat.sheep[i].position.z);
            CHECK(sheep.position.y == world.terrain.heightAt(sheep.position.x, sheep.position.z));
            CHECK(sheep.box.min.y ==
                  doctest::Approx(sheep.position.y - game::STONE_SHEEP_BOX_DEPTH));
            CHECK(glm::vec3{world.sheepMatrices[i][3]} == sheep.position);
            raised = raised || sheep.position.y > 0.01F;
        }
    }
    CHECK(raised);
}

TEST_CASE("no grass grows under a sheep, and every other tuft stays where it was") {
    for (const int size : {10, 16}) {
        for (std::uint32_t seed = 1; seed <= 4; ++seed) {
            CAPTURE(size);
            CAPTURE(seed);
            game::MazeWorld world = game::buildMazeWorld(size, size, seed, roughHeightmap(), 1.0F);
            REQUIRE_FALSE(world.sheep.empty());
            const std::vector<game::GrassTuft> with =
                game::placeGrass(world, game::MAX_GRASS_DENSITY);
            for (const game::GrassTuft& tuft : with) {
                REQUIRE_FALSE(game::stoneSheepBareGround(world, tuft.position.x, tuft.position.z));
            }

            // The same world without its flock has tufts there, and the rest of its
            // grass is the same list in the same order.
            game::MazeWorld bare = world;
            bare.sheep.clear();
            std::vector<game::GrassTuft> without = game::placeGrass(bare, game::MAX_GRASS_DENSITY);
            const std::size_t before = without.size();
            std::erase_if(without, [&world](const game::GrassTuft& tuft) {
                return game::stoneSheepBareGround(world, tuft.position.x, tuft.position.z);
            });
            CHECK(without.size() < before);
            REQUIRE(without.size() == with.size());
            for (std::size_t i = 0; i < with.size(); ++i) {
                REQUIRE(with[i].position == without[i].position);
            }
        }
    }

    // The bare ground is the box and a margin around it.
    game::MazeWorld world = game::buildMazeWorld(10, 10, 1U);
    const scene::Aabb box = world.sheep.front().box;
    const float margin = game::STONE_SHEEP_BARE_MARGIN;
    CHECK(game::stoneSheepBareGround(world, box.min.x - margin + SLACK, box.min.z));
    CHECK(game::stoneSheepBareGround(world, box.max.x, box.max.z + margin - SLACK));
    CHECK_FALSE(game::stoneSheepBareGround(world, box.min.x - margin - 0.01F, box.min.z));
    CHECK_FALSE(game::stoneSheepBareGround(world, box.max.x, box.max.z + margin + 0.01F));
}

TEST_CASE("the model of the sheep fits its box, turned each of the eight ways") {
    assets::ObjModel model;
    std::string error;
    const std::filesystem::path file =
        std::filesystem::path{NIGHT_MAZE_ASSETS_DIR} / "models" / "stone_sheep.obj";
    REQUIRE_MESSAGE(assets::loadObj(file, model, error), error);
    REQUIRE_FALSE(model.vertices.empty());
    CHECK(model.parts.size() == 1);
    CHECK(model.mirroredTriangleCount == 0);
    REQUIRE(model.materials.size() == 1);
    CHECK(model.materials.front().diffuseTexture.filename() == "stone_sheep.png");
    CHECK(model.materials.front().normalTexture.filename() == "stone_sheep_normal.png");
    // It is drawn three times per frame, like a wall, and there are at most five.
    CHECK(model.indices.size() / 3 <= 220);

    // As long as the box, from the nose to the end of the tail.
    float lowestX = 0.0F;
    float highestX = 0.0F;
    float lowest = 0.0F;
    float highest = 0.0F;
    for (const gfx::Vertex& vertex : model.vertices) {
        lowestX = std::min(lowestX, vertex.position.x);
        highestX = std::max(highestX, vertex.position.x);
        lowest = std::min(lowest, vertex.position.y);
        highest = std::max(highest, vertex.position.y);
    }
    CHECK(lowestX == doctest::Approx(-game::STONE_SHEEP_LENGTH / 2.0F).epsilon(0.001));
    CHECK(highestX == doctest::Approx(game::STONE_SHEEP_LENGTH / 2.0F).epsilon(0.001));
    // The legs go into the ground, for a sheep on a slope, and the back is knee high.
    CHECK(lowest < -0.08F);
    CHECK(lowest >= -game::STONE_SHEEP_BOX_DEPTH);
    CHECK(highest > 0.5F);
    CHECK(highest <= game::STONE_SHEEP_BOX_HEIGHT);

    for (const Compass way : EIGHT_WAYS) {
        CAPTURE(static_cast<int>(way));
        game::StoneSheep sheep;
        sheep.facing = way;
        const glm::mat4 matrix = game::stoneSheepMatrix(sheep);
        const scene::Aabb box = game::stoneSheepBox(sheep.position, way);
        const bool diagonal = std::abs(compassStep(way).x) + std::abs(compassStep(way).z) > 1.5F;
        // Along the grid the whole model is inside the box. On a diagonal the nose, the
        // tail and the ears reach past it, towards the two walls of the corner.
        const float room = (diagonal ? game::STONE_SHEEP_DIAGONAL_OVERHANG : 0.0F) + SLACK;
        float farthestOut = 0.0F;
        for (const gfx::Vertex& vertex : model.vertices) {
            const glm::vec3 point{matrix * glm::vec4{vertex.position, 1.0F}};
            CHECK(point.x >= box.min.x - room);
            CHECK(point.x <= box.max.x + room);
            CHECK(point.z >= box.min.z - room);
            CHECK(point.z <= box.max.z + room);
            farthestOut = std::max({farthestOut, std::abs(point.x), std::abs(point.z)});
        }
        if (diagonal) {
            // Standing across a corner, nothing of it reaches the face of a wall, which
            // is 0.9 m from the middle of the cell.
            CHECK(game::STONE_SHEEP_TO_CORNER + farthestOut <
                  game::CELL_SIZE / 2.0F - game::WALL_VISUAL_THICKNESS / 2.0F);
        }
    }
}
