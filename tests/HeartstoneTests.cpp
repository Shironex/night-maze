// Tests of the heartstone: which dead end it floats in (game/Heartstone.hpp), what it
// moved in the world of a seed (game/MazeWorld.hpp), what taking it does to a round
// (game/Round.hpp), to the stamina (game/Player.hpp), to the sounds (game/SoundCues.hpp)
// and to the HUD (debug/HudRules.hpp).
#include "game/Heartstone.hpp"

#include "debug/HudRules.hpp"
#include "game/Crystals.hpp"
#include "game/Difficulty.hpp"
#include "game/Exit.hpp"
#include "game/Flasks.hpp"
#include "game/Interactables.hpp"
#include "game/Maze.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Puddles.hpp"
#include "game/Round.hpp"
#include "game/Settings.hpp"
#include "game/SoundCues.hpp"
#include "game/Terrain.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float. 120 steps are one second.
constexpr float STEP = 1.0F / 120.0F;
constexpr int STEPS_PER_SECOND = 120;

// A place far away from everything in the mazes used here.
constexpr glm::vec3 NOWHERE{-50.0F, 0.0F, -50.0F};

// The sides of the square mazes that are tried: tiny ones, the three levels (10, 16, 22)
// and the odd sizes in between, 13 and 19.
constexpr std::array<int, 9> SIZES = {3, 4, 5, 8, 10, 13, 16, 19, 22};

constexpr std::uint32_t WORLD_SEEDS = 40;

bool contains(const std::vector<game::MazeCell>& cells, game::MazeCell cell) {
    return std::ranges::find(cells, cell) != cells.end();
}

std::vector<game::MazeCell> cellsOf(const std::vector<game::CrystalSpawn>& crystals) {
    std::vector<game::MazeCell> cells;
    cells.reserve(crystals.size());
    for (const game::CrystalSpawn& crystal : crystals) {
        cells.push_back(crystal.cell);
    }
    return cells;
}

int distanceTo(const game::Maze& maze, const std::vector<int>& distances, game::MazeCell cell) {
    return distances[static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
                     static_cast<std::size_t>(cell.x)];
}

// The cell on the open side of the exit: the one in front of the gate.
std::optional<game::MazeCell> cellBeforeExit(const game::Maze& maze, game::MazeCell exit) {
    for (const game::Direction side : game::ALL_DIRECTIONS) {
        if (!maze.hasWall(exit.x, exit.z, side)) {
            return game::MazeCell{.x = exit.x + game::columnStep(side),
                                  .z = exit.z + game::rowStep(side)};
        }
    }
    return std::nullopt;
}

// Where the feet have to be for the reach of the player to be centred on the heartstone.
glm::vec3 feetAtHeartstone(const game::MazeWorld& world) {
    const game::MazeCell cell = *world.heartstone;
    return game::heartstoneCenter(
               game::heartstoneRestPosition(cell, game::groundHeightAt(world, cell))) -
           glm::vec3{0.0F, game::PLAYER_REACH_HEIGHT, 0.0F};
}

// A world of one of the three levels.
game::MazeWorld levelWorld(game::Difficulty difficulty, std::uint32_t seed) {
    const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
    return game::buildMazeWorld(level.mazeWidth, level.mazeHeight, seed, {}, level.crystalCount);
}

// Runs the stamina for some seconds with the sprint key held or not. Returns how many
// of the steps were sprinted.
int runStamina(game::Stamina& stamina, const game::StaminaSettings& settings, bool wantsSprint,
               float seconds) {
    int sprinted = 0;
    const auto steps = std::lround(seconds * static_cast<float>(STEPS_PER_SECOND));
    for (long i = 0; i < steps; ++i) {
        if (game::advanceStamina(stamina, settings, wantsSprint, STEP)) {
            ++sprinted;
        }
    }
    return sprinted;
}

} // namespace

// ---- where it floats ---------------------------------------------------------------------

