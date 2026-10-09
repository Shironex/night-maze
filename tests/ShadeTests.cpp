// Tests of the shade (game/Shade.hpp): where it starts, how it wanders, what it hears and
// sees, when the flashlight is on it and burns it away, how it walks and when it catches,
// and how a round carries it (game/Round.hpp).
#include "game/Shade.hpp"

#include "game/Campaign.hpp"
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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float. 120 steps are one second.
constexpr float STEP = 1.0F / 120.0F;
constexpr int STEPS_PER_SECOND = 120;

// The cone and the range of the flashlight of the game (game::LightingSettings).
constexpr float CONE_DEGREES = 21.0F;
constexpr float RANGE = 10.0F;

// A place outside every maze: a player who flies there is neither heard nor seen, so
// the shade is left to itself.
constexpr glm::vec3 NOWHERE{-30.0F, 12.0F, -30.0F};

// A hearing distance that reaches across any maze, for tests that want the shade to
// come from wherever it is.
constexpr float HEARS_EVERYTHING = 100000.0F;

// A corridor of `length` cells that runs from west to east: one row, no walls between
// its cells.
game::Maze corridor(int length) {
    game::Maze maze(length, 1);
    for (int x = 0; x + 1 < length; ++x) {
        maze.removeWall(x, 0, game::Direction::East);
    }
    return maze;
}

// A maze of 10 x 3 cells with two corners in its one way:
//
//     row 0   (0,0) ... (9,0)      a corridor of ten cells
//     row 1           (5,1) ... (9,1)   joined to row 0 only at column 9
//     row 2           (5,2) (6,2)       joined to row 1 only at column 5
//
// So the cell (5,1) is 4.5 m from (7,0) in a straight line and seven passages (14 m)
// along the way, and nobody in rows 0 and 1 sees into the cell (6,2).
game::Maze bend() {
    game::Maze maze(10, 3);
    for (int x = 0; x < 9; ++x) {
        maze.removeWall(x, 0, game::Direction::East);
    }
    maze.removeWall(9, 0, game::Direction::South);
    for (int x = 5; x < 9; ++x) {
        maze.removeWall(x, 1, game::Direction::East);
    }
    maze.removeWall(5, 1, game::Direction::South);
    maze.removeWall(5, 2, game::Direction::East);
    return maze;
}

// Where the feet of someone stand who is in the middle of a cell, on flat ground.
glm::vec3 feetIn(game::MazeCell cell) {
    return game::cellCenter(cell.x, cell.z);
}

// A shade that stands in the middle of a cell and is past its grace time. It has not
// chosen where to wander yet.
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

// One step of a shade nobody shines at, with everything it reports.
game::ShadeEvents stepUnlit(game::Shade& shade, const game::Maze& maze, const glm::vec3& playerFeet,
                            const game::ShadeSettings& settings = {},
                            game::Noise noise = game::Noise::None, int openedWalls = 0) {
    const game::Terrain flat;
    const game::ShadeStep step{
        .playerFeet = playerFeet, .openedWalls = openedWalls, .noise = noise};
    return game::advanceShade(shade, settings, maze, flat, step, STEP);
}

// The same, for the tests that only ask whether the player was caught.
bool stepInTheDark(game::Shade& shade, const game::Maze& maze, const glm::vec3& playerFeet,
                   const game::ShadeSettings& settings = {}, game::Noise noise = game::Noise::None,
                   int openedWalls = 0) {
    return stepUnlit(shade, maze, playerFeet, settings, noise, openedWalls).caught;
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

// The maze of a night of the campaign.
game::MazeWorld worldOfNight(int night, std::uint32_t campaignSeed) {
    const game::CampaignNight& level = game::campaignNight(night);
    return game::buildMazeWorld(level.mazeWidth, level.mazeHeight,
                                game::campaignNightSeed(campaignSeed, night), {},
                                level.crystalCount);
}

// Every maze the sweeps go through: the three levels and the five nights, a few seeds
// each. A night without a shade is among them: the rule of the cells holds there too.
std::vector<game::MazeWorld> sweepWorlds(std::uint32_t seeds) {
    std::vector<game::MazeWorld> worlds;
    for (std::uint32_t seed = 1; seed <= seeds; ++seed) {
        for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
            worlds.push_back(worldOf(difficulty, seed));
        }
        for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
            worlds.push_back(worldOfNight(night, seed));
        }
    }
    return worlds;
}

// True when cell is a good far cell measured from `from`: a cell of the maze that can
// be reached, not `from`, not the exit, and at least farTenths tenths as far away as the
// farthest cell.
bool isFarCell(const game::MazeWorld& world, game::MazeCell from, game::MazeCell cell,
               int farTenths) {
    const game::Maze& maze = world.maze;
    if (!maze.contains(cell.x, cell.z) || cell == from || cell == world.exitCell) {
        return false;
    }
    const std::vector<int> distances = game::passageDistances(maze, from);
    const int farthest = *std::ranges::max_element(distances);
    const int own = distances[placeOf(maze, cell)];
    return own > 0 && own * 10 >= farthest * farTenths;
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
    // A chase is faster than the player walks, slower than the player sprints.
    CHECK(settings.speed > game::Player::WALK_SPEED);
    CHECK(settings.speed < game::Player::SPRINT_SPEED);
    // Wandering is slowest. Going to a noise is faster, and still slower than the player
    // walks.
    CHECK(settings.wanderSpeed == game::SHADE_WANDER_SPEED);
    CHECK(settings.investigateSpeed == game::SHADE_INVESTIGATE_SPEED);
    CHECK(settings.investigateSpeed > settings.wanderSpeed);
    CHECK(settings.investigateSpeed < game::Player::WALK_SPEED);
    // A walk that is just heard leaves three seconds until the shade is there.
    CHECK(settings.hearWalkMetres / settings.investigateSpeed == doctest::Approx(3.0F));
    // What it hears and sees.
    CHECK(settings.hearSprintMetres == game::SHADE_HEAR_SPRINT_METRES);
    CHECK(settings.hearLeverMetres == game::SHADE_HEAR_LEVER_METRES);
    CHECK(settings.hearPickupMetres == game::SHADE_HEAR_PICKUP_METRES);
    CHECK(settings.hearWalkMetres == game::SHADE_HEAR_WALK_METRES);
    CHECK(settings.sightMetres == game::SHADE_SIGHT_METRES);
    // It sees farther than the lamp reaches.
    CHECK(settings.sightMetres > RANGE);
    CHECK(settings.searchSeconds == game::SHADE_SEARCH_SECONDS);
    // The burn.
    CHECK(settings.burnSeconds == game::SHADE_BURN_SECONDS);
    CHECK(settings.burnRecoverRate == game::SHADE_BURN_RECOVER_RATE);
    CHECK(settings.quietSeconds == game::SHADE_QUIET_SECONDS);
    CHECK(settings.burnBatteryFactor == 3.0F);
}

TEST_CASE("a sprint carries farthest, then a lever, a pickup and a walk, and silence nowhere") {
    const game::ShadeSettings settings;
    CHECK(game::noiseReach(game::Noise::None, settings) == 0.0F);
    CHECK(game::noiseReach(game::Noise::Walk, settings) == settings.hearWalkMetres);
    CHECK(game::noiseReach(game::Noise::Pickup, settings) == settings.hearPickupMetres);
    CHECK(game::noiseReach(game::Noise::Lever, settings) == settings.hearLeverMetres);
    CHECK(game::noiseReach(game::Noise::Sprint, settings) == settings.hearSprintMetres);
    CHECK(game::noiseReach(game::Noise::Sprint, settings) >
          game::noiseReach(game::Noise::Lever, settings));
    CHECK(game::noiseReach(game::Noise::Lever, settings) >
          game::noiseReach(game::Noise::Pickup, settings));
    CHECK(game::noiseReach(game::Noise::Pickup, settings) >
          game::noiseReach(game::Noise::Walk, settings));

    // The numbers are settings: the debug window can change each one.
    game::ShadeSettings deaf = settings;
    deaf.hearWalkMetres = 0.0F;
    CHECK(game::noiseReach(game::Noise::Walk, deaf) == 0.0F);

    // Every noise and every state has a name of its own for the debug window.
    std::set<std::string> names;
    for (const game::Noise noise : {game::Noise::None, game::Noise::Walk, game::Noise::Pickup,
                                    game::Noise::Lever, game::Noise::Sprint}) {
        names.insert(game::noiseName(noise));
    }
    CHECK(names.size() == 5U);
}

TEST_CASE("the noise of the player follows from what the feet and the hands did") {
    const game::ShadeSettings settings;
    // A step of the game at the two speeds of the player.
    const game::NoiseSources standing{.stepSeconds = STEP,
                                      .walkSpeed = game::Player::WALK_SPEED,
                                      .sprintSpeed = game::Player::SPRINT_SPEED};
    game::NoiseSources walking = standing;
    walking.metres = game::Player::WALK_SPEED * STEP;
    game::NoiseSources sprinting = standing;
    sprinting.metres = game::Player::SPRINT_SPEED * STEP;

    // Standing still is silent. Reading the map is standing still (game::movementInput).
    CHECK(game::playerNoise(standing, settings) == game::Noise::None);
    CHECK(game::playerNoise(walking, settings) == game::Noise::Walk);
    CHECK(game::playerNoise(sprinting, settings) == game::Noise::Sprint);

    // The speed the feet really had decides, not a key: sliding slowly along a wall is
    // a walk, however the player meant it.
    game::NoiseSources sliding = standing;
    sliding.metres = 1.0F * STEP;
    CHECK(game::playerNoise(sliding, settings) == game::Noise::Walk);
    CHECK(game::sprintedStep(sprinting.metres, STEP, 3.0F, 5.5F));
    CHECK_FALSE(game::sprintedStep(walking.metres, STEP, 3.0F, 5.5F));
    CHECK_FALSE(game::sprintedStep(1.0F, 0.0F, 3.0F, 5.5F));

    // A player who flies has no feet on the ground.
    game::NoiseSources flying = sprinting;
    flying.flying = true;
    CHECK(game::playerNoise(flying, settings) == game::Noise::None);

    // A pickup and a lever are heard also from a player who stands.
    game::NoiseSources pickup = standing;
    pickup.pickedUp = true;
    CHECK(game::playerNoise(pickup, settings) == game::Noise::Pickup);
    game::NoiseSources lever = standing;
    lever.pulledLever = true;
    CHECK(game::playerNoise(lever, settings) == game::Noise::Lever);
    flying.pulledLever = true;
    CHECK(game::playerNoise(flying, settings) == game::Noise::Lever);

    // Several at once: the one that carries farthest.
    walking.pickedUp = true;
    CHECK(game::playerNoise(walking, settings) == game::Noise::Pickup);
    walking.pulledLever = true;
    CHECK(game::playerNoise(walking, settings) == game::Noise::Lever);
    sprinting.pickedUp = true;
    sprinting.pulledLever = true;
    CHECK(game::playerNoise(sprinting, settings) == game::Noise::Sprint);
    // "Farthest" is asked from the settings, not from the order of the names.
    game::ShadeSettings loudWalk = settings;
    loudWalk.hearWalkMetres = 50.0F;
    CHECK(game::playerNoise(walking, loudWalk) == game::Noise::Walk);
}

