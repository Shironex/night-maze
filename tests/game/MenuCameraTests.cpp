// Tests of game::MenuCamera: the route, the path in the corridors and the pose over time.
#include "game/MenuCamera.hpp"

#include "game/Exit.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Camera.hpp"
#include "scene/Collider.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <span>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {

// The seeds the tests of whole worlds run with.
constexpr std::array<std::uint32_t, 4> SEEDS = {1, 2, 7, 1234};

// The start cell of every maze of the game (see MazeWorld.hpp).
constexpr game::MazeCell START = game::START_CELL;

// The step of time most tests of the pose use, in seconds: one frame at 60 pictures
// per second.
constexpr float FRAME_SECONDS = 1.0F / 60.0F;

// A heightmap with four different values, so the ground of the maze is not flat.
game::Heightmap roughHeightmap() {
    return {.width = 2, .height = 2, .values = {0.0F, 1.0F, 0.6F, 0.2F}};
}

// A maze of the default size on uneven ground.
game::MazeWorld worldOf(std::uint32_t seed) {
    return game::buildMazeWorld(game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, seed,
                                roughHeightmap(), 1.0F);
}

// The direction of the step from one cell to the next, or false when the two are not
// neighbours.
bool stepBetween(game::MazeCell from, game::MazeCell to, game::Direction& direction) {
    for (const game::Direction candidate : game::ALL_DIRECTIONS) {
        if (from.x + game::columnStep(candidate) == to.x &&
            from.z + game::rowStep(candidate) == to.z) {
            direction = candidate;
            return true;
        }
    }
    return false;
}

// Checks that a route is a closed walk through open passages: every step, also the one
// from the last cell back to the first, leads to a neighbour with no wall in between.
void checkClosedWalk(const game::Maze& maze, const std::vector<game::MazeCell>& route) {
    REQUIRE_FALSE(route.empty());
    if (route.size() == 1) {
        return;
    }
    for (std::size_t i = 0; i < route.size(); ++i) {
        const game::MazeCell from = route[i];
        const game::MazeCell to = route[(i + 1) % route.size()];
        game::Direction direction = game::Direction::North;
        REQUIRE(stepBetween(from, to, direction));
        CHECK_FALSE(maze.hasWall(from.x, from.z, direction));
    }
}

// The unit vector a pose looks along, computed the way the game does it.
glm::vec3 forwardOf(const game::MenuCameraPose& pose) {
    scene::Camera camera;
    camera.yawDegrees = pose.yawDegrees;
    camera.pitchDegrees = pose.pitchDegrees;
    return camera.forward();
}

// The angle between the views of two poses, in degrees.
float degreesBetween(const game::MenuCameraPose& a, const game::MenuCameraPose& b) {
    const float cosine = std::clamp(glm::dot(forwardOf(a), forwardOf(b)), -1.0F, 1.0F);
    return glm::degrees(std::acos(cosine));
}

// The distance on the ground (x and z only) from a point to a box: 0 inside of it.
float groundDistance(const glm::vec3& point, const scene::Aabb& box) {
    const float dx = std::max({box.min.x - point.x, 0.0F, point.x - box.max.x});
    const float dz = std::max({box.min.z - point.z, 0.0F, point.z - box.max.z});
    return std::sqrt(dx * dx + dz * dz);
}

} // namespace

TEST_CASE("the menu camera is off by default and starts with the corridor walk") {
    const game::MenuCameraSettings settings;
    CHECK_FALSE(settings.enabled);
    CHECK(settings.shot == game::MenuShot::CorridorWalk);
    CHECK(settings.speed == game::DEFAULT_MENU_CAMERA_SPEED);
    CHECK(settings.eyeHeight == game::DEFAULT_MENU_CAMERA_EYE_HEIGHT);
    CHECK(settings.timeOffset == 0.0F);
}

