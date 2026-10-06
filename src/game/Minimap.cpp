// Minimap: the settings of the small map in a corner of the screen, where it stands, and
// the flat shapes it is drawn from (floors, walls, gate, crystals, levers, notes, player).
// See docs/modules/renderer/minimap.md
#include "game/Minimap.hpp"

#include "game/MazeLayout.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace game {

namespace {

// The square the map shows is this much larger than the longer side of the maze: 3 % of
// free space on each side. That is far more than half of a wall, which is all that
// reaches out of the maze, and it leaves a dark frame around the border walls.
constexpr float FRAME_SCALE = 1.06F;

// Thickness of a wall on the map, in metres: the thickness of a pillar in the world, so
// in a small maze the walls of the map have the proportions of the real ones.
constexpr float WALL_THICKNESS = PILLAR_SIZE;
// The thinnest a wall may get, in pixels of the picture. In a large maze 0.3 m is less
// than a pixel, and a rectangle thinner than one pixel can lie between the centres of
// two pixel rows and vanish. One that is 1.5 pixels thick always covers a centre.
constexpr float MIN_WALL_PIXELS = 1.5F;

// A crystal is a diamond that reaches this far from the middle of its cell, in metres:
// a little less than half of the free width of a corridor. And at least this many
// pixels, so it stays a visible dot.
constexpr float CRYSTAL_RADIUS = 0.4F;
constexpr float MIN_CRYSTAL_PIXELS = 2.0F;

// A lever or a note is a square with this half side, in metres, and at least this many
// pixels: smaller than a crystal, because it hangs on a wall and must not cover it.
constexpr float MOUNT_MARK_HALF_SIZE = 0.22F;
constexpr float MIN_MOUNT_MARK_PIXELS = 1.5F;

// The arrow of the player. Its tip is this far in front of the place of the player, in
// metres, and at least this many pixels, so the player can be found on the map of
// a maze of any size.
constexpr float PLAYER_ARROW_LENGTH = 0.9F;
constexpr float MIN_PLAYER_ARROW_PIXELS = 5.0F;
// The two back corners of the arrow, as parts of its length: how far behind the place
// of the player they lie and how far to each side.
constexpr float PLAYER_ARROW_BACK = 0.6F;
constexpr float PLAYER_ARROW_HALF_WIDTH = 0.65F;

// A rectangle is two triangles of three vertices, a diamond too.
constexpr std::size_t VERTICES_PER_QUAD = 6;

// The larger of a size in metres and a size in pixels, as metres.
float atLeastPixels(float metres, float pixels, float metresPerPixel) {
    return std::max(metres, pixels * metresPerPixel);
}

// Adds the triangle a, b, c in one colour.
void addTriangle(std::vector<MinimapVertex>& vertices, const glm::vec2& a, const glm::vec2& b,
                 const glm::vec2& c, const glm::vec3& color) {
    vertices.push_back({.position = a, .color = color});
    vertices.push_back({.position = b, .color = color});
    vertices.push_back({.position = c, .color = color});
}

// Adds a rectangle with sides along the axes: center is its middle and halfSize half of
// its width and of its height. It is cut into two triangles along one diagonal.
void addRectangle(std::vector<MinimapVertex>& vertices, const glm::vec2& center,
                  const glm::vec2& halfSize, const glm::vec3& color) {
    const glm::vec2 northWest{center.x - halfSize.x, center.y - halfSize.y};
    const glm::vec2 northEast{center.x + halfSize.x, center.y - halfSize.y};
    const glm::vec2 southEast{center.x + halfSize.x, center.y + halfSize.y};
    const glm::vec2 southWest{center.x - halfSize.x, center.y + halfSize.y};
    addTriangle(vertices, northWest, northEast, southEast, color);
    addTriangle(vertices, northWest, southEast, southWest, color);
}

// A place of the world as a place on the map: x stays, and z becomes the second number.
glm::vec2 mapPoint(const glm::vec3& worldPosition) {
    return {worldPosition.x, worldPosition.z};
}

// Adds the rectangle of a wall segment (or of the gate, which stands like one). It is
// half a thickness longer than the cell edge at both ends: two walls that meet at
// a grid corner then fill the corner between them, where the world has a pillar.
void addWall(std::vector<MinimapVertex>& vertices, const WallSegment& segment, float thickness,
             const glm::vec3& color) {
    const float halfLength = (WALL_LENGTH + thickness) / 2.0F;
    const float halfThickness = thickness / 2.0F;
    // A wall along X is long from west to east and thin from north to south. A wall
    // along Z is the same rectangle turned by a quarter.
    const glm::vec2 halfSize = segment.axis == WallAxis::AlongX
                                   ? glm::vec2{halfLength, halfThickness}
                                   : glm::vec2{halfThickness, halfLength};
    addRectangle(vertices, mapPoint(segment.position), halfSize, color);
}

} // namespace

