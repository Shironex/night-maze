// MazeLayout: where the cells, walls and pillars of a maze stand in the world.
// See docs/modules/game/maze-generator.md
#include "game/MazeLayout.hpp"

#include <cmath>

namespace game {

namespace {

// From the centre of a cell to its edge.
constexpr float HALF_CELL = 0.5F;

// Half extents of the boxes, as scene::Aabb::fromCenter wants them. A wall along Z is
// the same box with its length and its thickness swapped. The half thickness of a wall is
// written exactly like the half size of a pillar below (the constant divided by 2), so
// both give the same float and the faces of the two kinds of boxes meet without a step.
constexpr glm::vec3 WALL_ALONG_X_HALF_EXTENTS{WALL_LENGTH / 2.0F, WALL_HEIGHT / 2.0F,
                                              WALL_COLLISION_THICKNESS / 2.0F};
constexpr glm::vec3 WALL_ALONG_Z_HALF_EXTENTS{WALL_COLLISION_THICKNESS / 2.0F, WALL_HEIGHT / 2.0F,
                                              WALL_LENGTH / 2.0F};
constexpr glm::vec3 PILLAR_HALF_EXTENTS{PILLAR_SIZE / 2.0F, PILLAR_HEIGHT / 2.0F,
                                        PILLAR_SIZE / 2.0F};

// World coordinate of a grid line. Line 0 is the west (or north) border of the maze,
// line 1 is one cell further.
float gridLine(int line) {
    return static_cast<float>(line) * CELL_SIZE;
}

// World coordinate of the middle of a column or of a row.
float cellMiddle(int cell) {
    return (static_cast<float>(cell) + HALF_CELL) * CELL_SIZE;
}

// True when the maze has the cell and that cell has a wall on the given side.
bool cellHasWall(const Maze& maze, int x, int z, Direction side) {
    return maze.contains(x, z) && maze.hasWall(x, z, side);
}

// True when at least one wall ends at the grid corner where column line cornerX crosses
// row line cornerZ. Up to four cells meet at a corner. Each of them has two walls that
// end there: the two on the sides of the cell that face the corner. Border corners have
// fewer cells around them, the missing ones are skipped by cellHasWall.
bool cornerHasWall(const Maze& maze, int cornerX, int cornerZ) {
    // The cell north-west of the corner touches it with its south and east walls.
    if (cellHasWall(maze, cornerX - 1, cornerZ - 1, Direction::South) ||
        cellHasWall(maze, cornerX - 1, cornerZ - 1, Direction::East)) {
        return true;
    }
    // North-east of the corner: south and west walls.
    if (cellHasWall(maze, cornerX, cornerZ - 1, Direction::South) ||
        cellHasWall(maze, cornerX, cornerZ - 1, Direction::West)) {
        return true;
    }
    // South-west of the corner: north and east walls.
    if (cellHasWall(maze, cornerX - 1, cornerZ, Direction::North) ||
        cellHasWall(maze, cornerX - 1, cornerZ, Direction::East)) {
        return true;
    }
    // South-east of the corner: north and west walls.
    return cellHasWall(maze, cornerX, cornerZ, Direction::North) ||
           cellHasWall(maze, cornerX, cornerZ, Direction::West);
}

} // namespace

glm::vec3 cellCenter(int x, int z) {
    return {cellMiddle(x), 0.0F, cellMiddle(z)};
}

MazeCell cellAt(const glm::vec3& position) {
    // The column is the number of whole cells between the origin and the point. floor
    // rounds DOWN, also below zero: -0.5 m becomes column -1. A plain conversion to int
    // would cut the fraction off and give column 0, a cell of the maze, for a point
    // that is outside of it.
    return {.x = static_cast<int>(std::floor(position.x / CELL_SIZE)),
            .z = static_cast<int>(std::floor(position.z / CELL_SIZE))};
}

WallSegment wallSegmentOn(int x, int z, Direction side) {
    // The north edge of a cell is the row line z, the south edge the row line z + 1.
    // The west edge is the column line x, the east edge the column line x + 1.
    if (side == Direction::North) {
        return {.position = {cellMiddle(x), 0.0F, gridLine(z)}, .axis = WallAxis::AlongX};
    }
    if (side == Direction::South) {
        return {.position = {cellMiddle(x), 0.0F, gridLine(z + 1)}, .axis = WallAxis::AlongX};
    }
    if (side == Direction::West) {
        return {.position = {gridLine(x), 0.0F, cellMiddle(z)}, .axis = WallAxis::AlongZ};
    }
    // East is what is left.
    return {.position = {gridLine(x + 1), 0.0F, cellMiddle(z)}, .axis = WallAxis::AlongZ};
}

std::vector<WallSegment> wallSegments(const Maze& maze) {
    std::vector<WallSegment> segments;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            // Every cell reports its north and its west wall.
            if (maze.hasWall(x, z, Direction::North)) {
                segments.push_back(wallSegmentOn(x, z, Direction::North));
            }
            if (maze.hasWall(x, z, Direction::West)) {
                segments.push_back(wallSegmentOn(x, z, Direction::West));
            }

            // The south wall of a cell is the north wall of the cell below it, and the
            // east wall is the west wall of the cell to the right: those cells report
            // them. That is how a shared wall ends up in the list once. Only the last
            // row and the last column have no such neighbour and report the wall
            // themselves.
            if (z == maze.height() - 1 && maze.hasWall(x, z, Direction::South)) {
                segments.push_back(wallSegmentOn(x, z, Direction::South));
            }
            if (x == maze.width() - 1 && maze.hasWall(x, z, Direction::East)) {
                segments.push_back(wallSegmentOn(x, z, Direction::East));
            }
        }
    }
    return segments;
}

