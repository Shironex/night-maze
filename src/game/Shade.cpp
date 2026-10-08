// Shade: the enemy of a round. A dark figure that walks towards the player whenever the
// flashlight is not on it, and carries the player back to the start when it gets there.
#include "game/Shade.hpp"

#include "game/Exit.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/Terrain.hpp"
#include "scene/Raycast.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>

namespace game {

namespace {

// The generator of the shade is seeded with the seed of the maze plus this number, so
// it repeats the numbers of no other generator: the crystals add 1000003, the grass
// 2000003, the notes 3000017, the puddles 4000037, the levers 5000011 and the flasks
// 6000011. Adding to an unsigned number wraps around at 2^32, which is well defined.
constexpr std::uint32_t SHADE_SEED_OFFSET = 7000003U;

constexpr int TENTHS = 10;

// Closer than this to a point, in metres, counts as standing on it.
constexpr float ARRIVED = 0.001F;

// The lines of section 4.3 of the story, in their order.
constexpr std::array<std::string_view, CAUGHT_LINE_COUNT> CAUGHT_LINES = {
    "It carried you back to the stile. Nothing more than that.",
    "The same night. The same hedge. Walk it again.",
    "You turned too late. It was gentle about it.",
    "It set you down where you came in, and went on looking.",
    "Still dark. Still yours to do.",
};

// Position of a cell in a list that holds the rows one after another, like in Maze.
std::size_t cellIndex(const Maze& maze, MazeCell cell) {
    return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
           static_cast<std::size_t>(cell.x);
}

// The same point with its height taken from the ground.
glm::vec3 onGround(const Terrain& terrain, glm::vec3 point) {
    point.y = terrain.heightAt(point.x, point.z);
    return point;
}

// Moves from towards to by at most reach metres, on the ground plane (y is not used),
// and takes what was walked off reach.
glm::vec3 walkTowards(const glm::vec3& from, const glm::vec3& to, float& reach) {
    const glm::vec2 way{to.x - from.x, to.z - from.z};
    const float distance = glm::length(way);
    if (distance <= ARRIVED) {
        return from;
    }
    const float walked = std::min(reach, distance);
    reach -= walked;
    const glm::vec2 step = way * (walked / distance);
    return {from.x + step.x, from.y, from.z + step.y};
}

// The cell the shade walks to next when it is in the cell own: the neighbour through an
// open side that is one passage nearer to the player. Of two such neighbours (only in
// a maze with a loop, after a lever) the first one in the order of ALL_DIRECTIONS wins.
// own itself when there is no way from it, or when the search has not found it.
MazeCell nextCellTowardsPlayer(const Shade& shade, const Maze& maze, MazeCell own) {
    if (shade.pathDistances.size() !=
        static_cast<std::size_t>(maze.width()) * static_cast<std::size_t>(maze.height())) {
        return own;
    }
    const int ownDistance = shade.pathDistances[cellIndex(maze, own)];
    // UNREACHABLE is -1: it must not be taken for "nearer than anything".
    if (ownDistance == UNREACHABLE) {
        return own;
    }
    for (const Direction direction : ALL_DIRECTIONS) {
        if (maze.hasWall(own.x, own.z, direction)) {
            continue;
        }
        const MazeCell neighbour{.x = own.x + columnStep(direction),
                                 .z = own.z + rowStep(direction)};
        if (!maze.contains(neighbour.x, neighbour.z)) {
            continue;
        }
        const int distance = shade.pathDistances[cellIndex(maze, neighbour)];
        if (distance != UNREACHABLE && distance < ownDistance) {
            return neighbour;
        }
    }
    return own;
}

// True when the two cells are the same cell, or neighbours with no wall between them.
bool sameOrJoined(const Maze& maze, MazeCell from, MazeCell to) {
    if (from == to) {
        return true;
    }
    for (const Direction direction : ALL_DIRECTIONS) {
        const MazeCell neighbour{.x = from.x + columnStep(direction),
                                 .z = from.z + rowStep(direction)};
        if (neighbour == to) {
            return !maze.hasWall(from.x, from.z, direction);
        }
    }
    return false;
}

} // namespace

bool shadeStartCell(const Maze& maze, std::uint32_t seed, MazeCell start, MazeCell exit,
                    MazeCell& cell) {
    // passageDistances throws when start is not a cell of the maze.
    const std::vector<int> distances = passageDistances(maze, start);
    const int farthest = *std::ranges::max_element(distances);

    // The far cells, row after row: the order before the draw is part of the result.
    std::vector<MazeCell> farCells;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell candidate{.x = x, .z = z};
            if (candidate == start || candidate == exit) {
                continue;
            }
            // An unreachable cell has the distance -1 and fails this test too.
            const int distance = distances[cellIndex(maze, candidate)];
            if (distance > 0 && distance * TENTHS >= farthest * SHADE_START_FAR_TENTHS) {
                farCells.push_back(candidate);
            }
        }
    }
    if (farCells.empty()) {
        return false;
    }
    std::mt19937 generator(seed + SHADE_SEED_OFFSET);
    cell = farCells[randomBelow(generator, static_cast<std::uint32_t>(farCells.size()))];
    return true;
}

