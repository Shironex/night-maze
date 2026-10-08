// Tests of the shade (game/Shade.hpp): where it starts, when the flashlight is on it, how
// it walks to the player and when it catches, and how a round carries it (game/Round.hpp).
#include "game/Shade.hpp"

#include "game/Difficulty.hpp"
#include "game/Exit.hpp"
#include "game/Lighting.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "game/Terrain.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float. 120 steps are one second.
constexpr float STEP = 1.0F / 120.0F;
constexpr int STEPS_PER_SECOND = 120;

// The cone and the range of the flashlight of the game (game::LightingSettings).
constexpr float CONE_DEGREES = 21.0F;
constexpr float RANGE = 10.0F;

// A corridor of `length` cells that runs from west to east: one row, no walls between
// its cells.
game::Maze corridor(int length) {
    game::Maze maze(length, 1);
    for (int x = 0; x + 1 < length; ++x) {
        maze.removeWall(x, 0, game::Direction::East);
    }
    return maze;
}

// Where the feet of someone stand who is in the middle of a cell, on flat ground.
glm::vec3 feetIn(game::MazeCell cell) {
    return game::cellCenter(cell.x, cell.z);
}

// A shade that stands in the middle of a cell and is past its grace time.
game::Shade shadeIn(game::MazeCell cell) {
    game::Shade shade;
    shade.present = true;
    shade.position = feetIn(cell);
    shade.previousPosition = shade.position;
    shade.target = cell;
    return shade;
}

// A lamp that is on, held at the height of a hand at `feet`, and aimed at the chest of
// whoever stands at `towards`.
game::ShadeLamp lampAimedAt(const glm::vec3& feet, const glm::vec3& towards) {
    const glm::vec3 hand = feet + glm::vec3{0.0F, 1.45F, 0.0F};
    const glm::vec3 chest = towards + glm::vec3{0.0F, 1.2F, 0.0F};
    return {.on = true,
            .position = hand,
            .direction = glm::normalize(chest - hand),
            .outerDegrees = CONE_DEGREES,
            .range = RANGE};
}

// One step of a shade nobody shines at.
bool stepInTheDark(game::Shade& shade, const game::Maze& maze, const glm::vec3& playerFeet,
                   const game::ShadeSettings& settings = {}, int openedWalls = 0) {
    const game::Terrain flat;
    const game::ShadeStep step{.playerFeet = playerFeet, .openedWalls = openedWalls};
    return game::advanceShade(shade, settings, maze, flat, step, STEP);
}

// True when the two cells are the same, or neighbours with no wall between them.
bool sameOrJoined(const game::Maze& maze, game::MazeCell from, game::MazeCell to) {
    if (from == to) {
        return true;
    }
    for (const game::Direction direction : game::ALL_DIRECTIONS) {
        const game::MazeCell neighbour{.x = from.x + game::columnStep(direction),
                                       .z = from.z + game::rowStep(direction)};
        if (neighbour == to) {
            return !maze.hasWall(from.x, from.z, direction);
        }
    }
    return false;
}

// The place of a cell in a list that holds the rows one after another, like in Maze.
std::size_t placeOf(const game::Maze& maze, game::MazeCell cell) {
    return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
           static_cast<std::size_t>(cell.x);
}

// The maze of a difficulty level.
game::MazeWorld worldOf(game::Difficulty difficulty, std::uint32_t seed) {
    const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
    return game::buildMazeWorld(level.mazeWidth, level.mazeHeight, seed, {}, level.crystalCount);
}

} // namespace

TEST_CASE("the shade settings start with the agreed numbers") {
    const game::ShadeSettings settings;
    CHECK(settings.enabled);
    CHECK(settings.speed == 4.0F);
    CHECK(settings.graceSeconds == 8.0F);
    CHECK(settings.catchDistance == 0.9F);
    CHECK(settings.thawSeconds == 2.0F);
    CHECK_FALSE(settings.showOnMap);
    // Faster than the player walks, slower than the player sprints.
    CHECK(settings.speed > 3.0F);
    CHECK(settings.speed < 5.5F);
}

TEST_CASE("the shade is lit by a lamp that is on, aimed at it, near enough and unblocked") {
    const game::Maze maze = corridor(8);
    const std::vector<scene::Aabb> walls = game::mazeColliders(maze);
    const glm::vec3 player = feetIn({.x = 0, .z = 0});
    const game::Shade shade = shadeIn({.x = 3, .z = 0});

    const game::ShadeLamp lamp = lampAimedAt(player, shade.position);
    CHECK(game::shadeLit(shade, lamp, walls));
}

