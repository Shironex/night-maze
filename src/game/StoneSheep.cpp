// StoneSheep: the flock that went to stone with the hedge. Which cells of a maze hold
// a sheep, where in its cell each one stands and which way it looks: towards the gate.
#include "game/StoneSheep.hpp"

#include "game/Crystals.hpp"
#include "game/EnvironmentMapping.hpp"
#include "game/Exit.hpp"
#include "game/Flasks.hpp"
#include "game/GateLamp.hpp"
#include "game/MazeGenerator.hpp"
#include "game/MazeWorld.hpp"
#include "game/Puddles.hpp"
#include "scene/Transform.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>

namespace game {

namespace {

// The generator of the sheep is seeded with the seed of the maze plus this number, which
// no other generator adds (the largest of the others is the 9000011 of the shade).
constexpr std::uint32_t STONE_SHEEP_SEED_OFFSET = 11000027U;

// From 16 by 16 cells on a maze gets a fourth sheep, from 22 by 22 on a fifth.
constexpr int FOUR_SHEEP_CELLS = 16 * 16;
constexpr int FIVE_SHEEP_CELLS = 22 * 22;

constexpr float DEGREES_PER_COMPASS_STEP = 45.0F;
constexpr float FULL_TURN_DEGREES = 360.0F;
// The model looks east, which is a quarter turn clockwise from north.
constexpr float MODEL_FACING_DEGREES = 90.0F;

// The direction as a step on the ground: x towards the east and z towards the south.
glm::vec2 sideStep(Direction side) {
    return {static_cast<float>(columnStep(side)), static_cast<float>(rowStep(side))};
}

bool isDiagonal(Compass facing) {
    return facing == Compass::NorthEast || facing == Compass::SouthEast ||
           facing == Compass::SouthWest || facing == Compass::NorthWest;
}

// Looking north or south, the sheep is long along Z.
bool isAlongZ(Compass facing) {
    return facing == Compass::North || facing == Compass::South;
}

// The first or, with otherPlace and when there are two, the second of the places.
std::optional<glm::vec2> pick(std::span<const glm::vec2> places, bool otherPlace) {
    if (places.empty()) {
        return std::nullopt;
    }
    return places[otherPlace && places.size() > 1 ? 1 : 0];
}

// The place across a corner, for a sheep that looks along a diagonal.
std::optional<glm::vec2> cornerPlace(const Maze& maze, MazeCell cell, Compass facing,
                                     bool otherPlace) {
    // A sheep stands across a corner when the line into the corner is square to the way
    // it looks: looking north-east it fits the north-west and the south-east corner.
    using Corner = std::pair<Direction, Direction>;
    const bool risesEast = facing == Compass::NorthEast || facing == Compass::SouthWest;
    const std::array<Corner, 2> corners =
        risesEast ? std::array<Corner, 2>{Corner{Direction::North, Direction::West},
                                          Corner{Direction::South, Direction::East}}
                  : std::array<Corner, 2>{Corner{Direction::North, Direction::East},
                                          Corner{Direction::South, Direction::West}};
    std::vector<glm::vec2> places;
    for (const Corner& corner : corners) {
        if (maze.hasWall(cell.x, cell.z, corner.first) &&
            maze.hasWall(cell.x, cell.z, corner.second)) {
            places.push_back((sideStep(corner.first) + sideStep(corner.second)) *
                             STONE_SHEEP_TO_CORNER);
        }
    }
    return pick(places, otherPlace);
}

// The place along a wall, for a sheep that looks north, east, south or west.
std::optional<glm::vec2> wallPlace(const Maze& maze, MazeCell cell, Compass facing,
                                   bool otherPlace) {
    // The walls that run the way the sheep looks, and the two ends of that way.
    const bool alongZ = isAlongZ(facing);
    const std::array<Direction, 2> walls =
        alongZ ? std::array<Direction, 2>{Direction::West, Direction::East}
               : std::array<Direction, 2>{Direction::North, Direction::South};
    const std::array<Direction, 2> ends =
        alongZ ? std::array<Direction, 2>{Direction::North, Direction::South}
               : std::array<Direction, 2>{Direction::West, Direction::East};

    // Towards the closed end and away from the open one.
    glm::vec2 along{0.0F};
    for (const Direction end : ends) {
        if (maze.hasWall(cell.x, cell.z, end)) {
            along += sideStep(end) * STONE_SHEEP_ALONG_WALL;
        }
    }
    std::vector<glm::vec2> places;
    for (const Direction wall : walls) {
        if (maze.hasWall(cell.x, cell.z, wall)) {
            places.push_back(sideStep(wall) * STONE_SHEEP_TO_WALL + along);
        }
    }
    return pick(places, otherPlace);
}

// One flag per cell of a maze, row after row.
class CellFlags {
public:
    explicit CellFlags(const Maze& maze)
        : m_maze(&maze),
          m_flags(static_cast<std::size_t>(maze.width()) * static_cast<std::size_t>(maze.height()),
                  false) {}

