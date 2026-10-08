// Tests of game::Terrain: the heightmap lookup, the height formula, heightAt against the
// drawn triangles, and the maze, the round and the player standing on uneven ground.
#include "game/Terrain.hpp"

#include "assets/ImageLoader.hpp"
#include "assets/Tangents.hpp"
#include "game/Crystals.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// The fixed step of the game: core::Time::FIXED_DT as a float.
constexpr float STEP_SECONDS = 1.0F / 120.0F;

// How close two heights have to be to count as the same, in metres: a hundredth of
// a millimetre. The numbers compared come from different formulas for the same plane, so
// they differ by rounding only.
constexpr float HEIGHT_TOLERANCE = 0.00001F;

// A random number from 0 to just below 1, from the generator of the project.
float randomUnit(std::mt19937& generator) {
    constexpr std::uint32_t STEPS = 100000U;
    return static_cast<float>(game::randomBelow(generator, STEPS)) / static_cast<float>(STEPS);
}

// How far along a range sample number index of count steps is, from 0 to 1.
float partOf(int index, int count) {
    return static_cast<float>(index) / static_cast<float>(count);
}

// A rough heightmap for the tests: 16 by 16 values from 0 to 1 that jump up and down
// from one value to the next, so every grid square of a terrain made from it is bent.
// It is made by a formula and not by a random generator, so it is the same in every run.
game::Heightmap roughHeightmap() {
    constexpr int SIZE = 16;
    constexpr int LEVELS = 10;
    game::Heightmap heightmap;
    heightmap.width = SIZE;
    heightmap.height = SIZE;
    heightmap.values.clear();
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            const int level = (column * 7 + row * 13 + column * row) % LEVELS;
            heightmap.values.push_back(static_cast<float>(level) / static_cast<float>(LEVELS - 1));
        }
    }
    return heightmap;
}

// A heightmap with the same value everywhere.
game::Heightmap constantHeightmap(float value) {
    game::Heightmap heightmap;
    heightmap.values = {value};
    return heightmap;
}

// A heightmap that rises from west to east and is the same from north to south: two
// columns, 0 and 1.
game::Heightmap eastwardSlope() {
    game::Heightmap heightmap;
    heightmap.width = 2;
    heightmap.height = 1;
    heightmap.values = {0.0F, 1.0F};
    return heightmap;
}

// The height of the triangle of the mesh that lies above or below the point (x, z):
// the first triangle that contains the point, found by trying them all. Returns false
// when no triangle contains it.
//
// It knows nothing about the grid or the diagonal: it only reads the vertices and the
// indices, like the graphics card does. That makes it an independent check of heightAt.
bool meshHeightAt(const game::TerrainMeshData& mesh, float x, float z, float& height) {
    // A point just outside a triangle because of rounding still counts as inside.
    constexpr float EDGE_TOLERANCE = 0.0001F;

    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const glm::vec3& a = mesh.vertices[mesh.indices[i]].position;
        const glm::vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const glm::vec3& c = mesh.vertices[mesh.indices[i + 2]].position;

        // Barycentric coordinates in the horizontal plane: the point is
        // a + weightB * (b - a) + weightC * (c - a). It lies in the triangle when both
        // weights and the third one, 1 - weightB - weightC, are not negative.
        const float determinant = (b.x - a.x) * (c.z - a.z) - (c.x - a.x) * (b.z - a.z);
        const float weightB = ((x - a.x) * (c.z - a.z) - (c.x - a.x) * (z - a.z)) / determinant;
        const float weightC = ((b.x - a.x) * (z - a.z) - (x - a.x) * (b.z - a.z)) / determinant;
        const float weightA = 1.0F - weightB - weightC;
        if (weightA < -EDGE_TOLERANCE || weightB < -EDGE_TOLERANCE || weightC < -EDGE_TOLERANCE) {
            continue;
        }

        // The same weights blend the heights of the three corners: a triangle is flat.
        height = weightA * a.y + weightB * b.y + weightC * c.y;
        return true;
    }
    return false;
}

// Absolute path of the assets directory, compiled in by CMake.
std::filesystem::path assetsDirectory() {
    return {NIGHT_MAZE_ASSETS_DIR};
}

} // namespace

TEST_CASE("the terrain constants are the agreed sizes") {
    CHECK(game::TERRAIN_STEPS_PER_CELL == 4);
    CHECK(game::TERRAIN_SPACING == 0.5F);
    CHECK(game::TERRAIN_MARGIN == 14.0F);
    CHECK(game::HEIGHTMAP_SPAN == 48.0F);
    CHECK(game::DEFAULT_HEIGHT_SCALE == 1.0F);

    const game::TerrainSettings settings;
    CHECK(settings.heightScale == game::DEFAULT_HEIGHT_SCALE);
    CHECK_FALSE(settings.wireframe);
    CHECK_FALSE(settings.rebuild);

    // The largest ground difference inside the maze stays below the height of the body:
    // a player next to a wall then always overlaps its collision box in height.
    CHECK(game::MAX_HEIGHT_SCALE * game::MAZE_RELIEF < game::Player::BODY_HEIGHT);
}

TEST_CASE("Heightmap::sample blends the four values around a place and repeats") {
    // Two rows of two values:
    //   0.0  1.0
    //   0.5  0.25
    game::Heightmap heightmap;
    heightmap.width = 2;
    heightmap.height = 2;
    heightmap.values = {0.0F, 1.0F, 0.5F, 0.25F};

    SUBCASE("a value is found exactly at its own place") {
        CHECK(heightmap.sample(0.0F, 0.0F) == doctest::Approx(0.0F));
        CHECK(heightmap.sample(0.5F, 0.0F) == doctest::Approx(1.0F));
        CHECK(heightmap.sample(0.0F, 0.5F) == doctest::Approx(0.5F));
        CHECK(heightmap.sample(0.5F, 0.5F) == doctest::Approx(0.25F));
    }

    SUBCASE("half way between two values is their average") {
        CHECK(heightmap.sample(0.25F, 0.0F) == doctest::Approx(0.5F));
        CHECK(heightmap.sample(0.0F, 0.25F) == doctest::Approx(0.25F));
        // In the middle of all four: (0 + 1 + 0.5 + 0.25) / 4.
        CHECK(heightmap.sample(0.25F, 0.25F) == doctest::Approx(0.4375F));
    }

    SUBCASE("past the last value the first one follows again") {
        // Half way between the last value of the first row (1.0) and its first (0.0).
        CHECK(heightmap.sample(0.75F, 0.0F) == doctest::Approx(0.5F));
        // One whole picture further, and one back, is the same place.
        CHECK(heightmap.sample(1.25F, 0.25F) == doctest::Approx(0.4375F));
        CHECK(heightmap.sample(-0.75F, -0.75F) == doctest::Approx(0.4375F));
        CHECK(heightmap.sample(3.0F, -2.0F) == doctest::Approx(0.0F));
    }

    SUBCASE("a new heightmap is flat") {
        const game::Heightmap flat;
        CHECK(flat.sample(0.3F, 0.8F) == 0.0F);
        CHECK(flat.sample(-5.2F, 17.0F) == 0.0F);
    }
}