TEST_CASE("a noise is heard when it carries as far as the way to it") {
    const game::ShadeSettings settings;
    CHECK(game::shadeHears(13.9F, game::Noise::Sprint, settings));
    CHECK(game::shadeHears(14.0F, game::Noise::Sprint, settings));
    CHECK_FALSE(game::shadeHears(14.1F, game::Noise::Sprint, settings));
    CHECK(game::shadeHears(6.0F, game::Noise::Walk, settings));
    CHECK_FALSE(game::shadeHears(6.1F, game::Noise::Walk, settings));
    CHECK(game::shadeHears(10.0F, game::Noise::Lever, settings));
    CHECK_FALSE(game::shadeHears(10.1F, game::Noise::Lever, settings));
    CHECK(game::shadeHears(8.0F, game::Noise::Pickup, settings));
    CHECK_FALSE(game::shadeHears(8.1F, game::Noise::Pickup, settings));
    // Silence is never heard, and no noise carries where there is no way.
    CHECK_FALSE(game::shadeHears(0.0F, game::Noise::None, settings));
    CHECK_FALSE(game::shadeHears(game::SHADE_NO_WAY, game::Noise::Sprint, settings));
}

TEST_CASE("hearing is measured along the passages, so a wall between the two helps") {
    const game::Maze maze = bend();
    const game::ShadeSettings settings;
    const game::MazeCell noiseCell{.x = 5, .z = 1};
    const glm::vec3 player = feetIn(noiseCell);

    // Right behind the wall: the shade stands in the cell north of the noise, 2 m away
    // in a straight line and nine passages (18 m) along the way. Nothing is heard there,
    // not even a walk, which would carry the 2 m.
    game::Shade behindTheWall = shadeIn({.x = 5, .z = 0});
    REQUIRE(game::shadeDistance(behindTheWall, player) < settings.hearWalkMetres);
    CHECK_FALSE(stepUnlit(behindTheWall, maze, player, settings, game::Noise::Walk).alerted);
    CHECK(behindTheWall.wayMetres == doctest::Approx(18.0F).epsilon(0.01));
    CHECK(behindTheWall.hunt == game::ShadeHunt::Wandering);

    // Seven passages (14 m) away along the way, and 4.5 m in a straight line.
    const game::MazeCell shadeCell{.x = 7, .z = 0};

    // One step each, with a fresh shade: what a noise from that cell does to it.
    const auto heard = [&](game::Noise noise) {
        game::Shade shade = shadeIn(shadeCell);
        const game::ShadeEvents events = stepUnlit(shade, maze, player, settings, noise);
        CHECK(shade.wayMetres == doctest::Approx(14.0F).epsilon(0.01));
        CHECK(events.alerted == (shade.hunt == game::ShadeHunt::Investigating));
        return shade;
    };

    // A walk is heard from 6 m, a pickup from 8 m and a lever from 10 m: all of them
    // are too far along the way.
    for (const game::Noise noise :
         {game::Noise::None, game::Noise::Walk, game::Noise::Pickup, game::Noise::Lever}) {
        const game::Shade deaf = heard(noise);
        CHECK(deaf.hunt == game::ShadeHunt::Wandering);
        CHECK(deaf.lastHeard == game::Noise::None);
    }
    // A sprint carries 14 m: heard.
    const game::Shade alert = heard(game::Noise::Sprint);
    CHECK(alert.hunt == game::ShadeHunt::Investigating);
    CHECK(alert.goal == noiseCell);
    CHECK(alert.lastHeard == game::Noise::Sprint);
    CHECK(alert.lastHeardCell == noiseCell);
    CHECK(game::shadeState(alert) == game::ShadeState::Investigating);

    // One cell farther along the way (16 m) even the sprint is not heard.
    game::Shade farther = shadeIn({.x = 6, .z = 0});
    CHECK_FALSE(stepUnlit(farther, maze, player, settings, game::Noise::Sprint).alerted);
    CHECK(farther.hunt == game::ShadeHunt::Wandering);
}

TEST_CASE("the shade walks to the cell a noise came from, waits there and gives up") {
    const game::Maze maze = bend();
    const game::ShadeSettings settings;
    const game::MazeCell noiseCell{.x = 5, .z = 1};
    // The player sprints in the noise cell and then stands, silent, around the next
    // corner: no cell of the way of the shade looks into that cell.
    const glm::vec3 hidden = feetIn({.x = 6, .z = 2});
    game::Shade shade = shadeIn({.x = 7, .z = 0});

    // The noise: it is alerted once, in the step it hears it.
    CHECK(stepUnlit(shade, maze, feetIn(noiseCell), settings, game::Noise::Sprint).alerted);
    // It goes on hearing the sprint for a moment: that is no new alert.
    CHECK_FALSE(stepUnlit(shade, maze, feetIn(noiseCell), settings, game::Noise::Sprint).alerted);
    REQUIRE(shade.hunt == game::ShadeHunt::Investigating);

    // It walks the 14 m along the passages at its investigating speed, never across
    // a wall, and stops in the middle of the noise cell.
    game::MazeCell cell = game::cellAt(shade.position);
    int steps = 0;
    while (steps < 30 * STEPS_PER_SECOND &&
           game::shadeDistance(shade, feetIn(noiseCell)) > 0.001F) {
        const game::ShadeEvents events = stepUnlit(shade, maze, hidden, settings);
        REQUIRE_FALSE(events.caught);
        REQUIRE_FALSE(events.alerted);
        const game::MazeCell now = game::cellAt(shade.position);
        REQUIRE(sameOrJoined(maze, cell, now));
        cell = now;
        ++steps;
    }
    CHECK(static_cast<float>(steps) * STEP ==
          doctest::Approx(14.0F / settings.investigateSpeed).epsilon(0.02));
    CHECK(shade.goal == noiseCell);
    CHECK(shade.hunt == game::ShadeHunt::Investigating);

    // It finds nothing. For the search time it stands there and still investigates.
    const glm::vec3 there = shade.position;
    for (int i = 0; i < static_cast<int>(settings.searchSeconds) * STEPS_PER_SECOND - 2; ++i) {
        REQUIRE_FALSE(stepUnlit(shade, maze, hidden, settings).caught);
    }
    CHECK(shade.position == there);
    CHECK(shade.hunt == game::ShadeHunt::Investigating);
    CHECK_FALSE(game::shadeWalking(shade));

    // Then it gives up and wanders again, to a cell of its own choice.
    for (int i = 0; i < 6; ++i) {
        stepUnlit(shade, maze, hidden, settings);
    }
    CHECK(shade.hunt == game::ShadeHunt::Wandering);
    CHECK(game::shadeState(shade) == game::ShadeState::Wandering);
    CHECK_FALSE(shade.goal == noiseCell);
    CHECK(shade.position != there);
}

TEST_CASE("a new noise sends an investigating shade to the new place and restarts its wait") {
    const game::Maze maze = corridor(40);
    game::ShadeSettings settings;
    // Blind, so only the ears count in this straight corridor.
    settings.sightMetres = 0.0F;
    game::Shade shade = shadeIn({.x = 20, .z = 0});

    CHECK(stepUnlit(shade, maze, feetIn({.x = 25, .z = 0}), settings, game::Noise::Sprint).alerted);
    CHECK(shade.goal == game::MazeCell{.x = 25, .z = 0});
    // Half a second later the player sprints on the other side of it.
    for (int i = 0; i < STEPS_PER_SECOND / 2; ++i) {
        stepUnlit(shade, maze, feetIn({.x = 25, .z = 0}), settings);
    }
    shade.searchLeft = 1.0F;
    const game::ShadeEvents again =
        stepUnlit(shade, maze, feetIn({.x = 16, .z = 0}), settings, game::Noise::Sprint);
    // No second alert: it was not wandering.
    CHECK_FALSE(again.alerted);
    CHECK(shade.goal == game::MazeCell{.x = 16, .z = 0});
    CHECK(shade.searchLeft == settings.searchSeconds);
    CHECK(shade.hunt == game::ShadeHunt::Investigating);
}

