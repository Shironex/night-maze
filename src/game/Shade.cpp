// Shade: the enemy of a round. A dark figure that wanders the maze, comes when it hears or
// sees the player, stands while the flashlight is on it and is burned away by a long look
// of the lamp. It carries the player back to the start when it reaches them.
#include "game/Shade.hpp"

#include "game/Exit.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"
#include "game/Player.hpp"
#include "game/Terrain.hpp"
#include "scene/Raycast.hpp"

#include <glm/gtc/constants.hpp>

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

// The choices the shade makes later in the round have generators of their own: one per
// cell it wanders to and one per banish. Each is seeded with the seed of the maze, one
// of these two offsets and the number of the choice times a step. The steps are primes,
// so the seeds of the two rows never meet for the few hundred choices a round can have.
constexpr std::uint32_t SHADE_WANDER_SEED_OFFSET = 8000009U;
constexpr std::uint32_t SHADE_WANDER_SEED_STEP = 7919U;
constexpr std::uint32_t SHADE_BANISH_SEED_OFFSET = 9000011U;
constexpr std::uint32_t SHADE_BANISH_SEED_STEP = 104729U;

constexpr int TENTHS = 10;

// Closer than this to a point, in metres, counts as standing on it.
constexpr float ARRIVED = 0.001F;

// The cell that means "no cell": the goal of a shade that has not chosen one.
constexpr MazeCell NO_CELL{.x = -1, .z = -1};

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

// How far two points are apart on the ground (x and z, without the height).
float groundDistance(const glm::vec3& from, const glm::vec3& to) {
    return glm::length(glm::vec2{to.x - from.x, to.z - from.z});
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

// A cell far from the cell `from`, chosen by a generator with the seed generatorSeed: the
// rule the start cell, the wander cells and the banish cells share.
//   - never `from` itself and never the exit cell (the exit lies behind the gate),
//   - only cells that can be reached and are far: at least farTenths tenths as many
//     passages away as the farthest cell,
//   - of those cells, listed row after row, the generator chooses one.
// False when the maze has no such cell: cell is left as it was.
bool farCell(const Maze& maze, std::uint32_t generatorSeed, MazeCell from, MazeCell exit,
             int farTenths, MazeCell& cell) {
    // passageDistances throws when from is not a cell of the maze.
    const std::vector<int> distances = passageDistances(maze, from);
    const int farthest = *std::ranges::max_element(distances);

    // The far cells, row after row: the order before the draw is part of the result.
    std::vector<MazeCell> farCells;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell candidate{.x = x, .z = z};
            if (candidate == from || candidate == exit) {
                continue;
            }
            // An unreachable cell has the distance -1 and fails this test too.
            const int distance = distances[cellIndex(maze, candidate)];
            if (distance > 0 && distance * TENTHS >= farthest * farTenths) {
                farCells.push_back(candidate);
            }
        }
    }
    if (farCells.empty()) {
        return false;
    }
    std::mt19937 generator(generatorSeed);
    cell = farCells[randomBelow(generator, static_cast<std::uint32_t>(farCells.size()))];
    return true;
}

// True when distances is a list for this maze: one number per cell.
bool fitsMaze(const std::vector<int>& distances, const Maze& maze) {
    return distances.size() ==
           static_cast<std::size_t>(maze.width()) * static_cast<std::size_t>(maze.height());
}

