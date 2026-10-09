// Interactables: the levers and the notes of a maze, where they hang and what they do.
#include "game/Interactables.hpp"

#include "game/Exit.hpp"
#include "game/MazeGenerator.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <random>
#include <stdexcept>

namespace game {

namespace {

// The generators of the levers and of the notes are seeded with the seed of the maze
// plus these numbers, so they repeat neither the numbers the maze was carved with nor
// those of the crystals (which add 1000003), of the grass (2000003) or of the puddles
// (4000037), nor each other. Any numbers that differ from those and from each other
// would do. Adding to an unsigned number wraps around at 2^32, which is well defined.
constexpr std::uint32_t LEVER_SEED_OFFSET = 5000011U;
constexpr std::uint32_t NOTE_SEED_OFFSET = 3000017U;

// The two sides a cell reports its interior walls on. The wall on its north side is the
// south wall of the cell above it and the wall on its west side is the east wall of the
// cell to the left: those cells report them. Going through East and South of every cell
// meets every interior wall exactly once.
constexpr std::array<Direction, 2> REPORTED_SIDES = {Direction::East, Direction::South};

// compassTowards: an offset counts as "along an axis" when it is more than this many
// times the other offset. 2 puts the border at a slope of 1 in 2 (about 27 degrees),
// close to the 22.5 degrees that would cut the circle into eight equal parts, and it
// needs whole numbers only.
constexpr int STRAIGHT_FACTOR = 2;

// One line of the story.
struct StoryLine {
    std::string_view text;
    // True for a line about the shadow: a maze without a shade leaves it out.
    bool needsShade;
};

// The lines a story note (the kind Flavour) can show, in the order of the story: a maze
// reads them one after the other and starts again at the first one after the last.
// English, like the rest of the HUD. Eight of them are about the shadow.
constexpr std::array<StoryLine, 24> FLAVOUR_LINES = {{
    {.text = "The moon sees every corridor. You see one.", .needsShade = false},
    {.text = "A splinter remembers being moon. The lamp believes it.", .needsShade = false},
    {.text = "Dead ends are where the light hides.", .needsShade = false},
    {.text = "The gate counts what you carry. It never asks for all.", .needsShade = false},
    {.text = "Leave a few. The moon comes back to look for them.", .needsShade = false},
    {.text = "A lever moves a wall. Somewhere.", .needsShade = false},
    {.text = "Shepherds roped their gates under the turf. Pull. Listen.", .needsShade = false},
    {.text = "Chalk ground from a splinter. It leans. It ignores walls.", .needsShade = false},
    {.text = "The gate grows as far from the stile as it can.", .needsShade = false},
    {.text = "Lamp off saves the lamp. Something else is glad of it.", .needsShade = true},
    {.text = "It walks when you turn. It walks when the lamp sleeps.", .needsShade = true},
    {.text = "Shine on it and it is only ground. Look, and it waits.", .needsShade = true},
    {.text = "We played this as children. It learned the rules from us.", .needsShade = true},
    {.text = "If it reaches you, it only carries you back. Begin again.", .needsShade = true},
    {.text = "It does not mind the moon. The moon is where it lives.", .needsShade = true},
    {.text = "Each piece that falls leaves a hole. The hole comes after.", .needsShade = true},
    {.text = "It is not hunting you. It is looking in your pockets.", .needsShade = true},
    {.text = "Puddles hold stars. Stars are no use. Walk on.", .needsShade = false},
    {.text = "When the lamp stutters, it is forgetting. Feed it.", .needsShade = false},
    {.text = "Count your steps. The maze does not.", .needsShade = false},
    {.text = "My hands shake now. The lamp does not mind whose hand.", .needsShade = false},
    {.text = "I wrote these for whoever came next. I hoped for you.", .needsShade = false},
    {.text = "There is no way back. The gate is the way home.", .needsShade = false},
    {.text = "One lamp is enough, if it is the one still lit.", .needsShade = false},
}};

// A whole number brought into the range from 0 to count - 1 by going round: -1 becomes
// count - 1 (% of a negative number is negative, so count is added once).
int wrapped(int number, int count) {
    return ((number % count) + count) % count;
}

// Position of a cell in a list that holds the rows one after another, like in Maze.
std::size_t cellIndex(const Maze& maze, MazeCell cell) {
    return static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
           static_cast<std::size_t>(cell.x);
}

// The cell on the other side of a wall. It can lie outside the maze (for a border wall).
MazeCell cellBehind(const WallRef& wall) {
    return {.x = wall.cell.x + columnStep(wall.side), .z = wall.cell.z + rowStep(wall.side)};
}

// The same wall, named from the cell on its other side.
WallRef seenFromBehind(const WallRef& wall) {
    return {.cell = cellBehind(wall), .side = opposite(wall.side)};
}

// True when the wall is one of the walls in the list, under either of its two names.
bool isOneOf(const WallRef& wall, std::span<const WallRef> walls) {
    for (const WallRef& other : walls) {
        if (sameWall(wall, other)) {
            return true;
        }
    }
    return false;
}

// True when the cell is one of the cells in the list.
bool isOneOf(MazeCell cell, std::span<const MazeCell> cells) {
    for (const MazeCell other : cells) {
        if (cell == other) {
            return true;
        }
    }
    return false;
}

// The sides of a cell something can hang on: every side that has a wall, except a wall
// that a lever opens. In the fixed order of ALL_DIRECTIONS.
std::vector<Direction> mountableSides(const Maze& maze, MazeCell cell,
                                      std::span<const WallRef> openedByLevers) {
    std::vector<Direction> sides;
    for (const Direction side : ALL_DIRECTIONS) {
        if (!maze.hasWall(cell.x, cell.z, side)) {
            continue;
        }
        if (isOneOf(WallRef{.cell = cell, .side = side}, openedByLevers)) {
            continue;
        }
        sides.push_back(side);
    }
    return sides;
}

// One of the mountable sides of a cell, each with the same chance. The cell must have
// at least one (the callers only keep such cells).
Direction randomMountSide(const Maze& maze, MazeCell cell, std::span<const WallRef> openedByLevers,
                          std::mt19937& generator) {
    const std::vector<Direction> sides = mountableSides(maze, cell, openedByLevers);
    return sides[randomBelow(generator, static_cast<std::uint32_t>(sides.size()))];
}

// A wanted number brought into the range from 0 to largest.
std::size_t clampedCount(int wanted, int largest) {
    return static_cast<std::size_t>(std::clamp(wanted, 0, largest));
}

// The point on the visible face of a wall, in the middle of the wall, mountHeight above
// the ground.
glm::vec3 mountPosition(const WallRef& mount, float groundHeight, float mountHeight) {
    // From the centre of the cell towards the wall: half a cell would reach the middle
    // of the wall (it stands on the cell border), half of its thickness less reaches
    // the face that looks into the cell.
    const float toFace = CELL_SIZE / 2.0F - WALL_VISUAL_THICKNESS / 2.0F;
    const glm::vec3 towardsWall{static_cast<float>(columnStep(mount.side)), 0.0F,
                                static_cast<float>(rowStep(mount.side))};
    return cellCenter(mount.cell.x, mount.cell.z) + towardsWall * toFace +
           glm::vec3{0.0F, groundHeight + mountHeight, 0.0F};
}

// The pick box of something that hangs at position on the given side of its cell. The
// box starts on the wall and reaches depth into the cell.
scene::Aabb mountBox(const glm::vec3& position, Direction side, float width, float height,
                     float depth) {
    const glm::vec3 towardsWall{static_cast<float>(columnStep(side)), 0.0F,
                                static_cast<float>(rowStep(side))};
    // The position is on the wall, the centre of the box is half of the depth away
    // from the wall.
    const glm::vec3 center = position - towardsWall * (depth / 2.0F);

    // A wall on the north or south side of a cell runs along X: the width of the box is
    // then along X and its depth along Z. On the east or west side it is the other way.
    const bool wallRunsAlongX = side == Direction::North || side == Direction::South;
    const glm::vec3 halfExtents = wallRunsAlongX
                                      ? glm::vec3{width / 2.0F, height / 2.0F, depth / 2.0F}
                                      : glm::vec3{depth / 2.0F, height / 2.0F, width / 2.0F};
    return scene::Aabb::fromCenter(center, halfExtents);
}

// The cell of the list that is nearest to from, measured along the straight line on the
// grid. The list must not be empty. Of two cells equally near the earlier one wins.
MazeCell nearestCell(MazeCell from, std::span<const MazeCell> cells) {
    MazeCell nearest = cells.front();
    // Squared distances are compared: the order is the same as for the distances
    // themselves, and no square root (and so no float) is needed.
    int nearestSquared = -1;
    for (const MazeCell cell : cells) {
        const int columnOffset = cell.x - from.x;
        const int rowOffset = cell.z - from.z;
        const int squared = columnOffset * columnOffset + rowOffset * rowOffset;
        // -1 marks "nothing found yet". "Less", not "less or equal": the earlier cell
        // of two at the same distance stays.
        if (nearestSquared < 0 || squared < nearestSquared) {
            nearest = cell;
            nearestSquared = squared;
        }
    }
    return nearest;
}

// The sentence of a hint: who, what it does, then the direction. For example
// hintSentence("The gate", "waits", Compass::North) is "The gate waits to the north.".
std::string hintSentence(std::string_view subject, std::string_view verb, Compass direction) {
    std::string text{subject};
    text += ' ';
    text += verb;
    if (direction == Compass::Here) {
        text += " right here.";
        return text;
    }
    text += " to the ";
    text += compassName(direction);
    text += '.';
    return text;
}

} // namespace

bool sameWall(const WallRef& a, const WallRef& b) {
    return a == b || a == seenFromBehind(b);
}

bool isInteriorWall(const Maze& maze, const WallRef& wall) {
    const MazeCell behind = cellBehind(wall);
    // hasWall throws for a cell outside the maze.
    return maze.hasWall(wall.cell.x, wall.cell.z, wall.side) && maze.contains(behind.x, behind.z);
}

std::vector<WallRef> chooseShortcutWalls(const Maze& maze, MazeCell start, MazeCell exit,
                                         int count) {
    if (!maze.contains(start.x, start.z) || !maze.contains(exit.x, exit.z)) {
        throw std::out_of_range("chooseShortcutWalls: the start or the exit is outside the maze");
    }

    // The copy loses a wall for every shortcut that is chosen. The maze of the caller
    // stays as it is: its walls go down only when a lever is pulled.
    Maze opened = maze;
    const std::size_t wanted = clampedCount(count, MAX_LEVER_COUNT);

    std::vector<WallRef> chosen;
    while (chosen.size() < wanted) {
        // How many passages every cell is from the start, with the shortcuts chosen so
        // far open.
        const std::vector<int> distances = passageDistances(opened, start);

        // The best wall of this round. bestSaved starts just below the smallest score
        // that is worth a lever, so a wall has to reach LEVER_MIN_STEPS_SAVED to be
        // taken at all.
        bool found = false;
        WallRef best;
        int bestSaved = LEVER_MIN_STEPS_SAVED - 1;

        // Row after row: the order is part of the result, because of two walls with the
        // same score the earlier one stays ("greater" below, not "greater or equal").
        for (int z = 0; z < opened.height(); ++z) {
            for (int x = 0; x < opened.width(); ++x) {
                for (const Direction side : REPORTED_SIDES) {
                    const WallRef wall{.cell = {.x = x, .z = z}, .side = side};
                    // An open side, or a wall of the outer border.
                    if (!isInteriorWall(opened, wall)) {
                        continue;
                    }
                    // A wall of the exit cell: the gate must stay the only way in.
                    const MazeCell behind = cellBehind(wall);
                    if (wall.cell == exit || behind == exit) {
                        continue;
                    }
                    // A cell that cannot be reached at all (only in a maze built by
                    // hand) has no distance to compare.
                    const int distanceHere = distances[cellIndex(opened, wall.cell)];
                    const int distanceBehind = distances[cellIndex(opened, behind)];
                    if (distanceHere == UNREACHABLE || distanceBehind == UNREACHABLE) {
                        continue;
                    }

                    // With the wall gone, the far cell is one passage behind the near
                    // one. Before, it was std::abs(...) passages behind it.
                    const int saved = std::abs(distanceHere - distanceBehind) - 1;
                    if (saved > bestSaved) {
                        found = true;
                        best = wall;
                        bestSaved = saved;
                    }
                }
            }
        }

        // No wall is worth a lever any more: the maze gets fewer levers than wanted.
        if (!found) {
            break;
        }
        chosen.push_back(best);
        opened.removeWall(best.cell.x, best.cell.z, best.side);
    }
    return chosen;
}

Interactables placeInteractables(const Maze& maze, std::uint32_t seed, MazeCell start,
                                 MazeCell exit, std::span<const CrystalSpawn> crystals,
                                 const InteractableSettings& settings) {
    // chooseShortcutWalls checks that start and exit are cells of the maze.
    const std::vector<WallRef> shortcuts =
        chooseShortcutWalls(maze, start, exit, settings.leverCount);

    // The cells that hold a crystal.
    std::vector<MazeCell> crystalCells;
    crystalCells.reserve(crystals.size());
    for (const CrystalSpawn& crystal : crystals) {
        crystalCells.push_back(crystal.cell);
    }

    // ---- the levers -----------------------------------------------------------------
    // The cells a lever can hang in, in two lists, each filled row after row: the order
    // before the shuffle is part of the result, like in placeCrystals.
    std::vector<MazeCell> cellsWithoutCrystal;
    std::vector<MazeCell> cellsWithCrystal;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (cell == start || cell == exit) {
                continue;
            }
            // A cell in the middle of a crossing can be without any wall to hang on.
            if (mountableSides(maze, cell, shortcuts).empty()) {
                continue;
            }
            if (isOneOf(cell, crystalCells)) {
                cellsWithCrystal.push_back(cell);
            } else {
                cellsWithoutCrystal.push_back(cell);
            }
        }
    }

    // The order of the random draws is fixed, and it is part of the result: first the
    // two shuffles, then one draw per lever for its side.
    std::mt19937 leverGenerator(seed + LEVER_SEED_OFFSET);
    shuffleCells(cellsWithoutCrystal, leverGenerator);
    shuffleCells(cellsWithCrystal, leverGenerator);

    // One list of candidates: every cell without a crystal comes before every cell with
    // one. A cell is in it once, so no cell can get two levers.
    std::vector<MazeCell> leverCells = cellsWithoutCrystal;
    leverCells.insert(leverCells.end(), cellsWithCrystal.begin(), cellsWithCrystal.end());

    // One lever per shortcut, or fewer when the cells run out (a very small maze). The
    // best shortcuts are first in the list, so those are the ones that keep their lever.
    const std::size_t leverCount = std::min(shortcuts.size(), leverCells.size());

    Interactables interactables;
    interactables.levers.reserve(leverCount);
    std::vector<MazeCell> takenCells;
    for (std::size_t i = 0; i < leverCount; ++i) {
        Lever lever;
        lever.mount.cell = leverCells[i];
        lever.mount.side = randomMountSide(maze, lever.mount.cell, shortcuts, leverGenerator);
        lever.position = leverPosition(lever.mount, 0.0F);
        lever.box = leverBox(lever.position, lever.mount.side);
        lever.opens = shortcuts[i];
        interactables.levers.push_back(lever);
        takenCells.push_back(lever.mount.cell);
    }

    // ---- the notes ------------------------------------------------------------------
    // Every cell that is free and has a wall to hang on, row after row. A cell with
    // a crystal is fine for a note.
    std::vector<MazeCell> noteCells;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const MazeCell cell{.x = x, .z = z};
            if (cell == start || cell == exit || isOneOf(cell, takenCells)) {
                continue;
            }
            if (mountableSides(maze, cell, shortcuts).empty()) {
                continue;
            }
            noteCells.push_back(cell);
        }
    }

    // The order of the draws: the shuffle of the cells, then one draw per note for its
    // side.
    std::mt19937 noteGenerator(seed + NOTE_SEED_OFFSET);
    shuffleCells(noteCells, noteGenerator);

    const std::size_t noteCount =
        std::min(clampedCount(settings.noteCount, MAX_NOTE_COUNT), noteCells.size());
    interactables.notes.reserve(noteCount);

    // The mix: how many of the notes tell the story, and how many are hints. Without
    // a number of its own every third note is a story note.
    const std::size_t kindCount = NOTE_KIND_COUNT;
    std::size_t storyLeft =
        settings.storyNoteCount == NOTE_MIX_BY_TURNS
            ? noteCount / kindCount
            : std::min(clampedCount(settings.storyNoteCount, MAX_NOTE_COUNT), noteCount);
    std::size_t hintsLeft = noteCount - storyLeft;
    std::size_t hintsPlaced = 0;
    for (std::size_t i = 0; i < noteCount; ++i) {
        Note note;
        note.mount.cell = noteCells[i];
        note.mount.side = randomMountSide(maze, note.mount.cell, shortcuts, noteGenerator);
        note.position = notePosition(note.mount, 0.0F);
        note.box = noteBox(note.position, note.mount.side);
        // The kinds take turns: exit hint, crystal hint, story, exit hint and so on.
        // The turn of a kind that is used up goes to the other: a story note takes the
        // turn of a hint when no hint is left, and a hint the turn of a story note.
        const bool storyTurn = static_cast<NoteKind>(i % kindCount) == NoteKind::Flavour;
        if ((storyTurn && storyLeft > 0) || hintsLeft == 0) {
            note.kind = NoteKind::Flavour;
            --storyLeft;
        } else {
            // The hints take turns among themselves: the first one points to the exit,
            // the second to a crystal.
            note.kind = hintsPlaced % 2 == 0 ? NoteKind::ExitHint : NoteKind::CrystalHint;
            ++hintsPlaced;
            --hintsLeft;
        }
        interactables.notes.push_back(note);
    }

    // The story notes (the kind Flavour) get their lines nearest to the start first. The
    // numbers of these notes are put in the order of their distance. The sort is stable,
    // so two notes the same number of passages away keep the order they have in the list
    // (the order of the shuffle): the result never depends on the standard library.
    const std::vector<int> distances = passageDistances(maze, start);
    std::vector<std::size_t> storyNotes;
    for (std::size_t i = 0; i < interactables.notes.size(); ++i) {
        if (interactables.notes[i].kind == NoteKind::Flavour) {
            storyNotes.push_back(i);
        }
    }
    std::ranges::stable_sort(storyNotes, [&](std::size_t first, std::size_t second) {
        return distances[cellIndex(maze, interactables.notes[first].mount.cell)] <
               distances[cellIndex(maze, interactables.notes[second].mount.cell)];
    });
    for (std::size_t rank = 0; rank < storyNotes.size(); ++rank) {
        interactables.notes[storyNotes[rank]].flavourIndex =
            storyLineFor(settings.firstStoryLine, static_cast<int>(rank), settings.shadeLines);
    }
    return interactables;
}

