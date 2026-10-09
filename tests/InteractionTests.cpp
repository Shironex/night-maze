// Tests of the levers and the notes in a world and in a round (placement, pulling, the
// opened wall, the note card) and of game::Interaction (the picking of a frame). The last
// tests read the model files of the lever.
#include "game/Interaction.hpp"

#include "assets/ObjLoader.hpp"
#include "game/Discovery.hpp"
#include "game/Interactables.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Minimap.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "game/Terrain.hpp"
#include "scene/Camera.hpp"
#include "scene/Raycast.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(0.001));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(0.001));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(0.001));
}

// Where a model matrix puts a point of the local space of a model.
glm::vec3 transformPoint(const glm::mat4& matrix, const glm::vec3& point) {
    return glm::vec3{matrix * glm::vec4{point, 1.0F}};
}

// The fixed step of the game: 120 steps are one second.
constexpr float STEP = 1.0F / 120.0F;

// A place far away from every crystal, note and exit of the mazes used here.
constexpr glm::vec3 NOWHERE{-50.0F, 0.0F, -50.0F};

// The default maze of the game on flat ground: 10 x 10 cells from seed 1. It has levers
// (the tests that need one check that first).
game::MazeWorld defaultWorld() {
    return game::buildMazeWorld(10, 10, 1U);
}

// The step from a cell towards one of its sides, as a vector on the ground.
glm::vec3 towards(game::Direction side) {
    return {static_cast<float>(game::columnStep(side)), 0.0F,
            static_cast<float>(game::rowStep(side))};
}

// The cell on the other side of a wall.
game::MazeCell cellBehind(const game::WallRef& wall) {
    return {.x = wall.cell.x + game::columnStep(wall.side),
            .z = wall.cell.z + game::rowStep(wall.side)};
}

// The eyes of a player who stands in the middle of a cell on flat ground.
glm::vec3 eyeIn(game::MazeCell cell) {
    return game::cellCenter(cell.x, cell.z) + glm::vec3{0.0F, game::Player::EYE_HEIGHT, 0.0F};
}

// A ray from one point towards another.
scene::Ray rayTowards(const glm::vec3& from, const glm::vec3& target) {
    return {.origin = from, .direction = glm::normalize(target - from)};
}

// How many vertices of a minimap have exactly this colour.
int countColor(const std::vector<game::MinimapVertex>& vertices, const glm::vec3& color) {
    int count = 0;
    for (const game::MinimapVertex& vertex : vertices) {
        if (vertex.color == color) {
            ++count;
        }
    }
    return count;
}

// True when a point lies in a box, its faces included.
bool inside(const scene::Aabb& box, const glm::vec3& point) {
    return point.x >= box.min.x && point.x <= box.max.x && point.y >= box.min.y &&
           point.y <= box.max.y && point.z >= box.min.z && point.z <= box.max.z;
}

// A rectangle on the minimap is two triangles: six vertices.
constexpr int QUAD = 6;

// A model of the game, read from the assets directory that CMake compiles in.
assets::ObjModel loadModel(const char* fileName) {
    assets::ObjModel model;
    std::string error;
    const std::filesystem::path path =
        std::filesystem::path{NIGHT_MAZE_ASSETS_DIR} / "models" / fileName;
    REQUIRE_MESSAGE(assets::loadObj(path, model, error), error);
    return model;
}

// The ring under the board of the lever is the one part of the post that hangs out of
// the pick box: its lower edge is this far below the middle of the board
// (tools/blender/build_crook.py).
constexpr float POST_RING_BOTTOM = 0.341F;

} // namespace

TEST_CASE("a maze world holds the levers and notes of its seed and the wall of every lever") {
    const game::MazeWorld world = defaultWorld();
    const game::Interactables expected =
        game::placeInteractables(world.maze, world.seed, game::START_CELL, world.exitCell,
                                 world.crystals, game::InteractableSettings{});

    REQUIRE(world.interactables.levers.size() == expected.levers.size());
    REQUIRE(world.interactables.notes.size() == expected.notes.size());
    REQUIRE_FALSE(world.interactables.levers.empty());
    CHECK(world.interactables.notes.size() == 6);
    CHECK(world.leverWalls.size() == world.interactables.levers.size());

    for (std::size_t i = 0; i < expected.levers.size(); ++i) {
        const game::Lever& lever = world.interactables.levers[i];
        CHECK(lever.mount == expected.levers[i].mount);
        CHECK(lever.opens == expected.levers[i].opens);

        // The number in leverWalls names the segment openedWallSegment describes.
        const game::WallSegment opened = game::openedWallSegment(lever);
        REQUIRE(world.leverWalls[i] < world.walls.size());
        const game::WallSegment& found = world.walls[world.leverWalls[i]];
        CHECK(found.position.x == opened.position.x);
        CHECK(found.position.z == opened.position.z);
        CHECK(found.axis == opened.axis);
    }
}

TEST_CASE("the settings of a maze world decide how many levers and notes it gets") {
    const game::MazeWorld none = game::buildMazeWorld(
        10, 10, 1U, game::InteractableSettings{.leverCount = 0, .noteCount = 0});
    CHECK(none.interactables.levers.empty());
    CHECK(none.interactables.notes.empty());
    CHECK(none.leverWalls.empty());

    const game::MazeWorld many = game::buildMazeWorld(
        10, 10, 1U, game::InteractableSettings{.leverCount = 1, .noteCount = 7});
    CHECK(many.interactables.levers.size() == 1);
    CHECK(many.interactables.notes.size() == 7);

    // The settings of a new maze start with the default numbers.
    const game::MazeSettings settings;
    CHECK(settings.interactables.leverCount == game::DEFAULT_LEVER_COUNT);
    CHECK(settings.interactables.noteCount == game::DEFAULT_NOTE_COUNT);
}

