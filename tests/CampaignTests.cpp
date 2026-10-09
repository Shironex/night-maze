// Tests of game/Campaign: the table of the five nights, the mazes built from it, the
// rules of the progress and the timing of the two cards.
#include "game/Campaign.hpp"

#include "game/Crystals.hpp"
#include "game/Difficulty.hpp"
#include "game/Exit.hpp"
#include "game/Flasks.hpp"
#include "game/Interactables.hpp"
#include "game/Intro.hpp"
#include "game/Lighting.hpp"
#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"
#include "game/MenuCamera.hpp"
#include "game/Round.hpp"
#include "game/Shade.hpp"
#include "game/Shadows.hpp"
#include "scene/Camera.hpp"
#include "scene/LightSpace.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

// The world of a night on flat ground, built the way the game builds it: the size and
// the crystals of the night, its notes, and the seed that follows from campaignSeed.
game::MazeWorld worldOf(int night, std::uint32_t campaignSeed) {
    const game::CampaignNight& level = game::campaignNight(night);
    return game::buildMazeWorld(level.mazeWidth, level.mazeHeight,
                                game::campaignNightSeed(campaignSeed, night),
                                game::campaignInteractables(night, {}), level.crystalCount);
}

// The rules of a round of a night, the way the game sets them.
game::GameplaySettings rulesOf(int night) {
    const game::CampaignNight& level = game::campaignNight(night);
    game::GameplaySettings rules;
    rules.requiredFraction = level.requiredFraction;
    rules.batteryLifetimeSeconds = level.batteryLifetimeSeconds;
    rules.flaskCount = level.flaskCount;
    rules.shade.enabled = level.shade;
    return rules;
}

} // namespace

TEST_CASE("the five nights have these numbers") {
    // A change here is a change of the game: the owner chose every number.
    struct Expected {
        std::string_view title;
        int size;
        int crystals;
        float gate;
        float battery;
        int flasks;
        bool shade;
        int storyNotes;
        int hintNotes;
    };
    const std::vector<Expected> expected = {
        {.title = "First Frost",
         .size = 10,
         .crystals = 13,
         .gate = 0.7F,
         .battery = 180.0F,
         .flasks = 1,
         .shade = false,
         .storyNotes = 5,
         .hintNotes = 2},
        {.title = "The Shepherds' Gates",
         .size = 13,
         .crystals = 19,
         .gate = 0.7F,
         .battery = 165.0F,
         .flasks = 1,
         .shade = true,
         .storyNotes = 5,
         .hintNotes = 3},
        {.title = "Lamp's Back",
         .size = 16,
         .crystals = 26,
         .gate = 0.7F,
         .battery = 150.0F,
         .flasks = 2,
         .shade = true,
         .storyNotes = 5,
         .hintNotes = 4},
        {.title = "What the Moon Misses",
         .size = 19,
         .crystals = 33,
         .gate = 0.8F,
         .battery = 135.0F,
         .flasks = 2,
         .shade = true,
         .storyNotes = 5,
         .hintNotes = 5},
        {.title = "The Last Lamp",
         .size = 22,
         .crystals = 40,
         .gate = 0.8F,
         .battery = 120.0F,
         .flasks = 3,
         .shade = true,
         .storyNotes = 4,
         .hintNotes = 6},
    };
    REQUIRE(expected.size() == static_cast<std::size_t>(game::CAMPAIGN_NIGHT_COUNT));
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::CampaignNight& level = game::campaignNight(night);
        const Expected& wanted = expected[static_cast<std::size_t>(night - 1)];
        CHECK(std::string_view(level.title) == wanted.title);
        CHECK(level.mazeWidth == wanted.size);
        CHECK(level.mazeHeight == wanted.size);
        CHECK(level.crystalCount == wanted.crystals);
        CHECK(level.requiredFraction == wanted.gate);
        CHECK(level.batteryLifetimeSeconds == wanted.battery);
        CHECK(level.flaskCount == wanted.flasks);
        CHECK(level.shade == wanted.shade);
        CHECK(level.storyNotes == wanted.storyNotes);
        CHECK(level.hintNotes == wanted.hintNotes);
    }
}

