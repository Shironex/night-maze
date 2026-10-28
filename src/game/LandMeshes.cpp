// LandMeshes: the triangles of what the land builds in code: the ground of the ring, the
// field walls and the sheepfold, the hedgerows, the stream and the lit windows.
#include "game/LandMeshes.hpp"

#include "assets/Tangents.hpp"
#include "game/MazeLayout.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace game {

namespace {

constexpr glm::vec3 UP{0.0F, 1.0F, 0.0F};

// A wall or a hedge is cut into pieces about this long along its line, so it follows the
// ground.
constexpr float LINE_STEP = 0.8F;

// The top of a field wall goes up and down by up to this much, from stone to stone.
constexpr float WALL_TOP_WANDER = 0.07F;

// The ends of a hedge round off over this many metres.
constexpr float HEDGE_END_TAPER = 0.9F;

// The section of a hedge, from its foot on one side over its top to its foot on the
// other: across (a share of half the width, - is the left) and up (a share of the
// height). It bulges a little above its foot, as a hedge that is cut back from below.
struct SectionPoint {
    float across;
    float up;
};
constexpr std::array<SectionPoint, 9> HEDGE_SECTION{{
    {.across = -0.92F, .up = 0.0F},
    {.across = -1.0F, .up = 0.3F},
    {.across = -0.9F, .up = 0.65F},
    {.across = -0.6F, .up = 0.9F},
    {.across = 0.0F, .up = 1.0F},
    {.across = 0.6F, .up = 0.9F},
    {.across = 0.9F, .up = 0.65F},
    {.across = 1.0F, .up = 0.3F},
    {.across = 0.92F, .up = 0.0F},
}};

// The lumps of a hedge: two waves along it, these many metres long.
constexpr float HEDGE_LONG_LUMP = 2.3F;
constexpr float HEDGE_SHORT_LUMP = 0.9F;

// The stream lies this far over the ground of the ring, and its picture repeats every
// this many metres.
constexpr float STREAM_LIFT = 0.05F;
constexpr float STREAM_TEXTURE_SPAN = 4.0F;

// A number from 0 to 1 that depends only on the two whole numbers (a hash of them): the
// same on every computer, unlike the standard random distributions.
float hashUnit(std::uint32_t a, std::uint32_t b) {
    std::uint32_t h = a * 0x9E3779B1U ^ (b + 0x7F4A7C15U) * 0x85EBCA77U;
    h ^= h >> 15U;
    h *= 0x2C1B3C6DU;
    h ^= h >> 12U;
    h *= 0x297A2D39U;
    h ^= h >> 15U;
    constexpr std::uint32_t STEPS = 65536U;
    return static_cast<float>(h % STEPS) / static_cast<float>(STEPS);
}

std::uint32_t addVertex(TerrainMeshData& mesh, const glm::vec3& position, const glm::vec3& normal,
                        const glm::vec2& uv) {
    mesh.vertices.push_back({.position = position, .normal = normal, .uv = uv});
    return static_cast<std::uint32_t>(mesh.vertices.size() - 1);
}

void addQuad(TerrainMeshData& mesh, std::uint32_t a, std::uint32_t b, std::uint32_t c,
             std::uint32_t d) {
    mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
}

// A line cut into points about LINE_STEP apart, with the distance along it of each.
struct LinePoint {
    glm::vec2 position;
    glm::vec2 direction;
    float along;
};

std::vector<LinePoint> cutLine(std::span<const glm::vec2> corners) {
    std::vector<LinePoint> points;
    float along = 0.0F;
    for (std::size_t i = 0; i + 1 < corners.size(); ++i) {
        const glm::vec2 from = corners[i];
        const glm::vec2 to = corners[i + 1];
        const float length = glm::length(to - from);
        if (length <= 0.0F) {
            continue;
        }
        const glm::vec2 direction = (to - from) / length;
        const int steps = std::max(1, static_cast<int>(std::ceil(length / LINE_STEP)));
        // The first point of every stretch but the first is the last of the one before.
        for (int step = points.empty() ? 0 : 1; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            points.push_back({.position = from + (to - from) * t,
                              .direction = direction,
                              .along = along + length * t});
        }
        along += length;
    }
    return points;
}

// One drystone wall along a line of corners.
void addWall(TerrainMeshData& mesh, const Terrain& terrain, std::span<const glm::vec2> corners,
             std::uint32_t number) {
    const std::vector<LinePoint> points = cutLine(corners);
    if (points.size() < 2) {
        return;
    }
    const float slant = (LAND_WALL_BASE - LAND_WALL_TOP) / 2.0F;
    // The outward normal of the left side, in the frame (side, up) of a point.
    const glm::vec2 sideNormal = glm::normalize(glm::vec2{LAND_WALL_HEIGHT, slant});

    std::vector<std::array<std::uint32_t, 6>> rows;
    rows.reserve(points.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
        const LinePoint& point = points[i];
        const glm::vec3 left{-point.direction.y, 0.0F, point.direction.x};
        const glm::vec3 middle{point.position.x, 0.0F, point.position.y};
        const float ground = terrain.heightAt(point.position.x, point.position.y);
        const float foot = ground - LAND_WALL_SINK;
        const float top =
            foot + LAND_WALL_HEIGHT +
            WALL_TOP_WANDER * (2.0F * hashUnit(number, static_cast<std::uint32_t>(i)) - 1.0F);
        const float u = point.along / WALL_HEIGHT;
        const float topV = (top - foot) / WALL_HEIGHT;
        const glm::vec3 leftNormal = left * sideNormal.x + UP * sideNormal.y;
        const glm::vec3 rightNormal = -left * sideNormal.x + UP * sideNormal.y;
        const glm::vec3 leftFoot = middle + left * (LAND_WALL_BASE / 2.0F) + UP * foot;
        const glm::vec3 leftTop = middle + left * (LAND_WALL_TOP / 2.0F) + UP * top;
        const glm::vec3 rightTop = middle - left * (LAND_WALL_TOP / 2.0F) + UP * top;
        const glm::vec3 rightFoot = middle - left * (LAND_WALL_BASE / 2.0F) + UP * foot;
        rows.push_back({
            addVertex(mesh, leftFoot, leftNormal, {u, 0.0F}),
            addVertex(mesh, leftTop, leftNormal, {u, topV}),
            addVertex(mesh, leftTop, UP, {u, topV}),
            addVertex(mesh, rightTop, UP, {u, topV + LAND_WALL_TOP / WALL_HEIGHT}),
            addVertex(mesh, rightTop, rightNormal, {u, topV}),
            addVertex(mesh, rightFoot, rightNormal, {u, 0.0F}),
        });
    }
    for (std::size_t i = 0; i + 1 < rows.size(); ++i) {
        const auto& a = rows[i];
        const auto& b = rows[i + 1];
        addQuad(mesh, a[0], b[0], b[1], a[1]);
        addQuad(mesh, a[2], b[2], b[3], a[3]);
        addQuad(mesh, a[4], b[4], b[5], a[5]);
    }
    // The two ends, closed by a face each, when the line does not come back to its start.
    if (glm::length(corners.front() - corners.back()) > 0.01F) {
        for (const bool start : {true, false}) {
            const auto& row = start ? rows.front() : rows.back();
            const LinePoint& point = start ? points.front() : points.back();
            const float sign = start ? -1.0F : 1.0F;
            const glm::vec3 normal{sign * point.direction.x, 0.0F, sign * point.direction.y};
            const auto copy = [&mesh, &normal](std::uint32_t index, float u) {
                const gfx::Vertex vertex = mesh.vertices[index];
                return addVertex(mesh, vertex.position, normal, {u, vertex.uv.y});
            };
            const float half = LAND_WALL_BASE / 2.0F / WALL_HEIGHT;
            addQuad(mesh, copy(row[0], -half), copy(row[1], -half + slant / WALL_HEIGHT),
                    copy(row[4], half - slant / WALL_HEIGHT), copy(row[5], half));
        }
    }
}

// One hedgerow along a stretch.
void addHedge(TerrainMeshData& mesh, const Terrain& terrain, const FieldLine& line,
              std::uint32_t seed, std::uint32_t number) {
    const std::array<glm::vec2, 2> corners{line.from, line.to};
    const std::vector<LinePoint> points = cutLine(corners);
    if (points.size() < 2) {
        return;
    }
    const float length = points.back().along;
    const float turn = glm::two_pi<float>();
    const float phaseA = hashUnit(seed, number * 2U) * turn;
    const float phaseB = hashUnit(seed, number * 2U + 1U) * turn;
    const auto first = static_cast<std::uint32_t>(mesh.vertices.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
        const LinePoint& point = points[i];
        const glm::vec3 left{-point.direction.y, 0.0F, point.direction.x};
        const float ground = terrain.heightAt(point.position.x, point.position.y);
        const float lump = 0.6F * std::sin(point.along * turn / HEDGE_LONG_LUMP + phaseA) +
                           0.4F * std::sin(point.along * turn / HEDGE_SHORT_LUMP + phaseB);
        // The ends round off: the section shrinks towards them.
        const float toEnd = std::min(point.along, length - point.along);
        const float end = std::sqrt(std::clamp(toEnd / HEDGE_END_TAPER, 0.0F, 1.0F));
        const float height = (LAND_HEDGE_HEIGHT + LAND_HEDGE_LUMP * lump) * (0.25F + 0.75F * end);
        const float halfWidth =
            (LAND_HEDGE_WIDTH / 2.0F + 0.3F * LAND_HEDGE_LUMP * lump) * (0.3F + 0.7F * end);
        const float foot = ground - LAND_WALL_SINK;
        const float u = point.along / HEDGE_TEXTURE_SPAN;
        for (const SectionPoint& section : HEDGE_SECTION) {
            const glm::vec3 position = glm::vec3{point.position.x, foot, point.position.y} +
                                       left * (section.across * halfWidth) +
                                       UP * (section.up * height);
            // The normal of an ellipse through the section.
            const glm::vec3 normal =
                glm::normalize(left * (section.across / halfWidth) + UP * (section.up / height));
            // v: 0 at the foot, 1 at the top, on both sides. The top point is on both.
            addVertex(mesh, position, normal, {u, section.up});
        }
    }
    const auto across = static_cast<std::uint32_t>(HEDGE_SECTION.size());
    for (std::uint32_t i = 0; i + 1 < points.size(); ++i) {
        for (std::uint32_t k = 0; k + 1 < across; ++k) {
            const std::uint32_t a = first + i * across + k;
            const std::uint32_t b = first + (i + 1) * across + k;
            addQuad(mesh, a, b, b + 1, a + 1);
        }
    }
}

// The points of the fold: a circle without its gap.
std::vector<glm::vec2> foldCorners(const Land& land) {
    std::vector<glm::vec2> corners;
    if (!land.foldCentre) {
        return corners;
    }
    const float gapAngle = glm::radians(land.foldGapYawDegrees);
    const float halfGap = LAND_FOLD_GAP / 2.0F / LAND_FOLD_RADIUS;
    constexpr int STEPS = 28;
    const float sweep = glm::two_pi<float>() - 2.0F * halfGap;
    for (int step = 0; step <= STEPS; ++step) {
        const float angle =
            gapAngle + halfGap + sweep * static_cast<float>(step) / static_cast<float>(STEPS);
        corners.push_back(*land.foldCentre +
                          glm::vec2{std::cos(angle), std::sin(angle)} * LAND_FOLD_RADIUS);
    }
    return corners;
}

} // namespace

