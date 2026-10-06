// Tests of game::Interactables: which walls the levers open, where levers and notes hang,
// the state of the levers, picking them with a ray and the text of the notes.
// See docs/modules/game/interactables.md
#include "game/Interactables.hpp"

#include "game/Crystals.hpp"
#include "game/Exit.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Terrain.hpp"
#include "scene/Raycast.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// The start cell of every maze of the game (see MazeWorld.cpp).
constexpr game::MazeCell START{.x = 0, .z = 0};

// A maze with everything the placement needs, made the way buildMazeWorld makes it.
struct TestMaze {
    game::Maze maze;
    game::MazeCell exit;
    std::vector<game::CrystalSpawn> crystals;
};

TestMaze testMaze(int width, int height, std::uint32_t seed) {
    game::Maze maze = game::generateMaze(width, height, seed);
    const game::MazeCell exit = game::farthestCell(maze, START);
    std::vector<game::CrystalSpawn> crystals = game::placeCrystals(maze, seed, START, exit);
    return {.maze = std::move(maze), .exit = exit, .crystals = std::move(crystals)};
}

// The levers and notes of a test maze, with the seed of the maze.
game::Interactables placeIn(const TestMaze& test, std::uint32_t seed,
                            const game::InteractableSettings& settings = {}) {
    return game::placeInteractables(test.maze, seed, START, test.exit, test.crystals, settings);
}

// The cell on the other side of a wall.
game::MazeCell cellBehind(const game::WallRef& wall) {
    return {.x = wall.cell.x + game::columnStep(wall.side),
            .z = wall.cell.z + game::rowStep(wall.side)};
}

// How many passages closer to the start the far cell of a wall gets when the wall is
// opened: the score of chooseShortcutWalls, counted again here from the distances.
int stepsSaved(const game::Maze& maze, const game::WallRef& wall) {
    const std::vector<int> distances = game::passageDistances(maze, START);
    const auto width = static_cast<std::size_t>(maze.width());
    const game::MazeCell behind = cellBehind(wall);
    const int here = distances[static_cast<std::size_t>(wall.cell.z) * width +
                               static_cast<std::size_t>(wall.cell.x)];
    const int there =
        distances[static_cast<std::size_t>(behind.z) * width + static_cast<std::size_t>(behind.x)];
    return std::abs(here - there) - 1;
}

bool holdsCrystal(const TestMaze& test, game::MazeCell cell) {
    for (const game::CrystalSpawn& crystal : test.crystals) {
        if (crystal.cell == cell) {
            return true;
        }
    }
    return false;
}

// True when the two boxes are the same box.
bool sameBox(const scene::Aabb& a, const scene::Aabb& b) {
    return a.min == b.min && a.max == b.max;
}

bool sameLever(const game::Lever& a, const game::Lever& b) {
    return a.mount == b.mount && a.position == b.position && sameBox(a.box, b.box) &&
           a.opens == b.opens;
}

bool sameNote(const game::Note& a, const game::Note& b) {
    return a.mount == b.mount && a.position == b.position && sameBox(a.box, b.box) &&
           a.kind == b.kind && a.flavourIndex == b.flavourIndex;
}

bool sameInteractables(const game::Interactables& a, const game::Interactables& b) {
    if (a.levers.size() != b.levers.size() || a.notes.size() != b.notes.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.levers.size(); ++i) {
        if (!sameLever(a.levers[i], b.levers[i])) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.notes.size(); ++i) {
        if (!sameNote(a.notes[i], b.notes[i])) {
            return false;
        }
    }
    return true;
}

// Checks what is true for anything that hangs on a wall: the wall exists, no lever opens
// it, the position is on the visible face of that wall and the pick box reaches from the
// wall into the cell, past the collision box of the wall.
void checkMount(const game::Maze& maze, const game::Interactables& placed,
                const game::WallRef& mount, const glm::vec3& position, const scene::Aabb& box) {
    REQUIRE(maze.contains(mount.cell.x, mount.cell.z));
    CHECK(maze.hasWall(mount.cell.x, mount.cell.z, mount.side));
    for (const game::Lever& lever : placed.levers) {
        CHECK_FALSE(game::sameWall(mount, lever.opens));
    }

    // The wall stands on the cell border. Its visible face is half of its thickness
    // nearer to the centre of the cell, and the position is on that face.
    const glm::vec3 center = game::cellCenter(mount.cell.x, mount.cell.z);
    const glm::vec3 towardsWall{static_cast<float>(game::columnStep(mount.side)), 0.0F,
                                static_cast<float>(game::rowStep(mount.side))};
    const float toFace = glm::dot(position - center, towardsWall);
    CHECK(toFace == doctest::Approx(game::CELL_SIZE / 2.0F - game::WALL_VISUAL_THICKNESS / 2.0F));

    // The box lies inside the cell, seen from above.
    CHECK(box.min.x >= center.x - game::CELL_SIZE / 2.0F);
    CHECK(box.max.x <= center.x + game::CELL_SIZE / 2.0F);
    CHECK(box.min.z >= center.z - game::CELL_SIZE / 2.0F);
    CHECK(box.max.z <= center.z + game::CELL_SIZE / 2.0F);
    // The position is in the box, half way up.
    CHECK(position.y == doctest::Approx((box.min.y + box.max.y) / 2.0F));

    // The face of the box that looks into the cell is nearer to the centre of the cell
    // than the face of the collision box of the wall: the picking ray meets it first.
    const scene::Aabb wall =
        game::wallBox(game::wallSegmentOn(mount.cell.x, mount.cell.z, mount.side));
    const float boxFront =
        std::min(glm::dot(box.min - center, towardsWall), glm::dot(box.max - center, towardsWall));
    const float wallFront = std::min(glm::dot(wall.min - center, towardsWall),
                                     glm::dot(wall.max - center, towardsWall));
    CHECK(boxFront < wallFront);
}