Shade startShade(const Maze& maze, std::uint32_t seed, MazeCell start, MazeCell exit,
                 const Terrain& terrain, const ShadeSettings& settings) {
    Shade shade;
    MazeCell cell;
    if (!settings.enabled || !shadeStartCell(maze, seed, start, exit, cell)) {
        return shade;
    }
    shade.present = true;
    shade.position = onGround(terrain, cellCenter(cell.x, cell.z));
    shade.previousPosition = shade.position;
    shade.target = cell;
    shade.graceLeft = settings.graceSeconds;
    return shade;
}

bool shadeLit(const Shade& shade, const ShadeLamp& lamp, std::span<const scene::Aabb> obstacles) {
    if (!shade.present || !lamp.on) {
        return false;
    }
    // A point is inside the cone when the angle between the axis of the cone and the
    // line to the point is at most the half angle. The cosine gets smaller as the angle
    // grows, so that is: the cosine of the angle is at least the cosine of the half angle.
    const float cosOuter = std::cos(glm::radians(lamp.outerDegrees));
    for (const float height : SHADE_LIT_HEIGHTS) {
        const glm::vec3 point = shade.position + glm::vec3{0.0F, height, 0.0F};
        const glm::vec3 way = point - lamp.position;
        const float distance = glm::length(way);
        if (distance > lamp.range) {
            continue;
        }
        // The lamp is inside the shade: nothing can be in between.
        if (distance <= ARRIVED) {
            return true;
        }
        const glm::vec3 towards = way / distance;
        if (glm::dot(towards, lamp.direction) < cosOuter) {
            continue;
        }
        // A wall on the way to the point: a box the ray reaches before the point.
        const scene::NearestHit wall =
            scene::nearestHit({.origin = lamp.position, .direction = towards}, obstacles, distance);
        if (wall.hit && wall.distance < distance) {
            continue;
        }
        return true;
    }
    return false;
}

float shadeDistance(const Shade& shade, const glm::vec3& playerFeet) {
    return glm::length(glm::vec2{playerFeet.x - shade.position.x, playerFeet.z - shade.position.z});
}

float shadeYawDegrees(const glm::vec3& position, const glm::vec3& playerFeet) {
    // A turn by an angle a around Y moves the direction +Z to (sin a, 0, cos a). So the
    // angle of a direction (x, z) is atan2(x, z). atan2(0, 0) is 0.
    return glm::degrees(std::atan2(playerFeet.x - position.x, playerFeet.z - position.z));
}

