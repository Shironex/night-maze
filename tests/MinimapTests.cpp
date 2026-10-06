// Tests of game::Minimap: where the minimap stands on the screen, how the maze is mapped
// into its picture and which shapes it is drawn from.
// See docs/modules/renderer/minimap.md
#include "game/Minimap.hpp"

#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Round.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <initializer_list>
#include <vector>

namespace {

// A rectangle and a diamond are two triangles: six vertices. The player is one triangle.
constexpr int QUAD = 6;
constexpr int TRIANGLE = 3;

// A scale at which nothing reaches its smallest size in pixels: 1 cm per pixel.
constexpr float FINE = 0.01F;

// How many vertices of the list have exactly this colour. The colours are constants
// that are copied, never computed, so they can be compared with ==.
int countColor(const std::vector<game::MinimapVertex>& vertices, const glm::vec3& color) {
    int count = 0;
    for (const game::MinimapVertex& vertex : vertices) {
        if (vertex.color == color) {
            ++count;
        }
    }
    return count;
}

// The smallest second coordinate (the most northern point) among the vertices of one
// colour. Call it only for a colour the list has.
float northernmost(const std::vector<game::MinimapVertex>& vertices, const glm::vec3& color) {
    float smallest = 1.0e9F;
    for (const game::MinimapVertex& vertex : vertices) {
        if (vertex.color == color) {
            smallest = std::min(smallest, vertex.position.y);
        }
    }
    return smallest;
}

// A place of the map in clip space, as post/minimap.vert computes it.
glm::vec4 toClip(const game::Maze& maze, const glm::vec2& mapPosition) {
    return game::minimapProjection(maze) * glm::vec4{mapPosition, 0.0F, 1.0F};
}

// A world made by hand: a corridor of three cells from west to east, the start in the
// first cell and the exit in the last one. No gate and no crystals: the tests add them.
game::MazeWorld corridorWorld() {
    game::Maze maze(3, 1);
    maze.removeWall(0, 0, game::Direction::East);
    maze.removeWall(1, 0, game::Direction::East);
    game::MazeWorld world(maze);
    world.startPosition = game::cellCenter(0, 0);
    world.exitCell = {.x = 2, .z = 0};
    return world;
}

// A round on a world made by hand, with nothing discovered.
game::Round emptyRound(const game::MazeWorld& world) {
    game::Round round;
    round.discovery = game::Discovery(world.maze.width(), world.maze.height());
    return round;
}

// A player in the first cell, looking north.
constexpr game::MinimapPlayer PLAYER{.position = {1.0F, 0.0F, 1.0F}, .yawDegrees = 0.0F};

} // namespace

TEST_CASE("the minimap settings start with the agreed values") {
    const game::MinimapSettings settings;
    CHECK(settings.enabled);
    CHECK_FALSE(settings.revealAll);
    CHECK(settings.size == 0.28F);
    CHECK(settings.margin == 0.02F);
    CHECK(settings.corner == game::MinimapCorner::BottomRight);
    CHECK(settings.opacity == 0.85F);
}

TEST_CASE("the minimap is a square in its corner, measured in framebuffer pixels") {
    game::MinimapSettings settings;

    // 0.28 of 720 is 201.6 and 0.02 of 720 is 14.4: 202 and 14 pixels.
    settings.corner = game::MinimapCorner::BottomRight;
    game::MinimapRect rect = game::minimapRect(1280, 720, settings);
    CHECK(rect.size == 202);
    CHECK(rect.x == 1280 - 14 - 202);
    CHECK(rect.y == 14);

    settings.corner = game::MinimapCorner::BottomLeft;
    rect = game::minimapRect(1280, 720, settings);
    CHECK(rect.x == 14);
    CHECK(rect.y == 14);

    // y counts from the bottom edge, so a top corner has the large y.
    settings.corner = game::MinimapCorner::TopLeft;
    rect = game::minimapRect(1280, 720, settings);
    CHECK(rect.x == 14);
    CHECK(rect.y == 720 - 14 - 202);

    settings.corner = game::MinimapCorner::TopRight;
    rect = game::minimapRect(1280, 720, settings);
    CHECK(rect.x == 1280 - 14 - 202);
    CHECK(rect.y == 720 - 14 - 202);
}