std::vector<glm::vec3> pillarPositions(const Maze& maze) {
    std::vector<glm::vec3> positions;
    // A grid of width by height cells has one more line than cells in each direction,
    // hence "<=": corners run from 0 to width and from 0 to height.
    for (int cornerZ = 0; cornerZ <= maze.height(); ++cornerZ) {
        for (int cornerX = 0; cornerX <= maze.width(); ++cornerX) {
            if (cornerHasWall(maze, cornerX, cornerZ)) {
                positions.emplace_back(gridLine(cornerX), 0.0F, gridLine(cornerZ));
            }
        }
    }
    return positions;
}

scene::Aabb wallBox(const WallSegment& segment) {
    // The position is the base, the centre of the box is half of the height above it.
    const glm::vec3 center = segment.position + glm::vec3{0.0F, WALL_HEIGHT / 2.0F, 0.0F};
    const glm::vec3 halfExtents =
        segment.axis == WallAxis::AlongX ? WALL_ALONG_X_HALF_EXTENTS : WALL_ALONG_Z_HALF_EXTENTS;
    return scene::Aabb::fromCenter(center, halfExtents);
}

scene::Aabb pillarBox(const glm::vec3& position) {
    const glm::vec3 center = position + glm::vec3{0.0F, PILLAR_HEIGHT / 2.0F, 0.0F};
    return scene::Aabb::fromCenter(center, PILLAR_HALF_EXTENTS);
}

std::vector<scene::Aabb> colliderBoxes(std::span<const WallSegment> walls,
                                       std::span<const glm::vec3> pillars) {
    std::vector<scene::Aabb> boxes;
    boxes.reserve(walls.size() + pillars.size());
    for (const WallSegment& segment : walls) {
        boxes.push_back(wallBox(segment));
    }
    for (const glm::vec3& position : pillars) {
        boxes.push_back(pillarBox(position));
    }
    return boxes;
}

std::vector<scene::Aabb> mazeColliders(const Maze& maze) {
    return colliderBoxes(wallSegments(maze), pillarPositions(maze));
}

} // namespace game
