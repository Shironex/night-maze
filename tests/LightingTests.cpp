// Tests of game::Lighting: the dead ends of a maze and the lights built for one frame.
// See docs/modules/game/flashlight.md
#include "game/Lighting.hpp"

#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

// The start cell of every maze (see MazeWorld.cpp).
constexpr int START_COLUMN = 0;
constexpr int START_ROW = 0;

// A cell that is in no maze: nothing is skipped.
constexpr int NO_CELL = -1;

// A view direction for the tests that do not care about it: north, level.
constexpr glm::vec3 LOOK_NORTH{0.0F, 0.0F, -1.0F};

// Number of dead ends of a maze, counted cell by cell.
std::size_t countDeadEnds(const game::Maze& maze) {
    std::size_t count = 0;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            if (game::isDeadEnd(maze, x, z)) {
                ++count;
            }
        }
    }
    return count;
}

} // namespace

TEST_CASE("a cell is a dead end when it has exactly three walls") {
    // A new maze has every wall: four per cell, no dead end.
    game::Maze maze(2, 1);
    CHECK_FALSE(game::isDeadEnd(maze, 0, 0));

    // One passage between the two cells: both are closed on three sides.
    maze.removeWall(0, 0, game::Direction::East);
    CHECK(game::isDeadEnd(maze, 0, 0));
    CHECK(game::isDeadEnd(maze, 1, 0));

    // A second opening turns the cell into a corridor.
    maze.removeWall(1, 0, game::Direction::South);
    CHECK(game::isDeadEnd(maze, 0, 0));
    CHECK_FALSE(game::isDeadEnd(maze, 1, 0));
}

TEST_CASE("golden maze: 4 x 4 cells from seed 1 has lights in its two dead ends") {
    // The maze of the golden test in MazeGeneratorTests.cpp:
    //
    //   +--+--+--+--+
    //   |S |        |      S: the start cell (0, 0), a dead end that gets no light.
    //   +  +  +--+  +
    //   |  |     |* |      *: the dead ends (3, 1) and (0, 3).
    //   +  +--+  +--+
    //   |     |     |
    //   +--+  +--+  +
    //   |*          |
    //   +--+--+--+--+
    const game::Maze maze = game::generateMaze(4, 4, 1U);
    REQUIRE(game::isDeadEnd(maze, START_COLUMN, START_ROW));
    REQUIRE(countDeadEnds(maze) == 3);

    const std::vector<glm::vec3> positions =
        game::deadEndLightPositions(maze, START_COLUMN, START_ROW);

    // Row after row: the cell in row 1 comes before the cell in row 3. Each light hangs
    // above the centre of its cell, and a cell is 2 m wide.
    REQUIRE(positions.size() == 2);
    checkVector(positions[0], {7.0F, game::POINT_LIGHT_HEIGHT, 3.0F});
    checkVector(positions[1], {1.0F, game::POINT_LIGHT_HEIGHT, 7.0F});
    CHECK(game::POINT_LIGHT_HEIGHT == 1.4F);
}

TEST_CASE("without a skipped cell every dead end gets a light") {
    const game::Maze maze = game::generateMaze(4, 4, 1U);

    const std::vector<glm::vec3> positions = game::deadEndLightPositions(maze, NO_CELL, NO_CELL);

    REQUIRE(positions.size() == 3);
    checkVector(positions[0], game::cellCenter(0, 0) + glm::vec3{0.0F, 1.4F, 0.0F});
}

TEST_CASE("a maze never gets more point lights than the shader has room for") {
    const game::Maze maze = game::generateMaze(31, 20, 5U);
    // The test means something only when the maze has more dead ends than the limit.
    REQUIRE(countDeadEnds(maze) > static_cast<std::size_t>(scene::MAX_POINT_LIGHTS) + 1);

    const std::vector<glm::vec3> positions =
        game::deadEndLightPositions(maze, START_COLUMN, START_ROW);

    REQUIRE(positions.size() == static_cast<std::size_t>(scene::MAX_POINT_LIGHTS));

    // The choice is spread over the maze, not the first sixteen: it goes on row after
    // row (z never falls) and reaches the lower half of the maze. No position repeats.
    const float middleZ = static_cast<float>(maze.height()) * game::CELL_SIZE / 2.0F;
    CHECK(positions.back().z > middleZ);
    for (std::size_t i = 1; i < positions.size(); ++i) {
        CHECK(positions[i].z >= positions[i - 1].z);
        CHECK(positions[i] != positions[i - 1]);
    }
}