TEST_CASE("the minimap covers the same share of a Retina framebuffer") {
    const game::MinimapSettings settings;
    const game::MinimapRect normal = game::minimapRect(1280, 720, settings);
    // The same window on a Retina display: twice the pixels in each direction.
    const game::MinimapRect retina = game::minimapRect(2560, 1440, settings);

    // Twice as many pixels, give or take the one pixel of rounding.
    CHECK(std::abs(retina.size - 2 * normal.size) <= 1);
    CHECK(std::abs(retina.x - 2 * normal.x) <= 2);
    CHECK(std::abs(retina.y - 2 * normal.y) <= 1);
}

TEST_CASE("the minimap follows the height of the framebuffer, not its width") {
    const game::MinimapSettings settings;
    CHECK(game::minimapRect(1280, 720, settings).size ==
          game::minimapRect(1920, 720, settings).size);
    CHECK(game::minimapRect(1280, 1440, settings).size == 403);
}

TEST_CASE("the minimap shrinks to fit a small framebuffer and never leaves it") {
    game::MinimapSettings settings;
    settings.corner = game::MinimapCorner::BottomRight;

    // A narrow window: 202 pixels do not fit into 50. Margins of 14 leave 22.
    game::MinimapRect rect = game::minimapRect(50, 720, settings);
    CHECK(rect.size == 22);
    CHECK(rect.x == 14);
    CHECK(rect.y == 14);

    // Narrower than the two margins: the margin is given up.
    rect = game::minimapRect(20, 720, settings);
    CHECK(rect.size == 20);
    CHECK(rect.x == 0);
    CHECK(rect.y == 0);

    // One pixel, and none: a square of one pixel, and no square.
    CHECK(game::minimapRect(1, 1, settings).size == 1);
    CHECK(game::minimapRect(0, 0, settings).size == 0);
    CHECK(game::minimapRect(1280, 0, settings).size == 0);
}

TEST_CASE("settings outside their limits are brought into them") {
    game::MinimapSettings settings;
    settings.size = 5.0F;
    settings.margin = -1.0F;
    settings.corner = game::MinimapCorner::BottomLeft;

    // The largest size is 0.6 of the height, and the smallest margin is 0.
    const game::MinimapRect rect = game::minimapRect(1280, 720, settings);
    CHECK(rect.size == 432);
    CHECK(rect.x == 0);
    CHECK(rect.y == 0);
}

TEST_CASE("the picture shows a square a little larger than the longer side of the maze") {
    // 10 cells of 2 m: 20 m, half of it 10 m, and 6 percent more.
    CHECK(game::minimapHalfExtent(game::Maze(10, 10)) == doctest::Approx(10.6F));
    // The longer side decides.
    CHECK(game::minimapHalfExtent(game::Maze(4, 10)) == doctest::Approx(10.6F));
    CHECK(game::minimapHalfExtent(game::Maze(10, 4)) == doctest::Approx(10.6F));

    // 21.2 m shown in 212 pixels: 10 cm per pixel. No pixels are taken as one.
    CHECK(game::minimapMetresPerPixel(game::Maze(10, 10), 212) == doctest::Approx(0.1F));
    CHECK(game::minimapMetresPerPixel(game::Maze(10, 10), 0) == doctest::Approx(21.2F));
}

TEST_CASE("north is at the top of the map and east on the right") {
    const game::Maze maze(10, 10);
    // The share of the picture the maze fills from the middle to its edge.
    const float edge = 1.0F / 1.06F;

    // The middle of the maze is the middle of the picture.
    const glm::vec4 middle = toClip(maze, {10.0F, 10.0F});
    CHECK(middle.x == doctest::Approx(0.0F));
    CHECK(middle.y == doctest::Approx(0.0F));
    CHECK(middle.w == doctest::Approx(1.0F));

    // The north-west corner of the maze, (0, 0): left and UP, although z is smallest.
    const glm::vec4 northWest = toClip(maze, {0.0F, 0.0F});
    CHECK(northWest.x == doctest::Approx(-edge));
    CHECK(northWest.y == doctest::Approx(edge));

    // The south-east corner: right and down.
    const glm::vec4 southEast = toClip(maze, {20.0F, 20.0F});
    CHECK(southEast.x == doctest::Approx(edge));
    CHECK(southEast.y == doctest::Approx(-edge));

    // Everything of the maze is inside the picture, the square from -1 to 1.
    CHECK(edge < 1.0F);
}