TEST_CASE("the shade sees down a straight corridor, within its range and not through a wall") {
    const game::Maze maze = corridor(12);
    const glm::vec3 shade = feetIn({.x = 2, .z = 0});
    // The same cell, the next one, and six cells away: 12 m, the edge of the range.
    CHECK(game::shadeSees(maze, shade, shade + glm::vec3{0.3F, 0.0F, 0.2F}, 12.0F));
    CHECK(game::shadeSees(maze, shade, feetIn({.x = 3, .z = 0}), 12.0F));
    CHECK(game::shadeSees(maze, shade, feetIn({.x = 8, .z = 0}), 12.0F));
    // In both directions: it has no back.
    CHECK(game::shadeSees(maze, shade, feetIn({.x = 0, .z = 0}), 12.0F));
    // Seven cells away is out of range, and in range again with longer sight.
    CHECK_FALSE(game::shadeSees(maze, shade, feetIn({.x = 9, .z = 0}), 12.0F));
    CHECK(game::shadeSees(maze, shade, feetIn({.x = 9, .z = 0}), 20.0F));
    // The range is measured between the two, not between the cells.
    CHECK_FALSE(game::shadeSees(maze, shade, feetIn({.x = 8, .z = 0}) + glm::vec3{0.5F, 0.0F, 0.0F},
                                12.0F));
    // Blind.
    CHECK_FALSE(game::shadeSees(maze, shade, feetIn({.x = 3, .z = 0}), 0.0F));

    // One wall anywhere on the line ends the view, for the cells behind it only.
    game::Maze walled(12, 1);
    for (int x = 0; x < 11; ++x) {
        if (x != 4) {
            walled.removeWall(x, 0, game::Direction::East);
        }
    }
    CHECK(game::shadeSees(walled, shade, feetIn({.x = 4, .z = 0}), 12.0F));
    CHECK_FALSE(game::shadeSees(walled, shade, feetIn({.x = 5, .z = 0}), 12.0F));
    CHECK_FALSE(game::shadeSees(walled, feetIn({.x = 5, .z = 0}), shade, 12.0F));

    // Somebody outside the maze is not seen and sees nobody.
    CHECK_FALSE(game::shadeSees(maze, shade, NOWHERE, 1000.0F));
    CHECK_FALSE(game::shadeSees(maze, NOWHERE, shade, 1000.0F));
}

TEST_CASE("the shade does not see around a corner or through the wall between two rows") {
    const game::Maze maze = bend();
    // North to south through the one opening of column 9, and back.
    CHECK(game::shadeSees(maze, feetIn({.x = 9, .z = 0}), feetIn({.x = 9, .z = 1}), 12.0F));
    CHECK(game::shadeSees(maze, feetIn({.x = 9, .z = 1}), feetIn({.x = 9, .z = 0}), 12.0F));
    // Column 7 has a wall between the rows.
    CHECK_FALSE(game::shadeSees(maze, feetIn({.x = 7, .z = 0}), feetIn({.x = 7, .z = 1}), 12.0F));
    // Around the corner: neither the same row nor the same column.
    CHECK_FALSE(game::shadeSees(maze, feetIn({.x = 8, .z = 0}), feetIn({.x = 9, .z = 1}), 12.0F));
    CHECK_FALSE(game::shadeSees(maze, feetIn({.x = 5, .z = 1}), feetIn({.x = 6, .z = 2}), 12.0F));
    // Down the second corridor.
    CHECK(game::shadeSees(maze, feetIn({.x = 9, .z = 1}), feetIn({.x = 5, .z = 1}), 12.0F));
    CHECK(game::shadeSees(maze, feetIn({.x = 5, .z = 1}), feetIn({.x = 5, .z = 2}), 12.0F));
}

TEST_CASE("a shade that sees the player chases, and goes to where it saw them last") {
    const game::Maze maze = bend();
    const game::ShadeSettings settings;
    game::Shade shade = shadeIn({.x = 2, .z = 0});
    const game::MazeCell seenCell{.x = 8, .z = 0};

    // Down the corridor, 12 m away: seen, and that is an alert too.
    const game::ShadeEvents events = stepUnlit(shade, maze, feetIn(seenCell), settings);
    CHECK(events.alerted);
    CHECK(shade.hunt == game::ShadeHunt::Chasing);
    CHECK(shade.goal == seenCell);
    CHECK(game::shadeState(shade) == game::ShadeState::Chasing);
    // Seen, not heard.
    CHECK(shade.lastHeard == game::Noise::None);

    // The player is gone around two corners without a sound. The shade walks at its
    // chase speed to the cell it saw them in, and waits there.
    const glm::vec3 hidden = feetIn({.x = 6, .z = 2});
    const glm::vec3 before = shade.position;
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        REQUIRE_FALSE(stepUnlit(shade, maze, hidden, settings).caught);
    }
    CHECK(game::shadeDistance(shade, before) == doctest::Approx(settings.speed).epsilon(0.01));
    for (int i = 0; i < 3 * STEPS_PER_SECOND; ++i) {
        REQUIRE_FALSE(stepUnlit(shade, maze, hidden, settings).caught);
    }
    CHECK(shade.position == feetIn(seenCell));
    CHECK(shade.hunt == game::ShadeHunt::Chasing);

    // After the search time it wanders again.
    for (int i = 0; i < static_cast<int>(settings.searchSeconds) * STEPS_PER_SECOND; ++i) {
        stepUnlit(shade, maze, hidden, settings);
    }
    CHECK(shade.hunt == game::ShadeHunt::Wandering);
}

TEST_CASE("a shade on a chase that loses sight follows a noise without slowing down") {
    const game::Maze maze = bend();
    const game::ShadeSettings settings;
    game::Shade shade = shadeIn({.x = 5, .z = 0});
    REQUIRE(stepUnlit(shade, maze, feetIn({.x = 8, .z = 0}), settings).alerted);
    REQUIRE(shade.hunt == game::ShadeHunt::Chasing);

    // The player sprints around the corner: not seen from row 0, but heard.
    const game::MazeCell around{.x = 8, .z = 1};
    const game::ShadeEvents events =
        stepUnlit(shade, maze, feetIn(around), settings, game::Noise::Sprint);
    CHECK_FALSE(events.alerted);
    CHECK(shade.hunt == game::ShadeHunt::Chasing);
    CHECK(shade.goal == around);
    CHECK(shade.lastHeard == game::Noise::Sprint);
    CHECK(game::shadeSpeed(shade.hunt, settings) == settings.speed);
}

TEST_CASE("each hunt has its speed: wandering slowest, a chase fastest") {
    game::ShadeSettings settings;
    CHECK(game::shadeSpeed(game::ShadeHunt::Wandering, settings) == settings.wanderSpeed);
    CHECK(game::shadeSpeed(game::ShadeHunt::Investigating, settings) == settings.investigateSpeed);
    CHECK(game::shadeSpeed(game::ShadeHunt::Chasing, settings) == settings.speed);
    settings.wanderSpeed = -3.0F;
    CHECK(game::shadeSpeed(game::ShadeHunt::Wandering, settings) == 0.0F);

    // A wandering shade covers its wander speed in a second, whatever the player does
    // outside the maze.
    const game::ShadeSettings defaults;
    const game::Maze maze = corridor(40);
    game::Shade shade = shadeIn({.x = 0, .z = 0});
    const glm::vec3 before = shade.position;
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        REQUIRE_FALSE(stepInTheDark(shade, maze, NOWHERE, defaults, game::Noise::Sprint));
    }
    CHECK(shade.hunt == game::ShadeHunt::Wandering);
    CHECK(game::shadeDistance(shade, before) ==
          doctest::Approx(defaults.wanderSpeed).epsilon(0.01));
    CHECK(game::shadeWalking(shade));
}

TEST_CASE("a player who stands still and makes no noise is never heard") {
    // Real mazes of the three levels: the player stands at the start for two minutes.
    // The shade leaves its wandering only in a step in which it sees the player.
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        for (std::uint32_t seed = 1; seed <= 3; ++seed) {
            const game::MazeWorld world = worldOf(difficulty, seed);
            game::ShadeSettings settings;
            settings.graceSeconds = 0.0F;
            game::Shade shade = game::startShade(world.maze, seed, game::START_CELL, world.exitCell,
                                                 world.terrain, settings);
            REQUIRE(shade.present);
            const glm::vec3 player = world.startPosition;
            const game::Terrain flat;
            bool everWandered = false;
            for (int i = 0; i < 120 * STEPS_PER_SECOND; ++i) {
                const bool wandering = shade.hunt == game::ShadeHunt::Wandering;
                const bool sees =
                    game::shadeSees(world.maze, shade.position, player, settings.sightMetres);
                const game::ShadeEvents events = game::advanceShade(
                    shade, settings, world.maze, flat, {.playerFeet = player}, STEP);
                if (events.caught) {
                    break;
                }
                REQUIRE(shade.lastHeard == game::Noise::None);
                REQUIRE(shade.hunt != game::ShadeHunt::Investigating);
                if (wandering && !sees) {
                    REQUIRE(shade.hunt == game::ShadeHunt::Wandering);
                    REQUIRE_FALSE(events.alerted);
                }
                everWandered = everWandered || shade.hunt == game::ShadeHunt::Wandering;
            }
            CHECK(everWandered);
        }
    }
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

    // Two seconds in the light: less than it takes to burn it away.
    const game::ShadeStep step{
        .playerFeet = player, .lamp = lampAimedAt(player, shade.position), .obstacles = walls};
    for (int i = 0; i < 2 * STEPS_PER_SECOND; ++i) {
        const game::ShadeEvents events =
            game::advanceShade(shade, settings, maze, flat, step, STEP);
        CHECK_FALSE(events.caught);
        CHECK_FALSE(events.banished);
        // A lit shade is blind: it does not notice the player who holds the lamp.
        CHECK_FALSE(events.alerted);
    }
    CHECK(shade.lit);
    CHECK(game::shadeState(shade) == game::ShadeState::Lit);
    CHECK(shade.hunt == game::ShadeHunt::Wandering);
    CHECK(shade.position == before);

    // A player who walks right up to a lit shade is not caught either.
    const glm::vec3 close = shade.position + glm::vec3{-0.5F, 0.0F, 0.0F};
    const game::ShadeStep touching{
        .playerFeet = close, .lamp = lampAimedAt(close, shade.position), .obstacles = walls};
    CHECK_FALSE(game::advanceShade(shade, settings, maze, flat, touching, STEP).caught);
    CHECK(shade.position == before);
}

