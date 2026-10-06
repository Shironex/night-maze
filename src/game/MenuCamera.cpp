// MenuCamera: a camera that travels through the maze by itself, the picture behind the menu.
// See docs/modules/game/menu-camera.md
#include "game/MenuCamera.hpp"

#include "game/Exit.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Camera.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace game {

namespace {

// ---- the corridor walk ----------------------------------------------------------------

// The camera of the corridor walk looks at the farthest place of its path it can see,
// up to this many metres along the path: four cells. So it looks down a corridor as
// far as the corridor goes, and around a corner as soon as the way behind it shows.
constexpr float SIGHT_REACH_METRES = 8.0F;

// The places of the path that are tried as the place to look at lie this far apart, in
// metres along the path.
constexpr float SIGHT_CANDIDATE_METRES = 0.5F;

// The line of sight to such a place is checked at points this far apart, in metres.
// Far less than a cell, so no cell between the camera and the place is skipped.
constexpr float SIGHT_STEP_METRES = 0.2F;

// A place is only looked at while the path has not turned by more than this on the way
// to it, in degrees. Where the path turns around, the way back runs beside the way
// out and can be seen from it. Without this limit the camera would look over its
// shoulder at the way back while it still walks towards the end of the corridor.
constexpr float MAX_SIGHT_TURN_DEGREES = 135.0F;

// The yaw towards the place looked at jumps when a new corridor comes into sight. It
// is therefore averaged over this many metres of the path: the view takes that long to
// turn. A turn around, half a turn, so takes about 6 seconds at the default speed.
// Longer: every turn is slower, but the view stays on a wall for longer before it has
// found the new corridor.
constexpr float LOOK_WINDOW_METRES = 4.5F;

// The averaged yaw is averaged a second time over this many metres. The first average
// alone starts and stops a turn at once. The second one lets it begin and end softly.
constexpr float LOOK_EASE_METRES = 1.5F;

// The camera has the averaged yaw of the place this many metres behind it on the path.
// An average reaches as far forwards as backwards, so without this the view would
// start to turn metres before the new corridor shows, towards the wall that still
// hides it. With it the view starts to turn about when the corridor comes into sight,
// like the head of somebody who walks there.
constexpr float LOOK_LAG_METRES = 1.5F;

// The view is tilted up by this much, in degrees: the tops of the walls and the stars
// above them are part of the picture, and less of it is the ground right in front.
constexpr float LOOK_UP_DEGREES = 4.0F;

// The slope of the view follows the ground between the place this many metres behind
// the camera and the place as far ahead of it.
constexpr float SLOPE_REACH_METRES = 2.0F;

// The windows and reaches above are never longer than this share of the whole loop. It
// only matters for a very short loop (a maze of one or two cells), where a window of
// their full length would reach around the whole loop.
constexpr float MAX_WINDOW_SHARE = 0.25F;

// The sway: the view drifts a little to the left and right and up and down, like the
// head of somebody who walks slowly. One swing to both sides takes about this many
// metres of the path (11 seconds at the default speed). The real number is chosen so
// that a whole number of swings fits into the loop: the sway then has no jump where the
// loop closes.
constexpr float SWAY_METRES = 9.0F;

// How far the sway turns the view to each side and tilts it up and down, in degrees.
constexpr float SWAY_YAW_DEGREES = 2.5F;
constexpr float SWAY_PITCH_DEGREES = 1.0F;

// The view tilts up and down twice while it drifts left and right once, so the sway
// draws a lying figure of eight and not a line.
constexpr float SWAY_PITCH_SWINGS = 2.0F;

// A length below this counts as no length, in metres: nothing is divided by it.
constexpr float MIN_LENGTH = 0.0001F;

// A quarter of a full turn, in radians: every corner of the walk is a right angle,
// because the corridors run along the two axes of the grid.
constexpr float QUARTER_TURN = glm::half_pi<float>();

// One full turn in degrees and in radians.
constexpr float FULL_TURN_DEGREES = 360.0F;
constexpr float FULL_TURN = glm::two_pi<float>();

// ---- the high glide -------------------------------------------------------------------

// The circle of the high glide. Its radius is this share of the distance from the
// middle of the maze to one of its corners, so the camera stays above the maze or just
// outside of it and the far corner is still in the picture.
constexpr float GLIDE_RADIUS_SHARE = 0.9F;

// The circle is never smaller than this, in metres: above a maze of one cell the camera
// would otherwise turn on the spot.
constexpr float MIN_GLIDE_RADIUS = 6.0F;

// How high the camera of the glide flies above the ground in the middle of the maze:
// the walls, and on top this share of the radius of the circle. The view then goes
// down at about 30 degrees and reaches the ground of the corridors on the far side,
// between the walls.
constexpr float GLIDE_HEIGHT_SHARE = 0.38F;

// The point the glide looks at lies this far above the ground in the middle of the
// maze, in metres: half of the height of the walls.
constexpr float GLIDE_TARGET_HEIGHT = WALL_HEIGHT / 2.0F;

// The glide is this many times faster than the walk at the same speed setting. From
// far away a movement looks slower, so the same metres per second would seem to stand
// still.
constexpr float GLIDE_SPEED_SCALE = 2.5F;

// ---- cells and directions -------------------------------------------------------------

// Position of a cell in a list that holds the rows one after another, like in Maze.
std::size_t cellIndex(const Maze& maze, MazeCell cell) {
    return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
           static_cast<std::size_t>(cell.x);
}

// The cell one step away in a direction. It may lie outside of the maze.
MazeCell neighbourOf(MazeCell cell, Direction direction) {
    return {.x = cell.x + columnStep(direction), .z = cell.z + rowStep(direction)};
}

// The direction after a number of quarter turns to the right. The enum lists the
// directions clockwise (North, East, South, West), so a turn to the right is the next
// entry, and three turns to the right are one turn to the left.
Direction turnedRight(Direction heading, int quarterTurns) {
    const int index = (static_cast<int>(heading) + quarterTurns) % DIRECTION_COUNT;
    return ALL_DIRECTIONS[static_cast<std::size_t>(index)];
}

// The direction of the step from one cell to a neighbour.
Direction directionBetween(MazeCell from, MazeCell to) {
    for (const Direction direction : ALL_DIRECTIONS) {
        if (neighbourOf(from, direction) == to) {
            return direction;
        }
    }
    // Cannot happen: menuCameraRoute only joins neighbours.
    throw std::logic_error("menu camera: two cells of the route are not neighbours");
}

// One step in a direction as a vector on the ground: x grows to the east and the second
// number, the world z, to the south.
glm::vec2 stepVector(Direction direction) {
    return {static_cast<float>(columnStep(direction)), static_cast<float>(rowStep(direction))};
}

// The vector that points to the right of a vector on the ground, seen from above: east
// for north, south for east.
glm::vec2 rightOf(const glm::vec2& forward) {
    return {-forward.y, forward.x};
}

// ---- the tree of the route ------------------------------------------------------------

// What the route needs to know about one cell.
struct TreeCell {
    // Whether the cell lies on a shortest way from the start to a target.
    bool onTree = false;
    // The side on which the cell before it on that way lies. The start has none.
    bool hasParent = false;
    Direction parentSide = Direction::North;
};

// The first side of a cell, in the order of ALL_DIRECTIONS, whose neighbour is one
// passage nearer to the start. Every cell that can be reached, except the start, has one.
Direction sideTowardsStart(const Maze& maze, const std::vector<int>& distances, MazeCell cell) {
    const int distance = distances[cellIndex(maze, cell)];
    for (const Direction side : ALL_DIRECTIONS) {
        if (maze.hasWall(cell.x, cell.z, side)) {
            continue;
        }
        const MazeCell neighbour = neighbourOf(cell, side);
        if (maze.contains(neighbour.x, neighbour.z) &&
            distances[cellIndex(maze, neighbour)] == distance - 1) {
            return side;
        }
    }
    // Cannot happen: passageDistances gave the cell its distance through such a side.
    throw std::logic_error("menu camera: a reached cell has no way back to the start");
}

// True when the step from a cell in a direction follows a passage of the tree: the cell
// on the other side is the parent of this one, or this one is the parent of it. A maze
// with more than one way between two cells (one built by hand) has open passages that
// are not part of the tree, and the route must not use them: it would no longer close.
bool isTreePassage(const Maze& maze, const std::vector<TreeCell>& tree, MazeCell cell,
                   Direction direction) {
    const MazeCell neighbour = neighbourOf(cell, direction);
    if (!maze.contains(neighbour.x, neighbour.z)) {
        return false;
    }
    const TreeCell& here = tree[cellIndex(maze, cell)];
    const TreeCell& there = tree[cellIndex(maze, neighbour)];
    if (!here.onTree || !there.onTree) {
        return false;
    }
    const bool towardsParent = here.hasParent && here.parentSide == direction;
    const bool towardsChild = there.hasParent && there.parentSide == opposite(direction);
    return towardsParent || towardsChild;
}

// ---- from the route to a rounded line -------------------------------------------------

// The corners of the lane the camera follows, as points on the ground (x and world z):
// a closed line of straight pieces that all run along one of the two axes.
//
// The lane lies MENU_CAMERA_LANE_OFFSET to the right of the middle of the corridor, seen
// in the direction of travel. A corner of the route is the point where the lane before
// it and the lane after it cross. Where the route turns around, the lane stops
// MENU_CAMERA_TURN_SHORT before the middle of the cell, crosses to the other side and
// comes back: two corners, both to the left. A cell the route passes straight through
// adds no corner.
std::vector<glm::vec2> laneCorners(const std::vector<MazeCell>& route) {
    const float offset = MENU_CAMERA_LANE_OFFSET;
    std::vector<glm::vec2> corners;

    // A route of one cell has no direction of travel. The lane is then a square around
    // the middle of the cell, walked clockwise seen from above: its north-west,
    // north-east, south-east and south-west corner. Rounded, it is a circle.
    if (route.size() == 1) {
        const glm::vec3 center = cellCenter(route.front().x, route.front().z);
        const glm::vec2 middle{center.x, center.z};
        corners.push_back(middle + glm::vec2{-offset, -offset});
        corners.push_back(middle + glm::vec2{offset, -offset});
        corners.push_back(middle + glm::vec2{offset, offset});
        corners.push_back(middle + glm::vec2{-offset, offset});
        return corners;
    }

    const std::size_t count = route.size();
    for (std::size_t i = 0; i < count; ++i) {
        // The route is closed: the cell before the first one is the last one.
        const MazeCell before = route[(i + count - 1) % count];
        const MazeCell cell = route[i];
        const MazeCell after = route[(i + 1) % count];

        const Direction headingIn = directionBetween(before, cell);
        const Direction headingOut = directionBetween(cell, after);
        if (headingOut == headingIn) {
            continue;
        }

        const glm::vec3 center = cellCenter(cell.x, cell.z);
        const glm::vec2 middle{center.x, center.z};
        const glm::vec2 forwardIn = stepVector(headingIn);
        const glm::vec2 rightIn = rightOf(forwardIn);
        const glm::vec2 rightOut = rightOf(stepVector(headingOut));

        if (headingOut == opposite(headingIn)) {
            // Turning around: on the lane up to the place short of the middle of the
            // cell, then across to the lane that leads back.
            const glm::vec2 turnPlace = middle - MENU_CAMERA_TURN_SHORT * forwardIn;
            corners.push_back(turnPlace + offset * rightIn);
            corners.push_back(turnPlace + offset * rightOut);
        } else {
            // A turn to the left or to the right: where the two lanes cross. A turn to
            // the right is cut short, before the middle of the cell, and a turn to the
            // left goes wide around it.
            corners.push_back(middle + offset * (rightIn + rightOut));
        }
    }
    return corners;
}

// One rounded corner: the arc that replaces its tip.
struct CornerArc {
    // Where the arc leaves the straight piece before the corner and where it joins the
    // one after it.
    glm::vec2 start{0.0F};
    glm::vec2 end{0.0F};
    // The middle of the circle the arc is a part of, and its radius in metres.
    glm::vec2 center{0.0F};
    float radius = 0.0F;
    // 1 for a turn to the right, -1 for a turn to the left.
    float turnSign = 1.0F;
};

// The arc of the corner at tip, between the corners before and after it. Every corner
// is a right angle, so the arc is a quarter of a circle and starts as far before the
// tip as its radius is long. The radius is MENU_CAMERA_CORNER_RADIUS, or less when
// a neighbouring corner is so near that the two arcs would overlap: each arc may use
// half of the straight piece between them.
CornerArc cornerArc(const glm::vec2& before, const glm::vec2& tip, const glm::vec2& after) {
    const float lengthIn = glm::length(tip - before);
    const float lengthOut = glm::length(after - tip);

    CornerArc arc;
    arc.radius = std::min({MENU_CAMERA_CORNER_RADIUS, lengthIn / 2.0F, lengthOut / 2.0F});
    // Two corners in the same place: nothing to round, the arc is the tip itself.
    if (lengthIn < MIN_LENGTH || lengthOut < MIN_LENGTH) {
        arc.radius = 0.0F;
        arc.start = tip;
        arc.end = tip;
        arc.center = tip;
        return arc;
    }

    const glm::vec2 forwardIn = (tip - before) / lengthIn;
    const glm::vec2 forwardOut = (after - tip) / lengthOut;
    arc.start = tip - forwardIn * arc.radius;
    arc.end = tip + forwardOut * arc.radius;

    // The way out points to the right of the way in for a turn to the right: their dot
    // product is then positive.
    const glm::vec2 rightIn = rightOf(forwardIn);
    arc.turnSign = glm::dot(rightIn, forwardOut) >= 0.0F ? 1.0F : -1.0F;
    // The middle of the circle lies beside the start of the arc, on the side the corner
    // turns to.
    arc.center = arc.start + rightIn * (arc.turnSign * arc.radius);
    return arc;
}

// Adds the points of the straight piece from one point to another, MENU_CAMERA_SAMPLE_SPACING
// apart or a little less. The first point is added, the last one is not: the piece that
// follows starts with it.
void appendLine(std::vector<glm::vec2>& points, const glm::vec2& from, const glm::vec2& to) {
    const float length = glm::length(to - from);
    if (length < MIN_LENGTH) {
        return;
    }
    const int count = static_cast<int>(std::ceil(length / MENU_CAMERA_SAMPLE_SPACING));
    for (int i = 0; i < count; ++i) {
        const float share = static_cast<float>(i) / static_cast<float>(count);
        points.push_back(glm::mix(from, to, share));
    }
}

// Adds the points of a corner arc, the same way: with its first point and without its
// last one.
void appendArc(std::vector<glm::vec2>& points, const CornerArc& arc) {
    const float length = arc.radius * QUARTER_TURN;
    if (length < MIN_LENGTH) {
        return;
    }
    const int count = static_cast<int>(std::ceil(length / MENU_CAMERA_SAMPLE_SPACING));
    // The arm from the middle of the circle to the start of the arc is turned step by
    // step. On the ground, with x to the east and the second number to the south,
    // a turn to the right by an angle takes (x, z) to
    // (x cos - z sin, x sin + z cos): north (0, -1) becomes east (1, 0) after a quarter.
    const glm::vec2 arm = arc.start - arc.center;
    for (int i = 0; i < count; ++i) {
        const float share = static_cast<float>(i) / static_cast<float>(count);
        const float angle = arc.turnSign * QUARTER_TURN * share;
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);
        points.push_back(arc.center +
                         glm::vec2{arm.x * cosine - arm.y * sine, arm.x * sine + arm.y * cosine});
    }
}

