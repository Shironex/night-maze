// Land: the land around the maze. Its ground beyond the terrain (the ring), the field
// walls and hedgerows, the trees, houses and landmarks that stand on it, and where their
// windows light up.
#pragma once

#include "game/Maze.hpp"
#include "game/Terrain.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace scene {
struct Frustum;
} // namespace scene

namespace game {

struct MazeWorld;

// Plain data and math without OpenGL, like the rest of the game_logic library. The drawing
// is game::LandRenderer, the triangles of what is built in code are in game/LandMeshes.hpp.
//
// The land lies in bands, measured from the outer face of the border wall
// (game::distanceOutsideMaze):
//   - the near band, LAND_NEAR_START to TERRAIN_MARGIN: the hills of the terrain, with
//     field walls, a sheepfold, hedgerows, trees, a farmhouse, a barn, a shed and a rick,
//   - the middle band, out to LAND_REACH: the ring, ground of its own that rises to a crest
//     on the three sides away from the village and falls into a valley with a stream on the
//     village side, with bare trees, copses, cottages and the landmarks,
//   - the far band: the ridge and the village (game/Village.hpp), with a mill and a church
//     tower on other bearings, drawn around the eye like the village.
//
// Everything is placed from the seed of the maze by a generator of its own, after
// everything in the maze, so no wall, crystal or sheep of a seed moves for it.

/// Where the near band starts: no piece of the land comes closer to the border wall, so the
/// strip where the grass of the outer walls grows stays free.
constexpr float LAND_NEAR_START = 1.5F;

/// How far out the land has pieces, and how far out its ground goes. The ground ends below
/// y = 0 (LAND_RING_END_DEPTH), in fog so thick that its edge is never seen.
constexpr float LAND_REACH = 40.0F;
constexpr float LAND_RING_END = 46.0F;
constexpr float LAND_RING_END_DEPTH = -3.0F;

/// The distance between two grid points of the ring, four steps of the terrain. The sizes of
/// the terrain are whole multiples of it, so the grid of the ring meets the edge of the
/// terrain at its grid points.
constexpr float LAND_RING_SPACING = 2.0F;

/// Past the edge of the terrain the ground of the ring leaves the hills over this many
/// metres and follows the shape of its side.
constexpr float LAND_RING_BLEND = 8.0F;

/// The crests of the three sides away from the village: how far out (from the border wall)
/// and how high they are, each a range the crest wanders in along its side. The heights are
/// at a height scale of 1, like the hills of the terrain.
constexpr float LAND_CREST_NEAREST = 30.0F;
constexpr float LAND_CREST_FARTHEST = 36.0F;
constexpr float LAND_CREST_LOWEST = 8.0F;
constexpr float LAND_CREST_HIGHEST = 12.0F;

/// The valley on the village side: its floor lies from LAND_VALLEY_START to LAND_VALLEY_END
/// out, at LAND_VALLEY_FLOOR, below the base of the fog, so the mist pools in it. The stream
/// runs through it, wandering LAND_STREAM_WANDER either side of the middle. Beyond it the
/// far bank rises to LAND_FAR_BANK_HEIGHT at LAND_REACH.
constexpr float LAND_VALLEY_START = 22.0F;
constexpr float LAND_VALLEY_END = 30.0F;
constexpr float LAND_VALLEY_FLOOR = 0.15F;
constexpr float LAND_STREAM_WANDER = 1.5F;
constexpr float LAND_STREAM_WIDTH = 1.2F;
constexpr float LAND_FAR_BANK_HEIGHT = 3.5F;

/// The fields of the near band: a grid of lines LAND_FIELD_SMALLEST to LAND_FIELD_LARGEST
/// apart, turned by up to LAND_FIELD_TURN_DEGREES. Every stretch of a line between two
/// crossings is a drystone wall, a hedgerow or open (LAND_FIELD_OPEN_SHARE).
constexpr float LAND_FIELD_SMALLEST = 12.0F;
constexpr float LAND_FIELD_LARGEST = 20.0F;
constexpr float LAND_FIELD_TURN_DEGREES = 6.0F;
constexpr float LAND_FIELD_HEDGE_SHARE = 0.35F;
constexpr float LAND_FIELD_OPEN_SHARE = 0.2F;

/// A field wall: drystone, LAND_WALL_HEIGHT high, LAND_WALL_BASE wide at its foot and
/// LAND_WALL_TOP at its top, sunk LAND_WALL_SINK into the ground like the walls of the maze.
/// It wears the stone of the maze (wall_stone.png).
constexpr float LAND_WALL_HEIGHT = 1.2F;
constexpr float LAND_WALL_BASE = 0.6F;
constexpr float LAND_WALL_TOP = 0.4F;
constexpr float LAND_WALL_SINK = 0.15F;

/// The tallest piece of the near band stands at most this high over its foot: an oak at
/// the largest scale. The box of the moon shadow reaches this high over the terrain
/// (game::shadowCasterBounds).
constexpr float LAND_NEAR_TALLEST = 9.0F;

/// The round sheepfold: the same wall in a ring of this radius, with a gap to walk in.
constexpr float LAND_FOLD_RADIUS = 3.0F;
constexpr float LAND_FOLD_GAP = 1.4F;

/// A hedgerow: a lumpy strip LAND_HEDGE_WIDTH wide and LAND_HEDGE_HEIGHT high, its lumps
/// up to LAND_HEDGE_LUMP higher or lower.
constexpr float LAND_HEDGE_WIDTH = 1.1F;
constexpr float LAND_HEDGE_HEIGHT = 1.6F;
constexpr float LAND_HEDGE_LUMP = 0.25F;

/// A piece stands this far from the line of a field wall or a hedgerow at least, beyond its
/// own radius.
constexpr float LAND_LINE_CLEARANCE = 0.6F;

/// The reveal camera looks across the land on the village side (game::villageRevealPose).
/// There no piece may rise more than LAND_REVEAL_CLEAR_DEGREES over the camera within
/// LAND_REVEAL_CONE_DEGREES either side of the bearing to the village: the village above the
/// valley must stay clear.
constexpr float LAND_REVEAL_CLEAR_DEGREES = 5.0F;
constexpr float LAND_REVEAL_CONE_DEGREES = 8.0F;

/// The models of the land. Each is a file of tools/blender/build_land_pieces.py
/// (landModelFile). The near ones stand in the near band at full detail, the far
/// ones in the middle band, the landmarks on the crests.
enum class LandModel {
    Oak,
    Hawthorn,
    DeadOak,
    Farmhouse,
    Barn,
    Woodshed,
    Rick,
    DeadOakFar,
    Copse,
    FarmhouseFar,
    Cottage,
    StandingStone,
    Dovecote,
    ChapelGable,
    WatchHut,
    Cairn,
};
constexpr int LAND_MODEL_COUNT = 16;

/// The five landmarks in the order of LandModel, from StandingStone on. A maze gets three:
/// one on each crest.
constexpr int LAND_LANDMARK_KINDS = 5;
constexpr int LAND_LANDMARKS_PER_MAZE = 3;
constexpr LandModel FIRST_LANDMARK_MODEL = LandModel::StandingStone;

/// What placement needs to know about a model: how far it reaches from its middle on the
/// ground (the radius of a circle around it) and how high it is, both at a scale of 1, in
/// metres, and whether its windows can light up. The heights must agree with PIECES in
/// tools/blender/build_land_pieces.py (a test reads the models).
struct LandModelShape {
    float radius = 1.0F;
    float height = 1.0F;
    bool windows = false;
};

/// The shape of a model.
const LandModelShape& landModelShape(LandModel model);

/// True for the models of the near band, which stand on the terrain and cast moon shadows.
bool isNearLandModel(LandModel model);

/// The name of the model file of a model in assets/models, without ".obj". A model with
/// windows has them in "<name>_windows.txt", where <name> is the file name without the
/// "_far" of the far versions (landWindowsFile): a far version has the windows of its near
/// one, because both are the same shape in the same space.
std::string_view landModelFile(LandModel model);
std::string landWindowsFile(LandModel model);

/// One piece of the land: a model, standing with its origin at position, turned about the
/// vertical axis and scaled.
struct LandPiece {
    LandModel model = LandModel::Oak;
    glm::vec3 position{0.0F};
    float yawDegrees = 0.0F;
    float scale = 1.0F;
};

/// The model matrix of a piece.
glm::mat4 landPieceMatrix(const LandPiece& piece);

/// The height of the top of a piece, in the world.
float landPieceTop(const LandPiece& piece);

/// What stands on a line of the field grid between two crossings.
enum class FieldLineKind { Wall, Hedge };

/// One stretch of a field line, on the ground plan: x and z in the world.
struct FieldLine {
    glm::vec2 from{0.0F};
    glm::vec2 to{0.0F};
    FieldLineKind kind = FieldLineKind::Wall;
};

/// The shape of the ground of the ring, chosen once for a maze: everything
/// landRingHeight needs besides the terrain.
struct LandShape {
    int mazeWidth = 1;
    int mazeHeight = 1;
    /// The side of the village (game::villageSide): the side with the valley.
    Direction villageSide = Direction::North;
    float heightScale = DEFAULT_HEIGHT_SCALE;
    /// The phases of the waves that move the crest and the stream along each side, in
    /// radians: two per side, in the order of Direction.
    std::array<float, 2 * DIRECTION_COUNT> phases{};
};

/// The land around one maze. Built by placeLand, only read afterwards.
struct Land {
    LandShape shape;

