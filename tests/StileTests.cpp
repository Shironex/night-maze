// Tests of game::Stile: which wall of the start cell carries the stile, where its
// collision box stands and what it must never be in the way of, and the bare ground.
#include "game/Stile.hpp"

#include "assets/ObjLoader.hpp"
#include "game/Difficulty.hpp"
#include "game/Flasks.hpp"
#include "game/GateLamp.hpp"
#include "game/Grass.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Puddles.hpp"
#include "game/WallVariants.hpp"
#include "scene/Collider.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace {

// A size of a maze in cells.
struct Size {
    int width;
    int height;
};

// The sizes the tests build: the smallest mazes there are (in a maze of two cells the
// start cell is also the cell in front of the gate), a few odd ones, and the three sizes
// of the levels of the game: 10 by 10, 16 by 16 and 22 by 22.
constexpr std::array<Size, 12> SIZES = {{
    {.width = 1, .height = 1},
    {.width = 2, .height = 1},
    {.width = 1, .height = 2},
    {.width = 2, .height = 2},
    {.width = 3, .height = 2},
    {.width = 2, .height = 5},
    {.width = 4, .height = 4},
    {.width = 7, .height = 3},
    {.width = 6, .height = 6},
    {.width = 10, .height = 10},
    {.width = 16, .height = 16},
    {.width = 22, .height = 22},
}};

// Every size is built with the seeds 0 to SEED_COUNT - 1.
constexpr std::uint32_t SEED_COUNT = 120;

// Room for the rounding of a float: two faces that are meant to lie in one plane are
// computed in different ways and may differ in the last bit.
constexpr float SLACK = 0.0001F;

// The box made smaller by SLACK on every side: what it must not share with anything.
scene::Aabb inside(const scene::Aabb& box) {
    return {.min = box.min + glm::vec3{SLACK}, .max = box.max - glm::vec3{SLACK}};
}

// Calls check(world) for every size and seed. The world stands on flat ground.
template <typename Check>
void forEveryWorld(Check check) {
    for (const Size& size : SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size.width);
            CAPTURE(size.height);
            CAPTURE(seed);
            check(game::buildMazeWorld(size.width, size.height, seed));
        }
    }
}

// The box of the stile of a world: the last of its fixed obstacles.
scene::Aabb stileBoxOf(const game::MazeWorld& world) {
    REQUIRE(world.stileWall.has_value());
    REQUIRE(world.colliders.size() == world.walls.size() + world.pillars.size() + 1U);
    return world.colliders.back();
}

// A heightmap that is not flat: 8 by 8 values from 0 to 1 made by a formula.
game::Heightmap roughHeightmap() {
    constexpr int SIZE = 8;
    constexpr int LEVELS = 7;
    game::Heightmap heightmap;
    heightmap.width = SIZE;
    heightmap.height = SIZE;
    heightmap.values.clear();
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            const int level = (column * 5 + row * 3) % LEVELS;
            heightmap.values.push_back(static_cast<float>(level) / static_cast<float>(LEVELS - 1));
        }
    }
    return heightmap;
}

// Reads one model of the game. The assets directory is compiled in by CMake.
assets::ObjModel loadModel(const char* fileName) {
    assets::ObjModel model;
    std::string error;
    const std::filesystem::path file =
        std::filesystem::path{NIGHT_MAZE_ASSETS_DIR} / "models" / fileName;
    REQUIRE_MESSAGE(assets::loadObj(file, model, error), error);
    return model;
}

} // namespace

TEST_CASE("the stile is in the north border wall of the start cell of every maze") {
    forEveryWorld([](const game::MazeWorld& world) {
        REQUIRE(world.stileWall.has_value());
        const std::size_t index = world.stileWall.value_or(0);
        REQUIRE(index < world.walls.size());
        const game::WallSegment& wall = world.walls[index];

        CHECK(game::carriesStile(wall, game::START_CELL));
        CHECK(wall.axis == game::WallAxis::AlongX);
        CHECK(wall.position.x == game::CELL_SIZE / 2.0F);
        CHECK(wall.position.z == 0.0F);
        CHECK(world.maze.hasWall(game::START_CELL.x, game::START_CELL.z, game::STILE_SIDE));

        // It is the only wall that carries it.
        std::size_t carrying = 0;
        for (const game::WallSegment& other : world.walls) {
            carrying += game::carriesStile(other, game::START_CELL) ? 1U : 0U;
        }
        CHECK(carrying == 1U);
    });
}