TEST_CASE("the burn clock runs up in the light and falls back slowly in the dark") {
    const game::ShadeSettings settings;
    CHECK(game::burnAfter(0.0F, true, settings, 0.5F) == doctest::Approx(0.5F));
    CHECK(game::burnAfter(1.0F, true, settings, 0.5F) == doctest::Approx(1.5F));
    // Dark: half as fast back down, and never below nothing.
    CHECK(game::burnAfter(1.0F, false, settings, 1.0F) ==
          doctest::Approx(1.0F - settings.burnRecoverRate));
    CHECK(game::burnAfter(0.1F, false, settings, 10.0F) == 0.0F);
    CHECK(game::burnAfter(0.0F, false, settings, STEP) == 0.0F);
    // A rate of 0 never forgets, and a negative one counts as 0.
    game::ShadeSettings keeps = settings;
    keeps.burnRecoverRate = 0.0F;
    CHECK(game::burnAfter(1.0F, false, keeps, 60.0F) == 1.0F);
    keeps.burnRecoverRate = -1.0F;
    CHECK(game::burnAfter(1.0F, false, keeps, 60.0F) == 1.0F);
    // The light falls back slower than it burns.
    CHECK(settings.burnRecoverRate < 1.0F);
}

TEST_CASE("the beam burns the shade away: it reappears far from the player and is quiet") {
    const game::Maze maze = corridor(30);
    const std::vector<scene::Aabb> walls = game::mazeColliders(maze);
    const game::Terrain flat;
    const game::ShadeSettings settings;
    const game::MazeCell playerCell{.x = 0, .z = 0};
    const glm::vec3 player = feetIn(playerCell);
    game::Shade shade = shadeIn({.x = 3, .z = 0});
    shade.seed = 5U;
    const glm::vec3 burnedAt = shade.position;

    // The beam is held on it. After 2.5 s it is banished, in one step and only once.
    const game::ShadeStep lit{
        .playerFeet = player, .lamp = lampAimedAt(player, shade.position), .obstacles = walls};
    int steps = 0;
    game::ShadeEvents events;
    while (!events.banished && steps < 10 * STEPS_PER_SECOND) {
        CHECK(shade.burnSeconds == doctest::Approx(static_cast<float>(steps) * STEP));
        events = game::advanceShade(shade, settings, maze, flat, lit, STEP);
        ++steps;
    }
    REQUIRE(events.banished);
    CHECK_FALSE(events.caught);
    CHECK(static_cast<float>(steps) * STEP == doctest::Approx(settings.burnSeconds).epsilon(0.01));

    // It stands in the banish cell: the rule of the start cell, from the player.
    game::MazeCell cell;
    REQUIRE(game::shadeBanishCell(maze, 5U, 0, playerCell, shade.exit, cell));
    CHECK(shade.position == feetIn(cell));
    // The step of the banish is no walk: nothing swoops across the maze, and no step
    // of the shade is heard.
    CHECK(shade.previousPosition == shade.position);
    CHECK_FALSE(game::shadeWalking(shade));
    CHECK(shade.banishCount == 1);
    // Far: at least six tenths of the 29 passages of this corridor.
    CHECK(cell.x >= 18);
    // The figure dissolves where it was burned.
    CHECK(shade.banishedFrom == burnedAt);
    CHECK(shade.dissolveLeft == game::SHADE_DISSOLVE_SECONDS);

    // Quiet: not lit, the clock is empty, nothing is known of the player.
    CHECK(game::shadeState(shade) == game::ShadeState::Banished);
    CHECK(shade.quietLeft == settings.quietSeconds);
    CHECK_FALSE(shade.lit);
    CHECK(shade.burnSeconds == 0.0F);
    CHECK(shade.thawLeft == 0.0F);
    CHECK(shade.hunt == game::ShadeHunt::Wandering);
    CHECK(shade.wayMetres == game::SHADE_NO_WAY);
    CHECK(game::batteryDrainFactor(shade, settings) == 1.0F);

    // For the quiet time it stands, hears no sprint next to it, sees nobody in its own
    // cell, is not lit by a lamp in its face and catches nobody.
    const glm::vec3 quietAt = shade.position;
    const glm::vec3 beside = quietAt + glm::vec3{-0.5F, 0.0F, 0.0F};
    const game::ShadeStep loud{.playerFeet = beside,
                               .lamp = lampAimedAt(beside, quietAt),
                               .obstacles = walls,
                               .noise = game::Noise::Sprint};
    for (int i = 0; i < static_cast<int>(settings.quietSeconds) * STEPS_PER_SECOND - 2; ++i) {
        events = game::advanceShade(shade, settings, maze, flat, loud, STEP);
        REQUIRE_FALSE(events.caught);
        REQUIRE_FALSE(events.alerted);
        REQUIRE_FALSE(events.banished);
        REQUIRE_FALSE(shade.lit);
    }
    CHECK(shade.position == quietAt);
    CHECK(shade.lastHeard == game::Noise::None);
    CHECK(shade.burnSeconds == 0.0F);
    CHECK(shade.wayMetres == game::SHADE_NO_WAY);
    CHECK(shade.dissolveLeft == 0.0F);

    // Then it is back: left alone it wanders off.
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        stepUnlit(shade, maze, NOWHERE, settings);
    }
    CHECK(game::shadeState(shade) == game::ShadeState::Wandering);
    CHECK(shade.position != quietAt);
}

TEST_CASE("the burn does not have to be one unbroken look") {
    const game::Maze maze = corridor(30);
    const std::vector<scene::Aabb> walls = game::mazeColliders(maze);
    const game::Terrain flat;
    const game::ShadeSettings settings;
    const glm::vec3 player = feetIn({.x = 0, .z = 0});
    game::Shade shade = shadeIn({.x = 3, .z = 0});
    const game::ShadeStep lit{
        .playerFeet = player, .lamp = lampAimedAt(player, shade.position), .obstacles = walls};
    const game::ShadeStep dark{.playerFeet = player, .obstacles = walls};

    // A second and a half of light, then one second without. In that second it still
    // stands (the wait after the light), and the clock falls back by half a second.
    for (int i = 0; i < 3 * STEPS_PER_SECOND / 2; ++i) {
        REQUIRE_FALSE(game::advanceShade(shade, settings, maze, flat, lit, STEP).banished);
    }
    CHECK(shade.burnSeconds == doctest::Approx(1.5F).epsilon(0.01));
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        REQUIRE_FALSE(game::advanceShade(shade, settings, maze, flat, dark, STEP).banished);
    }
    CHECK(shade.burnSeconds == doctest::Approx(1.0F).epsilon(0.01));
    CHECK(game::shadeState(shade) == game::ShadeState::Thawing);
    CHECK(shade.position == feetIn({.x = 3, .z = 0}));

    // The light again: one and a half seconds more are enough, not two and a half.
    int steps = 0;
    while (steps < 10 * STEPS_PER_SECOND &&
           !game::advanceShade(shade, settings, maze, flat, lit, STEP).banished) {
        ++steps;
    }
    CHECK(static_cast<float>(steps) * STEP == doctest::Approx(1.5F).epsilon(0.02));
}

TEST_CASE("the beam on the shade drains the battery three times as fast") {
    const game::ShadeSettings settings;
    game::Shade shade;
    // No shade, an unlit one, a lit one.
    CHECK(game::batteryDrainFactor(shade, settings) == 1.0F);
    shade.present = true;
    CHECK(game::batteryDrainFactor(shade, settings) == 1.0F);
    shade.lit = true;
    CHECK(game::batteryDrainFactor(shade, settings) == 3.0F);
    // The shade is never a way to save light.
    game::ShadeSettings cheap = settings;
    cheap.burnBatteryFactor = 0.2F;
    CHECK(game::batteryDrainFactor(shade, cheap) == 1.0F);

    // In a round: one second of light costs one second of battery, and three with the
    // beam on the shade.
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 1U);
    const game::GameplaySettings gameplay;
    const auto drainedIn = [&](bool lit) {
        game::Round round = game::startRound(world, gameplay);
        round.shade.lit = lit;
        bool flashlightOn = true;
        for (int i = 0; i < STEPS_PER_SECOND; ++i) {
            game::updateRound(round, world, gameplay, world.startPosition, flashlightOn, STEP);
        }
        return 1.0F - round.battery;
    };
    CHECK(drainedIn(false) ==
          doctest::Approx(1.0F / gameplay.batteryLifetimeSeconds).epsilon(0.01));
    CHECK(drainedIn(true) == doctest::Approx(3.0F / gameplay.batteryLifetimeSeconds).epsilon(0.01));
}

TEST_CASE("a banish costs a small part of the battery on every level and every night") {
    const game::ShadeSettings settings;
    // The whole burn in one look: burnSeconds at three times the usual drain.
    const auto cost = [&](float batteryLifetimeSeconds) {
        return settings.burnSeconds * settings.burnBatteryFactor / batteryLifetimeSeconds;
    };
    // Easy, normal, hard: 4.2 %, 5.0 %, 6.3 %.
    CHECK(cost(game::difficultyLevel(game::Difficulty::Easy).batteryLifetimeSeconds) ==
          doctest::Approx(0.0417F).epsilon(0.01));
    CHECK(cost(game::difficultyLevel(game::Difficulty::Normal).batteryLifetimeSeconds) ==
          doctest::Approx(0.05F).epsilon(0.01));
    CHECK(cost(game::difficultyLevel(game::Difficulty::Hard).batteryLifetimeSeconds) ==
          doctest::Approx(0.0625F).epsilon(0.01));
    // Nights 2 to 5: 4.5 %, 5.0 %, 5.6 %, 6.3 %.
    CHECK(cost(game::campaignNight(2).batteryLifetimeSeconds) ==
          doctest::Approx(0.0455F).epsilon(0.01));
    CHECK(cost(game::campaignNight(4).batteryLifetimeSeconds) ==
          doctest::Approx(0.0556F).epsilon(0.01));
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        // Noticeable, and small: a crystal gives back a quarter of a battery.
        CHECK(cost(game::campaignNight(night).batteryLifetimeSeconds) > 0.03F);
        CHECK(cost(game::campaignNight(night).batteryLifetimeSeconds) < 0.07F);
    }

    // The same number comes out of a round that is played: the lamp on the shade until
    // it is gone, less what the lamp costs anyway in that time.
    const game::MazeWorld world = worldOf(game::Difficulty::Hard, 1U);
    game::GameplaySettings gameplay;
    gameplay.batteryLifetimeSeconds =
        game::difficultyLevel(game::Difficulty::Hard).batteryLifetimeSeconds;
    gameplay.shade.graceSeconds = 0.0F;
    game::Round round = game::startRound(world, gameplay);
    REQUIRE(round.shade.present);
    // The player stands on the spot of the shade, the lamp at its chest.
    const glm::vec3 feet = round.shade.position + glm::vec3{0.4F, 0.0F, 0.0F};
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    bool flashlightOn = true;
    bool banished = false;
    int steps = 0;
    while (!banished && steps < 10 * STEPS_PER_SECOND) {
        game::updateRound(round, world, gameplay, feet, flashlightOn, STEP);
        banished = game::updateRoundShade(round, world, gameplay, feet,
                                          lampAimedAt(feet, round.shade.position), obstacles, STEP)
                       .banished;
        ++steps;
    }
    REQUIRE(banished);
    CHECK(1.0F - round.battery == doctest::Approx(0.0625F).epsilon(0.02));
}