TerrainMeshData buildLandRingMesh(const Land& land, const Terrain& terrain) {
    TerrainMeshData mesh;
    const int columns = land.ringColumns;
    const int rows = land.ringRows;
    if (columns < 2 || rows < 2) {
        return mesh;
    }
    const float spacing = LAND_RING_SPACING;
    const auto pointX = [&land, spacing](int column) {
        return land.ringMinX + static_cast<float>(column) * spacing;
    };
    const auto pointZ = [&land, spacing](int row) {
        return land.ringMinZ + static_cast<float>(row) * spacing;
    };
    // Whether a point lies strictly inside the terrain, or on its edge or outside.
    const float slack = 0.01F;
    const auto insideTerrain = [&terrain, slack](float x, float z) {
        return x > terrain.minX() + slack && x < terrain.maxX() - slack &&
               z > terrain.minZ() + slack && z < terrain.maxZ() - slack;
    };
    const auto height = [&land](int column, int row) {
        const int c = std::clamp(column, 0, land.ringColumns - 1);
        const int r = std::clamp(row, 0, land.ringRows - 1);
        return land
            .ringHeights[static_cast<std::size_t>(r) * static_cast<std::size_t>(land.ringColumns) +
                         static_cast<std::size_t>(c)];
    };

    mesh.vertices.reserve(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const float x = pointX(column);
            const float z = pointZ(row);
            const float y = height(column, row) - (insideTerrain(x, z) ? LAND_RING_TUCK : 0.0F);
            // The normal from the slopes to the four neighbours.
            const float slopeX =
                (height(column + 1, row) - height(column - 1, row)) / (2.0F * spacing);
            const float slopeZ =
                (height(column, row + 1) - height(column, row - 1)) / (2.0F * spacing);
            mesh.vertices.push_back({.position = {x, y, z},
                                     .normal = glm::normalize(glm::vec3{-slopeX, 1.0F, -slopeZ}),
                                     .uv = {x / GROUND_TEXTURE_SPAN, -z / GROUND_TEXTURE_SPAN}});
        }
    }

    // A square is drawn when part of it lies outside the terrain, or when it lies inside
    // but within one square of its edge: that row is the tucked one.
    const auto drawn = [&](int column, int row) {
        const float x0 = pointX(column);
        const float z0 = pointZ(row);
        const float x1 = x0 + spacing;
        const float z1 = z0 + spacing;
        const bool inside = x0 >= terrain.minX() - slack && x1 <= terrain.maxX() + slack &&
                            z0 >= terrain.minZ() - slack && z1 <= terrain.maxZ() + slack;
        if (!inside) {
            return true;
        }
        return x0 < terrain.minX() + spacing - slack || x1 > terrain.maxX() - spacing + slack ||
               z0 < terrain.minZ() + spacing - slack || z1 > terrain.maxZ() - spacing + slack;
    };
    for (int row = 0; row + 1 < rows; ++row) {
        for (int column = 0; column + 1 < columns; ++column) {
            if (!drawn(column, row)) {
                continue;
            }
            const auto index = [columns](int c, int r) {
                return static_cast<std::uint32_t>(r * columns + c);
            };
            const std::uint32_t northWest = index(column, row);
            const std::uint32_t northEast = index(column + 1, row);
            const std::uint32_t southWest = index(column, row + 1);
            const std::uint32_t southEast = index(column + 1, row + 1);
            // The diagonal of Terrain, north-west to south-east, front faces up.
            mesh.indices.insert(mesh.indices.end(), {northWest, southEast, northEast});
            mesh.indices.insert(mesh.indices.end(), {northWest, southWest, southEast});
        }
    }
    assets::computeTangents(mesh.vertices, mesh.indices);
    return mesh;
}