// The cell to walk to next from the cell own, on the way distances describes
// (passageDistances from the goal): the neighbour through an open side that is one
// passage nearer to the goal. Of two such neighbours (only in a maze with a loop, after
// a lever) the first one in the order of ALL_DIRECTIONS wins. own itself when there is
// no way from it, or when it is the goal.
MazeCell nextCellTowards(const std::vector<int>& distances, const Maze& maze, MazeCell own) {
    if (!fitsMaze(distances, maze)) {
        return own;
    }
    const int ownDistance = distances[cellIndex(maze, own)];
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
        const int distance = distances[cellIndex(maze, neighbour)];
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

// How far somebody at position has to walk to the cell distances was searched from
// (passageDistances), in metres: to the centre of the cell they are in or of a neighbour
// through an open side, whichever is the shorter way, and from there cell by cell (2 m
// each). The shade walks from centre to centre, so one of those centres is always the
// next point of its shortest way. SHADE_NO_WAY when there is none.
float wayAlongPassages(const std::vector<int>& distances, const Maze& maze,
                       const glm::vec3& position) {
    const MazeCell own = cellAt(position);
    if (!fitsMaze(distances, maze) || !maze.contains(own.x, own.z)) {
        return SHADE_NO_WAY;
    }
    float way = SHADE_NO_WAY;
    const auto consider = [&](MazeCell cell) {
        const int passages = distances[cellIndex(maze, cell)];
        if (passages != UNREACHABLE) {
            way = std::min(way, groundDistance(position, cellCenter(cell.x, cell.z)) +
                                    static_cast<float>(passages) * CELL_SIZE);
        }
    };
    consider(own);
    for (const Direction direction : ALL_DIRECTIONS) {
        const MazeCell neighbour{.x = own.x + columnStep(direction),
                                 .z = own.z + rowStep(direction)};
        if (!maze.hasWall(own.x, own.z, direction) && maze.contains(neighbour.x, neighbour.z)) {
            consider(neighbour);
        }
    }
    return way;
}

// Keeps Shade::playerDistances and Shade::wayMetres up to date for a player in the maze.
// The search runs only when the player is in another cell than last time or another wall
// has opened.
void updateWayToPlayer(Shade& shade, const Maze& maze, const ShadeStep& step, MazeCell playerCell) {
    if (!(shade.playerCell == playerCell) || shade.playerOpenings != step.openedWalls) {
        shade.playerDistances = passageDistances(maze, playerCell);
        shade.playerCell = playerCell;
        shade.playerOpenings = step.openedWalls;
    }
    // In the same cell nothing stands between the two: the straight line.
    shade.wayMetres = cellAt(shade.position) == playerCell
                          ? shadeDistance(shade, step.playerFeet)
                          : wayAlongPassages(shade.playerDistances, maze, shade.position);
}

// The shade wanders from now on, to the next cell of its own choice. Without such a cell
// (a maze of one room) it stays where it is.
void startWandering(Shade& shade, const Maze& maze, MazeCell own) {
    shade.hunt = ShadeHunt::Wandering;
    shade.searchLeft = 0.0F;
    MazeCell goal = own;
    shadeWanderCell(maze, shade.seed, shade.wanderCount, own, shade.exit, goal);
    ++shade.wanderCount;
    shade.goal = goal;
}

// The beam has burned the shade away: it is gone from where it stood, stands in a cell
// far from the player and is quiet. Without such a cell it stays in its own.
void banish(Shade& shade, const ShadeSettings& settings, const Maze& maze, const Terrain& terrain,
            MazeCell own, MazeCell playerCell) {
    shade.banishedFrom = shade.position;
    shade.dissolveLeft = SHADE_DISSOLVE_SECONDS;

    MazeCell cell = own;
    if (maze.contains(playerCell.x, playerCell.z)) {
        shadeBanishCell(maze, shade.seed, shade.banishCount, playerCell, shade.exit, cell);
    }
    ++shade.banishCount;
    shade.position = onGround(terrain, cellCenter(cell.x, cell.z));
    // Both positions: a frame is drawn between the two, and the step of the banish is
    // not a walk across the maze. The sound of its steps asks the same two points.
    shade.previousPosition = shade.position;
    shade.target = cell;

    shade.lit = false;
    shade.thawLeft = 0.0F;
    shade.burnSeconds = 0.0F;
    shade.quietLeft = settings.quietSeconds;
    shade.wayMetres = SHADE_NO_WAY;
    // It has forgotten the player: when the quiet time is over it wanders, to a goal it
    // chooses then.
    shade.hunt = ShadeHunt::Wandering;
    shade.goal = NO_CELL;
    shade.searchLeft = 0.0F;
}

// The senses of the shade for one step, with a player in the maze: sight first, then
// hearing. Returns true when a shade that was wandering has noticed the player.
bool sense(Shade& shade, const ShadeSettings& settings, const Maze& maze, const ShadeStep& step,
           MazeCell playerCell, bool& sees) {
    const bool wasWandering = shade.hunt == ShadeHunt::Wandering;
    sees = shadeSees(maze, shade.position, step.playerFeet, settings.sightMetres);
    if (sees) {
        shade.hunt = ShadeHunt::Chasing;
    } else if (shadeHears(shade.wayMetres, step.noise, settings)) {
        shade.lastHeard = step.noise;
        shade.lastHeardCell = playerCell;
        // A shade on a chase stays on it: it has lost sight, and the noise tells it
        // where to go on looking.
        if (wasWandering) {
            shade.hunt = ShadeHunt::Investigating;
        }
    } else {
        return false;
    }
    // Seen or heard: the cell of the player at this moment is where it goes, and the
    // wait at that cell starts anew.
    shade.goal = playerCell;
    shade.searchLeft = settings.searchSeconds;
    return wasWandering;
}

// Walks the shade towards its goal by reach metres, from cell centre to cell centre.
// Returns true when it stands on the centre of its goal (or cannot get any nearer).
bool walkToGoal(Shade& shade, const Maze& maze, int openedWalls, float reach) {
    // The way to the goal is searched again only when it can have changed: the goal is
    // another cell, or another wall has opened. Then the shade also chooses its next
    // cell anew, wherever it is: the goal may be behind it now.
    if (!(shade.pathGoal == shade.goal) || shade.pathOpenings != openedWalls) {
        shade.goalDistances = passageDistances(maze, shade.goal);
        shade.pathGoal = shade.goal;
        shade.pathOpenings = openedWalls;
        shade.target = nextCellTowards(shade.goalDistances, maze, cellAt(shade.position));
    }
    const std::vector<int>& ways = shade.goalDistances;

    // A step can end exactly on a centre, and a fast shade can pass one within a step,
    // so the walk goes on with what is left of the step. The limit only guards against
    // a walk that gets nowhere.
    constexpr int MAX_CENTRES_PER_STEP = 4;
    for (int i = 0; i < MAX_CENTRES_PER_STEP; ++i) {
        const glm::vec3 centre = cellCenter(shade.target.x, shade.target.z);
        shade.position = walkTowards(shade.position, centre, reach);
        if (reach <= 0.0F) {
            return false;
        }
        // The centre is reached: on to the next cell, or stay when this is the goal or
        // as near as the shade can get.
        const MazeCell next = nextCellTowards(ways, maze, shade.target);
        if (next == shade.target) {
            return true;
        }
        shade.target = next;
    }
    return false;
}

} // namespace