TEST_CASE(
    "the wall of the stile keeps the plain stone, no lever opens it and nothing hangs on it") {
    forEveryWorld([](const game::MazeWorld& world) {
        const std::size_t index = world.stileWall.value_or(0);
        REQUIRE(index < world.wallVariants.size());
        CHECK(world.wallVariants[index] == game::WallVariant::Plain);

        const game::WallRef stile{.cell = game::START_CELL, .side = game::STILE_SIDE};
        for (const game::Lever& lever : world.interactables.levers) {
            CHECK_FALSE(game::sameWall(lever.mount, stile));
            CHECK_FALSE(game::sameWall(lever.opens, stile));
            CHECK(lever.mount.cell != game::START_CELL);
        }
        for (const game::Note& note : world.interactables.notes) {
            CHECK_FALSE(game::sameWall(note.mount, stile));
            CHECK(note.mount.cell != game::START_CELL);
        }
    });
}

TEST_CASE("the box of the stile lies along the steps, in front of the box of its wall") {
    const game::MazeWorld world = game::buildMazeWorld(10, 10, 1U);
    const scene::Aabb box = stileBoxOf(world);

    // The wall stands at (1, 0, 0). The box reaches from 0.19 to 1.85 m along it, from
    // the face of the wall box (0.15 m) to 0.46 m into the cell, and 1.92 m up.
    CHECK(box.min.x == doctest::Approx(0.19F));
    CHECK(box.max.x == doctest::Approx(1.85F));
    CHECK(box.min.y == doctest::Approx(0.0F));
    CHECK(box.max.y == doctest::Approx(1.92F));
    CHECK(box.min.z == doctest::Approx(0.15F));
    CHECK(box.max.z == doctest::Approx(0.46F));

    // Higher than the body of the player: nobody gets over it.
    CHECK(game::STILE_BOX_HEIGHT > game::Player::BODY_HEIGHT);
}

TEST_CASE("the box of the stile shares no room with a wall, a pillar, the gate or the exit") {
    forEveryWorld([](const game::MazeWorld& world) {
        const scene::Aabb box = inside(stileBoxOf(world));
        for (std::size_t i = 0; i + 1 < world.colliders.size(); ++i) {
            CHECK_FALSE(scene::overlaps(box, world.colliders[i]));
        }
        if (world.hasGate) {
            CHECK_FALSE(scene::overlaps(box, world.gateBox));
            CHECK_FALSE(scene::overlaps(box, world.exitZone));
        }
    });
}

TEST_CASE("nothing of a maze is placed where the stile stands") {
    forEveryWorld([](const game::MazeWorld& world) {
        // Crystals, flasks and puddles never get the start cell at all.
        for (const game::CrystalSpawn& crystal : world.crystals) {
            CHECK(crystal.cell != game::START_CELL);
        }
        for (const game::MazeCell cell :
             game::flaskDeadEnds(world.maze, world.seed, game::START_CELL, world.exitCell)) {
            CHECK(cell != game::START_CELL);
        }
        for (const game::PuddleSpawn& puddle : game::placePuddles(
                 world.maze, world.seed, game::START_CELL, world.exitCell, world.crystals, 1.0F)) {
            CHECK(puddle.cell != game::START_CELL);
        }

        // The milestone stands in the cell in front of the gate, which in a maze of two
        // cells is the start cell: then there is none.
        const game::GateScenery scenery = game::gateScenery(world);
        if (scenery.hasMilestone) {
            const glm::vec3 stone{scenery.milestone[3]};
            CHECK(game::cellAt(stone) != game::START_CELL);
        }
    });
}

TEST_CASE("the player starts clear of the stile, and so does whoever the shade carries back") {
    // Both are the same point: the shade sets the player down at MazeWorld::startPosition.
    forEveryWorld([](const game::MazeWorld& world) {
        constexpr float MIN_GAP = 0.2F;
        const scene::Aabb box = stileBoxOf(world);
        game::Player player;
        player.position = world.startPosition;
        const scene::Aabb body = player.box();

        CHECK_FALSE(scene::overlaps(body, box));
        // The stile is north of the player, and there is room between the two.
        CHECK(body.min.z - box.max.z >= MIN_GAP);
    });
}

