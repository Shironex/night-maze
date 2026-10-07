// Flasks: the flasks of tea a maze gets, which cells they lie in and how they glow.
#include "game/Flasks.hpp"

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

std::vector<MazeCell> placeFlasks(const Maze& maze, std::uint32_t seed, MazeCell start,
                                  MazeCell exit, std::span<const CrystalSpawn> crystals,
                                  int wantedCount) {
    // The free cells in two lists, each filled row after row: the order before the
    // shuffle is part of the result, like in placeCrystals.
    std::vector<MazeCell> deadEnds;
    std::vector<MazeCell> otherCells;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (cell == start || cell == exit || holdsCrystal(cell, crystals)) {
                continue;
            }
            if (isDeadEnd(maze, x, z)) {
                deadEnds.push_back(cell);
            } else {
                otherCells.push_back(cell);
            }
        }
    }

    std::mt19937 generator(seed + FLASK_SEED_OFFSET);
    shuffleCells(deadEnds, generator);
    shuffleCells(otherCells, generator);

    // One list of candidates: every dead end comes before every other cell. A cell is
    // in it once, so no cell can get two flasks.
    std::vector<MazeCell> candidates = deadEnds;
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
