// Land: the land around the maze. Its ground beyond the terrain (the ring), the field
// walls and hedgerows, the trees, houses and landmarks that stand on it, and where their
// windows light up.
#include "game/Land.hpp"

#include "game/MazeGenerator.hpp"
#include "game/MazeWorld.hpp"
#include "game/Village.hpp"
#include "scene/Frustum.hpp"
#include "scene/Raycast.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <sstream>
#include <string>

namespace game {

namespace {

// The generator of the land is seeded with the seed of the maze plus this number, and the
// one that deals the windows to the nights with the next. No other generator adds them
// (the largest of the others is the 11000027 of the sheep).
constexpr std::uint32_t LAND_SEED_OFFSET = 12000017U;
constexpr std::uint32_t LAND_WINDOW_SEED_OFFSET = 13000027U;

// randomUnit cuts the range from 0 to 1 into this many equal steps, like the one of the
// grass: std::uniform_real_distribution is not the same on every standard library.
constexpr std::uint32_t RANDOM_STEPS = 4096U;

constexpr float FULL_TURN_DEGREES = 360.0F;
constexpr float QUARTER_TURN_DEGREES = 90.0F;

// The shapes of the models, in the order of LandModel. The radius is half of the wider
// side of the model on the ground (of a tree: half of its crown), the height the one
// tools/blender/build_land_pieces.py scales it to. The copse is built 9 m high, and the
// decimation takes the thin tips of its tallest twigs: it ends 8.4 m high.
constexpr std::array<LandModelShape, LAND_MODEL_COUNT> MODEL_SHAPES{{
    {.radius = 3.2F, .height = 9.8F},                  // Oak
    {.radius = 2.2F, .height = 5.9F},                  // Hawthorn
    {.radius = 2.6F, .height = 9.0F},                  // DeadOak
    {.radius = 4.5F, .height = 7.0F, .windows = true}, // Farmhouse
    {.radius = 4.2F, .height = 6.5F},                  // Barn
    {.radius = 2.5F, .height = 3.2F},                  // Woodshed
    {.radius = 3.1F, .height = 4.0F},                  // Rick
    {.radius = 2.6F, .height = 8.8F},                  // DeadOakFar
    {.radius = 4.5F, .height = 8.4F},                  // Copse
    {.radius = 5.0F, .height = 7.0F, .windows = true}, // FarmhouseFar
    {.radius = 3.4F, .height = 5.5F, .windows = true}, // Cottage
    {.radius = 3.8F, .height = 5.5F},                  // StandingStone
    {.radius = 3.4F, .height = 6.5F},                  // Dovecote
    {.radius = 3.3F, .height = 7.0F},                  // ChapelGable
    {.radius = 3.8F, .height = 5.0F},                  // WatchHut
    {.radius = 3.5F, .height = 5.0F},                  // Cairn
}};

// The ground under the near band rises from about this share of HILL_RELIEF at the edge
// of the terrain, where the ring takes over from the hills.
constexpr float RING_START_SHARE = 0.55F;

// The crest and the stream wander along their side in two waves each, so long (in metres)
// that a side of a default maze sees about one of each.
constexpr float LONG_WAVE = 37.0F;
constexpr float SHORT_WAVE = 17.0F;
constexpr float STREAM_WAVE = 23.0F;

// A piece sinks this far into the ground under it: the near ones like the walls of the
// maze, the far ones a little more, because the ring is coarser.
constexpr float NEAR_SINK = 0.15F;
constexpr float MIDDLE_SINK = 0.25F;

// How many places are tried for each piece before it is given up.
constexpr int TRIES_PER_PIECE = 40;

// The pieces of the bands, as one per this many metres of the middle line of the band,
// and never more than the most: a large maze has a longer band, and the land keeps to a
// budget of triangles (about 150,000 for everything it draws). The middle line of the
// near band is 7.75 m out, of the middle band 27 m.
constexpr float NEAR_TREE_SPACING = 10.0F;
constexpr float MIDDLE_TREE_SPACING = 18.0F;
constexpr float MIDDLE_COPSE_SPACING = 36.0F;
constexpr int MOST_NEAR_TREES = 17;
constexpr int MOST_MIDDLE_TREES = 12;
constexpr int MOST_COPSES = 7;
constexpr float NEAR_BAND_MIDDLE = 7.75F;
constexpr float MIDDLE_BAND_MIDDLE = 27.0F;

// A tree may lean over a wall: only this share of its crown has to keep off a field line.
constexpr float TREE_LINE_SHARE = 0.4F;
// A house, a barn or a shed: its circle reaches past the corners of its walls, which are
// squared to the field grid, so only this share of it has to keep off a field line.
constexpr float HOUSE_LINE_SHARE = 0.75F;
// Two pieces may stand this close, as a share of their two radii added: crowns and roofs
// can overlap a little.
constexpr float PIECE_SPACING_SHARE = 0.8F;

// The trees of the near band, as shares: oaks, then hawthorns, then dead oaks.
constexpr float OAK_SHARE = 0.3F;
constexpr float HAWTHORN_SHARE = 0.35F;
// Trees and ricks are scaled a little, so no two look the same.
constexpr float SMALLEST_TREE = 0.85F;
constexpr float LARGEST_TREE = 1.15F;

// The shed stands near the farmhouse: this far from its middle.
constexpr float SHED_NEAREST = 6.0F;
constexpr float SHED_FARTHEST = 9.0F;

// The cottages of the valley: how many stand there on a default maze, and how many on the
// other sides. The valley is where the reveal looks, so most of the lit windows are there.
constexpr int VALLEY_COTTAGES = 3;
// They stand on the slope down into the valley or on its far bank: the band they may
// stand in starts this far before the valley floor.
constexpr float VALLEY_COTTAGE_SLOPE = 6.0F;
constexpr int OTHER_COTTAGES = 2;
constexpr int FAR_FARMHOUSES = 2;
// A cottage keeps this far from the middle of the stream, beyond its radius.
constexpr float STREAM_CLEARANCE = 1.0F;

// The landmarks stand this far out, on their crest.
constexpr float LANDMARK_NEAREST = 25.0F;
constexpr float LANDMARK_FARTHEST = 35.0F;
// and away from the ends of their side, as a share of its length.
constexpr float LANDMARK_SIDE_MARGIN = 0.15F;

// The far band: the bearing of each silhouette beside the bearing of the village (degrees,
// clockwise seen from above like a yaw), the height of its foot over the eye in the space
// of the village model, and its scale. In the order of FarSilhouette. The rim of the ridge
// stands about 4.5 m over the eye there, so the feet are behind it; the mill is 22 m and
// the tower 31.5 m high, their tops about 5 and 7 degrees over the horizon, below the cap
// of the tower of the village (9 degrees).
struct SilhouettePlace {
    float bearingDegrees;
    float footHeight;
    float scale;
};
constexpr std::array<SilhouettePlace, FAR_SILHOUETTE_COUNT> SILHOUETTE_PLACES{{
    {.bearingDegrees = 50.0F, .footHeight = 2.0F, .scale = 3.15F},
    {.bearingDegrees = -35.0F, .footHeight = 2.0F, .scale = 4.5F},
}};

float randomUnit(std::mt19937& generator) {
    return static_cast<float>(randomBelow(generator, RANDOM_STEPS)) /
           static_cast<float>(RANDOM_STEPS);
}

float randomBetween(std::mt19937& generator, float low, float high) {
    return low + (high - low) * randomUnit(generator);
}

// The curve 3t^2 - 2t^3, with t brought into 0 to 1 first.
float smoothStep(float t) {
    const float clamped = std::clamp(t, 0.0F, 1.0F);
    return clamped * clamped * (3.0F - 2.0F * clamped);
}

// How far a coordinate lies outside the range from low to high, and on which side: below
// 0 before it, above 0 after it, 0 inside.
float outsideRange(float coordinate, float low, float high) {
    if (coordinate < low) {
        return coordinate - low;
    }
    if (coordinate > high) {
        return coordinate - high;
    }
    return 0.0F;
}

// A number from 0 to 1 that wanders smoothly along a side: two waves with phases of their
// own.
float wander(float along, float phase) {
    const float turn = glm::two_pi<float>();
    return 0.5F + 0.25F * std::sin(along * turn / LONG_WAVE + phase) +
           0.25F * std::sin(along * turn / SHORT_WAVE + 2.0F * phase);
}

// The height of the land on one side at distance metres from the border wall, at along on
// that side, before the blend with the hills of the terrain.
float sideHeight(const LandShape& shape, Direction side, float along, float distance) {
    const float hills = RING_START_SHARE * HILL_RELIEF * shape.heightScale;
    const float fallFrom = distance - LAND_REACH;
    if (side == shape.villageSide) {
        if (distance < LAND_VALLEY_START) {
            return glm::mix(
                hills, LAND_VALLEY_FLOOR,
                smoothStep((distance - TERRAIN_MARGIN) / (LAND_VALLEY_START - TERRAIN_MARGIN)));
        }
        if (distance < LAND_VALLEY_END) {
            return LAND_VALLEY_FLOOR;
        }
        if (fallFrom < 0.0F) {
            return glm::mix(
                LAND_VALLEY_FLOOR, LAND_FAR_BANK_HEIGHT * shape.heightScale,
                smoothStep((distance - LAND_VALLEY_END) / (LAND_REACH - LAND_VALLEY_END)));
        }
        return glm::mix(LAND_FAR_BANK_HEIGHT * shape.heightScale, LAND_RING_END_DEPTH,
                        smoothStep(fallFrom / (LAND_RING_END - LAND_REACH)));
    }
    const auto phase = [&shape, side](int wave) {
        return shape.phases.at(static_cast<std::size_t>(2 * static_cast<int>(side) + wave));
    };
    const float crestDistance =
        glm::mix(LAND_CREST_NEAREST, LAND_CREST_FARTHEST, wander(along, phase(0)));
    const float crestHeight =
        glm::mix(LAND_CREST_LOWEST, LAND_CREST_HIGHEST, wander(along, phase(1))) *
        shape.heightScale;
    if (distance < crestDistance) {
        return glm::mix(hills, crestHeight,
                        smoothStep((distance - TERRAIN_MARGIN) / (crestDistance - TERRAIN_MARGIN)));
    }
    return glm::mix(crestHeight, LAND_RING_END_DEPTH,
                    smoothStep((distance - crestDistance) / (LAND_RING_END - crestDistance)));
}

// The coordinate along a side of a point: x on the north and the south side, z on the
// east and the west side.
float alongSide(Direction side, float x, float z) {
    return side == Direction::North || side == Direction::South ? x : z;
}

// The height of the grid of the ring at one of its points.
float ringPoint(const Land& land, int column, int row) {
    const int clampedColumn = std::clamp(column, 0, land.ringColumns - 1);
    const int clampedRow = std::clamp(row, 0, land.ringRows - 1);
    return land.ringHeights[static_cast<std::size_t>(clampedRow) *
                                static_cast<std::size_t>(land.ringColumns) +
                            static_cast<std::size_t>(clampedColumn)];
}

// The distance on the ground plan from a point to a stretch of line.
float distanceToLine(const glm::vec2& point, const glm::vec2& from, const glm::vec2& to) {
    const glm::vec2 along = to - from;
    const float length2 = glm::dot(along, along);
    const float t =
        length2 > 0.0F ? std::clamp(glm::dot(point - from, along) / length2, 0.0F, 1.0F) : 0.0F;
    return glm::length(point - (from + along * t));
}

// Everything the placement of one land shares.
struct Placement {
    const MazeWorld& world;
    Land& land;
    std::mt19937& generator;
    glm::vec3 revealCamera{0.0F};
    // The landmarks, kept apart until the middle band is placed: they come last in the
    // list of pieces.
    std::vector<LandPiece> landmarks;
};

float distanceOutside(const Placement& placement, const glm::vec2& point) {
    return distanceOutsideMaze(point.x, point.y, placement.world.maze.width(),
                               placement.world.maze.height());
}

// The side of the maze a point outside it lies beyond: the one it is farthest past.
Direction sideOf(const Placement& placement, const glm::vec2& point) {
    const float width = static_cast<float>(placement.world.maze.width()) * CELL_SIZE;
    const float height = static_cast<float>(placement.world.maze.height()) * CELL_SIZE;
    const float pastX = outsideRange(point.x, 0.0F, width);
    const float pastZ = outsideRange(point.y, 0.0F, height);
    if (std::abs(pastX) > std::abs(pastZ)) {
        return pastX < 0.0F ? Direction::West : Direction::East;
    }
    return pastZ < 0.0F ? Direction::North : Direction::South;
}

// A random point of the ground plan within reach metres of the maze.
glm::vec2 randomPoint(Placement& placement, float reach) {
    const float width = static_cast<float>(placement.world.maze.width()) * CELL_SIZE;
    const float height = static_cast<float>(placement.world.maze.height()) * CELL_SIZE;
    const float x = randomBetween(placement.generator, -reach, width + reach);
    const float z = randomBetween(placement.generator, -reach, height + reach);
    return {x, z};
}

// The height a piece stands at: the lowest ground under a square of half its radius
// around it, sunk a little. The near band stands on the terrain, the middle band on the
// ring.
float standingHeight(const Placement& placement, const glm::vec2& point, float radius, bool near) {
    const float half = radius * 0.5F;
    if (near) {
        return placement.world.terrain.lowestHeightUnder(point.x - half, point.y - half,
                                                         point.x + half, point.y + half) -
               NEAR_SINK;
    }
    float lowest = landGroundHeight(placement.land, point.x, point.y);
    for (const glm::vec2 corner : {glm::vec2{-half, -half}, glm::vec2{half, -half},
                                   glm::vec2{-half, half}, glm::vec2{half, half}}) {
        lowest = std::min(lowest,
                          landGroundHeight(placement.land, point.x + corner.x, point.y + corner.y));
    }
    return lowest - MIDDLE_SINK;
}

// Whether a piece of this radius can stand at point: clear of the field lines, the fold,
// the pieces placed so far and the landmarks. lineShare is the part of the radius that has
// to keep off a field line.
bool isClear(const Placement& placement, const glm::vec2& point, float radius, float lineShare) {
    for (const FieldLine& line : placement.land.fieldLines) {
        if (distanceToLine(point, line.from, line.to) < radius * lineShare + LAND_LINE_CLEARANCE) {
            return false;
        }
    }
    if (placement.land.foldCentre &&
        glm::length(point - *placement.land.foldCentre) < LAND_FOLD_RADIUS + radius) {
        return false;
    }
    const auto overlaps = [&point, radius](const LandPiece& other) {
        const float otherRadius = landModelShape(other.model).radius * other.scale;
        return glm::length(point - glm::vec2{other.position.x, other.position.z}) <
               (radius + otherRadius) * PIECE_SPACING_SHARE;
    };
    return std::ranges::none_of(placement.land.pieces, overlaps) &&
           std::ranges::none_of(placement.landmarks, overlaps);
}

// What a wanted piece is and where it may stand.
struct Wanted {
    LandModel model = LandModel::Oak;
    float scale = 1.0F;
    // The band, measured from the border wall, its radius must stay inside of.
    float nearest = LAND_NEAR_START;
    float farthest = TERRAIN_MARGIN;
    // Only on this side, when given.
    std::optional<Direction> side;
    // Within this many metres of a point, when given (the shed by the farmhouse).
    std::optional<glm::vec2> near;
    float nearDistance = 0.0F;
    float lineShare = 1.0F;
    // A house is turned to the field grid, everything else any way.
    bool squareToFields = false;
};

// Tries to place a wanted piece. Returns it, or nothing when no place was found.
std::optional<LandPiece> tryPlace(Placement& placement, const Wanted& wanted, float fieldTurn) {
    const LandModelShape& shape = landModelShape(wanted.model);
    const float radius = shape.radius * wanted.scale;
    const bool nearBand = isNearLandModel(wanted.model);
    for (int attempt = 0; attempt < TRIES_PER_PIECE; ++attempt) {
        glm::vec2 point;
        if (wanted.near) {
            const float angle = randomUnit(placement.generator) * glm::two_pi<float>();
            point = *wanted.near +
                    glm::vec2{std::cos(angle), std::sin(angle)} *
                        randomBetween(placement.generator, SHED_NEAREST, wanted.nearDistance);
        } else {
            point = randomPoint(placement, wanted.farthest);
        }
        // Every try takes its numbers, used or not, so the order of the generator does not
        // depend on which test below fails.
        const float yaw =
            wanted.squareToFields
                ? fieldTurn + QUARTER_TURN_DEGREES *
                                  static_cast<float>(randomBelow(placement.generator, 4U))
                : randomUnit(placement.generator) * FULL_TURN_DEGREES;

        const float distance = distanceOutside(placement, point);
        if (distance - radius < wanted.nearest || distance + radius > wanted.farthest) {
            continue;
        }
        if (wanted.side && sideOf(placement, point) != *wanted.side) {
            continue;
        }
        if (!isClear(placement, point, radius, wanted.lineShare)) {
            continue;
        }
        const float y = standingHeight(placement, point, radius, nearBand);
        const LandPiece piece{.model = wanted.model,
                              .position = {point.x, y, point.y},
                              .yawDegrees = yaw,
                              .scale = wanted.scale};
        const glm::vec3 top{point.x, landPieceTop(piece), point.y};
        if (!landKeepsRevealClear(placement.revealCamera, placement.land.shape.villageSide, top)) {
            continue;
        }
        return piece;
    }
    return std::nullopt;
}

// The length of the middle line of a band this far out around the maze: the four sides
// and the quarter circles at the corners.
float bandLength(const MazeWorld& world, float distance) {
    const float sides =
        2.0F * static_cast<float>(world.maze.width() + world.maze.height()) * CELL_SIZE;
    return sides + glm::two_pi<float>() * distance;
}

// The field grid of the near band and what stands on its lines.
void placeFields(Placement& placement, float fieldTurn) {
    const MazeWorld& world = placement.world;
    const float width = static_cast<float>(world.maze.width()) * CELL_SIZE;
    const float height = static_cast<float>(world.maze.height()) * CELL_SIZE;
    const glm::vec2 middle{width / 2.0F, height / 2.0F};
    const float turn = glm::radians(fieldTurn);
    const glm::vec2 alongU{std::cos(turn), std::sin(turn)};
    const glm::vec2 alongV{-std::sin(turn), std::cos(turn)};
    // Far enough to cover the whole near band, whatever the turn.
    const float reach = glm::length(glm::vec2{width, height}) / 2.0F + TERRAIN_MARGIN + 2.0F;

    // The places of the lines across each axis, from -reach to reach.
    const auto linesAcross = [&placement, reach]() {
        std::vector<float> places;
        float place = -reach + randomBetween(placement.generator, 0.0F, LAND_FIELD_LARGEST);
        while (place < reach) {
            places.push_back(place);
            place += randomBetween(placement.generator, LAND_FIELD_SMALLEST, LAND_FIELD_LARGEST);
        }
        return places;
    };
    const std::vector<float> placesU = linesAcross();
    const std::vector<float> placesV = linesAcross();

    // A stretch of a line between two crossings becomes a wall, a hedgerow or nothing. Only
    // its parts inside the near band are kept, a little away from both of its edges, cut
    // where they leave it: the line is walked in steps of STEP metres.
    constexpr float STEP = 0.5F;
    constexpr float SHORTEST = 2.0F;
    const float nearest = LAND_NEAR_START + LAND_WALL_BASE;
    const float farthest = TERRAIN_MARGIN - LAND_WALL_BASE;
    const auto addStretch = [&](const glm::vec2& from, const glm::vec2& to) {
        const float roll = randomUnit(placement.generator);
        if (roll < LAND_FIELD_OPEN_SHARE) {
            return;
        }
        const FieldLineKind kind = roll < LAND_FIELD_OPEN_SHARE + LAND_FIELD_HEDGE_SHARE
                                       ? FieldLineKind::Hedge
                                       : FieldLineKind::Wall;
        const float length = glm::length(to - from);
        const glm::vec2 direction = (to - from) / length;
        std::optional<glm::vec2> runStart;
        glm::vec2 runEnd{0.0F};
        const auto closeRun = [&]() {
            if (runStart && glm::length(runEnd - *runStart) >= SHORTEST) {
                placement.land.fieldLines.push_back(
                    {.from = *runStart, .to = runEnd, .kind = kind});
            }
            runStart.reset();
        };
        for (float t = 0.0F; t <= length; t += STEP) {
            const glm::vec2 point = from + direction * t;
            const float distance = distanceOutside(placement, point);
            if (distance >= nearest && distance <= farthest) {
                if (!runStart) {
                    runStart = point;
                }
                runEnd = point;
            } else {
                closeRun();
            }
        }
        closeRun();
    };
    const auto addLines = [&](const std::vector<float>& places, const std::vector<float>& crossings,
                              const glm::vec2& across, const glm::vec2& along) {
        for (const float place : places) {
            float previous = -reach;
            for (std::size_t i = 0; i <= crossings.size(); ++i) {
                const float next = i < crossings.size() ? crossings[i] : reach;
                addStretch(middle + across * place + along * previous,
                           middle + across * place + along * next);
                previous = next;
            }
        }
    };
    addLines(placesU, placesV, alongU, alongV);
    addLines(placesV, placesU, alongV, alongU);
}

// The sheepfold: in a field of the near band, clear of its lines.
void placeFold(Placement& placement) {
    const float nearest = LAND_NEAR_START + LAND_FOLD_RADIUS + 1.0F;
    const float farthest = TERRAIN_MARGIN - LAND_FOLD_RADIUS - 0.5F;
    for (int attempt = 0; attempt < TRIES_PER_PIECE; ++attempt) {
        const glm::vec2 point = randomPoint(placement, TERRAIN_MARGIN);
        const float gapYaw = randomUnit(placement.generator) * FULL_TURN_DEGREES;
        const float distance = distanceOutside(placement, point);
        if (distance < nearest || distance > farthest ||
            !isClear(placement, point, LAND_FOLD_RADIUS, 1.0F)) {
            continue;
        }
        const glm::vec3 top{point.x,
                            placement.world.terrain.heightAt(point.x, point.y) + LAND_WALL_HEIGHT,
                            point.y};
        if (!landKeepsRevealClear(placement.revealCamera, placement.land.shape.villageSide, top)) {
            continue;
        }
        placement.land.foldCentre = point;
        placement.land.foldGapYawDegrees = gapYaw;
        return;
    }
}

// Places a wanted piece and keeps it, when a place is found.
std::optional<LandPiece> placeOne(Placement& placement, const Wanted& wanted, float fieldTurn) {
    std::optional<LandPiece> piece = tryPlace(placement, wanted, fieldTurn);
    if (piece) {
        placement.land.pieces.push_back(*piece);
    }
    return piece;
}

void placeNearBand(Placement& placement, float fieldTurn) {
    const MazeWorld& world = placement.world;
    const float length = bandLength(world, NEAR_BAND_MIDDLE);

    // The farmhouse with its shed, the barn and the rick first: they are the largest.
    const Wanted house{
        .model = LandModel::Farmhouse, .lineShare = HOUSE_LINE_SHARE, .squareToFields = true};
    if (const std::optional<LandPiece> farmhouse = placeOne(placement, house, fieldTurn)) {
        Wanted shed{
            .model = LandModel::Woodshed, .lineShare = HOUSE_LINE_SHARE, .squareToFields = true};
        shed.near = glm::vec2{farmhouse->position.x, farmhouse->position.z};
        shed.nearDistance = SHED_FARTHEST;
        placeOne(placement, shed, fieldTurn);
    }
    placeOne(placement,
             {.model = LandModel::Barn, .lineShare = HOUSE_LINE_SHARE, .squareToFields = true},
             fieldTurn);
    placeOne(placement,
             {.model = LandModel::Rick,
              .scale = randomBetween(placement.generator, SMALLEST_TREE, LARGEST_TREE)},
             fieldTurn);

    const int trees =
        std::min(static_cast<int>(std::lround(length / NEAR_TREE_SPACING)), MOST_NEAR_TREES);
    for (int i = 0; i < trees; ++i) {
        const float roll = randomUnit(placement.generator);
        const LandModel model = roll < OAK_SHARE                    ? LandModel::Oak
                                : roll < OAK_SHARE + HAWTHORN_SHARE ? LandModel::Hawthorn
                                                                    : LandModel::DeadOak;
        placeOne(placement,
                 {.model = model,
                  .scale = randomBetween(placement.generator, SMALLEST_TREE, LARGEST_TREE),
                  .lineShare = TREE_LINE_SHARE},
                 fieldTurn);
    }
}

void placeLandmarks(Placement& placement) {
    const MazeWorld& world = placement.world;
    const float width = static_cast<float>(world.maze.width()) * CELL_SIZE;
    const float height = static_cast<float>(world.maze.height()) * CELL_SIZE;

    // The five kinds, shuffled: the first three go to the crests in the order of
    // ALL_DIRECTIONS, skipping the side of the village.
    std::array<int, LAND_LANDMARK_KINDS> kinds{};
    std::iota(kinds.begin(), kinds.end(), 0);
    for (std::size_t i = kinds.size() - 1; i > 0; --i) {
        const auto j = static_cast<std::size_t>(
            randomBelow(placement.generator, static_cast<std::uint32_t>(i + 1)));
        std::swap(kinds.at(i), kinds.at(j));
    }
    std::size_t kind = 0;
    for (const Direction side : ALL_DIRECTIONS) {
        if (side == placement.land.shape.villageSide) {
            continue;
        }
        const auto model =
            static_cast<LandModel>(static_cast<int>(FIRST_LANDMARK_MODEL) + kinds.at(kind++));
        const float radius = landModelShape(model).radius;
        const bool alongX = side == Direction::North || side == Direction::South;
        const float sideLength = alongX ? width : height;
        for (int attempt = 0; attempt < TRIES_PER_PIECE; ++attempt) {
            const float along =
                sideLength * randomBetween(placement.generator, LANDMARK_SIDE_MARGIN,
                                           1.0F - LANDMARK_SIDE_MARGIN);
            const float out =
                randomBetween(placement.generator, LANDMARK_NEAREST, LANDMARK_FARTHEST);
            const float yaw = randomUnit(placement.generator) * FULL_TURN_DEGREES;
            glm::vec2 point{0.0F};
            switch (side) {
            case Direction::North:
                point = {along, -out};
                break;
            case Direction::South:
                point = {along, height + out};
                break;
            case Direction::West:
                point = {-out, along};
                break;
            case Direction::East:
                point = {width + out, along};
                break;
            }
            if (!isClear(placement, point, radius, 1.0F)) {
                continue;
            }
            placement.landmarks.push_back(
                {.model = model,
                 .position = {point.x, standingHeight(placement, point, radius, false), point.y},
                 .yawDegrees = yaw});
            break;
        }
    }
}

void placeMiddleBand(Placement& placement, float fieldTurn) {
    const MazeWorld& world = placement.world;
    const float length = bandLength(world, MIDDLE_BAND_MIDDLE);
    const float nearest = TERRAIN_MARGIN;
    const Direction village = placement.land.shape.villageSide;

    // The cottages of the valley first: they stand on its floor, clear of the stream.
    for (int i = 0; i < VALLEY_COTTAGES; ++i) {
        const float radius = landModelShape(LandModel::Cottage).radius;
        for (int attempt = 0; attempt < TRIES_PER_PIECE; ++attempt) {
            std::optional<LandPiece> cottage =
                tryPlace(placement,
                         {.model = LandModel::Cottage,
                          .nearest = LAND_VALLEY_START - VALLEY_COTTAGE_SLOPE,
                          .farthest = LAND_REACH,
                          .side = village,
                          .squareToFields = true},
                         fieldTurn);
            if (!cottage) {
                break;
            }
            const glm::vec2 point{cottage->position.x, cottage->position.z};
            const float along = alongSide(village, point.x, point.y);
            const float out = distanceOutside(placement, point);
            if (std::abs(out - landStreamDistance(placement.land.shape, along)) >
                radius + STREAM_CLEARANCE) {
                placement.land.pieces.push_back(*cottage);
                break;
            }
        }
    }
    for (int i = 0; i < OTHER_COTTAGES; ++i) {
        placeOne(placement,
                 {.model = LandModel::Cottage,
                  .nearest = nearest,
                  .farthest = LAND_CREST_NEAREST,
                  .squareToFields = true},
                 fieldTurn);
    }
    for (int i = 0; i < FAR_FARMHOUSES; ++i) {
        placeOne(placement,
                 {.model = LandModel::FarmhouseFar,
                  .nearest = nearest,
                  .farthest = LAND_CREST_NEAREST,
                  .squareToFields = true},
                 fieldTurn);
    }
    const int copses =
        std::min(static_cast<int>(std::lround(length / MIDDLE_COPSE_SPACING)), MOST_COPSES);
    for (int i = 0; i < copses; ++i) {
        placeOne(placement,
                 {.model = LandModel::Copse,
                  .scale = randomBetween(placement.generator, SMALLEST_TREE, LARGEST_TREE),
                  .nearest = nearest,
                  .farthest = LAND_REACH},
                 fieldTurn);
    }
    const int trees =
        std::min(static_cast<int>(std::lround(length / MIDDLE_TREE_SPACING)), MOST_MIDDLE_TREES);
    for (int i = 0; i < trees; ++i) {
        placeOne(placement,
                 {.model = LandModel::DeadOakFar,
                  .scale = randomBetween(placement.generator, SMALLEST_TREE, LARGEST_TREE),
                  .nearest = nearest,
                  .farthest = LAND_REACH},
                 fieldTurn);
    }
}

} // namespace

const LandModelShape& landModelShape(LandModel model) {
    return MODEL_SHAPES.at(static_cast<std::size_t>(model));
}

std::string_view landModelFile(LandModel model) {
    constexpr std::array<std::string_view, LAND_MODEL_COUNT> FILES{
        "land_oak",           "land_hawthorn",
        "land_dead_oak",      "land_farmhouse",
        "land_barn",          "land_woodshed",
        "land_rick",          "land_dead_oak_far",
        "land_copse_far",     "land_farmhouse_far",
        "land_cottage_far",   "land_standing_stone_far",
        "land_dovecote_far",  "land_chapel_gable_far",
        "land_watch_hut_far", "land_cairn_far"};
    return FILES.at(static_cast<std::size_t>(model));
}

std::string landWindowsFile(LandModel model) {
    std::string name{landModelFile(model)};
    constexpr std::string_view FAR = "_far";
    if (name.ends_with(FAR)) {
        name.resize(name.size() - FAR.size());
    }
    return name + "_windows.txt";
}

bool isNearLandModel(LandModel model) {
    return static_cast<int>(model) < static_cast<int>(LandModel::DeadOakFar);
}

glm::mat4 landPieceMatrix(const LandPiece& piece) {
    glm::mat4 matrix = glm::translate(glm::mat4{1.0F}, piece.position);
    matrix = glm::rotate(matrix, glm::radians(piece.yawDegrees), {0.0F, 1.0F, 0.0F});
    return glm::scale(matrix, glm::vec3{piece.scale});
}

float landPieceTop(const LandPiece& piece) {
    return piece.position.y + landModelShape(piece.model).height * piece.scale;
}

float landRingHeight(const LandShape& shape, const Terrain& terrain, float x, float z) {
    const float width = static_cast<float>(shape.mazeWidth) * CELL_SIZE;
    const float height = static_cast<float>(shape.mazeHeight) * CELL_SIZE;
    // How far the point lies past the edge of the terrain: on it and inside, the ground is
    // the terrain.
    const float pastTerrain =
        glm::length(glm::vec2{outsideRange(x, -TERRAIN_MARGIN, width + TERRAIN_MARGIN),
                              outsideRange(z, -TERRAIN_MARGIN, height + TERRAIN_MARGIN)});
    // heightAt gives a point outside the grid the height of the nearest point of its
    // border: the hills go on level from their edge.
    const float hills = terrain.heightAt(x, z);
    if (pastTerrain <= 0.0F) {
        return hills;
    }

    const float pastX = outsideRange(x, 0.0F, width);
    const float pastZ = outsideRange(z, 0.0F, height);
    const float distance = glm::length(glm::vec2{pastX, pastZ});
    float profile = 0.0F;
    if (pastX != 0.0F && pastZ != 0.0F) {
        // Beyond a corner: the two sides that meet there, blended by the angle around it.
        const Direction sideX = pastX < 0.0F ? Direction::West : Direction::East;
        const Direction sideZ = pastZ < 0.0F ? Direction::North : Direction::South;
        const float angle = std::atan2(std::abs(pastZ), std::abs(pastX)) / glm::half_pi<float>();
        profile = glm::mix(sideHeight(shape, sideX, std::clamp(z, 0.0F, height), distance),
                           sideHeight(shape, sideZ, std::clamp(x, 0.0F, width), distance),
                           smoothStep(angle));
    } else if (pastX != 0.0F) {
        profile = sideHeight(shape, pastX < 0.0F ? Direction::West : Direction::East, z, distance);
    } else {
        profile =
            sideHeight(shape, pastZ < 0.0F ? Direction::North : Direction::South, x, distance);
    }
    return glm::mix(hills, profile, smoothStep(pastTerrain / LAND_RING_BLEND));
}

float landGroundHeight(const Land& land, float x, float z) {
    if (land.ringColumns < 2 || land.ringRows < 2) {
        return 0.0F;
    }
    // The diagonal convention of Terrain: a square is cut from its north-west corner to its
    // south-east corner.
    const float gridX = std::clamp((x - land.ringMinX) / LAND_RING_SPACING, 0.0F,
                                   static_cast<float>(land.ringColumns - 1));
    const float gridZ = std::clamp((z - land.ringMinZ) / LAND_RING_SPACING, 0.0F,
                                   static_cast<float>(land.ringRows - 1));
    const int column = std::min(static_cast<int>(gridX), land.ringColumns - 2);
    const int row = std::min(static_cast<int>(gridZ), land.ringRows - 2);
    const float alongX = gridX - static_cast<float>(column);
    const float alongZ = gridZ - static_cast<float>(row);
    const float northWest = ringPoint(land, column, row);
    const float northEast = ringPoint(land, column + 1, row);
    const float southWest = ringPoint(land, column, row + 1);
    const float southEast = ringPoint(land, column + 1, row + 1);
    if (alongX >= alongZ) {
        return northWest + alongX * (northEast - northWest) + alongZ * (southEast - northEast);
    }
    return northWest + alongZ * (southWest - northWest) + alongX * (southEast - southWest);
}

float landStreamDistance(const LandShape& shape, float along) {
    const float phase =
        shape.phases.at(static_cast<std::size_t>(2 * static_cast<int>(shape.villageSide)));
    return (LAND_VALLEY_START + LAND_VALLEY_END) / 2.0F +
           LAND_STREAM_WANDER * std::sin(along * glm::two_pi<float>() / STREAM_WAVE + phase);
}

bool landCoversGround(const Land& land, float x, float z) {
    const glm::vec2 point{x, z};
    for (const FieldLine& line : land.fieldLines) {
        const float halfWidth =
            (line.kind == FieldLineKind::Wall ? LAND_WALL_BASE : LAND_HEDGE_WIDTH) / 2.0F;
        if (distanceToLine(point, line.from, line.to) < halfWidth + LAND_BARE_MARGIN) {
            return true;
        }
    }
    if (land.foldCentre) {
        const glm::vec2 offset = point - *land.foldCentre;
        const float onRing = std::abs(glm::length(offset) - LAND_FOLD_RADIUS);
        // The gap of the fold, as an angle either side of the way it faces.
        const float halfGap = LAND_FOLD_GAP / 2.0F / LAND_FOLD_RADIUS;
        const float fromGap = std::abs(
            std::remainder(std::atan2(offset.y, offset.x) - glm::radians(land.foldGapYawDegrees),
                           glm::two_pi<float>()));
        if (onRing < LAND_WALL_BASE / 2.0F + LAND_BARE_MARGIN && fromGap > halfGap) {
            return true;
        }
    }
    // The floor of a building: most of the circle placement keeps around it. The circle
    // reaches the corners of the roof, the walls stand inside it.
    constexpr float FLOOR_SHARE = 0.8F;
    for (std::size_t i = 0; i < land.nearCount && i < land.pieces.size(); ++i) {
        const LandPiece& piece = land.pieces[i];
        const bool building = piece.model == LandModel::Farmhouse ||
                              piece.model == LandModel::Barn ||
                              piece.model == LandModel::Woodshed || piece.model == LandModel::Rick;
        if (!building) {
            continue;
        }
        const float radius = landModelShape(piece.model).radius * piece.scale * FLOOR_SHARE;
        if (glm::length(point - glm::vec2{piece.position.x, piece.position.z}) < radius) {
            return true;
        }
    }
    return false;
}

bool landKeepsRevealClear(const glm::vec3& camera, Direction villageSide, const glm::vec3& top) {
    const glm::vec2 offset{top.x - camera.x, top.z - camera.z};
    const float distance = glm::length(offset);
    if (distance <= 0.0F) {
        return true;
    }
    // The bearing to the village is the direction of its side, as a step on the ground.
    const glm::vec2 village{static_cast<float>(columnStep(villageSide)),
                            static_cast<float>(rowStep(villageSide))};
    const float off = std::acos(std::clamp(glm::dot(offset / distance, village), -1.0F, 1.0F));
    if (off > glm::radians(LAND_REVEAL_CONE_DEGREES)) {
        return true;
    }
    return top.y <= camera.y + distance * std::tan(glm::radians(LAND_REVEAL_CLEAR_DEGREES));
}

Land placeLand(const MazeWorld& world, float heightScale) {
    Land land;
    std::mt19937 generator(world.seed + LAND_SEED_OFFSET);

    // The shape of the ground, and the grid of the ring from it.
    land.shape.mazeWidth = world.maze.width();
    land.shape.mazeHeight = world.maze.height();
    land.shape.villageSide = world.hasGate ? villageSide(world) : Direction::North;
    land.shape.heightScale = heightScale;
    for (float& phase : land.shape.phases) {
        phase = randomUnit(generator) * glm::two_pi<float>();
    }
    land.ringMinX = -LAND_RING_END;
    land.ringMinZ = -LAND_RING_END;
    const int stepsOut = static_cast<int>(std::lround(LAND_RING_END / LAND_RING_SPACING));
    const int stepsPerCell = static_cast<int>(std::lround(CELL_SIZE / LAND_RING_SPACING));
    land.ringColumns = world.maze.width() * stepsPerCell + 2 * stepsOut + 1;
    land.ringRows = world.maze.height() * stepsPerCell + 2 * stepsOut + 1;
    land.ringHeights.reserve(static_cast<std::size_t>(land.ringColumns) *
                             static_cast<std::size_t>(land.ringRows));
    for (int row = 0; row < land.ringRows; ++row) {
        for (int column = 0; column < land.ringColumns; ++column) {
            land.ringHeights.push_back(
                landRingHeight(land.shape, world.terrain,
                               land.ringMinX + static_cast<float>(column) * LAND_RING_SPACING,
                               land.ringMinZ + static_cast<float>(row) * LAND_RING_SPACING));
        }
    }

    // The stream: the middle line of the valley, along the whole village side and a little
    // past its ends.
    const bool alongX =
        land.shape.villageSide == Direction::North || land.shape.villageSide == Direction::South;
    const float sideLength =
        static_cast<float>(alongX ? world.maze.width() : world.maze.height()) * CELL_SIZE;
    const glm::vec2 outward{static_cast<float>(columnStep(land.shape.villageSide)),
                            static_cast<float>(rowStep(land.shape.villageSide))};
    const glm::vec2 sideStart = glm::vec2{land.shape.villageSide == Direction::East
                                              ? static_cast<float>(world.maze.width()) * CELL_SIZE
                                              : 0.0F,
                                          land.shape.villageSide == Direction::South
                                              ? static_cast<float>(world.maze.height()) * CELL_SIZE
                                              : 0.0F};
    const glm::vec2 alongDirection = alongX ? glm::vec2{1.0F, 0.0F} : glm::vec2{0.0F, 1.0F};
    constexpr float STREAM_STEP = 1.0F;
    constexpr float STREAM_PAST_ENDS = 18.0F;
    for (float along = -STREAM_PAST_ENDS; along <= sideLength + STREAM_PAST_ENDS;
         along += STREAM_STEP) {
        land.stream.push_back(sideStart + alongDirection * along +
                              outward * landStreamDistance(land.shape, along));
    }

    // The fields, then the pieces: near band, landmarks, middle band. The landmarks come
    // last in the list.
    const float fieldTurn =
        randomBetween(generator, -LAND_FIELD_TURN_DEGREES, LAND_FIELD_TURN_DEGREES);
    Placement placement{.world = world,
                        .land = land,
                        .generator = generator,
                        .revealCamera =
                            world.exitPosition + glm::vec3{0.0F, VILLAGE_REVEAL_HEIGHT, 0.0F}};
    placeFields(placement, fieldTurn);
    placeFold(placement);
    placeNearBand(placement, fieldTurn);
    land.nearCount = land.pieces.size();
    placeLandmarks(placement);
    placeMiddleBand(placement, fieldTurn);
    land.landmarkCount = placement.landmarks.size();
    land.pieces.insert(land.pieces.end(), placement.landmarks.begin(), placement.landmarks.end());
    return land;
}

std::vector<LandmarkView> landmarks(const Land& land) {
    std::vector<LandmarkView> views;
    for (std::size_t i = land.pieces.size() - land.landmarkCount; i < land.pieces.size(); ++i) {
        const LandPiece& piece = land.pieces[i];
        views.push_back({.model = piece.model,
                         .top = {piece.position.x, landPieceTop(piece), piece.position.z}});
    }
    return views;
}

bool landmarkInSight(const glm::vec3& eye, const glm::vec3& top, const scene::Frustum& frustum,
                     std::span<const scene::Aabb> walls) {
    // A box of a few centimetres around the top: canBeSeen tests boxes.
    constexpr float TOP_HALF = 0.05F;
    if (!scene::canBeSeen(frustum, {.min = top - TOP_HALF, .max = top + TOP_HALF})) {
        return false;
    }
    const glm::vec3 offset = top - eye;
    const float distance = glm::length(offset);
    if (distance <= 0.0F) {
        return true;
    }
    const scene::Ray ray{.origin = eye, .direction = offset / distance};
    return !scene::nearestHit(ray, walls, distance).hit;
}

std::vector<LandWindow> parseLandWindows(std::string_view text) {
    std::vector<LandWindow> windows;
    std::istringstream lines{std::string(text)};
    std::string line;
    while (std::getline(lines, line)) {
        if (line.empty() || line.front() == '#') {
            continue;
        }
        std::istringstream numbers(line);
        LandWindow window;
        if (numbers >> window.middle.x >> window.middle.y >> window.middle.z >> window.normal.x >>
            window.normal.y >> window.normal.z >> window.width >> window.height) {
            windows.push_back(window);
        }
    }
    return windows;
}

std::array<std::vector<std::size_t>, LAND_LIT_NIGHTS> dealLandWindows(std::size_t windowCount,
                                                                      std::uint32_t seed) {
    std::vector<std::size_t> order(windowCount);
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::mt19937 generator(seed + LAND_WINDOW_SEED_OFFSET);
    for (std::size_t i = windowCount; i > 1; --i) {
        const auto j =
            static_cast<std::size_t>(randomBelow(generator, static_cast<std::uint32_t>(i)));
        std::swap(order[i - 1], order[j]);
    }

    std::array<std::vector<std::size_t>, LAND_LIT_NIGHTS> nights;
    float share = 0.0F;
    std::size_t start = 0;
    for (std::size_t night = 0; night < nights.size(); ++night) {
        share += LAND_WINDOW_SHARES.at(night);
        const std::size_t end =
            night + 1 == nights.size()
                ? windowCount
                : static_cast<std::size_t>(std::lround(share * static_cast<float>(windowCount)));
        nights.at(night).assign(order.begin() + static_cast<std::ptrdiff_t>(start),
                                order.begin() + static_cast<std::ptrdiff_t>(end));
        start = end;
    }
    return nights;
}

int farSilhouetteNight(FarSilhouette silhouette) {
    return silhouette == FarSilhouette::Mill ? 2 : 4;
}

LandModel farSilhouetteModel(FarSilhouette silhouette) {
    return silhouette == FarSilhouette::Mill ? LandModel::FarmhouseFar : LandModel::ChapelGable;
}

glm::mat4 farSilhouetteMatrix(FarSilhouette silhouette, Direction villageSide, float farPlane,
                              float elevationDegrees) {
    const SilhouettePlace& place = SILHOUETTE_PLACES.at(static_cast<std::size_t>(silhouette));
    // Read from the right: the model is scaled to its size, moved out along -Z behind the
    // rim, turned to its bearing beside the village, and then placed like the village
    // (game::villageMatrix): scaled about the eye, lifted with the ridge, turned to its
    // side.
    glm::mat4 matrix = villageMatrix(villageSide, farPlane, elevationDegrees);
    matrix = glm::rotate(matrix, glm::radians(-place.bearingDegrees), {0.0F, 1.0F, 0.0F});
    matrix = glm::translate(matrix, {0.0F, place.footHeight, -FAR_SILHOUETTE_DISTANCE});
    return glm::scale(matrix, glm::vec3{place.scale});
}

} // namespace game