TEST_CASE("a number that is no night is an error") {
    CHECK_THROWS_AS(game::campaignNight(0), std::out_of_range);
    CHECK_THROWS_AS(game::campaignNight(game::CAMPAIGN_NIGHT_COUNT + 1), std::out_of_range);
    CHECK_THROWS_AS(game::campaignNight(-3), std::out_of_range);
    CHECK_THROWS_AS(game::campaignNightName(6), std::out_of_range);
    CHECK_THROWS_AS(game::campaignInteractables(0, {}), std::out_of_range);
}

TEST_CASE("nights 1, 3 and 5 are the levels Easy, Normal and Hard of free play") {
    const std::vector<std::pair<int, game::Difficulty>> pairs = {
        {1, game::Difficulty::Easy}, {3, game::Difficulty::Normal}, {5, game::Difficulty::Hard}};
    for (const auto& [night, difficulty] : pairs) {
        CAPTURE(night);
        const game::CampaignNight& level = game::campaignNight(night);
        const game::DifficultyLevel& free = game::difficultyLevel(difficulty);
        CHECK(level.mazeWidth == free.mazeWidth);
        CHECK(level.mazeHeight == free.mazeHeight);
        CHECK(level.crystalCount == free.crystalCount);
        CHECK(level.flaskCount == free.flaskCount);
        CHECK(level.requiredFraction == free.requiredFraction);
        CHECK(level.batteryLifetimeSeconds == free.batteryLifetimeSeconds);
    }
}

TEST_CASE("only the first night has no shade") {
    CHECK_FALSE(game::campaignNight(1).shade);
    for (int night = 2; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CHECK(game::campaignNight(night).shade);
    }
}

TEST_CASE("every night is harder than the one before") {
    for (int night = 2; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::CampaignNight& lower = game::campaignNight(night - 1);
        const game::CampaignNight& higher = game::campaignNight(night);
        // A larger maze with more crystals to find, of which no smaller part is needed.
        CHECK(higher.mazeWidth * higher.mazeHeight > lower.mazeWidth * lower.mazeHeight);
        CHECK(higher.crystalCount > lower.crystalCount);
        CHECK(higher.requiredFraction >= lower.requiredFraction);
        CHECK(game::requiredCrystalCount(higher.crystalCount, higher.requiredFraction) >
              game::requiredCrystalCount(lower.crystalCount, lower.requiredFraction));
        // Less light to do it with, never fewer flasks, and more hints to find the way.
        CHECK(higher.batteryLifetimeSeconds < lower.batteryLifetimeSeconds);
        CHECK(higher.flaskCount >= lower.flaskCount);
        CHECK(higher.hintNotes > lower.hintNotes);
    }
}

TEST_CASE("the gates of the five nights open at 10, 14, 19, 27 and 32 crystals") {
    const std::vector<int> expected = {10, 14, 19, 27, 32};
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        const game::CampaignNight& level = game::campaignNight(night);
        CHECK(game::requiredCrystalCount(level.crystalCount, level.requiredFraction) ==
              expected[static_cast<std::size_t>(night - 1)]);
    }
}

TEST_CASE("the numbers of every night are ones the game accepts") {
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::CampaignNight& level = game::campaignNight(night);
        CHECK(level.mazeWidth >= 2);
        CHECK(level.mazeWidth <= game::Maze::MAX_SIZE);
        CHECK(level.mazeHeight >= 2);
        CHECK(level.mazeHeight <= game::Maze::MAX_SIZE);
        CHECK(level.crystalCount >= 1);
        CHECK(level.crystalCount <= game::MAX_CRYSTAL_COUNT);
        CHECK(level.requiredFraction > 0.0F);
        CHECK(level.requiredFraction <= 1.0F);
        CHECK(level.batteryLifetimeSeconds > 0.0F);
        CHECK(level.flaskCount >= 0);
        // The crystals leave this many dead ends alone, so this many flasks always find one.
        CHECK(level.flaskCount <= game::FLASK_RESERVED_DEAD_ENDS);
        CHECK(level.storyNotes >= 0);
        CHECK(level.hintNotes >= 0);
        CHECK(level.storyNotes + level.hintNotes <= game::MAX_NOTE_COUNT);
    }
}

