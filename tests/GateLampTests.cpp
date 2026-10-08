// Tests of game/GateLamp.hpp: how brightly the lamp of the gate burns and in which
// colour, its point light, and where the gatehouse, its lanterns and the milestone stand
// in every maze the game can build. The last tests read the three model files.
#include "game/GateLamp.hpp"

#include "assets/ImageLoader.hpp"
#include "assets/ObjLoader.hpp"
#include "game/Campaign.hpp"
#include "game/Difficulty.hpp"
#include "game/Interactables.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "game/Terrain.hpp"
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

// The fixed step of the game: core::Time::FIXED_DT as a float.
constexpr float STEP = 1.0F / 120.0F;

// The lantern model: 0.2 m wide and 0.36 m tall (tools/blender/build_gate_lantern.py).
constexpr float LANTERN_HALF_WIDTH = 0.1F;
constexpr float LANTERN_HEIGHT = 0.36F;
// The widest part of the lantern is its cap, which is a little wider than its base.
constexpr float LANTERN_CAP_HALF_WIDTH = 0.12F;

// The milestone model: 0.25 m wide above its foot (tools/blender/build_milestone.py).
constexpr float MILESTONE_HALF_WIDTH = 0.125F;

// The piers of the gatehouse model (tools/blender/build_gate_arch.py): they start this
// far below the base of the gate, and their caps end this far above it.
constexpr float PIER_DEPTH = 0.4F;
constexpr float PIER_CAP_TOP = 3.65F;

// Absolute path of the assets directory, compiled in by CMake.
std::filesystem::path assetsDirectory() {
    return {NIGHT_MAZE_ASSETS_DIR};
}

// The heightmap of the game, so the sweep below stands on the real hills.
game::Heightmap realHeightmap() {
    assets::Image image;
    std::string error;
    REQUIRE(assets::loadImage(assetsDirectory() / "textures" / "heightmap.png", image, error,
                              assets::RowOrder::TopFirst));
    return game::heightmapFromImage(image);
}

// Where a model matrix puts the origin of its model: the last column.
glm::vec3 placeOf(const glm::mat4& matrix) {
    return glm::vec3{matrix[3]};
}

// True when the two boxes overlap seen from above (their footprints).
bool footprintsOverlap(const scene::Aabb& a, const scene::Aabb& b) {
    return a.min.x < b.max.x && a.max.x > b.min.x && a.min.z < b.max.z && a.max.z > b.min.z;
}

// A box of the given half width around a place on the ground, for a footprint test.
scene::Aabb footprintAround(const glm::vec3& place, float halfWidth) {
    return scene::Aabb::fromCenter(place, {halfWidth, 1.0F, halfWidth});
}

// One size of maze the game builds, with the numbers of its levers and notes.
struct MazeKind {
    int width;
    int height;
    int crystals;
    game::InteractableSettings interactables;
};

// Every kind of maze the game builds: the three levels of free play and the five nights
// of the campaign.
std::vector<MazeKind> mazeKinds() {
    std::vector<MazeKind> kinds;
    for (const game::Difficulty difficulty : game::ALL_DIFFICULTIES) {
        const game::DifficultyLevel& level = game::difficultyLevel(difficulty);
        kinds.push_back({.width = level.mazeWidth,
                         .height = level.mazeHeight,
                         .crystals = level.crystalCount,
                         .interactables = {}});
    }
    for (int night = 1; night <= game::CAMPAIGN_NIGHT_COUNT; ++night) {
        const game::CampaignNight& numbers = game::campaignNight(night);
        kinds.push_back({.width = numbers.mazeWidth,
                         .height = numbers.mazeHeight,
                         .crystals = numbers.crystalCount,
                         .interactables = game::campaignInteractables(night, {})});
    }
    return kinds;
}

// Reads one model of the game.
assets::ObjModel loadModel(const char* fileName) {
    assets::ObjModel model;
    std::string error;
    REQUIRE_MESSAGE(assets::loadObj(assetsDirectory() / "models" / fileName, model, error), error);
    return model;
}

} // namespace

