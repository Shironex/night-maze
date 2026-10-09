// Minimap: the settings of the map in the middle of the screen, where it stands, and the
// flat shapes it is drawn from (floors, walls, gate, lantern, crystals, flasks, levers,
// notes, the tick towards the open gate, player).
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

// A flask is a plus sign: two bars that cross in the middle of its cell. Each reaches
// this far from the middle, in metres, and is this thick. And at least these many
// pixels, so the sign stays a sign on the map of a large maze.
constexpr float FLASK_MARK_REACH = 0.36F;
constexpr float FLASK_MARK_HALF_THICKNESS = 0.12F;
constexpr float MIN_FLASK_REACH_PIXELS = 3.0F;
constexpr float MIN_FLASK_HALF_THICKNESS_PIXELS = 1.0F;

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

// The lantern mark in the middle of the exit cell, in metres: half of the width and of
// the height of its glass, how much wider its cap and its foot are on each side, how
// high the cap is and how thick the foot. And the least half width of the glass in
// pixels: the other sizes grow with it, so the mark keeps its shape in a large maze.
constexpr float LANTERN_GLASS_HALF_WIDTH = 0.26F;
constexpr float LANTERN_GLASS_HALF_HEIGHT = 0.3F;
constexpr float LANTERN_OVERHANG = 0.12F;
constexpr float LANTERN_CAP_HEIGHT = 0.3F;
constexpr float LANTERN_FOOT_HEIGHT = 0.1F;
constexpr float MIN_LANTERN_PIXELS = 2.0F;

// The tick towards the open gate: a triangle with its tip on the edge of the map. Its
// length in metres and at least in pixels, and half of its width as a part of the
// length.
constexpr float EXIT_TICK_LENGTH = 1.0F;
constexpr float MIN_EXIT_TICK_PIXELS = 10.0F;
constexpr float EXIT_TICK_HALF_WIDTH = 0.45F;

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

// Adds the lantern mark around center: a glass in the given colour, a pointed cap above
// it and a flat foot below. Above is north on the map, which is the smaller second
// coordinate. scale makes the whole mark larger and keeps its proportions.
void addLantern(std::vector<MinimapVertex>& vertices, const glm::vec2& center, float scale,
                const glm::vec3& glassColor) {
    const float halfWidth = LANTERN_GLASS_HALF_WIDTH * scale;
    const float halfHeight = LANTERN_GLASS_HALF_HEIGHT * scale;
    const float overhang = LANTERN_OVERHANG * scale;
    const float top = center.y - halfHeight;
    const float bottom = center.y + halfHeight;

    addRectangle(vertices, center, {halfWidth, halfHeight}, glassColor);
    addTriangle(vertices, {center.x, top - LANTERN_CAP_HEIGHT * scale},
                {center.x + halfWidth + overhang, top}, {center.x - halfWidth - overhang, top},
                MINIMAP_LANTERN_FRAME_COLOR);
    const float footHalfHeight = LANTERN_FOOT_HEIGHT * scale / 2.0F;
    addRectangle(vertices, {center.x, bottom + footHalfHeight},
                 {halfWidth + overhang, footHalfHeight}, MINIMAP_LANTERN_FRAME_COLOR);
}

} // namespace

std::optional<glm::vec2> minimapExitTick(const Maze& maze, const glm::vec2& player,
                                         const glm::vec2& exit) {
    const glm::vec2 direction = exit - player;
    if (direction.x == 0.0F && direction.y == 0.0F) {
        return std::nullopt;
    }

    // The square of the map: half an extent to every side of the middle of the maze.
    const glm::vec2 center{static_cast<float>(maze.width()) * CELL_SIZE / 2.0F,
                           static_cast<float>(maze.height()) * CELL_SIZE / 2.0F};
    const float halfExtent = minimapHalfExtent(maze);
    if (std::abs(player.x - center.x) > halfExtent || std::abs(player.y - center.y) > halfExtent) {
        return std::nullopt;
    }

    // A point of the line is player + t * direction. Along each of the two axes the
    // line moves towards one edge of the square, and t tells when it gets there. The
    // smaller of the two answers is the edge it reaches first: that is where it leaves
    // the square. An axis the line does not move along never reaches its edge.
    float leaves = -1.0F;
    for (int axis = 0; axis < 2; ++axis) {
        if (direction[axis] == 0.0F) {
            continue;
        }
        const float edge = center[axis] + (direction[axis] > 0.0F ? halfExtent : -halfExtent);
        const float t = (edge - player[axis]) / direction[axis];
        if (leaves < 0.0F || t < leaves) {
            leaves = t;
        }
    }
    return player + direction * leaves;
}