MinimapRect minimapRect(int framebufferWidth, int framebufferHeight,
                        const MinimapSettings& settings) {
    if (framebufferWidth < 1 || framebufferHeight < 1) {
        return {};
    }

    // Both numbers are parts of the HEIGHT, also the margin to the left or the right
    // edge: the map is then a square with the same free space on both sides of its
    // corner. The settings come from sliders, where anything can be typed.
    const auto height = static_cast<float>(framebufferHeight);
    const float sizePart = std::clamp(settings.size, MIN_MINIMAP_SIZE, MAX_MINIMAP_SIZE);
    const float marginPart = std::clamp(settings.margin, MIN_MINIMAP_MARGIN, MAX_MINIMAP_MARGIN);
    int size = static_cast<int>(std::lround(height * sizePart));
    int margin = static_cast<int>(std::lround(height * marginPart));

    // The longest side that fits into the framebuffer with a margin on both sides. In
    // a window so small that the margins alone fill it, the margin is given up.
    const int shorterSide = std::min(framebufferWidth, framebufferHeight);
    if (shorterSide - 2 * margin < 1) {
        margin = 0;
    }
    size = std::clamp(size, 1, shorterSide - 2 * margin);

    // The corner. OpenGL counts y from the BOTTOM edge of the framebuffer, so a top
    // corner is the one with the large y.
    const bool left =
        settings.corner == MinimapCorner::TopLeft || settings.corner == MinimapCorner::BottomLeft;
    const bool bottom = settings.corner == MinimapCorner::BottomLeft ||
                        settings.corner == MinimapCorner::BottomRight;
    return {.x = left ? margin : framebufferWidth - margin - size,
            .y = bottom ? margin : framebufferHeight - margin - size,
            .size = size};
}

float minimapHalfExtent(const Maze& maze) {
    // The longer side of the maze in metres decides: the whole maze has to fit.
    const float longerSide = static_cast<float>(std::max(maze.width(), maze.height())) * CELL_SIZE;
    return longerSide / 2.0F * FRAME_SCALE;
}

glm::mat4 minimapProjection(const Maze& maze) {
    // The middle of the maze in metres: it covers x from 0 to width * CELL_SIZE and
    // z from 0 to height * CELL_SIZE.
    const float centerX = static_cast<float>(maze.width()) * CELL_SIZE / 2.0F;
    const float centerZ = static_cast<float>(maze.height()) * CELL_SIZE / 2.0F;
    const float halfExtent = minimapHalfExtent(maze);

    // glm::ortho(left, right, bottom, top) maps the first two coordinates of a point:
    // left to -1 and right to +1, bottom to -1 and top to +1. The second coordinate of
    // a MinimapVertex is the world z, which grows towards the south. The south edge is
    // given as "bottom" and the north edge as "top", so north ends up at the top of
    // the picture.
    const float west = centerX - halfExtent;
    const float east = centerX + halfExtent;
    const float south = centerZ + halfExtent;
    const float north = centerZ - halfExtent;
    return glm::ortho(west, east, south, north);
}