TEST_CASE("the lamp of the gate starts with the agreed numbers") {
    const game::GateSettings settings;
    CHECK(settings.glowStrength == game::GATE_LAMP_GLOW_STRENGTH);
    CHECK(settings.emberMin == game::GATE_LAMP_EMBER_MIN);
    CHECK(settings.emberMax == game::GATE_LAMP_EMBER_MAX);
    CHECK(settings.lightRadius == game::GATE_LAMP_RADIUS);
    CHECK(settings.bellSeconds == game::GATE_BELL_SECONDS);

    // The ember is never off, grows, and stays far below the open lamp.
    CHECK(game::GATE_LAMP_EMBER_MIN > 0.0F);
    CHECK(game::GATE_LAMP_EMBER_MAX > game::GATE_LAMP_EMBER_MIN);
    CHECK(game::GATE_LAMP_EMBER_MAX <= 0.5F);
    // The settings of a round carry them.
    CHECK(game::GameplaySettings{}.gate.glowStrength == game::GATE_LAMP_GLOW_STRENGTH);
}

TEST_CASE("the ember of a closed gate grows with every crystal and never falls") {
    for (int needed = 1; needed <= 40; ++needed) {
        float before = game::gateLampStrength(0, needed, 0.0F);
        CHECK(before == doctest::Approx(game::GATE_LAMP_EMBER_MIN));
        // A few more than needed too: a count past the end changes nothing.
        for (int collected = 1; collected <= needed + 3; ++collected) {
            const float now = game::gateLampStrength(collected, needed, 0.0F);
            CHECK(now >= before);
            if (collected <= needed) {
                CHECK(now > before);
            }
            // Closed, the lamp never burns brighter than the largest ember.
            CHECK(now <= game::GATE_LAMP_EMBER_MAX + 0.0001F);
            before = now;
        }
        CHECK(game::gateLampStrength(needed, needed, 0.0F) ==
              doctest::Approx(game::GATE_LAMP_EMBER_MAX));
    }
}

TEST_CASE("a fully open gate burns at exactly 1, whatever was collected") {
    CHECK(game::gateLampStrength(0, 10, 1.0F) == 1.0F);
    CHECK(game::gateLampStrength(7, 10, 1.0F) == 1.0F);
    CHECK(game::gateLampStrength(10, 10, 1.0F) == 1.0F);
    CHECK(game::gateLampStrength(13, 10, 1.0F) == 1.0F);
    CHECK(game::gateLampStrength(0, 0, 1.0F) == 1.0F);
    // Progress past the end is the end.
    CHECK(game::gateLampStrength(10, 10, 1.7F) == 1.0F);
}

TEST_CASE("nothing jumps at the moment the gate opens") {
    constexpr int NEEDED = 10;
    // The step before the last crystal, the step of the last crystal (the gate opens,
    // its progress is still 0) and the first step of the sinking.
    const float lastClosed = game::gateLampStrength(NEEDED - 1, NEEDED, 0.0F);
    const float opened = game::gateLampStrength(NEEDED, NEEDED, 0.0F);
    const float firstStep = game::gateLampStrength(NEEDED, NEEDED, STEP / game::GATE_OPEN_SECONDS);

    // The last crystal is one more step of the ember, like every crystal before it.
    const float crystalStep =
        (game::GATE_LAMP_EMBER_MAX - game::GATE_LAMP_EMBER_MIN) / static_cast<float>(NEEDED);
    CHECK(opened - lastClosed == doctest::Approx(crystalStep));
    CHECK(opened == doctest::Approx(game::GATE_LAMP_EMBER_MAX));
    // And the sinking starts from exactly there, a little at a time.
    CHECK(firstStep > opened);
    CHECK(firstStep - opened < 0.01F);

    // While the gate sinks the lamp only gets brighter, up to 1.
    float before = opened;
    for (int step = 1; step <= 180; ++step) {
        const float progress = static_cast<float>(step) / 180.0F;
        const float now = game::gateLampStrength(NEEDED, NEEDED, progress);
        CHECK(now >= before);
        CHECK(now - before < 0.01F);
        before = now;
    }
    CHECK(before == 1.0F);
}

