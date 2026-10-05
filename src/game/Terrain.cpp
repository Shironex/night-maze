// Terrain: the ground of the game as a grid of heights read from a heightmap.
// See docs/modules/renderer/terrain.md
#include "game/Terrain.hpp"

#include "assets/Tangents.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace game {

namespace {

// The largest value of one colour channel of a picture: 8 bits.
constexpr float MAX_CHANNEL_VALUE = 255.0F;

// The flat terrain is one grid square of this size. Its size does not matter: a point
// outside the grid gets the height of the border, and all of it is at y = 0.
constexpr int FLAT_GRID_POINTS = 2;
constexpr float FLAT_SPACING = 1.0F;

// The corners of a grid square, a triangle and a triangle pair, as counts.
constexpr std::size_t INDICES_PER_SQUARE = 6; // two triangles
constexpr std::size_t TRIANGLES_PER_SQUARE = 2;

// The curve 3t^2 - 2t^3 for t from 0 to 1: it starts and ends flat.
float smoothStep(float t) {
    return t * t * (3.0F - 2.0F * t);
}

// The part of a number after the decimal point, from 0 to 1, also for a negative number:
// -0.25 gives 0.75. This is what makes a picture repeat in both directions.
float fraction(float value) {
    return value - std::floor(value);
}

// How far a coordinate lies outside the range from 0 to size: 0 inside.
float distanceOutsideRange(float coordinate, float size) {
    return std::max({-coordinate, coordinate - size, 0.0F});
}

} // namespace

float Heightmap::sample(float u, float v) const {
    // Where the place lies on the grid of values: from 0 to width and from 0 to height.
    const float column = fraction(u) * static_cast<float>(width);
    const float row = fraction(v) * static_cast<float>(height);

    // The value to the west and north of the place. The min keeps the index inside the
    // picture when rounding lands exactly on width or height.
    const int column0 = std::min(static_cast<int>(column), width - 1);
    const int row0 = std::min(static_cast<int>(row), height - 1);
    // Its neighbours to the east and south. Past the last value comes the first again.
    const int column1 = (column0 + 1) % width;
    const int row1 = (row0 + 1) % height;

    // How far the place is from the first value towards the second, from 0 to 1.
    const float alongX = column - static_cast<float>(column0);
    const float alongZ = row - static_cast<float>(row0);

    const auto valueAt = [this](int valueColumn, int valueRow) {
        return values[static_cast<std::size_t>(valueRow) * static_cast<std::size_t>(width) +
                      static_cast<std::size_t>(valueColumn)];
    };

    // Blend along the row in the two rows, then between the rows.
    const float north = glm::mix(valueAt(column0, row0), valueAt(column1, row0), alongX);
    const float south = glm::mix(valueAt(column0, row1), valueAt(column1, row1), alongX);
    return glm::mix(north, south, alongZ);
}

Heightmap heightmapFromImage(const assets::Image& image) {
    const std::size_t pixelCount =
        static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height);
    const auto channels = static_cast<std::size_t>(image.channels);
    // No pixels, or fewer bytes than the sizes promise: flat ground instead of reading
    // past the end of the picture.
    if (image.width < 1 || image.height < 1 || image.channels < 1 ||
        image.pixels.size() < pixelCount * channels) {
        return {};
    }

    Heightmap heightmap;
    heightmap.width = image.width;
    heightmap.height = image.height;
    heightmap.values.resize(pixelCount);
    for (std::size_t pixel = 0; pixel < pixelCount; ++pixel) {
        // The first byte of a pixel: red, which in a grey picture is the grey value.
        heightmap.values[pixel] =
            static_cast<float>(image.pixels[pixel * channels]) / MAX_CHANNEL_VALUE;
    }
    return heightmap;
}

float distanceOutsideMaze(float x, float z, int mazeWidth, int mazeHeight) {
    const float outsideX = distanceOutsideRange(x, static_cast<float>(mazeWidth) * CELL_SIZE);
    const float outsideZ = distanceOutsideRange(z, static_cast<float>(mazeHeight) * CELL_SIZE);
    // Beside the maze only one of the two is above 0 and the length is that one. Past
    // a corner of the maze it is the straight distance to the corner.
    return glm::length(glm::vec2{outsideX, outsideZ});
}

float terrainRelief(float distanceOutside) {
    const float t = std::clamp(distanceOutside / TERRAIN_MARGIN, 0.0F, 1.0F);
    return MAZE_RELIEF + (HILL_RELIEF - MAZE_RELIEF) * smoothStep(t);
}