TEST_CASE("the story lines of the nights are the whole story, each line once, in order") {
    int next = 0;
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::CampaignNight& level = game::campaignNight(night);
        // The lines of a night follow the lines of the night before.
        CHECK(level.firstStoryLine == next);
        CHECK(level.storyLineCount >= 1);
        // A night never has more story notes than lines: no line shows twice.
        CHECK(level.storyNotes <= level.storyLineCount);
        // A night without a shade has no line about the shadow.
        if (!level.shade) {
            for (int line = 0; line < level.storyLineCount; ++line) {
                CHECK_FALSE(game::flavourLineNeedsShade(level.firstStoryLine + line));
            }
        }
        next += level.storyLineCount;
    }
    CHECK(next == game::flavourLineCount());
}

TEST_CASE("the first nights each start with the line the story document names") {
    CHECK(game::flavourLine(game::campaignNight(1).firstStoryLine) ==
          "The moon sees every corridor. You see one.");
    CHECK(game::flavourLine(game::campaignNight(2).firstStoryLine) ==
          "A lever moves a wall. Somewhere.");
    CHECK(game::flavourLine(game::campaignNight(3).firstStoryLine) ==
          "It walks when you turn. It walks when the lamp sleeps.");
    CHECK(game::flavourLine(game::campaignNight(4).firstStoryLine) ==
          "Each piece that falls leaves a hole. The hole comes after.");
    CHECK(game::flavourLine(game::campaignNight(5).firstStoryLine) ==
          "My hands shake now. The lamp does not mind whose hand.");
}

TEST_CASE("the first four nights end with their line, and the last with the ending card") {
    CHECK(std::string_view(game::campaignNight(1).endLine) ==
          "The near lamps are lit. The village slept a little.");
    CHECK(std::string_view(game::campaignNight(2).endLine) ==
          "The lane is lit as far as the well.");
    CHECK(std::string_view(game::campaignNight(3).endLine) ==
          "It followed you to the gate and stopped at the lamplight.");
    CHECK(std::string_view(game::campaignNight(4).endLine) ==
          "Lamps to the edge of the village. One window still dark.");
    CHECK(std::string_view(game::campaignNight(5).endLine).empty());

    const auto leftSome = game::endingLines(true);
    CHECK(std::string_view(leftSome[0]) == "Every lamp in the village has its splinter now.");
    CHECK(std::string_view(leftSome[1]) == "The last one hangs in a window you know.");
    CHECK(std::string_view(leftSome[2]) ==
          "You left a few behind. The moon will come back for them.");
    CHECK(std::string_view(leftSome[3]) == "So will you.");

    // Only the third line differs when every crystal was taken.
    const auto leftNone = game::endingLines(false);
    CHECK(std::string_view(leftNone[0]) == std::string_view(leftSome[0]));
    CHECK(std::string_view(leftNone[1]) == std::string_view(leftSome[1]));
    CHECK(std::string_view(leftNone[2]) == "You left none behind. The moon will look harder.");
    CHECK(std::string_view(leftNone[3]) == std::string_view(leftSome[3]));
}

TEST_CASE("every text of the campaign can be written into a menu document") {
    // The menus take text as RML, where < and & start a tag or an entity, and the font
    // has the printable ASCII letters only.
    std::vector<std::string_view> texts;
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        texts.emplace_back(game::campaignNight(night).title);
        texts.emplace_back(game::campaignNight(night).endLine);
    }
    for (const bool crystalsLeft : {true, false}) {
        for (const char* line : game::endingLines(crystalsLeft)) {
            texts.emplace_back(line);
        }
    }
    for (const std::string_view text : texts) {
        CAPTURE(text);
        for (const char character : text) {
            CHECK(character >= ' ');
            CHECK(character <= '~');
            CHECK(character != '<');
            CHECK(character != '&');
        }
    }
}

TEST_CASE("a night is named with its number and its title") {
    CHECK(game::campaignNightLabel(2) == "Night 2");
    CHECK(game::campaignNightName(2) == "Night 2: The Shepherds' Gates");
    CHECK(game::campaignNightName(5) == "Night 5: The Last Lamp");
}

TEST_CASE("the seed of a night follows from the campaign seed and the night alone") {
    for (const std::uint32_t campaignSeed : {1U, 2U, 76U, 999999U, 4294967295U}) {
        CAPTURE(campaignSeed);
        std::vector<std::uint32_t> seeds;
        for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
            const std::uint32_t seed = game::campaignNightSeed(campaignSeed, night);
            // The same question, the same answer: a night played again is the same maze.
            CHECK(seed == game::campaignNightSeed(campaignSeed, night));
            // At most six digits, and never 0.
            CHECK(seed >= 1U);
            CHECK(seed < game::CAMPAIGN_SEED_LIMIT);
            // The five nights of a campaign have five different seeds.
            CHECK(std::ranges::find(seeds, seed) == seeds.end());
            seeds.push_back(seed);
        }
    }
    // Another campaign, other mazes.
    CHECK(game::campaignNightSeed(1U, 1) != game::campaignNightSeed(2U, 1));
}