TEST_CASE("heightmapFromImage reads the first channel of every pixel") {
    SUBCASE("an RGB picture of 2 by 2 pixels") {
        assets::Image image;
        image.width = 2;
        image.height = 2;
        image.channels = 3;
        // Only the first byte of every pixel counts. The others are different on
        // purpose.
        image.pixels = {0, 9, 9, 255, 9, 9, 51, 9, 9, 102, 9, 9};

        const game::Heightmap heightmap = game::heightmapFromImage(image);
        CHECK(heightmap.width == 2);
        CHECK(heightmap.height == 2);
        REQUIRE(heightmap.values.size() == 4U);
        CHECK(heightmap.values[0] == doctest::Approx(0.0F));
        CHECK(heightmap.values[1] == doctest::Approx(1.0F));
        CHECK(heightmap.values[2] == doctest::Approx(0.2F));
        CHECK(heightmap.values[3] == doctest::Approx(0.4F));
    }

    SUBCASE("a picture without pixels, or with fewer than it claims, gives flat ground") {
        const assets::Image empty;
        const game::Heightmap fromEmpty = game::heightmapFromImage(empty);
        CHECK(fromEmpty.width == 1);
        CHECK(fromEmpty.height == 1);
        CHECK(fromEmpty.values == std::vector<float>{0.0F});

        assets::Image broken;
        broken.width = 4;
        broken.height = 4;
        broken.channels = 1;
        broken.pixels = {1, 2, 3};
        CHECK(game::heightmapFromImage(broken).values == std::vector<float>{0.0F});
    }
}

TEST_CASE("the relief is gentle inside the maze and grows into hills outside") {
    // A maze of 10 by 10 cells covers 0 to 20 m in x and in z.
    constexpr int SIZE = 10;

    SUBCASE("distanceOutsideMaze is 0 inside and the straight distance outside") {
        CHECK(game::distanceOutsideMaze(10.0F, 10.0F, SIZE, SIZE) == 0.0F);
        CHECK(game::distanceOutsideMaze(0.0F, 20.0F, SIZE, SIZE) == 0.0F);
        CHECK(game::distanceOutsideMaze(-3.0F, 10.0F, SIZE, SIZE) == doctest::Approx(3.0F));
        CHECK(game::distanceOutsideMaze(10.0F, 25.0F, SIZE, SIZE) == doctest::Approx(5.0F));
        // Past a corner: 3 m west and 4 m north of it are 5 m away.
        CHECK(game::distanceOutsideMaze(-3.0F, -4.0F, SIZE, SIZE) == doctest::Approx(5.0F));
    }

    SUBCASE("terrainRelief runs from MAZE_RELIEF to HILL_RELIEF over the margin") {
        CHECK(game::terrainRelief(0.0F) == doctest::Approx(game::MAZE_RELIEF));
        CHECK(game::terrainRelief(game::TERRAIN_MARGIN) == doctest::Approx(game::HILL_RELIEF));
        // Half way the smoothstep curve is at one half.
        CHECK(game::terrainRelief(game::TERRAIN_MARGIN / 2.0F) ==
              doctest::Approx((game::MAZE_RELIEF + game::HILL_RELIEF) / 2.0F));
        // Outside the range it stays at its ends.
        CHECK(game::terrainRelief(-1.0F) == doctest::Approx(game::MAZE_RELIEF));
        CHECK(game::terrainRelief(100.0F) == doctest::Approx(game::HILL_RELIEF));

        // It never falls on the way out.
        float previous = game::terrainRelief(0.0F);
        for (float distance = 0.5F; distance <= game::TERRAIN_MARGIN; distance += 0.5F) {
            const float relief = game::terrainRelief(distance);
            CHECK(relief >= previous);
            previous = relief;
        }
    }
}

TEST_CASE("a default terrain is flat ground at y = 0 everywhere") {
    const game::Terrain terrain;
    CHECK(terrain.heightAt(0.0F, 0.0F) == 0.0F);
    CHECK(terrain.heightAt(0.3F, 0.9F) == 0.0F);
    CHECK(terrain.heightAt(-250.0F, 1000.0F) == 0.0F);
    CHECK(terrain.minHeight() == 0.0F);
    CHECK(terrain.maxHeight() == 0.0F);
    CHECK(terrain.lowestHeightUnder(-5.0F, -5.0F, 5.0F, 5.0F) == 0.0F);
}

TEST_CASE("the grid covers the maze and the margin around it, 4 steps per cell") {
    const game::Terrain terrain(10, 6, roughHeightmap(), 1.0F);

    // 10 cells and 7 cells of margin on both sides are 24 cells, 96 steps, 97 points.
    CHECK(terrain.columns() == 97);
    // 6 + 14 cells are 20 cells, 80 steps, 81 points.
    CHECK(terrain.rows() == 81);
    CHECK(terrain.spacing() == 0.5F);
    CHECK(terrain.minX() == -14.0F);
    CHECK(terrain.minZ() == -14.0F);
    CHECK(terrain.maxX() == doctest::Approx(34.0F));
    CHECK(terrain.maxZ() == doctest::Approx(26.0F));
    CHECK(terrain.triangleCount() == 96U * 80U * 2U);

    // The corners of the grid and of the maze are grid points.
    const glm::vec3 first = terrain.gridPoint(0, 0);
    CHECK(first.x == -14.0F);
    CHECK(first.z == -14.0F);
    const glm::vec3 mazeCorner = terrain.gridPoint(28, 28);
    CHECK(mazeCorner.x == 0.0F);
    CHECK(mazeCorner.z == 0.0F);
    // So is every cell centre: the centre of cell (0, 0) is 2 steps further.
    const glm::vec3 cellCentre = terrain.gridPoint(30, 30);
    CHECK(cellCentre.x == game::cellCenter(0, 0).x);
    CHECK(cellCentre.z == game::cellCenter(0, 0).z);

    CHECK_THROWS_AS(terrain.gridPoint(97, 0), std::out_of_range);
    CHECK_THROWS_AS(terrain.gridPoint(0, -1), std::out_of_range);
    CHECK_THROWS_AS(terrain.gridNormal(-1, 0), std::out_of_range);
    CHECK_THROWS_AS(game::Terrain(0, 5, roughHeightmap(), 1.0F), std::invalid_argument);
    CHECK_THROWS_AS(game::Terrain(5, 0, roughHeightmap(), 1.0F), std::invalid_argument);
}