TEST_CASE("a maze that is not square keeps square cells on the map") {
    // 4 by 2 cells: 8 m wide and 4 m deep. The width decides the scale.
    const game::Maze maze(4, 2);
    const float halfExtent = game::minimapHalfExtent(maze);

    const glm::vec4 eastEdge = toClip(maze, {8.0F, 2.0F});
    const glm::vec4 northEdge = toClip(maze, {4.0F, 0.0F});
    // One metre is the same distance in the picture in both directions.
    CHECK(eastEdge.x == doctest::Approx(4.0F / halfExtent));
    CHECK(northEdge.y == doctest::Approx(2.0F / halfExtent));
}

TEST_CASE("only discovered cells get a floor, and the start has its own colour") {
    const game::MazeWorld world = corridorWorld();
    game::Round round = emptyRound(world);
    round.discovery.discover(0, 0);
    round.discovery.discover(1, 0);

    const std::vector<game::MinimapVertex> vertices =
        game::buildMinimapVertices(world, round, false, PLAYER, FINE);

    // The start cell, one plain cell, and no exit: its cell is not discovered.
    CHECK(countColor(vertices, game::MINIMAP_START_COLOR) == QUAD);
    CHECK(countColor(vertices, game::MINIMAP_FLOOR_COLOR) == QUAD);
    CHECK(countColor(vertices, game::MINIMAP_EXIT_COLOR) == 0);

    // The walls of the two cells: north and south of each, and the west wall of the
    // first. The walls of the third cell are not known.
    CHECK(countColor(vertices, game::MINIMAP_WALL_COLOR) == 5 * QUAD);

    // The player, and nothing else.
    CHECK(countColor(vertices, game::MINIMAP_PLAYER_COLOR) == TRIANGLE);
    CHECK(vertices.size() == static_cast<std::size_t>(7 * QUAD + TRIANGLE));
}

TEST_CASE("with nothing discovered the map shows only the player") {
    const game::MazeWorld world = corridorWorld();
    const game::Round round = emptyRound(world);

    const std::vector<game::MinimapVertex> vertices =
        game::buildMinimapVertices(world, round, false, PLAYER, FINE);

    CHECK(vertices.size() == static_cast<std::size_t>(TRIANGLE));
    CHECK(countColor(vertices, game::MINIMAP_PLAYER_COLOR) == TRIANGLE);

    // A round that was never started has a grid without cells: the same picture.
    const std::vector<game::MinimapVertex> unstarted =
        game::buildMinimapVertices(world, game::Round{}, false, PLAYER, FINE);
    CHECK(unstarted.size() == static_cast<std::size_t>(TRIANGLE));
}

TEST_CASE("reveal all shows the whole maze without changing the discovery") {
    const game::MazeWorld world = corridorWorld();
    const game::Round round = emptyRound(world);

    const std::vector<game::MinimapVertex> vertices =
        game::buildMinimapVertices(world, round, true, PLAYER, FINE);

    CHECK(countColor(vertices, game::MINIMAP_START_COLOR) == QUAD);
    CHECK(countColor(vertices, game::MINIMAP_FLOOR_COLOR) == QUAD);
    CHECK(countColor(vertices, game::MINIMAP_EXIT_COLOR) == QUAD);
    // North and south of three cells, the west end and the east end.
    CHECK(countColor(vertices, game::MINIMAP_WALL_COLOR) == 8 * QUAD);
    CHECK(round.discovery.count() == 0);
}

TEST_CASE("a wall is drawn when one of its two cells is discovered") {
    // Two closed rooms side by side, the first one discovered.
    game::MazeWorld world(game::Maze(2, 1));
    world.exitCell = {.x = 1, .z = 0};
    game::Round round = emptyRound(world);
    round.discovery.discover(0, 0);

    const std::vector<game::MinimapVertex> vertices =
        game::buildMinimapVertices(world, round, false, PLAYER, FINE);

    // The four walls of the first room. The wall between the rooms is one of them and
    // is drawn once, although the second room is unknown.
    CHECK(countColor(vertices, game::MINIMAP_WALL_COLOR) == 4 * QUAD);
}