// Checks every rule of the placement on one maze.
void checkPlacement(const TestMaze& test, const game::Interactables& placed,
                    const game::InteractableSettings& settings) {
    const game::Maze& maze = test.maze;

    CHECK(placed.levers.size() <= static_cast<std::size_t>(settings.leverCount));
    CHECK(placed.notes.size() <= static_cast<std::size_t>(settings.noteCount));

    // A copy of the maze in which the walls of the earlier levers are already open: the
    // score of every lever is counted in it.
    game::Maze opened = maze;

    for (std::size_t i = 0; i < placed.levers.size(); ++i) {
        CAPTURE(i);
        const game::Lever& lever = placed.levers[i];

        // Where it hangs.
        CHECK_FALSE(lever.mount.cell == START);
        CHECK_FALSE(lever.mount.cell == test.exit);
        checkMount(maze, placed, lever.mount, lever.position, lever.box);
        CHECK(sameBox(lever.box, game::leverBox(lever.position, lever.mount.side)));
        checkVector(lever.position, game::leverPosition(lever.mount, 0.0F));

        // What it opens: a wall between two cells that exists, named from its western
        // or northern cell, and not a wall of the exit cell.
        CHECK(game::isInteriorWall(maze, lever.opens));
        const bool namedFromFirstCell =
            lever.opens.side == game::Direction::East || lever.opens.side == game::Direction::South;
        CHECK(namedFromFirstCell);
        CHECK_FALSE(lever.opens.cell == test.exit);
        CHECK_FALSE(cellBehind(lever.opens) == test.exit);

        // It is worth it, also with the earlier shortcuts open.
        REQUIRE(game::isInteriorWall(opened, lever.opens));
        CHECK(stepsSaved(opened, lever.opens) >= game::LEVER_MIN_STEPS_SAVED);
        opened.removeWall(lever.opens.cell.x, lever.opens.cell.z, lever.opens.side);

        // No two levers share a cell or open the same wall.
        for (std::size_t other = 0; other < i; ++other) {
            CHECK_FALSE(lever.mount.cell == placed.levers[other].mount.cell);
            CHECK_FALSE(game::sameWall(lever.opens, placed.levers[other].opens));
        }
    }

    for (std::size_t i = 0; i < placed.notes.size(); ++i) {
        CAPTURE(i);
        const game::Note& note = placed.notes[i];

        CHECK_FALSE(note.mount.cell == START);
        CHECK_FALSE(note.mount.cell == test.exit);
        checkMount(maze, placed, note.mount, note.position, note.box);
        CHECK(sameBox(note.box, game::noteBox(note.position, note.mount.side)));
        checkVector(note.position, game::notePosition(note.mount, 0.0F));

        // The kinds take turns, and a flavour line is one of the table.
        CHECK(static_cast<std::size_t>(note.kind) ==
              i % static_cast<std::size_t>(game::NOTE_KIND_COUNT));
        CHECK(note.flavourIndex >= 0);
        CHECK(note.flavourIndex < game::flavourLineCount());

        // A cell of its own: no lever and no other note in it.
        for (const game::Lever& lever : placed.levers) {
            CHECK_FALSE(note.mount.cell == lever.mount.cell);
        }
        for (std::size_t other = 0; other < i; ++other) {
            CHECK_FALSE(note.mount.cell == placed.notes[other].mount.cell);
        }
    }
}

// A lever or a note made by hand, hanging on one side of a cell on flat ground.
game::Lever leverOn(game::MazeCell cell, game::Direction side) {
    game::Lever lever;
    lever.mount = {.cell = cell, .side = side};
    lever.position = game::leverPosition(lever.mount, 0.0F);
    lever.box = game::leverBox(lever.position, side);
    return lever;
}

game::Note noteOn(game::MazeCell cell, game::Direction side, game::NoteKind kind) {
    game::Note note;
    note.mount = {.cell = cell, .side = side};
    note.position = game::notePosition(note.mount, 0.0F);
    note.box = game::noteBox(note.position, side);
    note.kind = kind;
    return note;
}

// A ray from the eyes of a player who stands in the middle of a cell towards a point.
scene::Ray rayFromCell(game::MazeCell cell, const glm::vec3& target) {
    const glm::vec3 eye =
        game::cellCenter(cell.x, cell.z) + glm::vec3{0.0F, game::Player::EYE_HEIGHT, 0.0F};
    return {.origin = eye, .direction = glm::normalize(target - eye)};
}

} // namespace

TEST_CASE("the default maze gets two levers and three notes, one of each kind") {
    CHECK(game::DEFAULT_LEVER_COUNT == 2);
    CHECK(game::DEFAULT_NOTE_COUNT == 3);
    const game::InteractableSettings settings;
    CHECK(settings.leverCount == game::DEFAULT_LEVER_COUNT);
    CHECK(settings.noteCount == game::DEFAULT_NOTE_COUNT);

    const TestMaze test =
        testMaze(game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, game::DEFAULT_MAZE_SEED);
    const game::Interactables placed = placeIn(test, game::DEFAULT_MAZE_SEED);

    REQUIRE(placed.levers.size() == 2U);
    REQUIRE(placed.notes.size() == 3U);
    CHECK(placed.notes[0].kind == game::NoteKind::ExitHint);
    CHECK(placed.notes[1].kind == game::NoteKind::CrystalHint);
    CHECK(placed.notes[2].kind == game::NoteKind::Flavour);
    checkPlacement(test, placed, settings);
}

TEST_CASE("a wall has two names, and sameWall knows both") {
    const game::WallRef east{.cell = {.x = 2, .z = 3}, .side = game::Direction::East};
    const game::WallRef fromBehind{.cell = {.x = 3, .z = 3}, .side = game::Direction::West};
    const game::WallRef south{.cell = {.x = 2, .z = 3}, .side = game::Direction::South};
    const game::WallRef southFromBehind{.cell = {.x = 2, .z = 4}, .side = game::Direction::North};

    CHECK(game::sameWall(east, east));
    CHECK(game::sameWall(east, fromBehind));
    CHECK(game::sameWall(fromBehind, east));
    CHECK(game::sameWall(south, southFromBehind));

    // operator== compares the names, not the walls.
    CHECK_FALSE(east == fromBehind);

    // Another side of the same cell, and the same side of another cell.
    CHECK_FALSE(game::sameWall(east, south));
    CHECK_FALSE(game::sameWall(east, {.cell = {.x = 3, .z = 3}, .side = game::Direction::East}));
}