// The closed line through the corners with every corner rounded, as points on the
// ground that lie close to each other.
std::vector<glm::vec2> roundedLoop(const std::vector<glm::vec2>& corners) {
    const std::size_t count = corners.size();
    std::vector<CornerArc> arcs;
    arcs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        arcs.push_back(
            cornerArc(corners[(i + count - 1) % count], corners[i], corners[(i + 1) % count]));
    }

    // Around the loop: the straight piece that leads to a corner, then its arc.
    std::vector<glm::vec2> points;
    for (std::size_t i = 0; i < count; ++i) {
        const CornerArc& previous = arcs[(i + count - 1) % count];
        appendLine(points, previous.end, arcs[i].start);
        appendArc(points, arcs[i]);
    }
    return points;
}

// ---- the view along the path ----------------------------------------------------------

// The yaw of a step on the ground, in degrees as in scene::Camera: 0 for a step to the
// north (-Z), 90 for a step to the east (+X).
float yawOfStep(const glm::vec3& step) {
    return glm::degrees(std::atan2(step.x, -step.z));
}

// The mean of every value and its neighbours, reach entries to each side, around
// a closed list: the entry after the last one is the first one again. The values run
// on: one time around the list they have grown by turn, so a neighbour that is found
// by going around the end is taken with turn added or taken away.
std::vector<float> averagedAround(const std::vector<float>& values, float turn, std::size_t reach) {
    // Signed numbers: a neighbour before the first entry has a negative position.
    const auto count = static_cast<std::ptrdiff_t>(values.size());
    const auto signedReach = static_cast<std::ptrdiff_t>(reach);

    std::vector<float> averaged(values.size());
    for (std::ptrdiff_t i = 0; i < count; ++i) {
        float sum = 0.0F;
        for (std::ptrdiff_t position = i - signedReach; position <= i + signedReach; ++position) {
            // How many times the way from the list to this position passes its end:
            // 0 inside the list, 1 past the end, -1 before the beginning. floor rounds
            // down also for a negative number, which a division of whole numbers does
            // not.
            const float laps = std::floor(static_cast<float>(position) / static_cast<float>(count));
            const std::ptrdiff_t index = position - static_cast<std::ptrdiff_t>(laps) * count;
            sum += values[static_cast<std::size_t>(index)] + laps * turn;
        }
        averaged[static_cast<std::size_t>(i)] = sum / static_cast<float>(reach * 2 + 1);
    }
    return averaged;
}