TEST_CASE("the route is a closed walk from the start that visits every target") {
    for (const std::uint32_t seed : SEEDS) {
        const game::MazeWorld world = worldOf(seed);
        const std::vector<game::MazeCell> targets = game::menuCameraTargets(world);
        const std::vector<game::MazeCell> route = game::menuCameraRoute(world.maze, START, targets);

        REQUIRE(route.size() > 1);
        CHECK(route.front() == START);
        checkClosedWalk(world.maze, route);
        for (const game::MazeCell target : targets) {
            CHECK(std::ranges::find(route, target) != route.end());
        }
    }
}

TEST_CASE("the targets are the crystals and the cell in front of the gate, not the exit") {
    const game::MazeWorld world = worldOf(1);
    const std::vector<game::MazeCell> targets = game::menuCameraTargets(world);
    REQUIRE(world.hasGate);
    REQUIRE(targets.size() == world.crystals.size() + 1);

    // The last target is a neighbour of the exit cell with no wall in between: the
    // closed gate stands on that side.
    const game::MazeCell beforeGate = targets.back();
    game::Direction side = game::Direction::North;
    REQUIRE(stepBetween(world.exitCell, beforeGate, side));
    CHECK_FALSE(world.maze.hasWall(world.exitCell.x, world.exitCell.z, side));

    // The route never enters the exit cell: the gate is closed.
    const std::vector<game::MazeCell> route = game::menuCameraRoute(world.maze, START, targets);
    CHECK(std::ranges::find(route, world.exitCell) == route.end());
}

TEST_CASE("the route uses every passage of its tree once in each direction") {
    const game::MazeWorld world = worldOf(7);
    const std::vector<game::MazeCell> route =
        game::menuCameraRoute(world.maze, START, game::menuCameraTargets(world));

    // Every step as (from x, from z, to x, to z). A set keeps each one once.
    std::set<std::tuple<int, int, int, int>> steps;
    for (std::size_t i = 0; i < route.size(); ++i) {
        const game::MazeCell from = route[i];
        const game::MazeCell to = route[(i + 1) % route.size()];
        steps.emplace(from.x, from.z, to.x, to.z);
    }
    // No step is taken twice, and every step has its way back.
    CHECK(steps.size() == route.size());
    for (const auto& [fromX, fromZ, toX, toZ] : steps) {
        CHECK(steps.contains({toX, toZ, fromX, fromZ}));
    }
}

TEST_CASE("a route without a target to go to is the start alone") {
    // Two cells with the wall between them still standing.
    const game::Maze maze(2, 1);
    const std::vector<game::MazeCell> noTargets;
    CHECK(game::menuCameraRoute(maze, START, noTargets) == std::vector<game::MazeCell>{START});

    // A target behind the wall and one outside of the maze are left out.
    const std::vector<game::MazeCell> unreachable = {{.x = 1, .z = 0}, {.x = 5, .z = 5}};
    CHECK(game::menuCameraRoute(maze, START, unreachable) == std::vector<game::MazeCell>{START});

    // The start itself is no way to walk either.
    const std::vector<game::MazeCell> onlyStart = {START};
    CHECK(game::menuCameraRoute(maze, START, onlyStart) == std::vector<game::MazeCell>{START});
}

TEST_CASE("a start outside of the maze is an error") {
    const game::Maze maze(2, 2);
    const std::vector<game::MazeCell> noTargets;
    CHECK_THROWS_AS(game::menuCameraRoute(maze, {.x = 2, .z = 0}, noTargets), std::out_of_range);
}

TEST_CASE("in a corridor the route walks to the target and back") {
    // Three cells in a row, joined into one corridor.
    game::Maze maze(3, 1);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::East);

    const std::vector<game::MazeCell> targets = {{.x = 2, .z = 0}};
    const std::vector<game::MazeCell> expected = {
        {.x = 0, .z = 0}, {.x = 1, .z = 0}, {.x = 2, .z = 0}, {.x = 1, .z = 0}};
    CHECK(game::menuCameraRoute(maze, START, targets) == expected);
}

