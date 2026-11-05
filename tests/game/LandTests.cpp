// Tests of game::Land: the ground of the ring, where the pieces of the land stand, what they
// keep clear of, the windows and the nights that light them.
#include "game/Land.hpp"

#include "assets/ImageLoader.hpp"
#include "assets/ObjLoader.hpp"
#include "core/Files.hpp"
#include "game/Grass.hpp"
#include "game/LandMeshes.hpp"
#include "game/MazeWorld.hpp"
#include "game/Minimap.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "game/Settings.hpp"
#include "game/Terrain.hpp"
#include "game/Village.hpp"
#include "scene/Frustum.hpp"

#include <doctest/doctest.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace {

// The five sizes of the campaign.
constexpr std::array<int, 5> GAME_SIZES = {10, 13, 16, 19, 22};
constexpr std::uint32_t SEED_COUNT = 12;
constexpr float SLACK = 0.001F;

std::filesystem::path assetsDirectory() {
    return {NIGHT_MAZE_ASSETS_DIR};
}

const game::Heightmap& realHeightmap() {
    static const game::Heightmap heightmap = [] {
        assets::Image image;
        std::string error;
        const bool loaded = assets::loadImage(assetsDirectory() / "textures" / "heightmap.png",
                                              image, error, assets::RowOrder::TopFirst);
        REQUIRE_MESSAGE(loaded, error);
        return game::heightmapFromImage(image);
    }();
    return heightmap;
}

game::MazeWorld realWorld(int size, std::uint32_t seed) {
    return game::buildMazeWorld(size, size, seed, realHeightmap(), game::DEFAULT_HEIGHT_SCALE);
}

template <typename Check>
void forEveryWorld(Check check) {
    for (const int size : GAME_SIZES) {
        for (std::uint32_t seed = 0; seed < SEED_COUNT; ++seed) {
            CAPTURE(size);
            CAPTURE(seed);
            check(realWorld(size, seed));
        }
    }
}

float distanceToLine(const glm::vec2& point, const game::FieldLine& line) {
    const glm::vec2 along = line.to - line.from;
    const float t =
        std::clamp(glm::dot(point - line.from, along) / glm::dot(along, along), 0.0F, 1.0F);
    return glm::length(point - (line.from + along * t));
}

glm::vec2 ground(const game::LandPiece& piece) {
    return {piece.position.x, piece.position.z};
}

} // namespace

TEST_CASE("the land changes nothing that was there before it") {
    // The land has a generator of its own: the maze, its walls, crystals and sheep are the
    // ones a world without the land had. This compares against a world rebuilt on flat
    // ground, whose land differs, but whose maze must not.
    const game::MazeWorld hills = realWorld(16, 7);
    const game::MazeWorld flat = game::buildMazeWorld(16, 16, 7);
    REQUIRE(hills.walls.size() == flat.walls.size());
    CHECK(hills.crystals.size() == flat.crystals.size());
    for (std::size_t i = 0; i < hills.crystals.size(); ++i) {
        CHECK(hills.crystals[i].cell == flat.crystals[i].cell);
    }
    REQUIRE(hills.sheep.size() == flat.sheep.size());
    for (std::size_t i = 0; i < hills.sheep.size(); ++i) {
        CHECK(hills.sheep[i].position.x == flat.sheep[i].position.x);
        CHECK(hills.sheep[i].position.z == flat.sheep[i].position.z);
    }
    CHECK(hills.exitCell == flat.exitCell);
}

TEST_CASE("the same maze always gets the same land") {
    const game::MazeWorld first = realWorld(13, 3);
    const game::MazeWorld second = realWorld(13, 3);
    REQUIRE(first.land.pieces.size() == second.land.pieces.size());
    for (std::size_t i = 0; i < first.land.pieces.size(); ++i) {
        CHECK(first.land.pieces[i].model == second.land.pieces[i].model);
        CHECK(first.land.pieces[i].position == second.land.pieces[i].position);
        CHECK(first.land.pieces[i].yawDegrees == second.land.pieces[i].yawDegrees);
    }
    CHECK(first.land.fieldLines.size() == second.land.fieldLines.size());
    CHECK(first.land.ringHeights == second.land.ringHeights);
}