TEST_CASE("each condition of lit is needed: the lamp on, the cone, the range, no wall") {
    const game::Maze maze = corridor(8);
    const std::vector<scene::Aabb> walls = game::mazeColliders(maze);
    const glm::vec3 player = feetIn({.x = 0, .z = 0});
    const game::Shade shade = shadeIn({.x = 3, .z = 0});
    const game::ShadeLamp good = lampAimedAt(player, shade.position);
    REQUIRE(game::shadeLit(shade, good, walls));

    SUBCASE("the lamp is off: looking at the shade does not stop it") {
        game::ShadeLamp lamp = good;
        lamp.on = false;
        CHECK_FALSE(game::shadeLit(shade, lamp, walls));
    }
    SUBCASE("the shade is outside the cone") {
        // The lamp points the other way, and then across the corridor.
        game::ShadeLamp lamp = good;
        lamp.direction = -good.direction;
        CHECK_FALSE(game::shadeLit(shade, lamp, walls));
        lamp.direction = {0.0F, 0.0F, 1.0F};
        CHECK_FALSE(game::shadeLit(shade, lamp, walls));
    }
    SUBCASE("the shade is out of range") {
        // Six cells are 12 m, and the lamp reaches 10 m.
        const game::Shade far = shadeIn({.x = 6, .z = 0});
        const game::ShadeLamp lamp = lampAimedAt(player, far.position);
        CHECK_FALSE(game::shadeLit(far, lamp, walls));
        // The same lamp with a longer reach lights it: only the range was missing.
        game::ShadeLamp longer = lamp;
        longer.range = 20.0F;
        CHECK(game::shadeLit(far, longer, walls));
    }
    SUBCASE("a wall stands between the lamp and the shade") {
        // The same two places in a maze that has every wall: three walls in between.
        const game::Maze closed(8, 1);
        CHECK_FALSE(game::shadeLit(shade, good, game::mazeColliders(closed)));
    }
    SUBCASE("a round without a shade has nothing to light") {
        CHECK_FALSE(game::shadeLit(game::Shade{}, good, walls));
    }
}

TEST_CASE("an empty battery gives no light, whatever the switch says") {
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 1U);
    const game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    game::LightingSettings lighting;
    const game::FlashlightPose pose;

    lighting.flashlightOn = true;
    CHECK(game::roundShadeLamp(lighting, round, pose).on);
    round.battery = 0.0F;
    CHECK_FALSE(game::roundShadeLamp(lighting, round, pose).on);
    round.battery = 0.5F;
    lighting.flashlightOn = false;
    CHECK_FALSE(game::roundShadeLamp(lighting, round, pose).on);

    // The cone and the range are the ones of the lighting settings.
    const game::ShadeLamp lamp = game::roundShadeLamp(lighting, round, pose);
    CHECK(lamp.outerDegrees == lighting.flashlightOuterDegrees);
    CHECK(lamp.range == lighting.flashlightRange);
}

TEST_CASE("a lit shade does not move and does not catch") {
    const game::Maze maze = corridor(8);
    const std::vector<scene::Aabb> walls = game::mazeColliders(maze);
    const game::Terrain flat;
    const game::ShadeSettings settings;
    const glm::vec3 player = feetIn({.x = 0, .z = 0});
    game::Shade shade = shadeIn({.x = 3, .z = 0});
    const glm::vec3 before = shade.position;

    const game::ShadeStep step{
        .playerFeet = player, .lamp = lampAimedAt(player, shade.position), .obstacles = walls};
    for (int i = 0; i < 5 * STEPS_PER_SECOND; ++i) {
        CHECK_FALSE(game::advanceShade(shade, settings, maze, flat, step, STEP));
    }
    CHECK(shade.lit);
    CHECK(shade.position == before);

    // A player who walks right up to a lit shade is not caught either.
    const glm::vec3 close = shade.position + glm::vec3{-0.5F, 0.0F, 0.0F};
    const game::ShadeStep touching{
        .playerFeet = close, .lamp = lampAimedAt(close, shade.position), .obstacles = walls};
    CHECK_FALSE(game::advanceShade(shade, settings, maze, flat, touching, STEP));
    CHECK(shade.position == before);
}