TEST_CASE("an interior wall exists and stands between two cells of the maze") {
    // A new maze has every wall.
    game::Maze maze(3, 2);
    const game::WallRef between{.cell = {.x = 0, .z = 0}, .side = game::Direction::East};

    CHECK(game::isInteriorWall(maze, between));
    CHECK(game::isInteriorWall(maze, {.cell = {.x = 1, .z = 0}, .side = game::Direction::West}));
    CHECK(game::isInteriorWall(maze, {.cell = {.x = 1, .z = 0}, .side = game::Direction::South}));

    // The outer border is not interior.
    CHECK_FALSE(
        game::isInteriorWall(maze, {.cell = {.x = 0, .z = 0}, .side = game::Direction::North}));
    CHECK_FALSE(
        game::isInteriorWall(maze, {.cell = {.x = 2, .z = 1}, .side = game::Direction::East}));

    // A wall that was removed is no wall.
    maze.removeWall(0, 0, game::Direction::East);
    CHECK_FALSE(game::isInteriorWall(maze, between));

    CHECK_THROWS_AS(
        game::isInteriorWall(maze, {.cell = {.x = 3, .z = 0}, .side = game::Direction::West}),
        std::out_of_range);
}

TEST_CASE("golden maze: 4 x 4 cells from seed 1 has exactly one wall worth a lever") {
    // The maze of the golden test in MazeGeneratorTests.cpp, with the number of passages
    // from the start written into every cell:
    //
    //   +--+--+--+--+
    //   | 0#11 12 13|      #: the wall between (0, 0) and (1, 0). The cell behind it is
    //   +  +  +--+==+         11 passages from the start. With the wall open it is 1:
    //   | 1|10  9|14|         that saves 10, the best score of this maze.
    //   +  +--+  +--+      14: the exit cell. Its walls are never opened.
    //   | 2  3| 8  7|
    //   +--+  +--+  +      The other walls: 1|10 saves 8, 10 over 3 saves 6, 3|8 saves 4,
    //   | 5  4  5  6|         and the rest save 2.
    //   +--+--+--+--+
    const game::Maze maze = game::generateMaze(4, 4, 1U);
    const game::MazeCell exit = game::farthestCell(maze, START);
    REQUIRE(exit == game::MazeCell{.x = 3, .z = 1});

    const game::WallRef best{.cell = {.x = 0, .z = 0}, .side = game::Direction::East};
    CHECK(stepsSaved(maze, best) == 10);
    CHECK(stepsSaved(maze, {.cell = {.x = 0, .z = 1}, .side = game::Direction::East}) == 8);
    CHECK(stepsSaved(maze, {.cell = {.x = 1, .z = 1}, .side = game::Direction::South}) == 6);

    // Asked for one wall: the best one.
    const std::vector<game::WallRef> one = game::chooseShortcutWalls(maze, START, exit, 1);
    REQUIRE(one.size() == 1U);
    CHECK(one[0] == best);

    // Asked for more: still one. With the first shortcut open the start is next to the
    // top row, and the two walls that saved 8 and 6 before save nothing any more: the
    // cells behind them are now reached through the shortcut.
    const std::vector<game::WallRef> three = game::chooseShortcutWalls(maze, START, exit, 3);
    REQUIRE(three.size() == 1U);
    CHECK(three[0] == best);

    // Asked for none, or for a negative number: none.
    CHECK(game::chooseShortcutWalls(maze, START, exit, 0).empty());
    CHECK(game::chooseShortcutWalls(maze, START, exit, -3).empty());

    // The maze of the caller is not changed.
    CHECK(maze.hasWall(0, 0, game::Direction::East));
}

TEST_CASE("the first shortcut is the wall that saves the most, the first one in row order") {
    constexpr std::uint32_t SEED_COUNT = 10;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::Maze maze = game::generateMaze(9, 6, seed);
        const game::MazeCell exit = game::farthestCell(maze, START);
        const std::vector<game::WallRef> chosen = game::chooseShortcutWalls(maze, START, exit, 1);
        REQUIRE(chosen.size() == 1U);
        const int chosenSaved = stepsSaved(maze, chosen[0]);

        // Every other wall that could have been chosen: none saves more, and none that
        // comes earlier in row order saves as much.
        bool beforeChosen = true;
        for (int z = 0; z < maze.height(); ++z) {
            for (int x = 0; x < maze.width(); ++x) {
                for (const game::Direction side : {game::Direction::East, game::Direction::South}) {
                    const game::WallRef wall{.cell = {.x = x, .z = z}, .side = side};
                    if (wall == chosen[0]) {
                        beforeChosen = false;
                        continue;
                    }
                    if (!game::isInteriorWall(maze, wall) || wall.cell == exit ||
                        cellBehind(wall) == exit) {
                        continue;
                    }
                    if (beforeChosen) {
                        CHECK(stepsSaved(maze, wall) < chosenSaved);
                    } else {
                        CHECK(stepsSaved(maze, wall) <= chosenSaved);
                    }
                }
            }
        }
    }
}

TEST_CASE("a maze without a wall worth opening gets no lever") {
    const game::InteractableSettings settings;

    SUBCASE("one cell: nothing at all") {
        const TestMaze test = testMaze(1, 1, 1U);
        const game::Interactables placed = placeIn(test, 1U);
        CHECK(placed.levers.empty());
        CHECK(placed.notes.empty());
    }

    SUBCASE("one row or one column: a corridor has no wall between two cells") {
        constexpr std::uint32_t SEED_COUNT = 5;
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(seed);
            const TestMaze row = testMaze(7, 1, seed);
            const game::Interactables inRow = placeIn(row, seed);
            CHECK(inRow.levers.empty());
            // The five cells between the start and the exit can still hold the notes.
            CHECK(inRow.notes.size() == 3U);
            checkPlacement(row, inRow, settings);

            const TestMaze column = testMaze(1, 7, seed);
            const game::Interactables inColumn = placeIn(column, seed);
            CHECK(inColumn.levers.empty());
            CHECK(inColumn.notes.size() == 3U);
            checkPlacement(column, inColumn, settings);
        }
    }

    SUBCASE("a corridor of two cells is only the start and the exit: no note either") {
        const TestMaze test = testMaze(2, 1, 1U);
        const game::Interactables placed = placeIn(test, 1U);
        CHECK(placed.levers.empty());
        CHECK(placed.notes.empty());
    }

    SUBCASE("2 x 2 cells: the one wall left is a wall of the exit cell") {
        constexpr std::uint32_t SEED_COUNT = 10;
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(seed);
            const TestMaze test = testMaze(2, 2, seed);
            const game::Interactables placed = placeIn(test, seed);
            CHECK(placed.levers.empty());
            // Two cells are neither the start nor the exit.
            CHECK(placed.notes.size() == 2U);
            checkPlacement(test, placed, settings);
        }
    }
}