TEST_CASE("levers and notes of a world hang above its terrain, also after a rebuild") {
    // A heightmap with a slope: the height grows from west to east.
    game::Heightmap heightmap;
    heightmap.width = 2;
    heightmap.height = 2;
    heightmap.values = {0.0F, 1.0F, 0.0F, 1.0F};

    game::MazeWorld world = game::buildMazeWorld(10, 10, 1U, heightmap, 2.0F);
    REQUIRE_FALSE(world.interactables.levers.empty());
    REQUIRE_FALSE(world.interactables.notes.empty());

    const auto checkHeights = [&world]() {
        for (const game::Lever& lever : world.interactables.levers) {
            const float ground = world.terrain.heightAt(lever.position.x, lever.position.z);
            CHECK(lever.position.y == doctest::Approx(ground + game::LEVER_MOUNT_HEIGHT));
            // The pick box follows: the lever hangs in the middle of its height.
            CHECK((lever.box.min.y + lever.box.max.y) / 2.0F == doctest::Approx(lever.position.y));
        }
        for (const game::Note& note : world.interactables.notes) {
            const float ground = world.terrain.heightAt(note.position.x, note.position.z);
            CHECK(note.position.y == doctest::Approx(ground + game::NOTE_MOUNT_HEIGHT));
        }
    };
    checkHeights();

    // Another height scale: everything moves up or down, nothing sideways.
    const glm::vec3 before = world.interactables.levers[0].position;
    game::placeOnTerrain(world, heightmap, 0.5F);
    checkHeights();
    CHECK(world.interactables.levers[0].position.x == before.x);
    CHECK(world.interactables.levers[0].position.z == before.z);
}

TEST_CASE("a new round has no lever pulled, every wall standing and no note open") {
    const game::MazeWorld world = defaultWorld();
    const game::Round round = game::startRound(world, game::GameplaySettings{});

    CHECK(round.interactables.leverPulled.size() == world.interactables.levers.size());
    CHECK(round.wallProgress.size() == world.interactables.levers.size());
    CHECK(game::pulledLeverCount(round) == 0);
    CHECK_FALSE(round.noteOpen);
    CHECK(game::openNoteText(world, round).empty());

    CHECK(game::roundObstacles(world, round).size() == world.colliders.size() + 1);
    const std::vector<glm::mat4> matrices = game::roundWallMatrices(world, round);
    REQUIRE(matrices.size() == world.wallMatrices.size());
    for (std::size_t i = 0; i < matrices.size(); ++i) {
        CHECK(matrices[i] == world.wallMatrices[i]);
    }

    // A round that was never started falls back to the maze of the world.
    const game::Round notStarted;
    CHECK(&game::roundMaze(world, notStarted) == &world.maze);
    CHECK(&game::roundMaze(world, round) != &world.maze);
}

TEST_CASE("the sink formulas reach the full depth in 1.5 s and stop there") {
    CHECK(game::sinkDepth(0.0F) == 0.0F);
    CHECK(game::sinkDepth(0.5F) == doctest::Approx(game::GATE_SINK_DEPTH / 2.0F));
    CHECK(game::sinkDepth(1.0F) == game::GATE_SINK_DEPTH);

    // One step is its share of GATE_OPEN_SECONDS.
    CHECK(game::sinkProgressAfter(0.0F, 0.75F) == doctest::Approx(0.5F));
    float progress = 0.0F;
    for (int i = 0; i < 180; ++i) {
        progress = game::sinkProgressAfter(progress, STEP);
    }
    CHECK(progress == doctest::Approx(1.0F).epsilon(0.001));
    // Never past 1, however long it runs.
    CHECK(game::sinkProgressAfter(progress, 10.0F) == 1.0F);
    // The wall (3 m) and the pillars (3.15 m) are lower than the full depth.
    CHECK(game::GATE_SINK_DEPTH > game::WALL_HEIGHT);
}

