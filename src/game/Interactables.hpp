// Interactables: the levers and the notes of a maze, where they hang and what they do.
// See docs/modules/game/interactables.md
#pragma once

#include "game/Crystals.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "game/Terrain.hpp"
#include "scene/Collider.hpp"
#include "scene/Raycast.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library.
//
// A lever hangs on a wall. Pulling it lowers one OTHER wall somewhere in the maze. The
// maze is a perfect maze (one way between any two cells), so a wall that is gone joins
// two ways into a loop: a shortcut.
// A note hangs on a wall too. Reading it shows one line of text: a hint computed from
// the maze, or a line from a small table.

/// How many levers and notes a maze gets when nothing else is asked for. The default maze
/// has 10 by 10 cells: two shortcuts change the ways through it without turning it into
/// an open field, and three notes are one of each kind (see NoteKind).
constexpr int DEFAULT_LEVER_COUNT = 2;
constexpr int DEFAULT_NOTE_COUNT = 3;

/// The largest numbers that can be asked for. They only keep a wrong setting from
/// filling the maze: nothing else depends on them.
constexpr int MAX_LEVER_COUNT = 16;
constexpr int MAX_NOTE_COUNT = 16;

/// A wall is worth a lever only when opening it brings the cell behind it at least this
/// many passages closer to the start (see chooseShortcutWalls). The number is always
/// even, so 6 means: a walk of at least 7 passages becomes a walk of 1.
constexpr int LEVER_MIN_STEPS_SAVED = 6;

// All sizes are in metres.

/// How high above the ground the middle of a lever and of a note hangs. The lever is at
/// the height of a hand, the note a little below the eyes of the player (1.7 m).
constexpr float LEVER_MOUNT_HEIGHT = 1.2F;
constexpr float NOTE_MOUNT_HEIGHT = 1.5F;

/// The box a lever is picked with: its width along the wall, its height, and its depth
/// (how far it stands out from the wall into the cell).
constexpr float LEVER_BOX_WIDTH = 0.3F;
constexpr float LEVER_BOX_HEIGHT = 0.4F;
constexpr float LEVER_BOX_DEPTH = 0.25F;

/// The box a note is picked with. A sheet of paper is flat, but its box is 15 cm deep on
/// purpose: see MOUNT_BOX_MIN_DEPTH.
constexpr float NOTE_BOX_WIDTH = 0.4F;
constexpr float NOTE_BOX_HEIGHT = 0.5F;
constexpr float NOTE_BOX_DEPTH = 0.15F;

/// The collision box of a wall is thicker than the wall that is drawn (0.3 m against
/// 0.2 m), so it stands out this far in front of the visible face. A pick box starts on
/// the visible face and has to be deeper than this, or the picking ray would always
/// reach the box of the wall first and the wall would hide what hangs on it.
constexpr float MOUNT_BOX_MIN_DEPTH = (WALL_COLLISION_THICKNESS - WALL_VISUAL_THICKNESS) / 2.0F;
static_assert(LEVER_BOX_DEPTH > MOUNT_BOX_MIN_DEPTH);
static_assert(NOTE_BOX_DEPTH > MOUNT_BOX_MIN_DEPTH);

/// How far the player can reach with the picking ray, in metres: a little more than one
/// cell (2 m), so a lever on the far wall of the cell the player stands in is always
/// within reach, and one two cells away is not.
constexpr float INTERACTION_REACH = 2.5F;

/// How many levers and notes the next maze should get. The debug UI can edit the two
/// numbers. They are read when a maze is built (placeInteractables).
struct InteractableSettings {
    /// Wanted number of levers, from 0 to MAX_LEVER_COUNT. A maze gets fewer when it has
    /// fewer walls that are worth opening: a small maze often has none.
    int leverCount = DEFAULT_LEVER_COUNT;