Terrain::Terrain()
    : m_columns(FLAT_GRID_POINTS),
      m_rows(FLAT_GRID_POINTS),
      m_spacing(FLAT_SPACING),
      m_minX(0.0F),
      m_minZ(0.0F),
      m_heights(static_cast<std::size_t>(FLAT_GRID_POINTS) * FLAT_GRID_POINTS, 0.0F) {}

Terrain::Terrain(int mazeWidth, int mazeHeight, const Heightmap& heightmap, float heightScale)
    : m_columns(0),
      m_rows(0),
      m_spacing(TERRAIN_SPACING),
      m_minX(-TERRAIN_MARGIN),
      m_minZ(-TERRAIN_MARGIN) {
    if (mazeWidth < 1 || mazeHeight < 1) {
        throw std::invalid_argument("Terrain: the maze must be at least 1 by 1 cells");
    }

    // The maze and the margin on both sides, in grid steps. A row of N steps has N + 1
    // points.
    m_columns = (mazeWidth + 2 * TERRAIN_MARGIN_CELLS) * TERRAIN_STEPS_PER_CELL + 1;
    m_rows = (mazeHeight + 2 * TERRAIN_MARGIN_CELLS) * TERRAIN_STEPS_PER_CELL + 1;

    m_heights.reserve(static_cast<std::size_t>(m_columns) * static_cast<std::size_t>(m_rows));
    for (int row = 0; row < m_rows; ++row) {
        for (int column = 0; column < m_columns; ++column) {
            const float x = m_minX + static_cast<float>(column) * m_spacing;
            const float z = m_minZ + static_cast<float>(row) * m_spacing;

            // The formula of the class comment: the picture gives a number from 0 to 1,
            // the relief says how many metres that number is worth at this distance from
            // the maze, and the height scale stretches or flattens everything.
            const float sample = heightmap.sample(x / HEIGHTMAP_SPAN, z / HEIGHTMAP_SPAN);
            const float relief = terrainRelief(distanceOutsideMaze(x, z, mazeWidth, mazeHeight));
            m_heights.push_back(heightScale * relief * sample);
        }
    }

    // minmax_element returns a pair of iterators: to the smallest and to the largest.
    const auto [lowest, highest] = std::minmax_element(m_heights.begin(), m_heights.end());
    m_minHeight = *lowest;
    m_maxHeight = *highest;
}

float Terrain::maxX() const {
    return m_minX + static_cast<float>(m_columns - 1) * m_spacing;
}

float Terrain::maxZ() const {
    return m_minZ + static_cast<float>(m_rows - 1) * m_spacing;
}

std::size_t Terrain::heightIndex(int column, int row) const {
    if (column < 0 || column >= m_columns || row < 0 || row >= m_rows) {
        throw std::out_of_range("Terrain: the grid point is outside the grid");
    }
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(m_columns) +
           static_cast<std::size_t>(column);
}

float Terrain::clampedHeight(int column, int row) const {
    return m_heights[heightIndex(std::clamp(column, 0, m_columns - 1),
                                 std::clamp(row, 0, m_rows - 1))];
}

glm::vec3 Terrain::gridPoint(int column, int row) const {
    const float height = m_heights[heightIndex(column, row)];
    return {m_minX + static_cast<float>(column) * m_spacing, height,
            m_minZ + static_cast<float>(row) * m_spacing};
}

glm::vec3 Terrain::gridNormal(int column, int row) const {
    // Checks the point: the neighbours below are clamped, the point itself must exist.
    heightIndex(column, row);

    // The neighbours on both sides, and how far apart they really are: two steps inside
    // the grid, one step on its border, where one neighbour is the point itself.
    const int west = std::max(column - 1, 0);
    const int east = std::min(column + 1, m_columns - 1);
    const int north = std::max(row - 1, 0);
    const int south = std::min(row + 1, m_rows - 1);

    const float slopeX = (clampedHeight(east, row) - clampedHeight(west, row)) /
                         (static_cast<float>(east - west) * m_spacing);
    const float slopeZ = (clampedHeight(column, south) - clampedHeight(column, north)) /
                         (static_cast<float>(south - north) * m_spacing);

    // Where the ground rises towards +X, the surface leans back towards -X.
    return glm::normalize(glm::vec3{-slopeX, 1.0F, -slopeZ});
}