TEST_CASE("a maze with two ways between cells still gives a closed route") {
    // Four cells in a square with every wall between them removed: a ring.
    game::Maze maze(2, 2);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(0, 0, game::Direction::South);
    maze.removeWall(1, 1, game::Direction::North);
    maze.removeWall(1, 1, game::Direction::West);

    const std::vector<game::MazeCell> targets = {{.x = 1, .z = 1}, {.x = 1, .z = 0}};
    const std::vector<game::MazeCell> route = game::menuCameraRoute(maze, START, targets);
    checkClosedWalk(maze, route);
    for (const game::MazeCell target : targets) {
        CHECK(std::ranges::find(route, target) != route.end());
    }
}

TEST_CASE("the path never passes through a wall") {
    for (const std::uint32_t seed : SEEDS) {
        const game::MazeWorld world = worldOf(seed);
        const game::MenuCameraPath path = game::buildMenuCameraPath(world);
        REQUIRE(path.points.size() > 1);

        // Every piece of the path, also the one that closes the loop, stays in one cell
        // or crosses into a neighbour through an open side. The pieces are a few
        // centimetres long, so a piece cannot skip a cell.
        for (std::size_t i = 0; i < path.points.size(); ++i) {
            const game::MazeCell from = game::cellAt(path.points[i]);
            const game::MazeCell to = game::cellAt(path.points[(i + 1) % path.points.size()]);
            REQUIRE(world.maze.contains(from.x, from.z));
            if (from == to) {
                continue;
            }
            game::Direction direction = game::Direction::North;
            REQUIRE(stepBetween(from, to, direction));
            CHECK_FALSE(world.maze.hasWall(from.x, from.z, direction));
        }
    }
}

TEST_CASE("the path keeps its distance from every wall, every pillar and the gate") {
    // The lane lies 0.2 m from the middle of a corridor and the face of a wall box
    // 0.85 m, so 0.65 m is what stays in a straight corridor. A corner that is cut
    // short comes a little nearer to the pillar on its inside. 0.5 m is five times
    // the near plane of the camera (0.1 m), so nothing is cut open.
    constexpr float MIN_CLEARANCE = 0.5F;

    for (const std::uint32_t seed : SEEDS) {
        const game::MazeWorld world = worldOf(seed);
        const game::MenuCameraPath path = game::buildMenuCameraPath(world);

        // The stile is the last box of the list, and the one exception: its steps reach
        // 0.31 m further into the start cell than the box of a wall, and the lane passes
        // them at 0.34 m. That is still three times the near plane.
        constexpr float MIN_STILE_CLEARANCE = 0.3F;
        REQUIRE(world.stileWall.has_value());
        const std::span<const scene::Aabb> boxes{world.colliders};
        // The stone sheep are the other exception. One that lies along a wall on the
        // side of the lane is passed at 0.25 m, and it is 0.6 m high: the camera looks
        // over it from the height of the eyes.
        constexpr float MIN_SHEEP_CLEARANCE = 0.2F;
        const std::size_t wallsAndPillars = world.walls.size() + world.pillars.size();
        REQUIRE(boxes.size() == wallsAndPillars + world.sheep.size() + 1U);

        float nearest = MIN_CLEARANCE * 10.0F;
        float nearestToStile = nearest;
        float nearestToSheep = nearest;
        for (const glm::vec3& point : path.points) {
            for (const scene::Aabb& box : boxes.first(wallsAndPillars)) {
                nearest = std::min(nearest, groundDistance(point, box));
            }
            for (const game::StoneSheep& sheep : world.sheep) {
                nearestToSheep = std::min(nearestToSheep, groundDistance(point, sheep.box));
            }
            nearest = std::min(nearest, groundDistance(point, world.gateBox));
            nearestToStile = std::min(nearestToStile, groundDistance(point, boxes.back()));
        }
        CHECK(nearest >= MIN_CLEARANCE);
        CHECK(nearestToStile >= MIN_STILE_CLEARANCE);
        CHECK(nearestToSheep >= MIN_SHEEP_CLEARANCE);
    }
}

