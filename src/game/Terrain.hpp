// Terrain: the ground of the game as a grid of heights read from a heightmap.
#pragma once

#include "assets/ImageLoader.hpp"
#include "game/MazeLayout.hpp"
#include "gfx/Vertex.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can build a terrain and ask it for heights without a window.

// All sizes are in metres.

/// Grid steps per maze cell, along X and along Z: with 4 the grid points are 0.5 m apart.
/// A whole number on purpose: every cell border and every cell centre is then a grid
/// point.
constexpr int TERRAIN_STEPS_PER_CELL = 4;

/// Distance between two neighbouring grid points.
constexpr float TERRAIN_SPACING = CELL_SIZE / static_cast<float>(TERRAIN_STEPS_PER_CELL);

/// The land around the maze, as a number of cells on every side, and the same in metres.
/// Counted in cells so that the grid of the land continues the grid of the maze.
constexpr int TERRAIN_MARGIN_CELLS = 7;
constexpr float TERRAIN_MARGIN = static_cast<float>(TERRAIN_MARGIN_CELLS) * CELL_SIZE;

/// One repeat of the heightmap covers a square of this size. The picture is laid on the
/// ground again and again, like a tiling texture, so the hills keep their size whatever
/// the size of the maze. 48 m is the default maze (20 m) with its margin on both sides.
constexpr float HEIGHTMAP_SPAN = 48.0F;

/// Height of the brightest heightmap pixel above the darkest one inside the maze, at
/// a height scale of 1: gentle ground that is easy to walk on. The part of the heightmap
/// that lies under the default maze does not reach from black to white, so there the
/// lowest and the highest ground are about 0.4 m apart (a test in TerrainTests.cpp holds
/// that between 0.3 m and 0.5 m).
constexpr float MAZE_RELIEF = 0.6F;

/// The same difference at the outer edge of the land: hills. Between the border of the
/// maze and that edge the value grows smoothly from MAZE_RELIEF to this (terrainRelief).
constexpr float HILL_RELIEF = 4.5F;

/// The height scale the game starts with, and the largest one its slider offers. The
/// limit keeps the ground inside the maze flatter than the player is tall: the collision
/// boxes of the walls then always reach the box of a player who stands next to them.
constexpr float DEFAULT_HEIGHT_SCALE = 1.0F;
constexpr float MAX_HEIGHT_SCALE = 2.5F;

/// One repeat of the ground texture covers a square of this size on the terrain.
constexpr float GROUND_TEXTURE_SPAN = 4.0F;

/// What can be changed about the terrain while the game runs. The debug UI edits the
/// fields. The values here are the defaults.
struct TerrainSettings {
    /// Every height of the terrain is multiplied by this number: 0 is a flat world, 1
    /// the terrain as designed, MAX_HEIGHT_SCALE the largest the game accepts. Changing
    /// it means building the terrain again: the panel sets rebuild.
    float heightScale = DEFAULT_HEIGHT_SCALE;

    /// True draws the terrain as the edges of its triangles instead of filled.
    bool wireframe = false;

    /// True when the height scale changed and the terrain has not been built again yet.
    /// The debug UI sets it, the application rebuilds at the start of the next frame and
    /// clears it, like MazeSettings::regenerate.
    bool rebuild = false;
};

/// A heightmap: a picture read as numbers. Each value is the height of the ground at one
/// place, from 0 (the lowest) to 1 (the highest). What that is in metres is decided by
/// the terrain.
struct Heightmap {
    /// Number of values in a row and number of rows. Both at least 1.
    int width = 1;
    int height = 1;

    /// width * height values, row after row. Row 0 is the north edge (-Z), and the first
    /// value of a row is its west end (-X). A new heightmap is one value of 0: flat ground.
    std::vector<float> values{0.0F};

    /// The height at a place between the values, from 0 to 1. u runs along a row and v
    /// from row to row. A step of 1 in u or v is one whole picture: value number i of
    /// a row lies at u = i / width, and past the last value the first one follows again
    /// (the picture repeats, so u and v may be any number).
    ///
    /// Bilinear lookup: the four values around the place are blended by how close the
    /// place is to each of them.
    float sample(float u, float v) const;
};

/// Makes a heightmap from a picture: the first channel of every pixel (red, or the grey
/// value) divided by 255. The picture must be loaded top row first
/// (assets::RowOrder::TopFirst), so that its top edge is north. A picture without pixels
/// gives the flat heightmap.
Heightmap heightmapFromImage(const assets::Image& image);

/// How far a point lies outside the maze, in metres: 0 inside the maze and on its border.
/// The maze covers x from 0 to mazeWidth * CELL_SIZE and z from 0 to mazeHeight *
/// CELL_SIZE.
float distanceOutsideMaze(float x, float z, int mazeWidth, int mazeHeight);

/// The height of the brightest heightmap pixel above the darkest one, at a height scale
/// of 1 and distanceOutside metres away from the maze:
///
///     relief = MAZE_RELIEF + (HILL_RELIEF - MAZE_RELIEF) * smoothstep(distance / TERRAIN_MARGIN)
///
/// smoothstep(t) = 3t^2 - 2t^3 for t from 0 to 1: it starts and ends flat, so the gentle
/// ground of the maze turns into hills without a crease. Past the margin the value stays
/// HILL_RELIEF.
float terrainRelief(float distanceOutside);