// How many points of the path lie within metres of path, at the spacing the path was
// built with, but never more than the share of the loop the windows may cover.
std::size_t pointsWithin(const MenuCameraPath& path, float metres) {
    const float limited = std::min(metres, path.length * MAX_WINDOW_SHARE);
    return static_cast<std::size_t>(limited / MENU_CAMERA_SAMPLE_SPACING);
}

// Writes angles so that they run on: each one becomes the one before it plus the turn
// between the two, taken the short way round (never more than half a turn). atan2 gives
// angles between -180 and 180, so a view that turns through south would otherwise jump
// by a full turn. The list is closed: the function returns how much the angles have
// changed once around it, back to the first entry. That is a whole number of turns.
float runOn(std::vector<float>& angles) {
    const std::size_t count = angles.size();
    const std::vector<float> raw = angles;
    float turnSum = 0.0F;
    for (std::size_t i = 1; i <= count; ++i) {
        // The turn from the entry before, brought into the range -180 to 180.
        float turn = raw[i % count] - raw[i - 1];
        turn -= FULL_TURN_DEGREES * std::round(turn / FULL_TURN_DEGREES);
        turnSum += turn;
        // The last round looks at the first entry again and only adds to the sum.
        if (i < count) {
            angles[i] = angles[0] + turnSum;
        }
    }
    return turnSum;
}