TEST_CASE("a shade that sees the player walks at its chase speed and catches") {
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
    CHECK(shade.hunt == game::ShadeHunt::Chasing);
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

    // Two steps per turn of the loop: just under the two seconds in all. It neither
    // sees the player next to it nor hears a sprint.
    for (int i = 0; i < STEPS_PER_SECOND - 1; ++i) {
        REQUIRE_FALSE(stepUnlit(shade, maze, player, settings, game::Noise::Sprint).alerted);
        REQUIRE_FALSE(stepInTheDark(shade, maze, before, settings));
    }
    CHECK(shade.position == before);
    CHECK(game::shadeState(shade) == game::ShadeState::Grace);
    CHECK(shade.hunt == game::ShadeHunt::Wandering);

    // After the grace time it sees the player, and reaches them half a cell away.
    bool caught = false;
    for (int i = 0; i < STEPS_PER_SECOND && !caught; ++i) {
        caught = stepInTheDark(shade, maze, player, settings);
    }
    CHECK(caught);
    CHECK(shade.position != before);
}

TEST_CASE("the shade follows the passages to a noise and never crosses a wall") {
    // Generated mazes of the three levels, the shade from its own start cell, the
    // player sprinting on the spot at the start of the maze. With ears for the whole
    // maze, and every speed the same, the time tells that it took the shortest way.
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        for (std::uint32_t seed = 1; seed <= 3; ++seed) {
            const game::MazeWorld world = worldOf(difficulty, seed);
            const game::Maze& maze = world.maze;
            game::ShadeSettings settings;
            settings.graceSeconds = 0.0F;
            settings.hearSprintMetres = HEARS_EVERYTHING;
            settings.investigateSpeed = settings.speed;
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
                caught = stepInTheDark(shade, maze, player, settings, game::Noise::Sprint);
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

    // After a player it sees in the east for a quarter of a second.
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

TEST_CASE("the ways are searched again only when a cell changes or a wall opens") {
    const game::Maze maze = corridor(8);
    game::Shade shade = shadeIn({.x = 6, .z = 0});
    const glm::vec3 player = feetIn({.x = 0, .z = 0});

    REQUIRE_FALSE(stepInTheDark(shade, maze, player));
    CHECK(shade.playerCell == game::MazeCell{.x = 0, .z = 0});
    CHECK(shade.playerOpenings == 0);
    // It sees the player, so its goal is their cell, and the way to it is searched too.
    CHECK(shade.pathGoal == game::MazeCell{.x = 0, .z = 0});
    CHECK(shade.pathOpenings == 0);

    // A mark in both kept lists: it stays while nothing changes, also when the player
    // moves inside the cell.
    shade.playerDistances[7] = 77;
    shade.goalDistances[7] = 77;
    REQUIRE_FALSE(stepInTheDark(shade, maze, player + glm::vec3{0.4F, 0.0F, 0.3F}));
    CHECK(shade.playerDistances[7] == 77);
    CHECK(shade.goalDistances[7] == 77);

    // Another cell: searched again.
    REQUIRE_FALSE(stepInTheDark(shade, maze, feetIn({.x = 1, .z = 0})));
    CHECK(shade.playerDistances[7] == 6);
    CHECK(shade.goalDistances[7] == 6);
    CHECK(shade.playerCell == game::MazeCell{.x = 1, .z = 0});
    CHECK(shade.pathGoal == game::MazeCell{.x = 1, .z = 0});

    // A lever was pulled: searched again.
    shade.playerDistances[7] = 77;
    shade.goalDistances[7] = 77;
    REQUIRE_FALSE(stepInTheDark(shade, maze, feetIn({.x = 1, .z = 0}), {}, game::Noise::None, 1));
    CHECK(shade.playerDistances[7] == 6);
    CHECK(shade.goalDistances[7] == 6);
    CHECK(shade.playerOpenings == 1);
    CHECK(shade.pathOpenings == 1);
}

TEST_CASE("a player outside the maze or behind closed walls is not hunted") {
    game::ShadeSettings settings;
    settings.hearSprintMetres = HEARS_EVERYTHING;
    SUBCASE("outside the maze: not heard, not seen, not caught, and nothing throws") {
        const game::Maze maze = corridor(6);
        game::Shade shade = shadeIn({.x = 3, .z = 0});
        for (int i = 0; i < 5 * STEPS_PER_SECOND; ++i) {
            const game::ShadeEvents events =
                stepUnlit(shade, maze, NOWHERE, settings, game::Noise::Sprint);
            CHECK_FALSE(events.caught);
            CHECK_FALSE(events.alerted);
        }
        CHECK(shade.hunt == game::ShadeHunt::Wandering);
        CHECK(shade.wayMetres == game::SHADE_NO_WAY);
        // It wanders by itself meanwhile, inside the maze.
        CHECK(maze.contains(game::cellAt(shade.position).x, game::cellAt(shade.position).z));
    }
    SUBCASE("no way to the player: nothing is heard, and the shade stays in its cell") {
        // Every wall stands: no cell leads anywhere.
        const game::Maze maze(4, 1);
        game::Shade shade = shadeIn({.x = 3, .z = 0});
        const glm::vec3 before = shade.position;
        for (int i = 0; i < STEPS_PER_SECOND; ++i) {
            const game::ShadeEvents events =
                stepUnlit(shade, maze, feetIn({.x = 0, .z = 0}), settings, game::Noise::Sprint);
            CHECK_FALSE(events.caught);
            CHECK_FALSE(events.alerted);
        }
        CHECK(shade.position == before);
        CHECK(shade.wayMetres == game::SHADE_NO_WAY);
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
            // It starts with nothing to hunt, and knows what its choices are drawn from.
            CHECK(a.hunt == game::ShadeHunt::Wandering);
            CHECK(a.seed == seed);
            CHECK(a.exit == first.exitCell);
            CHECK(a.burnSeconds == 0.0F);
            CHECK(a.quietLeft == 0.0F);
            cells.insert(a.target.z * first.maze.width() + a.target.x);
        }
    }
    // Not one fixed cell for every maze.
    CHECK(cells.size() > 10U);
}

TEST_CASE("the cells a shade wanders to are always far, reachable and never the exit") {
    // The three levels and the five nights, ten seeds each: a chain of thirty choices
    // from the start cell of the shade, each one made from the cell before.
    for (const game::MazeWorld& world : sweepWorlds(10U)) {
        game::MazeCell from;
        REQUIRE(
            game::shadeStartCell(world.maze, world.seed, game::START_CELL, world.exitCell, from));
        std::set<int> goals;
        for (int pick = 0; pick < 30; ++pick) {
            game::MazeCell cell{.x = -1, .z = -1};
            REQUIRE(
                game::shadeWanderCell(world.maze, world.seed, pick, from, world.exitCell, cell));
            REQUIRE(isFarCell(world, from, cell, game::SHADE_WANDER_FAR_TENTHS));
            goals.insert(cell.z * world.maze.width() + cell.x);
            from = cell;
        }
        // Many different cells, not two corners it paces between.
        CHECK(goals.size() >= 10U);
    }
}

TEST_CASE("a wandering shade covers the maze and never crosses a wall or enters the exit") {
    // Left to itself for ten minutes on each level and on a night of each size.
    for (const game::MazeWorld& world : sweepWorlds(1U)) {
        const game::Maze& maze = world.maze;
        game::ShadeSettings settings;
        settings.graceSeconds = 0.0F;
        game::Shade shade = game::startShade(maze, world.seed, game::START_CELL, world.exitCell,
                                             world.terrain, settings);
        REQUIRE(shade.present);
        std::set<int> visited;
        game::MazeCell cell = game::cellAt(shade.position);
        for (int i = 0; i < 600 * STEPS_PER_SECOND; ++i) {
            const game::ShadeEvents events = game::advanceShade(
                shade, settings, maze, world.terrain, {.playerFeet = NOWHERE}, STEP);
            REQUIRE_FALSE(events.caught);
            REQUIRE_FALSE(events.alerted);
            const game::MazeCell now = game::cellAt(shade.position);
            REQUIRE(maze.contains(now.x, now.z));
            REQUIRE(sameOrJoined(maze, cell, now));
            REQUIRE_FALSE(now == world.exitCell);
            cell = now;
            visited.insert(now.z * maze.width() + now.x);
        }
        CHECK(shade.hunt == game::ShadeHunt::Wandering);
        CHECK(shade.wanderCount >= 3);
        // 1200 m of walking: a good part of the maze has been walked through.
        const int cells = maze.width() * maze.height();
        CHECK(static_cast<int>(visited.size()) * 4 >= cells);
    }
}

TEST_CASE("the cell a banished shade reappears in is always far from the player") {
    // The three levels and the five nights, five seeds each, the player in every cell
    // but the exit, and two banishes in a row.
    for (const game::MazeWorld& world : sweepWorlds(5U)) {
        const game::Maze& maze = world.maze;
        for (int z = 0; z < maze.height(); ++z) {
            for (int x = 0; x < maze.width(); ++x) {
                const game::MazeCell playerCell{.x = x, .z = z};
                if (playerCell == world.exitCell) {
                    continue;
                }
                for (int count = 0; count < 2; ++count) {
                    game::MazeCell cell{.x = -1, .z = -1};
                    REQUIRE(game::shadeBanishCell(maze, world.seed, count, playerCell,
                                                  world.exitCell, cell));
                    REQUIRE(isFarCell(world, playerCell, cell, game::SHADE_START_FAR_TENTHS));
                }
            }
        }
    }
}

TEST_CASE("the choices of the shade follow from the seed and their number alone") {
    const game::MazeWorld world = worldOf(game::Difficulty::Normal, 4U);
    const game::MazeCell from{.x = 3, .z = 3};
    std::set<int> wanderCells;
    std::set<int> banishCells;
    for (int number = 0; number < 20; ++number) {
        game::MazeCell a;
        game::MazeCell b;
        REQUIRE(game::shadeWanderCell(world.maze, 4U, number, from, world.exitCell, a));
        REQUIRE(game::shadeWanderCell(world.maze, 4U, number, from, world.exitCell, b));
        CHECK(a == b);
        wanderCells.insert(a.z * world.maze.width() + a.x);
        REQUIRE(game::shadeBanishCell(world.maze, 4U, number, from, world.exitCell, a));
        REQUIRE(game::shadeBanishCell(world.maze, 4U, number, from, world.exitCell, b));
        CHECK(a == b);
        banishCells.insert(a.z * world.maze.width() + a.x);
    }
    // Another number is another draw: not the same cell every time.
    CHECK(wanderCells.size() > 5U);
    CHECK(banishCells.size() > 5U);

    // A maze of one room has nowhere to go.
    const game::Maze one(1, 1);
    game::MazeCell cell{.x = 7, .z = 7};
    CHECK_FALSE(game::shadeWanderCell(one, 1U, 0, {.x = 0, .z = 0}, {.x = 0, .z = 0}, cell));
    CHECK_FALSE(game::shadeBanishCell(one, 1U, 0, {.x = 0, .z = 0}, {.x = 0, .z = 0}, cell));
    CHECK(cell == game::MazeCell{.x = 7, .z = 7});
    CHECK_THROWS_AS(game::shadeWanderCell(one, 1U, 0, {.x = 5, .z = 0}, {.x = 0, .z = 0}, cell),
                    std::out_of_range);
}

TEST_CASE("the same seed and level play the same night of the shade, another seed another") {
    // A minute of a round on each level, twice: the player stands at the start and
    // sprints on the spot every ten seconds, with a lamp that points at a wall.
    std::set<int> firstGoals;
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        for (std::uint32_t seed = 1; seed <= 4; ++seed) {
            const game::MazeWorld world = worldOf(difficulty, seed);
            const game::GameplaySettings gameplay;
            const auto play = [&]() {
                game::Round round = game::startRound(world, gameplay);
                const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
                for (int i = 0; i < 60 * STEPS_PER_SECOND; ++i) {
                    const game::Noise noise = i % (10 * STEPS_PER_SECOND) < STEPS_PER_SECOND
                                                  ? game::Noise::Sprint
                                                  : game::Noise::None;
                    if (game::updateRoundShade(round, world, gameplay, world.startPosition, {},
                                               obstacles, STEP, noise)
                            .caught) {
                        break;
                    }
                }
                return round.shade;
            };
            const game::Shade a = play();
            const game::Shade b = play();
            CHECK(a.position == b.position);
            CHECK(a.goal == b.goal);
            CHECK(a.hunt == b.hunt);
            CHECK(a.wanderCount == b.wanderCount);
            CHECK(a.wanderCount >= 1);

            // The first cell it wanders to, for the check below.
            game::MazeCell start;
            game::MazeCell goal;
            REQUIRE(
                game::shadeStartCell(world.maze, seed, game::START_CELL, world.exitCell, start));
            REQUIRE(game::shadeWanderCell(world.maze, seed, 0, start, world.exitCell, goal));
            firstGoals.insert(goal.z * 100 + goal.x);
        }
    }
    // Twelve mazes do not all send it to the same cell.
    CHECK(firstGoals.size() > 6U);
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

    // Nothing ever catches the player, wherever the player stands, however long and
    // however loud, and nothing is ever heard of a shade.
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    for (int i = 0; i < 60 * STEPS_PER_SECOND; ++i) {
        const game::ShadeEvents events = game::updateRoundShade(
            round, world, gameplay, world.startPosition, {}, obstacles, STEP, game::Noise::Sprint);
        CHECK_FALSE(events.caught);
        CHECK_FALSE(events.alerted);
        CHECK_FALSE(events.banished);
    }
    CHECK_FALSE(round.shade.present);
}