TEST_CASE("the heartstone floats in the farthest dead end that is not the exit") {
    for (const int size : SIZES) {
        for (std::uint32_t seed = 1; seed <= 150; ++seed) {
            CAPTURE(size);
            CAPTURE(seed);
            const game::Maze maze = game::generateMaze(size, size, seed);
            const game::MazeCell exit = game::placeExit(maze, game::START_CELL).cell;
            const std::vector<int> distances = game::passageDistances(maze, game::START_CELL);
            const std::optional<game::MazeCell> heartstone =
                game::heartstoneCell(maze, game::START_CELL, exit);

            // The dead ends it may take, and the farthest of them.
            int farthest = 0;
            for (int z = 0; z < size; ++z) {
                for (int x = 0; x < size; ++x) {
                    const game::MazeCell cell{.x = x, .z = z};
                    if (cell != game::START_CELL && cell != exit && game::isDeadEnd(maze, x, z)) {
                        farthest = std::max(farthest, distanceTo(maze, distances, cell));
                    }
                }
            }
            // A maze with a dead end to spare has a heartstone, and no other maze has.
            REQUIRE(heartstone.has_value() == (farthest > 0));
            if (!heartstone) {
                continue;
            }
            CHECK(game::isDeadEnd(maze, heartstone->x, heartstone->z));
            CHECK(*heartstone != game::START_CELL);
            CHECK(*heartstone != exit);
            CHECK(heartstone != cellBeforeExit(maze, exit));
            CHECK(distanceTo(maze, distances, *heartstone) == farthest);
            // The exit is the farthest cell of all, so the heartstone is never beyond it.
            CHECK(farthest <= distanceTo(maze, distances, exit));
            // No random number is drawn: asked again, it is the same cell.
            CHECK(game::heartstoneCell(maze, game::START_CELL, exit) == heartstone);
        }
    }
}

TEST_CASE("a maze without a dead end to spare has no heartstone") {
    // One cell, and corridors: the far end is the exit, and nothing else is a dead end.
    for (const auto& [width, height] : {std::pair{1, 1}, std::pair{2, 1}, std::pair{1, 5}}) {
        CAPTURE(width);
        CAPTURE(height);
        const game::MazeWorld world = game::buildMazeWorld(width, height, 7);
        CHECK_FALSE(world.heartstone.has_value());
        const game::Round round = game::startRound(world, {});
        CHECK_FALSE(round.hasHeartstone);
        CHECK(game::crystalTotal(round) == static_cast<int>(round.crystals.size()));
        CHECK_FALSE(game::heartstoneBase(world, round).has_value());
    }
}

TEST_CASE("of two dead ends equally far the first one in row order holds the heartstone") {
    // A T of five cells: the start in the middle of the top row, a stem down to the
    // exit, and one dead end to each side of the start, both one passage away.
    game::Maze maze(3, 3);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::South);
    maze.removeWall(1, 1, game::Direction::South);
    const game::MazeCell start{.x = 1, .z = 0};
    const game::MazeCell exit{.x = 1, .z = 2};
    CHECK(game::heartstoneCell(maze, start, exit) == game::MazeCell{.x = 0, .z = 0});
}