glm::vec3 leverPosition(const WallRef& mount, float groundHeight) {
    return mountPosition(mount, groundHeight, LEVER_MOUNT_HEIGHT);
}

scene::Aabb leverBox(const glm::vec3& position, Direction side) {
    return mountBox(position, side, LEVER_BOX_WIDTH, LEVER_BOX_HEIGHT, LEVER_BOX_DEPTH);
}

glm::vec3 notePosition(const WallRef& mount, float groundHeight) {
    return mountPosition(mount, groundHeight, NOTE_MOUNT_HEIGHT);
}

scene::Aabb noteBox(const glm::vec3& position, Direction side) {
    return mountBox(position, side, NOTE_BOX_WIDTH, NOTE_BOX_HEIGHT, NOTE_BOX_DEPTH);
}

void placeInteractablesOnTerrain(Interactables& interactables, const Terrain& terrain) {
    // The x and the z of a position never change, so the old position says where to
    // look up the ground, whatever height it had before.
    for (Lever& lever : interactables.levers) {
        const float ground = terrain.heightAt(lever.position.x, lever.position.z);
        lever.position = leverPosition(lever.mount, ground);
        lever.box = leverBox(lever.position, lever.mount.side);
    }
    for (Note& note : interactables.notes) {
        const float ground = terrain.heightAt(note.position.x, note.position.z);
        note.position = notePosition(note.mount, ground);
        note.box = noteBox(note.position, note.mount.side);
    }
}