TEST_CASE("golden seeds: the nights of campaign seed 1 have exactly these seeds") {
    // Written down once. A saved campaign keeps only its own seed, so its five mazes are
    // the same after an update only as long as these numbers stay.
    CHECK(game::campaignNightSeed(game::TOOL_CAMPAIGN_SEED, 1) == 134539U);
    CHECK(game::campaignNightSeed(game::TOOL_CAMPAIGN_SEED, 2) == 344651U);
    CHECK(game::campaignNightSeed(game::TOOL_CAMPAIGN_SEED, 3) == 367500U);
    CHECK(game::campaignNightSeed(game::TOOL_CAMPAIGN_SEED, 4) == 434081U);
    CHECK(game::campaignNightSeed(game::TOOL_CAMPAIGN_SEED, 5) == 45040U);
}

TEST_CASE("a drawn campaign seed is never the seed that means no campaign") {
    for (const std::uint32_t random : {0U, 1U, 999998U, 999999U, 1000000U, 4294967295U}) {
        const std::uint32_t seed = game::drawnCampaignSeed(random);
        CHECK(seed != game::NO_CAMPAIGN_SEED);
        CHECK(seed < game::CAMPAIGN_SEED_LIMIT);
    }
}

TEST_CASE("the notes of a night are its own mix and its own lines, whatever free play has") {
    // The story line counter of free play and its calm night do not reach a night.
    const game::InteractableSettings freePlay{
        .leverCount = 3, .noteCount = 6, .firstStoryLine = 17, .shadeLines = false};
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::CampaignNight& level = game::campaignNight(night);
        const game::InteractableSettings settings = game::campaignInteractables(night, freePlay);
        CHECK(settings.noteCount == level.storyNotes + level.hintNotes);
        CHECK(settings.storyNoteCount == level.storyNotes);
        CHECK(settings.firstStoryLine == level.firstStoryLine);
        CHECK(settings.shadeLines);
        // The levers are not part of a night.
        CHECK(settings.leverCount == 3);
    }
}