TEST_CASE("every rule of the placement holds, over many seeds and sizes") {
    // Width and height of the mazes: small, narrow, the default, not square, and large.
    struct Size {
        int width;
        int height;
    };
    constexpr std::array<Size, 8> SIZES = {
        Size{.width = 3, .height = 3},   Size{.width = 2, .height = 9},
        Size{.width = 9, .height = 2},   Size{.width = 5, .height = 5},
        Size{.width = 10, .height = 10}, Size{.width = 9, .height = 6},
        Size{.width = 6, .height = 14},  Size{.width = 40, .height = 40}};
    constexpr std::uint32_t SEED_COUNT = 12;

    // More than the default, so that also the later, weaker shortcuts are checked.
    const game::InteractableSettings settings{.leverCount = 5, .noteCount = 7};

    for (const Size size : SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            const TestMaze test = testMaze(size.width, size.height, seed);
            const game::Interactables placed = placeIn(test, seed, settings);
            checkPlacement(test, placed, settings);
        }
    }
}

TEST_CASE("the default maze size gets all its levers, in cells without a crystal") {
    constexpr std::uint32_t SEED_COUNT = 40;
    const game::InteractableSettings settings;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const TestMaze test = testMaze(game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, seed);
        const game::Interactables placed = placeIn(test, seed);

        CHECK(placed.levers.size() == 2U);
        CHECK(placed.notes.size() == 3U);
        // 100 cells and 13 crystals: there is always a cell without a crystal.
        for (const game::Lever& lever : placed.levers) {
            CHECK_FALSE(holdsCrystal(test, lever.mount.cell));
        }
        checkPlacement(test, placed, settings);
    }
}

TEST_CASE("a lever goes into a cell with a crystal only when no other cell is left") {
    const TestMaze test = testMaze(10, 10, 1U);
    const game::InteractableSettings oneLever{.leverCount = 1, .noteCount = 0};

    // A dead end that is neither the start nor the exit. It has three walls, so there is
    // always one to hang a lever on.
    game::MazeCell deadEnd = START;
    for (int z = 0; z < test.maze.height(); ++z) {
        for (int x = 0; x < test.maze.width(); ++x) {
            const game::MazeCell cell{.x = x, .z = z};
            if (!(cell == START) && !(cell == test.exit) && game::isDeadEnd(test.maze, x, z)) {
                deadEnd = cell;
            }
        }
    }
    REQUIRE_FALSE(deadEnd == START);

    // A crystal in every cell of the maze but that one: the lever has to take it,
    // whatever the seed says.
    std::vector<game::CrystalSpawn> allButOne;
    std::vector<game::CrystalSpawn> all;
    for (int z = 0; z < test.maze.height(); ++z) {
        for (int x = 0; x < test.maze.width(); ++x) {
            const game::MazeCell cell{.x = x, .z = z};
            all.push_back({.cell = cell, .variant = 0});
            if (!(cell == deadEnd)) {
                allButOne.push_back({.cell = cell, .variant = 0});
            }
        }
    }

    constexpr std::uint32_t SEED_COUNT = 10;
    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const game::Interactables placed =
            game::placeInteractables(test.maze, seed, START, test.exit, allButOne, oneLever);
        REQUIRE(placed.levers.size() == 1U);
        CHECK(placed.levers[0].mount.cell == deadEnd);

        // A crystal in every cell: the lever is not left out, it shares a cell.
        const game::Interactables crowded =
            game::placeInteractables(test.maze, seed, START, test.exit, all, oneLever);
        CHECK(crowded.levers.size() == 1U);
    }
}

TEST_CASE("the same maze and seed give the same levers and notes") {
    constexpr std::uint32_t SEED_COUNT = 10;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const TestMaze first = testMaze(10, 10, seed);
        const TestMaze second = testMaze(10, 10, seed);
        CHECK(sameInteractables(placeIn(first, seed), placeIn(second, seed)));
    }
}

TEST_CASE("another seed gives other levers and notes") {
    SUBCASE("another maze") {
        const TestMaze first = testMaze(10, 10, 1U);
        const TestMaze second = testMaze(10, 10, 2U);
        CHECK_FALSE(sameInteractables(placeIn(first, 1U), placeIn(second, 2U)));
    }

    SUBCASE("the same maze: the walls to open stay, the places to hang change") {
        // The walls follow from the maze alone. The seed only chooses the cells and the
        // sides the levers and the notes hang on.
        const TestMaze test = testMaze(10, 10, 1U);
        const game::Interactables first = placeIn(test, 1U);

        constexpr std::uint32_t SEED_COUNT = 10;
        std::size_t different = 0;
        for (std::uint32_t seed = 2; seed < 2 + SEED_COUNT; ++seed) {
            CAPTURE(seed);
            const game::Interactables other = placeIn(test, seed);
            REQUIRE(other.levers.size() == first.levers.size());
            for (std::size_t i = 0; i < first.levers.size(); ++i) {
                CHECK(other.levers[i].opens == first.levers[i].opens);
            }
            if (!sameInteractables(first, other)) {
                ++different;
            }
        }
        // 98 cells to choose from: ten other seeds do not all choose the same ones.
        CHECK(different == static_cast<std::size_t>(SEED_COUNT));
    }
}

TEST_CASE("the number of notes does not move the levers") {
    const TestMaze test = testMaze(10, 10, 1U);
    const game::Interactables few = placeIn(test, 1U, {.leverCount = 2, .noteCount = 0});
    const game::Interactables many = placeIn(test, 1U, {.leverCount = 2, .noteCount = 9});

    CHECK(few.notes.empty());
    CHECK(many.notes.size() == 9U);
    REQUIRE(few.levers.size() == many.levers.size());
    for (std::size_t i = 0; i < few.levers.size(); ++i) {
        CHECK(sameLever(few.levers[i], many.levers[i]));
    }
}