TEST_CASE("a flat heightmap gives a flat terrain") {
    SUBCASE("all zeros: flat everywhere, also on the hills") {
        const game::Terrain terrain(4, 4, constantHeightmap(0.0F), 1.0F);
        CHECK(terrain.minHeight() == 0.0F);
        CHECK(terrain.maxHeight() == 0.0F);
        CHECK(terrain.heightAt(3.3F, 1.7F) == 0.0F);
        CHECK(terrain.heightAt(-10.0F, 20.0F) == 0.0F);
    }

    SUBCASE("one value everywhere: flat inside the maze, and the land rises around it") {
        const game::Terrain terrain(4, 4, constantHeightmap(1.0F), 1.0F);
        // Inside the maze (0 to 8 m) every point has the height MAZE_RELIEF.
        for (float z = 0.0F; z <= 8.0F; z += 0.7F) {
            for (float x = 0.0F; x <= 8.0F; x += 0.7F) {
                CHECK(terrain.heightAt(x, z) == doctest::Approx(game::MAZE_RELIEF));
            }
        }
        // The outer edge of the land is at the height HILL_RELIEF.
        CHECK(terrain.heightAt(-14.0F, 4.0F) == doctest::Approx(game::HILL_RELIEF));
        CHECK(terrain.maxHeight() == doctest::Approx(game::HILL_RELIEF));
        CHECK(terrain.minHeight() == doctest::Approx(game::MAZE_RELIEF));
    }
}

TEST_CASE("the height scale multiplies every height, and 0 gives a flat world") {
    const game::Heightmap heightmap = roughHeightmap();
    const game::Terrain flat(5, 5, heightmap, 0.0F);
    const game::Terrain normal(5, 5, heightmap, 1.0F);
    const game::Terrain doubled(5, 5, heightmap, 2.0F);

    CHECK(flat.minHeight() == 0.0F);
    CHECK(flat.maxHeight() == 0.0F);
    // The rough heightmap is not flat.
    CHECK(normal.maxHeight() > 1.0F);

    for (int row = 0; row < normal.rows(); row += 5) {
        for (int column = 0; column < normal.columns(); column += 5) {
            CHECK(flat.gridPoint(column, row).y == 0.0F);
            CHECK(doubled.gridPoint(column, row).y ==
                  doctest::Approx(2.0F * normal.gridPoint(column, row).y));
        }
    }
}

TEST_CASE("the height of a grid point follows the formula of the terrain") {
    const game::Heightmap heightmap = roughHeightmap();
    constexpr int MAZE_WIDTH = 6;
    constexpr int MAZE_HEIGHT = 4;
    constexpr float HEIGHT_SCALE = 1.5F;
    const game::Terrain terrain(MAZE_WIDTH, MAZE_HEIGHT, heightmap, HEIGHT_SCALE);

    for (int row = 0; row < terrain.rows(); row += 3) {
        for (int column = 0; column < terrain.columns(); column += 3) {
            const glm::vec3 point = terrain.gridPoint(column, row);
            const float sample =
                heightmap.sample(point.x / game::HEIGHTMAP_SPAN, point.z / game::HEIGHTMAP_SPAN);
            const float relief = game::terrainRelief(
                game::distanceOutsideMaze(point.x, point.z, MAZE_WIDTH, MAZE_HEIGHT));
            CHECK(point.y == doctest::Approx(HEIGHT_SCALE * relief * sample));
        }
    }

    // The lowest and the highest point are what the grid says they are.
    float lowest = terrain.gridPoint(0, 0).y;
    float highest = lowest;
    for (int row = 0; row < terrain.rows(); ++row) {
        for (int column = 0; column < terrain.columns(); ++column) {
            lowest = std::min(lowest, terrain.gridPoint(column, row).y);
            highest = std::max(highest, terrain.gridPoint(column, row).y);
        }
    }
    CHECK(terrain.minHeight() == lowest);
    CHECK(terrain.maxHeight() == highest);
}

TEST_CASE("the same heightmap, size and scale give the same terrain") {
    const game::Terrain first(7, 5, roughHeightmap(), 1.25F);
    const game::Terrain second(7, 5, roughHeightmap(), 1.25F);

    REQUIRE(first.columns() == second.columns());
    REQUIRE(first.rows() == second.rows());
    bool same = true;
    for (int row = 0; row < first.rows(); ++row) {
        for (int column = 0; column < first.columns(); ++column) {
            same = same && first.gridPoint(column, row) == second.gridPoint(column, row);
        }
    }
    CHECK(same);
    CHECK(first.heightAt(3.21F, 7.77F) == second.heightAt(3.21F, 7.77F));
}

TEST_CASE("heightAt returns the height of a grid point at that grid point") {
    const game::Terrain terrain(3, 3, roughHeightmap(), 1.0F);
    for (int row = 0; row < terrain.rows(); ++row) {
        for (int column = 0; column < terrain.columns(); ++column) {
            const glm::vec3 point = terrain.gridPoint(column, row);
            CHECK(terrain.heightAt(point.x, point.z) ==
                  doctest::Approx(point.y).epsilon(HEIGHT_TOLERANCE));
        }
    }
}