TEST_CASE("the mazes of every night keep every rule, over many campaigns") {
    constexpr std::uint32_t CAMPAIGN_COUNT = 60;
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::CampaignNight& level = game::campaignNight(night);
        for (std::uint32_t campaignSeed = 1; campaignSeed <= CAMPAIGN_COUNT; ++campaignSeed) {
            CAPTURE(campaignSeed);
            const game::MazeWorld world = worldOf(night, campaignSeed);
            const game::Maze& maze = world.maze;

            // The crystals of the night, and a gate to open with them.
            CHECK(world.crystals.size() == static_cast<std::size_t>(level.crystalCount));
            CHECK(world.hasGate);

            // Every note is placed, in a cell of its own, and the mix is the one of the
            // night: its story notes, and hints that point to the exit and to crystals
            // in turns.
            const std::vector<game::Note>& notes = world.interactables.notes;
            REQUIRE(notes.size() == static_cast<std::size_t>(level.storyNotes + level.hintNotes));
            CHECK(game::storyNoteCount(world.interactables) == level.storyNotes);
            const auto exitHints = std::ranges::count_if(notes, [](const game::Note& note) {
                return note.kind == game::NoteKind::ExitHint;
            });
            CHECK(exitHints == (level.hintNotes + 1) / 2);
            for (std::size_t i = 0; i < notes.size(); ++i) {
                CHECK_FALSE(notes[i].mount.cell == game::START_CELL);
                CHECK_FALSE(notes[i].mount.cell == world.exitCell);
                CHECK(maze.hasWall(notes[i].mount.cell.x, notes[i].mount.cell.z,
                                   notes[i].mount.side));
                for (std::size_t other = 0; other < i; ++other) {
                    CHECK_FALSE(notes[i].mount.cell == notes[other].mount.cell);
                }
            }

            // The story notes show the lines of the night and no other, none twice, and
            // in the order of the story from the start outwards.
            const std::vector<int> distances = game::passageDistances(maze, game::START_CELL);
            const auto width = static_cast<std::size_t>(maze.width());
            std::vector<std::pair<int, int>> byDistance; // distance, line
            for (const game::Note& note : notes) {
                if (note.kind != game::NoteKind::Flavour) {
                    continue;
                }
                CHECK(note.flavourIndex >= level.firstStoryLine);
                CHECK(note.flavourIndex < level.firstStoryLine + level.storyLineCount);
                const auto place = static_cast<std::size_t>(note.mount.cell.z) * width +
                                   static_cast<std::size_t>(note.mount.cell.x);
                byDistance.emplace_back(distances[place], note.flavourIndex);
            }
            std::ranges::stable_sort(
                byDistance, [](const auto& a, const auto& b) { return a.first < b.first; });
            // The nearest note shows the first line of the night, the next one the line
            // after it, and so on. Two notes equally far away keep the order of the
            // list (the sort is stable), like in placeInteractables.
            REQUIRE(byDistance.size() == static_cast<std::size_t>(level.storyNotes));
            for (std::size_t rank = 0; rank < byDistance.size(); ++rank) {
                CHECK(byDistance[rank].second == level.firstStoryLine + static_cast<int>(rank));
            }

            // The round: its flasks lie in dead ends without a crystal, and the shade
            // is there on every night but the first, in a cell of the maze that is
            // neither the start nor the exit.
            const game::Round round = game::startRound(world, rulesOf(night));
            REQUIRE(round.flasks.size() == static_cast<std::size_t>(level.flaskCount));
            for (const game::RoundFlask& flask : round.flasks) {
                CHECK(game::isDeadEnd(maze, flask.cell.x, flask.cell.z));
                CHECK_FALSE(flask.cell == game::START_CELL);
                CHECK_FALSE(flask.cell == world.exitCell);
                for (const game::CrystalSpawn& crystal : world.crystals) {
                    CHECK_FALSE(crystal.cell == flask.cell);
                }
            }
            CHECK(round.requiredCount ==
                  game::requiredCrystalCount(level.crystalCount, level.requiredFraction));
            CHECK(round.shade.present == level.shade);
            if (level.shade) {
                game::MazeCell shadeCell;
                REQUIRE(game::shadeStartCell(maze, world.seed, game::START_CELL, world.exitCell,
                                             shadeCell));
                CHECK(maze.contains(shadeCell.x, shadeCell.z));
                CHECK_FALSE(shadeCell == game::START_CELL);
                CHECK_FALSE(shadeCell == world.exitCell);
                // It can walk to the player: a way leads from its cell to the start.
                const auto place = static_cast<std::size_t>(shadeCell.z) * width +
                                   static_cast<std::size_t>(shadeCell.x);
                CHECK(distances[place] != game::UNREACHABLE);
            }
        }
    }
}

TEST_CASE("the glide of the menu camera sees the whole land of every night") {
    // The same check the levels of free play pass (DifficultyTests.cpp), for the two
    // sizes only the campaign has as well.
    const scene::Camera camera;
    game::MenuCameraSettings glide;
    glide.shot = game::MenuShot::HighGlide;
    constexpr int SAMPLES = 32;

    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::MazeWorld world = worldOf(night, game::TOOL_CAMPAIGN_SEED);
        const game::MenuCameraPath path = game::buildMenuCameraPath(world);
        const float loopSeconds = game::menuCameraLoopSeconds(path, world, glide);
        REQUIRE(loopSeconds > 0.0F);

        const game::Terrain& terrain = world.terrain;
        for (int i = 0; i < SAMPLES; ++i) {
            const float seconds = loopSeconds * static_cast<float>(i) / static_cast<float>(SAMPLES);
            const glm::vec3 eye = game::menuCameraPose(path, world, glide, seconds).eye;
            for (const float x : {terrain.minX(), terrain.maxX()}) {
                for (const float z : {terrain.minZ(), terrain.maxZ()}) {
                    CHECK(glm::distance(eye, glm::vec3{x, 0.0F, z}) < camera.farPlane);
                }
            }
        }
    }
}