TEST_CASE("the wanted numbers are kept between zero and the largest allowed") {
    const TestMaze test = testMaze(40, 40, 3U);

    SUBCASE("zero and below: nothing") {
        const game::Interactables none = placeIn(test, 3U, {.leverCount = 0, .noteCount = 0});
        CHECK(none.levers.empty());
        CHECK(none.notes.empty());

        const game::Interactables negative = placeIn(test, 3U, {.leverCount = -4, .noteCount = -1});
        CHECK(negative.levers.empty());
        CHECK(negative.notes.empty());
    }

    SUBCASE("far too many: the largest allowed") {
        const game::InteractableSettings tooMany{.leverCount = 1000, .noteCount = 1000};
        const game::Interactables placed = placeIn(test, 3U, tooMany);
        CHECK(placed.levers.size() <= static_cast<std::size_t>(game::MAX_LEVER_COUNT));
        CHECK(placed.notes.size() == static_cast<std::size_t>(game::MAX_NOTE_COUNT));
        // The largest number of notes: every flavour line number stays in range.
        checkPlacement(test, placed, tooMany);
    }
}

TEST_CASE("the largest maze gets its levers and notes") {
    // 256 by 256 cells: the cost of the placement is one search over the maze per lever,
    // so this is a matter of a moment and not of minutes.
    const TestMaze test = testMaze(game::Maze::MAX_SIZE, game::Maze::MAX_SIZE, 1U);
    const game::InteractableSettings settings;
    const game::Interactables placed = placeIn(test, 1U);

    CHECK(placed.levers.size() == 2U);
    CHECK(placed.notes.size() == 3U);
    checkPlacement(test, placed, settings);
}

TEST_CASE("a start or an exit outside the maze is an error") {
    const game::Maze maze = game::generateMaze(4, 4, 1U);
    const game::MazeCell outside{.x = 4, .z = 0};
    const game::MazeCell inside{.x = 3, .z = 1};
    const game::InteractableSettings settings;

    CHECK_THROWS_AS(game::chooseShortcutWalls(maze, outside, inside, 1), std::out_of_range);
    CHECK_THROWS_AS(game::chooseShortcutWalls(maze, START, outside, 1), std::out_of_range);
    CHECK_THROWS_AS(game::placeInteractables(maze, 1U, outside, inside, {}, settings),
                    std::out_of_range);
    CHECK_THROWS_AS(game::placeInteractables(maze, 1U, START, outside, {}, settings),
                    std::out_of_range);
}

TEST_CASE("a lever hangs on the visible face of its wall, a hand high above the ground") {
    // Cell (2, 3) covers x from 4 to 6 and z from 6 to 8. Its centre is (5, 7).
    const game::MazeCell cell{.x = 2, .z = 3};
    constexpr float GROUND = 0.5F;

    SUBCASE("on the north side") {
        // The wall stands on the line z = 6 and is 0.2 m thick: its face is at z = 6.1.
        const game::WallRef mount{.cell = cell, .side = game::Direction::North};
        const glm::vec3 position = game::leverPosition(mount, GROUND);
        checkVector(position, {5.0F, GROUND + game::LEVER_MOUNT_HEIGHT, 6.1F});

        // 0.3 m wide along X, 0.4 m high, and 0.25 m deep from the wall towards +Z.
        const scene::Aabb box = game::leverBox(position, mount.side);
        checkVector(box.min, {4.85F, 1.5F, 6.1F});
        checkVector(box.max, {5.15F, 1.9F, 6.35F});
    }

    SUBCASE("on the east side") {
        // The wall stands on the line x = 6: its face is at x = 5.9.
        const game::WallRef mount{.cell = cell, .side = game::Direction::East};
        const glm::vec3 position = game::leverPosition(mount, GROUND);
        checkVector(position, {5.9F, GROUND + game::LEVER_MOUNT_HEIGHT, 7.0F});

        // The width is along Z now, the depth along X, from the wall towards -X.
        const scene::Aabb box = game::leverBox(position, mount.side);
        checkVector(box.min, {5.65F, 1.5F, 6.85F});
        checkVector(box.max, {5.9F, 1.9F, 7.15F});
    }

    SUBCASE("on the south and on the west side") {
        checkVector(game::leverPosition({.cell = cell, .side = game::Direction::South}, GROUND),
                    {5.0F, GROUND + game::LEVER_MOUNT_HEIGHT, 7.9F});
        checkVector(game::leverPosition({.cell = cell, .side = game::Direction::West}, GROUND),
                    {4.1F, GROUND + game::LEVER_MOUNT_HEIGHT, 7.0F});
    }
}

TEST_CASE("a note hangs like a lever, a little below the eyes, with a box of its own size") {
    const game::WallRef mount{.cell = {.x = 0, .z = 0}, .side = game::Direction::West};
    const glm::vec3 position = game::notePosition(mount, 0.0F);
    checkVector(position, {0.1F, game::NOTE_MOUNT_HEIGHT, 1.0F});
    CHECK(game::NOTE_MOUNT_HEIGHT < game::Player::EYE_HEIGHT);

    // 0.15 m deep from the wall towards +X, 0.5 m high and 0.4 m wide along Z.
    const scene::Aabb box = game::noteBox(position, mount.side);
    checkVector(box.min, {0.1F, 1.25F, 0.8F});
    checkVector(box.max, {0.25F, 1.75F, 1.2F});
}