// True when nothing of the maze stands between two points on the ground: the straight
// line from the one to the other only passes from cell to cell through open sides. The
// line is walked in short steps. A step that lands in a cell which is no neighbour by
// a side (out of the maze, or across a corner, where a pillar stands) counts as blocked.
bool sightIsOpen(const Maze& maze, const glm::vec3& from, const glm::vec3& to) {
    const float length = glm::distance(from, to);
    const int stepCount = std::max(static_cast<int>(std::ceil(length / SIGHT_STEP_METRES)), 1);

    MazeCell cell = cellAt(from);
    for (int step = 1; step <= stepCount; ++step) {
        const float share = static_cast<float>(step) / static_cast<float>(stepCount);
        const MazeCell next = cellAt(glm::mix(from, to, share));
        if (next == cell) {
            continue;
        }
        bool open = false;
        for (const Direction side : ALL_DIRECTIONS) {
            if (neighbourOf(cell, side) == next && maze.contains(cell.x, cell.z) &&
                !maze.hasWall(cell.x, cell.z, side)) {
                open = true;
            }
        }
        if (!open) {
            return false;
        }
        cell = next;
    }
    return true;
}

// The yaw from point number index of the path towards the farthest place of the path
// ahead that can be seen from it. travel holds the direction of travel at every point
// as angles that run on, and travelTurn is their change over one loop. The place lies
// at most reach metres ahead and before the path has turned by more than
// MAX_SIGHT_TURN_DEGREES. When no place can be seen the answer is the direction of
// travel.
float sightYaw(const Maze& maze, const MenuCameraPath& path, const std::vector<float>& travel,
               float travelTurn, std::size_t index, float reach) {
    const std::size_t count = path.points.size();
    // How many points of the path lie between two places that are tried.
    const auto stride = std::max<std::size_t>(
        static_cast<std::size_t>(SIGHT_CANDIDATE_METRES / MENU_CAMERA_SAMPLE_SPACING), 1);
    const auto candidateCount = static_cast<std::size_t>(reach / SIGHT_CANDIDATE_METRES);

    const glm::vec3& eye = path.points[index];
    float yaw = travel[index];
    for (std::size_t candidate = 1; candidate <= candidateCount; ++candidate) {
        const std::size_t ahead = index + candidate * stride;
        // Past the end of the list the path starts again, one loop of turning later.
        const std::size_t laps = ahead / count;
        const float turned =
            travel[ahead % count] + static_cast<float>(laps) * travelTurn - travel[index];
        if (std::abs(turned) > MAX_SIGHT_TURN_DEGREES) {
            break;
        }
        const glm::vec3& place = path.points[ahead % count];
        if (glm::distance(eye, place) >= MIN_LENGTH && sightIsOpen(maze, eye, place)) {
            yaw = yawOfStep(place - eye);
        }
    }
    return yaw;
}