TEST_CASE("the lamp of a real round follows its crystals and its gate") {
    // The golden maze: 4 x 4 cells from seed 1, with a gate and two crystals.
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1U);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    REQUIRE(round.requiredCount > 0);
    bool flashlightOn = true;

    const auto strength = [&round, &settings]() {
        return game::gateLampStrength(round.collectedCount, round.requiredCount, round.gateProgress,
                                      settings.gate);
    };
    CHECK(strength() == doctest::Approx(game::GATE_LAMP_EMBER_MIN));

    // The player walks into every crystal. The lamp never gets dimmer on the way.
    float before = strength();
    for (const game::RoundCrystal& crystal : std::vector<game::RoundCrystal>(round.crystals)) {
        game::updateRound(round, world, settings, crystal.restPosition, flashlightOn, STEP);
        CHECK(strength() >= before);
        before = strength();
    }
    REQUIRE(round.gateOpen);
    // The gate sinks for GATE_OPEN_SECONDS: after that the lamp is fully lit.
    const glm::vec3 nowhere{-50.0F, 0.0F, -50.0F};
    for (int step = 0; step < 200; ++step) {
        game::updateRound(round, world, settings, nowhere, flashlightOn, STEP);
        CHECK(strength() >= before);
        before = strength();
    }
    CHECK(strength() == 1.0F);
}

TEST_CASE("a gate that asks for nothing has its ember at the maximum") {
    CHECK(game::gateLampStrength(0, 0, 0.0F) == doctest::Approx(game::GATE_LAMP_EMBER_MAX));
    CHECK(game::gateLampStrength(0, 0, 0.5F) ==
          doctest::Approx((game::GATE_LAMP_EMBER_MAX + 1.0F) / 2.0F));
}

TEST_CASE("the ember follows its settings, also when they are typed in the wrong order") {
    game::GateSettings settings;
    settings.emberMin = 0.1F;
    settings.emberMax = 0.6F;
    CHECK(game::gateLampStrength(0, 4, 0.0F, settings) == doctest::Approx(0.1F));
    CHECK(game::gateLampStrength(2, 4, 0.0F, settings) == doctest::Approx(0.35F));
    CHECK(game::gateLampStrength(4, 4, 0.0F, settings) == doctest::Approx(0.6F));

    // Swapped: the smaller number is still the ember without crystals.
    settings.emberMin = 0.6F;
    settings.emberMax = 0.1F;
    CHECK(game::gateLampStrength(0, 4, 0.0F, settings) == doctest::Approx(0.1F));
    CHECK(game::gateLampStrength(4, 4, 0.0F, settings) == doctest::Approx(0.6F));

    // Out of range: a strength stays between 0 and 1.
    settings.emberMin = -2.0F;
    settings.emberMax = 5.0F;
    CHECK(game::gateLampStrength(0, 4, 0.0F, settings) == 0.0F);
    CHECK(game::gateLampStrength(4, 4, 0.0F, settings) == 1.0F);
}

TEST_CASE("the lamp is the colour of the moon while closed and amber when open") {
    CHECK(game::gateLampColor(0.0F) == game::GATE_LAMP_COLD_COLOR);
    CHECK(game::gateLampColor(1.0F) == game::GATE_LAMP_WARM_COLOR);
    CHECK(game::gateLampColor(-1.0F) == game::GATE_LAMP_COLD_COLOR);
    CHECK(game::gateLampColor(2.0F) == game::GATE_LAMP_WARM_COLOR);
    // The cold colour is the one of the moon light, and it is cold: more blue than red.
    CHECK(game::GATE_LAMP_COLD_COLOR == game::LightingSettings{}.moonColor);
    CHECK(game::GATE_LAMP_COLD_COLOR.b > game::GATE_LAMP_COLD_COLOR.r);
    CHECK(game::GATE_LAMP_WARM_COLOR.r > game::GATE_LAMP_WARM_COLOR.b);

    // Half way it is half way, and red only rises while blue only falls.
    const glm::vec3 half = game::gateLampColor(0.5F);
    CHECK(half.r ==
          doctest::Approx((game::GATE_LAMP_COLD_COLOR.r + game::GATE_LAMP_WARM_COLOR.r) / 2.0F));
    CHECK(half.b ==
          doctest::Approx((game::GATE_LAMP_COLD_COLOR.b + game::GATE_LAMP_WARM_COLOR.b) / 2.0F));
}