TEST_CASE("an unlit shade walks at its speed and catches at the catch distance") {
    const game::Maze maze = corridor(8);
    const game::ShadeSettings settings;
    const glm::vec3 player = feetIn({.x = 0, .z = 0});
    game::Shade shade = shadeIn({.x = 6, .z = 0});
    const float startDistance = game::shadeDistance(shade, player);
    REQUIRE(startDistance == doctest::Approx(12.0F));

    // One second: four metres nearer.
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        REQUIRE_FALSE(stepInTheDark(shade, maze, player, settings));
    }
    CHECK_FALSE(shade.lit);
    CHECK(game::shadeDistance(shade, player) ==
          doctest::Approx(startDistance - 4.0F).epsilon(0.01));

    // It goes on until it is at the catch distance, and not before.
    int steps = STEPS_PER_SECOND;
    bool caught = false;
    while (!caught && steps < 10 * STEPS_PER_SECOND) {
        const float distanceBefore = game::shadeDistance(shade, player);
        caught = stepInTheDark(shade, maze, player, settings);
        ++steps;
        if (!caught) {
            CHECK(game::shadeDistance(shade, player) > settings.catchDistance);
            CHECK(game::shadeDistance(shade, player) < distanceBefore);
        }
    }
    REQUIRE(caught);
    CHECK(game::shadeDistance(shade, player) <= settings.catchDistance);
    // (12 m - 0.9 m) at 4 m/s is 2.775 s.
    CHECK(static_cast<float>(steps) * STEP == doctest::Approx(2.775F).epsilon(0.01));
}

TEST_CASE("the shade stands still for the grace time and cannot catch in it") {
    const game::Maze maze = corridor(8);
    game::ShadeSettings settings;
    settings.graceSeconds = 2.0F;
    game::Shade shade = shadeIn({.x = 1, .z = 0});
    shade.graceLeft = settings.graceSeconds;
    const glm::vec3 before = shade.position;
    // The player stands in the next cell, and then right inside the shade.
    const glm::vec3 player = feetIn({.x = 0, .z = 0});

    // Two steps per turn of the loop: just under the two seconds in all.
    for (int i = 0; i < STEPS_PER_SECOND - 1; ++i) {
        REQUIRE_FALSE(stepInTheDark(shade, maze, player, settings));
        REQUIRE_FALSE(stepInTheDark(shade, maze, before, settings));
    }
    CHECK(shade.position == before);

    // After the grace time it walks, and reaches the player half a cell away.
    bool caught = false;
    for (int i = 0; i < STEPS_PER_SECOND && !caught; ++i) {
        caught = stepInTheDark(shade, maze, player, settings);
    }
    CHECK(caught);
    CHECK(shade.position != before);
}

TEST_CASE("the shade follows the passages and never crosses a wall") {
    // Generated mazes of the three levels, the shade from its own start cell, the
    // player standing at the start of the maze.
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        for (std::uint32_t seed = 1; seed <= 3; ++seed) {
            const game::MazeWorld world = worldOf(difficulty, seed);
            const game::Maze& maze = world.maze;
            game::ShadeSettings settings;
            settings.graceSeconds = 0.0F;
            game::Shade shade = game::startShade(maze, seed, game::START_CELL, world.exitCell,
                                                 world.terrain, settings);
            REQUIRE(shade.present);
            const glm::vec3 player = world.startPosition;
            const int passages = game::passageDistances(
                maze, game::START_CELL)[placeOf(maze, game::cellAt(shade.position))];

            game::MazeCell cell = game::cellAt(shade.position);
            bool caught = false;
            int steps = 0;
            // Far more steps than the longest way needs: every cell of the maze once.
            const int limit = maze.width() * maze.height() * STEPS_PER_SECOND;
            while (!caught && steps < limit) {
                caught = stepInTheDark(shade, maze, player, settings);
                ++steps;
                const game::MazeCell now = game::cellAt(shade.position);
                REQUIRE(maze.contains(now.x, now.z));
                REQUIRE(sameOrJoined(maze, cell, now));
                cell = now;
            }
            REQUIRE(caught);
            // It took the shortest way: `passages` cells of 2 m at 4 m/s, less the catch
            // distance. A detour would take longer.
            const float seconds = static_cast<float>(steps) * STEP;
            const float shortest =
                (static_cast<float>(passages) * game::CELL_SIZE - settings.catchDistance) /
                settings.speed;
            CHECK(seconds == doctest::Approx(shortest).epsilon(0.02));
        }
    }
}