/// The ground: a regular grid of heights that covers the maze and TERRAIN_MARGIN of land
/// around it. It is built once per maze and only read afterwards.
///
/// The height of a grid point at (x, z) is
///
///     height = heightScale * terrainRelief(distanceOutsideMaze(x, z)) * heightmap(x, z)
///
/// where heightmap(x, z) is Heightmap::sample(x / HEIGHTMAP_SPAN, z / HEIGHTMAP_SPAN), a
/// number from 0 to 1. So the lowest ground is at y = 0, and a height scale of 0 gives
/// a flat world.
///
/// Between the grid points the surface is made of triangles, two per grid square. THE
/// DIAGONAL CONVENTION, used by heightAt and by buildTerrainMesh alike: a square is cut
/// along the diagonal from its north-west corner (smallest x and z) to its south-east
/// corner (largest x and z). The two triangles are the north-east one (north-west,
/// south-east, north-east corner) and the south-west one (north-west, south-west,
/// south-east corner).
class Terrain {
public:
    /// Flat ground at y = 0, everywhere: the terrain of code and tests that need none.
    Terrain();

    /// The terrain of a maze of mazeWidth by mazeHeight cells.
    /// Throws std::invalid_argument when a size is below 1.
    Terrain(int mazeWidth, int mazeHeight, const Heightmap& heightmap, float heightScale);

    /// Number of grid points along X and along Z.
    int columns() const { return m_columns; }
    int rows() const { return m_rows; }

    /// Distance between two neighbouring grid points.
    float spacing() const { return m_spacing; }

    /// The world x of grid column 0 and the world z of grid row 0: the north-west corner
    /// of the land.
    float minX() const { return m_minX; }
    float minZ() const { return m_minZ; }

    /// The world x of the last grid column and the world z of the last grid row.
    float maxX() const;
    float maxZ() const;

    /// The grid point in column `column` and row `row`, in world space.
    /// Throws std::out_of_range when the point is not on the grid.
    glm::vec3 gridPoint(int column, int row) const;

    /// The direction the surface faces at a grid point, with length 1. Central
    /// differences: the slope along X is the height difference between the neighbour to
    /// the east and the neighbour to the west, divided by their distance, and the same
    /// along Z. On the border of the grid the missing neighbour is replaced by the point
    /// itself. The normal of a surface y = h(x, z) is (-slope along X, 1, -slope along
    /// Z), brought to length 1.
    /// Throws std::out_of_range when the point is not on the grid.
    glm::vec3 gridNormal(int column, int row) const;

    /// The height of the surface at (x, z): exactly what is drawn there. The point lies
    /// in one triangle of one grid square (see the diagonal convention above), and the
    /// height is read off the flat plane of that triangle. A plain bilinear blend of the
    /// four corners would give a slightly different number in the middle of a square,
    /// and things placed with it would float or sink a little.
    ///
    /// A point outside the grid gets the height of the nearest point of its border.
    float heightAt(float x, float z) const;

    /// The lowest height the surface has anywhere above the rectangle from (minX, minZ)
    /// to (maxX, maxZ), or a little less: the lowest corner of every grid square the
    /// rectangle touches. A flat triangle is never lower than its lowest corner, so
    /// nothing that stands on this height has a gap under it.
    float lowestHeightUnder(float minX, float minZ, float maxX, float maxZ) const;

    /// The lowest and the highest grid point.
    float minHeight() const { return m_minHeight; }
    float maxHeight() const { return m_maxHeight; }

    /// Number of triangles of the surface: two per grid square.
    std::size_t triangleCount() const;

private:
    /// Position of a grid point in m_heights. Throws std::out_of_range for a point that
    /// is not on the grid.
    std::size_t heightIndex(int column, int row) const;

    /// The height of a grid point. The point is moved onto the grid first: a column or
    /// row outside takes the nearest one inside.
    float clampedHeight(int column, int row) const;

    int m_columns;
    int m_rows;
    float m_spacing;
    float m_minX;
    float m_minZ;

    // One height per grid point, row after row: the point (column, row) is at index
    // row * m_columns + column.
    std::vector<float> m_heights;

    float m_minHeight = 0.0F;
    float m_maxHeight = 0.0F;
};

/// The terrain as triangles, ready for gfx::Mesh: plain data, so tests can check it.
struct TerrainMeshData {
    /// One vertex per grid point, row after row like the heights: position in world
    /// space, normal (Terrain::gridNormal), texture coordinate and tangent.
    std::vector<gfx::Vertex> vertices;

    /// Three indices per triangle, two triangles per grid square, counter-clockwise when
    /// seen from above.
    std::vector<std::uint32_t> indices;
};

/// Builds the vertices and the indices of a terrain from its grid.
///
/// The squares are cut along the diagonal Terrain::heightAt uses, so the height the
/// game computes for a point is the height of the triangle drawn there. The texture
/// coordinate is the position in metres divided by GROUND_TEXTURE_SPAN: u grows towards
/// +X and v towards -Z, so the ground texture is seen from above the right way round.
/// The tangents come from assets::computeTangents, like the tangents of the models.
TerrainMeshData buildTerrainMesh(const Terrain& terrain);

} // namespace game