TEST_CASE("the light of the lamp has its own colour, radius and intensity") {
    game::GateSettings settings;
    settings.lightRadius = 4.5F;
    const glm::vec3 position{3.0F, 2.5F, 7.0F};

    const game::PointLightSpot closed = game::gateLampLight(position, 0.2F, 0.0F, settings);
    CHECK(closed.ownLook);
    CHECK(closed.position == position);
    CHECK(closed.strength == 0.2F);
    CHECK(closed.color == game::GATE_LAMP_COLD_COLOR);
    CHECK(closed.radius == 4.5F);
    CHECK(closed.intensity == game::GATE_LAMP_LIGHT_INTENSITY);

    const game::PointLightSpot open = game::gateLampLight(position, 1.0F, 1.0F, settings);
    CHECK(open.color == game::GATE_LAMP_WARM_COLOR);
    CHECK(open.strength == 1.0F);
}

TEST_CASE("the gate side is the open side of the exit cell, and the approach lies behind it") {
    const game::MazeWorld world = game::buildMazeWorld(4, 4, 1U);
    REQUIRE(world.hasGate);

    const game::Direction side = game::gateSide(world);
    CHECK_FALSE(world.maze.hasWall(world.exitCell.x, world.exitCell.z, side));
    const game::MazeCell approach = game::approachCell(world);
    CHECK(approach.x == world.exitCell.x + game::columnStep(side));
    CHECK(approach.z == world.exitCell.z + game::rowStep(side));
    CHECK(world.maze.contains(approach.x, approach.z));

    // The gate stands exactly between the two cells.
    const glm::vec3 between = (game::cellCenter(world.exitCell.x, world.exitCell.z) +
                               game::cellCenter(approach.x, approach.z)) /
                              2.0F;
    CHECK(world.gate.position.x == doctest::Approx(between.x));
    CHECK(world.gate.position.z == doctest::Approx(between.z));
}