TEST_CASE("the stile never blocks a way out of the start cell") {
    forEveryWorld([](const game::MazeWorld& world) {
        game::Player player;
        player.position = world.startPosition;
        const scene::Aabb body = player.box();

        int openSides = 0;
        for (const game::Direction side : game::ALL_DIRECTIONS) {
            if (world.maze.hasWall(game::START_CELL.x, game::START_CELL.z, side)) {
                continue;
            }
            ++openSides;
            // North and west of the start cell is the border.
            REQUIRE((side == game::Direction::East || side == game::Direction::South));
            // Straight from the middle of the start cell to the middle of the next cell:
            // the whole step is allowed, with every fixed obstacle of the world in play.
            const glm::vec3 step = side == game::Direction::East
                                       ? glm::vec3{game::CELL_SIZE, 0.0F, 0.0F}
                                       : glm::vec3{0.0F, 0.0F, game::CELL_SIZE};
            const glm::vec3 allowed = scene::moveAndSlide(body, step, world.colliders);
            CHECK(allowed.x == doctest::Approx(step.x));
            CHECK(allowed.z == doctest::Approx(step.z));
        }
        // Only the maze of one cell has no way out.
        CHECK((openSides > 0) == (world.maze.width() * world.maze.height() > 1));

        // The box ends where the east side of the cell begins: it does not reach into an
        // opening there, and it leaves more than twice the body between itself and the
        // south side.
        const scene::Aabb box = stileBoxOf(world);
        CHECK(box.max.x <= game::CELL_SIZE - game::WALL_COLLISION_THICKNESS / 2.0F + SLACK);
        CHECK(game::CELL_SIZE - game::WALL_COLLISION_THICKNESS / 2.0F - box.max.z >
              2.0F * game::Player::BODY_WIDTH);
    });
}

TEST_CASE("the box of the stile follows its wall onto the terrain") {
    game::MazeWorld world = game::buildMazeWorld(10, 10, 3U, roughHeightmap(), 1.0F);
    REQUIRE(world.stileWall.has_value());
    const std::size_t index = world.stileWall.value_or(0);
    CHECK(stileBoxOf(world).min.y == world.walls[index].position.y);
    CHECK(stileBoxOf(world).max.y ==
          doctest::Approx(world.walls[index].position.y + game::STILE_BOX_HEIGHT));

    // Another height scale: the list is built again, with one box of the stile.
    game::placeOnTerrain(world, roughHeightmap(), 3.0F);
    CHECK(stileBoxOf(world).min.y == world.walls[index].position.y);

    // A world without a stile has no box for it.
    world.stileWall.reset();
    game::placeOnTerrain(world, roughHeightmap(), 3.0F);
    CHECK(world.colliders.size() == world.walls.size() + world.pillars.size());
}

TEST_CASE("the ground in front of the steps is worn, the ground around it is not") {
    const game::WallSegment wall =
        game::wallSegmentOn(game::START_CELL.x, game::START_CELL.z, game::STILE_SIDE);

    // Under the steps and where one stands before climbing them.
    CHECK(game::stileWornGround(wall, 1.0F, 0.2F));
    CHECK(game::stileWornGround(wall, 0.2F, 0.3F));
    CHECK(game::stileWornGround(wall, 1.8F, 0.3F));
    CHECK(game::stileWornGround(wall, 0.95F, 1.45F));
    // Past the round end of the oval, beside it, and behind the wall.
    CHECK_FALSE(game::stileWornGround(wall, 0.95F, 1.55F));
    CHECK_FALSE(game::stileWornGround(wall, 0.2F, 1.4F));
    CHECK_FALSE(game::stileWornGround(wall, 1.9F, 0.3F));
    CHECK_FALSE(game::stileWornGround(wall, 1.0F, -0.3F));
    // The whole footprint of the box is on worn ground.
    const scene::Aabb box = game::stileBox(wall);
    CHECK(game::stileWornGround(wall, box.min.x + SLACK, box.max.z));
    CHECK(game::stileWornGround(wall, box.max.x - SLACK, box.max.z));
}

TEST_CASE("no grass grows in front of the stile, and the grass behind its wall stays") {
    for (const game::Difficulty difficulty :
         {game::Difficulty::Easy, game::Difficulty::Normal, game::Difficulty::Hard}) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        for (std::uint32_t seed = 1; seed <= 5; ++seed) {
            CAPTURE(level.name);
            CAPTURE(seed);
            game::MazeWorld world = game::buildMazeWorld(level.mazeWidth, level.mazeHeight, seed,
                                                         roughHeightmap(), 1.0F);
            REQUIRE(world.stileWall.has_value());
            const game::WallSegment& wall = world.walls[world.stileWall.value_or(0)];
            const std::vector<game::GrassTuft> tufts =
                game::placeGrass(world, game::MAX_GRASS_DENSITY);

            std::size_t worn = 0;
            std::size_t behind = 0;
            for (const game::GrassTuft& tuft : tufts) {
                worn += game::stileWornGround(wall, tuft.position.x, tuft.position.z) ? 1U : 0U;
                behind += game::carriesStile(wall, game::START_CELL) && tuft.position.z < 0.0F &&
                                  tuft.position.z > -1.0F && tuft.position.x > 0.0F &&
                                  tuft.position.x < game::CELL_SIZE
                              ? 1U
                              : 0U;
            }
            CHECK(worn == 0U);
            CHECK(behind > 0U);

            // Without the stile the same world has tufts there: the rule took them away.
            world.stileWall.reset();
            std::size_t before = 0;
            for (const game::GrassTuft& tuft : game::placeGrass(world, game::MAX_GRASS_DENSITY)) {
                before += game::stileWornGround(wall, tuft.position.x, tuft.position.z) ? 1U : 0U;
            }
            CHECK(before > 0U);
        }
    }
}