TEST_CASE("the heartstone floats low, is 0.9 m tall and turns slower than a crystal") {
    CHECK(game::HEARTSTONE_WORTH == 3);
    CHECK(game::HEARTSTONE_HEIGHT == doctest::Approx(0.9F));
    CHECK(game::HEARTSTONE_FLOAT_HEIGHT < game::CRYSTAL_FLOAT_HEIGHT);
    CHECK(game::HEARTSTONE_SPIN_DEGREES_PER_SECOND < game::CRYSTAL_SPIN_DEGREES_PER_SECOND);
    CHECK(game::HEARTSTONE_GLOW_FACTOR > 1.0F);

    const game::MazeCell cell{.x = 2, .z = 3};
    const glm::vec3 rest = game::heartstoneRestPosition(cell, 1.5F);
    const glm::vec3 centre = game::cellCenter(cell.x, cell.z);
    CHECK(rest.x == doctest::Approx(centre.x));
    CHECK(rest.z == doctest::Approx(centre.z));
    CHECK(rest.y == doctest::Approx(1.5F + game::HEARTSTONE_FLOAT_HEIGHT));
    CHECK(game::heartstoneCenter(rest).y == doctest::Approx(rest.y + 0.45F));
    // The light hangs above the tip, outside the mesh.
    CHECK(game::heartstoneLightPosition(rest).y > rest.y + game::HEARTSTONE_HEIGHT);

    CHECK(game::heartstoneSpinDegrees(0.0F) == doctest::Approx(0.0F));
    CHECK(game::heartstoneSpinDegrees(10.0F) == doctest::Approx(180.0F));
    for (const float seconds : {0.0F, 19.9F, 20.1F, 1234.5F}) {
        CHECK(game::heartstoneSpinDegrees(seconds) >= 0.0F);
        CHECK(game::heartstoneSpinDegrees(seconds) < 360.0F);
    }
}

// ---- what it moved in the world of a seed ------------------------------------------------

TEST_CASE("the heartstone moves at most two crystals of a seed and nothing else") {
    int worlds = 0;
    int worldsWithMovedCrystal = 0;
    int movedCrystals = 0;
    for (const int size : SIZES) {
        for (std::uint32_t seed = 1; seed <= WORLD_SEEDS; ++seed) {
            CAPTURE(size);
            CAPTURE(seed);
            const game::MazeWorld world = game::buildMazeWorld(size, size, seed);
            const game::Maze& maze = world.maze;

            // seedCrystals is what the world had before there was a heartstone: the
            // first dead ends of the flasks reserved, and nothing kept free.
            std::vector<game::MazeCell> reserved =
                game::flaskDeadEnds(maze, seed, game::START_CELL, world.exitCell);
            reserved.resize(std::min(reserved.size(),
                                     static_cast<std::size_t>(game::FLASK_RESERVED_DEAD_ENDS)));
            const std::vector<game::CrystalSpawn> before =
                game::placeCrystals(maze, seed, game::START_CELL, world.exitCell,
                                    game::CRYSTAL_COUNT_FROM_SIZE, reserved);
            REQUIRE(world.seedCrystals.size() == before.size());
            for (std::size_t i = 0; i < before.size(); ++i) {
                CHECK(world.seedCrystals[i].cell == before[i].cell);
                CHECK(world.seedCrystals[i].variant == before[i].variant);
            }

            // The levers and the notes are the ones placed around those crystals, wall
            // for wall, so the looks of the walls (which follow them) are the same too.
            const game::Interactables old =
                game::placeInteractables(maze, seed, game::START_CELL, world.exitCell, before, {});
            REQUIRE(world.interactables.levers.size() == old.levers.size());
            for (std::size_t i = 0; i < old.levers.size(); ++i) {
                CHECK(world.interactables.levers[i].mount == old.levers[i].mount);
                CHECK(world.interactables.levers[i].opens == old.levers[i].opens);
            }
            REQUIRE(world.interactables.notes.size() == old.notes.size());
            for (std::size_t i = 0; i < old.notes.size(); ++i) {
                CHECK(world.interactables.notes[i].mount == old.notes[i].mount);
                CHECK(world.interactables.notes[i].kind == old.notes[i].kind);
            }
            CHECK(world.wallVariants ==
                  game::chooseWallVariants(maze, seed, game::START_CELL, old));

            if (!world.heartstone) {
                // No heartstone: the crystals are exactly the ones of the seed.
                CHECK(cellsOf(world.crystals) == cellsOf(before));
                continue;
            }
            ++worlds;

            // Every crystal that stayed keeps its model, and the ones that stayed are in
            // the order they had.
            const std::vector<game::MazeCell> oldCells = cellsOf(before);
            const std::vector<game::MazeCell> newCells = cellsOf(world.crystals);
            int moved = 0;
            std::vector<game::MazeCell> stayed;
            for (const game::CrystalSpawn& crystal : before) {
                const auto found = std::ranges::find(newCells, crystal.cell);
                if (found == newCells.end()) {
                    ++moved;
                    continue;
                }
                stayed.push_back(crystal.cell);
                CHECK(world.crystals[static_cast<std::size_t>(found - newCells.begin())].variant ==
                      crystal.variant);
            }
            std::vector<game::MazeCell> stayedNow;
            for (const game::MazeCell cell : newCells) {
                if (contains(oldCells, cell)) {
                    stayedNow.push_back(cell);
                }
            }
            CHECK(stayed == stayedNow);
            CHECK(moved <= 2);
            movedCrystals += moved;
            worldsWithMovedCrystal += moved > 0 ? 1 : 0;

            // No crystal in the dead end of the heartstone.
            CHECK_FALSE(contains(newCells, *world.heartstone));
            // A maze with room keeps the number of its crystals. Only one with fewer free
            // cells than crystals can lose one, and from 5 by 5 on none is that small.
            if (size >= 5) {
                CHECK(world.crystals.size() == before.size());
            }
        }
    }
    // For the decision record: how often a crystal had to make room.
    MESSAGE("heartstone: ", worldsWithMovedCrystal, " of ", worlds, " worlds moved a crystal (",
            movedCrystals, " crystals in all)");
}