TEST_CASE("the shadow map of the moon stays finer than a wall is thick on every night") {
    constexpr float LARGEST_TEXEL = 0.05F;
    const glm::vec3 moon = game::moonDirection(game::LightingSettings{});
    float previous = 0.0F;
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CAPTURE(night);
        const game::MazeWorld world = worldOf(night, game::TOOL_CAMPAIGN_SEED);
        const scene::LightSpace lightSpace =
            scene::directionalLightSpace(game::shadowCasterBounds(world.terrain), moon);
        const float texel = game::shadowTexelSize(lightSpace, game::SHADOW_MAP_SIZE_HIGH);
        CHECK(texel < LARGEST_TEXEL);
        CHECK(texel > previous);
        previous = texel;
    }
}

TEST_CASE("the first entry of the main menu reads Begin, Continue or New campaign") {
    CHECK(game::campaignStage(1) == game::CampaignStage::NotStarted);
    for (int night = 2; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CHECK(game::campaignStage(night) == game::CampaignStage::Running);
    }
    CHECK(game::campaignStage(game::CAMPAIGN_FINISHED) == game::CampaignStage::Finished);
    // Numbers a broken file could hold never give a stage that does not exist.
    CHECK(game::campaignStage(0) == game::CampaignStage::NotStarted);
    CHECK(game::campaignStage(-7) == game::CampaignStage::NotStarted);
    CHECK(game::campaignStage(99) == game::CampaignStage::Finished);
}

TEST_CASE("the menu offers the next night, and the first one once the campaign is finished") {
    CHECK(game::nightToOffer(1) == 1);
    CHECK(game::nightToOffer(2) == 2);
    CHECK(game::nightToOffer(5) == 5);
    CHECK(game::nightToOffer(game::CAMPAIGN_FINISHED) == 1);
    CHECK(game::nightToOffer(0) == 1);
    CHECK(game::nightToOffer(99) == 1);
}

TEST_CASE("finished nights can be replayed, the next one is open, later ones are locked") {
    // Two nights won: the third is next.
    CHECK(game::nightStatus(3, 1) == game::NightStatus::Finished);
    CHECK(game::nightStatus(3, 2) == game::NightStatus::Finished);
    CHECK(game::nightStatus(3, 3) == game::NightStatus::Open);
    CHECK(game::nightStatus(3, 4) == game::NightStatus::Locked);
    CHECK(game::nightStatus(3, 5) == game::NightStatus::Locked);

    // No campaign yet: only the first night is open.
    CHECK(game::nightStatus(1, 1) == game::NightStatus::Open);
    CHECK(game::nightStatus(1, 2) == game::NightStatus::Locked);

    // A finished campaign: every night can be played again.
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CHECK(game::nightStatus(game::CAMPAIGN_FINISHED, night) == game::NightStatus::Finished);
    }
}

TEST_CASE("winning the next night moves the campaign on, a replay never does") {
    CHECK(game::nightAfterWin(1, 1) == 2);
    CHECK(game::nightAfterWin(4, 4) == 5);
    CHECK(game::nightAfterWin(5, 5) == game::CAMPAIGN_FINISHED);

    // A finished night won again.
    CHECK(game::nightAfterWin(3, 1) == 3);
    CHECK(game::nightAfterWin(3, 2) == 3);
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CHECK(game::nightAfterWin(game::CAMPAIGN_FINISHED, night) == game::CAMPAIGN_FINISHED);
    }

    // Five wins in order finish the campaign.
    int campaign = 1;
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        campaign = game::nightAfterWin(campaign, night);
    }
    CHECK(campaign == game::CAMPAIGN_FINISHED);
}

TEST_CASE("a first win offers the next night on the result screen") {
    using game::NightEndOffer;
    // The campaign after the win waits for the night after the won one.
    for (int night = 1; night < game::CAMPAIGN_NIGHT_COUNT; ++night) {
        const int after = game::nightAfterWin(night, night);
        CHECK(game::nightEndOffer(after, night, true) == NightEndOffer::NextNight);
    }
}

TEST_CASE("a replayed night offers the next night only when that is the open one") {
    using game::NightEndOffer;
    // The campaign waits for night 4. Night 3 again: night 4 is still the next one.
    CHECK(game::nightEndOffer(game::nightAfterWin(4, 3), 3, true) == NightEndOffer::NextNight);
    // Nights 1 and 2 again: the night after them is finished, and night 4 is not next.
    CHECK(game::nightEndOffer(game::nightAfterWin(4, 1), 1, true) == NightEndOffer::BackToNights);
    CHECK(game::nightEndOffer(game::nightAfterWin(4, 2), 2, true) == NightEndOffer::BackToNights);
    // A finished campaign: every replay leads back to the list, also of night 4, whose
    // next night is finished too.
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        CHECK(game::nightEndOffer(game::CAMPAIGN_FINISHED, night, true) ==
              NightEndOffer::BackToNights);
    }
}