TEST_CASE("every wall of a generated maze is drawn exactly once when all is revealed") {
    // The default maze and the largest one the Maze panel offers.
    for (const int size : {10, 40}) {
        const game::MazeWorld world = game::buildMazeWorld(size, size, 1U);
        const game::Round round = game::startRound(world, game::GameplaySettings{});

        const std::vector<game::MinimapVertex> vertices = game::buildMinimapVertices(
            world, round, true, PLAYER, game::minimapMetresPerPixel(world.maze, 202));

        // MazeWorld::walls is the list the 3D scene is drawn from: one entry per wall.
        CHECK(countColor(vertices, game::MINIMAP_WALL_COLOR) ==
              static_cast<int>(world.walls.size()) * QUAD);
        // One floor per cell, two of them in the colours of the start and of the exit.
        CHECK(countColor(vertices, game::MINIMAP_FLOOR_COLOR) == (size * size - 2) * QUAD);
        CHECK(countColor(vertices, game::MINIMAP_START_COLOR) == QUAD);
        CHECK(countColor(vertices, game::MINIMAP_EXIT_COLOR) == QUAD);
        CHECK(countColor(vertices, game::MINIMAP_CRYSTAL_COLOR) ==
              static_cast<int>(world.crystals.size()) * QUAD);
        CHECK(countColor(vertices, game::MINIMAP_GATE_COLOR) == QUAD);
        // Whole triangles only.
        CHECK(vertices.size() % static_cast<std::size_t>(TRIANGLE) == 0);
    }
}

TEST_CASE("the gate is drawn with the exit cell, in the colour of its state") {
    game::MazeWorld world = corridorWorld();
    world.hasGate = true;
    world.gate = game::wallSegmentOn(2, 0, game::Direction::West);
    game::Round round = emptyRound(world);
    round.discovery.discover(1, 0);

    // The exit cell is unknown: no gate, although the cell in front of it is known.
    std::vector<game::MinimapVertex> vertices =
        game::buildMinimapVertices(world, round, false, PLAYER, FINE);
    CHECK(countColor(vertices, game::MINIMAP_GATE_COLOR) == 0);
    CHECK(countColor(vertices, game::MINIMAP_GATE_OPEN_COLOR) == 0);

    // Discovered, with the gate closed.
    round.discovery.discover(2, 0);
    vertices = game::buildMinimapVertices(world, round, false, PLAYER, FINE);
    CHECK(countColor(vertices, game::MINIMAP_EXIT_COLOR) == QUAD);
    CHECK(countColor(vertices, game::MINIMAP_GATE_COLOR) == QUAD);
    CHECK(countColor(vertices, game::MINIMAP_GATE_OPEN_COLOR) == 0);

    // The gate has opened.
    round.gateOpen = true;
    vertices = game::buildMinimapVertices(world, round, false, PLAYER, FINE);
    CHECK(countColor(vertices, game::MINIMAP_GATE_COLOR) == 0);
    CHECK(countColor(vertices, game::MINIMAP_GATE_OPEN_COLOR) == QUAD);
}

TEST_CASE("a crystal is drawn while it is not collected and its cell is discovered") {
    game::MazeWorld world = corridorWorld();
    world.crystals.push_back(game::CrystalSpawn{.cell = {.x = 1, .z = 0}, .variant = 0});
    world.crystals.push_back(game::CrystalSpawn{.cell = {.x = 2, .z = 0}, .variant = 1});
    game::Round round = emptyRound(world);
    round.crystals.resize(world.crystals.size());
    round.discovery.discover(1, 0);

    // Only the crystal in the discovered cell.
    std::vector<game::MinimapVertex> vertices =
        game::buildMinimapVertices(world, round, false, PLAYER, FINE);
    CHECK(countColor(vertices, game::MINIMAP_CRYSTAL_COLOR) == QUAD);

    // Both cells known: both crystals.
    round.discovery.discover(2, 0);
    vertices = game::buildMinimapVertices(world, round, false, PLAYER, FINE);
    CHECK(countColor(vertices, game::MINIMAP_CRYSTAL_COLOR) == 2 * QUAD);

    // Collected crystals leave the map.
    round.crystals[0].collected = true;
    vertices = game::buildMinimapVertices(world, round, false, PLAYER, FINE);
    CHECK(countColor(vertices, game::MINIMAP_CRYSTAL_COLOR) == QUAD);

    // A round with fewer crystals than the world (it belongs to another one): the
    // crystals it does not have are skipped, nothing is read past its list.
    round.crystals.resize(1);
    vertices = game::buildMinimapVertices(world, round, true, PLAYER, FINE);
    CHECK(countColor(vertices, game::MINIMAP_CRYSTAL_COLOR) == 0);
}