TEST_CASE("pulling a lever opens its wall once: obstacles, maze of the round, sinking") {
    const game::MazeWorld world = defaultWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    REQUIRE_FALSE(world.interactables.levers.empty());
    const game::Lever& lever = world.interactables.levers[0];
    const std::size_t wall = world.leverWalls[0];
    const std::size_t obstaclesBefore = game::roundObstacles(world, round).size();

    CHECK(game::pullRoundLever(round, world, 0));
    CHECK(game::pulledLeverCount(round) == 1);
    // The second pull changes nothing.
    CHECK_FALSE(game::pullRoundLever(round, world, 0));
    CHECK(game::pulledLeverCount(round) == 1);
    CHECK_THROWS_AS(game::pullRoundLever(round, world, world.interactables.levers.size()),
                    std::out_of_range);

    // The box of the wall is gone from the obstacles at once, and only that box.
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);
    CHECK(obstacles.size() == obstaclesBefore - 1);
    const scene::Aabb& wallBox = world.colliders[wall];
    for (const scene::Aabb& box : obstacles) {
        CHECK_FALSE((box.min == wallBox.min && box.max == wallBox.max));
    }
    CHECK(game::openedWallFlags(world, round)[wall]);

    // The maze of the round lost the wall, seen from both cells. The world kept it.
    const game::MazeCell behind = cellBehind(lever.opens);
    const game::Maze& maze = game::roundMaze(world, round);
    CHECK_FALSE(maze.hasWall(lever.opens.cell.x, lever.opens.cell.z, lever.opens.side));
    CHECK_FALSE(maze.hasWall(behind.x, behind.z, game::opposite(lever.opens.side)));
    CHECK(world.maze.hasWall(lever.opens.cell.x, lever.opens.cell.z, lever.opens.side));

    // The wall has not moved yet. It sinks with the steps, like the gate.
    CHECK(round.wallProgress[0] == 0.0F);
    CHECK(game::roundWallMatrices(world, round)[wall] == world.wallMatrices[wall]);
    CHECK(game::leverHandleProgress(round, 0) == 0.0F);

    bool flashlightOn = false;
    constexpr int HALF_OPEN_STEPS = 90;
    for (int i = 0; i < HALF_OPEN_STEPS; ++i) {
        game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    }
    CHECK(round.wallProgress[0] == doctest::Approx(0.5F).epsilon(0.001));
    // The handle is long down: it takes 0.3 s.
    CHECK(game::leverHandleProgress(round, 0) == 1.0F);

    // Half way: the wall is drawn half of the depth lower, in the same place.
    const std::vector<glm::mat4> matrices = game::roundWallMatrices(world, round);
    const glm::vec3 standing = transformPoint(world.wallMatrices[wall], glm::vec3{0.0F});
    const glm::vec3 sinking = transformPoint(matrices[wall], glm::vec3{0.0F});
    checkVector(sinking, standing - glm::vec3{0.0F, game::GATE_SINK_DEPTH / 2.0F, 0.0F});
    // No other wall moved.
    for (std::size_t i = 0; i < matrices.size(); ++i) {
        if (i != wall) {
            CHECK(matrices[i] == world.wallMatrices[i]);
        }
    }

    for (int i = 0; i < 2 * HALF_OPEN_STEPS; ++i) {
        game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    }
    CHECK(round.wallProgress[0] == 1.0F);
    const glm::vec3 sunk =
        transformPoint(game::roundWallMatrices(world, round)[wall], glm::vec3{0.0F});
    checkVector(sunk, standing - glm::vec3{0.0F, game::GATE_SINK_DEPTH, 0.0F});

    // A lever that was not pulled keeps its wall up.
    if (world.interactables.levers.size() > 1) {
        CHECK(round.wallProgress[1] == 0.0F);
    }
}

TEST_CASE("the handle of a lever swings down in 0.3 s after the pull") {
    const game::MazeWorld world = defaultWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    REQUIRE_FALSE(world.interactables.levers.empty());
    bool flashlightOn = false;

    game::pullRoundLever(round, world, 0);
    // 18 steps are 0.15 s: half of the swing.
    for (int i = 0; i < 18; ++i) {
        game::updateRound(round, world, settings, NOWHERE, flashlightOn, STEP);
    }
    CHECK(game::leverHandleProgress(round, 0) == doctest::Approx(0.5F).epsilon(0.01));
    // A lever the round does not have.
    CHECK(game::leverHandleProgress(round, 99) == 0.0F);
}

TEST_CASE("the view passes an opened wall, and a restart brings every wall back") {
    const game::MazeWorld world = defaultWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    REQUIRE_FALSE(world.interactables.levers.empty());
    const game::WallRef opens = world.interactables.levers[0].opens;
    const game::MazeCell behind = cellBehind(opens);

    // With the wall standing, the cell behind it cannot be seen from the cell in front.
    game::Discovery closedView(world.maze.width(), world.maze.height());
    game::discoverFrom(closedView, game::roundMaze(world, round), opens.cell);
    CHECK_FALSE(closedView.isDiscovered(behind.x, behind.z));

    game::pullRoundLever(round, world, 0);
    game::Discovery openView(world.maze.width(), world.maze.height());
    game::discoverFrom(openView, game::roundMaze(world, round), opens.cell);
    CHECK(openView.isDiscovered(behind.x, behind.z));

    // The step of the round uses the same walls: standing in front of the opening
    // discovers the cell behind it.
    bool flashlightOn = false;
    const glm::vec3 feet = game::cellCenter(opens.cell.x, opens.cell.z);
    game::updateRound(round, world, settings, feet, flashlightOn, STEP);
    CHECK(round.discovery.isDiscovered(behind.x, behind.z));

    // A new round on the same world: no lever pulled, the wall is back everywhere.
    const game::Round restarted = game::startRound(world, settings);
    CHECK(game::pulledLeverCount(restarted) == 0);
    CHECK(game::roundMaze(world, restarted).hasWall(opens.cell.x, opens.cell.z, opens.side));
    CHECK(game::roundObstacles(world, restarted).size() == world.colliders.size() + 1);
    CHECK(restarted.wallProgress[0] == 0.0F);
}

TEST_CASE("pull all levers opens every wall and counts them") {
    const game::MazeWorld world = defaultWorld();
    game::Round round = game::startRound(world, game::GameplaySettings{});
    const int levers = static_cast<int>(world.interactables.levers.size());
    REQUIRE(levers > 0);

    CHECK(game::pullAllLevers(round, world) == levers);
    CHECK(game::pulledLeverCount(round) == levers);
    CHECK(game::pullAllLevers(round, world) == 0);
    CHECK(game::roundObstacles(world, round).size() ==
          world.colliders.size() + 1 - static_cast<std::size_t>(levers));
}