TEST_CASE("every maze gets three landmarks, one on each crest away from the village") {
    forEveryWorld([](const game::MazeWorld& world) {
        const std::vector<game::LandmarkView> views = game::landmarks(world.land);
        CHECK(views.size() == game::LAND_LANDMARKS_PER_MAZE);
        std::set<game::LandModel> kinds;
        for (const game::LandmarkView& view : views) {
            kinds.insert(view.model);
            CHECK(static_cast<int>(view.model) >= static_cast<int>(game::FIRST_LANDMARK_MODEL));
            const float out = game::distanceOutsideMaze(view.top.x, view.top.z, world.maze.width(),
                                                        world.maze.height());
            CHECK(out >= 25.0F - SLACK);
            CHECK(out <= 35.0F + SLACK);
        }
        CHECK(kinds.size() == views.size());
    });
}

TEST_CASE("the pieces stay in their bands, off the field lines and clear of the reveal") {
    forEveryWorld([](const game::MazeWorld& world) {
        const game::Land& land = world.land;
        CHECK(land.nearCount >= 8);
        CHECK(land.pieces.size() >= land.nearCount + 20);
        const glm::vec3 camera =
            world.exitPosition + glm::vec3{0.0F, game::VILLAGE_REVEAL_HEIGHT, 0.0F};
        for (std::size_t i = 0; i < land.pieces.size(); ++i) {
            CAPTURE(i);
            const game::LandPiece& piece = land.pieces[i];
            const game::LandModelShape& shape = game::landModelShape(piece.model);
            const float radius = shape.radius * piece.scale;
            const glm::vec2 point = ground(piece);
            const float out = game::distanceOutsideMaze(point.x, point.y, world.maze.width(),
                                                        world.maze.height());
            CHECK(out - radius >= game::LAND_NEAR_START - SLACK);
            CHECK(out <= game::LAND_REACH + SLACK);
            CHECK(game::isNearLandModel(piece.model) == (i < land.nearCount));
            if (game::isNearLandModel(piece.model)) {
                CHECK(out + radius <= game::TERRAIN_MARGIN + SLACK);
                CHECK(game::landPieceTop(piece) - piece.position.y <= game::LAND_NEAR_TALLEST);
            }
            for (const game::FieldLine& line : land.fieldLines) {
                CHECK(distanceToLine(point, line) > game::LAND_LINE_CLEARANCE);
            }
            if (i < land.pieces.size() - land.landmarkCount) {
                const glm::vec3 top{point.x, game::landPieceTop(piece), point.y};
                CHECK(game::landKeepsRevealClear(camera, land.shape.villageSide, top));
            }
        }
        for (const game::FieldLine& line : land.fieldLines) {
            for (const glm::vec2 end : {line.from, line.to}) {
                const float out = game::distanceOutsideMaze(end.x, end.y, world.maze.width(),
                                                            world.maze.height());
                CHECK(out >= game::LAND_NEAR_START);
                CHECK(out <= game::TERRAIN_MARGIN);
            }
        }
    });
}

TEST_CASE("the ground of the ring meets the terrain and ends below the fog") {
    const game::MazeWorld world = realWorld(16, 5);
    const game::Land& land = world.land;
    const float width = 16.0F * game::CELL_SIZE;
    // On the edge of the terrain the ring is the terrain.
    for (float x = -game::TERRAIN_MARGIN; x <= width + game::TERRAIN_MARGIN; x += 2.0F) {
        CAPTURE(x);
        CHECK(game::landRingHeight(land.shape, world.terrain, x, -game::TERRAIN_MARGIN) ==
              doctest::Approx(world.terrain.heightAt(x, -game::TERRAIN_MARGIN)));
    }
    // At the end of the ring every side is below y = 0.
    for (float x = 0.0F; x <= width; x += 4.0F) {
        CHECK(game::landGroundHeight(land, x, -game::LAND_RING_END) < 0.0F);
        CHECK(game::landGroundHeight(land, x, width + game::LAND_RING_END) < 0.0F);
        CHECK(game::landGroundHeight(land, -game::LAND_RING_END, x) < 0.0F);
    }
    // A crest away from the village rises to 8 to 12 m, the valley floor lies low.
    const game::Direction village = land.shape.villageSide;
    for (const game::Direction side : game::ALL_DIRECTIONS) {
        float highest = -100.0F;
        for (float out = game::TERRAIN_MARGIN; out <= game::LAND_REACH; out += 1.0F) {
            const glm::vec2 point =
                glm::vec2{width / 2.0F} + glm::vec2{static_cast<float>(game::columnStep(side)),
                                                    static_cast<float>(game::rowStep(side))} *
                                              (width / 2.0F + out);
            highest = std::max(highest,
                               game::landRingHeight(land.shape, world.terrain, point.x, point.y));
        }
        CAPTURE(side);
        if (side == village) {
            CHECK(highest < game::LAND_CREST_LOWEST);
        } else {
            CHECK(highest >= game::LAND_CREST_LOWEST - 0.5F);
            CHECK(highest <= game::LAND_CREST_HIGHEST + SLACK);
        }
    }
}