bool advanceShade(Shade& shade, const ShadeSettings& settings, const Maze& maze,
                  const Terrain& terrain, const ShadeStep& step, float stepSeconds) {
    if (!shade.present) {
        return false;
    }
    if (!settings.enabled) {
        shade = Shade{};
        return false;
    }
    shade.previousPosition = shade.position;
    shade.lit = shadeLit(shade, step.lamp, step.obstacles);

    // It stands still in its grace time, while it is lit and for a moment after. The way to the
    // player is kept up to date all the same, because the hum is told how long that way is.
    const bool waiting = shade.graceLeft > 0.0F;
    if (waiting) {
        shade.graceLeft -= stepSeconds;
    }
    // Lit: the wait after the light starts anew. Not lit any more: that wait runs down.
    const bool thawing = !shade.lit && shade.thawLeft > 0.0F;
    if (shade.lit) {
        shade.thawLeft = settings.thawSeconds;
    } else if (thawing) {
        shade.thawLeft -= stepSeconds;
    }
    const bool walks = !waiting && !shade.lit && !thawing;

    MazeCell own = cellAt(shade.position);
    const MazeCell playerCell = cellAt(step.playerFeet);
    // Both have to be cells of the maze for a way to exist. A player who flies out of
    // the maze is not followed and not caught, and passageDistances would throw for
    // such a cell.
    if (!maze.contains(own.x, own.z) || !maze.contains(playerCell.x, playerCell.z)) {
        shade.wayMetres = SHADE_NO_WAY;
        return false;
    }

    // The way is searched again only when it can have changed. Then the shade also
    // chooses its next cell anew, wherever it is: the player may be behind it now.
    if (!(shade.pathGoal == playerCell) || shade.pathOpenings != step.openedWalls) {
        shade.pathDistances = passageDistances(maze, playerCell);
        shade.pathGoal = playerCell;
        shade.pathOpenings = step.openedWalls;
        shade.target = nextCellTowardsPlayer(shade, maze, own);
    }

    if (walks) {
        float reach = std::max(settings.speed, 0.0F) * stepSeconds;
        if (own == playerCell) {
            // The same cell: nothing stands inside a cell, so straight at the player.
            shade.target = own;
            shade.position = walkTowards(shade.position, step.playerFeet, reach);
        } else {
            // From centre to centre. A step can end exactly on a centre, and a fast
            // shade can pass one within a step, so the walk goes on with what is left
            // of the step. The limit only guards against a walk that gets nowhere.
            constexpr int MAX_CENTRES_PER_STEP = 4;
            for (int i = 0; i < MAX_CENTRES_PER_STEP && reach > 0.0F; ++i) {
                const glm::vec3 centre = cellCenter(shade.target.x, shade.target.z);
                shade.position = walkTowards(shade.position, centre, reach);
                if (reach <= 0.0F) {
                    break;
                }
                // The centre is reached: on to the next cell, or stay when this is as
                // near as the shade can get.
                const MazeCell next = nextCellTowardsPlayer(shade, maze, shade.target);
                if (next == shade.target) {
                    break;
                }
                shade.target = next;
            }
        }
        shade.position = onGround(terrain, shade.position);
        own = cellAt(shade.position);
    }

    // How far it still has to walk: straight to the player in the same cell, otherwise
    // to the centre it is heading for and from there cell by cell (2 m each).
    const int passagesLeft = shade.pathDistances[cellIndex(maze, shade.target)];
    if (own == playerCell) {
        shade.wayMetres = shadeDistance(shade, step.playerFeet);
    } else if (passagesLeft == UNREACHABLE) {
        shade.wayMetres = SHADE_NO_WAY;
    } else {
        const glm::vec3 centre = cellCenter(shade.target.x, shade.target.z);
        shade.wayMetres =
            glm::length(glm::vec2{centre.x - shade.position.x, centre.z - shade.position.z}) +
            static_cast<float>(passagesLeft) * CELL_SIZE;
    }

    // Near enough, and nothing between the two: a player who stands against the other
    // side of a wall is not caught through it.
    return walks && shadeDistance(shade, step.playerFeet) <= settings.catchDistance &&
           sameOrJoined(maze, own, playerCell);
}

std::string_view caughtLine(int index) {
    if (index < 0 || index >= CAUGHT_LINE_COUNT) {
        throw std::out_of_range("caughtLine: there is no caught line with this number");
    }
    return CAUGHT_LINES[static_cast<std::size_t>(index)];
}

int nextCaughtLine(int previous) {
    if (previous < 0) {
        return 0;
    }
    return (previous + 1) % CAUGHT_LINE_COUNT;
}

float caughtBrightness(float secondsSinceCaught) {
    const float t = std::clamp(secondsSinceCaught / CAUGHT_FADE_SECONDS, 0.0F, 1.0F);
    // The curve 3t^2 - 2t^3: it starts and ends flat, so the light neither jumps on nor
    // stops with a corner.
    return t * t * (3.0F - 2.0F * t);
}

} // namespace game
