// Tests of game/Difficulty: the table of the three levels.
#include "game/Difficulty.hpp"

#include "game/Crystals.hpp"
#include "game/Lighting.hpp"
#include "game/Maze.hpp"
#include "game/MazeWorld.hpp"
#include "game/MenuCamera.hpp"
#include "game/Round.hpp"
#include "game/Shadows.hpp"
#include "scene/Camera.hpp"
#include "scene/LightSpace.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace {

using game::Difficulty;

// The world of a level, on flat ground, built the way a new game builds it.
game::MazeWorld worldOf(Difficulty difficulty, std::uint32_t seed) {
    const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
    return game::buildMazeWorld(level.mazeWidth, level.mazeHeight, seed,
                                game::InteractableSettings{}, level.crystalCount);
}

} // namespace

TEST_CASE("the three levels have these numbers") {
    // A change here is a change of the game: the table in Difficulty.hpp, the handoff
    // for the documents and the measurements behind them have to follow.
    const game::DifficultyLevel& easy = game::difficultyLevel(Difficulty::Easy);
    CHECK(std::string_view(easy.name) == "Easy");
    CHECK(easy.mazeWidth == 10);
    CHECK(easy.mazeHeight == 10);
    CHECK(easy.crystalCount == 13);
    CHECK(easy.requiredFraction == 0.7F);
    CHECK(easy.batteryLifetimeSeconds == 180.0F);

    const game::DifficultyLevel& normal = game::difficultyLevel(Difficulty::Normal);
    CHECK(std::string_view(normal.name) == "Normal");
    CHECK(normal.mazeWidth == 16);
    CHECK(normal.mazeHeight == 16);
    CHECK(normal.crystalCount == 26);
    CHECK(normal.requiredFraction == 0.7F);
    CHECK(normal.batteryLifetimeSeconds == 150.0F);

    const game::DifficultyLevel& hard = game::difficultyLevel(Difficulty::Hard);
    CHECK(std::string_view(hard.name) == "Hard");
    CHECK(hard.mazeWidth == 22);
    CHECK(hard.mazeHeight == 22);
    CHECK(hard.crystalCount == 40);
    CHECK(hard.requiredFraction == 0.8F);
    CHECK(hard.batteryLifetimeSeconds == 120.0F);
}

TEST_CASE("the easy level is the game as it was before the levels existed") {
    const game::DifficultyLevel& easy = game::difficultyLevel(Difficulty::Easy);
    const game::GameplaySettings defaults;
    CHECK(easy.mazeWidth == game::DEFAULT_MAZE_WIDTH);
    CHECK(easy.mazeHeight == game::DEFAULT_MAZE_HEIGHT);
    CHECK(easy.crystalCount == game::crystalCountFor(easy.mazeWidth * easy.mazeHeight));
    CHECK(easy.requiredFraction == defaults.requiredFraction);
    CHECK(easy.batteryLifetimeSeconds == defaults.batteryLifetimeSeconds);
}

TEST_CASE("every level is harder than the one before") {
    for (std::size_t i = 1; i < game::ALL_DIFFICULTIES.size(); ++i) {
        const game::DifficultyLevel& lower = game::difficultyLevel(game::ALL_DIFFICULTIES[i - 1]);
        const game::DifficultyLevel& higher = game::difficultyLevel(game::ALL_DIFFICULTIES[i]);
        CAPTURE(higher.name);
        // A larger maze with more crystals to find, of which no smaller part is needed.
        CHECK(higher.mazeWidth * higher.mazeHeight > lower.mazeWidth * lower.mazeHeight);
        CHECK(higher.crystalCount > lower.crystalCount);
        CHECK(higher.requiredFraction >= lower.requiredFraction);
        CHECK(game::requiredCrystalCount(higher.crystalCount, higher.requiredFraction) >
              game::requiredCrystalCount(lower.crystalCount, lower.requiredFraction));
        // And less light to do it with.
        CHECK(higher.batteryLifetimeSeconds < lower.batteryLifetimeSeconds);
    }
}