TEST_CASE("the first night of the campaign has no shade to hear or to burn") {
    const game::MazeWorld world = worldOfNight(1, 1U);
    game::GameplaySettings gameplay;
    gameplay.shade.enabled = game::campaignNight(1).shade;
    game::Round round = game::startRound(world, gameplay);
    CHECK_FALSE(round.shade.present);
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    for (int i = 0; i < 20 * STEPS_PER_SECOND; ++i) {
        const game::ShadeEvents events = game::updateRoundShade(
            round, world, gameplay, world.startPosition, {}, obstacles, STEP, game::Noise::Sprint);
        CHECK_FALSE(events.alerted);
        CHECK_FALSE(events.banished);
    }
    CHECK(game::batteryDrainFactor(round.shade, gameplay.shade) == 1.0F);
}

TEST_CASE("switching the shade off in a round removes it at once") {
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 2U);
    game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    REQUIRE(round.shade.present);
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);

    gameplay.shade.enabled = false;
    CHECK_FALSE(
        game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP)
            .caught);
    CHECK_FALSE(round.shade.present);
}

TEST_CASE("a round catches a player who keeps sprinting on the spot, after the grace time") {
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::MazeWorld world = worldOf(difficulty, 1U);
        game::GameplaySettings gameplay;
        // Ears for the whole maze: the shade starts far from the start on every level.
        gameplay.shade.hearSprintMetres = HEARS_EVERYTHING;
        game::Round round = game::startRound(world, gameplay);
        REQUIRE(round.shade.present);
        const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
        const float startDistance = game::shadeDistance(round.shade, world.startPosition);

        int steps = 0;
        int alerts = 0;
        bool caught = false;
        while (!caught && steps < 600 * STEPS_PER_SECOND) {
            const game::ShadeEvents events =
                game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles,
                                       STEP, game::Noise::Sprint);
            caught = events.caught;
            alerts += events.alerted ? 1 : 0;
            ++steps;
            if (steps == static_cast<int>(gameplay.shade.graceSeconds) * STEPS_PER_SECOND - 1) {
                // Still in the grace time: it has not moved and has heard nothing.
                CHECK(game::shadeDistance(round.shade, world.startPosition) == startDistance);
                CHECK(alerts == 0);
            }
        }
        REQUIRE(caught);
        CHECK(static_cast<float>(steps) * STEP > gameplay.shade.graceSeconds);
        // It noticed the player once, and never lost them.
        CHECK(alerts == 1);
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
        const game::ShadeEvents events = game::updateRoundShade(
            round, world, gameplay, before, {}, obstacles, STEP, game::Noise::Sprint);
        CHECK_FALSE(events.caught);
        CHECK_FALSE(events.alerted);
    }
    CHECK(round.shade.position == before);
}