TEST_CASE("the gatehouse, the lanterns and the milestone fit every maze the game builds") {
    const game::Heightmap heightmap = realHeightmap();
    // How often the gate stood on each side, and how many mazes got a milestone.
    std::array<int, game::DIRECTION_COUNT> sideCount{};
    int worlds = 0;
    int milestones = 0;

    for (const MazeKind& kind : mazeKinds()) {
        for (std::uint32_t seed = 1; seed <= 40; ++seed) {
            const game::MazeWorld world =
                game::buildMazeWorld(kind.width, kind.height, seed, heightmap,
                                     game::DEFAULT_HEIGHT_SCALE, kind.interactables, kind.crystals);
            REQUIRE(world.hasGate);
            ++worlds;
            CAPTURE(kind.width);
            CAPTURE(seed);

            // The side and the approach cell. The exit is a dead end: one open side.
            const game::Direction side = game::gateSide(world);
            ++sideCount.at(static_cast<std::size_t>(side));
            for (const game::Direction other : game::ALL_DIRECTIONS) {
                CHECK(world.maze.hasWall(world.exitCell.x, world.exitCell.z, other) ==
                      (other != side));
            }
            const game::MazeCell approach = game::approachCell(world);
            REQUIRE(world.maze.contains(approach.x, approach.z));

            const game::GateScenery scenery = game::gateScenery(world);
            const glm::vec3 base = world.gate.position;
            const scene::Aabb approachSquare =
                footprintAround(game::cellCenter(approach.x, approach.z), game::CELL_SIZE / 2.0F);

            // The gatehouse stands exactly where the gate does.
            CHECK(scenery.arch == game::wallModelMatrix(world.gate));

            // The pillars at the two ends of the gate are inside the piers: not lower
            // than a pier reaches into the ground, and not higher than its cap.
            int pillarsAtGate = 0;
            for (const glm::vec3& pillar : world.pillars) {
                const float alongGate = std::abs(pillar.x - base.x) + std::abs(pillar.z - base.z);
                if (std::abs(alongGate - game::CELL_SIZE / 2.0F) > 0.01F ||
                    (world.gate.axis == game::WallAxis::AlongX
                         ? std::abs(pillar.z - base.z)
                         : std::abs(pillar.x - base.x)) > 0.01F) {
                    continue;
                }
                ++pillarsAtGate;
                CHECK(pillar.y >= base.y - PIER_DEPTH);
                CHECK(pillar.y + game::PILLAR_HEIGHT <= base.y + PIER_CAP_TOP);
            }
            CHECK(pillarsAtGate == 2);

            // The big lantern: over the middle of the gate, its glass above every wall
            // and every pillar of the maze, and under the roof of the gatehouse.
            const glm::vec3 big = placeOf(scenery.lanterns[0]);
            CHECK(big.x == doctest::Approx(base.x));
            CHECK(big.z == doctest::Approx(base.z));
            CHECK(big.y == doctest::Approx(base.y + game::GATE_COTE_LANTERN_HEIGHT));
            CHECK(big.y + LANTERN_HEIGHT * game::GATE_COTE_LANTERN_SCALE <
                  base.y + game::GATE_HOUSE_HEIGHT);
            for (const scene::Aabb& box : world.colliders) {
                // Only what stands near enough to hide it from somebody in a corridor.
                if (std::abs(box.min.x - base.x) < 12.0F && std::abs(box.min.z - base.z) < 12.0F) {
                    CHECK(big.y > box.max.y + 1.0F);
                }
            }

            // The two bracket lanterns: in the approach cell, at the same height, the
            // same distance to both sides of the middle, clear of every wall and
            // pillar, and above the head of a player who stands under them.
            const glm::vec3 left = placeOf(scenery.lanterns[1]);
            const glm::vec3 right = placeOf(scenery.lanterns[2]);
            CHECK(left.y == doctest::Approx(right.y));
            CHECK(glm::distance(left, right) ==
                  doctest::Approx(2.0F * game::GATE_BRACKET_LANTERN_ALONG));
            const glm::vec3 middle = (left + right) / 2.0F;
            CHECK(glm::distance(glm::vec2{middle.x, middle.z}, glm::vec2{base.x, base.z}) ==
                  doctest::Approx(game::GATE_BRACKET_LANTERN_OUT));
            for (const glm::vec3& lantern : {left, right}) {
                CHECK(game::cellAt(lantern) == approach);
                const scene::Aabb footprint = footprintAround(lantern, LANTERN_CAP_HALF_WIDTH);
                for (const scene::Aabb& box : world.colliders) {
                    CHECK_FALSE(footprintsOverlap(footprint, box));
                }
                const float ground = world.terrain.heightAt(lantern.x, lantern.z);
                CHECK(lantern.y > ground + game::Player::BODY_HEIGHT + 0.1F);
                // And they hang under the stone head of the doorway.
                CHECK(lantern.y + LANTERN_HEIGHT < base.y + game::WALL_HEIGHT);
            }

            // The light: in the approach cell, in the air above the door.
            CHECK(game::cellAt(scenery.lightPosition) == approach);
            CHECK(scenery.lightPosition.y == doctest::Approx(base.y + game::GATE_LIGHT_HEIGHT));

            // The milestone: in the approach cell, on the ground, against a wall that
            // is there, stays there and carries nothing, and inside no box.
            if (!scenery.hasMilestone) {
                continue;
            }
            ++milestones;
            const glm::vec3 stone = placeOf(scenery.milestone);
            const game::WallRef wall{.cell = approach, .side = scenery.milestoneSide};
            CHECK(world.maze.hasWall(approach.x, approach.z, wall.side));
            for (const game::Lever& lever : world.interactables.levers) {
                CHECK_FALSE(game::sameWall(lever.opens, wall));
                CHECK_FALSE(game::sameWall(lever.mount, wall));
            }
            for (const game::Note& note : world.interactables.notes) {
                CHECK_FALSE(game::sameWall(note.mount, wall));
            }
            CHECK(stone.y == doctest::Approx(world.terrain.heightAt(stone.x, stone.z)));
            const scene::Aabb stoneFootprint = footprintAround(stone, MILESTONE_HALF_WIDTH);
            CHECK(stoneFootprint.min.x >= approachSquare.min.x);
            CHECK(stoneFootprint.max.x <= approachSquare.max.x);
            CHECK(stoneFootprint.min.z >= approachSquare.min.z);
            CHECK(stoneFootprint.max.z <= approachSquare.max.z);
            for (const scene::Aabb& box : world.colliders) {
                CHECK_FALSE(footprintsOverlap(stoneFootprint, box));
            }
            // It leans on its wall: nearer to it than the body of the player can get,
            // so the middle of the player never enters the stone.
            const glm::vec3 wallMiddle =
                game::wallSegmentOn(approach.x, approach.z, wall.side).position;
            const float toWall =
                wall.side == game::Direction::North || wall.side == game::Direction::South
                    ? std::abs(stone.z - wallMiddle.z)
                    : std::abs(stone.x - wallMiddle.x);
            CHECK(toWall + MILESTONE_HALF_WIDTH <
                  game::WALL_COLLISION_THICKNESS / 2.0F + game::Player::BODY_WIDTH / 2.0F);
        }
    }

    // The gate stood on every one of the four sides, so all four turns were checked.
    for (const int count : sideCount) {
        CHECK(count > 0);
    }
    MESSAGE("gate sides north/east/south/west: "
            << sideCount[0] << "/" << sideCount[1] << "/" << sideCount[2] << "/" << sideCount[3]
            << ", milestones in " << milestones << " of " << worlds << " mazes");
    // Nearly every maze has a wall for the stone.
    CHECK(milestones * 10 >= worlds * 9);
}