TEST_CASE("the numbers of every level are ones the game accepts") {
    for (const Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        CAPTURE(level.name);
        CHECK(level.mazeWidth >= 2);
        CHECK(level.mazeWidth <= game::Maze::MAX_SIZE);
        CHECK(level.mazeHeight >= 2);
        CHECK(level.mazeHeight <= game::Maze::MAX_SIZE);
        CHECK(level.crystalCount >= 1);
        CHECK(level.crystalCount <= game::MAX_CRYSTAL_COUNT);
        CHECK(level.requiredFraction > 0.0F);
        CHECK(level.requiredFraction <= 1.0F);
        CHECK(level.batteryLifetimeSeconds > 0.0F);
    }
}

TEST_CASE("a maze of every level really gets the crystals of its level") {
    constexpr std::uint32_t SEED_COUNT = 10;
    for (const Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        CAPTURE(level.name);
        for (std::uint32_t seed = 1; seed <= SEED_COUNT; ++seed) {
            const game::MazeWorld world = worldOf(difficulty, seed);
            CHECK(world.crystals.size() == static_cast<std::size_t>(level.crystalCount));
            CHECK(world.hasGate);
        }
    }
}

TEST_CASE("the glide of the menu camera sees the whole land of every level") {
    // The camera of the main menu flies around the maze and looks across it. The far
    // plane of the camera cuts off what is farther away, so the far corner of the land
    // has to be nearer than that from every point of the circle.
    const scene::Camera camera;
    game::MenuCameraSettings glide;
    glide.shot = game::MenuShot::HighGlide;
    constexpr int SAMPLES = 32;

    for (const Difficulty difficulty : game::ALL_DIFFICULTIES) {
        CAPTURE(game::difficultyLevel(difficulty).name);
        const game::MazeWorld world = worldOf(difficulty, 1U);
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

TEST_CASE("the shadow map of the moon stays finer than a wall is thick on every level") {
    // The map of the moon covers the whole land, so a larger maze means larger texels.
    // The walls are 0.2 m thick: a texel of a quarter of that still draws their shadows.
    constexpr float LARGEST_TEXEL = 0.05F;
    const glm::vec3 moon = game::moonDirection(game::LightingSettings{});
    float previous = 0.0F;
    for (const Difficulty difficulty : game::ALL_DIFFICULTIES) {
        CAPTURE(game::difficultyLevel(difficulty).name);
        const game::MazeWorld world = worldOf(difficulty, 1U);
        const scene::LightSpace lightSpace =
            scene::directionalLightSpace(game::shadowCasterBounds(world.terrain), moon);
        const float texel = game::shadowTexelSize(lightSpace, game::SHADOW_MAP_SIZE_HIGH);
        CHECK(texel < LARGEST_TEXEL);
        CHECK(texel > previous);
        previous = texel;
    }
}

TEST_CASE("a level is found by its key, and an unknown key changes nothing") {
    Difficulty difficulty = Difficulty::Normal;
    CHECK(game::difficultyFromKey("easy", difficulty));
    CHECK(difficulty == Difficulty::Easy);
    CHECK(game::difficultyFromKey("hard", difficulty));
    CHECK(difficulty == Difficulty::Hard);
    CHECK(game::difficultyFromKey("normal", difficulty));
    CHECK(difficulty == Difficulty::Normal);

    CHECK_FALSE(game::difficultyFromKey("Easy", difficulty));
    CHECK_FALSE(game::difficultyFromKey("", difficulty));
    CHECK_FALSE(game::difficultyFromKey("nightmare", difficulty));
    CHECK(difficulty == Difficulty::Normal);

    // Every level finds itself.
    for (const Difficulty level : game::ALL_DIFFICULTIES) {
        Difficulty found = Difficulty::Easy;
        CHECK(game::difficultyFromKey(game::difficultyLevel(level).key, found));
        CHECK(found == level);
    }
}