InteractableState startInteractables(const Interactables& interactables) {
    InteractableState state;
    state.leverPulled.assign(interactables.levers.size(), false);
    return state;
}

bool isLeverPulled(const InteractableState& state, std::size_t index) {
    if (index >= state.leverPulled.size()) {
        throw std::out_of_range("isLeverPulled: there is no lever with this number");
    }
    return state.leverPulled[index];
}

PullResult pullLever(InteractableState& state, const Interactables& interactables,
                     std::size_t index) {
    if (index >= interactables.levers.size() || index >= state.leverPulled.size()) {
        throw std::out_of_range("pullLever: there is no lever with this number");
    }
    // Pulled before: nothing happens, and nothing is reported.
    if (state.leverPulled[index]) {
        return {};
    }
    state.leverPulled[index] = true;
    return {.opened = true, .wall = interactables.levers[index].opens};
}

std::vector<WallRef> openedWalls(const Interactables& interactables,
                                 const InteractableState& state) {
    std::vector<WallRef> walls;
    // The shorter of the two lists limits the loop, so a state that was started for
    // other interactables cannot make it read past the end.
    const std::size_t count = std::min(interactables.levers.size(), state.leverPulled.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (state.leverPulled[i]) {
            walls.push_back(interactables.levers[i].opens);
        }
    }
    return walls;
}