TEST_CASE("the flasks keep out of the dead end of the heartstone and stay in dead ends") {
    int rounds = 0;
    int roundsWithMovedFlask = 0;
    int worldsWithMovedCrystal = 0;
    for (const game::Difficulty difficulty :
         {game::Difficulty::Easy, game::Difficulty::Normal, game::Difficulty::Hard}) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        for (std::uint32_t seed = 1; seed <= WORLD_SEEDS; ++seed) {
            CAPTURE(level.mazeWidth);
            CAPTURE(seed);
            const game::MazeWorld world = levelWorld(difficulty, seed);
            REQUIRE(world.heartstone.has_value());
            CHECK(world.crystals.size() == static_cast<std::size_t>(level.crystalCount));

            game::GameplaySettings settings;
            settings.flaskCount = level.flaskCount;
            const game::Round round = game::startRound(world, settings);
            REQUIRE(round.flasks.size() == static_cast<std::size_t>(level.flaskCount));
            const std::vector<game::MazeCell> crystalCells = cellsOf(world.crystals);
            for (const game::RoundFlask& flask : round.flasks) {
                CHECK(flask.cell != *world.heartstone);
                CHECK(game::isDeadEnd(world.maze, flask.cell.x, flask.cell.z));
                CHECK_FALSE(contains(crystalCells, flask.cell));
            }

            // What the flasks of this level were before the heartstone: the first dead
            // ends of their order that held no crystal of the seed.
            const std::vector<game::MazeCell> before =
                game::placeFlasks(world.maze, seed, game::START_CELL, world.exitCell,
                                  world.seedCrystals, level.flaskCount);
            int moved = 0;
            for (const game::RoundFlask& flask : round.flasks) {
                moved += contains(before, flask.cell) ? 0 : 1;
            }
            // Only the flask that lay in the dead end of the heartstone moves.
            CHECK(moved <= 1);
            CHECK((moved == 1) == contains(before, *world.heartstone));
            ++rounds;
            roundsWithMovedFlask += moved;
            const bool crystalMoved =
                std::ranges::any_of(world.seedCrystals, [&](const game::CrystalSpawn& crystal) {
                    return !contains(crystalCells, crystal.cell);
                });
            worldsWithMovedCrystal += crystalMoved ? 1 : 0;
        }
    }
    // For the decision record, over the three levels.
    MESSAGE("heartstone: ", roundsWithMovedFlask, " of ", rounds, " rounds moved a flask, ",
            worldsWithMovedCrystal, " moved a crystal");
}