TEST_CASE("the same maze always gets the same lights") {
    const game::Maze maze = game::generateMaze(31, 20, 5U);
    CHECK(game::deadEndLightPositions(maze, START_COLUMN, START_ROW) ==
          game::deadEndLightPositions(maze, START_COLUMN, START_ROW));
}

TEST_CASE("a maze world carries the light positions of its maze") {
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1U);

    REQUIRE(world.pointLightPositions.size() == 2);
    checkVector(world.pointLightPositions[0], {7.0F, 1.4F, 3.0F});
    checkVector(world.pointLightPositions[1], {1.0F, 1.4F, 7.0F});
    // The start cell has none, although it is a dead end.
    for (const glm::vec3& position : world.pointLightPositions) {
        CHECK(position != world.startPosition + glm::vec3{0.0F, 1.4F, 0.0F});
    }
}

TEST_CASE("the default maze has this many point lights") {
    const game::MazeWorld world = game::buildMazeWorld(
        game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, game::DEFAULT_MAZE_SEED);
    CHECK(world.pointLightPositions.size() == 11U);
}

TEST_CASE("the lighting starts as a night scene shaded with Blinn-Phong") {
    const game::LightingSettings settings;
    CHECK(settings.mode == game::LightingMode::BlinnPhong);
    CHECK(settings.flashlightOn);
    CHECK(settings.flashlightInnerDegrees < settings.flashlightOuterDegrees);
    CHECK(settings.pointRadius == 3.0F);
    // The moon shines downwards.
    CHECK(scene::directionFromAngles(settings.moonYawDegrees, settings.moonPitchDegrees).y < 0.0F);
}

TEST_CASE("the numbers of the lighting modes are the entries of the list in the panel") {
    CHECK(static_cast<int>(game::LightingMode::Unlit) == 0);
    CHECK(static_cast<int>(game::LightingMode::Gouraud) == 1);
    CHECK(static_cast<int>(game::LightingMode::Phong) == 2);
    CHECK(static_cast<int>(game::LightingMode::BlinnPhong) == 3);
}

TEST_CASE("Gouraud and Phong use the Phong highlight, Blinn-Phong its own") {
    CHECK(game::specularModelOf(game::LightingMode::Gouraud) == game::SpecularModel::Phong);
    CHECK(game::specularModelOf(game::LightingMode::Phong) == game::SpecularModel::Phong);
    CHECK(game::specularModelOf(game::LightingMode::BlinnPhong) == game::SpecularModel::BlinnPhong);
    // The numbers the shader compares uSpecularModel with.
    CHECK(static_cast<int>(game::SpecularModel::Phong) == 0);
    CHECK(static_cast<int>(game::SpecularModel::BlinnPhong) == 1);
}

TEST_CASE("normal mapping is on by default and applies to every mode except Gouraud") {
    game::LightingSettings settings;
    CHECK(settings.normalMapping);

    // Phong and Blinn-Phong light per fragment: they can read a normal per texel.
    settings.mode = game::LightingMode::Phong;
    CHECK(game::usesNormalMap(settings));
    settings.mode = game::LightingMode::BlinnPhong;
    CHECK(game::usesNormalMap(settings));
    // Unlit has no lighting, but its view of the normals shows the normal maps.
    settings.mode = game::LightingMode::Unlit;
    CHECK(game::usesNormalMap(settings));
    // Gouraud lights per vertex: a normal map cannot take part.
    settings.mode = game::LightingMode::Gouraud;
    CHECK_FALSE(game::usesNormalMap(settings));

    // Switched off, it applies nowhere.
    settings.normalMapping = false;
    for (const game::LightingMode mode :
         {game::LightingMode::Unlit, game::LightingMode::Gouraud, game::LightingMode::Phong,
          game::LightingMode::BlinnPhong}) {
        settings.mode = mode;
        CHECK_FALSE(game::usesNormalMap(settings));
    }
}

