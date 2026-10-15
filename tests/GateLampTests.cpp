// Tests of game/GateLamp.hpp: how brightly the lamp of the gate burns and in which
// colour, its point light, how its bell swings, and where the gatehouse, its lanterns,
// its bell and the milestone stand in every maze the game can build. The last tests read
// the four model files.
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
#include "game/SoundCues.hpp"
#include "game/Terrain.hpp"
#include "scene/Camera.hpp"
#include "scene/Collider.hpp"

#include <doctest/doctest.h>
#include <glm/gtc/constants.hpp>

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

// The lantern model: 0.2 m wide at its base and 0.36 m tall to the top of its ring
// (tools/blender/build_gate_lantern.py).
constexpr float LANTERN_HALF_WIDTH = 0.1F;
constexpr float LANTERN_HEIGHT = 0.36F;
// The widest part of the lantern is its roof, which is a little wider than its base. The
// placement keeps this much room around a bracket lantern.
constexpr float LANTERN_CAP_HALF_WIDTH = 0.12F;

// The bell model reaches this far down from its pivot, the top of its rope
// (tools/blender/build_gate_bell.py).
constexpr float BELL_HEIGHT = 0.6F;
// The top of the rope reaches into the stone over the doorway: the corners this close
// under the pivot may be inside it.
constexpr float BELL_ROPE_IN_STONE = 0.03F;

// The models are built from generated sources, so their triangle counts and the
// triangles whose picture is mirrored (charts the generator laid out mirrored, on which
// the normal map shows its relief the wrong way) are whatever the source gave. They are
// pinned here, so a rebuild that changes a model is noticed. The arch has some more:
// the build pushes the stone out around the pillars, which folds a few small faces.
constexpr std::size_t ARCH_TRIANGLES = 28463;
constexpr std::size_t ARCH_MIRRORED = 115;
constexpr std::size_t LANTERN_TRIANGLES = 6167;
constexpr std::size_t LANTERN_MIRRORED = 8;
constexpr std::size_t BELL_TRIANGLES = 2450;
constexpr std::size_t BELL_MIRRORED = 4;
constexpr std::size_t MILESTONE_TRIANGLES = 7143;
constexpr std::size_t MILESTONE_MIRRORED = 102;

// The panes of the lantern are its only faces on the square of pale glass in its picture
// (four panes of two triangles), and everything else in the picture is dark iron, no
// brighter than this in any channel (IRON_MAX in build_gate_lantern.py, as 8 bits).
constexpr std::size_t LANTERN_PANE_TRIANGLES = 8;
constexpr int LANTERN_GLASS_LEAST = 180;
constexpr int LANTERN_IRON_MOST = 36;

// How far the stone of the arch may stand back from a face of a pillar inside it before
// the pillar shows: the generated stone is uneven, and a sliver this thin is not seen.
constexpr float PILLAR_PEEK = 0.02F;

// The milestone model above its foot (tools/blender/build_milestone.py): a slab 0.54 m
// wide along its broad faces and 0.28 m thick, its broad faces turned along its wall.
constexpr float MILESTONE_HALF_WIDTH = 0.275F;
constexpr float MILESTONE_HALF_THICKNESS = 0.145F;

// The piers of the arch model (tools/blender/build_gate_arch.py): they start this far
// below the base of the gate, and up to this height above it they hide a pillar that
// stands inside them (the model test checks both).
constexpr float PIER_DEPTH = 0.4F;
constexpr float PIER_CAP_TOP = 3.4F;

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

// The nearest triangle of a model that a ray meets: how far along the ray (direction has
// length 1) and the direction the triangle looks in. distance is negative when the ray
// meets none (Moller and Trumbore).
struct RayHit {
    float distance = -1.0F;
    glm::vec3 normal{0.0F};
};