TEST_CASE("the minimap drops an opened wall and marks levers and notes") {
    const game::MazeWorld world = defaultWorld();
    game::Round round = game::startRound(world, game::GameplaySettings{});
    const int levers = static_cast<int>(world.interactables.levers.size());
    const int notes = static_cast<int>(world.interactables.notes.size());
    REQUIRE(levers > 0);
    constexpr game::MinimapPlayer PLAYER{.position = {1.0F, 0.0F, 1.0F}, .yawDegrees = 0.0F};
    const float scale = game::minimapMetresPerPixel(world.maze, 202);

    const std::vector<game::MinimapVertex> before =
        game::buildMinimapVertices(world, round, true, PLAYER, scale);
    CHECK(countColor(before, game::MINIMAP_WALL_COLOR) ==
          static_cast<int>(world.walls.size()) * QUAD);
    CHECK(countColor(before, game::MINIMAP_LEVER_COLOR) == levers * QUAD);
    CHECK(countColor(before, game::MINIMAP_LEVER_PULLED_COLOR) == 0);
    CHECK(countColor(before, game::MINIMAP_NOTE_COLOR) == notes * QUAD);

    game::pullRoundLever(round, world, 0);
    const std::vector<game::MinimapVertex> after =
        game::buildMinimapVertices(world, round, true, PLAYER, scale);
    CHECK(countColor(after, game::MINIMAP_WALL_COLOR) ==
          (static_cast<int>(world.walls.size()) - 1) * QUAD);
    CHECK(countColor(after, game::MINIMAP_LEVER_COLOR) == (levers - 1) * QUAD);
    CHECK(countColor(after, game::MINIMAP_LEVER_PULLED_COLOR) == QUAD);

    // Nothing discovered: no lever and no note is shown.
    game::Round blind = game::startRound(world, game::GameplaySettings{});
    blind.discovery = game::Discovery(world.maze.width(), world.maze.height());
    const std::vector<game::MinimapVertex> hidden =
        game::buildMinimapVertices(world, blind, false, PLAYER, scale);
    CHECK(countColor(hidden, game::MINIMAP_LEVER_COLOR) == 0);
    CHECK(countColor(hidden, game::MINIMAP_NOTE_COLOR) == 0);
}

TEST_CASE("the picking ray starts in the eye and keeps its direction") {
    scene::Camera camera;
    const glm::vec3 eye{3.0F, 1.7F, 5.0F};
    const glm::mat4 view = camera.viewMatrix(eye);
    const glm::mat4 projection = camera.projectionMatrix(16.0F / 9.0F);
    const scene::Ray screenRay =
        scene::screenPointRay({320.0F, 500.0F}, {1280.0F, 720.0F}, glm::inverse(projection * view));

    const scene::Ray ray = game::rayFromEye(screenRay, eye);
    checkVector(ray.origin, eye);
    checkVector(ray.direction, screenRay.direction);
    // The ray from the eye passes the point of the near plane the screen ray starts in:
    // it shows the same pixel.
    const float toNear = glm::length(screenRay.origin - eye);
    checkVector(ray.origin + ray.direction * toNear, screenRay.origin);
}

TEST_CASE("every lever and note of a world is picked from the middle of its cell") {
    const game::MazeWorld world = defaultWorld();
    const game::Round round = game::startRound(world, game::GameplaySettings{});
    const std::vector<scene::Aabb> obstacles = game::roundObstacles(world, round);

    for (std::size_t i = 0; i < world.interactables.levers.size(); ++i) {
        const game::Lever& lever = world.interactables.levers[i];
        const game::PickState pick = game::pickInRound(
            rayTowards(eyeIn(lever.mount.cell), lever.position), true, world, round, obstacles);
        CHECK(pick.hasRay);
        CHECK(pick.centered);
        CHECK(pick.picked.kind == game::InteractableKind::Lever);
        CHECK(pick.picked.index == i);
        CHECK(pick.action == game::Interaction::PullLever);
    }
    for (std::size_t i = 0; i < world.interactables.notes.size(); ++i) {
        const game::Note& note = world.interactables.notes[i];
        const game::PickState pick = game::pickInRound(
            rayTowards(eyeIn(note.mount.cell), note.position), false, world, round, obstacles);
        CHECK_FALSE(pick.centered);
        CHECK(pick.picked.kind == game::InteractableKind::Note);
        CHECK(pick.picked.index == i);
        CHECK(pick.action == game::Interaction::ReadNote);
    }

    // Looking straight up picks nothing.
    const game::PickState sky =
        game::pickInRound({.origin = eyeIn(game::START_CELL), .direction = {0.0F, 1.0F, 0.0F}},
                          true, world, round, obstacles);
    CHECK(sky.picked.kind == game::InteractableKind::None);
    CHECK(sky.action == game::Interaction::None);
}

TEST_CASE("a closed wall hides what is behind it, the wall a lever opened does not") {
    game::MazeWorld world = defaultWorld();
    REQUIRE_FALSE(world.interactables.levers.empty());
    const game::WallRef opens = world.interactables.levers[0].opens;
    const game::MazeCell behind = cellBehind(opens);

    // A note by hand on the far side of the cell behind the wall, so that the wall
    // stands between it and a player in the cell in front. Only this note is kept.
    game::Note note;
    note.mount = {.cell = behind, .side = opens.side};
    note.position = game::notePosition(note.mount, 0.0F);
    note.box = game::noteBox(note.position, opens.side);
    world.interactables.notes.assign(1, note);

    // The eye stands close to the wall, so the note is within reach: 0.4 m to the cell
    // edge and 1.9 m more to the note.
    const glm::vec3 eye = eyeIn(opens.cell) + towards(opens.side) * 0.6F;
    const scene::Ray ray = rayTowards(eye, note.position);
    REQUIRE(glm::length(note.position - eye) < game::INTERACTION_REACH);

    game::Round round = game::startRound(world, game::GameplaySettings{});
    const game::PickState blocked =
        game::pickInRound(ray, true, world, round, game::roundObstacles(world, round));
    CHECK(blocked.picked.kind == game::InteractableKind::None);
    CHECK(blocked.action == game::Interaction::None);

    game::pullRoundLever(round, world, 0);
    const game::PickState open =
        game::pickInRound(ray, true, world, round, game::roundObstacles(world, round));
    CHECK(open.picked.kind == game::InteractableKind::Note);
    CHECK(open.picked.index == 0);
    CHECK(open.action == game::Interaction::ReadNote);
}