MinimapRect minimapRect(int framebufferWidth, int framebufferHeight,
                        const MinimapSettings& settings) {
    if (framebufferWidth < 1 || framebufferHeight < 1) {
        return {};
    }

    // The side is a part of the HEIGHT. The setting comes from a slider, where anything
    // can be typed, so it is brought into its limits first. A window that is narrower
    // than the map is high gets a map as wide as the window.
    const float sizePart = std::clamp(settings.size, MIN_MINIMAP_SIZE, MAX_MINIMAP_SIZE);
    const int wanted =
        static_cast<int>(std::lround(static_cast<float>(framebufferHeight) * sizePart));
    const int size = std::clamp(wanted, 1, std::min(framebufferWidth, framebufferHeight));

    // The middle: the free pixels are shared between the two sides. Whole number
    // division rounds down, so with an odd number the extra pixel is on the right and
    // at the top.
    return {.x = (framebufferWidth - size) / 2, .y = (framebufferHeight - size) / 2, .size = size};
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
                                                float metresPerPixel, bool showShade) {
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

    // The lantern in the middle of the exit cell, drawn with the gate: cold while the
    // gate blocks the way, amber once it has opened.
    if (world.hasGate && shown(world.exitCell.x, world.exitCell.z)) {
        const float scale =
            atLeastPixels(LANTERN_GLASS_HALF_WIDTH, MIN_LANTERN_PIXELS, metresPerPixel) /
            LANTERN_GLASS_HALF_WIDTH;
        addLantern(vertices, mapPoint(cellCenter(world.exitCell.x, world.exitCell.z)), scale,
                   gateBlocks(world, round) ? MINIMAP_LANTERN_COLD_COLOR
                                            : MINIMAP_LANTERN_LIT_COLOR);
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

    // 5. The flasks that are still there, by the rule of the crystals: not picked up,
    // and the cell is shown. A plus sign is a bar from west to east and a bar from
    // north to south, both through the middle of the cell.
    const float flaskReach =
        atLeastPixels(FLASK_MARK_REACH, MIN_FLASK_REACH_PIXELS, metresPerPixel);
    const float flaskHalfThickness =
        atLeastPixels(FLASK_MARK_HALF_THICKNESS, MIN_FLASK_HALF_THICKNESS_PIXELS, metresPerPixel);
    for (const RoundFlask& flask : round.flasks) {
        if (flask.collected || !shown(flask.cell.x, flask.cell.z)) {
            continue;
        }
        const glm::vec2 center = mapPoint(cellCenter(flask.cell.x, flask.cell.z));
        addRectangle(vertices, center, {flaskReach, flaskHalfThickness}, MINIMAP_FLASK_COLOR);
        addRectangle(vertices, center, {flaskHalfThickness, flaskReach}, MINIMAP_FLASK_COLOR);
    }

    // 6. The levers and the notes, where they hang. A lever shows whether it is pulled.
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

    // The shade, only for the debug switch: a square as large as a crystal, where it is.
    if (showShade && round.shade.present) {
        const float halfSize = atLeastPixels(CRYSTAL_RADIUS, MIN_CRYSTAL_PIXELS, metresPerPixel);
        addRectangle(vertices, mapPoint(round.shade.position), {halfSize, halfSize},
                     MINIMAP_SHADE_COLOR);
    }

    // 7. The tick towards the open gate, on the edge of the map: a triangle whose tip
    // touches the edge and points along the line from the player to the exit. Only while
    // the exit cell is not on the map: once its lantern is there, the lantern says it.
    const glm::vec2 place = mapPoint(player.position);
    if (world.hasGate && round.gateOpen && !shown(world.exitCell.x, world.exitCell.z)) {
        const glm::vec2 exit = mapPoint(cellCenter(world.exitCell.x, world.exitCell.z));
        if (const std::optional<glm::vec2> edgePoint = minimapExitTick(maze, place, exit)) {
            const glm::vec2 towards = glm::normalize(exit - place);
            const glm::vec2 across{-towards.y, towards.x};
            const float tickLength =
                atLeastPixels(EXIT_TICK_LENGTH, MIN_EXIT_TICK_PIXELS, metresPerPixel);
            const glm::vec2 tickBack = *edgePoint - towards * tickLength;
            const glm::vec2 tickSide = across * (tickLength * EXIT_TICK_HALF_WIDTH);
            addTriangle(vertices, *edgePoint, tickBack + tickSide, tickBack - tickSide,
                        MINIMAP_EXIT_TICK_COLOR);
        }
    }

    // 8. The player, last, so nothing covers it. Yaw 0 looks north, which is towards
    // smaller z, and the yaw grows clockwise seen from above. So "forward" on the map
    // is (sin yaw, -cos yaw), the x and z of scene::Camera::forward for a level look,
    // and "to the right" is that direction turned by a quarter: (cos yaw, sin yaw).
    const float yaw = glm::radians(player.yawDegrees);
    const glm::vec2 forward{std::sin(yaw), -std::cos(yaw)};
    const glm::vec2 right{std::cos(yaw), std::sin(yaw)};
    const float length =
        atLeastPixels(PLAYER_ARROW_LENGTH, MIN_PLAYER_ARROW_PIXELS, metresPerPixel);
    const glm::vec2 tip = place + forward * length;
    const glm::vec2 back = place - forward * (length * PLAYER_ARROW_BACK);
    const glm::vec2 sideways = right * (length * PLAYER_ARROW_HALF_WIDTH);
    addTriangle(vertices, tip, back + sideways, back - sideways, MINIMAP_PLAYER_COLOR);

    return vertices;
}

} // namespace game