TEST_CASE("the arrow of the player is drawn last and points where the camera looks") {
    const game::MazeWorld world = corridorWorld();
    const game::Round round = emptyRound(world);
    constexpr glm::vec3 FEET{3.0F, 7.0F, 5.0F};
    // The arrow is 0.9 m long from the place of the player to its tip.
    constexpr float LENGTH = 0.9F;

    // The three vertices of the player are the last ones, the tip first. Yaw 0 looks
    // north: towards smaller z, the second coordinate of the map. The height of the
    // feet (7 m) plays no part.
    std::vector<game::MinimapVertex> vertices = game::buildMinimapVertices(
        world, round, true, {.position = FEET, .yawDegrees = 0.0F}, FINE);
    game::MinimapVertex tip = vertices[vertices.size() - TRIANGLE];
    CHECK(tip.color == game::MINIMAP_PLAYER_COLOR);
    CHECK(tip.position.x == doctest::Approx(3.0F));
    CHECK(tip.position.y == doctest::Approx(5.0F - LENGTH));

    // Yaw 90 looks east: towards larger x.
    vertices = game::buildMinimapVertices(world, round, true,
                                          {.position = FEET, .yawDegrees = 90.0F}, FINE);
    tip = vertices[vertices.size() - TRIANGLE];
    CHECK(tip.position.x == doctest::Approx(3.0F + LENGTH));
    CHECK(tip.position.y == doctest::Approx(5.0F));

    // Yaw 180 looks south: towards larger z.
    vertices = game::buildMinimapVertices(world, round, true,
                                          {.position = FEET, .yawDegrees = 180.0F}, FINE);
    tip = vertices[vertices.size() - TRIANGLE];
    CHECK(tip.position.x == doctest::Approx(3.0F));
    CHECK(tip.position.y == doctest::Approx(5.0F + LENGTH));
}

TEST_CASE("thin shapes keep a smallest size in pixels on the map of a large maze") {
    // One closed room: its north wall lies on z = 0, half of its thickness to each side.
    const game::MazeWorld world(game::Maze(1, 1));
    const game::Round round = emptyRound(world);

    // Fine picture: the wall has its own thickness of 0.3 m.
    std::vector<game::MinimapVertex> vertices =
        game::buildMinimapVertices(world, round, true, PLAYER, FINE);
    CHECK(northernmost(vertices, game::MINIMAP_WALL_COLOR) == doctest::Approx(-0.15F));

    // Coarse picture, 1 m per pixel: 0.3 m would be a third of a pixel. The wall is
    // 1.5 pixels thick instead, 1.5 m.
    constexpr float COARSE = 1.0F;
    vertices = game::buildMinimapVertices(world, round, true, PLAYER, COARSE);
    CHECK(northernmost(vertices, game::MINIMAP_WALL_COLOR) == doctest::Approx(-0.75F));

    // The arrow of the player is 5 pixels long there, 5 m, instead of 0.9 m.
    const game::MinimapVertex tip = vertices[vertices.size() - TRIANGLE];
    CHECK(tip.position.y == doctest::Approx(PLAYER.position.z - 5.0F));
}

TEST_CASE("the map of the largest maze can be built") {
    // game::Maze::MAX_SIZE cells in each direction, far more than the game offers. The
    // map is unreadable at that size, but building it must not fail.
    const game::Maze maze(game::Maze::MAX_SIZE, game::Maze::MAX_SIZE);
    const game::MazeWorld world(maze);
    const game::Round round = emptyRound(world);

    const std::vector<game::MinimapVertex> vertices = game::buildMinimapVertices(
        world, round, true, PLAYER, game::minimapMetresPerPixel(maze, 202));

    // One floor per cell. The start cell (0, 0) is also the exit cell of this bare world.
    const int cells = game::Maze::MAX_SIZE * game::Maze::MAX_SIZE;
    CHECK(countColor(vertices, game::MINIMAP_FLOOR_COLOR) == (cells - 1) * QUAD);
    CHECK(countColor(vertices, game::MINIMAP_EXIT_COLOR) == QUAD);
}