TEST_CASE("the shade turns round when the player gets behind it") {
    const game::Maze maze = corridor(9);
    const game::ShadeSettings settings;
    game::Shade shade = shadeIn({.x = 4, .z = 0});

    // Towards a player in the east for a quarter of a second.
    for (int i = 0; i < STEPS_PER_SECOND / 4; ++i) {
        REQUIRE_FALSE(stepInTheDark(shade, maze, feetIn({.x = 8, .z = 0}), settings));
    }
    const float eastmost = shade.position.x;
    CHECK(eastmost > feetIn({.x = 4, .z = 0}).x);

    // The player is in the west now: the shade comes back at once, from where it is.
    for (int i = 0; i < STEPS_PER_SECOND / 4; ++i) {
        REQUIRE_FALSE(stepInTheDark(shade, maze, feetIn({.x = 0, .z = 0}), settings));
    }
    CHECK(shade.position.x == doctest::Approx(eastmost - 1.0F).epsilon(0.01));
}

TEST_CASE("the way is searched again only when the player changes cell or a wall opens") {
    const game::Maze maze = corridor(8);
    game::Shade shade = shadeIn({.x = 6, .z = 0});
    const glm::vec3 player = feetIn({.x = 0, .z = 0});

    REQUIRE_FALSE(stepInTheDark(shade, maze, player));
    CHECK(shade.pathGoal == game::MazeCell{.x = 0, .z = 0});
    CHECK(shade.pathOpenings == 0);

    // A mark in the kept distances: it stays while nothing changes, also when the
    // player moves inside the cell.
    shade.pathDistances[7] = 77;
    REQUIRE_FALSE(stepInTheDark(shade, maze, player + glm::vec3{0.4F, 0.0F, 0.3F}));
    CHECK(shade.pathDistances[7] == 77);

    // Another cell: searched again.
    REQUIRE_FALSE(stepInTheDark(shade, maze, feetIn({.x = 1, .z = 0})));
    CHECK(shade.pathDistances[7] == 6);
    CHECK(shade.pathGoal == game::MazeCell{.x = 1, .z = 0});

    // A lever was pulled: searched again.
    shade.pathDistances[7] = 77;
    REQUIRE_FALSE(stepInTheDark(shade, maze, feetIn({.x = 1, .z = 0}), {}, 1));
    CHECK(shade.pathDistances[7] == 6);
    CHECK(shade.pathOpenings == 1);
}

TEST_CASE("a player outside the maze or behind closed walls is not followed") {
    game::ShadeSettings settings;
    SUBCASE("outside the maze: the shade waits and nothing throws") {
        const game::Maze maze = corridor(6);
        game::Shade shade = shadeIn({.x = 3, .z = 0});
        const glm::vec3 before = shade.position;
        for (int i = 0; i < STEPS_PER_SECOND; ++i) {
            CHECK_FALSE(stepInTheDark(shade, maze, {-30.0F, 12.0F, -30.0F}, settings));
        }
        CHECK(shade.position == before);
    }
    SUBCASE("no way to the player: the shade stays in its cell") {
        // Every wall stands: no cell leads anywhere.
        const game::Maze maze(4, 1);
        game::Shade shade = shadeIn({.x = 3, .z = 0});
        const glm::vec3 before = shade.position;
        for (int i = 0; i < STEPS_PER_SECOND; ++i) {
            CHECK_FALSE(stepInTheDark(shade, maze, feetIn({.x = 0, .z = 0}), settings));
        }
        CHECK(shade.position == before);
    }
}

TEST_CASE("the shade starts in a far cell that is neither the start nor the exit") {
    // The three levels, fifty seeds each.
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        for (std::uint32_t seed = 1; seed <= 50; ++seed) {
            const game::MazeWorld world = worldOf(difficulty, seed);
            const game::Maze& maze = world.maze;
            game::MazeCell cell;
            REQUIRE(game::shadeStartCell(maze, seed, game::START_CELL, world.exitCell, cell));
            CHECK(maze.contains(cell.x, cell.z));
            CHECK_FALSE(cell == game::START_CELL);
            CHECK_FALSE(cell == world.exitCell);

            const std::vector<int> distances = game::passageDistances(maze, game::START_CELL);
            const int farthest = distances[placeOf(maze, world.exitCell)];
            const int own = distances[placeOf(maze, cell)];
            CHECK(own * 10 >= farthest * game::SHADE_START_FAR_TENTHS);
            // Far in metres too: more than the lamp reaches, on every level.
            CHECK(static_cast<float>(own) * game::CELL_SIZE > 2.0F * RANGE);
            CHECK(level.mazeWidth == maze.width());
        }
    }
}