TerrainMeshData buildFieldWallMesh(const Land& land, const Terrain& terrain) {
    TerrainMeshData mesh;
    std::uint32_t number = 0;
    for (const FieldLine& line : land.fieldLines) {
        if (line.kind == FieldLineKind::Wall) {
            const std::array<glm::vec2, 2> corners{line.from, line.to};
            addWall(mesh, terrain, corners, number);
        }
        ++number;
    }
    const std::vector<glm::vec2> fold = foldCorners(land);
    if (!fold.empty()) {
        addWall(mesh, terrain, fold, number);
    }
    assets::computeTangents(mesh.vertices, mesh.indices);
    return mesh;
}

TerrainMeshData buildHedgerowMesh(const Land& land, const Terrain& terrain, std::uint32_t seed) {
    TerrainMeshData mesh;
    std::uint32_t number = 0;
    for (const FieldLine& line : land.fieldLines) {
        if (line.kind == FieldLineKind::Hedge) {
            addHedge(mesh, terrain, line, seed, number);
        }
        ++number;
    }
    assets::computeTangents(mesh.vertices, mesh.indices);
    return mesh;
}

TerrainMeshData buildStreamMesh(const Land& land) {
    TerrainMeshData mesh;
    // The stream is cut into runs where it lies in the valley; each run is one strip.
    std::vector<glm::vec2> run;
    const auto closeRun = [&mesh, &land, &run]() {
        const std::vector<LinePoint> points = cutLine(run);
        run.clear();
        if (points.size() < 2) {
            return;
        }
        const auto first = static_cast<std::uint32_t>(mesh.vertices.size());
        for (const LinePoint& point : points) {
            const glm::vec2 left{-point.direction.y, point.direction.x};
            for (const float side : {-1.0F, 1.0F}) {
                const glm::vec2 at = point.position + left * (side * LAND_STREAM_WIDTH / 2.0F);
                const float y = landGroundHeight(land, at.x, at.y) + STREAM_LIFT;
                addVertex(mesh, {at.x, y, at.y}, UP,
                          {point.along / STREAM_TEXTURE_SPAN, (side + 1.0F) / 2.0F});
            }
        }
        for (std::uint32_t i = 0; i + 1 < points.size(); ++i) {
            const std::uint32_t a = first + 2 * i;
            addQuad(mesh, a, a + 2, a + 3, a + 1);
        }
    };
    for (const glm::vec2& point : land.stream) {
        if (landGroundHeight(land, point.x, point.y) < LAND_STREAM_TOP) {
            run.push_back(point);
        } else {
            closeRun();
        }
    }
    closeRun();
    assets::computeTangents(mesh.vertices, mesh.indices);
    return mesh;
}