TEST_CASE("the reveal rule keeps a cone above the camera clear") {
    const glm::vec3 camera{0.0F, 5.0F, 0.0F};
    // North is -z. Straight ahead, 20 m out: 5 degrees is 1.75 m over the camera.
    CHECK(game::landKeepsRevealClear(camera, game::Direction::North, {0.0F, 6.5F, -20.0F}));
    CHECK_FALSE(game::landKeepsRevealClear(camera, game::Direction::North, {0.0F, 7.0F, -20.0F}));
    // Outside the cone anything goes.
    CHECK(game::landKeepsRevealClear(camera, game::Direction::North, {10.0F, 30.0F, -20.0F}));
    // Behind the camera too.
    CHECK(game::landKeepsRevealClear(camera, game::Direction::North, {0.0F, 30.0F, 20.0F}));
}

TEST_CASE("the windows are dealt to nights one to four in their shares") {
    for (const std::size_t count : {0U, 1U, 7U, 40U, 101U}) {
        CAPTURE(count);
        const auto nights = game::dealLandWindows(count, 9);
        std::vector<std::size_t> all;
        for (const auto& night : nights) {
            all.insert(all.end(), night.begin(), night.end());
        }
        std::ranges::sort(all);
        std::vector<std::size_t> expected(count);
        for (std::size_t i = 0; i < count; ++i) {
            expected[i] = i;
        }
        CHECK(all == expected);
        if (count == 40) {
            CHECK(nights[0].size() == 8);
            CHECK(nights[1].size() == 10);
            CHECK(nights[2].size() == 10);
            CHECK(nights[3].size() == 12);
        }
    }
    CHECK(game::dealLandWindows(40, 9)[0] == game::dealLandWindows(40, 9)[0]);
    CHECK(game::dealLandWindows(40, 9)[0] != game::dealLandWindows(40, 10)[0]);
}

TEST_CASE("a windows file is read line by line") {
    const std::vector<game::LandWindow> windows = game::parseLandWindows(
        "# a comment\n1 2 3 0 0 -1 0.5 0.75\n\nnot a window\n4 5 6 1 0 0 1 1\n");
    REQUIRE(windows.size() == 2);
    CHECK(windows[0].middle == glm::vec3{1.0F, 2.0F, 3.0F});
    CHECK(windows[0].normal == glm::vec3{0.0F, 0.0F, -1.0F});
    CHECK(windows[0].width == 0.5F);
    CHECK(windows[0].height == 0.75F);
    CHECK(windows[1].middle == glm::vec3{4.0F, 5.0F, 6.0F});
}

TEST_CASE("the models of the land are as high as placement thinks") {
    for (int i = 0; i < game::LAND_MODEL_COUNT; ++i) {
        const auto model = static_cast<game::LandModel>(i);
        const std::string name{game::landModelFile(model)};
        CAPTURE(name);
        assets::ObjModel obj;
        std::string error;
        REQUIRE_MESSAGE(assets::loadObj(assetsDirectory() / "models" / (name + ".obj"), obj, error),
                        error);
        float lowest = 1000.0F;
        float highest = -1000.0F;
        glm::vec2 groundMin{1000.0F};
        glm::vec2 groundMax{-1000.0F};
        for (const gfx::Vertex& vertex : obj.vertices) {
            lowest = std::min(lowest, vertex.position.y);
            highest = std::max(highest, vertex.position.y);
            groundMin = glm::min(groundMin, glm::vec2{vertex.position.x, vertex.position.z});
            groundMax = glm::max(groundMax, glm::vec2{vertex.position.x, vertex.position.z});
        }
        // The radius placement keeps around a piece is about half of its wider side: less
        // for a tree, whose crown may reach over a neighbour, a little more for a cottage
        // with its yard. A model swapped for one of another shape fails here, and the
        // build script prints the numbers to put into MODEL_SHAPES.
        const glm::vec2 ground = groundMax - groundMin;
        const float halfWidth = std::max(ground.x, ground.y) / 2.0F;
        CHECK(game::landModelShape(model).radius >= 0.55F * halfWidth);
        CHECK(game::landModelShape(model).radius <= 1.3F * halfWidth);
        // The build sinks a model by up to 0.3 m below its origin and scales it to its
        // height from its foot.
        CHECK(highest - lowest ==
              doctest::Approx(game::landModelShape(model).height).epsilon(0.03));
        CHECK(lowest <= 0.0F);
        CHECK(lowest >= -0.35F);
        if (game::landModelShape(model).windows) {
            std::string text;
            REQUIRE(core::readTextFile(assetsDirectory() / "models" / game::landWindowsFile(model),
                                       text));
            CHECK_FALSE(game::parseLandWindows(text).empty());
        }
    }
}