TEST_CASE("the start of the shade follows from the maze and the seed alone") {
    std::set<int> cells;
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        for (std::uint32_t seed = 1; seed <= 10; ++seed) {
            const game::MazeWorld first = worldOf(difficulty, seed);
            const game::MazeWorld second = worldOf(difficulty, seed);
            const game::ShadeSettings settings;
            const game::Shade a = game::startShade(first.maze, seed, game::START_CELL,
                                                   first.exitCell, first.terrain, settings);
            const game::Shade b = game::startShade(second.maze, seed, game::START_CELL,
                                                   second.exitCell, second.terrain, settings);
            REQUIRE(a.present);
            CHECK(a.position == b.position);
            CHECK(a.target == b.target);
            CHECK(a.graceLeft == settings.graceSeconds);
            CHECK(a.target == game::cellAt(a.position));
            cells.insert(a.target.z * first.maze.width() + a.target.x);
        }
    }
    // Not one fixed cell for every maze.
    CHECK(cells.size() > 10U);
}

TEST_CASE("a maze too small for a far cell has no shade") {
    const game::Terrain flat;
    const game::ShadeSettings settings;
    // One cell, and two cells: only the start and the exit.
    const game::Maze one(1, 1);
    CHECK_FALSE(
        game::startShade(one, 1U, {.x = 0, .z = 0}, {.x = 0, .z = 0}, flat, settings).present);
    const game::Maze two = corridor(2);
    CHECK_FALSE(
        game::startShade(two, 1U, {.x = 0, .z = 0}, {.x = 1, .z = 0}, flat, settings).present);
}

TEST_CASE("a calm round has no shade") {
    const game::MazeWorld world = worldOf(game::Difficulty::Normal, 3U);
    game::GameplaySettings gameplay;
    REQUIRE(game::startRound(world, gameplay).shade.present);

    gameplay.shade.enabled = false;
    game::Round round = game::startRound(world, gameplay);
    CHECK_FALSE(round.shade.present);

    // Nothing ever catches the player, wherever the player stands and for however long.
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    for (int i = 0; i < 60 * STEPS_PER_SECOND; ++i) {
        CHECK_FALSE(game::updateRoundShade(round, world, gameplay, world.startPosition, {},
                                           obstacles, STEP));
    }
    CHECK_FALSE(round.shade.present);
}

TEST_CASE("switching the shade off in a round removes it at once") {
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 2U);
    game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    REQUIRE(round.shade.present);
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);

    gameplay.shade.enabled = false;
    CHECK_FALSE(
        game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP));
    CHECK_FALSE(round.shade.present);
}

TEST_CASE("a round catches a player who stands still in the dark, after the grace time") {
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::MazeWorld world = worldOf(difficulty, 1U);
        const game::GameplaySettings gameplay;
        game::Round round = game::startRound(world, gameplay);
        REQUIRE(round.shade.present);
        const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
        const float startDistance = game::shadeDistance(round.shade, world.startPosition);

        int steps = 0;
        bool caught = false;
        while (!caught && steps < 600 * STEPS_PER_SECOND) {
            caught = game::updateRoundShade(round, world, gameplay, world.startPosition, {},
                                            obstacles, STEP);
            ++steps;
            if (steps == static_cast<int>(gameplay.shade.graceSeconds) * STEPS_PER_SECOND - 1) {
                // Still in the grace time: it has not moved.
                CHECK(game::shadeDistance(round.shade, world.startPosition) == startDistance);
            }
        }
        REQUIRE(caught);
        CHECK(static_cast<float>(steps) * STEP > gameplay.shade.graceSeconds);
    }
}

TEST_CASE("a won round has no shade that moves or catches") {
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 1U);
    game::GameplaySettings gameplay;
    gameplay.shade.graceSeconds = 0.0F;
    game::Round round = game::startRound(world, gameplay);
    REQUIRE(round.shade.present);
    round.state = game::RoundState::Won;
    const glm::vec3 before = round.shade.position;
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        CHECK_FALSE(game::updateRoundShade(round, world, gameplay, before, {}, obstacles, STEP));
    }
    CHECK(round.shade.position == before);
}