TEST_CASE("the points of the path lie on the ground, close together, and add up") {
    const game::MazeWorld world = worldOf(1);
    const game::MenuCameraPath path = game::buildMenuCameraPath(world);
    REQUIRE(path.points.size() == path.distances.size());
    REQUIRE(path.points.size() == path.yawDegrees.size());
    CHECK(path.distances.front() == 0.0F);

    // The spacing is measured on the ground. A piece on a slope is a little longer.
    constexpr float SLOPE_ALLOWANCE = 1.1F;
    float length = 0.0F;
    for (std::size_t i = 0; i < path.points.size(); ++i) {
        const glm::vec3& point = path.points[i];
        const glm::vec3& next = path.points[(i + 1) % path.points.size()];
        CHECK(point.y == doctest::Approx(world.terrain.heightAt(point.x, point.z)));
        CHECK(path.distances[i] == doctest::Approx(length));

        const float piece = glm::distance(point, next);
        CHECK(piece > 0.0F);
        CHECK(piece <= game::MENU_CAMERA_SAMPLE_SPACING * SLOPE_ALLOWANCE);
        length += piece;
    }
    CHECK(path.length == doctest::Approx(length));
}

TEST_CASE("a point of the path is found by its distance, around the loop and backwards") {
    const game::MazeWorld world = worldOf(1);
    const game::MenuCameraPath path = game::buildMenuCameraPath(world);
    constexpr float NEAR = 0.001F;

    CHECK(glm::distance(game::menuCameraPathPoint(path, 0.0F), path.points.front()) < NEAR);
    // A distance that is stored gives its point.
    const std::size_t middle = path.points.size() / 2;
    CHECK(glm::distance(game::menuCameraPathPoint(path, path.distances[middle]),
                        path.points[middle]) < NEAR);
    // One loop later the place is the same, and a negative distance counts back.
    CHECK(glm::distance(game::menuCameraPathPoint(path, 3.0F + path.length),
                        game::menuCameraPathPoint(path, 3.0F)) < NEAR);
    CHECK(glm::distance(game::menuCameraPathPoint(path, -2.0F),
                        game::menuCameraPathPoint(path, path.length - 2.0F)) < NEAR);

    // A path without points gives the origin.
    const game::MenuCameraPath empty;
    CHECK(game::menuCameraPathPoint(empty, 5.0F) == glm::vec3{0.0F});
}

TEST_CASE("the same world gives the same path and the same pose") {
    const game::MazeWorld first = worldOf(7);
    const game::MazeWorld second = worldOf(7);
    const game::MenuCameraPath pathA = game::buildMenuCameraPath(first);
    const game::MenuCameraPath pathB = game::buildMenuCameraPath(second);

    REQUIRE(pathA.points.size() == pathB.points.size());
    CHECK(pathA.points == pathB.points);
    CHECK(pathA.distances == pathB.distances);
    CHECK(pathA.yawDegrees == pathB.yawDegrees);
    CHECK(pathA.length == pathB.length);

    const game::MenuCameraSettings settings;
    const game::MenuCameraPose poseA = game::menuCameraPose(pathA, first, settings, 12.3F);
    const game::MenuCameraPose poseB = game::menuCameraPose(pathB, second, settings, 12.3F);
    CHECK(poseA.eye == poseB.eye);
    CHECK(poseA.yawDegrees == poseB.yawDegrees);
    CHECK(poseA.pitchDegrees == poseB.pitchDegrees);

    // Another seed is another maze and another path.
    const game::MenuCameraPath other = game::buildMenuCameraPath(worldOf(2));
    CHECK(other.points != pathA.points);
}