const char* noiseName(Noise noise) {
    switch (noise) {
    case Noise::None:
        return "nothing";
    case Noise::Walk:
        return "walking";
    case Noise::Pickup:
        return "a pickup";
    case Noise::Lever:
        return "a lever";
    case Noise::Sprint:
        return "a sprint";
    }
    // Only reached with a number that is no noise at all.
    return "unknown";
}

const char* shadeStateName(ShadeState state) {
    switch (state) {
    case ShadeState::Banished:
        return "banished and quiet";
    case ShadeState::Grace:
        return "grace time";
    case ShadeState::Lit:
        return "lit: standing still";
    case ShadeState::Thawing:
        return "waiting after the light";
    case ShadeState::Chasing:
        return "chasing";
    case ShadeState::Investigating:
        return "investigating a noise";
    case ShadeState::Wandering:
        return "wandering";
    }
    // Only reached with a number that is no state at all.
    return "unknown";
}

bool shadeStartCell(const Maze& maze, std::uint32_t seed, MazeCell start, MazeCell exit,
                    MazeCell& cell) {
    return farCell(maze, seed + SHADE_SEED_OFFSET, start, exit, SHADE_START_FAR_TENTHS, cell);
}

bool shadeWanderCell(const Maze& maze, std::uint32_t seed, int pick, MazeCell from, MazeCell exit,
                     MazeCell& cell) {
    const std::uint32_t generatorSeed =
        seed + SHADE_WANDER_SEED_OFFSET + static_cast<std::uint32_t>(pick) * SHADE_WANDER_SEED_STEP;
    return farCell(maze, generatorSeed, from, exit, SHADE_WANDER_FAR_TENTHS, cell);
}