TEST_CASE("a wall sunk by a lever shortens the way of the shade") {
    // A maze with a lever: its wall stands between a near and a far cell, at least seven
    // passages apart (LEVER_MIN_STEPS_SAVED). The shade stands on one side and the
    // player on the other.
    const game::MazeWorld world = worldOf(game::Difficulty::Normal, 1U);
    REQUIRE_FALSE(world.interactables.levers.empty());
    const game::WallRef wall = world.interactables.levers[0].opens;
    const game::MazeCell here = wall.cell;
    const game::MazeCell behind{.x = here.x + game::columnStep(wall.side),
                                .z = here.z + game::rowStep(wall.side)};
    game::GameplaySettings gameplay;
    gameplay.shade.graceSeconds = 0.0F;

    // How many steps the shade needs from `here` to a player in `behind`.
    const auto stepsToCatch = [&](bool leverPulled) {
        game::Round round = game::startRound(world, gameplay);
        round.shade = shadeIn(here);
        if (leverPulled) {
            REQUIRE(game::pullRoundLever(round, world, 0));
        }
        const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
        int steps = 0;
        while (
            steps < 600 * STEPS_PER_SECOND &&
            !game::updateRoundShade(round, world, gameplay, feetIn(behind), {}, obstacles, STEP)) {
            ++steps;
        }
        return steps;
    };

    const int around = stepsToCatch(false);
    const int through = stepsToCatch(true);
    // Through the opening it is one passage: 2 m less the catch distance, at 4 m/s.
    CHECK(static_cast<float>(through) * STEP == doctest::Approx(0.275F).epsilon(0.05));
    // Around it is at least seven passages.
    CHECK(static_cast<float>(around) * STEP > 3.0F);
    CHECK(through < around);
}

TEST_CASE("a lever pulled while the shade is on its way opens the way for it too") {
    const game::MazeWorld world = worldOf(game::Difficulty::Normal, 1U);
    REQUIRE_FALSE(world.interactables.levers.empty());
    const game::WallRef wall = world.interactables.levers[0].opens;
    const game::MazeCell here = wall.cell;
    const game::MazeCell behind{.x = here.x + game::columnStep(wall.side),
                                .z = here.z + game::rowStep(wall.side)};
    game::GameplaySettings gameplay;
    gameplay.shade.graceSeconds = 0.0F;
    game::Round round = game::startRound(world, gameplay);
    round.shade = shadeIn(here);

    // One step the long way round, then the lever: the next step heads for the opening.
    std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    REQUIRE_FALSE(
        game::updateRoundShade(round, world, gameplay, feetIn(behind), {}, obstacles, STEP));
    CHECK_FALSE(round.shade.target == behind);
    REQUIRE(game::pullRoundLever(round, world, 0));
    obstacles = game::roundObstacles(world, round);
    REQUIRE_FALSE(
        game::updateRoundShade(round, world, gameplay, feetIn(behind), {}, obstacles, STEP));
    CHECK(round.shade.target == behind);
}

TEST_CASE("starting the round again puts the shade back") {
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 5U);
    game::GameplaySettings gameplay;
    gameplay.shade.graceSeconds = 0.0F;
    game::Round round = game::startRound(world, gameplay);
    const glm::vec3 start = round.shade.position;
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    for (int i = 0; i < 2 * STEPS_PER_SECOND; ++i) {
        game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP);
    }
    REQUIRE(round.shade.position != start);

    round = game::startRound(world, gameplay);
    CHECK(round.shade.position == start);
    CHECK(round.caughtLine == game::NO_CAUGHT_LINE);
}

TEST_CASE("the caught lines rotate in order and never repeat twice in a row") {
    CHECK(game::nextCaughtLine(game::NO_CAUGHT_LINE) == 0);
    int line = game::NO_CAUGHT_LINE;
    for (int i = 0; i < 3 * game::CAUGHT_LINE_COUNT; ++i) {
        const int next = game::nextCaughtLine(line);
        CHECK(next != line);
        CHECK(next == i % game::CAUGHT_LINE_COUNT);
        line = next;
    }
}