TEST_CASE("interacting pulls a lever once and tells when a wall opened") {
    const game::MazeWorld world = defaultWorld();
    game::Round round = game::startRound(world, game::GameplaySettings{});
    REQUIRE_FALSE(world.interactables.levers.empty());
    const game::Lever& lever = world.interactables.levers[0];
    const scene::Ray ray = rayTowards(eyeIn(lever.mount.cell), lever.position);

    const game::PickState pick =
        game::pickInRound(ray, true, world, round, game::roundObstacles(world, round));
    REQUIRE(pick.action == game::Interaction::PullLever);
    CHECK(game::interact(round, world, pick));
    CHECK(game::isLeverPulled(round.interactables, 0));

    // The same old PickState again: nothing happens, the round is asked, not the pick.
    CHECK_FALSE(game::interact(round, world, pick));
    CHECK(game::pulledLeverCount(round) == 1);

    // The ray still finds the pulled lever, but there is nothing to do with it.
    const game::PickState again =
        game::pickInRound(ray, true, world, round, game::roundObstacles(world, round));
    CHECK(again.picked.kind == game::InteractableKind::Lever);
    CHECK(again.action == game::Interaction::None);
    CHECK(game::interactionFor(round, again.picked) == game::Interaction::None);
}

TEST_CASE("a note opens its card, the key closes it first, and walking away closes it") {
    const game::MazeWorld world = defaultWorld();
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    REQUIRE_FALSE(world.interactables.notes.empty());
    const game::Note& note = world.interactables.notes[0];
    const glm::vec3 feet = game::cellCenter(note.mount.cell.x, note.mount.cell.z);
    bool flashlightOn = false;

    const game::PickState pick =
        game::pickInRound(rayTowards(eyeIn(note.mount.cell), note.position), true, world, round,
                          game::roundObstacles(world, round));
    REQUIRE(pick.action == game::Interaction::ReadNote);
    // Reading opens no wall.
    CHECK_FALSE(game::interact(round, world, pick));
    CHECK(round.noteOpen);
    CHECK(round.noteIndex == 0);
    CHECK_FALSE(game::openNoteText(world, round).empty());

    // With the card open the key closes it, whatever the ray points at, also nothing.
    CHECK(game::interactionFor(round, pick.picked) == game::Interaction::CloseNote);
    const game::PickState noRay = game::pickNothing(round);
    CHECK_FALSE(noRay.hasRay);
    CHECK(noRay.action == game::Interaction::CloseNote);
    game::interact(round, world, noRay);
    CHECK_FALSE(round.noteOpen);
    CHECK(game::pickNothing(round).action == game::Interaction::None);

    // Open again. Standing at the note keeps the card, walking away closes it.
    game::readNote(round, world, 0);
    game::updateRound(round, world, settings, feet, flashlightOn, STEP);
    CHECK(round.noteOpen);
    const glm::vec3 away = note.position + glm::vec3{game::NOTE_READ_DISTANCE + 0.1F, 0.0F, 0.0F};
    game::updateRound(round, world, settings, away, flashlightOn, STEP);
    CHECK_FALSE(round.noteOpen);

    // A note the maze does not have is ignored.
    game::readNote(round, world, world.interactables.notes.size());
    CHECK_FALSE(round.noteOpen);
}

TEST_CASE("the card of a crystal hint counts only the crystals that are left") {
    game::MazeWorld world = defaultWorld();
    REQUIRE(world.crystals.size() >= 2);
    // One note by hand, of the kind that points at the nearest crystal.
    game::Note note;
    note.mount = {.cell = {.x = 5, .z = 5}, .side = game::Direction::North};
    note.kind = game::NoteKind::CrystalHint;
    world.interactables.notes.assign(1, note);

    game::Round round = game::startRound(world, game::GameplaySettings{});
    game::readNote(round, world, 0);

    // The text is the one noteText gives for the crystals that are not collected.
    std::vector<game::MazeCell> left;
    left.reserve(world.crystals.size());
    for (const game::CrystalSpawn& spawn : world.crystals) {
        left.push_back(spawn.cell);
    }
    CHECK(game::openNoteText(world, round) == game::noteText(note, world.exitCell, left));

    // Collect the first crystal: it no longer counts.
    round.crystals[0].collected = true;
    left.erase(left.begin());
    CHECK(game::openNoteText(world, round) == game::noteText(note, world.exitCell, left));

    // All collected: the note says so.
    for (game::RoundCrystal& crystal : round.crystals) {
        crystal.collected = true;
    }
    CHECK(game::openNoteText(world, round) == "You took every one. The moon will look harder.");
}