    /// Wanted number of notes, from 0 to MAX_NOTE_COUNT. A maze gets fewer when it has
    /// fewer free cells.
    int noteCount = DEFAULT_NOTE_COUNT;
};

/// One side of one cell: the name of a wall (or of the place where a wall could stand).
/// A wall between two cells has two names, one from each cell: the east side of a cell
/// is the west side of its eastern neighbour.
struct WallRef {
    MazeCell cell;
    Direction side = Direction::North;

    /// Equal when the cell and the side match. Two names of the same wall seen from its
    /// two cells are NOT equal here: sameWall compares those.
    bool operator==(const WallRef& other) const = default;
};

/// True when a and b name the same wall: they are equal, or one is the other seen from
/// the cell on the other side.
bool sameWall(const WallRef& a, const WallRef& b);

/// True when the maze has this wall and it stands between two cells of the maze (it is
/// not part of the outer border). Only such a wall can become a shortcut.
/// Throws std::out_of_range when the cell is not in the maze.
bool isInteriorWall(const Maze& maze, const WallRef& wall);

/// Chooses the walls the levers open: at most count of them, the best one first.
///
/// How good a wall is: the number of passages its two cells are from the start is
/// counted (game::passageDistances, a breadth first search). The wall stands between
/// a "near" and a "far" cell. With the wall gone the far cell is one passage behind the
/// near one, so the way to it gets shorter by (far - near - 1) passages. That number is
/// the score.
///
/// The rules:
///   - only interior walls that exist (isInteriorWall),
///   - never a wall of the exit cell: the gate is the only way in there, and a second
///     opening would let the player walk around it,
///   - the wall with the highest score wins. Of several with the same score the first
///     one in row order wins (row 0 from west to east, then row 1, and in a cell East
///     before South),
///   - a score below LEVER_MIN_STEPS_SAVED is not worth a lever,
///   - after a wall is chosen it is taken out of a COPY of the maze and the distances
///     are counted again. The next wall is scored in the maze with the earlier shortcuts
///     open, so two levers never open nearly the same shortcut.
///
/// Every wall is named from its western or northern cell (its side is East or South).
/// Nothing is random here: the answer follows from the maze alone.
///
/// The cost is one search over the maze per lever, also for the largest maze.
/// Throws std::out_of_range when start or exit is not a cell of the maze.
std::vector<WallRef> chooseShortcutWalls(const Maze& maze, MazeCell start, MazeCell exit,
                                         int count);

/// A lever of a maze.
struct Lever {
    /// The cell the lever is in and the side of that cell whose wall it hangs on.
    WallRef mount;

    /// The middle of the back of the lever: the point on the visible face of the wall it
    /// is fixed to. placeInteractables gives y for flat ground at 0.
    glm::vec3 position{0.0F};

    /// The box the picking ray has to hit (leverBox). The lever is not an obstacle: the
    /// player is never stopped by this box.
    scene::Aabb box;

    /// The wall that goes down when the lever is pulled. It is never the wall the lever
    /// hangs on.
    WallRef opens;
};

/// What a note says.
enum class NoteKind {
    ExitHint = 0, ///< the compass direction from the note towards the exit cell
    CrystalHint,  ///< the compass direction towards the nearest crystal that is left
    Flavour,      ///< one line from the table of flavour lines
};

/// The number of note kinds.
constexpr int NOTE_KIND_COUNT = 3;

/// A note of a maze.
struct Note {
    /// The cell the note is in and the side of that cell whose wall it hangs on.
    WallRef mount;

    /// The middle of the back of the note, on the visible face of the wall, and the box
    /// the picking ray has to hit (noteBox). See Lever.
    glm::vec3 position{0.0F};
    scene::Aabb box;

    NoteKind kind = NoteKind::Flavour;

    /// Which flavour line a note of the kind Flavour shows: 0 to flavourLineCount() - 1.
    /// The other kinds do not use it.
    int flavourIndex = 0;
};