TEST_CASE("a wall sunk by a lever shortens the way of the shade") {
    // A maze with a lever: its wall stands between a near and a far cell, at least seven
    // passages apart (LEVER_MIN_STEPS_SAVED). The shade stands on one side and the
    // player, who keeps making noise, on the other.
    const game::MazeWorld world = worldOf(game::Difficulty::Normal, 1U);
    REQUIRE_FALSE(world.interactables.levers.empty());
    const game::WallRef wall = world.interactables.levers[0].opens;
    const game::MazeCell here = wall.cell;
    const game::MazeCell behind{.x = here.x + game::columnStep(wall.side),
                                .z = here.z + game::rowStep(wall.side)};
    game::GameplaySettings gameplay;
    gameplay.shade.graceSeconds = 0.0F;
    gameplay.shade.hearSprintMetres = HEARS_EVERYTHING;
    gameplay.shade.investigateSpeed = gameplay.shade.speed;

    // How many steps the shade needs from `here` to a player in `behind`.
    const auto stepsToCatch = [&](bool leverPulled) {
        game::Round round = game::startRound(world, gameplay);
        round.shade = shadeIn(here);
        if (leverPulled) {
            REQUIRE(game::pullRoundLever(round, world, 0));
        }
        const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
        int steps = 0;
        while (steps < 600 * STEPS_PER_SECOND &&
               !game::updateRoundShade(round, world, gameplay, feetIn(behind), {}, obstacles, STEP,
                                       game::Noise::Sprint)
                    .caught) {
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
    gameplay.shade.hearSprintMetres = HEARS_EVERYTHING;
    game::Round round = game::startRound(world, gameplay);
    round.shade = shadeIn(here);

    // One step the long way round, then the lever: the next step heads for the opening.
    std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    REQUIRE_FALSE(game::updateRoundShade(round, world, gameplay, feetIn(behind), {}, obstacles,
                                         STEP, game::Noise::Sprint)
                      .caught);
    CHECK(round.shade.goal == behind);
    CHECK_FALSE(round.shade.target == behind);
    REQUIRE(game::pullRoundLever(round, world, 0));
    obstacles = game::roundObstacles(world, round);
    REQUIRE_FALSE(game::updateRoundShade(round, world, gameplay, feetIn(behind), {}, obstacles,
                                         STEP, game::Noise::Lever)
                      .caught);
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
    REQUIRE(round.shade.wanderCount > 0);

    round = game::startRound(world, gameplay);
    CHECK(round.shade.position == start);
    CHECK(round.shade.wanderCount == 0);
    CHECK(round.shade.banishCount == 0);
    CHECK(round.shade.hunt == game::ShadeHunt::Wandering);
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

TEST_CASE("the picture, and with it the wind, follows the catch fade") {
    const game::MazeWorld world = worldOf(game::Difficulty::Easy, 1U);
    const game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    // No catch: full.
    CHECK(game::pictureBrightness(-1.0F, round) == 1.0F);
    // A catch: down with the fade, black at its end.
    CHECK(game::pictureBrightness(0.0F, round) == 1.0F);
    CHECK(game::pictureBrightness(game::CATCH_FADE_OUT_SECONDS * 0.5F, round) ==
          doctest::Approx(0.5F));
    CHECK(game::pictureBrightness(game::CATCH_FADE_OUT_SECONDS, round) == 0.0F);
    // After the carry back: up again with the picture.
    game::showCaughtLine(round, 1);
    CHECK(game::pictureBrightness(-1.0F, round) == 0.0F);
    round.caughtSeconds = game::CAUGHT_FADE_SECONDS;
    CHECK(game::pictureBrightness(-1.0F, round) == 1.0F);
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

TEST_CASE("a catch fades the picture to black, then the round starts again") {
    CHECK(game::catchFadeBrightness(0.0F) == 1.0F);
    CHECK(game::catchFadeBrightness(game::CATCH_FADE_OUT_SECONDS) == 0.0F);
    CHECK(game::catchFadeBrightness(60.0F) == 0.0F);
    CHECK(game::catchFadeBrightness(-1.0F) == 1.0F);
    CHECK(game::catchFadeBrightness(game::CATCH_FADE_OUT_SECONDS / 2.0F) == doctest::Approx(0.5F));
    float before = 1.0F;
    for (int i = 1; i <= 6; ++i) {
        const float now = game::catchFadeBrightness(static_cast<float>(i) * 0.1F - 0.01F);
        CHECK(now < before);
        before = now;
    }
    CHECK(game::catchPhase(0.0F) == game::CatchPhase::FadingOut);
    CHECK(game::catchPhase(game::CATCH_FADE_OUT_SECONDS - 0.01F) == game::CatchPhase::FadingOut);
    CHECK(game::catchPhase(game::CATCH_FADE_OUT_SECONDS) == game::CatchPhase::Black);
}

TEST_CASE("a standing shade sways slowly and stays within its small numbers") {
    const game::ShadeSwaySettings sway;
    float maxSide = 0.0F;
    float minRise = 1.0F;
    float maxRise = 0.0F;
    for (int i = 0; i < 900; ++i) {
        const game::ShadePose pose =
            game::shadeSwayPose(0.0F, 4.0F, static_cast<float>(i) * 0.01F, sway);
        CHECK(pose.forwardLeanDegrees == 0.0F);
        maxSide = std::max(maxSide, std::abs(pose.sideLeanDegrees));
        minRise = std::min(minRise, pose.riseMetres);
        maxRise = std::max(maxRise, pose.riseMetres);
    }
    CHECK(maxSide == doctest::Approx(sway.standLeanDegrees).epsilon(0.01));
    CHECK(minRise >= -1.0e-6F);
    CHECK(maxRise == doctest::Approx(sway.standRiseMetres).epsilon(0.01));
    // It repeats after one period, and it is at rest-lean 0 at time 0.
    const game::ShadePose a = game::shadeSwayPose(0.0F, 4.0F, 1.0F, sway);
    const game::ShadePose b = game::shadeSwayPose(0.0F, 4.0F, 1.0F + sway.periodSeconds, sway);
    CHECK(a.sideLeanDegrees == doctest::Approx(b.sideLeanDegrees).epsilon(0.001));
    CHECK(game::shadeSwayPose(0.0F, 4.0F, 0.0F, sway).sideLeanDegrees == 0.0F);
}

TEST_CASE("a walking shade leans forward and bobs in step with its speed") {
    const game::ShadeSwaySettings sway;
    const game::ShadePose walking = game::shadeSwayPose(1.0F, 4.0F, 0.3F, sway);
    CHECK(walking.forwardLeanDegrees == doctest::Approx(sway.walkLeanDegrees));
    CHECK(walking.sideLeanDegrees == 0.0F);
    float maxBob = 0.0F;
    for (int i = 0; i < 300; ++i) {
        maxBob = std::max(
            maxBob,
            game::shadeSwayPose(1.0F, 4.0F, static_cast<float>(i) * 0.01F, sway).riseMetres);
    }
    CHECK(maxBob == doctest::Approx(sway.walkBobMetres).epsilon(0.01));
    // One bump per step: the bob is back at the same height after one stride of walking.
    const float stride = game::SHADE_STRIDE_METRES / 4.0F;
    CHECK(game::shadeSwayPose(1.0F, 4.0F, 0.2F, sway).riseMetres ==
          doctest::Approx(game::shadeSwayPose(1.0F, 4.0F, 0.2F + stride, sway).riseMetres)
              .epsilon(0.001));
    // Halfway between the poses is halfway between the numbers, and speed 0 does not bob.
    CHECK(game::shadeSwayPose(0.5F, 4.0F, 0.3F, sway).forwardLeanDegrees ==
          doctest::Approx(sway.walkLeanDegrees / 2.0F));
    CHECK(game::shadeSwayPose(1.0F, 0.0F, 5.0F, sway).riseMetres == 0.0F);
}

TEST_CASE("the shade counts as walking only in a step in which its feet moved") {
    game::Shade shade;
    CHECK_FALSE(game::shadeWalking(shade));
    shade.present = true;
    // Standing: lit, waiting, searching at its goal, quiet. Both positions are the same.
    CHECK_FALSE(game::shadeWalking(shade));
    shade.position = {0.5F, 0.0F, 0.0F};
    CHECK(game::shadeWalking(shade));
    // Only the ground counts: a rise of the terrain under a standing shade is no walk.
    shade.position = {0.0F, 0.3F, 0.0F};
    CHECK_FALSE(game::shadeWalking(shade));
    // No shade walks in a round without one.
    shade.position = {0.5F, 0.0F, 0.0F};
    shade.present = false;
    CHECK_FALSE(game::shadeWalking(shade));
}

TEST_CASE("the state of the shade is the first rule that holds it") {
    game::Shade shade;
    shade.present = true;
    CHECK(game::shadeState(shade) == game::ShadeState::Wandering);
    shade.hunt = game::ShadeHunt::Investigating;
    CHECK(game::shadeState(shade) == game::ShadeState::Investigating);
    shade.hunt = game::ShadeHunt::Chasing;
    CHECK(game::shadeState(shade) == game::ShadeState::Chasing);
    // What holds it comes before what it is after.
    shade.thawLeft = 0.5F;
    CHECK(game::shadeState(shade) == game::ShadeState::Thawing);
    shade.lit = true;
    CHECK(game::shadeState(shade) == game::ShadeState::Lit);
    shade.graceLeft = 1.0F;
    CHECK(game::shadeState(shade) == game::ShadeState::Grace);
    shade.quietLeft = 1.0F;
    CHECK(game::shadeState(shade) == game::ShadeState::Banished);

    // Every state has a name of its own for the debug window.
    std::set<std::string> names;
    for (const game::ShadeState state :
         {game::ShadeState::Banished, game::ShadeState::Grace, game::ShadeState::Lit,
          game::ShadeState::Thawing, game::ShadeState::Chasing, game::ShadeState::Investigating,
          game::ShadeState::Wandering}) {
        names.insert(game::shadeStateName(state));
    }
    CHECK(names.size() == 7U);
    CHECK(std::string(game::shadeStateName(game::ShadeState::Wandering)) == "wandering");
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

TEST_CASE("a shade that does not know of the player looks the way it walks") {
    const game::Maze maze = corridor(40);
    const game::ShadeSettings settings;
    // It wanders east from the west end, and the player is outside the maze.
    game::Shade shade = shadeIn({.x = 0, .z = 0});
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        stepUnlit(shade, maze, NOWHERE, settings);
    }
    REQUIRE(shade.hunt == game::ShadeHunt::Wandering);
    CHECK(shade.headingDegrees == doctest::Approx(90.0F));
    // A player in the south-west is not looked at.
    const glm::vec3 player = shade.position + glm::vec3{-3.0F, 0.0F, 3.0F};
    CHECK(game::shadeFacingDegrees(shade, player) == doctest::Approx(90.0F));
    // Investigating, it still looks where it goes.
    shade.hunt = game::ShadeHunt::Investigating;
    CHECK(game::shadeFacingDegrees(shade, player) == doctest::Approx(90.0F));

    // Chasing, lit, or just out of the light: it knows where the player is.
    const float towardsPlayer = game::shadeYawDegrees(shade.position, player);
    shade.hunt = game::ShadeHunt::Chasing;
    CHECK(game::shadeFacingDegrees(shade, player) == doctest::Approx(towardsPlayer));
    shade.hunt = game::ShadeHunt::Wandering;
    shade.lit = true;
    CHECK(game::shadeFacingDegrees(shade, player) == doctest::Approx(towardsPlayer));
    shade.lit = false;
    shade.thawLeft = 1.0F;
    CHECK(game::shadeFacingDegrees(shade, player) == doctest::Approx(towardsPlayer));
}

TEST_CASE("a banished figure sinks away smoothly") {
    CHECK(game::shadeDissolveHeight(game::SHADE_DISSOLVE_SECONDS) == 1.0F);
    CHECK(game::shadeDissolveHeight(0.0F) == 0.0F);
    CHECK(game::shadeDissolveHeight(-1.0F) == 0.0F);
    CHECK(game::shadeDissolveHeight(60.0F) == 1.0F);
    CHECK(game::shadeDissolveHeight(game::SHADE_DISSOLVE_SECONDS / 2.0F) == doctest::Approx(0.5F));
    float before = 1.0F;
    for (int i = 1; i <= 7; ++i) {
        const float now =
            game::shadeDissolveHeight(game::SHADE_DISSOLVE_SECONDS - static_cast<float>(i) * 0.1F);
        CHECK(now < before);
        before = now;
    }
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
    REQUIRE_FALSE(game::advanceShade(shade, settings, maze, flat, lit, STEP).caught);
    REQUIRE(shade.lit);

    // The lamp is switched off. For two seconds nothing moves, and a player who walks
    // right through the shade in that time is not caught. It sees the player again at
    // once, but that does not move it yet.
    for (int i = 0; i < 2 * STEPS_PER_SECOND - 1; ++i) {
        REQUIRE_FALSE(stepInTheDark(shade, maze, i % 2 == 0 ? player : before, settings));
    }
    CHECK_FALSE(shade.lit);
    CHECK(shade.position == before);
    CHECK(shade.hunt == game::ShadeHunt::Chasing);

    // Then it comes: 6 m less the catch distance at 4 m/s.
    int steps = 0;
    while (steps < 10 * STEPS_PER_SECOND && !stepInTheDark(shade, maze, player, settings)) {
        ++steps;
    }
    CHECK(static_cast<float>(steps) * STEP == doctest::Approx(1.275F).epsilon(0.05));

    // Without the wait it walks in the very step after the light.
    settings.thawSeconds = 0.0F;
    game::Shade eager = shadeIn({.x = 3, .z = 0});
    REQUIRE_FALSE(game::advanceShade(eager, settings, maze, flat, lit, STEP).caught);
    REQUIRE_FALSE(stepInTheDark(eager, maze, player, settings));
    CHECK(eager.position != before);
}

TEST_CASE("sprinting away from a shade that saw you works, walking away does not") {
    // A long straight corridor, the shade behind the player, nobody shines at it.
    const game::Maze maze = corridor(200);
    const game::ShadeSettings settings;
    const game::StaminaSettings staminaSettings;

    // What happens in 30 seconds to a player who runs east and holds the sprint key or
    // not. Returns the gap at the end, or a negative number when the shade caught up.
    // The player starts in cell 10 and the shade in the cell shadeColumn: every cell
    // between them is 2 m. The shade hears the feet of the player like in the game.
    const auto flee = [&](bool holdsSprint, game::Stamina stamina, int shadeColumn) {
        glm::vec3 player = feetIn({.x = 10, .z = 0});
        game::Shade shade = shadeIn({.x = shadeColumn, .z = 0});
        for (int i = 0; i < 30 * STEPS_PER_SECOND; ++i) {
            const bool sprints = game::advanceStamina(stamina, staminaSettings, holdsSprint, STEP);
            player.x += (sprints ? game::Player::SPRINT_SPEED : game::Player::WALK_SPEED) * STEP;
            const game::Noise noise = sprints ? game::Noise::Sprint : game::Noise::Walk;
            if (stepInTheDark(shade, maze, player, settings, noise)) {
                return -1.0F;
            }
        }
        return game::shadeDistance(shade, player);
    };

    // With the sprint key held the stamina runs out and comes back in turns, and over
    // those turns the player is a little faster than the shade: never caught, and
    // farther away than at the start.
    CHECK(flee(true, game::Stamina{}, 7) > 6.0F);
    // Walking away from a shade that sees you loses a metre every second: caught.
    CHECK(flee(false, game::Stamina{}, 7) < 0.0F);
    // A shade 20 m behind has neither seen nor heard the player: walking is enough,
    // because it only wanders, and slower than the player walks.
    CHECK(flee(false, game::Stamina{}, 0) > 20.0F);
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
        REQUIRE_FALSE(
            game::updateRoundShade(round, world, gameplay, feet, {}, obstacles, STEP).caught);
    }
    CHECK(round.shade.position == feet);
    CHECK(round.shade.wayMetres == 0.0F);
    CHECK(std::isfinite(game::shadeYawDegrees(round.shade.position, feet)));

    // The grace time is over: it sees the player in its own cell and has them within
    // the next second.
    bool caught = false;
    for (int i = 0; i < STEPS_PER_SECOND && !caught; ++i) {
        caught = game::updateRoundShade(round, world, gameplay, feet, {}, obstacles, STEP).caught;
    }
    CHECK(caught);
}

TEST_CASE("the loudness mark shows what the shade hears: one tick for a walk, three for a sprint") {
    const game::ShadeSettings settings;
    const auto ticksOf = [&settings](game::Noise noise) {
        return game::noiseTicks(game::noiseReach(noise, settings), settings);
    };
    CHECK(ticksOf(game::Noise::None) == 0);
    CHECK(ticksOf(game::Noise::Walk) == 1);
    CHECK(ticksOf(game::Noise::Pickup) == 2);
    CHECK(ticksOf(game::Noise::Lever) == 2);
    CHECK(ticksOf(game::Noise::Sprint) == game::NOISE_TICK_COUNT);
}

TEST_CASE("the ticks follow the hearing distances of the settings") {
    // A shade that hears a walk as far as a sprint: both are as loud as it gets.
    game::ShadeSettings keen;
    keen.hearWalkMetres = keen.hearSprintMetres;
    CHECK(game::noiseTicks(game::noiseReach(game::Noise::Walk, keen), keen) ==
          game::NOISE_TICK_COUNT);

    // A noise that carries a hand's breadth is still a noise: never 0 ticks.
    CHECK(game::noiseTicks(0.1F, game::ShadeSettings{}) == 1);
    // Farther than a sprint carries is not louder than the loudest.
    CHECK(game::noiseTicks(1000.0F, game::ShadeSettings{}) == game::NOISE_TICK_COUNT);
    CHECK(game::noiseTicks(-1.0F, game::ShadeSettings{}) == 0);

    // A deaf shade hears nothing, so nothing is shown.
    game::ShadeSettings deaf;
    deaf.hearWalkMetres = 0.0F;
    deaf.hearPickupMetres = 0.0F;
    deaf.hearLeverMetres = 0.0F;
    deaf.hearSprintMetres = 0.0F;
    for (const game::Noise noise : {game::Noise::None, game::Noise::Walk, game::Noise::Pickup,
                                    game::Noise::Lever, game::Noise::Sprint}) {
        CHECK(game::noiseTicks(game::noiseReach(noise, deaf), deaf) == 0);
    }
    // Only the sprint switched off: what is still heard has no scale, and counts as loud.
    game::ShadeSettings noSprint;
    noSprint.hearSprintMetres = 0.0F;
    CHECK(game::noiseTicks(game::noiseReach(game::Noise::Walk, noSprint), noSprint) ==
          game::NOISE_TICK_COUNT);
}

TEST_CASE("the noise meter shows a walk and a sprint only for as long as they last") {
    const game::ShadeSettings settings;
    game::NoiseMeter meter;
    CHECK(meter.noise == game::Noise::None);

    game::advanceNoiseMeter(meter, game::Noise::Walk, settings, STEP);
    CHECK(meter.noise == game::Noise::Walk);
    game::advanceNoiseMeter(meter, game::Noise::Sprint, settings, STEP);
    CHECK(meter.noise == game::Noise::Sprint);
    // The player stops: silent in the very next step.
    game::advanceNoiseMeter(meter, game::Noise::None, settings, STEP);
    CHECK(meter.noise == game::Noise::None);
    CHECK(meter.holdLeft == 0.0F);
}

TEST_CASE("the noise meter holds a pickup and a lever for a moment") {
    const game::ShadeSettings settings;
    for (const game::Noise flash : {game::Noise::Pickup, game::Noise::Lever}) {
        game::NoiseMeter meter;
        // One step of the noise, then the player walks on.
        game::advanceNoiseMeter(meter, flash, settings, STEP);
        CHECK(meter.noise == flash);
        const int held = static_cast<int>(game::NOISE_FLASH_SECONDS * STEPS_PER_SECOND);
        for (int i = 0; i < held - 1; ++i) {
            game::advanceNoiseMeter(meter, game::Noise::Walk, settings, STEP);
            CHECK(meter.noise == flash);
        }
        // The time is over: back to what the player does now.
        for (int i = 0; i < 2; ++i) {
            game::advanceNoiseMeter(meter, game::Noise::Walk, settings, STEP);
        }
        CHECK(meter.noise == game::Noise::Walk);
    }
}

TEST_CASE("a louder noise takes the place of a held one at once") {
    const game::ShadeSettings settings;
    game::NoiseMeter meter;
    game::advanceNoiseMeter(meter, game::Noise::Pickup, settings, STEP);
    game::advanceNoiseMeter(meter, game::Noise::Sprint, settings, STEP);
    CHECK(meter.noise == game::Noise::Sprint);
    // And a sprint is not held: the player stops, the meter is silent.
    game::advanceNoiseMeter(meter, game::Noise::None, settings, STEP);
    CHECK(meter.noise == game::Noise::None);
}

TEST_CASE("the round shows on its meter the noise it hands to the shade") {
    const game::MazeWorld world = worldOf(game::Difficulty::Normal, 3U);
    const game::GameplaySettings gameplay;
    game::Round round = game::startRound(world, gameplay);
    REQUIRE(round.shade.present);
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    CHECK(round.noiseMeter.noise == game::Noise::None);

    game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP,
                           game::Noise::Sprint);
    CHECK(round.noiseMeter.noise == game::Noise::Sprint);
    game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP,
                           game::Noise::Walk);
    CHECK(round.noiseMeter.noise == game::Noise::Walk);
    game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP);
    CHECK(round.noiseMeter.noise == game::Noise::None);

    // A round that is won is silent, whatever the player still does.
    game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP,
                           game::Noise::Sprint);
    round.state = game::RoundState::Won;
    game::updateRoundShade(round, world, gameplay, world.startPosition, {}, obstacles, STEP,
                           game::Noise::Sprint);
    CHECK(round.noiseMeter.noise == game::Noise::None);
}