TEST_CASE("the last night and a night outside the campaign never offer a next night") {
    using game::NightEndOffer;
    // The last night has none: its win shows the ending card.
    CHECK(game::nightEndOffer(game::nightAfterWin(5, 5), 5, true) == NightEndOffer::BackToNights);
    // A night of the command line (--night) does not belong to the campaign of the
    // settings file, whatever that campaign waits for.
    for (int campaign = 1; campaign <= game::CAMPAIGN_FINISHED; ++campaign) {
        for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
            CHECK(game::nightEndOffer(campaign, night, false) == NightEndOffer::BackToNights);
        }
    }
    // A number that is no night.
    CHECK(game::nightEndOffer(1, 0, true) == NightEndOffer::BackToNights);
}

TEST_CASE("the best time of a night is its shortest win, in whole seconds") {
    // The first win is the best time. Parts of a second are cut off, like on the screen.
    CHECK(game::bestAfterWin(game::NO_BEST_TIME, 95.9F) == 95);
    // A faster win replaces it, a slower one does not.
    CHECK(game::bestAfterWin(95, 80.2F) == 80);
    CHECK(game::bestAfterWin(95, 120.0F) == 95);
    CHECK(game::bestAfterWin(95, 95.5F) == 95);
    // A win in no time at all still counts as a time: 0 means "none yet".
    CHECK(game::bestAfterWin(game::NO_BEST_TIME, 0.0F) == 1);
    CHECK(game::bestAfterWin(game::NO_BEST_TIME, 0.4F) == 1);
    // A round left open over night.
    CHECK(game::bestAfterWin(game::NO_BEST_TIME, 1.0e9F) == game::MAX_BEST_SECONDS);
}

TEST_CASE("the title card of a night shows two lines for three seconds") {
    // Nothing at the cut, all of both lines in the middle, nothing again before the end.
    const game::StoryCardFrame start = game::nightCardFrame(0.0F);
    CHECK_FALSE(start.finished);
    CHECK(start.lineOpacity[0] == 0.0F);
    CHECK(start.lineOpacity[1] == 0.0F);

    const game::StoryCardFrame middle = game::nightCardFrame(game::NIGHT_CARD_SECONDS / 2.0F);
    CHECK(middle.lineOpacity[0] == 1.0F);
    CHECK(middle.lineOpacity[1] == 1.0F);
    // The card has two lines only, and no hint.
    CHECK(middle.lineOpacity[2] == 0.0F);
    CHECK(middle.lineOpacity[3] == 0.0F);
    CHECK(middle.hintOpacity == 0.0F);

    const game::StoryCardFrame late =
        game::nightCardFrame(game::NIGHT_CARD_SECONDS - game::INTRO_TEXT_GAP_SECONDS);
    CHECK_FALSE(late.finished);
    CHECK(late.lineOpacity[0] == 0.0F);

    CHECK(game::nightCardFrame(game::NIGHT_CARD_SECONDS).finished);
    CHECK(game::NIGHT_CARD_SECONDS == 3.0F);
}

TEST_CASE("a card cannot be skipped in its first half second") {
    // The key that started the night is still down when the card comes up.
    CHECK_FALSE(game::nightCardFrame(0.0F).skippable);
    CHECK_FALSE(game::nightCardFrame(game::INTRO_SKIP_DELAY_SECONDS - 0.01F).skippable);
    CHECK(game::nightCardFrame(game::INTRO_SKIP_DELAY_SECONDS).skippable);
    CHECK_FALSE(game::endingCardFrame(0.0F).skippable);
    CHECK(game::endingCardFrame(game::INTRO_SKIP_DELAY_SECONDS).skippable);
}