    // A cell outside the maze is skipped: the cell in front of a gate always exists, but
    // nothing here has to rely on it.
    void set(MazeCell cell) {
        if (m_maze->contains(cell.x, cell.z)) {
            m_flags[index(cell)] = true;
        }
    }
    bool has(MazeCell cell) const { return m_flags[index(cell)]; }

private:
    std::size_t index(MazeCell cell) const {
        return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(m_maze->width()) +
               static_cast<std::size_t>(cell.x);
    }

    const Maze* m_maze;
    std::vector<bool> m_flags;
};

// The cells no sheep may stand in, whatever their shape.
CellFlags takenCells(const MazeWorld& world) {
    const Maze& maze = world.maze;
    CellFlags taken(maze);
    taken.set(START_CELL);
    taken.set(world.exitCell);
    if (world.hasGate) {
        // The milestone stands there, and the gatehouse reaches into it.
        taken.set(approachCell(world));
    }
    for (const CrystalSpawn& crystal : world.crystals) {
        taken.set(crystal.cell);
    }
    for (const Lever& lever : world.interactables.levers) {
        taken.set(lever.mount.cell);
        // The two cells of the wall the lever opens: a ring hangs in each, and a side of
        // each is a way through once the wall is down.
        for (const WallRef& ring : slabRingMounts(lever)) {
            taken.set(ring.cell);
        }
    }
    for (const Note& note : world.interactables.notes) {
        taken.set(note.mount.cell);
    }
    for (const PuddleSpawn& puddle : placePuddles(maze, world.seed, START_CELL, world.exitCell,
                                                  world.crystals, DEFAULT_PUDDLE_SHARE)) {
        taken.set(puddle.cell);
    }

    // The flasks come first: the dead ends they have reserved, and the cells the flasks
    // of the three levels lie in (other cells only in a maze with too few dead ends).
    const std::vector<MazeCell> reserved =
        flaskDeadEnds(maze, world.seed, START_CELL, world.exitCell);
    for (std::size_t i = 0;
         i < reserved.size() && i < static_cast<std::size_t>(FLASK_RESERVED_DEAD_ENDS); ++i) {
        taken.set(reserved[i]);
    }
    for (const MazeCell cell : placeFlasks(maze, world.seed, START_CELL, world.exitCell,
                                           world.crystals, FLASK_RESERVED_DEAD_ENDS)) {
        taken.set(cell);
    }
    for (const MazeCell cell : farthestDeadEnds(maze, START_CELL, world.exitCell)) {
        taken.set(cell);
    }
    return taken;
}

} // namespace

int stoneSheepCountFor(int cellCount) {
    if (cellCount >= FIVE_SHEEP_CELLS) {
        return 5;
    }
    return cellCount >= FOUR_SHEEP_CELLS ? 4 : 3;
}

bool isCornerCell(const Maze& maze, MazeCell cell) {
    int openSides = 0;
    for (const Direction side : ALL_DIRECTIONS) {
        openSides += maze.hasWall(cell.x, cell.z, side) ? 0 : 1;
    }
    // Two open sides are either opposite each other (a straight passage) or not.
    const bool straight = maze.hasWall(cell.x, cell.z, Direction::North) ==
                          maze.hasWall(cell.x, cell.z, Direction::South);
    return openSides == 2 && !straight;
}

float sheepYawDegrees(Compass facing) {
    if (facing == Compass::Here) {
        return 0.0F;
    }
    // The compass counts clockwise from north and the yaw of a model counter clockwise.
    const float clockwise =
        static_cast<float>(static_cast<int>(facing) - static_cast<int>(Compass::North)) *
        DEGREES_PER_COMPASS_STEP;
    return std::fmod(MODEL_FACING_DEGREES - clockwise + FULL_TURN_DEGREES, FULL_TURN_DEGREES);
}

glm::vec2 stoneSheepBoxHalfSize(Compass facing) {
    if (isDiagonal(facing)) {
        return glm::vec2{STONE_SHEEP_DIAGONAL_HALF};
    }
    const glm::vec2 alongX{STONE_SHEEP_LENGTH / 2.0F, STONE_SHEEP_WIDTH / 2.0F};
    return isAlongZ(facing) ? glm::vec2{alongX.y, alongX.x} : alongX;
}

std::optional<glm::vec2> stoneSheepPlace(const Maze& maze, MazeCell cell, Compass facing,
                                         bool otherPlace) {
    if (facing == Compass::Here) {
        return std::nullopt;
    }
    return isDiagonal(facing) ? cornerPlace(maze, cell, facing, otherPlace)
                              : wallPlace(maze, cell, facing, otherPlace);
}

scene::Aabb stoneSheepBox(const glm::vec3& position, Compass facing) {
    const glm::vec2 half = stoneSheepBoxHalfSize(facing);
    return {.min = position - glm::vec3{half.x, STONE_SHEEP_BOX_DEPTH, half.y},
            .max = position + glm::vec3{half.x, STONE_SHEEP_BOX_HEIGHT, half.y}};
}

glm::mat4 stoneSheepMatrix(const StoneSheep& sheep) {
    scene::Transform transform;
    transform.position = sheep.position;
    transform.rotationDegrees.y = sheepYawDegrees(sheep.facing);
    return transform.matrix();
}

std::vector<MazeCell> farthestDeadEnds(const Maze& maze, MazeCell start, MazeCell exit) {
    const std::vector<int> distances = passageDistances(maze, start);
    std::vector<MazeCell> farthest;
    int farthestDistance = 0;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            const int index = z * maze.width() + x;
            const int distance = distances[static_cast<std::size_t>(index)];
            if (cell == start || cell == exit || distance == UNREACHABLE ||
                !isDeadEnd(maze, x, z) || distance < farthestDistance) {
                continue;
            }
            if (distance > farthestDistance) {
                farthest.clear();
                farthestDistance = distance;
            }
            farthest.push_back(cell);
        }
    }
    return farthest;
}