TEST_CASE("the scenery follows the ground when the terrain is rebuilt") {
    const game::Heightmap heightmap = realHeightmap();
    game::MazeWorld world = game::buildMazeWorld(10, 10, 1U, heightmap, 1.0F);
    const game::GateScenery low = game::gateScenery(world);
    game::placeOnTerrain(world, heightmap, game::MAX_HEIGHT_SCALE);
    const game::GateScenery high = game::gateScenery(world);

    // Nothing moves sideways, and every height follows the gate.
    CHECK(placeOf(low.lanterns[0]).x == placeOf(high.lanterns[0]).x);
    CHECK(placeOf(low.lanterns[0]).z == placeOf(high.lanterns[0]).z);
    CHECK(placeOf(high.lanterns[0]).y ==
          doctest::Approx(world.gate.position.y + game::GATE_COTE_LANTERN_HEIGHT));
    CHECK(high.lightPosition.y == doctest::Approx(world.gate.position.y + game::GATE_LIGHT_HEIGHT));
}

TEST_CASE("the gatehouse model stays inside the pillars where the player walks") {
    const assets::ObjModel arch = loadModel("gate_arch.obj");
    REQUIRE_FALSE(arch.vertices.empty());
    CHECK(arch.parts.size() == 1);
    CHECK(arch.mirroredTriangleCount == 0);
    // A few hundred triangles at most: it is drawn three times per frame.
    CHECK(arch.indices.size() / 3 <= 300);

    // The foot of a pillar is this wide to each side of its middle (its box and the
    // margin of its model): below the head of the player the piers are no wider, so
    // the gatehouse adds no stone the player could walk into. Above the head it may be.
    const float footHalf = game::PILLAR_SIZE / 2.0F + game::FOOTPRINT_MARGIN;
    float highest = 0.0F;
    for (const gfx::Vertex& vertex : arch.vertices) {
        highest = std::max(highest, vertex.position.y);
        if (vertex.position.y > game::Player::BODY_HEIGHT + 0.2F) {
            continue;
        }
        CHECK(std::abs(std::abs(vertex.position.x) - game::WALL_LENGTH / 2.0F) <=
              footHalf + 0.001F);
        CHECK(std::abs(vertex.position.z) <= footHalf + 0.001F);
    }
    // Its ridge is the height the shadow map of the moon is fitted to.
    CHECK(highest <= game::GATE_HOUSE_HEIGHT);
    CHECK(highest > game::GATE_HOUSE_HEIGHT - 0.1F);

    // The same from both sides and from both ends: every corner has a twin mirrored in
    // x and one mirrored in z. So the matrix of the gate needs no turn.
    const auto hasVertexAt = [&arch](const glm::vec3& place) {
        return std::ranges::any_of(arch.vertices, [&place](const gfx::Vertex& vertex) {
            return glm::distance(vertex.position, place) < 0.001F;
        });
    };
    bool mirrored = true;
    for (const gfx::Vertex& vertex : arch.vertices) {
        const glm::vec3& p = vertex.position;
        mirrored = mirrored && hasVertexAt({-p.x, p.y, p.z}) && hasVertexAt({p.x, p.y, -p.z});
    }
    CHECK(mirrored);
}