TEST_CASE("levers and notes follow the height of the terrain") {
    const TestMaze test = testMaze(10, 10, 1U);
    game::Interactables placed = placeIn(test, 1U);
    const game::Interactables flat = placed;

    // Ground that is not level: four heights, tiled over the maze.
    const game::Heightmap heightmap{.width = 2, .height = 2, .values = {0.0F, 1.0F, 0.6F, 0.2F}};
    const game::Terrain terrain(10, 10, heightmap, 2.0F);

    game::placeInteractablesOnTerrain(placed, terrain);

    bool anyMoved = false;
    REQUIRE(placed.levers.size() == flat.levers.size());
    for (std::size_t i = 0; i < placed.levers.size(); ++i) {
        const game::Lever& lever = placed.levers[i];
        // Nothing moves sideways.
        CHECK(lever.position.x == flat.levers[i].position.x);
        CHECK(lever.position.z == flat.levers[i].position.z);
        const float ground = terrain.heightAt(lever.position.x, lever.position.z);
        CHECK(lever.position.y == doctest::Approx(ground + game::LEVER_MOUNT_HEIGHT));
        CHECK(sameBox(lever.box, game::leverBox(lever.position, lever.mount.side)));
        anyMoved = anyMoved || lever.position.y != flat.levers[i].position.y;
    }
    REQUIRE(placed.notes.size() == flat.notes.size());
    for (std::size_t i = 0; i < placed.notes.size(); ++i) {
        const game::Note& note = placed.notes[i];
        CHECK(note.position.x == flat.notes[i].position.x);
        CHECK(note.position.z == flat.notes[i].position.z);
        const float ground = terrain.heightAt(note.position.x, note.position.z);
        CHECK(note.position.y == doctest::Approx(ground + game::NOTE_MOUNT_HEIGHT));
        CHECK(sameBox(note.box, game::noteBox(note.position, note.mount.side)));
        anyMoved = anyMoved || note.position.y != flat.notes[i].position.y;
    }
    // The test would prove nothing on ground that happens to be level.
    CHECK(anyMoved);

    // Doing it again changes nothing, and a flat terrain brings everything back down.
    const game::Interactables once = placed;
    game::placeInteractablesOnTerrain(placed, terrain);
    CHECK(sameInteractables(placed, once));
    game::placeInteractablesOnTerrain(placed, game::Terrain{});
    CHECK(sameInteractables(placed, flat));
}

TEST_CASE("a round starts with no lever pulled, and a lever opens its wall once") {
    const TestMaze test = testMaze(10, 10, 1U);
    const game::Interactables placed = placeIn(test, 1U);
    REQUIRE(placed.levers.size() == 2U);

    game::InteractableState state = game::startInteractables(placed);
    REQUIRE(state.leverPulled.size() == 2U);
    CHECK_FALSE(game::isLeverPulled(state, 0));
    CHECK_FALSE(game::isLeverPulled(state, 1));
    CHECK(game::openedWalls(placed, state).empty());

    // The first pull of lever 1 opens its wall.
    const game::PullResult first = game::pullLever(state, placed, 1);
    CHECK(first.opened);
    CHECK(first.wall == placed.levers[1].opens);
    CHECK_FALSE(game::isLeverPulled(state, 0));
    CHECK(game::isLeverPulled(state, 1));
    REQUIRE(game::openedWalls(placed, state).size() == 1U);
    CHECK(game::openedWalls(placed, state)[0] == placed.levers[1].opens);

    // The second pull of the same lever changes nothing and reports nothing.
    const game::PullResult second = game::pullLever(state, placed, 1);
    CHECK_FALSE(second.opened);
    CHECK_FALSE(game::isLeverPulled(state, 0));
    CHECK(game::isLeverPulled(state, 1));
    CHECK(game::openedWalls(placed, state).size() == 1U);

    // The other lever still works. The walls are listed in the order of the levers.
    const game::PullResult other = game::pullLever(state, placed, 0);
    CHECK(other.opened);
    CHECK(other.wall == placed.levers[0].opens);
    const std::vector<game::WallRef> walls = game::openedWalls(placed, state);
    REQUIRE(walls.size() == 2U);
    CHECK(walls[0] == placed.levers[0].opens);
    CHECK(walls[1] == placed.levers[1].opens);

    // A new round: every lever is back.
    state = game::startInteractables(placed);
    CHECK(game::openedWalls(placed, state).empty());
}

TEST_CASE("pulling a lever that does not exist is an error") {
    const TestMaze test = testMaze(10, 10, 1U);
    const game::Interactables placed = placeIn(test, 1U);
    game::InteractableState state = game::startInteractables(placed);

    CHECK_THROWS_AS(game::pullLever(state, placed, placed.levers.size()), std::out_of_range);

    // A state that was not started for these levers.
    game::InteractableState empty;
    CHECK_THROWS_AS(game::pullLever(empty, placed, 0), std::out_of_range);
    CHECK_THROWS_AS(game::isLeverPulled(empty, 0), std::out_of_range);
    CHECK(game::openedWalls(placed, empty).empty());
}

TEST_CASE("the wall a lever opens is one of the wall segments of the maze") {
    constexpr std::uint32_t SEED_COUNT = 10;

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const TestMaze test = testMaze(10, 10, seed);
        const game::Interactables placed = placeIn(test, seed);
        const std::vector<game::WallSegment> segments = game::wallSegments(test.maze);

        for (const game::Lever& lever : placed.levers) {
            const game::WallSegment opened = game::openedWallSegment(lever);
            std::size_t matches = 0;
            for (const game::WallSegment& segment : segments) {
                if (segment.position == opened.position && segment.axis == opened.axis) {
                    ++matches;
                }
            }
            // Exactly one: this is how buildMazeWorld finds the wall a lever lowers
            // (MazeWorld::leverWalls).
            CHECK(matches == 1);
        }
    }
}