TEST_CASE("the far silhouettes stay inside the reach of the village") {
    constexpr float FAR_PLANE = 300.0F;
    for (const auto silhouette : {game::FarSilhouette::Mill, game::FarSilhouette::ChurchTower}) {
        for (const game::Direction side : game::ALL_DIRECTIONS) {
            const glm::mat4 matrix = game::farSilhouetteMatrix(silhouette, side, FAR_PLANE, 0.0F);
            const glm::vec3 foot{matrix * glm::vec4{0.0F, 0.0F, 0.0F, 1.0F}};
            CHECK(glm::length(foot) < game::VILLAGE_FAR_SHARE * FAR_PLANE + SLACK);
        }
    }
    CHECK(game::farSilhouetteNight(game::FarSilhouette::Mill) == 2);
    CHECK(game::farSilhouetteNight(game::FarSilhouette::ChurchTower) == 4);
}

TEST_CASE("the land keeps to its budget of draws and triangles") {
    // The triangles of every model, read once.
    std::array<std::size_t, game::LAND_MODEL_COUNT> triangles{};
    for (int i = 0; i < game::LAND_MODEL_COUNT; ++i) {
        assets::ObjModel obj;
        std::string error;
        const std::string name{game::landModelFile(static_cast<game::LandModel>(i))};
        REQUIRE_MESSAGE(assets::loadObj(assetsDirectory() / "models" / (name + ".obj"), obj, error),
                        error);
        triangles.at(static_cast<std::size_t>(i)) = obj.indices.size() / 3;
    }
    // Besides the pieces the land draws the ring, the walls, the hedgerows, the stream and
    // the windows of four nights: 8 draws and about 10,000 triangles.
    static constexpr std::size_t MOST_PIECES = 82;
    static constexpr std::size_t MOST_PIECE_TRIANGLES = 140000;
    forEveryWorld([&triangles](const game::MazeWorld& world) {
        std::size_t sum = 0;
        for (const game::LandPiece& piece : world.land.pieces) {
            sum += triangles.at(static_cast<std::size_t>(piece.model));
        }
        CHECK(world.land.pieces.size() <= MOST_PIECES);
        CHECK(sum <= MOST_PIECE_TRIANGLES);
        const game::TerrainMeshData ring = game::buildLandRingMesh(world.land, world.terrain);
        const game::TerrainMeshData walls = game::buildFieldWallMesh(world.land, world.terrain);
        const game::TerrainMeshData hedges =
            game::buildHedgerowMesh(world.land, world.terrain, world.seed);
        CHECK((ring.indices.size() + walls.indices.size() + hedges.indices.size()) / 3 <= 12000);
    });
}

TEST_CASE("the ring meets the edge of the terrain without a crack") {
    // Every point of the ring on the edge of the terrain is at the height of the terrain
    // there, and the row under the edge lies below the terrain, so the two meshes overlap.
    const game::MazeWorld world = realWorld(13, 2);
    const game::TerrainMeshData ring = game::buildLandRingMesh(world.land, world.terrain);
    REQUIRE_FALSE(ring.indices.empty());
    const auto onEdge = [](float value, float low, float high) {
        return std::abs(value - low) < SLACK || std::abs(value - high) < SLACK;
    };
    std::set<std::uint32_t> used(ring.indices.begin(), ring.indices.end());
    int edgePoints = 0;
    for (const std::uint32_t index : used) {
        const glm::vec3 p = ring.vertices[index].position;
        const bool insideX =
            p.x > world.terrain.minX() - SLACK && p.x < world.terrain.maxX() + SLACK;
        const bool insideZ =
            p.z > world.terrain.minZ() - SLACK && p.z < world.terrain.maxZ() + SLACK;
        if (!insideX || !insideZ) {
            continue;
        }
        const bool edge = onEdge(p.x, world.terrain.minX(), world.terrain.maxX()) ||
                          onEdge(p.z, world.terrain.minZ(), world.terrain.maxZ());
        if (edge) {
            ++edgePoints;
            CHECK(p.y == doctest::Approx(world.terrain.heightAt(p.x, p.z)).epsilon(0.001));
        } else {
            CHECK(p.y < world.terrain.heightAt(p.x, p.z));
        }
    }
    CHECK(edgePoints > 0);
}