// Fills yawDegrees and turnDegrees of a path whose points are set. maze is the maze
// the path leads through.
void computeView(const Maze& maze, MenuCameraPath& path) {
    const std::size_t count = path.points.size();
    if (count < 2) {
        path.yawDegrees.assign(count, 0.0F);
        path.turnDegrees = 0.0F;
        return;
    }

    // The direction of travel at every point: the yaw of the piece that starts there.
    std::vector<float> travel(count);
    for (std::size_t i = 0; i < count; ++i) {
        travel[i] = yawOfStep(path.points[(i + 1) % count] - path.points[i]);
    }
    const float travelTurn = runOn(travel);

    // The yaw towards the farthest place in sight, for every point.
    const float reach = std::min(SIGHT_REACH_METRES, path.length * MAX_WINDOW_SHARE);
    std::vector<float> sight(count);
    for (std::size_t i = 0; i < count; ++i) {
        sight[i] = sightYaw(maze, path, travel, travelTurn, i, reach);
    }
    path.turnDegrees = runOn(sight);

    // The two averages. The points lie almost evenly along the path, so a mean over
    // points is a mean over metres.
    const std::size_t windowReach = pointsWithin(path, LOOK_WINDOW_METRES) / 2;
    const std::size_t easeReach = pointsWithin(path, LOOK_EASE_METRES) / 2;
    path.yawDegrees = averagedAround(averagedAround(sight, path.turnDegrees, windowReach),
                                     path.turnDegrees, easeReach);
}

