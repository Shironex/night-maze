// LandMeshes: the triangles of what the land builds in code: the ground of the ring, the
// field walls and the sheepfold, the hedgerows, the stream and the lit windows.
#pragma once

#include "game/Land.hpp"
#include "game/Terrain.hpp"

#include <span>
#include <vector>

namespace game {

// Plain data, like the terrain mesh (game::buildTerrainMesh): positions in world space,
// normals, texture coordinates and tangents, ready for gfx::Mesh. game::LandRenderer
// uploads them.

/// The ground of the ring: every square of the grid of the ring outside the terrain, and
/// one row of squares under its edge, sunk there by LAND_RING_TUCK. The tucked row lies
/// under the terrain, so the two meshes meet without a crack, though their edges have
/// different numbers of points. The texture coordinates are the ones of the terrain.
TerrainMeshData buildLandRingMesh(const Land& land, const Terrain& terrain);

/// How far the row of the ring under the edge of the terrain is sunk below it.
constexpr float LAND_RING_TUCK = 0.6F;

/// The field walls and the sheepfold: a drystone wall along every wall line and around the
/// fold, following the ground of the terrain. Its section is a trapezoid
/// (LAND_WALL_BASE wide at its foot, LAND_WALL_TOP at its top) and its top wanders a few
/// centimetres. It wears the picture of the walls of the maze, whose height is WALL_HEIGHT.
TerrainMeshData buildFieldWallMesh(const Land& land, const Terrain& terrain);

/// The hedgerows: a lumpy strip along every hedge line, LAND_HEDGE_WIDTH wide and
/// LAND_HEDGE_HEIGHT high, rounded across, its lumps from the seed of the maze. It wears
/// the picture of the side of a hedge (land_hedge.png): u along the strip, one repeat
/// every HEDGE_TEXTURE_SPAN metres, v from the foot (0) to the top (1) on both sides.
TerrainMeshData buildHedgerowMesh(const Land& land, const Terrain& terrain, std::uint32_t seed);

/// How many metres of hedge one repeat of its picture covers: twice its height, because
/// the picture is twice as wide as high.
constexpr float HEDGE_TEXTURE_SPAN = 2.0F * LAND_HEDGE_HEIGHT;

/// The stream: a strip LAND_STREAM_WIDTH wide along the stream line, a few centimetres
/// over the ring, only where the ground lies in the valley (below LAND_STREAM_TOP).
TerrainMeshData buildStreamMesh(const Land& land);
constexpr float LAND_STREAM_TOP = 1.0F;

/// One window that is lit: the window of a model and the model matrix of its piece.
struct PlacedWindow {
    LandWindow window;
    glm::mat4 matrix{1.0F};
};

/// A quad for every window, moved into the world by its matrix and a little in front of
/// the wall it sits in, facing out. The texture coordinates run from 0 to 1 across it.
TerrainMeshData buildWindowMesh(std::span<const PlacedWindow> windows);

/// How far a window quad stands in front of the point the build found on the wall.
constexpr float LAND_WINDOW_LIFT = 0.03F;

} // namespace game