TEST_CASE("the corridor walk moves at the speed of the settings, everywhere") {
    const game::MazeWorld world = worldOf(1);
    const game::MenuCameraPath path = game::buildMenuCameraPath(world);
    game::MenuCameraSettings settings;
    settings.speed = 1.25F;

    // The way between two frames is the speed times the time between them. Around
    // a corner the straight line between the two places is a little shorter than the
    // arc, by less than one part in a hundred.
    constexpr float TOLERANCE = 0.01F;
    const float expected = settings.speed * FRAME_SECONDS;
    const float loopSeconds = game::menuCameraLoopSeconds(path, world, settings);
    // A step that is no multiple of a frame, so the samples fall on all kinds of places.
    constexpr float SAMPLE_STEP_SECONDS = 0.37F;
    for (float seconds = 0.0F; seconds < loopSeconds; seconds += SAMPLE_STEP_SECONDS) {
        const glm::vec3 here = game::menuCameraPose(path, world, settings, seconds).eye;
        const glm::vec3 there =
            game::menuCameraPose(path, world, settings, seconds + FRAME_SECONDS).eye;
        CHECK(glm::distance(here, there) == doctest::Approx(expected).epsilon(TOLERANCE));
    }
}

TEST_CASE("the corridor walk stands at eye height above the path") {
    const game::MazeWorld world = worldOf(1);
    const game::MenuCameraPath path = game::buildMenuCameraPath(world);
    game::MenuCameraSettings settings;
    settings.eyeHeight = 1.1F;
    settings.speed = 2.0F;

    const game::MenuCameraPose pose = game::menuCameraPose(path, world, settings, 4.0F);
    const glm::vec3 onPath = game::menuCameraPathPoint(path, 8.0F);
    CHECK(pose.eye.x == doctest::Approx(onPath.x));
    CHECK(pose.eye.y == doctest::Approx(onPath.y + 1.1F));
    CHECK(pose.eye.z == doctest::Approx(onPath.z));
    // The yaw is in the range scene::Camera keeps it in.
    CHECK(pose.yawDegrees >= 0.0F);
    CHECK(pose.yawDegrees < 360.0F);
}

TEST_CASE("the time offset moves the shot along its loop") {
    const game::MazeWorld world = worldOf(1);
    const game::MenuCameraPath path = game::buildMenuCameraPath(world);
    game::MenuCameraSettings later;
    later.timeOffset = 30.0F;
    const game::MenuCameraSettings now;

    for (const game::MenuShot shot : {game::MenuShot::CorridorWalk, game::MenuShot::HighGlide}) {
        later.shot = shot;
        game::MenuCameraSettings plain = now;
        plain.shot = shot;
        const game::MenuCameraPose shifted = game::menuCameraPose(path, world, later, 5.0F);
        const game::MenuCameraPose direct = game::menuCameraPose(path, world, plain, 35.0F);
        CHECK(glm::distance(shifted.eye, direct.eye) < 0.001F);
        CHECK(degreesBetween(shifted, direct) < 0.1F);
    }
}

TEST_CASE("the view of the corridor walk never turns suddenly") {
    // A turn around, half a turn of the view, is spread over 4.5 m of the path
    // (LOOK_WINDOW_METRES in MenuCamera.cpp). At the default speed that is more than
    // 6 seconds, so about 28 degrees per second. A corner right before or after it and
    // the sway add to that. 50 degrees per second is the limit: a slow pan, nothing
    // that looks like a jump.
    constexpr float MAX_DEGREES_PER_SECOND = 50.0F;

    for (const std::uint32_t seed : SEEDS) {
        const game::MazeWorld world = worldOf(seed);
        const game::MenuCameraPath path = game::buildMenuCameraPath(world);
        const game::MenuCameraSettings settings;
        const float loopSeconds = game::menuCameraLoopSeconds(path, world, settings);

        float fastest = 0.0F;
        game::MenuCameraPose previous = game::menuCameraPose(path, world, settings, 0.0F);
        // Frame after frame, once around the loop and a little past its end.
        for (float seconds = FRAME_SECONDS; seconds < loopSeconds + 1.0F;
             seconds += FRAME_SECONDS) {
            const game::MenuCameraPose pose = game::menuCameraPose(path, world, settings, seconds);
            fastest = std::max(fastest, degreesBetween(previous, pose) / FRAME_SECONDS);
            previous = pose;
        }
        CHECK(fastest <= MAX_DEGREES_PER_SECOND);
    }
}