TEST_CASE("a won round has nothing to interact with and closes the card") {
    const game::MazeWorld world = defaultWorld();
    game::Round round = game::startRound(world, game::GameplaySettings{});
    REQUIRE_FALSE(world.interactables.levers.empty());

    round.state = game::RoundState::Won;
    const game::PickedInteractable lever{.kind = game::InteractableKind::Lever, .index = 0};
    CHECK(game::interactionFor(round, lever) == game::Interaction::None);
    const game::PickedInteractable note{.kind = game::InteractableKind::Note, .index = 0};
    CHECK(game::interactionFor(round, note) == game::Interaction::None);

    // A lever number the round does not have is not an error.
    round.state = game::RoundState::Playing;
    const game::PickedInteractable unknown{.kind = game::InteractableKind::Lever, .index = 99};
    CHECK(game::interactionFor(round, unknown) == game::Interaction::None);
}

TEST_CASE("every action has its prompt") {
    CHECK(game::interactionPrompt(game::Interaction::PullLever, "E") == "E: pull lever");
    CHECK(game::interactionPrompt(game::Interaction::ReadNote, "E") == "E: read note");
    CHECK(game::interactionPrompt(game::Interaction::CloseNote, "E") == "E: close");
    CHECK(game::interactionPrompt(game::Interaction::None, "E").empty());
    // The prompt names the key "use" is on, whatever the player has chosen.
    CHECK(game::interactionPrompt(game::Interaction::PullLever, "Left Shift") ==
          "Left Shift: pull lever");
}

TEST_CASE("every action has its words, without the key") {
    CHECK(game::interactionWords(game::Interaction::PullLever) == "pull lever");
    CHECK(game::interactionWords(game::Interaction::ReadNote) == "read note");
    CHECK(game::interactionWords(game::Interaction::CloseNote) == "close");
    CHECK(game::interactionWords(game::Interaction::None).empty());
}

TEST_CASE("the highlight pulses between its weakest and its strongest glow") {
    float weakest = 1.0e9F;
    float strongest = 0.0F;
    for (int i = 0; i < 600; ++i) {
        const glm::vec3 glow = game::highlightGlow(static_cast<float>(i) * STEP);
        // The red channel of HIGHLIGHT_COLOR is 1, so it is the factor itself.
        weakest = std::min(weakest, glow.r);
        strongest = std::max(strongest, glow.r);
        CHECK(glow.r >= game::HIGHLIGHT_MIN_GLOW - 0.001F);
        CHECK(glow.r <= game::HIGHLIGHT_MAX_GLOW + 0.001F);
    }
    CHECK(weakest == doctest::Approx(game::HIGHLIGHT_MIN_GLOW).epsilon(0.01));
    CHECK(strongest == doctest::Approx(game::HIGHLIGHT_MAX_GLOW).epsilon(0.01));
    // The same moment gives the same glow.
    CHECK(game::highlightGlow(1.25F) == game::highlightGlow(1.25F));
}

TEST_CASE("a model on a wall points away from the wall on every side of the cell") {
    const glm::vec3 position{5.0F, 1.2F, 7.0F};
    for (const game::Direction side : game::ALL_DIRECTIONS) {
        CAPTURE(static_cast<int>(side));
        const glm::mat4 matrix = game::mountModelMatrix(position, side);
        // The origin of the model is the point on the wall.
        checkVector(transformPoint(matrix, glm::vec3{0.0F}), position);
        // One metre along +Z of the model is one metre away from the wall.
        checkVector(transformPoint(matrix, {0.0F, 0.0F, 1.0F}), position - towards(side));
        // Up stays up.
        checkVector(transformPoint(matrix, {0.0F, 1.0F, 0.0F}),
                    position + glm::vec3{0.0F, 1.0F, 0.0F});
    }
    // On the north wall the model is not turned at all: its +X is the world +X.
    checkVector(transformPoint(game::mountModelMatrix(position, game::Direction::North),
                               {1.0F, 0.0F, 0.0F}),
                position + glm::vec3{1.0F, 0.0F, 0.0F});
}

TEST_CASE("the handle of a lever turns around its pivot: up before the pull, down after") {
    for (const game::Direction side : game::ALL_DIRECTIONS) {
        CAPTURE(static_cast<int>(side));
        game::Lever lever;
        lever.mount = {.cell = {.x = 2, .z = 3}, .side = side};
        lever.position = game::leverPosition(lever.mount, 0.5F);
        const glm::vec3 pivot = lever.position - towards(side) * game::LEVER_PIVOT_DEPTH;

        const glm::mat4 up = game::leverHandleMatrix(lever, 0.0F);
        const glm::mat4 down = game::leverHandleMatrix(lever, 1.0F);
        checkVector(transformPoint(up, glm::vec3{0.0F}), pivot);
        checkVector(transformPoint(down, glm::vec3{0.0F}), pivot);

        // A point of the crook 0.2 m from the pivot, near the turn of its hook: out of
        // the wall and up, then out and down by the same amounts (the same angle each way).
        constexpr float LENGTH = 0.2F;
        CHECK(game::LEVER_HANDLE_UP_DEGREES == -game::LEVER_HANDLE_DOWN_DEGREES);
        const float swing = glm::radians(game::LEVER_HANDLE_DOWN_DEGREES);
        const float out = LENGTH * std::cos(swing);
        const float rise = LENGTH * std::sin(swing);
        const glm::vec3 tip{0.0F, 0.0F, LENGTH};
        checkVector(transformPoint(up, tip),
                    pivot - towards(side) * out + glm::vec3{0.0F, rise, 0.0F});
        checkVector(transformPoint(down, tip),
                    pivot - towards(side) * out - glm::vec3{0.0F, rise, 0.0F});
        // Both tips stay inside the pick box of the lever.
        const scene::Aabb box = game::leverBox(lever.position, side);
        CHECK(inside(box, transformPoint(up, tip)));
        CHECK(inside(box, transformPoint(down, tip)));
    }
}