TEST_CASE("more flasks than a level asks for never lie on the heartstone either") {
    const game::MazeWorld world = levelWorld(game::Difficulty::Easy, 5);
    REQUIRE(world.heartstone.has_value());
    game::GameplaySettings settings;
    settings.flaskCount = game::MAX_FLASK_COUNT;
    for (const game::RoundFlask& flask : game::startRound(world, settings).flasks) {
        CHECK(flask.cell != *world.heartstone);
    }
}

TEST_CASE("no puddle lies under the heartstone, and at most two puddles of a seed are gone") {
    for (std::uint32_t seed = 1; seed <= WORLD_SEEDS; ++seed) {
        CAPTURE(seed);
        const game::MazeWorld world = game::buildMazeWorld(10, 10, seed);
        REQUIRE(world.heartstone.has_value());
        // Every free cell of the seed gets a puddle: the strictest case.
        const std::vector<game::PuddleSpawn> chosen = game::placePuddles(
            world.maze, seed, game::START_CELL, world.exitCell, world.seedCrystals, 1.0F);
        const std::vector<game::Puddle> puddles = game::puddlesOnGround(world, 1.0F);
        CHECK(puddles.size() <= chosen.size());
        CHECK(puddles.size() + 2U >= chosen.size());

        // The puddles that are left are the chosen ones, in their order, without the
        // ones under the heartstone and under the crystals of the world.
        const std::vector<game::MazeCell> crystalCells = cellsOf(world.crystals);
        std::size_t next = 0;
        for (const game::PuddleSpawn& spawn : chosen) {
            if (spawn.cell == *world.heartstone || contains(crystalCells, spawn.cell)) {
                continue;
            }
            REQUIRE(next < puddles.size());
            const glm::vec3 centre = game::cellCenter(spawn.cell.x, spawn.cell.z);
            CHECK(puddles[next].center.x == doctest::Approx(centre.x + spawn.offset.x));
            CHECK(puddles[next].center.z == doctest::Approx(centre.z + spawn.offset.y));
            ++next;
        }
        CHECK(next == puddles.size());
    }
}

// ---- what taking it does to a round ------------------------------------------------------

TEST_CASE("the heartstone counts as three, fills the battery and never raises the threshold") {
    for (const game::Difficulty difficulty :
         {game::Difficulty::Easy, game::Difficulty::Normal, game::Difficulty::Hard}) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        const game::MazeWorld world = levelWorld(difficulty, 3);
        REQUIRE(world.heartstone.has_value());
        game::GameplaySettings settings;
        settings.requiredFraction = level.requiredFraction;
        game::Round round = game::startRound(world, settings);

        // The number that opens the gate is counted from the ordinary crystals alone,
        // as it was before there was a heartstone. "Of N" grows by three.
        const int needed = game::requiredCrystalCount(level.crystalCount, level.requiredFraction);
        CHECK(round.hasHeartstone);
        CHECK_FALSE(round.heartstoneTaken);
        CHECK(round.requiredCount == needed);
        CHECK(game::crystalTotal(round) == level.crystalCount + game::HEARTSTONE_WORTH);
        CHECK(game::heartstoneBase(world, round).has_value());

        // Far away nothing is taken, and the light drains.
        bool flashlightOn = true;
        round.battery = 0.2F;
        game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
        CHECK(round.collectedCount == 0);
        CHECK(round.battery < 0.2F);

        game::updateRound(round, world, settings, feetAtHeartstone(world), flashlightOn, STEP);
        CHECK(round.heartstoneTaken);
        CHECK(round.collectedCount == game::HEARTSTONE_WORTH);
        CHECK(round.battery == doctest::Approx(1.0F));
        CHECK(round.requiredCount == needed);
        CHECK(game::crystalTotal(round) == level.crystalCount + game::HEARTSTONE_WORTH);
        CHECK_FALSE(game::heartstoneBase(world, round).has_value());
        CHECK_FALSE(round.gateOpen);

        // It is taken once: standing there longer adds nothing.
        game::updateRound(round, world, settings, feetAtHeartstone(world), flashlightOn, STEP);
        CHECK(round.collectedCount == game::HEARTSTONE_WORTH);
    }
}