TEST_CASE("buildLightSet takes the ambient light and the moon from the settings") {
    game::LightingSettings settings;
    settings.ambient = {0.1F, 0.2F, 0.3F};
    settings.moonYawDegrees = 90.0F;
    settings.moonPitchDegrees = -90.0F;
    settings.moonColor = {0.4F, 0.5F, 0.6F};
    settings.moonIntensity = 0.7F;

    const scene::LightSet lights = game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, {});

    checkVector(lights.ambient, {0.1F, 0.2F, 0.3F});
    // Pitch -90: the light travels straight down.
    checkVector(lights.directional.direction, {0.0F, -1.0F, 0.0F});
    checkVector(lights.directional.color, {0.4F, 0.5F, 0.6F});
    CHECK(lights.directional.intensity == 0.7F);
    CHECK(lights.pointCount == 0);
}

TEST_CASE("the flashlight sits at the eye and points where the camera looks") {
    game::LightingSettings settings;
    settings.flashlightRange = 10.0F;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    const glm::vec3 viewDirection{1.0F, 0.0F, 0.0F};

    const scene::LightSet lights = game::buildLightSet(settings, eye, viewDirection, {});

    CHECK(lights.spotEnabled);
    checkVector(lights.spot.position, eye);
    checkVector(lights.spot.direction, viewDirection);
    checkVector(lights.spot.color, settings.flashlightColor);
    CHECK(lights.spot.intensity == settings.flashlightIntensity);
    CHECK(lights.spot.innerConeDegrees == settings.flashlightInnerDegrees);
    CHECK(lights.spot.outerConeDegrees == settings.flashlightOuterDegrees);
    // The range sets the attenuation: 5 % is left at 10 m.
    CHECK(scene::attenuationFactor(lights.spot.attenuation, 10.0F) ==
          doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
}

TEST_CASE("switching the flashlight off keeps its settings") {
    game::LightingSettings settings;
    settings.flashlightOn = false;

    const scene::LightSet lights = game::buildLightSet(settings, glm::vec3{1.0F}, LOOK_NORTH, {});

    CHECK_FALSE(lights.spotEnabled);
    CHECK(lights.spot.intensity == settings.flashlightIntensity);
}

TEST_CASE("the inner cone of the flashlight is never wider than the outer cone") {
    game::LightingSettings settings;
    settings.flashlightInnerDegrees = 40.0F;
    settings.flashlightOuterDegrees = 15.0F;

    const scene::LightSet lights = game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, {});

    CHECK(lights.spot.innerConeDegrees == 15.0F);
    CHECK(lights.spot.outerConeDegrees == 15.0F);
}

TEST_CASE("every point light gets the shared colour, intensity and radius") {
    game::LightingSettings settings;
    settings.pointColor = {0.1F, 0.8F, 0.9F};
    settings.pointIntensity = 1.5F;
    settings.pointRadius = 4.0F;
    const std::vector<glm::vec3> positions = {{1.0F, 1.4F, 7.0F}, {7.0F, 1.4F, 3.0F}};

    const scene::LightSet lights =
        game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, positions);

    REQUIRE(lights.pointCount == 2);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        checkVector(lights.points[i].position, positions[i]);
        checkVector(lights.points[i].color, settings.pointColor);
        CHECK(lights.points[i].intensity == 1.5F);
        CHECK(scene::attenuationFactor(lights.points[i].attenuation, 4.0F) ==
              doctest::Approx(scene::BRIGHTNESS_AT_RADIUS));
    }
}

TEST_CASE("buildLightSet ignores positions past the largest number of point lights") {
    const game::LightingSettings settings;
    const std::vector<glm::vec3> positions(static_cast<std::size_t>(scene::MAX_POINT_LIGHTS) + 4,
                                           glm::vec3{1.0F, 1.4F, 1.0F});

    const scene::LightSet lights =
        game::buildLightSet(settings, glm::vec3{0.0F}, LOOK_NORTH, positions);

    CHECK(lights.pointCount == scene::MAX_POINT_LIGHTS);
}