/// Everything of a maze that can be picked. It belongs to the maze, like the crystals in
/// MazeWorld: it does not change while a round is played.
struct Interactables {
    std::vector<Lever> levers;
    std::vector<Note> notes;
};

/// Places the levers and the notes of a maze. The same maze, seed, start, exit, crystals
/// and settings always give the same result, on every compiler: every random choice uses
/// std::mt19937 and game::randomBelow only (see docs/decisions/deterministic-random.md).
///
/// The levers:
///   - first the walls to open are chosen (chooseShortcutWalls). There is one lever per
///     wall, so there can be fewer levers than settings.leverCount, and none at all,
///   - every lever gets a cell of its own: never the start cell and never the exit cell.
///     Cells without a crystal come first, in an order shuffled by the seed. Cells with
///     a crystal are only used when the others run out,
///   - in its cell the lever hangs on one of the walls of the cell, chosen by the seed,
///     but never on a wall that a lever opens (it would go down with the wall).
///
/// The notes:
///   - every note gets a cell of its own, shuffled by the seed: never the start cell,
///     never the exit cell and never a cell with a lever,
///   - it hangs on a wall of its cell like a lever does,
///   - note number i has the kind number i % NOTE_KIND_COUNT, so the first three notes
///     are one of each kind. The seed chooses the flavour line of the first flavour
///     note, the next ones take the lines that follow it in the table, so none repeats
///     before all were used.
///
/// The levers and the notes use two generators of their own, so asking for another
/// number of notes never moves a lever.
///
/// All heights are for flat ground at 0 (placeInteractablesOnTerrain puts them on
/// a terrain). Throws std::out_of_range when start or exit is not a cell of the maze.
Interactables placeInteractables(const Maze& maze, std::uint32_t seed, MazeCell start,
                                 MazeCell exit, std::span<const CrystalSpawn> crystals,
                                 const InteractableSettings& settings);

/// Where a lever hangs: on the visible face of the wall on mount.side of mount.cell, in
/// the middle of the wall, LEVER_MOUNT_HEIGHT above the ground. groundHeight is the
/// height of the ground under that point. It is passed in, like in crystalRestPosition,
/// so this function needs no terrain.
glm::vec3 leverPosition(const WallRef& mount, float groundHeight);

/// The pick box of a lever that hangs at position (a result of leverPosition) on the
/// given side of its cell: LEVER_BOX_WIDTH along the wall, LEVER_BOX_HEIGHT high, and
/// LEVER_BOX_DEPTH deep from the wall into the cell.
scene::Aabb leverBox(const glm::vec3& position, Direction side);

/// The same two for a note: NOTE_MOUNT_HEIGHT above the ground, and a box of
/// NOTE_BOX_WIDTH, NOTE_BOX_HEIGHT and NOTE_BOX_DEPTH.
glm::vec3 notePosition(const WallRef& mount, float groundHeight);
scene::Aabb noteBox(const glm::vec3& position, Direction side);

/// Moves every lever and every note to the height of the terrain: the ground is looked
/// up under the point of the wall each one hangs on (Terrain::heightAt), and the position
/// and the box follow. Nothing moves sideways. It can be called again after the terrain
/// was rebuilt with another height scale.
void placeInteractablesOnTerrain(Interactables& interactables, const Terrain& terrain);

/// The part of a round that belongs to the levers: which of them are pulled. It is
/// a struct of its own so that it can become a field of the round later.
struct InteractableState {
    /// One entry per lever, in the order of Interactables::levers: true once pulled.
    /// A pulled lever never goes back within a round.
    std::vector<bool> leverPulled;
};

/// The state at the start of a round: no lever is pulled.
InteractableState startInteractables(const Interactables& interactables);

/// True when lever number index is pulled.
/// Throws std::out_of_range when the state has no lever with that number.
bool isLeverPulled(const InteractableState& state, std::size_t index);