TEST_CASE("both shots are loops: after one loop the camera is where it began") {
    const game::MazeWorld world = worldOf(2);
    const game::MenuCameraPath path = game::buildMenuCameraPath(world);

    for (const game::MenuShot shot : {game::MenuShot::CorridorWalk, game::MenuShot::HighGlide}) {
        game::MenuCameraSettings settings;
        settings.shot = shot;
        const float loopSeconds = game::menuCameraLoopSeconds(path, world, settings);
        REQUIRE(loopSeconds > 0.0F);

        // The same moment one loop later.
        constexpr float MOMENT_SECONDS = 3.0F;
        const game::MenuCameraPose early =
            game::menuCameraPose(path, world, settings, MOMENT_SECONDS);
        const game::MenuCameraPose late =
            game::menuCameraPose(path, world, settings, MOMENT_SECONDS + loopSeconds);
        CHECK(glm::distance(early.eye, late.eye) < 0.01F);
        CHECK(degreesBetween(early, late) < 0.1F);

        // Across the point where the loop closes: the last frame before it and the
        // first frame after it are as close as any two frames.
        const game::MenuCameraPose before =
            game::menuCameraPose(path, world, settings, loopSeconds - FRAME_SECONDS / 2.0F);
        const game::MenuCameraPose after =
            game::menuCameraPose(path, world, settings, FRAME_SECONDS / 2.0F);
        // The glide is faster than the walk at the same setting, never more than 3 times.
        constexpr float FASTEST_SPEED_SCALE = 3.0F;
        CHECK(glm::distance(before.eye, after.eye) <=
              settings.speed * FASTEST_SPEED_SCALE * FRAME_SECONDS * 1.01F);
        CHECK(degreesBetween(before, after) < 1.0F);
    }
}

TEST_CASE("the yaw of the path changes by whole turns per loop") {
    for (const std::uint32_t seed : SEEDS) {
        const game::MenuCameraPath path = game::buildMenuCameraPath(worldOf(seed));
        // The camera looks the same way again when the loop closes.
        const float turns = path.turnDegrees / 360.0F;
        CHECK(turns == doctest::Approx(std::round(turns)).epsilon(0.001));
    }
}

TEST_CASE("a speed of zero lets the camera stand still") {
    const game::MazeWorld world = worldOf(1);
    const game::MenuCameraPath path = game::buildMenuCameraPath(world);
    game::MenuCameraSettings settings;
    settings.speed = 0.0F;

    CHECK(game::menuCameraLoopSeconds(path, world, settings) == 0.0F);
    const game::MenuCameraPose first = game::menuCameraPose(path, world, settings, 1.0F);
    const game::MenuCameraPose second = game::menuCameraPose(path, world, settings, 50.0F);
    CHECK(first.eye == second.eye);
    CHECK(first.yawDegrees == second.yawDegrees);
}