WallSegment openedWallSegment(const Lever& lever) {
    return wallSegmentOn(lever.opens.cell.x, lever.opens.cell.z, lever.opens.side);
}

PickedInteractable pickInteractable(const scene::Ray& ray, const Interactables& interactables,
                                    std::span<const scene::Aabb> blockers, float reach) {
    // The pick boxes as plain lists, in the order of the levers and of the notes, so the
    // index of a hit is the number of the lever or of the note.
    std::vector<scene::Aabb> leverBoxes;
    leverBoxes.reserve(interactables.levers.size());
    for (const Lever& lever : interactables.levers) {
        leverBoxes.push_back(lever.box);
    }
    std::vector<scene::Aabb> noteBoxes;
    noteBoxes.reserve(interactables.notes.size());
    for (const Note& note : interactables.notes) {
        noteBoxes.push_back(note.box);
    }

    const scene::NearestHit lever = scene::nearestHit(ray, leverBoxes, reach);
    const scene::NearestHit note = scene::nearestHit(ray, noteBoxes, reach);

    // The nearer of the two. A note has to be strictly nearer to win against a lever.
    PickedInteractable picked;
    if (lever.hit) {
        picked = {
            .kind = InteractableKind::Lever, .index = lever.index, .distance = lever.distance};
    }
    if (note.hit && (!lever.hit || note.distance < lever.distance)) {
        picked = {.kind = InteractableKind::Note, .index = note.index, .distance = note.distance};
    }
    if (picked.kind == InteractableKind::None) {
        return picked;
    }

    // Something that hides it: a blocker the ray reaches first. Blockers farther away
    // than the picked box do not matter, so its distance is the reach of this search.
    // A blocker at exactly the same distance does not hide it either.
    const scene::NearestHit blocker = scene::nearestHit(ray, blockers, picked.distance);
    if (blocker.hit && blocker.distance < picked.distance) {
        return {};
    }
    return picked;
}