TEST_CASE("the field walls and the hedgerows stand on the ground") {
    const game::MazeWorld world = realWorld(16, 7);
    const game::TerrainMeshData walls = game::buildFieldWallMesh(world.land, world.terrain);
    const game::TerrainMeshData hedges = game::buildHedgerowMesh(world.land, world.terrain, 7);
    REQUIRE_FALSE(walls.indices.empty());
    for (const game::TerrainMeshData* mesh : {&walls, &hedges}) {
        for (const gfx::Vertex& vertex : mesh->vertices) {
            const float ground = world.terrain.heightAt(vertex.position.x, vertex.position.z);
            // No foot floats: the lowest points are sunk, the highest within the height
            // of a hedge and its lumps.
            CHECK(vertex.position.y >= ground - game::LAND_WALL_SINK - 0.3F);
            CHECK(vertex.position.y <=
                  ground + game::LAND_HEDGE_HEIGHT + game::LAND_HEDGE_LUMP + 0.3F);
        }
    }
}

TEST_CASE("the map shows a seen landmark on its edge") {
    const game::MazeWorld world = realWorld(13, 4);
    game::Round round = game::startRound(world, {});
    REQUIRE(round.landmarksSeen.size() == game::LAND_LANDMARKS_PER_MAZE);
    const game::MinimapPlayer player{.position = world.startPosition};
    const auto landmarkVertices = [&]() {
        const std::vector<game::MinimapVertex> vertices =
            game::buildMinimapVertices(world, round, false, player, 0.01F);
        return std::ranges::count_if(vertices, [](const game::MinimapVertex& vertex) {
            return vertex.color == game::MINIMAP_LANDMARK_COLOR;
        });
    };
    CHECK(landmarkVertices() == 0);
    round.landmarksSeen[0] = true;
    CHECK(landmarkVertices() > 0);
}

TEST_CASE("a landmark is seen only in the frustum and over the walls") {
    const glm::mat4 projection = glm::perspective(glm::radians(60.0F), 1.5F, 0.1F, 300.0F);
    const glm::vec3 eye{0.0F, 1.7F, 0.0F};
    const glm::mat4 view = glm::lookAt(eye, {0.0F, 1.7F, -1.0F}, {0.0F, 1.0F, 0.0F});
    const scene::Frustum frustum = scene::frustumFromMatrix(projection * view);
    const glm::vec3 ahead{0.0F, 8.0F, -30.0F};
    const glm::vec3 behind{0.0F, 8.0F, 30.0F};
    const std::vector<scene::Aabb> none;
    CHECK(game::landmarkInSight(eye, ahead, frustum, none));
    CHECK_FALSE(game::landmarkInSight(eye, behind, frustum, none));
    // A wall 3 m high, 2 m ahead, hides it; one off to the side does not.
    const std::vector<scene::Aabb> wall{{.min = {-2.0F, 0.0F, -2.2F}, .max = {2.0F, 3.0F, -2.0F}}};
    CHECK_FALSE(game::landmarkInSight(eye, ahead, frustum, wall));
    const std::vector<scene::Aabb> aside{{.min = {4.0F, 0.0F, -2.2F}, .max = {6.0F, 3.0F, -2.0F}}};
    CHECK(game::landmarkInSight(eye, ahead, frustum, aside));
}

TEST_CASE("the land knows where its walls, hedgerows and buildings cover the ground") {
    const game::MazeWorld world = realWorld(16, 3);
    const game::Land& land = world.land;
    REQUIRE_FALSE(land.fieldLines.empty());
    const game::FieldLine& line = land.fieldLines.front();
    const glm::vec2 middle = (line.from + line.to) / 2.0F;
    CHECK(game::landCoversGround(land, middle.x, middle.y));
    // Far off every line, piece and the fold: the middle of the maze.
    const glm::vec3 centre = world.startPosition;
    CHECK_FALSE(game::landCoversGround(land, centre.x, centre.z));
}

