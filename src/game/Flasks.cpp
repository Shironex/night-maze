// Flasks: the flasks of tea a maze gets, which cells they lie in and how they glow.
#include "game/Flasks.hpp"

#include "game/Exit.hpp"
#include "game/MazeLayout.hpp"

#include <algorithm>
#include <cstddef>
#include <random>

namespace game {

namespace {

// The generator of the flasks is seeded with the seed of the maze plus this number, so
// it repeats the numbers of no other generator: the crystals add 1000003, the grass
// 2000003, the notes 3000017, the puddles 4000037 and the levers 5000011. Adding to an
// unsigned number wraps around at 2^32, which is well defined.
constexpr std::uint32_t FLASK_SEED_OFFSET = 6000011U;

// True when one of the crystals floats in the cell.
bool holdsCrystal(MazeCell cell, std::span<const CrystalSpawn> crystals) {
    return std::ranges::any_of(
        crystals, [cell](const CrystalSpawn& crystal) { return crystal.cell == cell; });
}

} // namespace

std::vector<MazeCell> flaskDeadEnds(const Maze& maze, std::uint32_t seed, MazeCell start,
                                    MazeCell exit) {
    // The dead ends in row order, then shuffled by the generator of the flasks: the
    // order before the shuffle is part of the result, like in placeCrystals.
    std::vector<MazeCell> deadEnds;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (cell != start && cell != exit && isDeadEnd(maze, x, z)) {
                deadEnds.push_back(cell);
            }
        }
    }
    std::mt19937 generator(seed + FLASK_SEED_OFFSET);
    shuffleCells(deadEnds, generator);

    // The far dead ends first: stable_partition moves the ones that pass the test to
    // the front and keeps the shuffled order inside both groups. A far dead end is one
    // at least half as many passages from the start as the farthest cell of the maze.
    const std::vector<int> distances = passageDistances(maze, start);
    const int farthest = *std::ranges::max_element(distances);
    std::ranges::stable_partition(deadEnds, [&](MazeCell cell) {
        const int index = cell.z * maze.width() + cell.x;
        return distances[static_cast<std::size_t>(index)] * 2 >= farthest;
    });
    return deadEnds;
}

std::vector<MazeCell> placeFlasks(const Maze& maze, std::uint32_t seed, MazeCell start,
                                  MazeCell exit, std::span<const CrystalSpawn> crystals,
                                  int wantedCount) {
    // The dead ends that hold no crystal, in the order of flaskDeadEnds, and then the
    // other free cells in row order.
    std::vector<MazeCell> candidates;
    for (const MazeCell cell : flaskDeadEnds(maze, seed, start, exit)) {
        if (!holdsCrystal(cell, crystals)) {
            candidates.push_back(cell);
        }
    }
    std::vector<MazeCell> otherCells;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (cell != start && cell != exit && !holdsCrystal(cell, crystals) &&
                !isDeadEnd(maze, x, z)) {
                otherCells.push_back(cell);
            }
        }
    }

    // The fallback when the dead ends run out: the other cells, shuffled by the seed.
    // A cell is in the list once, so no cell can get two flasks.
    std::mt19937 generator(seed + FLASK_SEED_OFFSET);
    shuffleCells(otherCells, generator);
    candidates.insert(candidates.end(), otherCells.begin(), otherCells.end());

    // Never more than there are candidates. resize cuts the list off after the first
    // count cells.
    const auto wanted = static_cast<std::size_t>(std::clamp(wantedCount, 0, MAX_FLASK_COUNT));
    candidates.resize(std::min(wanted, candidates.size()));
    return candidates;
}

glm::vec3 flaskRestPosition(MazeCell cell, float groundHeight) {
    return cellCenter(cell.x, cell.z) + glm::vec3{0.0F, groundHeight + FLASK_FLOAT_HEIGHT, 0.0F};
}

} // namespace game