bool shadeBanishCell(const Maze& maze, std::uint32_t seed, int count, MazeCell playerCell,
                     MazeCell exit, MazeCell& cell) {
    const std::uint32_t generatorSeed = seed + SHADE_BANISH_SEED_OFFSET +
                                        static_cast<std::uint32_t>(count) * SHADE_BANISH_SEED_STEP;
    return farCell(maze, generatorSeed, playerCell, exit, SHADE_START_FAR_TENTHS, cell);
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
    shade.seed = seed;
    shade.exit = exit;
    return shade;
}

float noiseReach(Noise noise, const ShadeSettings& settings) {
    switch (noise) {
    case Noise::None:
        return 0.0F;
    case Noise::Walk:
        return settings.hearWalkMetres;
    case Noise::Pickup:
        return settings.hearPickupMetres;
    case Noise::Lever:
        return settings.hearLeverMetres;
    case Noise::Sprint:
        return settings.hearSprintMetres;
    }
    return 0.0F;
}

Noise playerNoise(const NoiseSources& sources, const ShadeSettings& settings) {
    Noise loudest = Noise::None;
    // Takes a noise when it carries farther than the loudest one so far.
    const auto hear = [&](Noise noise) {
        if (noiseReach(noise, settings) > noiseReach(loudest, settings)) {
            loudest = noise;
        }
    };
    if (!sources.flying && sources.metres > 0.0F) {
        hear(sprintedStep(sources.metres, sources.stepSeconds, sources.walkSpeed,
                          sources.sprintSpeed)
                 ? Noise::Sprint
                 : Noise::Walk);
    }
    if (sources.pickedUp) {
        hear(Noise::Pickup);
    }
    if (sources.pulledLever) {
        hear(Noise::Lever);
    }
    return loudest;
}

bool shadeHears(float wayMetres, Noise noise, const ShadeSettings& settings) {
    return noise != Noise::None && wayMetres <= noiseReach(noise, settings);
}

int noiseTicks(float reachMetres, const ShadeSettings& settings) {
    if (reachMetres <= 0.0F) {
        return 0;
    }
    // Settings in which a sprint carries nowhere have no scale to measure against:
    // whatever is heard then counts as the loudest.
    if (settings.hearSprintMetres <= 0.0F) {
        return NOISE_TICK_COUNT;
    }
    const float part = reachMetres / settings.hearSprintMetres;
    // lround rounds to the nearest whole number: 1.3 becomes 1 and 1.7 becomes 2.
    const int ticks = static_cast<int>(std::lround(part * static_cast<float>(NOISE_TICK_COUNT)));
    return std::clamp(ticks, 1, NOISE_TICK_COUNT);
}

void advanceNoiseMeter(NoiseMeter& meter, Noise noise, const ShadeSettings& settings,
                       float stepSeconds) {
    meter.holdLeft = std::max(meter.holdLeft - stepSeconds, 0.0F);
    const bool louder = noiseReach(noise, settings) >= noiseReach(meter.noise, settings);
    if (!louder && meter.holdLeft > 0.0F) {
        return;
    }
    meter.noise = noise;
    const bool oneStep = noise == Noise::Pickup || noise == Noise::Lever;
    meter.holdLeft = oneStep ? NOISE_FLASH_SECONDS : 0.0F;
}