    /// The grid of the ring, LAND_RING_SPACING apart, from (ringMinX, ringMinZ), with
    /// ringColumns * ringRows heights row after row. It covers the terrain too: there the
    /// heights are the ones of the terrain, and nothing of the ring is drawn.
    float ringMinX = 0.0F;
    float ringMinZ = 0.0F;
    int ringColumns = 0;
    int ringRows = 0;
    std::vector<float> ringHeights;

    /// The pieces: the near band first (nearCount of them), then the middle band, then the
    /// landmarks (the last landmarkCount).
    std::vector<LandPiece> pieces;
    std::size_t nearCount = 0;
    std::size_t landmarkCount = 0;

    /// The field walls and the hedgerows of the near band.
    std::vector<FieldLine> fieldLines;

    /// The middle of the sheepfold, and the yaw its gap faces (degrees, like LandPiece).
    /// Empty when no field had room for it.
    std::optional<glm::vec2> foldCentre;
    float foldGapYawDegrees = 0.0F;

    /// The stream: a line through the valley, x and z in the world.
    std::vector<glm::vec2> stream;
};

/// The height of the ground of the land at (x, z). On the terrain it is the terrain. Past
/// its edge it leaves the hills over LAND_RING_BLEND and follows its side: the crest, or on
/// the village side the valley and the far bank, and from LAND_REACH on it falls to
/// LAND_RING_END_DEPTH at LAND_RING_END. Corners blend the two sides that meet there. The
/// heights are multiplied by the height scale, the fall below 0 is not.
float landRingHeight(const LandShape& shape, const Terrain& terrain, float x, float z);

/// The height of the grid of the ring under (x, z), blended in its squares like the
/// terrain: exactly what is drawn there, up to the shape of the triangles.
float landGroundHeight(const Land& land, float x, float z);

/// The middle line of the stream at a place along the village side: its distance from the
/// border wall. along is the coordinate along that side, in metres.
float landStreamDistance(const LandShape& shape, float along);

/// Builds the land of a world that stands on its terrain (placeOnTerrain): the ring, then
/// the field grid with its walls, hedgerows and the fold, then the pieces of the near band,
/// the middle band and the landmarks, in that order, each from the next numbers of one
/// generator seeded with the seed of the maze. A candidate is rejected when it reaches
/// within LAND_NEAR_START of the border wall or past LAND_REACH, stands on a field line, the
/// fold or another piece, or on the village side rises over the line the reveal camera keeps
/// clear (landKeepsRevealClear).
Land placeLand(const MazeWorld& world, float heightScale);

/// Whether a piece whose top is at top keeps the view of the reveal camera clear: false
/// when it lies within LAND_REVEAL_CONE_DEGREES of the bearing to the village, as seen from
/// camera, and rises more than LAND_REVEAL_CLEAR_DEGREES above it.
bool landKeepsRevealClear(const glm::vec3& camera, Direction villageSide, const glm::vec3& top);

/// The landmarks of a land, in its order: the model of each and the point at its top.
struct LandmarkView {
    LandModel model = LandModel::StandingStone;
    glm::vec3 top{0.0F};
};
std::vector<LandmarkView> landmarks(const Land& land);

/// Whether a landmark counts as seen in this frame: its top lies in the frustum of the
/// camera, and the straight line from the eye to it crosses none of the boxes (the walls of
/// the round).
bool landmarkInSight(const glm::vec3& eye, const glm::vec3& top, const scene::Frustum& frustum,
                     std::span<const scene::Aabb> walls);

/// Where a window of a house can light up: its middle, the way it faces (length 1) and its
/// size, in metres, in the space of the model (assets/models/land_<name>_windows.txt).
struct LandWindow {
    glm::vec3 middle{0.0F};
    glm::vec3 normal{0.0F, 0.0F, 1.0F};
    float width = 1.0F;
    float height = 1.0F;
};

/// Reads a windows file: one window per line, eight numbers (the middle, the normal, the
/// width and the height). Empty lines and lines that start with '#' are skipped, and so is
/// a line that does not hold eight numbers.
std::vector<LandWindow> parseLandWindows(std::string_view text);

/// The share of the windows of the land that each night of the campaign lights, nights 1 to
/// 4: night 5 lights none, its one new light is the window of the player in the village.
constexpr std::array<float, 4> LAND_WINDOW_SHARES{0.20F, 0.25F, 0.25F, 0.30F};
constexpr int LAND_LIT_NIGHTS = 4;

/// Deals windowCount windows to the nights 1 to 4: the windows are shuffled by the seed and
/// cut in the shares of LAND_WINDOW_SHARES. Entry n of the result holds the numbers of the
/// windows of night n + 1. Every window is in exactly one night.
std::array<std::vector<std::size_t>, LAND_LIT_NIGHTS> dealLandWindows(std::size_t windowCount,
                                                                      std::uint32_t seed);

// The far band: the mill and the church tower stand on the ridge, drawn around the eye like
// the village (game::villageMatrix), on bearings of their own beside it.

/// The two silhouettes of the far band.
enum class FarSilhouette { Mill, ChurchTower };
constexpr int FAR_SILHOUETTE_COUNT = 2;

/// The night that lights each of them: the mill with night 2, the tower with night 4.
int farSilhouetteNight(FarSilhouette silhouette);

/// The model a silhouette is drawn with. Both are stand-ins until a mill and a church tower
/// of their own are generated: the far farmhouse and the chapel gable, scaled up.
LandModel farSilhouetteModel(FarSilhouette silhouette);

/// The model matrix of a silhouette, for a view matrix without its translation (like the
/// village): it stands FAR_SILHOUETTE_DISTANCE out, on a bearing of its own beside the
/// bearing of the village, its foot behind the rim of the ridge, and is placed like the
/// village (game::villageMatrix, with the same elevationDegrees), so nothing of it lies
/// past VILLAGE_REACH.
glm::mat4 farSilhouetteMatrix(FarSilhouette silhouette, Direction villageSide, float farPlane,
                              float elevationDegrees);

/// How far out the silhouettes stand in the space of the village model: a few metres
/// behind the rim, which lies about 255 m out.
constexpr float FAR_SILHOUETTE_DISTANCE = 262.0F;

} // namespace game