TEST_CASE("the rope of a lever is slack until the handle is down") {
    CHECK_FALSE(game::leverRopeTaut(0.0F));
    CHECK_FALSE(game::leverRopeTaut(0.5F));
    CHECK(game::leverRopeTaut(game::LEVER_ROPE_TAUT_PROGRESS));
    CHECK(game::leverRopeTaut(1.0F));
}

TEST_CASE("the slab rings hang on both faces of the opened wall and sink with it") {
    // The real hills are not needed: four heights, tiled over the maze, are not level.
    const game::Heightmap heightmap{.width = 2, .height = 2, .values = {0.0F, 1.0F, 0.6F, 0.2F}};
    const game::MazeWorld world =
        game::buildMazeWorld(10, 10, 1U, heightmap, 2.0F, game::InteractableSettings{});
    REQUIRE_FALSE(world.interactables.levers.empty());
    game::Round round = game::startRound(world, game::GameplaySettings{});

    const game::WallSegment& wall = world.walls[world.leverWalls[0]];
    const std::array<game::WallRef, 2> mounts = game::slabRingMounts(world.interactables.levers[0]);
    const std::array<glm::mat4, 2> hanging = game::slabRingMatrices(world, round, 0);
    for (std::size_t face = 0; face < hanging.size(); ++face) {
        CAPTURE(face);
        const glm::vec3 place = transformPoint(hanging[face], glm::vec3{0.0F});
        // On the face of that wall: half of its thickness from its middle line, towards
        // the cell the face looks into, and in the middle of its length.
        checkVector({place.x, 0.0F, place.z},
                    glm::vec3{wall.position.x, 0.0F, wall.position.z} -
                        towards(mounts[face].side) * (game::WALL_VISUAL_THICKNESS / 2.0F));
        // At the foot, above the ground under the ring itself.
        CHECK(place.y ==
              doctest::Approx(world.terrain.heightAt(place.x, place.z) + game::SLAB_RING_HEIGHT));
        // It looks away from the wall, into its cell.
        checkVector(transformPoint(hanging[face], {0.0F, 0.0F, 1.0F}),
                    place - towards(mounts[face].side));
    }
    // The two rings look opposite ways.
    CHECK(mounts[0].side == game::opposite(mounts[1].side));

    // Pulled, the rings go down exactly as far as the wall does, step by step.
    const glm::vec3 wallStanding =
        transformPoint(game::roundWallMatrices(world, round)[world.leverWalls[0]], glm::vec3{0.0F});
    REQUIRE(game::pullRoundLever(round, world, 0));
    bool flashlightOn = false;
    for (int i = 0; i < 90; ++i) {
        game::updateRound(round, world, game::GameplaySettings{}, NOWHERE, flashlightOn, STEP);
    }
    const float wallDrop =
        wallStanding.y -
        transformPoint(game::roundWallMatrices(world, round)[world.leverWalls[0]], glm::vec3{0.0F})
            .y;
    CHECK(wallDrop == doctest::Approx(game::GATE_SINK_DEPTH / 2.0F).epsilon(0.001));
    const std::array<glm::mat4, 2> sinking = game::slabRingMatrices(world, round, 0);
    for (std::size_t face = 0; face < sinking.size(); ++face) {
        const glm::vec3 before = transformPoint(hanging[face], glm::vec3{0.0F});
        checkVector(transformPoint(sinking[face], glm::vec3{0.0F}),
                    before - glm::vec3{0.0F, wallDrop, 0.0F});
    }
    // The ring of another lever, whose wall still stands, has not moved.
    if (world.interactables.levers.size() > 1) {
        game::Round fresh = game::startRound(world, game::GameplaySettings{});
        CHECK(game::slabRingMatrices(world, round, 1) == game::slabRingMatrices(world, fresh, 1));
    }
    // A lever the world does not have is an error, not a ring somewhere.
    CHECK_THROWS_AS(game::slabRingMatrices(world, round, world.interactables.levers.size()),
                    std::out_of_range);
}

TEST_CASE("the models of the lever fit its pick box, and the rope ends in the ground") {
    for (const game::Direction side : game::ALL_DIRECTIONS) {
        CAPTURE(static_cast<int>(side));
        game::Lever lever;
        lever.mount = {.cell = {.x = 2, .z = 3}, .side = side};
        lever.position = game::leverPosition(lever.mount, 0.0F);
        const scene::Aabb box = game::leverBox(lever.position, side);
        // The box with a thousandth of a millimetre around it: the crook pointing
        // straight out may end close to its front.
        const scene::Aabb roomy{.min = box.min - glm::vec3{0.000001F},
                                .max = box.max + glm::vec3{0.000001F}};

        // The crook, up, half way and down.
        const assets::ObjModel handle = loadModel("crook_handle.obj");
        REQUIRE_FALSE(handle.vertices.empty());
        for (const float progress : {0.0F, 0.5F, 1.0F}) {
            const glm::mat4 matrix = game::leverHandleMatrix(lever, progress);
            for (const gfx::Vertex& vertex : handle.vertices) {
                CHECK(inside(roomy, transformPoint(matrix, vertex.position)));
            }
        }

        // The board with its straps and its pin. Only the ring under it hangs lower.
        const glm::mat4 onWall = game::mountModelMatrix(lever.position, side);
        const assets::ObjModel post = loadModel("crook_post.obj");
        REQUIRE_FALSE(post.vertices.empty());
        for (const gfx::Vertex& vertex : post.vertices) {
            glm::vec3 place = transformPoint(onWall, vertex.position);
            CHECK(vertex.position.y >= -POST_RING_BOTTOM - 0.0005F);
            place.y = std::max(place.y, box.min.y);
            CHECK(inside(roomy, place));
        }

        // Both ropes hang from the board and end under the ground the lever stands on,
        // slack and taut in the same place.
        float lowestSlack = 0.0F;
        float lowestTaut = 0.0F;
        for (const gfx::Vertex& vertex : loadModel("crook_rope_slack.obj").vertices) {
            lowestSlack = std::min(lowestSlack, transformPoint(onWall, vertex.position).y);
        }
        for (const gfx::Vertex& vertex : loadModel("crook_rope_taut.obj").vertices) {
            lowestTaut = std::min(lowestTaut, transformPoint(onWall, vertex.position).y);
        }
        CHECK(lowestSlack < -0.1F);
        CHECK(lowestTaut == doctest::Approx(lowestSlack));
    }
}