TEST_CASE("the lines of the ending card come up one after another and leave together") {
    // Right before a line begins it is not there, and the lines above it are.
    for (std::size_t line = 1; line < game::STORY_CARD_LINE_COUNT; ++line) {
        CAPTURE(line);
        const float begins = static_cast<float>(line) * game::ENDING_LINE_SECONDS;
        const game::StoryCardFrame frame = game::endingCardFrame(begins);
        CHECK(frame.lineOpacity[line] == 0.0F);
        CHECK(frame.lineOpacity[line - 1] == 1.0F);
    }

    // All four stand together for a while: long enough to read the last one.
    const float allThere = 3.0F * game::ENDING_LINE_SECONDS + game::INTRO_TEXT_DELAY_SECONDS +
                           game::INTRO_TEXT_FADE_SECONDS;
    const float leaving =
        game::ENDING_CARD_SECONDS - game::INTRO_TEXT_GAP_SECONDS - game::INTRO_TEXT_FADE_SECONDS;
    CHECK(leaving - allThere >= 4.0F);
    for (const float seconds : {allThere, (allThere + leaving) / 2.0F, leaving}) {
        const game::StoryCardFrame frame = game::endingCardFrame(seconds);
        CHECK_FALSE(frame.finished);
        for (const float opacity : frame.lineOpacity) {
            CHECK(opacity == doctest::Approx(1.0F));
        }
    }

    // They leave together, and nothing is left when the card ends.
    const game::StoryCardFrame fading =
        game::endingCardFrame(leaving + game::INTRO_TEXT_FADE_SECONDS / 2.0F);
    for (const float opacity : fading.lineOpacity) {
        CHECK(opacity == doctest::Approx(0.5F));
    }
    const game::StoryCardFrame last =
        game::endingCardFrame(game::ENDING_CARD_SECONDS - game::INTRO_TEXT_GAP_SECONDS);
    for (const float opacity : last.lineOpacity) {
        CHECK(opacity == doctest::Approx(0.0F));
    }
    CHECK(game::endingCardFrame(game::ENDING_CARD_SECONDS).finished);
    CHECK_FALSE(game::endingCardFrame(game::ENDING_CARD_SECONDS - 0.01F).finished);
}

TEST_CASE("the ending card shows the skip hint at its start, and its bell rings out") {
    CHECK(game::endingCardFrame(game::INTRO_HINT_FADE_SECONDS).hintOpacity == 1.0F);
    CHECK(game::endingCardFrame(game::INTRO_HINT_SECONDS + game::INTRO_HINT_FADE_SECONDS)
              .hintOpacity == 0.0F);
    // The bell is 5.5 seconds long (game/Intro.cpp): it has rung out when the card ends
    // and every sound is stopped.
    constexpr float BELL_SECONDS = 5.5F;
    CHECK(game::ENDING_BELL_SECONDS + BELL_SECONDS < game::ENDING_CARD_SECONDS);
}

TEST_CASE(
    "a night shows the village as the nights before it left it, and its result adds its own") {
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        // The progress of the campaign and the tool switch do not matter for a night: a
        // finished night that is played again shows what it showed the first time.
        for (const int progress : {1, night, game::CAMPAIGN_FINISHED}) {
            for (const bool tool : {false, true}) {
                CHECK(game::villageNightsLit(night, false, progress, tool) == night - 1);
                CHECK(game::villageNightsLit(night, true, progress, tool) == night);
            }
        }
    }
    // The first night begins with a dark village, and the last one lights the last window.
    CHECK(game::villageNightsLit(1, false, 1, false) == 0);
    CHECK(game::villageNightsLit(5, true, 5, false) == game::CAMPAIGN_NIGHT_COUNT);
}

TEST_CASE("free play, the maze of the day and the menu show the progress of the campaign") {
    CHECK(game::villageNightsLit(0, false, 1, false) == 0);
    CHECK(game::villageNightsLit(0, false, 3, false) == 2);
    CHECK(game::villageNightsLit(0, false, game::CAMPAIGN_NIGHT_COUNT, false) == 4);
    CHECK(game::villageNightsLit(0, false, game::CAMPAIGN_FINISHED, false) == 5);
    // Winning a maze of free play lights nothing.
    CHECK(game::villageNightsLit(0, true, 3, false) == 2);
    // A number from a broken settings file stays inside the five nights.
    CHECK(game::villageNightsLit(0, false, -4, false) == 0);
    CHECK(game::villageNightsLit(0, false, 99, false) == 5);
}

TEST_CASE("a tool run without a night shows a dark village, whatever the settings file says") {
    for (int progress = 1; progress <= game::CAMPAIGN_FINISHED; ++progress) {
        CHECK(game::villageNightsLit(0, false, progress, true) == 0);
        CHECK(game::villageNightsLit(0, true, progress, true) == 0);
    }
}