TEST_CASE("a maze of one cell gives a small circle inside of that cell") {
    const game::MazeWorld world = game::buildMazeWorld(1, 1, 1);
    CHECK(game::menuCameraTargets(world).empty());

    const game::MenuCameraPath path = game::buildMenuCameraPath(world);
    REQUIRE(path.points.size() > 2);
    const glm::vec3 center = game::cellCenter(0, 0);
    for (const glm::vec3& point : path.points) {
        CHECK(glm::distance(point, center) ==
              doctest::Approx(game::MENU_CAMERA_LANE_OFFSET).epsilon(0.01));
    }
    // Clockwise seen from above: one turn to the right.
    CHECK(path.turnDegrees == doctest::Approx(360.0F).epsilon(0.001));
    // The whole circle is as long as its radius says.
    CHECK(path.length ==
          doctest::Approx(2.0F * 3.14159265F * game::MENU_CAMERA_LANE_OFFSET).epsilon(0.01));

    // Both shots give numbers, not NaN, at any moment.
    for (const game::MenuShot shot : {game::MenuShot::CorridorWalk, game::MenuShot::HighGlide}) {
        game::MenuCameraSettings settings;
        settings.shot = shot;
        for (const float seconds : {0.0F, 0.5F, 17.0F, 1000.0F}) {
            const game::MenuCameraPose pose = game::menuCameraPose(path, world, settings, seconds);
            CHECK(std::isfinite(pose.eye.x));
            CHECK(std::isfinite(pose.eye.y));
            CHECK(std::isfinite(pose.eye.z));
            CHECK(std::isfinite(pose.yawDegrees));
            CHECK(std::isfinite(pose.pitchDegrees));
        }
    }
}

TEST_CASE("a path without points gives the pose a new camera has") {
    const game::MazeWorld world = worldOf(1);
    const game::MenuCameraPath empty;
    const game::MenuCameraSettings settings;
    const game::MenuCameraPose pose = game::menuCameraPose(empty, world, settings, 3.0F);
    CHECK(pose.eye == glm::vec3{0.0F});
    CHECK(pose.yawDegrees == 0.0F);
    CHECK(pose.pitchDegrees == 0.0F);
    CHECK(game::menuCameraLoopSeconds(empty, world, settings) == 0.0F);
}

TEST_CASE("the high glide flies above the walls and looks down at the middle of the maze") {
    for (const std::uint32_t seed : SEEDS) {
        const game::MazeWorld world = worldOf(seed);
        const game::MenuCameraPath path = game::buildMenuCameraPath(world);
        game::MenuCameraSettings settings;
        settings.shot = game::MenuShot::HighGlide;

        const float width = static_cast<float>(world.maze.width()) * game::CELL_SIZE;
        const float depth = static_cast<float>(world.maze.height()) * game::CELL_SIZE;
        const glm::vec3 middle{width / 2.0F, 0.0F, depth / 2.0F};

        const float loopSeconds = game::menuCameraLoopSeconds(path, world, settings);
        constexpr int MOMENTS = 16;
        float radius = 0.0F;
        for (int i = 0; i < MOMENTS; ++i) {
            const float seconds = loopSeconds * static_cast<float>(i) / static_cast<float>(MOMENTS);
            const game::MenuCameraPose pose = game::menuCameraPose(path, world, settings, seconds);

            // Above the tops of the walls and above the ground it flies over.
            const float ground = world.terrain.heightAt(pose.eye.x, pose.eye.z);
            CHECK(pose.eye.y > ground + game::WALL_HEIGHT);

            // Always the same distance from the middle, on the ground.
            const glm::vec3 toMiddle = middle - glm::vec3{pose.eye.x, 0.0F, pose.eye.z};
            const float distance = glm::length(toMiddle);
            if (i == 0) {
                radius = distance;
            }
            CHECK(distance == doctest::Approx(radius));

            // It looks down, and on the ground towards the middle.
            const glm::vec3 forward = forwardOf(pose);
            CHECK(pose.pitchDegrees < 0.0F);
            const glm::vec3 level = glm::normalize(glm::vec3{forward.x, 0.0F, forward.z});
            CHECK(glm::dot(level, toMiddle / distance) == doctest::Approx(1.0F));
        }
    }
}