TEST_CASE("the chalk crook stays clear of the lever, on the same wall") {
    // Everything here is in the space of the lever model: x to the right along the wall,
    // z out of it.
    float crookLeft = 1.0e9F;
    float crookRight = -1.0e9F;
    const assets::ObjModel crook = loadModel("chalk_crook.obj");
    REQUIRE_FALSE(crook.vertices.empty());
    for (const gfx::Vertex& vertex : crook.vertices) {
        crookLeft = std::min(crookLeft, vertex.position.x + game::CHALK_CROOK_OFFSET);
        crookRight = std::max(crookRight, vertex.position.x + game::CHALK_CROOK_OFFSET);
    }
    // To the left of the pick box of the lever, with a gap a finger wide or more.
    CHECK(crookRight < -game::LEVER_BOX_WIDTH / 2.0F - 0.02F);
    // And so of everything that is drawn of the lever: the board, the crook, the ropes.
    for (const char* file :
         {"crook_post.obj", "crook_handle.obj", "crook_rope_slack.obj", "crook_rope_taut.obj"}) {
        CAPTURE(file);
        for (const gfx::Vertex& vertex : loadModel(file).vertices) {
            CHECK(vertex.position.x > crookRight);
        }
    }
    // On the same wall segment: not behind the pillar at its end, which covers the last
    // half of its own width.
    CHECK(crookLeft > -(game::WALL_LENGTH - game::PILLAR_SIZE) / 2.0F);
}

TEST_CASE("the chalk arrow of a hint leans along its wall, or points up and down") {
    using game::Compass;
    using game::Direction;

    // In front of the north wall the right hand is east.
    CHECK(game::chalkArrowDegrees(Compass::East, Direction::North) == 0.0F);
    CHECK(game::chalkArrowDegrees(Compass::North, Direction::North) == 90.0F);
    CHECK(game::chalkArrowDegrees(Compass::West, Direction::North) == 180.0F);
    CHECK(game::chalkArrowDegrees(Compass::South, Direction::North) == 270.0F);

    // In front of the east wall the right hand is south, in front of the south wall
    // west, and in front of the west wall north.
    CHECK(game::chalkArrowDegrees(Compass::South, Direction::East) == 0.0F);
    CHECK(game::chalkArrowDegrees(Compass::East, Direction::East) == 90.0F);
    CHECK(game::chalkArrowDegrees(Compass::West, Direction::South) == 0.0F);
    CHECK(game::chalkArrowDegrees(Compass::North, Direction::South) == 270.0F);
    CHECK(game::chalkArrowDegrees(Compass::North, Direction::West) == 0.0F);
    CHECK(game::chalkArrowDegrees(Compass::East, Direction::West) == 270.0F);

    // A diagonal leans along the wall, whether it goes through the wall or away from it.
    CHECK(game::chalkArrowDegrees(Compass::NorthEast, Direction::North) == 0.0F);
    CHECK(game::chalkArrowDegrees(Compass::SouthEast, Direction::North) == 0.0F);
    CHECK(game::chalkArrowDegrees(Compass::NorthWest, Direction::North) == 180.0F);
    CHECK(game::chalkArrowDegrees(Compass::NorthEast, Direction::East) == 180.0F);

    // What lies in the cell of the reader is behind the reader.
    for (const Direction side : game::ALL_DIRECTIONS) {
        CHECK(game::chalkArrowDegrees(Compass::Here, side) == 270.0F);
    }
}

TEST_CASE("a hint leans where its text points, and a story line leans nowhere") {
    const game::MazeCell exit{.x = 9, .z = 1};
    game::Note note;
    note.mount = {.cell = {.x = 5, .z = 5}, .side = game::Direction::North};

    note.kind = game::NoteKind::ExitHint;
    CHECK(game::noteLean(note, exit, {}) == game::Compass::NorthEast);

    note.kind = game::NoteKind::CrystalHint;
    const std::vector<game::MazeCell> crystals = {{.x = 0, .z = 5}, {.x = 5, .z = 7}};
    CHECK(game::noteLean(note, exit, crystals) == game::Compass::South);
    // No crystal is left: the card has a line of its own and nothing to point at.
    CHECK_FALSE(game::noteLean(note, exit, {}).has_value());

    note.kind = game::NoteKind::Flavour;
    CHECK_FALSE(game::noteLean(note, exit, crystals).has_value());
}