TEST_CASE("every caught line fits the HUD: at most 60 printable ASCII characters") {
    std::set<std::string_view> lines;
    for (int index = 0; index < game::CAUGHT_LINE_COUNT; ++index) {
        const std::string_view line = game::caughtLine(index);
        CHECK_FALSE(line.empty());
        CHECK(line.size() <= 60U);
        for (const char character : line) {
            CHECK(character >= ' ');
            CHECK(character <= '~');
        }
        lines.insert(line);
    }
    CHECK(lines.size() == static_cast<std::size_t>(game::CAUGHT_LINE_COUNT));
    CHECK(game::caughtLine(0) == "It carried you back to the stile. Nothing more than that.");
    CHECK(game::caughtLine(4) == "Still dark. Still yours to do.");
    CHECK_THROWS_AS(game::caughtLine(-1), std::out_of_range);
    CHECK_THROWS_AS(game::caughtLine(game::CAUGHT_LINE_COUNT), std::out_of_range);
}

TEST_CASE("the caught line of a round shows for five seconds of play") {
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 1U);
    const game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    CHECK(round.caughtLine == game::NO_CAUGHT_LINE);
    CHECK(game::roundBrightness(round) == 1.0F);

    game::showCaughtLine(round, 2);
    CHECK(round.caughtLine == 2);
    // Black at the moment of the catch.
    CHECK(game::roundBrightness(round) == 0.0F);

    bool flashlightOn = true;
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        game::updateRound(round, world, gameplay, world.startPosition, flashlightOn, STEP);
    }
    CHECK(round.caughtLine == 2);
    CHECK(game::roundBrightness(round) > 0.5F);
    for (int i = 0; i < 5 * STEPS_PER_SECOND; ++i) {
        game::updateRound(round, world, gameplay, world.startPosition, flashlightOn, STEP);
    }
    CHECK(round.caughtLine == game::NO_CAUGHT_LINE);
    CHECK(game::roundBrightness(round) == 1.0F);
}

TEST_CASE("the picture comes back from black smoothly") {
    CHECK(game::caughtBrightness(0.0F) == 0.0F);
    CHECK(game::caughtBrightness(game::CAUGHT_FADE_SECONDS) == 1.0F);
    CHECK(game::caughtBrightness(60.0F) == 1.0F);
    CHECK(game::caughtBrightness(-1.0F) == 0.0F);
    float before = 0.0F;
    for (int i = 1; i <= 12; ++i) {
        const float now = game::caughtBrightness(static_cast<float>(i) * 0.1F);
        CHECK(now > before);
        before = now;
    }
}

TEST_CASE("the shade turns its front towards the player") {
    const glm::vec3 here{4.0F, 0.0F, 4.0F};
    // The model looks along +Z: no turn for a player in the south.
    CHECK(game::shadeYawDegrees(here, {4.0F, 0.0F, 9.0F}) == doctest::Approx(0.0F));
    CHECK(game::shadeYawDegrees(here, {9.0F, 0.0F, 4.0F}) == doctest::Approx(90.0F));
    CHECK(game::shadeYawDegrees(here, {-1.0F, 0.0F, 4.0F}) == doctest::Approx(-90.0F));
    CHECK(std::abs(game::shadeYawDegrees(here, {4.0F, 0.0F, -1.0F})) == doctest::Approx(180.0F));
    // The height of the player does not matter, and the same spot gives no turn.
    CHECK(game::shadeYawDegrees(here, {9.0F, 5.0F, 9.0F}) == doctest::Approx(45.0F));
    CHECK(game::shadeYawDegrees(here, here) == 0.0F);
}