TEST_CASE("the heartstone is a shortcut worth three crystals at the gate") {
    const game::MazeWorld world = levelWorld(game::Difficulty::Easy, 3);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    // Three crystals short of the gate, then the heartstone: the gate opens.
    int taken = 0;
    for (const game::RoundCrystal& crystal : round.crystals) {
        if (taken + game::HEARTSTONE_WORTH >= round.requiredCount) {
            break;
        }
        const glm::vec3 feet =
            crystal.restPosition - glm::vec3{0.0F, game::PLAYER_REACH_HEIGHT, 0.0F};
        game::updateRound(round, world, settings, feet, flashlightOn, 0.0F);
        taken = round.collectedCount;
    }
    REQUIRE(round.collectedCount == round.requiredCount - game::HEARTSTONE_WORTH);
    CHECK_FALSE(round.gateOpen);
    game::updateRound(round, world, settings, feetAtHeartstone(world), flashlightOn, 0.0F);
    CHECK(round.gateOpen);
}

TEST_CASE("a player who took every crystal and the heartstone left nothing behind") {
    const game::MazeWorld world = levelWorld(game::Difficulty::Easy, 9);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;
    // A copy of the places: the step changes the crystals it walks over.
    const std::vector<game::RoundCrystal> crystals = round.crystals;
    for (const game::RoundCrystal& crystal : crystals) {
        const glm::vec3 feet =
            crystal.restPosition - glm::vec3{0.0F, game::PLAYER_REACH_HEIGHT, 0.0F};
        game::updateRound(round, world, settings, feet, flashlightOn, 0.0F);
    }
    // Every ordinary crystal, and still not everything: the ending card says so.
    CHECK(round.collectedCount == static_cast<int>(crystals.size()));
    CHECK(round.collectedCount < game::crystalTotal(round));
    game::updateRound(round, world, settings, feetAtHeartstone(world), flashlightOn, 0.0F);
    CHECK(round.collectedCount == game::crystalTotal(round));
}

TEST_CASE("a new round puts the heartstone back") {
    const game::MazeWorld world = levelWorld(game::Difficulty::Easy, 3);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;
    game::updateRound(round, world, settings, feetAtHeartstone(world), flashlightOn, STEP);
    REQUIRE(round.heartstoneTaken);
    round = game::startRound(world, settings);
    CHECK_FALSE(round.heartstoneTaken);
    CHECK(round.collectedCount == 0);
}

TEST_CASE("the heartstone has no light among the crystal lights and no cell among the hints") {
    const game::MazeWorld world = levelWorld(game::Difficulty::Easy, 3);
    const game::Round round = game::startRound(world, {});
    // One light and one hint target per ordinary crystal: the heartstone is a find.
    CHECK(game::crystalLightPositions(round).size() == round.crystals.size());
    const std::vector<game::MazeCell> hinted = game::remainingCrystalCells(world, round);
    CHECK(hinted.size() == round.crystals.size());
    CHECK_FALSE(contains(hinted, *world.heartstone));
}

// ---- the sound ---------------------------------------------------------------------------

TEST_CASE("taking the heartstone plays its own sound and not the one of a crystal") {
    const game::MazeWorld world = levelWorld(game::Difficulty::Easy, 3);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    bool flashlightOn = false;

    const game::RoundSoundSnapshot before = game::soundSnapshot(round, flashlightOn);
    CHECK_FALSE(before.heartstoneTaken);
    game::updateRound(round, world, settings, feetAtHeartstone(world), flashlightOn, STEP);
    const std::vector<game::SoundCue> cues = game::roundStepCues(before, round, flashlightOn);
    CHECK(cues == std::vector<game::SoundCue>{game::SoundCue::HeartstonePickup});

    // The step after it is silent, and a crystal after it sounds like a crystal.
    const game::RoundSoundSnapshot after = game::soundSnapshot(round, flashlightOn);
    game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    CHECK(game::roundStepCues(after, round, flashlightOn).empty());
    const glm::vec3 feet =
        round.crystals[0].restPosition - glm::vec3{0.0F, game::PLAYER_REACH_HEIGHT, 0.0F};
    game::updateRound(round, world, settings, feet, flashlightOn, STEP);
    CHECK(game::roundStepCues(after, round, flashlightOn) ==
          std::vector<game::SoundCue>{game::SoundCue::CrystalPickup});
    CHECK(std::string_view(game::soundCueFile(game::SoundCue::HeartstonePickup)) ==
          "audio/heartstone_pickup.wav");
}