Compass compassTowards(MazeCell from, MazeCell to) {
    // East is +X (a larger column), South is +Z (a larger row).
    const int columnOffset = to.x - from.x;
    const int rowOffset = to.z - from.z;
    if (columnOffset == 0 && rowOffset == 0) {
        return Compass::Here;
    }

    // How far apart the cells are in each of the two directions, without the sign.
    const int columns = std::abs(columnOffset);
    const int rows = std::abs(rowOffset);

    // Mostly sideways: the line is close to the east-west axis.
    if (columns > STRAIGHT_FACTOR * rows) {
        return columnOffset > 0 ? Compass::East : Compass::West;
    }
    // Mostly up or down the map: close to the north-south axis.
    if (rows > STRAIGHT_FACTOR * columns) {
        return rowOffset > 0 ? Compass::South : Compass::North;
    }

    // What is left is a diagonal, and both offsets are not 0 here: with one of them 0
    // one of the two tests above would have been true.
    if (rowOffset < 0) {
        return columnOffset > 0 ? Compass::NorthEast : Compass::NorthWest;
    }
    return columnOffset > 0 ? Compass::SouthEast : Compass::SouthWest;
}

std::string_view compassName(Compass direction) {
    switch (direction) {
    case Compass::Here:
        return "here";
    case Compass::North:
        return "north";
    case Compass::NorthEast:
        return "north-east";
    case Compass::East:
        return "east";
    case Compass::SouthEast:
        return "south-east";
    case Compass::South:
        return "south";
    case Compass::SouthWest:
        return "south-west";
    case Compass::West:
        return "west";
    case Compass::NorthWest:
        return "north-west";
    }
    // Not reached for a value of the enum. The compiler wants a return on every path.
    return "here";
}