TEST_CASE("the two models of the stile fit the wall and the box they are placed by") {
    // The models are measured from the middle of the base of their wall: x along it and
    // z out of it, into the start cell. The box of that wall, at the origin.
    const scene::Aabb box = game::stileBox({});
    constexpr float WALL_FACE = game::WALL_COLLISION_THICKNESS / 2.0F;

    const assets::ObjModel stile = loadModel("stile.obj");
    REQUIRE_FALSE(stile.vertices.empty());
    CHECK(stile.parts.size() == 1);
    CHECK(stile.mirroredTriangleCount == 0);
    REQUIRE(stile.materials.size() == 1);
    CHECK(stile.materials.front().diffuseTexture.filename() == "wall_stone.png");
    CHECK(stile.materials.front().normalTexture.filename() == "wall_stone_normal.png");
    // It is drawn three times per frame, like every wall.
    CHECK(stile.indices.size() / 3 <= 150);

    float lowestTop = game::WALL_HEIGHT;
    for (const gfx::Vertex& vertex : stile.vertices) {
        const glm::vec3& point = vertex.position;
        // One wall segment: as long and as high as the plain wall.
        CHECK(std::abs(point.x) <= game::WALL_LENGTH / 2.0F + SLACK);
        CHECK(point.y >= -SLACK);
        CHECK(point.y <= game::WALL_HEIGHT + SLACK);
        // What stands in front of the box of the wall is a step: inside the box of the stile.
        if (point.z > WALL_FACE) {
            CHECK(point.x >= box.min.x);
            CHECK(point.x <= box.max.x);
            CHECK(point.z <= box.max.z);
            CHECK(point.y <= box.max.y);
        }
        // The top of the wall in the notch: the lowest face that looks up, well above
        // the top step.
        constexpr float ABOVE_THE_STEPS = 2.1F;
        if (vertex.normal.y > 0.99F && point.y > ABOVE_THE_STEPS) {
            lowestTop = std::min(lowestTop, point.y);
        }
    }
    // The wall drops to 2.4 m over the notch (its second stone lies 1 cm lower).
    CHECK(lowestTop == doctest::Approx(2.39F));

    const assets::ObjModel post = loadModel("stile_post.obj");
    REQUIRE_FALSE(post.vertices.empty());
    CHECK(post.parts.size() == 1);
    CHECK(post.mirroredTriangleCount == 0);
    REQUIRE(post.materials.size() == 1);
    CHECK(post.materials.front().diffuseTexture.filename() == "gate_wood.png");
    CHECK(post.materials.front().normalTexture.filename() == "gate_wood_normal.png");
    CHECK(post.indices.size() / 3 <= 100);

    // The post and its hook stand in front of the wall, clear of a wall across the end of
    // the segment (its plinth begins 0.86 m from the middle), and as far as the player
    // reaches up they are inside the box. The post goes into the ground and ends above
    // the pillars.
    constexpr float CROSS_WALL_FACE = 0.86F;
    constexpr float CAP_OVERHANG = 0.02F;
    float lowest = 0.0F;
    float highest = 0.0F;
    for (const gfx::Vertex& vertex : post.vertices) {
        const glm::vec3& point = vertex.position;
        lowest = std::min(lowest, point.y);
        highest = std::max(highest, point.y);
        CHECK(point.z > WALL_FACE);
        CHECK(point.z < box.max.z);
        CHECK(point.x > box.min.x);
        if (point.y < game::Player::BODY_HEIGHT) {
            CHECK(point.x <= box.max.x + SLACK);
            CHECK(point.x < CROSS_WALL_FACE);
        }
        CHECK(point.x <= box.max.x + CAP_OVERHANG + SLACK);
    }
    CHECK(lowest < -0.1F);
    CHECK(highest > game::PILLAR_HEIGHT);
}