float minimapMetresPerPixel(const Maze& maze, int pixels) {
    // The picture shows a square with a side of two half extents.
    return 2.0F * minimapHalfExtent(maze) / static_cast<float>(std::max(pixels, 1));
}

std::vector<MinimapVertex> buildMinimapVertices(const MazeWorld& world, const Round& round,
                                                bool revealAll, const MinimapPlayer& player,
                                                float metresPerPixel) {
    // The walls of the ROUND: the maze of the world without the walls that pulled
    // levers have opened.
    const Maze& maze = roundMaze(world, round);
    const Discovery& discovery = round.discovery;

    // Whether a cell is drawn. isDiscovered is false for a cell outside the grid, so
    // the neighbour of a border cell can be asked without a check.
    const auto shown = [revealAll, &discovery](int x, int z) {
        return revealAll || discovery.isDiscovered(x, z);
    };

    std::vector<MinimapVertex> vertices;
    // Room for the floors of the cells that are shown, which is most of the list in
    // a maze that is explored. The vector grows by itself for the rest.
    const std::size_t shownCells =
        revealAll ? static_cast<std::size_t>(maze.width()) * static_cast<std::size_t>(maze.height())
                  : static_cast<std::size_t>(discovery.count());
    vertices.reserve(shownCells * VERTICES_PER_QUAD);

    // 1. The floors. A cell is a square of CELL_SIZE around its centre.
    const MazeCell startCell = cellAt(world.startPosition);
    constexpr glm::vec2 HALF_CELL{CELL_SIZE / 2.0F, CELL_SIZE / 2.0F};
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            if (!shown(x, z)) {
                continue;
            }
            const MazeCell cell{.x = x, .z = z};
            // The exit is asked first: in a maze of one cell the start is the exit.
            glm::vec3 color = MINIMAP_FLOOR_COLOR;
            if (cell == world.exitCell) {
                color = MINIMAP_EXIT_COLOR;
            } else if (cell == startCell) {
                color = MINIMAP_START_COLOR;
            }
            addRectangle(vertices, mapPoint(cellCenter(x, z)), HALF_CELL, color);
        }
    }

    // 2. The walls, each one once, in the order of game::wallSegments: every cell
    // reports its north and its west wall, and only the last row and the last column
    // report the south and the east one. A wall between two cells is drawn when either
    // of them is shown: the player has seen it from one side.
    const float wallThickness = atLeastPixels(WALL_THICKNESS, MIN_WALL_PIXELS, metresPerPixel);
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            if (maze.hasWall(x, z, Direction::North) && (shown(x, z) || shown(x, z - 1))) {
                addWall(vertices, wallSegmentOn(x, z, Direction::North), wallThickness,
                        MINIMAP_WALL_COLOR);
            }
            if (maze.hasWall(x, z, Direction::West) && (shown(x, z) || shown(x - 1, z))) {
                addWall(vertices, wallSegmentOn(x, z, Direction::West), wallThickness,
                        MINIMAP_WALL_COLOR);
            }
            // The border walls in the south and in the east have no cell behind them.
            if (z == maze.height() - 1 && maze.hasWall(x, z, Direction::South) && shown(x, z)) {
                addWall(vertices, wallSegmentOn(x, z, Direction::South), wallThickness,
                        MINIMAP_WALL_COLOR);
            }
            if (x == maze.width() - 1 && maze.hasWall(x, z, Direction::East) && shown(x, z)) {
                addWall(vertices, wallSegmentOn(x, z, Direction::East), wallThickness,
                        MINIMAP_WALL_COLOR);
            }
        }
    }

    // 3. The gate, together with the exit cell it closes. A maze of one cell has none.
    if (world.hasGate && shown(world.exitCell.x, world.exitCell.z)) {
        addWall(vertices, world.gate, wallThickness,
                gateBlocks(world, round) ? MINIMAP_GATE_COLOR : MINIMAP_GATE_OPEN_COLOR);
    }

    // 4. The crystals that are still there. The crystals of a round are in the order
    // of MazeWorld::crystals, which knows their cells. The smaller of the two sizes
    // guards against a round that belongs to another world.
    const float crystalRadius = atLeastPixels(CRYSTAL_RADIUS, MIN_CRYSTAL_PIXELS, metresPerPixel);
    const std::size_t crystalCount = std::min(round.crystals.size(), world.crystals.size());
    for (std::size_t i = 0; i < crystalCount; ++i) {
        const MazeCell cell = world.crystals[i].cell;
        if (round.crystals[i].collected || !shown(cell.x, cell.z)) {
            continue;
        }
        // A diamond: a square standing on a corner, with its four corners to the
        // north, the east, the south and the west of the middle of the cell.
        const glm::vec2 center = mapPoint(cellCenter(cell.x, cell.z));
        const glm::vec2 north{center.x, center.y - crystalRadius};
        const glm::vec2 east{center.x + crystalRadius, center.y};
        const glm::vec2 south{center.x, center.y + crystalRadius};
        const glm::vec2 west{center.x - crystalRadius, center.y};
        addTriangle(vertices, north, east, south, MINIMAP_CRYSTAL_COLOR);
        addTriangle(vertices, north, south, west, MINIMAP_CRYSTAL_COLOR);
    }

    // 5. The levers and the notes, where they hang. A lever shows whether it is pulled.
    const float markHalfSize =
        atLeastPixels(MOUNT_MARK_HALF_SIZE, MIN_MOUNT_MARK_PIXELS, metresPerPixel);
    const glm::vec2 markHalf{markHalfSize, markHalfSize};
    const std::vector<Lever>& levers = world.interactables.levers;
    for (std::size_t i = 0; i < levers.size(); ++i) {
        const MazeCell cell = levers[i].mount.cell;
        if (!shown(cell.x, cell.z)) {
            continue;
        }
        // A lever the round does not know (a round of another world) counts as not
        // pulled.
        const bool pulled =
            i < round.interactables.leverPulled.size() && round.interactables.leverPulled[i];
        addRectangle(vertices, mapPoint(levers[i].position), markHalf,
                     pulled ? MINIMAP_LEVER_PULLED_COLOR : MINIMAP_LEVER_COLOR);
    }
    for (const Note& note : world.interactables.notes) {
        if (shown(note.mount.cell.x, note.mount.cell.z)) {
            addRectangle(vertices, mapPoint(note.position), markHalf, MINIMAP_NOTE_COLOR);
        }
    }

    // 6. The player, last, so nothing covers it. Yaw 0 looks north, which is towards
    // smaller z, and the yaw grows clockwise seen from above. So "forward" on the map
    // is (sin yaw, -cos yaw), the x and z of scene::Camera::forward for a level look,
    // and "to the right" is that direction turned by a quarter: (cos yaw, sin yaw).
    const float yaw = glm::radians(player.yawDegrees);
    const glm::vec2 forward{std::sin(yaw), -std::cos(yaw)};
    const glm::vec2 right{std::cos(yaw), std::sin(yaw)};
    const float length =
        atLeastPixels(PLAYER_ARROW_LENGTH, MIN_PLAYER_ARROW_PIXELS, metresPerPixel);
    const glm::vec2 place = mapPoint(player.position);
    const glm::vec2 tip = place + forward * length;
    const glm::vec2 back = place - forward * (length * PLAYER_ARROW_BACK);
    const glm::vec2 sideways = right * (length * PLAYER_ARROW_HALF_WIDTH);
    addTriangle(vertices, tip, back + sideways, back - sideways, MINIMAP_PLAYER_COLOR);

    return vertices;
}

} // namespace game