// Where a distance along the path falls: on the piece that starts at point before and
// ends at point next (the first point again for the piece that closes the loop), share
// of the way from the one to the other (0 to 1).
struct PathPlace {
    std::size_t before = 0;
    std::size_t next = 0;
    bool closing = false;
    float share = 0.0F;
};

// Finds the place of a distance on a path that has points and a length. The path is
// a loop: a distance past its length starts again from the beginning, and a negative
// one counts back from the end.
PathPlace placeOnPath(const MenuCameraPath& path, float distance) {
    // Into the loop: fmod keeps the sign of the distance, so a negative rest is moved
    // up by one loop.
    float along = std::fmod(distance, path.length);
    if (along < 0.0F) {
        along += path.length;
    }

    // The first point that lies farther along than the wanted place. The point before
    // it starts the piece the place is on. distances starts with 0, which is never
    // "farther", so there always is a point before.
    const auto farther = std::ranges::upper_bound(path.distances, along);
    PathPlace place;
    place.next = static_cast<std::size_t>(farther - path.distances.begin());
    place.before = place.next - 1;

    // Past the last point the piece is the one that closes the loop.
    place.closing = place.next == path.points.size();
    const float endDistance = place.closing ? path.length : path.distances[place.next];
    if (place.closing) {
        place.next = 0;
    }

    const float pieceLength = endDistance - path.distances[place.before];
    if (pieceLength >= MIN_LENGTH) {
        place.share = (along - path.distances[place.before]) / pieceLength;
    }
    return place;
}

// The yaw of the view at a distance along the path, blended between the two points
// around that place. Not brought into the range 0 to 360.
float pathYawAt(const MenuCameraPath& path, float distance) {
    const PathPlace place = placeOnPath(path, distance);
    const float endYaw = path.yawDegrees[place.next] + (place.closing ? path.turnDegrees : 0.0F);
    return glm::mix(path.yawDegrees[place.before], endYaw, place.share);
}

// ---- angles ---------------------------------------------------------------------------

// The two angles of scene::Camera for a view along direction, which must have a length.
// The reverse of Camera::forward: (sin(yaw) cos(pitch), sin(pitch), -cos(yaw) cos(pitch)).
void lookAlong(const glm::vec3& direction, MenuCameraPose& pose) {
    const glm::vec3 forward = glm::normalize(direction);
    pose.yawDegrees = glm::degrees(std::atan2(forward.x, -forward.z));
    pose.pitchDegrees = glm::degrees(std::asin(std::clamp(forward.y, -1.0F, 1.0F)));
}

// Brings the angles of a pose into the ranges scene::Camera keeps them in: the yaw
// from 0 to 360 degrees and the pitch away from straight up and straight down.
void keepInRange(MenuCameraPose& pose) {
    pose.yawDegrees -= FULL_TURN_DEGREES * std::floor(pose.yawDegrees / FULL_TURN_DEGREES);
    pose.pitchDegrees = std::clamp(pose.pitchDegrees, -scene::Camera::MAX_PITCH_DEGREES,
                                   scene::Camera::MAX_PITCH_DEGREES);
}

// ---- the two shots --------------------------------------------------------------------

// The middle of the maze on the ground, and the circle the glide flies on around it.
struct GlideCircle {
    glm::vec3 center{0.0F};
    float radius = 0.0F;
    // Height of the camera above center, in metres.
    float height = 0.0F;
};