int storyLineFor(int firstLine, int rank, bool shadeLines) {
    // The first line is brought into the table first, so a negative or a huge number
    // cannot give a place outside of it.
    const int count = flavourLineCount();
    int line = wrapped(firstLine, count);

    // How many lines this maze may show at all, and how many of them are passed before
    // the wanted one: the rank, going round when a maze has more notes than lines.
    const int shown = shadeLines ? count : oldStoryLineCount();
    int toPass = wrapped(rank, shown);

    // Line after line from the first one. A line this maze may show counts, a line
    // about the shadow in a maze without one is stepped over. The loop ends: there are
    // more lines a maze may show than toPass.
    while (true) {
        const bool mayShow =
            shadeLines || !FLAVOUR_LINES[static_cast<std::size_t>(line)].needsShade;
        if (mayShow) {
            if (toPass == 0) {
                return line;
            }
            --toPass;
        }
        line = (line + 1) % count;
    }
}

int storyNoteCount(const Interactables& interactables) {
    int count = 0;
    for (const Note& note : interactables.notes) {
        if (note.kind == NoteKind::Flavour) {
            ++count;
        }
    }
    return count;
}

int advanceStoryLine(int firstLine, int storyNotes, bool shadeLines) {
    const int count = flavourLineCount();
    if (storyNotes <= 0) {
        return wrapped(firstLine, count);
    }
    // One past the line the last story note showed.
    return (storyLineFor(firstLine, storyNotes - 1, shadeLines) + 1) % count;
}