TEST_CASE("the picking ray finds the lever or the note it points at") {
    // By hand: a lever on the north wall of cell (0, 0) and a note on its east wall.
    const game::MazeCell cell{.x = 0, .z = 0};
    game::Interactables placed;
    placed.levers.push_back(leverOn(cell, game::Direction::North));
    placed.notes.push_back(noteOn(cell, game::Direction::East, game::NoteKind::Flavour));
    const glm::vec3 leverTarget = placed.levers[0].position;
    const glm::vec3 noteTarget = placed.notes[0].position;

    SUBCASE("looking at the lever") {
        const game::PickedInteractable picked = game::pickInteractable(
            rayFromCell(cell, leverTarget), placed, {}, game::INTERACTION_REACH);
        CHECK(picked.kind == game::InteractableKind::Lever);
        CHECK(picked.index == 0U);
        // The eye is 1 m from the border, the front of the box 0.35 m: about 0.65 m,
        // a little more because the ray looks down from the eyes to the lever.
        CHECK(picked.distance > 0.65F);
        CHECK(picked.distance < 0.9F);
    }

    SUBCASE("looking at the note") {
        const game::PickedInteractable picked = game::pickInteractable(
            rayFromCell(cell, noteTarget), placed, {}, game::INTERACTION_REACH);
        CHECK(picked.kind == game::InteractableKind::Note);
        CHECK(picked.index == 0U);
    }

    SUBCASE("looking somewhere else") {
        const glm::vec3 south = game::cellCenter(0, 5) + glm::vec3{0.0F, 1.7F, 0.0F};
        const game::PickedInteractable picked =
            game::pickInteractable(rayFromCell(cell, south), placed, {}, game::INTERACTION_REACH);
        CHECK(picked.kind == game::InteractableKind::None);
    }

    SUBCASE("too far away") {
        // From two cells further south the lever is about 4.7 m away.
        const game::PickedInteractable picked = game::pickInteractable(
            rayFromCell({.x = 0, .z = 2}, leverTarget), placed, {}, game::INTERACTION_REACH);
        CHECK(picked.kind == game::InteractableKind::None);

        // With a longer arm it is found.
        const game::PickedInteractable longArm =
            game::pickInteractable(rayFromCell({.x = 0, .z = 2}, leverTarget), placed, {}, 10.0F);
        CHECK(longArm.kind == game::InteractableKind::Lever);
    }

    SUBCASE("the wall it hangs on does not hide it") {
        const std::array<scene::Aabb, 2> walls = {
            game::wallBox(game::wallSegmentOn(0, 0, game::Direction::North)),
            game::wallBox(game::wallSegmentOn(0, 0, game::Direction::East))};
        CHECK(game::pickInteractable(rayFromCell(cell, leverTarget), placed, walls,
                                     game::INTERACTION_REACH)
                  .kind == game::InteractableKind::Lever);
        CHECK(game::pickInteractable(rayFromCell(cell, noteTarget), placed, walls,
                                     game::INTERACTION_REACH)
                  .kind == game::InteractableKind::Note);
    }

    SUBCASE("a wall in between hides it") {
        // From the cell to the east the note on the east wall of cell (0, 0) is behind
        // that wall: the ray reaches the box of the wall first.
        const std::array<scene::Aabb, 1> wall = {
            game::wallBox(game::wallSegmentOn(0, 0, game::Direction::East))};
        const scene::Ray ray = rayFromCell({.x = 1, .z = 0}, noteTarget);

        CHECK(game::pickInteractable(ray, placed, wall, game::INTERACTION_REACH).kind ==
              game::InteractableKind::None);
        // Without the wall in the list the note would be picked through it.
        CHECK(game::pickInteractable(ray, placed, {}, game::INTERACTION_REACH).kind ==
              game::InteractableKind::Note);
    }

    SUBCASE("a blocker behind the lever does not matter") {
        const std::array<scene::Aabb, 1> behind = {
            scene::Aabb{.min = {0.0F, 0.0F, -3.0F}, .max = {2.0F, 3.0F, -2.0F}}};
        CHECK(game::pickInteractable(rayFromCell(cell, leverTarget), placed, behind,
                                     game::INTERACTION_REACH)
                  .kind == game::InteractableKind::Lever);
    }

    SUBCASE("of a note and a lever in the line of the ray the nearer one is picked") {
        // A second note on the south wall of the cell, and a ray from outside the cell
        // that goes through it first and through the lever on the north wall after it.
        placed.notes.push_back(noteOn(cell, game::Direction::South, game::NoteKind::Flavour));
        const scene::Ray ray{.origin = {1.0F, 1.4F, 2.5F}, .direction = {0.0F, 0.0F, -1.0F}};

        const game::PickedInteractable picked = game::pickInteractable(ray, placed, {}, 10.0F);
        CHECK(picked.kind == game::InteractableKind::Note);
        CHECK(picked.index == 1U);
    }

    SUBCASE("nothing to pick") {
        const game::Interactables nothing;
        CHECK(game::pickInteractable(rayFromCell(cell, leverTarget), nothing, {},
                                     game::INTERACTION_REACH)
                  .kind == game::InteractableKind::None);
    }
}

TEST_CASE("every lever and note of a maze can be picked from the middle of its cell") {
    constexpr std::uint32_t SEED_COUNT = 10;
    const game::InteractableSettings settings{.leverCount = 4, .noteCount = 8};

    for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
        CAPTURE(seed);
        const TestMaze test = testMaze(10, 10, seed);
        const game::Interactables placed = placeIn(test, seed, settings);
        // Every wall and pillar of the maze can be in the way.
        const std::vector<scene::Aabb> blockers = game::mazeColliders(test.maze);

        for (std::size_t i = 0; i < placed.levers.size(); ++i) {
            CAPTURE(i);
            const game::Lever& lever = placed.levers[i];
            const game::PickedInteractable picked =
                game::pickInteractable(rayFromCell(lever.mount.cell, lever.position), placed,
                                       blockers, game::INTERACTION_REACH);
            CHECK(picked.kind == game::InteractableKind::Lever);
            CHECK(picked.index == i);
        }
        for (std::size_t i = 0; i < placed.notes.size(); ++i) {
            CAPTURE(i);
            const game::Note& note = placed.notes[i];
            const game::PickedInteractable picked =
                game::pickInteractable(rayFromCell(note.mount.cell, note.position), placed,
                                       blockers, game::INTERACTION_REACH);
            CHECK(picked.kind == game::InteractableKind::Note);
            CHECK(picked.index == i);
        }
    }
}