TerrainMeshData buildWindowMesh(std::span<const PlacedWindow> windows) {
    TerrainMeshData mesh;
    for (const PlacedWindow& placed : windows) {
        const LandWindow& window = placed.window;
        const glm::vec3 normal = glm::normalize(window.normal);
        glm::vec3 across = glm::cross(UP, normal);
        if (glm::length(across) < 0.001F) {
            continue;
        }
        across = glm::normalize(across);
        const glm::vec3 middle = window.middle + normal * LAND_WINDOW_LIFT;
        const glm::vec3 halfAcross = across * (window.width / 2.0F);
        const glm::vec3 halfUp = UP * (window.height / 2.0F);
        const glm::vec3 worldNormal = glm::normalize(glm::mat3{placed.matrix} * normal);
        const auto corner = [&](const glm::vec3& point, const glm::vec2& uv) {
            return addVertex(mesh, glm::vec3{placed.matrix * glm::vec4{point, 1.0F}}, worldNormal,
                             uv);
        };
        const std::uint32_t a = corner(middle - halfAcross - halfUp, {0.0F, 0.0F});
        const std::uint32_t b = corner(middle + halfAcross - halfUp, {1.0F, 0.0F});
        const std::uint32_t c = corner(middle + halfAcross + halfUp, {1.0F, 1.0F});
        const std::uint32_t d = corner(middle - halfAcross + halfUp, {0.0F, 1.0F});
        addQuad(mesh, a, b, c, d);
    }
    assets::computeTangents(mesh.vertices, mesh.indices);
    return mesh;
}

} // namespace game