bool shadeSees(const Maze& maze, const glm::vec3& shadeFeet, const glm::vec3& playerFeet,
               float rangeMetres) {
    const MazeCell from = cellAt(shadeFeet);
    const MazeCell to = cellAt(playerFeet);
    if (!maze.contains(from.x, from.z) || !maze.contains(to.x, to.z)) {
        return false;
    }
    if (groundDistance(shadeFeet, playerFeet) > rangeMetres) {
        return false;
    }
    // Not in one row and not in one column: a corner lies between the two.
    if (from.x != to.x && from.z != to.z) {
        return false;
    }
    // Cell by cell along the line, from the shade towards the player: one wall on the
    // side it leaves a cell by ends the view. In the same cell the loop does nothing.
    Direction direction = Direction::East;
    if (to.x < from.x) {
        direction = Direction::West;
    } else if (to.z > from.z) {
        direction = Direction::South;
    } else if (to.z < from.z) {
        direction = Direction::North;
    }
    for (MazeCell cell = from; !(cell == to);
         cell = {.x = cell.x + columnStep(direction), .z = cell.z + rowStep(direction)}) {
        if (maze.hasWall(cell.x, cell.z, direction)) {
            return false;
        }
    }
    return true;
}

float burnAfter(float burnSeconds, bool lit, const ShadeSettings& settings, float stepSeconds) {
    if (lit) {
        return burnSeconds + stepSeconds;
    }
    return std::max(burnSeconds - std::max(settings.burnRecoverRate, 0.0F) * stepSeconds, 0.0F);
}

float batteryDrainFactor(const Shade& shade, const ShadeSettings& settings) {
    // Never less than usual: a factor below 1 would make the shade a way to save light.
    return shade.present && shade.lit ? std::max(settings.burnBatteryFactor, 1.0F) : 1.0F;
}

float shadeBurnProgress(const Shade& shade, const ShadeSettings& settings) {
    if (!shade.present || settings.burnSeconds <= 0.0F) {
        return 0.0F;
    }
    if (shade.dissolveLeft > 0.0F) {
        return 1.0F;
    }
    return std::clamp(shade.burnSeconds / settings.burnSeconds, 0.0F, 1.0F);
}

float shadeHoodBrightness(float burnProgress) {
    const float progress = std::clamp(burnProgress, 0.0F, 1.0F);
    return SHADE_HOOD_REST_BRIGHTNESS +
           (SHADE_HOOD_BURNED_BRIGHTNESS - SHADE_HOOD_REST_BRIGHTNESS) * progress * progress;
}

float shadeHoodStarBoost(float burnProgress) {
    const float progress = std::clamp(burnProgress, 0.0F, 1.0F);
    return SHADE_HOOD_BURNED_STAR_BOOST * progress * progress;
}

ShadeState shadeState(const Shade& shade) {
    if (shade.quietLeft > 0.0F) {
        return ShadeState::Banished;
    }
    if (shade.graceLeft > 0.0F) {
        return ShadeState::Grace;
    }
    if (shade.lit) {
        return ShadeState::Lit;
    }
    if (shade.thawLeft > 0.0F) {
        return ShadeState::Thawing;
    }
    if (shade.hunt == ShadeHunt::Chasing) {
        return ShadeState::Chasing;
    }
    return shade.hunt == ShadeHunt::Investigating ? ShadeState::Investigating
                                                  : ShadeState::Wandering;
}

float shadeSpeed(ShadeHunt hunt, const ShadeSettings& settings) {
    float speed = settings.wanderSpeed;
    if (hunt == ShadeHunt::Chasing) {
        speed = settings.speed;
    } else if (hunt == ShadeHunt::Investigating) {
        speed = settings.investigateSpeed;
    }
    return std::max(speed, 0.0F);
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
    return groundDistance(shade.position, playerFeet);
}