std::vector<MazeCell> chooseSpreadCells(const Maze& maze, std::span<const MazeCell> candidates,
                                        int count) {
    std::vector<MazeCell> chosen;
    if (count < 1) {
        return chosen;
    }
    const auto wanted = static_cast<std::size_t>(count);
    // For every chosen cell the passages from it to every other cell: one search each.
    std::vector<std::vector<int>> ways;
    std::vector<MazeCell> passedOver;
    for (const MazeCell cell : candidates) {
        if (chosen.size() == wanted) {
            break;
        }
        const int index = cell.z * maze.width() + cell.x;
        const bool apart = std::ranges::all_of(ways, [index](const std::vector<int>& way) {
            return way[static_cast<std::size_t>(index)] > STONE_SHEEP_MIN_PASSAGES_APART;
        });
        if (apart) {
            chosen.push_back(cell);
            ways.push_back(passageDistances(maze, cell));
        } else {
            passedOver.push_back(cell);
        }
    }
    for (const MazeCell cell : passedOver) {
        if (chosen.size() == wanted) {
            break;
        }
        chosen.push_back(cell);
    }
    return chosen;
}

std::vector<StoneSheep> placeStoneSheep(const MazeWorld& world) {
    const Maze& maze = world.maze;
    const CellFlags taken = takenCells(world);

    // The cells that can hold a sheep, row after row: the order before the shuffle is
    // part of the result.
    std::vector<MazeCell> candidates;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (taken.has(cell) || !(isDeadEnd(maze, x, z) || isCornerCell(maze, cell))) {
                continue;
            }
            if (stoneSheepPlace(maze, cell, compassTowards(cell, world.exitCell), false)) {
                candidates.push_back(cell);
            }
        }
    }
    std::mt19937 generator(world.seed + STONE_SHEEP_SEED_OFFSET);
    shuffleCells(candidates, generator);

    std::vector<StoneSheep> flock;
    for (const MazeCell cell :
         chooseSpreadCells(maze, candidates, stoneSheepCountFor(maze.width() * maze.height()))) {
        StoneSheep sheep;
        sheep.cell = cell;
        sheep.facing = compassTowards(cell, world.exitCell);
        // Which of two walls it lies along is the choice of the seed.
        const bool otherPlace = randomBelow(generator, 2) == 1;
        const glm::vec2 place =
            stoneSheepPlace(maze, cell, sheep.facing, otherPlace).value_or(glm::vec2{0.0F});
        sheep.position = cellCenter(cell.x, cell.z) + glm::vec3{place.x, 0.0F, place.y};
        sheep.box = stoneSheepBox(sheep.position, sheep.facing);
        flock.push_back(sheep);
    }
    return flock;
}

bool stoneSheepBareGround(const MazeWorld& world, float x, float z) {
    return std::ranges::any_of(world.sheep, [x, z](const StoneSheep& sheep) {
        return x >= sheep.box.min.x - STONE_SHEEP_BARE_MARGIN &&
               x <= sheep.box.max.x + STONE_SHEEP_BARE_MARGIN &&
               z >= sheep.box.min.z - STONE_SHEEP_BARE_MARGIN &&
               z <= sheep.box.max.z + STONE_SHEEP_BARE_MARGIN;
    });
}

} // namespace game