TEST_CASE("the lantern and the milestone models have the sizes the game places them by") {
    const assets::ObjModel lantern = loadModel("gate_lantern.obj");
    REQUIRE_FALSE(lantern.vertices.empty());
    CHECK(lantern.parts.size() == 1);
    CHECK(lantern.mirroredTriangleCount == 0);
    CHECK(lantern.indices.size() / 3 <= 150);
    float top = 0.0F;
    bool pale = false;
    bool dark = false;
    for (const gfx::Vertex& vertex : lantern.vertices) {
        CHECK(vertex.position.y >= 0.0F);
        CHECK(std::abs(vertex.position.x) <= LANTERN_CAP_HALF_WIDTH + 0.001F);
        CHECK(std::abs(vertex.position.z) <= LANTERN_CAP_HALF_WIDTH + 0.001F);
        top = std::max(top, vertex.position.y);
        // The left half of the picture is the pale glass, the right half the dark iron:
        // every corner lies well inside one of the two, away from the line between them.
        CHECK(std::abs(vertex.uv.x - 0.5F) > 0.05F);
        CHECK(vertex.uv.x > 0.0F);
        CHECK(vertex.uv.x < 1.0F);
        pale = pale || vertex.uv.x < 0.5F;
        dark = dark || vertex.uv.x > 0.5F;
    }
    CHECK(top == doctest::Approx(LANTERN_HEIGHT));
    CHECK(pale);
    CHECK(dark);
    // The base of the lantern model is 0.2 m wide, the number the placement uses.
    CHECK(LANTERN_HALF_WIDTH * 2.0F == doctest::Approx(0.2F));

    const assets::ObjModel milestone = loadModel("milestone.obj");
    REQUIRE_FALSE(milestone.vertices.empty());
    CHECK(milestone.parts.size() == 1);
    CHECK(milestone.mirroredTriangleCount == 0);
    CHECK(milestone.indices.size() / 3 <= 60);
    float stoneTop = 0.0F;
    for (const gfx::Vertex& vertex : milestone.vertices) {
        stoneTop = std::max(stoneTop, vertex.position.y);
    }
    // Knee high: between 0.4 and 0.8 m.
    CHECK(stoneTop > 0.4F);
    CHECK(stoneTop < 0.8F);
}