TEST_CASE("heightAt is a straight line along the edges and along the diagonal of a square") {
    const game::Terrain terrain(3, 3, roughHeightmap(), 1.0F);

    // A check for every tenth grid square.
    for (int row = 0; row < terrain.rows() - 1; row += 3) {
        for (int column = 0; column < terrain.columns() - 1; column += 3) {
            const glm::vec3 northWest = terrain.gridPoint(column, row);
            const glm::vec3 northEast = terrain.gridPoint(column + 1, row);
            const glm::vec3 southWest = terrain.gridPoint(column, row + 1);
            const glm::vec3 southEast = terrain.gridPoint(column + 1, row + 1);

            // The middle of a line between two points, in all three coordinates.
            const auto middleOf = [](const glm::vec3& a, const glm::vec3& b) {
                return (a + b) / 2.0F;
            };
            // The height of the surface above the same place.
            const auto surfaceAt = [&terrain](const glm::vec3& point) {
                return terrain.heightAt(point.x, point.z);
            };

            // The four edges: each belongs to one triangle and is straight.
            for (const glm::vec3& middle :
                 {middleOf(northWest, northEast), middleOf(northWest, southWest),
                  middleOf(southWest, southEast), middleOf(northEast, southEast)}) {
                CHECK(surfaceAt(middle) == doctest::Approx(middle.y).epsilon(HEIGHT_TOLERANCE));
            }

            // The diagonal convention: north-west to south-east. The middle of the
            // square lies on it, so its height is the average of THOSE two corners. The
            // other two corners have no say there, which is what tells the triangles
            // from a bilinear blend of all four.
            const glm::vec3 centre = middleOf(northWest, southEast);
            CHECK(surfaceAt(centre) == doctest::Approx(centre.y).epsilon(HEIGHT_TOLERANCE));

            // The middles of the two triangles: the average of their three corners.
            const glm::vec3 northEastMiddle = (northWest + southEast + northEast) / 3.0F;
            const glm::vec3 southWestMiddle = (northWest + southWest + southEast) / 3.0F;
            CHECK(surfaceAt(northEastMiddle) ==
                  doctest::Approx(northEastMiddle.y).epsilon(HEIGHT_TOLERANCE));
            CHECK(surfaceAt(southWestMiddle) ==
                  doctest::Approx(southWestMiddle.y).epsilon(HEIGHT_TOLERANCE));
        }
    }
}

TEST_CASE("heightAt outside the grid is the height of the nearest border point") {
    const game::Terrain terrain(3, 3, roughHeightmap(), 1.0F);
    const float west = terrain.minX();
    const float east = terrain.maxX();
    const float north = terrain.minZ();

    CHECK(terrain.heightAt(west - 100.0F, 2.3F) == terrain.heightAt(west, 2.3F));
    CHECK(terrain.heightAt(east + 0.01F, 2.3F) == terrain.heightAt(east, 2.3F));
    CHECK(terrain.heightAt(1.7F, north - 3.0F) == terrain.heightAt(1.7F, north));
    // Past a corner: the corner itself.
    CHECK(terrain.heightAt(east + 5.0F, north - 5.0F) ==
          terrain.gridPoint(terrain.columns() - 1, 0).y);
}

TEST_CASE("the mesh has one vertex per grid point and two triangles per square") {
    const game::Terrain terrain(2, 3, roughHeightmap(), 1.0F);
    const game::TerrainMeshData mesh = game::buildTerrainMesh(terrain);

    const auto columns = static_cast<std::size_t>(terrain.columns());
    const auto rows = static_cast<std::size_t>(terrain.rows());
    CHECK(mesh.vertices.size() == columns * rows);
    CHECK(mesh.indices.size() == terrain.triangleCount() * 3U);

    // The vertices lie row after row, like the grid points.
    CHECK(mesh.vertices[0].position == terrain.gridPoint(0, 0));
    CHECK(mesh.vertices[columns + 2].position == terrain.gridPoint(2, 1));
    CHECK(mesh.vertices.back().position ==
          terrain.gridPoint(terrain.columns() - 1, terrain.rows() - 1));

    // Every index points at a vertex.
    const std::uint32_t largest = *std::ranges::max_element(mesh.indices);
    CHECK(largest == mesh.vertices.size() - 1);
}

TEST_CASE("every triangle of the mesh faces up, and its texture is not mirrored") {
    const game::Terrain terrain(2, 2, roughHeightmap(), 2.0F);
    const game::TerrainMeshData mesh = game::buildTerrainMesh(terrain);

    // Counter-clockwise seen from above: the cross product of two edges points up.
    bool allFaceUp = true;
    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const glm::vec3& a = mesh.vertices[mesh.indices[i]].position;
        const glm::vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const glm::vec3& c = mesh.vertices[mesh.indices[i + 2]].position;
        allFaceUp = allFaceUp && glm::cross(b - a, c - a).y > 0.0F;
    }
    CHECK(allFaceUp);

    // The shader builds the bitangent as cross(N, T), which is right only for a texture
    // that is not mirrored (see assets::countMirroredTriangles).
    CHECK(assets::countMirroredTriangles(mesh.vertices, mesh.indices) == 0U);
}

TEST_CASE("the vertices of the mesh carry normals, texture coordinates and tangents") {
    const game::Terrain terrain(2, 2, roughHeightmap(), 2.0F);
    const game::TerrainMeshData mesh = game::buildTerrainMesh(terrain);

    for (std::size_t i = 0; i < mesh.vertices.size(); i += 7) {
        const gfx::Vertex& vertex = mesh.vertices[i];
        CAPTURE(i);

        // A normal of length 1 that points up, more or less.
        CHECK(glm::length(vertex.normal) == doctest::Approx(1.0F));
        CHECK(vertex.normal.y > 0.0F);

        // The texture repeats every GROUND_TEXTURE_SPAN metres: u along +X, v along -Z.
        CHECK(vertex.uv.x == doctest::Approx(vertex.position.x / game::GROUND_TEXTURE_SPAN));
        CHECK(vertex.uv.y == doctest::Approx(-vertex.position.z / game::GROUND_TEXTURE_SPAN));

        // A tangent of length 1, perpendicular to the normal, pointing the way u grows:
        // towards +X.
        CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
        CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));
        CHECK(vertex.tangent.x > 0.0F);
    }
}