GlideCircle glideCircle(const MazeWorld& world) {
    const float width = static_cast<float>(world.maze.width()) * CELL_SIZE;
    const float depth = static_cast<float>(world.maze.height()) * CELL_SIZE;

    GlideCircle circle;
    circle.center = {width / 2.0F, 0.0F, depth / 2.0F};
    circle.center.y = world.terrain.heightAt(circle.center.x, circle.center.z);
    // Half of the diagonal of the maze: from its middle to a corner.
    const float toCorner = std::sqrt(width * width + depth * depth) / 2.0F;
    circle.radius = std::max(toCorner * GLIDE_RADIUS_SHARE, MIN_GLIDE_RADIUS);
    circle.height = WALL_HEIGHT + circle.radius * GLIDE_HEIGHT_SHARE;
    return circle;
}

MenuCameraPose corridorWalkPose(const MenuCameraPath& path, const MenuCameraSettings& settings,
                                float seconds) {
    MenuCameraPose pose;
    if (path.points.empty() || path.length < MIN_LENGTH) {
        return pose;
    }

    // Constant speed: the distance along the path grows evenly with the time.
    const float travelled = (seconds + settings.timeOffset) * settings.speed;
    const glm::vec3 ground = menuCameraPathPoint(path, travelled);
    pose.eye = ground + scene::Camera::WORLD_UP * settings.eyeHeight;

    // The view: towards the farthest place of the path in sight, with every turn
    // spread over a few metres (MenuCameraPath::yawDegrees).
    const float lag = std::min(LOOK_LAG_METRES, path.length * MAX_WINDOW_SHARE);
    pose.yawDegrees = pathYawAt(path, travelled - lag);

    // The slope: how much the ground rises between a place behind the camera and
    // a place ahead of it, over the length of path between them. The ground of the
    // maze is gentle, so this is a degree or two.
    const float reach = std::min(SLOPE_REACH_METRES, path.length * MAX_WINDOW_SHARE);
    const float rise = menuCameraPathPoint(path, travelled + reach).y -
                       menuCameraPathPoint(path, travelled - reach).y;
    pose.pitchDegrees = glm::degrees(std::asin(std::clamp(rise / (reach * 2.0F), -1.0F, 1.0F)));
    pose.pitchDegrees += LOOK_UP_DEGREES;

    // The sway, measured along the path and not in seconds: a whole number of swings
    // fits into the loop, so the view has no jump where the loop closes.
    const float swings = std::max(std::round(path.length / SWAY_METRES), 1.0F);
    const float phase = FULL_TURN * swings * travelled / path.length;
    pose.yawDegrees += SWAY_YAW_DEGREES * std::sin(phase);
    pose.pitchDegrees += SWAY_PITCH_DEGREES * std::sin(SWAY_PITCH_SWINGS * phase);
    keepInRange(pose);
    return pose;
}

MenuCameraPose highGlidePose(const MazeWorld& world, const MenuCameraSettings& settings,
                             float seconds) {
    const GlideCircle circle = glideCircle(world);

    // The angle on the circle, in radians: the way flown divided by the radius. 0 is
    // north of the middle, and it grows clockwise seen from above, like a yaw.
    const float flown = (seconds + settings.timeOffset) * settings.speed * GLIDE_SPEED_SCALE;
    const float angle = flown / circle.radius;

    MenuCameraPose pose;
    pose.eye = circle.center + glm::vec3{circle.radius * std::sin(angle), circle.height,
                                         -circle.radius * std::cos(angle)};
    const glm::vec3 target = circle.center + scene::Camera::WORLD_UP * GLIDE_TARGET_HEIGHT;
    lookAlong(target - pose.eye, pose);
    keepInRange(pose);
    return pose;
}

} // namespace