TEST_CASE("no grass grows through the field walls, the hedgerows or the buildings") {
    forEveryWorld([](const game::MazeWorld& world) {
        const std::vector<game::GrassTuft> tufts = game::placeGrass(world, 2.5F);
        std::size_t covered = 0;
        for (const game::GrassTuft& tuft : tufts) {
            if (game::landCoversGround(world.land, tuft.position.x, tuft.position.z)) {
                ++covered;
            }
        }
        CHECK(covered == 0);
    });
}

namespace {

// The views down a corridor that runs straight to the border wall, from up to
// BORDER_CELLS - 1 cells before it, looking level along it with the field of view of the
// settings, and how many of them show the crown of a tree or a roof of the land over the
// walls: a point of a piece at one of CROWN_SHARES of its height in the frustum, with
// nothing of the maze in the way. The eye is the one of the player.
struct BorderViews {
    int views = 0;
    int withNear = 0;
    int withAny = 0;
};

constexpr int BORDER_CELLS = 6;
constexpr std::array<float, 4> CROWN_SHARES{0.7F, 0.8F, 0.9F, 1.0F};

BorderViews borderViews(const game::MazeWorld& world) {
    const glm::mat4 projection = glm::perspective(glm::radians(game::DEFAULT_FIELD_OF_VIEW_DEGREES),
                                                  16.0F / 9.0F, 0.1F, 100.0F);
    const int width = world.maze.width();
    const int height = world.maze.height();
    BorderViews result;
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            for (const game::Direction side : game::ALL_DIRECTIONS) {
                const int toBorder = side == game::Direction::North   ? z
                                     : side == game::Direction::South ? height - 1 - z
                                     : side == game::Direction::West  ? x
                                                                      : width - 1 - x;
                if (toBorder == 0 || toBorder >= BORDER_CELLS) {
                    continue;
                }
                bool open = true;
                for (int k = 0; k < toBorder && open; ++k) {
                    open = !world.maze.hasWall(x + k * game::columnStep(side),
                                               z + k * game::rowStep(side), side);
                }
                if (!open) {
                    continue;
                }
                const glm::vec3 centre = game::cellCenter(x, z);
                const glm::vec3 eye{
                    centre.x, world.terrain.heightAt(centre.x, centre.z) + game::Player::EYE_HEIGHT,
                    centre.z};
                const float yaw = glm::radians(game::yawTowards(side));
                const glm::vec3 ahead{std::sin(yaw), 0.0F, -std::cos(yaw)};
                const glm::mat4 view = glm::lookAt(eye, eye + ahead, {0.0F, 1.0F, 0.0F});
                const scene::Frustum frustum = scene::frustumFromMatrix(projection * view);
                bool seenNear = false;
                bool seenAny = false;
                for (std::size_t i = 0; i < world.land.pieces.size() && !seenNear; ++i) {
                    const game::LandPiece& piece = world.land.pieces[i];
                    const float top = game::landPieceTop(piece);
                    bool shows = false;
                    for (const float share : CROWN_SHARES) {
                        const glm::vec3 crown{piece.position.x,
                                              glm::mix(piece.position.y, top, share),
                                              piece.position.z};
                        shows =
                            shows || game::landmarkInSight(eye, crown, frustum, world.colliders);
                    }
                    if (shows) {
                        seenAny = true;
                        seenNear = seenNear || i < world.land.nearCount;
                    }
                }
                ++result.views;
                result.withNear += seenNear ? 1 : 0;
                result.withAny += seenAny ? 1 : 0;
            }
        }
    }
    return result;
}

} // namespace

TEST_CASE("most corridor views towards the border show a crown or a roof over the walls") {
    // The near band alone, and with the middle band, which stands higher and farther out.
    // Measured 2026-10-28: 41 and 65 % before the trees and roofs were made taller and the
    // near trees more, 65 and 81 % after.
    BorderViews all;
    for (const int size : {10, 16, 22}) {
        for (std::uint32_t seed = 0; seed < 6; ++seed) {
            CAPTURE(size);
            CAPTURE(seed);
            const BorderViews one = borderViews(realWorld(size, seed));
            all.views += one.views;
            all.withNear += one.withNear;
            all.withAny += one.withAny;
        }
    }
    REQUIRE(all.views > 0);
    const float near = static_cast<float>(all.withNear) / static_cast<float>(all.views);
    const float any = static_cast<float>(all.withAny) / static_cast<float>(all.views);
    CHECK(near > 0.6F);
    CHECK(any > 0.75F);
}