float shadeYawDegrees(const glm::vec3& position, const glm::vec3& playerFeet) {
    // A turn by an angle a around Y moves the direction +Z to (sin a, 0, cos a). So the
    // angle of a direction (x, z) is atan2(x, z). atan2(0, 0) is 0.
    return glm::degrees(std::atan2(playerFeet.x - position.x, playerFeet.z - position.z));
}

float shadeFacingDegrees(const Shade& shade, const glm::vec3& playerFeet) {
    const ShadeState state = shadeState(shade);
    const bool knowsThePlayer =
        state == ShadeState::Chasing || state == ShadeState::Lit || state == ShadeState::Thawing;
    return knowsThePlayer ? shadeYawDegrees(shade.position, playerFeet) : shade.headingDegrees;
}

float shadeDissolveHeight(float dissolveLeft) {
    const float t = std::clamp(dissolveLeft / SHADE_DISSOLVE_SECONDS, 0.0F, 1.0F);
    // The curve 3t^2 - 2t^3, as in caughtBrightness: it starts and ends flat.
    return t * t * (3.0F - 2.0F * t);
}

ShadeEvents advanceShade(Shade& shade, const ShadeSettings& settings, const Maze& maze,
                         const Terrain& terrain, const ShadeStep& step, float stepSeconds) {
    ShadeEvents events;
    if (!shade.present) {
        return events;
    }
    if (!settings.enabled) {
        shade = Shade{};
        return events;
    }
    shade.previousPosition = shade.position;
    // The figure that dissolves where the shade was banished: drawing only.
    shade.dissolveLeft = std::max(shade.dissolveLeft - stepSeconds, 0.0F);

    // Quiet after a banish: it stands, and it has no senses. No way is kept, so nothing
    // hums.
    if (shade.quietLeft > 0.0F) {
        shade.quietLeft -= stepSeconds;
        return events;
    }

    MazeCell own = cellAt(shade.position);
    const MazeCell playerCell = cellAt(step.playerFeet);
    if (!maze.contains(own.x, own.z)) {
        shade.wayMetres = SHADE_NO_WAY;
        return events;
    }
    // A player who flies out of the maze is not heard, not seen and not caught, and
    // passageDistances would throw for such a cell. The shade goes on by itself.
    const bool playerInMaze = maze.contains(playerCell.x, playerCell.z);
    if (playerInMaze) {
        updateWayToPlayer(shade, maze, step, playerCell);
    } else {
        shade.wayMetres = SHADE_NO_WAY;
    }

    // The grace time: it stands, and the light and its senses do not count yet. The way
    // to the player is kept all the same, because the hum is told how long it is.
    if (shade.graceLeft > 0.0F) {
        shade.graceLeft -= stepSeconds;
        return events;
    }

    // The light. Lit: the wait after the light starts anew, and the burn clock runs. Not
    // lit any more: that wait runs down, and the clock falls back.
    shade.lit = shadeLit(shade, step.lamp, step.obstacles);
    const bool thawing = !shade.lit && shade.thawLeft > 0.0F;
    if (shade.lit) {
        shade.thawLeft = settings.thawSeconds;
    } else if (thawing) {
        shade.thawLeft -= stepSeconds;
    }
    shade.burnSeconds = burnAfter(shade.burnSeconds, shade.lit, settings, stepSeconds);
    if (shade.lit && shade.burnSeconds >= settings.burnSeconds) {
        banish(shade, settings, maze, terrain, own, playerCell);
        events.banished = true;
        return events;
    }

    // Its senses: only while the light is not on it. A lit shade is held, and blind.
    bool sees = false;
    if (playerInMaze && !shade.lit) {
        events.alerted = sense(shade, settings, maze, step, playerCell, sees);
    }
    if (shade.lit || thawing) {
        return events;
    }

    // A wandering shade without a goal (at the start, after a banish) chooses one.
    if (!maze.contains(shade.goal.x, shade.goal.z)) {
        startWandering(shade, maze, own);
    }

    float reach = shadeSpeed(shade.hunt, settings) * stepSeconds;
    if (sees && own == playerCell) {
        // The same cell: nothing stands inside a cell, so straight at the player.
        shade.target = own;
        shade.position = walkTowards(shade.position, step.playerFeet, reach);
    } else if (walkToGoal(shade, maze, step.openedWalls, reach)) {
        // At the goal. A wandering shade walks on to the next cell of its choice. One
        // that came for the player waits, and gives up when the wait is over.
        if (shade.hunt == ShadeHunt::Wandering) {
            startWandering(shade, maze, cellAt(shade.position));
        } else {
            shade.searchLeft -= stepSeconds;
            if (shade.searchLeft <= 0.0F) {
                startWandering(shade, maze, cellAt(shade.position));
            }
        }
    }
    shade.position = onGround(terrain, shade.position);
    own = cellAt(shade.position);

    // Where it looks while it does not look at the player: the way it has just walked.
    if (groundDistance(shade.previousPosition, shade.position) > 0.0F) {
        shade.headingDegrees = shadeYawDegrees(shade.previousPosition, shade.position);
    }
    if (playerInMaze) {
        updateWayToPlayer(shade, maze, step, playerCell);
    }

    // Near enough, and nothing between the two: a player who stands against the other
    // side of a wall is not caught through it. Only a shade that nothing holds gets
    // here, so one that stands in the light or in its grace time catches nobody.
    events.caught = playerInMaze &&
                    shadeDistance(shade, step.playerFeet) <= settings.catchDistance &&
                    sameOrJoined(maze, own, playerCell);
    return events;
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

// The shortest swing the pose divides by (a period of 0 would divide by 0), and one half.
constexpr float MIN_SWAY_PERIOD_SECONDS = 0.01F;
constexpr float HALF = 0.5F;

ShadePose shadeSwayPose(float walkAmount, float speed, float seconds,
                        const ShadeSwaySettings& sway) {
    const float walk = std::clamp(walkAmount, 0.0F, 1.0F);
    // Standing: a slow swing to the side, and a breath that is a quarter of a swing
    // late, so the figure never rises exactly when it is most bent.
    const float swing =
        glm::two_pi<float>() * seconds / std::max(sway.periodSeconds, MIN_SWAY_PERIOD_SECONDS);
    const float sideStand = sway.standLeanDegrees * std::sin(swing);
    const float riseStand =
        sway.standRiseMetres * HALF * (1.0F + std::sin(swing - glm::half_pi<float>()));
    // Walking: one beat per step. |sin| makes one bump per step, as a bob does.
    const float beat = glm::two_pi<float>() * HALF * speed / SHADE_STRIDE_METRES * seconds;
    const float riseWalk = sway.walkBobMetres * std::abs(std::sin(beat));
    return {.riseMetres = glm::mix(riseStand, riseWalk, walk),
            .forwardLeanDegrees = sway.walkLeanDegrees * walk,
            .sideLeanDegrees = sideStand * (1.0F - walk)};
}

bool shadeWalking(const Shade& shade) {
    return shade.present && groundDistance(shade.previousPosition, shade.position) > 0.0F;
}

CatchPhase catchPhase(float secondsSinceCatch) {
    return secondsSinceCatch < CATCH_FADE_OUT_SECONDS ? CatchPhase::FadingOut : CatchPhase::Black;
}

float catchFadeBrightness(float secondsSinceCatch) {
    const float t = std::clamp(secondsSinceCatch / CATCH_FADE_OUT_SECONDS, 0.0F, 1.0F);
    return 1.0F - t * t * (3.0F - 2.0F * t);
}

float caughtBrightness(float secondsSinceCaught) {
    const float t = std::clamp(secondsSinceCaught / CAUGHT_FADE_SECONDS, 0.0F, 1.0F);
    // The curve 3t^2 - 2t^3: it starts and ends flat, so the light neither jumps on nor
    // stops with a corner.
    return t * t * (3.0F - 2.0F * t);
}

} // namespace game