// ---- heavy -------------------------------------------------------------------------------

TEST_CASE("a new stamina is not heavy, and the weight lasts 30 seconds") {
    CHECK(game::StaminaSettings{}.heavySeconds == doctest::Approx(30.0F));
    CHECK(game::Stamina{}.heavySecondsLeft == doctest::Approx(0.0F));
    CHECK(game::heavyFraction(game::Stamina{}, {}) == doctest::Approx(0.0F));
}

TEST_CASE("a heavy player cannot sprint for 30 seconds and is not winded") {
    const game::StaminaSettings settings;
    game::Stamina stamina;
    // Half a bar, to see that it goes on refilling.
    stamina.level = 0.5F;
    stamina.secondsSinceDrain = settings.refillDelaySeconds;
    game::carryHeartstone(stamina, settings);
    CHECK(stamina.heavySecondsLeft == doctest::Approx(30.0F));
    CHECK(game::heavyFraction(stamina, settings) == doctest::Approx(1.0F));
    CHECK(game::staminaBarVisible(stamina));

    // 29.5 seconds with the sprint key held: no step is sprinted, the bar refills and
    // nobody is winded.
    CHECK(runStamina(stamina, settings, true, 29.5F) == 0);
    CHECK_FALSE(stamina.winded);
    CHECK(stamina.level == doctest::Approx(1.0F));
    CHECK(stamina.heavySecondsLeft == doctest::Approx(0.5F).epsilon(0.01));
    CHECK(game::heavyFraction(stamina, settings) > 0.0F);
    CHECK(game::staminaBarVisible(stamina));

    // After the 30 seconds the sprint is back.
    runStamina(stamina, settings, false, 0.6F);
    CHECK(stamina.heavySecondsLeft == doctest::Approx(0.0F));
    CHECK(runStamina(stamina, settings, true, 1.0F) == STEPS_PER_SECOND);
}

TEST_CASE("a heavy player walks as fast as ever with the sprint key held") {
    const game::Terrain terrain;
    game::Player player;
    game::carryHeartstone(player.stamina, player.staminaSettings);
    game::PlayerInput input;
    input.forward = true;
    input.sprint = true;
    for (int i = 0; i < STEPS_PER_SECOND; ++i) {
        player.update(input, 0.0F, 0.0F, STEP, {}, terrain);
    }
    CHECK(glm::length(player.position) == doctest::Approx(game::Player::WALK_SPEED).epsilon(0.001));
}

TEST_CASE("a flask drunk while heavy does not lift the weight, and its tea starts after") {
    const game::StaminaSettings settings;
    game::Stamina stamina;
    game::carryHeartstone(stamina, settings);
    runStamina(stamina, settings, false, 10.0F);

    game::drinkFlask(stamina, settings);
    CHECK(stamina.heavySecondsLeft == doctest::Approx(20.0F).epsilon(0.001));
    CHECK(stamina.noDrainSecondsLeft == doctest::Approx(settings.flaskSeconds));

    // The 20 heavy seconds that are left: no sprint, and the tea does not run.
    CHECK(runStamina(stamina, settings, true, 19.5F) == 0);
    CHECK(stamina.noDrainSecondsLeft == doctest::Approx(settings.flaskSeconds));
    runStamina(stamina, settings, false, 0.6F);
    REQUIRE(stamina.heavySecondsLeft == doctest::Approx(0.0F));

    // Now the tea works, for all but the tenth of a second that just ran: the sprint is
    // free.
    CHECK(stamina.noDrainSecondsLeft > settings.flaskSeconds - 0.2F);
    CHECK(runStamina(stamina, settings, true, 5.0F) == 5 * STEPS_PER_SECOND);
    CHECK(stamina.level == doctest::Approx(1.0F));
}