TEST_CASE("gridNormal leans away from rising ground") {
    SUBCASE("flat ground: straight up") {
        const game::Terrain terrain(2, 2, constantHeightmap(0.0F), 1.0F);
        CHECK(terrain.gridNormal(40, 40) == glm::vec3{0.0F, 1.0F, 0.0F});
        // On the border of the grid too.
        CHECK(terrain.gridNormal(0, 0) == glm::vec3{0.0F, 1.0F, 0.0F});
    }

    SUBCASE("ground that rises towards +X: the normal leans towards -X") {
        // A maze of 12 cells is 24 m wide: half of one repeat of the heightmap, which
        // rises from its west edge to its middle. So the ground rises all the way from
        // x = 0 to x = 24.
        const game::Terrain terrain(12, 2, eastwardSlope(), 1.0F);
        // The grid point in the middle of the maze.
        const int column = (game::TERRAIN_MARGIN_CELLS + 6) * game::TERRAIN_STEPS_PER_CELL;
        const int row = (game::TERRAIN_MARGIN_CELLS + 1) * game::TERRAIN_STEPS_PER_CELL;
        const glm::vec3 normal = terrain.gridNormal(column, row);

        CHECK(terrain.gridPoint(column + 1, row).y > terrain.gridPoint(column - 1, row).y);
        CHECK(normal.x < 0.0F);
        CHECK(normal.y > 0.0F);
        // Nothing changes from north to south.
        CHECK(normal.z == doctest::Approx(0.0F));
        CHECK(glm::length(normal) == doctest::Approx(1.0F));

        // The slope inside the maze: MAZE_RELIEF over half a repeat of the heightmap.
        const float slope = game::MAZE_RELIEF / (game::HEIGHTMAP_SPAN / 2.0F);
        const glm::vec3 expected = glm::normalize(glm::vec3{-slope, 1.0F, 0.0F});
        CHECK(normal.x == doctest::Approx(expected.x));
        CHECK(normal.y == doctest::Approx(expected.y));
    }
}

TEST_CASE("heightAt agrees with the triangle of the mesh at random points") {
    // The smallest maze keeps the search through all triangles short.
    const game::Terrain terrain(1, 1, roughHeightmap(), 2.0F);
    const game::TerrainMeshData mesh = game::buildTerrainMesh(terrain);

    constexpr std::uint32_t SEED = 2024;
    constexpr int POINT_COUNT = 300;
    std::mt19937 generator(SEED);

    const float width = terrain.maxX() - terrain.minX();
    const float depth = terrain.maxZ() - terrain.minZ();
    for (int i = 0; i < POINT_COUNT; ++i) {
        // Two numbers per point, always drawn in this order.
        const float x = terrain.minX() + width * randomUnit(generator);
        const float z = terrain.minZ() + depth * randomUnit(generator);
        CAPTURE(x);
        CAPTURE(z);

        float meshHeight = 0.0F;
        REQUIRE(meshHeightAt(mesh, x, z, meshHeight));
        // An absolute tolerance: the heights go down to 0, where a relative one
        // would allow nothing.
        CHECK(std::abs(terrain.heightAt(x, z) - meshHeight) < 0.0005F);
    }
}

TEST_CASE("lowestHeightUnder is never above the surface over its rectangle") {
    const game::Terrain terrain(4, 4, roughHeightmap(), 2.0F);

    constexpr std::uint32_t SEED = 77;
    constexpr int RECTANGLE_COUNT = 40;
    constexpr int SAMPLES_PER_SIDE = 6;
    std::mt19937 generator(SEED);

    for (int i = 0; i < RECTANGLE_COUNT; ++i) {
        // A rectangle somewhere in the maze, up to 2 m by 2 m.
        const float minX = 6.0F * randomUnit(generator);
        const float minZ = 6.0F * randomUnit(generator);
        const float maxX = minX + 2.0F * randomUnit(generator);
        const float maxZ = minZ + 2.0F * randomUnit(generator);
        const float lowest = terrain.lowestHeightUnder(minX, minZ, maxX, maxZ);

        bool neverAbove = true;
        for (int row = 0; row <= SAMPLES_PER_SIDE; ++row) {
            for (int column = 0; column <= SAMPLES_PER_SIDE; ++column) {
                const float x = glm::mix(minX, maxX, partOf(column, SAMPLES_PER_SIDE));
                const float z = glm::mix(minZ, maxZ, partOf(row, SAMPLES_PER_SIDE));
                neverAbove = neverAbove && lowest <= terrain.heightAt(x, z) + HEIGHT_TOLERANCE;
            }
        }
        CHECK(neverAbove);
        // And it is a height of the terrain, not just any small number.
        CHECK(lowest >= terrain.minHeight());
    }

    // A rectangle that is one grid point: the height of that point.
    const glm::vec3 point = terrain.gridPoint(40, 40);
    CHECK(terrain.lowestHeightUnder(point.x, point.z, point.x, point.z) == point.y);
}

TEST_CASE("the heightmap of the game loads and gives gentle ground inside the default maze") {
    assets::Image image;
    std::string error;
    REQUIRE(assets::loadImage(assetsDirectory() / "textures" / "heightmap.png", image, error,
                              assets::RowOrder::TopFirst));
    CHECK(image.width == 256);
    CHECK(image.height == 256);
    CHECK(image.channels == 3);

    const game::Heightmap heightmap = game::heightmapFromImage(image);
    REQUIRE(heightmap.values.size() == 65536U);
    // The picture uses the whole range: its darkest pixel is black, its brightest white.
    const auto [darkest, brightest] = std::ranges::minmax_element(heightmap.values);
    CHECK(*darkest == 0.0F);
    CHECK(*brightest == 1.0F);

    // The default maze at the default height scale. The lowest and the highest ground
    // inside the maze are 0.3 to 0.5 m apart: enough to see and to feel, and nothing
    // that looks like a hill between the walls.
    const game::Terrain terrain(game::DEFAULT_MAZE_WIDTH, game::DEFAULT_MAZE_HEIGHT, heightmap,
                                game::DEFAULT_HEIGHT_SCALE);
    const float mazeWidth = static_cast<float>(game::DEFAULT_MAZE_WIDTH) * game::CELL_SIZE;
    const float mazeDepth = static_cast<float>(game::DEFAULT_MAZE_HEIGHT) * game::CELL_SIZE;
    float lowest = terrain.heightAt(0.0F, 0.0F);
    float highest = lowest;
    for (float z = 0.0F; z <= mazeDepth; z += game::TERRAIN_SPACING) {
        for (float x = 0.0F; x <= mazeWidth; x += game::TERRAIN_SPACING) {
            lowest = std::min(lowest, terrain.heightAt(x, z));
            highest = std::max(highest, terrain.heightAt(x, z));
        }
    }
    CHECK(highest - lowest >= 0.3F);
    CHECK(highest - lowest <= 0.5F);

    // Around the maze the land rises into hills of a few metres.
    CHECK(terrain.maxHeight() > 2.0F);
    CHECK(terrain.maxHeight() <= game::HILL_RELIEF);
    CHECK(terrain.minHeight() >= 0.0F);
}