RayHit castRay(const assets::ObjModel& model, const glm::vec3& origin, const glm::vec3& direction) {
    RayHit nearest;
    for (std::size_t i = 0; i + 2 < model.indices.size(); i += 3) {
        const glm::vec3& a = model.vertices[model.indices[i]].position;
        const glm::vec3 edge1 = model.vertices[model.indices[i + 1]].position - a;
        const glm::vec3 edge2 = model.vertices[model.indices[i + 2]].position - a;
        const glm::vec3 p = glm::cross(direction, edge2);
        const float determinant = glm::dot(edge1, p);
        if (std::abs(determinant) < 1.0e-12F) {
            continue;
        }
        const glm::vec3 t = origin - a;
        const float u = glm::dot(t, p) / determinant;
        if (u < 0.0F || u > 1.0F) {
            continue;
        }
        const glm::vec3 q = glm::cross(t, edge1);
        const float v = glm::dot(direction, q) / determinant;
        if (v < 0.0F || u + v > 1.0F) {
            continue;
        }
        const float distance = glm::dot(edge2, q) / determinant;
        if (distance > 0.0F && (nearest.distance < 0.0F || distance < nearest.distance)) {
            nearest = {.distance = distance, .normal = glm::cross(edge1, edge2)};
        }
    }
    return nearest;
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
            // than a pier reaches into the ground, and not higher than it hides them.
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

            // The big lantern: on the crown of the arch, where the matrix of the arch puts
            // that point of the model, above every wall and every pillar of the maze, and
            // under the height the shadow map of the moon reaches.
            const glm::vec3 big = placeOf(scenery.lanterns[0]);
            const glm::vec3 inModel{scenery.arch * glm::vec4{game::GATE_COTE_LANTERN_ALONG,
                                                             game::GATE_COTE_LANTERN_HEIGHT, 0.0F,
                                                             1.0F}};
            CHECK(glm::distance(big, inModel) < 0.0001F);
            CHECK(glm::distance(glm::vec2{big.x, big.z}, glm::vec2{base.x, base.z}) ==
                  doctest::Approx(game::GATE_COTE_LANTERN_ALONG));
            CHECK(big.y == doctest::Approx(base.y + game::GATE_COTE_LANTERN_HEIGHT));

            // The bell: its pivot under the crown, in the middle of the gate like the big
            // lantern. Hanging still or swung out, the pivot stays where it is.
            const glm::vec3 pivot = placeOf(game::gateBellMatrix(scenery.arch, 0.0F));
            CHECK(pivot.y == doctest::Approx(base.y + game::GATE_BELL_HEIGHT));
            CHECK(pivot.y < base.y + game::GATE_HOUSE_HEIGHT);
            CHECK((pivot.x + big.x) / 2.0F == doctest::Approx(base.x));
            CHECK((pivot.z + big.z) / 2.0F == doctest::Approx(base.z));
            CHECK(glm::distance(pivot, placeOf(game::gateBellMatrix(
                                           scenery.arch, game::GATE_BELL_SWING_DEGREES))) <
                  0.0001F);
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
            const bool alongX =
                wall.side == game::Direction::North || wall.side == game::Direction::South;
            const scene::Aabb stoneFootprint = scene::Aabb::fromCenter(
                stone, alongX ? glm::vec3{MILESTONE_HALF_WIDTH, 1.0F, MILESTONE_HALF_THICKNESS}
                              : glm::vec3{MILESTONE_HALF_THICKNESS, 1.0F, MILESTONE_HALF_WIDTH});
            // Its broad faces look along the wall: the turned model is as wide along it.
            const glm::vec3 sideways{scenery.milestone * glm::vec4{1.0F, 0.0F, 0.0F, 0.0F}};
            CHECK(std::abs(alongX ? sideways.x : sideways.z) == doctest::Approx(1.0F));
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
                alongX ? std::abs(stone.z - wallMiddle.z) : std::abs(stone.x - wallMiddle.x);
            CHECK(toWall + MILESTONE_HALF_THICKNESS <
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

TEST_CASE("the arch model keeps clear of the player, hides the pillars and carries the lanterns") {
    const assets::ObjModel arch = loadModel("gate_arch.obj");
    REQUIRE_FALSE(arch.vertices.empty());
    CHECK(arch.parts.size() == 1);
    CHECK(arch.indices.size() / 3 == ARCH_TRIANGLES);
    CHECK(arch.mirroredTriangleCount == ARCH_MIRRORED);

    // Below the head of the player the arch has no collision box of its own, so its stone
    // stays where the eye can never come nearer than the near plane of the camera. The
    // eye is stopped half a body from the box of a pillar or wall: so it stays this far
    // from the line of the wall, at most this far from the middle in the doorway, and at
    // least this far from it in an open segment beside the gate.
    const float nearPlane = scene::Camera{}.nearPlane;
    const float eyeFromLine = game::PILLAR_SIZE / 2.0F + game::Player::BODY_WIDTH / 2.0F;
    const float doorwayEye = game::WALL_LENGTH / 2.0F - eyeFromLine;
    const float besideEye = game::WALL_LENGTH / 2.0F + eyeFromLine;
    float highest = 0.0F;
    for (const gfx::Vertex& vertex : arch.vertices) {
        const glm::vec3& p = vertex.position;
        highest = std::max(highest, p.y);
        if (p.y > game::Player::BODY_HEIGHT + 0.2F) {
            continue;
        }
        CHECK(std::abs(p.z) <= eyeFromLine - nearPlane);
        CHECK(std::abs(p.x) >= doorwayEye + nearPlane);
        CHECK(std::abs(p.x) <= besideEye - nearPlane);
    }

    // The pillars at the two ends of the gate stand inside the piers, from the ground up
    // to PIER_CAP_TOP: rays from the doorway, from outside along the wall and from both
    // faces meet stone before the faces of a pillar (PILLAR_PEEK).
    const float pillarHalf = game::PILLAR_SIZE / 2.0F;
    const float pillarAt = game::WALL_LENGTH / 2.0F;
    for (const float end : {-1.0F, 1.0F}) {
        for (int step = 1; static_cast<float>(step) * 0.05F <= PIER_CAP_TOP; ++step) {
            const float y = static_cast<float>(step) * 0.05F;
            CAPTURE(end);
            CAPTURE(y);
            for (const float across : {-0.14F, -0.07F, 0.0F, 0.07F, 0.14F}) {
                const RayHit fromDoorway = castRay(arch, {0.0F, y, across}, {end, 0.0F, 0.0F});
                CHECK(fromDoorway.distance > 0.0F);
                CHECK(fromDoorway.distance < pillarAt - pillarHalf + PILLAR_PEEK);
                const RayHit fromBeside =
                    castRay(arch, {end * (pillarAt + 1.0F), y, across}, {-end, 0.0F, 0.0F});
                CHECK(fromBeside.distance > 0.0F);
                CHECK(fromBeside.distance < 1.0F - pillarHalf + PILLAR_PEEK);
            }
            for (const float face : {-1.0F, 1.0F}) {
                for (const float along : {-0.14F, -0.07F, 0.0F, 0.07F, 0.14F}) {
                    const RayHit fromFace =
                        castRay(arch, {end * pillarAt + along, y, face}, {0.0F, 0.0F, -face});
                    CHECK(fromFace.distance > 0.0F);
                    CHECK(fromFace.distance < 1.0F - pillarHalf + PILLAR_PEEK);
                }
            }
        }
    }

    // The big lantern stands on the highest stone of the crown under its base, sunk into
    // it by at most two centimetres, and its ring is the highest point at the gate: the
    // height the shadow map of the moon reaches.
    const float bigHalf = LANTERN_HALF_WIDTH * game::GATE_COTE_LANTERN_SCALE;
    float crown = 0.0F;
    for (const gfx::Vertex& vertex : arch.vertices) {
        const glm::vec3& p = vertex.position;
        if (std::abs(p.x - game::GATE_COTE_LANTERN_ALONG) <= bigHalf && std::abs(p.z) <= bigHalf) {
            crown = std::max(crown, p.y);
        }
    }
    CHECK(game::GATE_COTE_LANTERN_HEIGHT <= crown);
    CHECK(game::GATE_COTE_LANTERN_HEIGHT > crown - 0.02F);
    const float bigTop =
        game::GATE_COTE_LANTERN_HEIGHT + LANTERN_HEIGHT * game::GATE_COTE_LANTERN_SCALE;
    CHECK(highest < bigTop);
    CHECK(bigTop <= game::GATE_HOUSE_HEIGHT);
    CHECK(bigTop > game::GATE_HOUSE_HEIGHT - 0.05F);

    // A hook on each face of each pier, its end in the ring of a bracket lantern: the
    // game hangs the two lanterns on the side of the approach, which can be either face.
    const float ringTop = game::GATE_BRACKET_LANTERN_HEIGHT + LANTERN_HEIGHT;
    for (const float end : {-1.0F, 1.0F}) {
        for (const float face : {-1.0F, 1.0F}) {
            const glm::vec3 ring{end * game::GATE_BRACKET_LANTERN_ALONG, ringTop,
                                 face * game::GATE_BRACKET_LANTERN_OUT};
            CAPTURE(ring);
            CHECK(std::ranges::any_of(arch.vertices, [&ring](const gfx::Vertex& vertex) {
                return glm::distance(vertex.position, ring) < 0.07F;
            }));
        }
    }
}

TEST_CASE("the lantern and the milestone models have the sizes the game places them by") {
    const assets::ObjModel lantern = loadModel("gate_lantern.obj");
    REQUIRE_FALSE(lantern.vertices.empty());
    CHECK(lantern.parts.size() == 1);
    CHECK(lantern.indices.size() / 3 == LANTERN_TRIANGLES);
    CHECK(lantern.mirroredTriangleCount == LANTERN_MIRRORED);
    float top = 0.0F;
    for (const gfx::Vertex& vertex : lantern.vertices) {
        CHECK(vertex.position.y >= 0.0F);
        CHECK(std::abs(vertex.position.x) <= LANTERN_CAP_HALF_WIDTH + 0.001F);
        CHECK(std::abs(vertex.position.z) <= LANTERN_CAP_HALF_WIDTH + 0.001F);
        top = std::max(top, vertex.position.y);
    }
    CHECK(top == doctest::Approx(LANTERN_HEIGHT));
    // The base of the lantern model is 0.2 m wide, the number the placement uses.
    CHECK(LANTERN_HALF_WIDTH * 2.0F == doctest::Approx(0.2F));

    // The game makes the whole lantern glow by its colour, one value for the model. So
    // the picture has one square of pale glass and is dark iron everywhere else, and the
    // panes are the only faces on that square.
    assets::Image picture;
    std::string error;
    REQUIRE(assets::loadImage(assetsDirectory() / "textures" / "gate_lantern.png", picture, error));
    REQUIRE(picture.channels == 3);
    int glassLeft = picture.width;
    int glassRight = -1;
    int glassBottom = picture.height;
    int glassTop = -1;
    int glassTexels = 0;
    int brightestIron = 0;
    for (int row = 0; row < picture.height; ++row) {
        for (int column = 0; column < picture.width; ++column) {
            const auto texel = static_cast<std::size_t>((row * picture.width + column) * 3);
            const int least = std::min(
                {picture.pixels[texel], picture.pixels[texel + 1], picture.pixels[texel + 2]});
            const int most = std::max(
                {picture.pixels[texel], picture.pixels[texel + 1], picture.pixels[texel + 2]});
            if (least >= LANTERN_GLASS_LEAST) {
                ++glassTexels;
                glassLeft = std::min(glassLeft, column);
                glassRight = std::max(glassRight, column);
                glassBottom = std::min(glassBottom, row);
                glassTop = std::max(glassTop, row);
            } else {
                brightestIron = std::max(brightestIron, most);
            }
        }
    }
    REQUIRE(glassTexels > 0);
    CHECK(glassTexels == (glassRight - glassLeft + 1) * (glassTop - glassBottom + 1));
    CHECK(brightestIron <= LANTERN_IRON_MOST);

    const auto onGlass = [&](const gfx::Vertex& vertex) {
        const float column = vertex.uv.x * static_cast<float>(picture.width);
        const float row = vertex.uv.y * static_cast<float>(picture.height);
        return column > static_cast<float>(glassLeft) &&
               column < static_cast<float>(glassRight + 1) &&
               row > static_cast<float>(glassBottom) && row < static_cast<float>(glassTop + 1);
    };
    std::size_t panes = 0;
    for (std::size_t i = 0; i + 2 < lantern.indices.size(); i += 3) {
        const gfx::Vertex& a = lantern.vertices[lantern.indices[i]];
        const gfx::Vertex& b = lantern.vertices[lantern.indices[i + 1]];
        const gfx::Vertex& c = lantern.vertices[lantern.indices[i + 2]];
        const int corners = (onGlass(a) ? 1 : 0) + (onGlass(b) ? 1 : 0) + (onGlass(c) ? 1 : 0);
        // A face is all on the glass or not on it at all.
        CHECK((corners == 0 || corners == 3));
        if (corners != 3) {
            continue;
        }
        ++panes;
        // A pane stands upright in a window and looks out of the lantern.
        const glm::vec3 normal =
            glm::normalize(glm::cross(b.position - a.position, c.position - a.position));
        const glm::vec3 middle = (a.position + b.position + c.position) / 3.0F;
        CHECK(std::abs(normal.y) < 0.01F);
        CHECK(glm::dot(normal, glm::vec3{middle.x, 0.0F, middle.z}) > 0.0F);
    }
    CHECK(panes == LANTERN_PANE_TRIANGLES);

    const assets::ObjModel milestone = loadModel("milestone.obj");
    REQUIRE_FALSE(milestone.vertices.empty());
    CHECK(milestone.parts.size() == 1);
    CHECK(milestone.indices.size() / 3 == MILESTONE_TRIANGLES);
    CHECK(milestone.mirroredTriangleCount == MILESTONE_MIRRORED);
    float stoneTop = 0.0F;
    for (const gfx::Vertex& vertex : milestone.vertices) {
        stoneTop = std::max(stoneTop, vertex.position.y);
    }
    // Knee high: between 0.4 and 0.8 m.
    CHECK(stoneTop > 0.4F);
    CHECK(stoneTop < 0.8F);
}
TEST_CASE("a bell that is never pushed hangs still, and the bell of a shut gate is never pushed") {
    // The gate stays shut for a minute: the clock of the bell gives no toll, so nothing
    // pushes the bell, and it stays exactly where it hangs.
    const game::Round closed;
    game::GateBell bell;
    game::BellSwing swing;
    for (int i = 0; i < 120 * 60; ++i) {
        if (game::advanceGateBell(bell, closed, game::GATE_BELL_SECONDS, STEP)) {
            game::tollBellSwing(swing);
        }
        game::advanceBellSwing(swing, STEP);
        REQUIRE(swing.degrees == 0.0F);
        REQUIRE(swing.degreesPerSecond == 0.0F);
    }
}

TEST_CASE("a toll swings the bell to both sides and the swing dies away before the next toll") {
    game::BellSwing swing;
    game::tollBellSwing(swing);

    // One wait between two tolls, step by step.
    const int steps = static_cast<int>(game::GATE_BELL_SECONDS / STEP);
    float furthest = 0.0F;
    float furthestBack = 0.0F;
    float afterFourSeconds = 0.0F;
    int turns = 0;
    float before = 0.0F;
    for (int i = 1; i <= steps; ++i) {
        game::advanceBellSwing(swing, STEP);
        CHECK(std::abs(swing.degrees) <= game::GATE_BELL_SWING_DEGREES);
        furthest = std::max(furthest, swing.degrees);
        furthestBack = std::min(furthestBack, swing.degrees);
        // A change of side: the bell went through the middle.
        if (before * swing.degrees < 0.0F) {
            ++turns;
        }
        if (swing.degrees != 0.0F) {
            before = swing.degrees;
        }
        if (static_cast<float>(i) * STEP >= 4.0F) {
            afterFourSeconds = std::max(afterFourSeconds, std::abs(swing.degrees));
        }
        // It starts with the toll: after a tenth of a second it is well on its way.
        if (i == 12) {
            CHECK(swing.degrees > 5.0F);
        }
    }
    // Nearly the whole way out on the first swing, and well out on the way back.
    CHECK(furthest > game::GATE_BELL_SWING_DEGREES - 2.0F);
    CHECK(furthestBack < -game::GATE_BELL_SWING_DEGREES / 3.0F);
    // Several swings, not one slow lean.
    CHECK(turns >= 4);
    // Under a degree is left after four seconds, and at the next toll it hangs still.
    CHECK(afterFourSeconds < 1.0F);
    CHECK(swing.degrees == 0.0F);
    CHECK(swing.degreesPerSecond == 0.0F);
}

TEST_CASE("tolls in quick succession never swing the bell past its limit") {
    // A toll every second, the shortest wait the debug UI allows, and then one in every
    // step: the pushes add up, and the stop holds the bell.
    game::BellSwing swing;
    for (int i = 0; i < 120 * 20; ++i) {
        if (i % 120 == 0 || i > 120 * 10) {
            game::tollBellSwing(swing);
        }
        game::advanceBellSwing(swing, STEP);
        REQUIRE(std::abs(swing.degrees) <= game::GATE_BELL_SWING_DEGREES);
    }
    // Left alone it comes to rest again.
    for (int i = 0; i < 120 * 10; ++i) {
        game::advanceBellSwing(swing, STEP);
    }
    CHECK(swing.degrees == 0.0F);
}

TEST_CASE("the bell model hangs from its pivot and swings clear of the stone and the door") {
    const assets::ObjModel bell = loadModel("gate_bell.obj");
    REQUIRE_FALSE(bell.vertices.empty());
    CHECK(bell.parts.size() == 1);
    CHECK(bell.indices.size() / 3 == BELL_TRIANGLES);
    CHECK(bell.mirroredTriangleCount == BELL_MIRRORED);

    // Everything hangs under the pivot, the origin, around the axis down from it, and
    // the clapper is the lowest point.
    float lowest = 0.0F;
    glm::vec2 low{0.0F};
    glm::vec2 high{0.0F};
    for (const gfx::Vertex& vertex : bell.vertices) {
        const glm::vec3& p = vertex.position;
        CHECK(p.y <= 0.0001F);
        lowest = std::min(lowest, p.y);
        low = glm::min(low, glm::vec2{p.x, p.z});
        high = glm::max(high, glm::vec2{p.x, p.z});
    }
    CHECK(lowest == doctest::Approx(-BELL_HEIGHT));
    CHECK(std::abs(low.x + high.x) < 0.02F);
    CHECK(std::abs(low.y + high.y) < 0.02F);

    // The bell is open below, and the renderer draws no back faces: somebody who looks
    // up into it sees faces that look back down at them, the inside of its wall.
    for (const float radius : {0.06F, 0.1F}) {
        for (int step = 0; step < 8; ++step) {
            const float angle = static_cast<float>(step) * glm::quarter_pi<float>();
            const glm::vec3 below{radius * std::cos(angle), -BELL_HEIGHT - 0.1F,
                                  radius * std::sin(angle)};
            const RayHit hit = castRay(bell, below, {0.0F, 1.0F, 0.0F});
            CAPTURE(below);
            CHECK(hit.distance > 0.0F);
            CHECK(hit.normal.y < 0.0F);
        }
    }

    // In its opening of the arch, at rest and swung out as far as it goes to either side,
    // with the matrix the game draws with for an arch at the origin: no stone stands
    // between the axis of the opening and any corner of the bell (only the top of the
    // rope reaches into the stone over it), and all of it hangs above the top of the
    // closed door.
    const assets::ObjModel arch = loadModel("gate_arch.obj");
    const assets::ObjModel door = loadModel("gate.obj");
    float doorTop = 0.0F;
    for (const gfx::Vertex& vertex : door.vertices) {
        doorTop = std::max(doorTop, vertex.position.y);
    }
    for (const float degrees :
         {-game::GATE_BELL_SWING_DEGREES, 0.0F, game::GATE_BELL_SWING_DEGREES}) {
        CAPTURE(degrees);
        const glm::mat4 matrix = game::gateBellMatrix(glm::mat4{1.0F}, degrees);
        for (const gfx::Vertex& vertex : bell.vertices) {
            if (vertex.position.y > -BELL_ROPE_IN_STONE) {
                continue;
            }
            const glm::vec3 place{matrix * glm::vec4{vertex.position, 1.0F}};
            CHECK(place.y > doorTop);
            const float side = place.x < 0.0F ? -1.0F : 1.0F;
            const RayHit stone = castRay(arch, {0.0F, place.y, place.z}, {side, 0.0F, 0.0F});
            CHECK((stone.distance < 0.0F || stone.distance > std::abs(place.x)));
        }
    }

    // The opening goes through the slab over the door: from both sides of the gate, the
    // middle of the bell is seen and not the stone.
    const float middle = game::GATE_BELL_HEIGHT - BELL_HEIGHT / 2.0F;
    const glm::mat4 still = game::gateBellMatrix(glm::mat4{1.0F}, 0.0F);
    for (const float face : {-1.0F, 1.0F}) {
        const glm::vec3 eye{0.0F, middle, face * 2.0F};
        const glm::vec3 inBell{glm::inverse(still) * glm::vec4{eye, 1.0F}};
        const RayHit seenBell = castRay(bell, inBell, {0.0F, 0.0F, -face});
        const RayHit seenStone = castRay(arch, eye, {0.0F, 0.0F, -face});
        CHECK(seenBell.distance > 0.0F);
        CHECK((seenStone.distance < 0.0F || seenStone.distance > seenBell.distance));
    }
}