TEST_CASE("compassTowards names the eight directions and the cell itself") {
    const game::MazeCell from{.x = 5, .z = 5};

    // North is a smaller row number (-Z), East a larger column number (+X).
    CHECK(game::compassTowards(from, {.x = 5, .z = 1}) == game::Compass::North);
    CHECK(game::compassTowards(from, {.x = 8, .z = 2}) == game::Compass::NorthEast);
    CHECK(game::compassTowards(from, {.x = 9, .z = 5}) == game::Compass::East);
    CHECK(game::compassTowards(from, {.x = 8, .z = 8}) == game::Compass::SouthEast);
    CHECK(game::compassTowards(from, {.x = 5, .z = 9}) == game::Compass::South);
    CHECK(game::compassTowards(from, {.x = 2, .z = 8}) == game::Compass::SouthWest);
    CHECK(game::compassTowards(from, {.x = 1, .z = 5}) == game::Compass::West);
    CHECK(game::compassTowards(from, {.x = 2, .z = 2}) == game::Compass::NorthWest);

    CHECK(game::compassTowards(from, from) == game::Compass::Here);
}

TEST_CASE("a direction is straight when one offset is more than twice the other") {
    const game::MazeCell from{.x = 10, .z = 10};

    // Exactly twice is still a diagonal.
    CHECK(game::compassTowards(from, {.x = 12, .z = 9}) == game::Compass::NorthEast);
    CHECK(game::compassTowards(from, {.x = 11, .z = 8}) == game::Compass::NorthEast);
    CHECK(game::compassTowards(from, {.x = 6, .z = 12}) == game::Compass::SouthWest);

    // One more and it is straight.
    CHECK(game::compassTowards(from, {.x = 13, .z = 9}) == game::Compass::East);
    CHECK(game::compassTowards(from, {.x = 11, .z = 7}) == game::Compass::North);
    CHECK(game::compassTowards(from, {.x = 5, .z = 12}) == game::Compass::West);
    CHECK(game::compassTowards(from, {.x = 9, .z = 13}) == game::Compass::South);

    // The neighbouring cells.
    CHECK(game::compassTowards(from, {.x = 10, .z = 9}) == game::Compass::North);
    CHECK(game::compassTowards(from, {.x = 11, .z = 11}) == game::Compass::SouthEast);
}

TEST_CASE("the compass directions have English names") {
    CHECK(game::compassName(game::Compass::Here) == "here");
    CHECK(game::compassName(game::Compass::North) == "north");
    CHECK(game::compassName(game::Compass::NorthEast) == "north-east");
    CHECK(game::compassName(game::Compass::East) == "east");
    CHECK(game::compassName(game::Compass::SouthEast) == "south-east");
    CHECK(game::compassName(game::Compass::South) == "south");
    CHECK(game::compassName(game::Compass::SouthWest) == "south-west");
    CHECK(game::compassName(game::Compass::West) == "west");
    CHECK(game::compassName(game::Compass::NorthWest) == "north-west");
}

TEST_CASE("the flavour lines are short plain text") {
    REQUIRE(game::flavourLineCount() > 0);

    for (int index = 0; index < game::flavourLineCount(); ++index) {
        CAPTURE(index);
        const std::string_view line = game::flavourLine(index);
        CHECK_FALSE(line.empty());
        // One line of the HUD card.
        CHECK(line.size() <= 60U);
        // The font of the HUD has the plain ASCII letters only.
        for (const char letter : line) {
            CHECK(letter >= ' ');
            CHECK(letter <= '~');
        }
        // No line is in the table twice.
        for (int other = 0; other < index; ++other) {
            CHECK(line != game::flavourLine(other));
        }
    }

    CHECK_THROWS_AS(game::flavourLine(-1), std::out_of_range);
    CHECK_THROWS_AS(game::flavourLine(game::flavourLineCount()), std::out_of_range);
}

TEST_CASE("a note says where the exit is, where the nearest crystal is, or a flavour line") {
    const game::MazeCell cell{.x = 5, .z = 5};
    const game::MazeCell exit{.x = 9, .z = 1};

    SUBCASE("the exit hint") {
        const game::Note note = noteOn(cell, game::Direction::North, game::NoteKind::ExitHint);
        CHECK(game::noteText(note, exit, {}) == "The exit lies to the north-east.");
        CHECK(game::noteText(note, {.x = 5, .z = 0}, {}) == "The exit lies to the north.");
        // Not possible in the game (no note hangs in the exit cell), but it has an answer.
        CHECK(game::noteText(note, cell, {}) == "The exit lies right here.");
    }

    SUBCASE("the crystal hint points at the nearest crystal that is left") {
        const game::Note note = noteOn(cell, game::Direction::North, game::NoteKind::CrystalHint);

        // One far away in the west and one near in the south.
        const std::vector<game::MazeCell> crystals = {{.x = 0, .z = 5}, {.x = 5, .z = 7}};
        CHECK(game::noteText(note, exit, crystals) == "A crystal glows to the south.");

        // The near one was collected: the caller passes only what is left.
        const std::vector<game::MazeCell> left = {{.x = 0, .z = 5}};
        CHECK(game::noteText(note, exit, left) == "A crystal glows to the west.");

        // Of two equally near crystals the earlier one in the list is named.
        const std::vector<game::MazeCell> equal = {{.x = 5, .z = 3}, {.x = 7, .z = 5}};
        CHECK(game::noteText(note, exit, equal) == "A crystal glows to the north.");

        // A crystal in the cell of the note.
        const std::vector<game::MazeCell> here = {{.x = 0, .z = 5}, cell};
        CHECK(game::noteText(note, exit, here) == "A crystal glows right here.");

        CHECK(game::noteText(note, exit, {}) == "No crystal is left to find.");
    }

    SUBCASE("the flavour line") {
        game::Note note = noteOn(cell, game::Direction::North, game::NoteKind::Flavour);
        note.flavourIndex = 2;
        CHECK(game::noteText(note, exit, {}) == game::flavourLine(2));
    }
}

TEST_CASE("the flavour notes of a maze take different lines until the table runs out") {
    const TestMaze test = testMaze(10, 10, 1U);
    // Twelve notes: four of each kind.
    const game::Interactables placed = placeIn(test, 1U, {.leverCount = 0, .noteCount = 12});
    REQUIRE(placed.notes.size() == 12U);
    REQUIRE(game::flavourLineCount() >= 4);

    std::vector<int> used;
    for (const game::Note& note : placed.notes) {
        if (note.kind != game::NoteKind::Flavour) {
            continue;
        }
        for (const int earlier : used) {
            CHECK(note.flavourIndex != earlier);
        }
        used.push_back(note.flavourIndex);
    }
    CHECK(used.size() == 4U);
}
