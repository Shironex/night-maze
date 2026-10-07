// WallVariants: which wall segments of a maze look cracked, mossy or damaged.
#include "game/WallVariants.hpp"

#include "game/MazeGenerator.hpp"
#include "game/MazeLayout.hpp"

#include <algorithm>
#include <cstdlib>
#include <random>

namespace game {

namespace {

// The generator of the wall looks is seeded with the seed of the maze plus this number,
// so it does not repeat the numbers the maze, the crystals or the levers were made
// with. Any number other than theirs would do. Adding to an unsigned number wraps around
// at 2^32, which is well defined.
constexpr std::uint32_t WALL_VARIANT_SEED_OFFSET = 4000037U;

// A roll of the dice from 0 to 99 is compared with a number of walls out of 100.
constexpr std::uint32_t PERCENT = 100;

// The looks a worn wall can get: all of them but the plain one.
constexpr std::uint32_t WORN_VARIANT_COUNT = static_cast<std::uint32_t>(WALL_VARIANT_COUNT) - 1;

// From a wall to the middle of a cell it belongs to: half a cell, across the wall.
constexpr float HALF_CELL = CELL_SIZE / 2.0F;

// The two cells a wall segment stands between. A wall along X has one cell to the north
// (towards -Z) and one to the south, a wall along Z one to the west and one to the
// east. On the outer border one of the two is not a cell of the maze.
struct WallSides {
    MazeCell first;
    MazeCell second;
};

WallSides sidesOf(const WallSegment& segment) {
    const glm::vec3 across = segment.axis == WallAxis::AlongX ? glm::vec3{0.0F, 0.0F, HALF_CELL}
                                                              : glm::vec3{HALF_CELL, 0.0F, 0.0F};
    return {.first = cellAt(segment.position - across),
            .second = cellAt(segment.position + across)};
}

// True when the cell is in the maze and at most WALL_VARIANT_START_CLEARANCE columns
// and rows away from start.
bool isNearStart(const Maze& maze, MazeCell cell, MazeCell start) {
    return maze.contains(cell.x, cell.z) &&
           std::max(std::abs(cell.x - start.x), std::abs(cell.z - start.z)) <=
               WALL_VARIANT_START_CLEARANCE;
}

// True when the lever or the note with this mount hangs on the segment. The mount names
// a side of a cell, and wallSegmentOn gives the segment on that side exactly as
// wallSegments lists it, so the two can be compared number for number.
bool hangsOn(const WallRef& mount, const WallSegment& segment) {
    const WallSegment mounted = wallSegmentOn(mount.cell.x, mount.cell.z, mount.side);
    return mounted.position == segment.position && mounted.axis == segment.axis;
}

// True when a lever or a note hangs on the segment.
bool carriesSomething(const Interactables& interactables, const WallSegment& segment) {
    const auto onSegment = [&segment](const auto& thing) { return hangsOn(thing.mount, segment); };
    return std::ranges::any_of(interactables.levers, onSegment) ||
           std::ranges::any_of(interactables.notes, onSegment);
}

} // namespace

std::vector<WallVariant> chooseWallVariants(const Maze& maze, std::uint32_t seed, MazeCell start,
                                            const Interactables& interactables) {
    const std::vector<WallSegment> segments = wallSegments(maze);
    std::mt19937 generator(seed + WALL_VARIANT_SEED_OFFSET);

    std::vector<WallVariant> variants;
    variants.reserve(segments.size());
    for (const WallSegment& segment : segments) {
        // Both numbers are drawn for every wall, before any rule is looked at: the
        // numbers a wall gets then depend on its place in the list alone.
        const std::uint32_t roll = randomBelow(generator, PERCENT);
        const std::uint32_t worn = randomBelow(generator, WORN_VARIANT_COUNT);

        const WallSides sides = sidesOf(segment);
        const bool border = !maze.contains(sides.first.x, sides.first.z) ||
                            !maze.contains(sides.second.x, sides.second.z);
        const std::uint32_t chance =
            border ? BORDER_WALL_VARIANT_PERCENT : INNER_WALL_VARIANT_PERCENT;

        const bool staysPlain = roll >= chance || isNearStart(maze, sides.first, start) ||
                                isNearStart(maze, sides.second, start) ||
                                carriesSomething(interactables, segment);
        // The worn looks have the numbers 1 to 3 in the enum, right after Plain.
        variants.push_back(staysPlain ? WallVariant::Plain : static_cast<WallVariant>(worn + 1));
    }
    return variants;
}

std::array<int, WALL_VARIANT_COUNT> countWallVariants(std::span<const WallVariant> variants) {
    std::array<int, WALL_VARIANT_COUNT> counts{};
    for (const WallVariant variant : variants) {
        ++counts.at(static_cast<std::size_t>(variant));
    }
    return counts;
}

} // namespace game