// ---- the maze on the terrain ------------------------------------------------------------

TEST_CASE("a maze world without a heightmap stands on flat ground") {
    const game::MazeWorld world = game::buildMazeWorld(5, 4, 3U);

    CHECK(world.terrain.minHeight() == 0.0F);
    CHECK(world.terrain.maxHeight() == 0.0F);
    // The terrain has the size of this maze all the same.
    CHECK(world.terrain.columns() == (5 + 14) * 4 + 1);
    CHECK(world.terrain.rows() == (4 + 14) * 4 + 1);

    for (const game::WallSegment& wall : world.walls) {
        CHECK(wall.position.y == 0.0F);
    }
    for (const glm::vec3& pillar : world.pillars) {
        CHECK(pillar.y == 0.0F);
    }
    CHECK(world.startPosition.y == 0.0F);
    CHECK(world.exitPosition.y == 0.0F);
}

TEST_CASE("on uneven ground the walls, pillars and the gate are sunk until no gap shows") {
    constexpr int WIDTH = 6;
    constexpr int HEIGHT = 5;
    constexpr std::uint32_t SEED = 4;
    const game::Heightmap heightmap = roughHeightmap();
    const game::MazeWorld flat = game::buildMazeWorld(WIDTH, HEIGHT, SEED);
    const game::MazeWorld world =
        game::buildMazeWorld(WIDTH, HEIGHT, SEED, heightmap, game::MAX_HEIGHT_SCALE);
    const game::Terrain& terrain = world.terrain;

    // The ground under the maze really is uneven.
    REQUIRE(terrain.maxHeight() > 1.0F);

    // The base of a box is never above the ground anywhere under its footprint. The
    // footprint is checked at a grid of points, a little past the box on every side
    // (the models are wider than their boxes at the base).
    const auto standsInTheGround = [&terrain](const scene::Aabb& box) {
        constexpr int SAMPLES_PER_SIDE = 8;
        bool noGap = true;
        for (int row = 0; row <= SAMPLES_PER_SIDE; ++row) {
            for (int column = 0; column <= SAMPLES_PER_SIDE; ++column) {
                const float x =
                    glm::mix(box.min.x - game::FOOTPRINT_MARGIN, box.max.x + game::FOOTPRINT_MARGIN,
                             partOf(column, SAMPLES_PER_SIDE));
                const float z =
                    glm::mix(box.min.z - game::FOOTPRINT_MARGIN, box.max.z + game::FOOTPRINT_MARGIN,
                             partOf(row, SAMPLES_PER_SIDE));
                noGap = noGap && box.min.y <= terrain.heightAt(x, z) + HEIGHT_TOLERANCE;
            }
        }
        return noGap;
    };

    SUBCASE("nothing moved sideways") {
        REQUIRE(world.walls.size() == flat.walls.size());
        for (std::size_t i = 0; i < world.walls.size(); ++i) {
            CHECK(world.walls[i].position.x == flat.walls[i].position.x);
            CHECK(world.walls[i].position.z == flat.walls[i].position.z);
            CHECK(world.walls[i].axis == flat.walls[i].axis);
        }
        REQUIRE(world.pillars.size() == flat.pillars.size());
        for (std::size_t i = 0; i < world.pillars.size(); ++i) {
            CHECK(world.pillars[i].x == flat.pillars[i].x);
            CHECK(world.pillars[i].z == flat.pillars[i].z);
        }
        // So the collision boxes are the same ones seen from above.
        REQUIRE(world.colliders.size() == flat.colliders.size());
        for (std::size_t i = 0; i < world.colliders.size(); ++i) {
            CHECK(world.colliders[i].min.x == flat.colliders[i].min.x);
            CHECK(world.colliders[i].max.x == flat.colliders[i].max.x);
            CHECK(world.colliders[i].min.z == flat.colliders[i].min.z);
            CHECK(world.colliders[i].max.z == flat.colliders[i].max.z);
        }
        CHECK(world.exitCell == flat.exitCell);
        CHECK(world.startYawDegrees == flat.startYawDegrees);
        REQUIRE(world.crystals.size() == flat.crystals.size());
    }

    SUBCASE("every wall and pillar has its base in the ground") {
        for (const game::WallSegment& wall : world.walls) {
            CHECK(standsInTheGround(game::wallBox(wall)));
            // Sunk, but not buried: its base is a height the terrain has.
            CHECK(wall.position.y >= terrain.minHeight());
        }
        for (const glm::vec3& pillar : world.pillars) {
            CHECK(standsInTheGround(game::pillarBox(pillar)));
        }
    }

    SUBCASE("the boxes and the matrices follow the lowered positions") {
        const std::vector<scene::Aabb> expected = game::colliderBoxes(world.walls, world.pillars);
        REQUIRE(world.colliders.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(world.colliders[i].min == expected[i].min);
            CHECK(world.colliders[i].max == expected[i].max);
        }
        // A wall box is still WALL_HEIGHT high, from its lowered base.
        CHECK(world.colliders[0].min.y == world.walls[0].position.y);
        CHECK(world.colliders[0].max.y ==
              doctest::Approx(world.walls[0].position.y + game::WALL_HEIGHT));

        REQUIRE(world.wallMatrices.size() == world.walls.size());
        for (std::size_t i = 0; i < world.walls.size(); ++i) {
            // The last column of a model matrix is where the origin of the model goes.
            CHECK(glm::vec3{world.wallMatrices[i][3]} == world.walls[i].position);
        }
        REQUIRE(world.pillarMatrices.size() == world.pillars.size());
        for (std::size_t i = 0; i < world.pillars.size(); ++i) {
            CHECK(glm::vec3{world.pillarMatrices[i][3]} == world.pillars[i]);
        }
    }

    SUBCASE("the gate is lowered like a wall, with its box") {
        REQUIRE(world.hasGate);
        CHECK(standsInTheGround(world.gateBox));
        CHECK(world.gateBox.min.y == world.gate.position.y);
        CHECK(world.gateBox.max.y == doctest::Approx(world.gate.position.y + game::WALL_HEIGHT));
        CHECK(world.gate.position.x == flat.gate.position.x);
        CHECK(world.gate.position.z == flat.gate.position.z);
    }

    SUBCASE("the start, the exit and the exit zone stand on the ground") {
        const glm::vec3 start = game::cellCenter(0, 0);
        CHECK(world.startPosition.x == start.x);
        CHECK(world.startPosition.z == start.z);
        CHECK(world.startPosition.y == terrain.heightAt(start.x, start.z));

        const float exitGround = game::groundHeightAt(world, world.exitCell);
        CHECK(world.exitPosition.y == exitGround);
        CHECK(world.exitZone.min.y == doctest::Approx(exitGround));
        CHECK(world.exitZone.max.y == doctest::Approx(exitGround + game::WALL_HEIGHT));
        CHECK(world.exitZone.min.x == flat.exitZone.min.x);
        CHECK(world.exitZone.max.z == flat.exitZone.max.z);
    }

    SUBCASE("a player next to a wall always overlaps its box in height") {
        // Collisions are tested in three dimensions: a wall stops the player only while
        // the two boxes share some height. The player stands on the ground beside the
        // wall and the wall on the lowest ground under it, so their feet differ. Checked
        // at points around every wall box, at the distance of a body that touches it.
        constexpr float BESIDE = game::Player::BODY_WIDTH / 2.0F;
        constexpr int SAMPLES_PER_SIDE = 4;
        bool alwaysOverlaps = true;
        for (const scene::Aabb& box : world.colliders) {
            for (int row = 0; row <= SAMPLES_PER_SIDE; ++row) {
                for (int column = 0; column <= SAMPLES_PER_SIDE; ++column) {
                    const float x = glm::mix(box.min.x - BESIDE, box.max.x + BESIDE,
                                             partOf(column, SAMPLES_PER_SIDE));
                    const float z = glm::mix(box.min.z - BESIDE, box.max.z + BESIDE,
                                             partOf(row, SAMPLES_PER_SIDE));
                    const float feet = terrain.heightAt(x, z);
                    const float head = feet + game::Player::BODY_HEIGHT;
                    // More than the contact tolerance of moveAndSlide in common.
                    const float shared = std::min(head, box.max.y) - std::max(feet, box.min.y);
                    alwaysOverlaps = alwaysOverlaps && shared > 0.1F;
                }
            }
        }
        CHECK(alwaysOverlaps);
    }
}