float Terrain::heightAt(float x, float z) const {
    // The place on the grid, in grid steps from the north-west corner. A point outside
    // is moved to the border.
    const float gridX =
        std::clamp((x - m_minX) / m_spacing, 0.0F, static_cast<float>(m_columns - 1));
    const float gridZ = std::clamp((z - m_minZ) / m_spacing, 0.0F, static_cast<float>(m_rows - 1));

    // The grid square the place lies in, by its north-west corner. A place exactly on
    // the last column or row belongs to the last square, which ends there.
    const int column = std::min(static_cast<int>(gridX), m_columns - 2);
    const int row = std::min(static_cast<int>(gridZ), m_rows - 2);

    // Where the place lies inside the square: 0 to 1 from west to east and from north to
    // south.
    const float alongX = gridX - static_cast<float>(column);
    const float alongZ = gridZ - static_cast<float>(row);

    const float northWest = clampedHeight(column, row);
    const float northEast = clampedHeight(column + 1, row);
    const float southWest = clampedHeight(column, row + 1);
    const float southEast = clampedHeight(column + 1, row + 1);

    // The diagonal from north-west to south-east is the line alongX == alongZ. On each
    // side of it the surface is one flat triangle: start at the north-west corner and
    // walk along two of its edges.
    if (alongX >= alongZ) {
        // The north-east triangle: east along the north edge, then south along the east
        // edge.
        return northWest + alongX * (northEast - northWest) + alongZ * (southEast - northEast);
    }
    // The south-west triangle: south along the west edge, then east along the south edge.
    return northWest + alongZ * (southWest - northWest) + alongX * (southEast - southWest);
}

float Terrain::lowestHeightUnder(float minX, float minZ, float maxX, float maxZ) const {
    // The grid columns and rows that enclose the rectangle: the last one at or before
    // its start and the first one at or after its end.
    const int firstColumn = static_cast<int>(std::floor((minX - m_minX) / m_spacing));
    const int lastColumn = static_cast<int>(std::ceil((maxX - m_minX) / m_spacing));
    const int firstRow = static_cast<int>(std::floor((minZ - m_minZ) / m_spacing));
    const int lastRow = static_cast<int>(std::ceil((maxZ - m_minZ) / m_spacing));

    // clampedHeight handles a rectangle that reaches past the grid.
    float lowest = clampedHeight(firstColumn, firstRow);
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            lowest = std::min(lowest, clampedHeight(column, row));
        }
    }
    return lowest;
}

std::size_t Terrain::triangleCount() const {
    return static_cast<std::size_t>(m_columns - 1) * static_cast<std::size_t>(m_rows - 1) *
           TRIANGLES_PER_SQUARE;
}

TerrainMeshData buildTerrainMesh(const Terrain& terrain) {
    const int columns = terrain.columns();
    const int rows = terrain.rows();

    TerrainMeshData mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            gfx::Vertex vertex;
            vertex.position = terrain.gridPoint(column, row);
            vertex.normal = terrain.gridNormal(column, row);
            // The texture repeats every GROUND_TEXTURE_SPAN metres. v grows towards -Z:
            // seen from above with north at the top, u goes right and v goes up, like on
            // the picture itself.
            vertex.uv = {vertex.position.x / GROUND_TEXTURE_SPAN,
                         -vertex.position.z / GROUND_TEXTURE_SPAN};
            mesh.vertices.push_back(vertex);
        }
    }

    // The index of the vertex of a grid point: the vertices lie row after row.
    const auto vertexIndex = [columns](int column, int row) {
        return static_cast<std::uint32_t>(row * columns + column);
    };

    mesh.indices.reserve(static_cast<std::size_t>(columns - 1) *
                         static_cast<std::size_t>(rows - 1) * INDICES_PER_SQUARE);
    for (int row = 0; row < rows - 1; ++row) {
        for (int column = 0; column < columns - 1; ++column) {
            const std::uint32_t northWest = vertexIndex(column, row);
            const std::uint32_t northEast = vertexIndex(column + 1, row);
            const std::uint32_t southWest = vertexIndex(column, row + 1);
            const std::uint32_t southEast = vertexIndex(column + 1, row + 1);

            // The diagonal convention of Terrain: north-west to south-east. Both
            // triangles go counter-clockwise when seen from above, so their front faces
            // look up.
            mesh.indices.insert(mesh.indices.end(), {northWest, southEast, northEast});
            mesh.indices.insert(mesh.indices.end(), {northWest, southWest, southEast});
        }
    }

    // The tangent of every vertex: the direction on the surface in which u grows, which
    // the normal map of the ground needs.
    assets::computeTangents(mesh.vertices, mesh.indices);
    return mesh;
}

} // namespace game