TEST_CASE("after the light leaves it the shade stands still a moment longer, then walks") {
    const game::Maze maze = corridor(8);
    const std::vector<scene::Aabb> walls = game::mazeColliders(maze);
    const game::Terrain flat;
    game::ShadeSettings settings;
    settings.thawSeconds = 2.0F;
    const glm::vec3 player = feetIn({.x = 0, .z = 0});
    game::Shade shade = shadeIn({.x = 3, .z = 0});
    const glm::vec3 before = shade.position;

    // One step in the light.
    const game::ShadeStep lit{
        .playerFeet = player, .lamp = lampAimedAt(player, shade.position), .obstacles = walls};
    REQUIRE_FALSE(game::advanceShade(shade, settings, maze, flat, lit, STEP));
    REQUIRE(shade.lit);

    // The lamp is switched off. For two seconds nothing moves, and a player who walks
    // right through the shade in that time is not caught.
    for (int i = 0; i < 2 * STEPS_PER_SECOND - 1; ++i) {
        REQUIRE_FALSE(stepInTheDark(shade, maze, i % 2 == 0 ? player : before, settings));
    }
    CHECK_FALSE(shade.lit);
    CHECK(shade.position == before);

    // Then it comes: 6 m less the catch distance at 4 m/s.
    int steps = 0;
    while (steps < 10 * STEPS_PER_SECOND && !stepInTheDark(shade, maze, player, settings)) {
        ++steps;
    }
    CHECK(static_cast<float>(steps) * STEP == doctest::Approx(1.275F).epsilon(0.05));

    // Without the wait it walks in the very step after the light.
    settings.thawSeconds = 0.0F;
    game::Shade eager = shadeIn({.x = 3, .z = 0});
    REQUIRE_FALSE(game::advanceShade(eager, settings, maze, flat, lit, STEP));
    REQUIRE_FALSE(stepInTheDark(eager, maze, player, settings));
    CHECK(eager.position != before);
}

TEST_CASE("sprinting away from the shade works, walking away and being winded do not") {
    // A long straight corridor, the shade behind the player, nobody shines at it.
    const game::Maze maze = corridor(200);
    const game::ShadeSettings settings;
    const game::StaminaSettings staminaSettings;

    // What happens in 30 seconds to a player who runs east and holds the sprint key or
    // not. Returns the gap at the end, or a negative number when the shade caught up.
    // The player starts in cell 10 and the shade in the cell shadeColumn: every cell
    // between them is 2 m.
    const auto flee = [&](bool holdsSprint, game::Stamina stamina, int shadeColumn) {
        glm::vec3 player = feetIn({.x = 10, .z = 0});
        game::Shade shade = shadeIn({.x = shadeColumn, .z = 0});
        for (int i = 0; i < 30 * STEPS_PER_SECOND; ++i) {
            const bool sprints = game::advanceStamina(stamina, staminaSettings, holdsSprint, STEP);
            player.x += (sprints ? game::Player::SPRINT_SPEED : game::Player::WALK_SPEED) * STEP;
            if (stepInTheDark(shade, maze, player, settings)) {
                return -1.0F;
            }
        }
        return game::shadeDistance(shade, player);
    };

    // With the sprint key held the stamina runs out and comes back in turns, and over
    // those turns the player is a little faster than the shade: never caught, and
    // farther away than at the start.
    CHECK(flee(true, game::Stamina{}, 7) > 6.0F);
    // Walking loses a metre every second: caught, from 6 m behind and from 20 m behind.
    CHECK(flee(false, game::Stamina{}, 7) < 0.0F);
    CHECK(flee(false, game::Stamina{}, 0) < 0.0F);
    // A winded player cannot sprint until the stamina is half back, which takes three
    // seconds of walking. With the shade 2 m behind that is too late. With it 6 m behind
    // there is just time.
    game::Stamina winded;
    winded.level = 0.0F;
    winded.winded = true;
    CHECK(flee(true, winded, 9) < 0.0F);
    CHECK(flee(true, winded, 7) > 0.0F);
}

TEST_CASE("a player who starts on the very spot of the shade has the grace time too") {
    // What a capture run can do with --start-cell: the first round starts in the cell
    // the shade starts in. Nobody is caught in the grace time, the numbers stay
    // numbers (the two stand on one point), and after it the shade has the player.
    const game::MazeWorld world = worldOf(game::Difficulty::Normal, 7U);
    const game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    REQUIRE(round.shade.present);
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    const glm::vec3 feet = round.shade.position;

    const int graceSteps = static_cast<int>(gameplay.shade.graceSeconds) * STEPS_PER_SECOND;
    for (int i = 0; i < graceSteps - 1; ++i) {
        REQUIRE_FALSE(game::updateRoundShade(round, world, gameplay, feet, {}, obstacles, STEP));
    }
    CHECK(round.shade.position == feet);
    CHECK(round.shade.wayMetres == 0.0F);
    CHECK(std::isfinite(game::shadeYawDegrees(round.shade.position, feet)));

    // The grace time is over: within the next second it has the player.
    bool caught = false;
    for (int i = 0; i < STEPS_PER_SECOND && !caught; ++i) {
        caught = game::updateRoundShade(round, world, gameplay, feet, {}, obstacles, STEP);
    }
    CHECK(caught);
}