TEST_CASE(
    "placeOnTerrain can be called again, and a height scale of 0 brings the flat world back") {
    const game::Heightmap heightmap = roughHeightmap();
    const game::MazeWorld flat = game::buildMazeWorld(5, 5, 8U);
    game::MazeWorld world = game::buildMazeWorld(5, 5, 8U, heightmap, 2.0F);
    REQUIRE(world.terrain.maxHeight() > 1.0F);

    game::placeOnTerrain(world, heightmap, 0.0F);

    CHECK(world.terrain.maxHeight() == 0.0F);
    REQUIRE(world.walls.size() == flat.walls.size());
    for (std::size_t i = 0; i < world.walls.size(); ++i) {
        CHECK(world.walls[i].position == flat.walls[i].position);
    }
    REQUIRE(world.colliders.size() == flat.colliders.size());
    for (std::size_t i = 0; i < world.colliders.size(); ++i) {
        CHECK(world.colliders[i].min == flat.colliders[i].min);
        CHECK(world.colliders[i].max == flat.colliders[i].max);
    }
    // The lists were replaced, not added to.
    CHECK(world.wallMatrices.size() == flat.wallMatrices.size());
    CHECK(world.pillarMatrices.size() == flat.pillarMatrices.size());
    CHECK(world.startPosition == flat.startPosition);
    CHECK(world.exitPosition == flat.exitPosition);
    CHECK(world.exitZone.min == flat.exitZone.min);
    CHECK(world.gateBox.min == flat.gateBox.min);
    CHECK(world.gateBox.max == flat.gateBox.max);
}

TEST_CASE("the crystals of a round float above the ground of their cells") {
    const game::Heightmap heightmap = roughHeightmap();
    game::MazeWorld world = game::buildMazeWorld(6, 6, 2U, heightmap, 2.0F);
    const game::GameplaySettings settings;
    game::Round round = game::startRound(world, settings);
    REQUIRE(round.crystals.size() == world.crystals.size());
    REQUIRE(round.crystals.size() >= 2U);

    for (std::size_t i = 0; i < round.crystals.size(); ++i) {
        const game::MazeCell cell = world.crystals[i].cell;
        const glm::vec3 centre = game::cellCenter(cell.x, cell.z);
        const float ground = world.terrain.heightAt(centre.x, centre.z);
        CHECK(round.crystals[i].restPosition.x == centre.x);
        CHECK(round.crystals[i].restPosition.z == centre.z);
        CHECK(round.crystals[i].restPosition.y ==
              doctest::Approx(ground + game::CRYSTAL_FLOAT_HEIGHT));
    }

    SUBCASE("restCrystalsOnGround moves them to new ground and keeps what is collected") {
        round.crystals[0].collected = true;
        round.collectedCount = 1;
        const float heightBefore = round.crystals[1].restPosition.y;

        game::placeOnTerrain(world, heightmap, 0.0F);
        game::restCrystalsOnGround(round, world);

        CHECK(round.crystals[0].collected);
        CHECK_FALSE(round.crystals[1].collected);
        CHECK(round.collectedCount == 1);
        for (const game::RoundCrystal& crystal : round.crystals) {
            CHECK(crystal.restPosition.y == doctest::Approx(game::CRYSTAL_FLOAT_HEIGHT));
        }
        // The ground under that crystal was not at 0 before.
        CHECK(heightBefore > game::CRYSTAL_FLOAT_HEIGHT);
    }
}

// ---- the player on the terrain ----------------------------------------------------------