int flavourLineCount() {
    return static_cast<int>(FLAVOUR_LINES.size());
}

std::string_view flavourLine(int index) {
    if (index < 0 || index >= flavourLineCount()) {
        throw std::out_of_range("flavourLine: there is no flavour line with this number");
    }
    return FLAVOUR_LINES[static_cast<std::size_t>(index)].text;
}

bool flavourLineNeedsShade(int index) {
    if (index < 0 || index >= flavourLineCount()) {
        throw std::out_of_range("flavourLineNeedsShade: there is no flavour line with this number");
    }
    return FLAVOUR_LINES[static_cast<std::size_t>(index)].needsShade;
}

int oldStoryLineCount() {
    int count = 0;
    for (const StoryLine& line : FLAVOUR_LINES) {
        if (!line.needsShade) {
            ++count;
        }
    }
    return count;
}

int storyLineFromOldTable(int oldLine) {
    // The old table was the lines without the shadow, in the same order. So old line
    // number n is the line a maze without a shade shows as its note number n when it
    // starts at the top of the table.
    return storyLineFor(0, wrapped(oldLine, oldStoryLineCount()), false);
}

std::optional<Compass> noteLean(const Note& note, MazeCell exit,
                                std::span<const MazeCell> crystalCells) {
    if (note.kind == NoteKind::ExitHint) {
        return compassTowards(note.mount.cell, exit);
    }
    if (note.kind == NoteKind::CrystalHint && !crystalCells.empty()) {
        return compassTowards(note.mount.cell, nearestCell(note.mount.cell, crystalCells));
    }
    return std::nullopt;
}

std::string noteText(const Note& note, MazeCell exit, std::span<const MazeCell> crystalCells) {
    const std::optional<Compass> lean = noteLean(note, exit, crystalCells);
    // Only a hint leans: the one to the gate always, the one to a splinter while one is left.
    if (lean) {
        return note.kind == NoteKind::ExitHint ? hintSentence("The gate", "waits", *lean)
                                               : hintSentence("A splinter", "glows", *lean);
    }
    if (note.kind == NoteKind::CrystalHint) {
        return "You took every one. The moon will look harder.";
    }
    // Flavour is what is left.
    return std::string{flavourLine(note.flavourIndex)};
}

} // namespace game