/// The answer of pullLever.
struct PullResult {
    /// True when this pull opened a wall. False when the lever was pulled before.
    bool opened = false;

    /// The wall that opened. It means nothing when opened is false.
    WallRef wall;
};

/// Pulls lever number index. The first pull marks the lever as pulled and reports the
/// wall that opens. Every later pull of the same lever changes nothing and reports
/// nothing.
///
/// Throws std::out_of_range when there is no lever with that number, or when the state
/// was not started for these interactables (startInteractables).
PullResult pullLever(InteractableState& state, const Interactables& interactables,
                     std::size_t index);

/// The walls that are open now: the wall of every pulled lever, in the order of the
/// levers. The code that builds the obstacle list of a round leaves these walls out.
std::vector<WallRef> openedWalls(const Interactables& interactables,
                                 const InteractableState& state);

/// The wall segment of the layout that a lever opens, at y = 0: the same segment
/// game::wallSegments lists for that wall, so it can be found there by its x and z.
WallSegment openedWallSegment(const Lever& lever);

/// What the picking ray points at.
enum class InteractableKind {
    None = 0, ///< nothing within reach, or a wall is in the way
    Lever,
    Note,
};

/// The answer of pickInteractable.
struct PickedInteractable {
    InteractableKind kind = InteractableKind::None;

    /// The number of the lever in Interactables::levers, or of the note in
    /// Interactables::notes. It means nothing when kind is None.
    std::size_t index = 0;

    /// How far along the ray it is, in metres. It means nothing when kind is None.
    float distance = 0.0F;
};

/// The lever or the note the ray points at: the nearest pick box it hits within reach
/// metres (scene::nearestHit). blockers are the boxes that hide things: the walls and
/// pillars of the maze, and the gate while it is closed. When the ray runs into one of
/// them BEFORE it reaches the nearest lever or note, the answer is None: the player
/// cannot pull a lever through a wall. Of a lever and a note at the same distance the
/// lever wins.
PickedInteractable pickInteractable(const scene::Ray& ray, const Interactables& interactables,
                                    std::span<const scene::Aabb> blockers, float reach);

/// A direction on the map, for hints. Here is the answer for "towards the cell I am in".
enum class Compass {
    Here = 0,
    North,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
};

/// The compass direction of the straight line from one cell to another, whatever walls
/// are in between. North is -Z (a smaller row number) and East is +X (a larger column
/// number), like everywhere in the game.
///
/// The cells are columnOffset columns and rowOffset rows apart. When one of the two
/// offsets is MORE than twice the other, the line is close to an axis and the answer is
/// one of North, East, South and West. Otherwise it is one of the four diagonals. So
/// 3 columns east and 1 row north is East, and 2 columns east and 1 row north is
/// NorthEast. Whole numbers only, no angles.
Compass compassTowards(MazeCell from, MazeCell to);

/// The direction as words for a sentence: "north", "north-east", and "here" for Here.
std::string_view compassName(Compass direction);

/// The number of flavour lines in the table, and one of them. index runs from 0 to
/// flavourLineCount() - 1. Throws std::out_of_range for another index.
int flavourLineCount();
std::string_view flavourLine(int index);

/// The text of a note, in English like the rest of the HUD.
///
/// The text of a hint is computed when it is asked for, not stored in the note, because
/// the answer changes while the round is played: crystalCells are the cells of the
/// crystals that are NOT collected yet, and a hint towards a crystal that is gone would
/// be a lie. "Nearest" is the straight line on the grid, like the compass direction. Of
/// two crystals equally near the earlier one in the list wins.
///
///   - ExitHint: "The exit lies to the north-east."
///   - CrystalHint: "A crystal glows to the south.", or "No crystal is left to find."
///     when crystalCells is empty,
///   - Flavour: flavourLine(note.flavourIndex).
std::string noteText(const Note& note, MazeCell exit, std::span<const MazeCell> crystalCells);

} // namespace game