TEST_CASE("a walking player keeps the feet on the ground, uphill and downhill") {
    // Ground that rises towards +X inside a maze of 12 by 2 cells (see the test of
    // gridNormal). No walls: only the terrain is tested here.
    const game::Terrain terrain(12, 2, eastwardSlope(), game::MAX_HEIGHT_SCALE);
    constexpr float YAW_EAST = 90.0F;
    constexpr float YAW_WEST = 270.0F;
    constexpr int STEPS = 240; // two seconds

    game::Player player;
    player.position = {4.0F, 0.0F, 2.0F};
    const game::PlayerInput forward{.forward = true};

    // One step with no key held puts the feet on the ground where the player stands.
    player.update({}, YAW_EAST, 0.0F, STEP_SECONDS, {}, terrain);
    CHECK(player.position.x == 4.0F);
    CHECK(player.position.y == terrain.heightAt(4.0F, 2.0F));
    const float startHeight = player.position.y;

    // Uphill. The feet are on the surface after every step.
    bool onTheGround = true;
    for (int i = 0; i < STEPS; ++i) {
        player.update(forward, YAW_EAST, 0.0F, STEP_SECONDS, {}, terrain);
        onTheGround = onTheGround &&
                      player.position.y == terrain.heightAt(player.position.x, player.position.z);
    }
    CHECK(onTheGround);
    CHECK(player.position.y > startHeight);
    // The speed over the ground does not depend on the slope: 3 m/s for 2 s.
    CHECK(player.position.x == doctest::Approx(4.0F + 2.0F * game::Player::WALK_SPEED));
    CHECK(player.position.z == doctest::Approx(2.0F));
    // The eyes and the box go up with the feet.
    CHECK(player.eyePosition().y == doctest::Approx(player.position.y + game::Player::EYE_HEIGHT));
    CHECK(player.box().min.y == doctest::Approx(player.position.y));

    // And back down to where it started.
    for (int i = 0; i < STEPS; ++i) {
        player.update(forward, YAW_WEST, 0.0F, STEP_SECONDS, {}, terrain);
    }
    CHECK(player.position.x == doctest::Approx(4.0F));
    CHECK(player.position.y == doctest::Approx(startHeight));
}

TEST_CASE("the height of the feet changes a little in every step, never in a jump") {
    // What the interpolation between two fixed steps relies on: from one step to the
    // next the ground under a walking player changes by a small amount, so blending the
    // two heights is a smooth ramp.
    const game::MazeWorld world =
        game::buildMazeWorld(6, 6, 5U, roughHeightmap(), game::MAX_HEIGHT_SCALE);
    game::Player player;
    player.position = world.startPosition;

    // The largest height difference between two neighbouring grid points inside the
    // maze, where the player walks (the hills around it are much steeper). The grid
    // points of the maze start TERRAIN_MARGIN_CELLS cells into the grid.
    const game::Terrain& terrain = world.terrain;
    const int firstPoint = game::TERRAIN_MARGIN_CELLS * game::TERRAIN_STEPS_PER_CELL;
    const int lastColumn = firstPoint + world.maze.width() * game::TERRAIN_STEPS_PER_CELL;
    const int lastRow = firstPoint + world.maze.height() * game::TERRAIN_STEPS_PER_CELL;
    float steepest = 0.0F;
    for (int row = firstPoint; row < lastRow; ++row) {
        for (int column = firstPoint; column < lastColumn; ++column) {
            const float here = terrain.gridPoint(column, row).y;
            steepest = std::max(steepest, std::abs(terrain.gridPoint(column + 1, row).y - here));
            steepest = std::max(steepest, std::abs(terrain.gridPoint(column, row + 1).y - here));
        }
    }
    // A step along the diagonal of a square can climb both slopes at once.
    const float largestChange =
        2.0F * (steepest / terrain.spacing()) * game::Player::SPRINT_SPEED * STEP_SECONDS;

    const game::PlayerInput sprint{.forward = true, .right = true, .sprint = true};
    float previous = player.position.y;
    bool smooth = true;
    for (int i = 0; i < 600; ++i) {
        player.update(sprint, 135.0F, 0.0F, STEP_SECONDS, world.colliders, terrain);
        smooth = smooth && std::abs(player.position.y - previous) <= largestChange;
        previous = player.position.y;
    }
    CHECK(smooth);
    CHECK(largestChange < 0.2F);
}

TEST_CASE("the walls stop the player on uneven ground exactly as on flat ground") {
    // The same keys in the same maze, once on flat ground and once on rough ground at
    // the largest height scale. The terrain only moves the player up and down, so the
    // two walks must agree in x and z at every step.
    constexpr int WIDTH = 6;
    constexpr int HEIGHT = 6;
    constexpr std::uint32_t SEED = 11;
    const game::MazeWorld flat = game::buildMazeWorld(WIDTH, HEIGHT, SEED);
    const game::MazeWorld rough =
        game::buildMazeWorld(WIDTH, HEIGHT, SEED, roughHeightmap(), game::MAX_HEIGHT_SCALE);

    game::Player flatPlayer;
    flatPlayer.position = flat.startPosition;
    game::Player roughPlayer;
    roughPlayer.position = rough.startPosition;

    constexpr std::uint32_t WALK_SEED = 5;
    constexpr int TURN_COUNT = 60;
    constexpr int STEPS_PER_TURN = 60;
    constexpr std::uint32_t FULL_TURN_DEGREES = 360;
    std::mt19937 generator(WALK_SEED);
    const auto coin = [&generator] { return game::randomBelow(generator, 2U) == 1U; };

    bool samePath = true;
    bool onTheGround = true;
    float farthest = 0.0F;
    for (int turn = 0; turn < TURN_COUNT; ++turn) {
        // One field per line: the order of the calls to coin() is then plain to see.
        game::PlayerInput input;
        input.forward = coin();
        input.backward = coin();
        input.left = coin();
        input.right = coin();
        input.sprint = coin();
        const auto yaw = static_cast<float>(game::randomBelow(generator, FULL_TURN_DEGREES));

        for (int i = 0; i < STEPS_PER_TURN; ++i) {
            flatPlayer.update(input, yaw, 0.0F, STEP_SECONDS, flat.colliders, flat.terrain);
            roughPlayer.update(input, yaw, 0.0F, STEP_SECONDS, rough.colliders, rough.terrain);

            samePath = samePath && flatPlayer.position.x == roughPlayer.position.x &&
                       flatPlayer.position.z == roughPlayer.position.z;
            onTheGround = onTheGround &&
                          roughPlayer.position.y == rough.terrain.heightAt(roughPlayer.position.x,
                                                                           roughPlayer.position.z);
        }
        farthest =
            std::max(farthest, glm::distance(flatPlayer.position, glm::vec3{flat.startPosition}));
    }
    CHECK(samePath);
    CHECK(onTheGround);
    // The walk did go somewhere and did run into walls on the way.
    CHECK(farthest > game::CELL_SIZE);
}