std::vector<MazeCell> menuCameraRoute(const Maze& maze, MazeCell start,
                                      std::span<const MazeCell> targets) {
    // Throws for a start outside of the maze.
    const std::vector<int> distances = passageDistances(maze, start);

    // The tree: from every target back to the start, always to a neighbour that is one
    // passage nearer, until a cell that is already on the tree. Each new cell brings
    // one passage with it, the one to its parent.
    std::vector<TreeCell> tree(distances.size());
    tree[cellIndex(maze, start)].onTree = true;
    std::size_t passageCount = 0;
    for (const MazeCell target : targets) {
        if (!maze.contains(target.x, target.z) ||
            distances[cellIndex(maze, target)] == UNREACHABLE) {
            continue;
        }
        MazeCell cell = target;
        while (!tree[cellIndex(maze, cell)].onTree) {
            TreeCell& entry = tree[cellIndex(maze, cell)];
            entry.onTree = true;
            entry.hasParent = true;
            entry.parentSide = sideTowardsStart(maze, distances, cell);
            ++passageCount;
            cell = neighbourOf(cell, entry.parentSide);
        }
    }

    // The walk around the tree. In every cell the walker tries the way to its right
    // first, then straight on, then to the left, and turns around when there is nothing
    // else: the rule of keeping one hand on the wall. Around a tree that rule uses
    // every passage exactly twice, once in each direction, and then stands at the start
    // again, so the number of steps is known. The heading before the first step is not
    // a real one: it only says where "right" is in the start cell.
    std::vector<MazeCell> route;
    route.reserve(std::max<std::size_t>(passageCount * 2, 1));
    route.push_back(start);

    constexpr int TURN_COUNT = 4;
    // Quarter turns to the right: 1 is right, 0 straight on, 3 left and 2 back.
    constexpr std::array<int, TURN_COUNT> TURN_ORDER = {1, 0, 3, 2};

    MazeCell cell = start;
    Direction heading = Direction::North;
    const std::size_t stepCount = passageCount * 2;
    for (std::size_t step = 0; step < stepCount; ++step) {
        for (const int quarterTurns : TURN_ORDER) {
            const Direction candidate = turnedRight(heading, quarterTurns);
            if (isTreePassage(maze, tree, cell, candidate)) {
                heading = candidate;
                break;
            }
        }
        cell = neighbourOf(cell, heading);
        // The last step leads back into the start, which opens the list already.
        if (step + 1 < stepCount) {
            route.push_back(cell);
        }
    }
    return route;
}

std::vector<MazeCell> menuCameraTargets(const MazeWorld& world) {
    std::vector<MazeCell> targets;
    targets.reserve(world.crystals.size() + 1);
    for (const CrystalSpawn& crystal : world.crystals) {
        targets.push_back(crystal.cell);
    }

    // The cell in front of the gate: the neighbour on the first open side of the exit
    // cell, the side placeExit put the gate on. The exit of a generated maze is a dead
    // end, so no way to a target leads through the closed gate.
    if (world.hasGate) {
        const MazeCell exit = world.exitCell;
        for (const Direction side : ALL_DIRECTIONS) {
            if (!world.maze.hasWall(exit.x, exit.z, side)) {
                targets.push_back(neighbourOf(exit, side));
                break;
            }
        }
    }
    return targets;
}

MenuCameraPath buildMenuCameraPath(const MazeWorld& world) {
    const std::vector<MazeCell> targets = menuCameraTargets(world);
    const std::vector<MazeCell> route = menuCameraRoute(world.maze, START_CELL, targets);
    const std::vector<glm::vec2> flat = roundedLoop(laneCorners(route));

    MenuCameraPath path;
    path.points.reserve(flat.size());
    path.distances.reserve(flat.size());
    for (const glm::vec2& point : flat) {
        // On the ground: the second number of a flat point is the world z.
        const glm::vec3 onGround{point.x, world.terrain.heightAt(point.x, point.y), point.y};
        if (!path.points.empty()) {
            path.length += glm::distance(path.points.back(), onGround);
        }
        path.points.push_back(onGround);
        path.distances.push_back(path.length);
    }
    // The piece that closes the loop: from the last point back to the first.
    if (!path.points.empty()) {
        path.length += glm::distance(path.points.back(), path.points.front());
    }
    computeView(world.maze, path);
    return path;
}

glm::vec3 menuCameraPathPoint(const MenuCameraPath& path, float distance) {
    if (path.points.empty()) {
        return glm::vec3{0.0F};
    }
    if (path.length < MIN_LENGTH) {
        return path.points.front();
    }

    const PathPlace place = placeOnPath(path, distance);
    return glm::mix(path.points[place.before], path.points[place.next], place.share);
}

float menuCameraLoopSeconds(const MenuCameraPath& path, const MazeWorld& world,
                            const MenuCameraSettings& settings) {
    if (settings.speed <= 0.0F) {
        return 0.0F;
    }
    if (settings.shot == MenuShot::HighGlide) {
        // Once around the circle, at the speed of the glide.
        return FULL_TURN * glideCircle(world).radius / (settings.speed * GLIDE_SPEED_SCALE);
    }
    return path.length / settings.speed;
}

MenuCameraPose menuCameraPose(const MenuCameraPath& path, const MazeWorld& world,
                              const MenuCameraSettings& settings, float seconds) {
    if (settings.shot == MenuShot::HighGlide) {
        return highGlidePose(world, settings, seconds);
    }
    return corridorWalkPose(path, settings, seconds);
}

} // namespace game