TEST_CASE("the heartstone taken while the tea works stops the sprint and keeps the tea") {
    const game::StaminaSettings settings;
    game::Stamina stamina;
    game::drinkFlask(stamina, settings);
    runStamina(stamina, settings, true, 5.0F);
    const float teaLeft = stamina.noDrainSecondsLeft;
    CHECK(teaLeft == doctest::Approx(15.0F).epsilon(0.001));

    game::carryHeartstone(stamina, settings);
    CHECK(runStamina(stamina, settings, true, 29.0F) == 0);
    CHECK(stamina.noDrainSecondsLeft == doctest::Approx(teaLeft));
}

TEST_CASE("the two clocks of the stamina never run at once and never below 0") {
    game::Stamina stamina;
    stamina.heavySecondsLeft = 0.01F;
    stamina.noDrainSecondsLeft = 0.01F;
    game::advanceStaminaClocks(stamina, 1.0F);
    CHECK(stamina.heavySecondsLeft == doctest::Approx(0.0F));
    CHECK(stamina.noDrainSecondsLeft == doctest::Approx(0.01F));
    game::advanceStaminaClocks(stamina, 1.0F);
    CHECK(stamina.noDrainSecondsLeft == doctest::Approx(0.0F));
    game::advanceStaminaClocks(stamina, 1.0F);
    CHECK(stamina.heavySecondsLeft == doctest::Approx(0.0F));
    CHECK(stamina.noDrainSecondsLeft == doctest::Approx(0.0F));
}

TEST_CASE("the share of the weight that is left stays between 0 and 1") {
    game::StaminaSettings settings;
    game::Stamina stamina;
    stamina.heavySecondsLeft = 15.0F;
    CHECK(game::heavyFraction(stamina, settings) == doctest::Approx(0.5F));
    settings.heavySeconds = 10.0F;
    CHECK(game::heavyFraction(stamina, settings) == doctest::Approx(1.0F));
    settings.heavySeconds = 0.0F;
    CHECK(game::heavyFraction(stamina, settings) == doctest::Approx(0.0F));
    // A new round starts with a new stamina, which is not heavy.
    CHECK(game::Stamina{}.heavySecondsLeft == doctest::Approx(0.0F));
}

// ---- the HUD and the settings ------------------------------------------------------------

TEST_CASE("the sentence of the heartstone fits the line of the hints") {
    const std::string_view hint = debug::HEAVY_HINT;
    CHECK_FALSE(hint.empty());
    CHECK(hint.size() <= 60U);
    for (const char letter : hint) {
        // The printable ASCII characters, from the space to the tilde.
        CHECK(letter >= ' ');
        CHECK(letter <= '~');
    }
}

TEST_CASE("secondsSince counts up while its clock counts down") {
    CHECK(debug::secondsSince(30.0F, 30.0F) == doctest::Approx(0.0F));
    CHECK(debug::secondsSince(30.0F, 26.0F) == doctest::Approx(4.0F));
    CHECK(debug::secondsSince(30.0F, 0.0F) == doctest::Approx(30.0F));
    CHECK(debug::secondsSince(10.0F, 30.0F) == doctest::Approx(0.0F));
}

TEST_CASE("the settings file knows nothing of the heartstone") {
    // Nothing of it is a setting or is saved: the file of a player stays as it was.
    const std::string text = game::formatSettings(game::GameSettings{});
    CHECK(text.find("heart") == std::string::npos);
    CHECK(text.find("heavy") == std::string::npos);
}
