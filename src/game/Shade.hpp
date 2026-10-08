// Shade: the enemy of a round. A dark figure that wanders the maze, comes when it hears or
// sees the player, stands while the flashlight is on it and is burned away by a long look
// of the lamp. It carries the player back to the start when it reaches them.
#pragma once

#include "game/Maze.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace game {

class Terrain;

// Plain data and pure functions without OpenGL, like the rest of the game_logic library,
// so tests can let the shade walk without a window.
//
// The rules in short:
//
//   - The shade does NOT know where the player is. It WANDERS: it walks to cells its own
//     random generator chooses, far apart, at a slow pace (shadeWanderCell).
//   - It HEARS noise (Noise, noiseReach): a sprint from far, a walk only from near, a
//     lever and a pickup from in between. The distance is the way along the passages,
//     not the straight line, so a wall between the two helps. It then walks to the cell
//     the noise came from (INVESTIGATING), not to where the player went afterwards.
//   - It SEES the player down a straight corridor with no wall between (shadeSees). Then
//     it walks straight after the player at its full speed (CHASING).
//   - At the last place it heard or saw the player it waits a few seconds. With nothing
//     new it goes back to wandering.
//   - It moves only while it is NOT LIT, and "lit" means exactly that the flashlight is
//     on, the shade is inside its cone and its range, and no wall stands between the two
//     (shadeLit). Looking at the shade with the lamp off does not stop it, and neither
//     do the moon or the glow of the crystals.
//   - The beam BURNS it: a clock runs while it is lit (burnAfter). When the clock is full
//     the shade is BANISHED: it dissolves, stands in a cell far from the player
//     (shadeBanishCell) and is quiet for a while. While the beam is on it, the battery
//     drains faster (batteryDrainFactor).
//
// The code says "shade", because "shadow" already means shadow mapping here. Text the
// player reads says "the shadow".

/// How fast the shade walks when it chases the player it sees, in metres per second:
/// faster than the player walks (3.0) and slower than the player sprints (5.5). So
/// walking away from a shade that sees you never works, and the stamina and the flasks
/// decide whether running does.
constexpr float SHADE_SPEED = 4.0F;

/// How fast it wanders, in metres per second: half as fast as the player walks, so
/// a player who has not been noticed walks away from it, and it crosses the way of the
/// player less often than a faster one would.
constexpr float SHADE_WANDER_SPEED = 1.5F;

/// How fast it walks to a noise, in metres per second: as fast as the player walks. It
/// comes with purpose, but a player who walks on does not lose ground to it.
constexpr float SHADE_INVESTIGATE_SPEED = 3.0F;

/// How far each noise of the player carries, in metres along the passages (Noise):
/// a sprint seven cells, a lever five, a pickup four, and a walk a cell and a half, so
/// walking is heard only by a shade that is almost there.
constexpr float SHADE_HEAR_SPRINT_METRES = 14.0F;
constexpr float SHADE_HEAR_LEVER_METRES = 10.0F;
constexpr float SHADE_HEAR_PICKUP_METRES = 8.0F;
constexpr float SHADE_HEAR_WALK_METRES = 3.0F;

/// How far the shade sees down a straight corridor, in metres: a little farther than the
/// flashlight reaches (10 m), so it has seen the player before the beam can stop it.
constexpr float SHADE_SIGHT_METRES = 12.0F;

/// How long the shade waits at the last place it heard or saw the player before it gives
/// up and wanders again, in seconds.
constexpr float SHADE_SEARCH_SECONDS = 6.0F;

/// How long the beam has to be on the shade, in all, to banish it, in seconds. It does
/// not have to be one unbroken look: the clock only falls back slowly while the light is
/// off (SHADE_BURN_RECOVER_RATE).
constexpr float SHADE_BURN_SECONDS = 2.5F;

/// How fast the burn clock falls back while the shade is not lit, in seconds of the
/// clock per second: 0.5 means that a second of light is forgotten after two in the dark.
constexpr float SHADE_BURN_RECOVER_RATE = 0.5F;

/// How long a banished shade is quiet, in seconds: it stands in its new cell, makes no
/// sound, hears nothing, sees nothing and catches nobody.
constexpr float SHADE_QUIET_SECONDS = 20.0F;

/// How many times faster the battery drains while the beam is on the shade.
constexpr float SHADE_BURN_BATTERY_FACTOR = 3.0F;

/// How long the drawn figure takes to dissolve where it was banished, in seconds.
constexpr float SHADE_DISSOLVE_SECONDS = 0.8F;

/// How long the shade stands still after a round starts or starts again, in seconds. It
/// neither moves nor catches in that time, so a player who was just carried back is
/// never caught again at once.
constexpr float SHADE_GRACE_SECONDS = 8.0F;

/// The shade has reached the player when the two are this close, in metres, measured on
/// the ground from the middle of one to the middle of the other. The player is 0.6
/// m wide and the shade about 0.8 m, so at 0.9 m they almost touch.
constexpr float SHADE_CATCH_DISTANCE = 0.9F;

/// How long the shade goes on standing still after the light has left it, in seconds.
/// Without this wait nobody could get past a shade that stands lit in a corridor: the
/// moment the player has passed it, it is behind the lamp, unlit, and within reach. With
/// it, shining at the shade buys a few steps: about 6 m at a walk, 11 m at a sprint.
constexpr float SHADE_THAW_SECONDS = 2.0F;

/// The height of the figure, in metres (the model shade.obj).
constexpr float SHADE_HEIGHT = 2.1F;

/// The heights above its feet at which the shade is tested for light, in metres: the
/// knees, the chest and the hood. The shade is lit when the flashlight reaches ANY of
/// them, so a beam that only catches its head over a rise of the ground still stops it.
constexpr std::array<float, 3> SHADE_LIT_HEIGHTS = {0.5F, 1.2F, 1.9F};

/// The length of a way that does not exist, in metres: farther than any maze is long.
constexpr float SHADE_NO_WAY = 1.0e9F;

/// Where a shade may start: a cell counts as far from the start of the maze when it is
/// at least this many tenths of the way to the farthest cell, counted in passages. A
/// banished shade reappears by the same rule, measured from the cell of the player.
constexpr int SHADE_START_FAR_TENTHS = 6;

/// Where a wandering shade walks next: to a cell at least this many tenths of the way to
/// the cell farthest from where it stands. Long walks, so it covers the whole maze and
/// does not pace up and down one corner.
constexpr int SHADE_WANDER_FAR_TENTHS = 5;

/// A noise the player makes in one fixed step.
enum class Noise {
    None = 0, ///< standing still, reading the map, flying: silent
    Walk,     ///< walking
    Pickup,   ///< a crystal or a flask was picked up
    Lever,    ///< a lever was pulled
    Sprint,   ///< sprinting
};

/// A noise in words, for the debug window: "sprint", "nothing".
const char* noiseName(Noise noise);

/// What the shade is after.
enum class ShadeHunt {
    Wandering = 0, ///< nothing: it walks to a cell of its own choice
    Investigating, ///< a noise: it walks to the cell the noise came from
    Chasing,       ///< the player it saw: it walks to them, or to where it saw them last
};

/// What the shade is doing, as one word: the first of these that holds
/// (shadeState).
enum class ShadeState {
    Banished = 0,  ///< burned away a moment ago: it stands far off and is quiet
    Grace,         ///< the round has just started: it stands still
    Lit,           ///< the flashlight is on it: it stands still
    Thawing,       ///< the light has just left it: it still stands
    Chasing,       ///< see ShadeHunt
    Investigating, ///< see ShadeHunt
    Wandering,     ///< see ShadeHunt
};

/// A state in words, for the debug window: "wandering", "banished and quiet".
const char* shadeStateName(ShadeState state);

/// How the drawn figure moves when it is not just a statue (drawing only: the position, the
/// lit test points and the catch distance never see it). The values here are the defaults.
struct ShadeSwaySettings {
    /// Standing: the whole figure leans to one side and the other, in degrees, and rises
    /// and sinks like a slow breath, in metres. One full swing takes periodSeconds.
    float standLeanDegrees = 1.5F;
    float standRiseMetres = 0.015F;
    float periodSeconds = 4.5F;

    /// Walking: the figure leans forward, in degrees, and bobs up and down once per
    /// step, in metres. The steps follow the speed (see SHADE_STRIDE_METRES).
    float walkLeanDegrees = 4.0F;
    float walkBobMetres = 0.04F;
};

/// How far the shade travels in one step of its walk, in metres: the bob has one beat per
/// step, so a faster shade bobs faster.
constexpr float SHADE_STRIDE_METRES = 1.6F;

/// The numbers of the shade that can be changed while the game runs. The debug UI edits
/// them. The values here are the defaults.
struct ShadeSettings {
    /// False: the round has no shade. A new game writes "not a calm night" here. Switched
    /// off in the middle of a round, the shade is gone at once. Switched on, it comes
    /// with the next round.
    bool enabled = true;

    /// See SHADE_SPEED (the speed of a chase), SHADE_GRACE_SECONDS and
    /// SHADE_CATCH_DISTANCE.
    float speed = SHADE_SPEED;
    float graceSeconds = SHADE_GRACE_SECONDS;
    float catchDistance = SHADE_CATCH_DISTANCE;

    /// See SHADE_THAW_SECONDS.
    float thawSeconds = SHADE_THAW_SECONDS;

    /// See SHADE_WANDER_SPEED and SHADE_INVESTIGATE_SPEED.
    float wanderSpeed = SHADE_WANDER_SPEED;
    float investigateSpeed = SHADE_INVESTIGATE_SPEED;

    /// How far each noise carries (noiseReach), and how far the shade sees.
    float hearSprintMetres = SHADE_HEAR_SPRINT_METRES;
    float hearLeverMetres = SHADE_HEAR_LEVER_METRES;
    float hearPickupMetres = SHADE_HEAR_PICKUP_METRES;
    float hearWalkMetres = SHADE_HEAR_WALK_METRES;
    float sightMetres = SHADE_SIGHT_METRES;

    /// See SHADE_SEARCH_SECONDS.
    float searchSeconds = SHADE_SEARCH_SECONDS;

    /// See SHADE_BURN_SECONDS, SHADE_BURN_RECOVER_RATE, SHADE_QUIET_SECONDS and
    /// SHADE_BURN_BATTERY_FACTOR.
    float burnSeconds = SHADE_BURN_SECONDS;
    float burnRecoverRate = SHADE_BURN_RECOVER_RATE;
    float quietSeconds = SHADE_QUIET_SECONDS;
    float burnBatteryFactor = SHADE_BURN_BATTERY_FACTOR;

    /// Debug switch: draw the shade on the map. The map of the game never shows it.
    bool showOnMap = false;

    /// How the figure sways while it stands and walks (drawing only).
    ShadeSwaySettings sway;
};

/// The shade of one round. It is a field of the round (Round::shade), so starting the
/// round again puts it back where it started.
struct Shade {
    /// False when the round has none: a calm night, or a maze too small to have a cell
    /// far from the start. Nothing else of the struct means anything then.
    bool present = false;

    /// Where its feet are, in the world, on the ground.
    glm::vec3 position{0.0F};

    /// Where its feet were before the last step. A frame is drawn from a point between
    /// the two, like the player (the fixed steps are not the frames).
    glm::vec3 previousPosition{0.0F};

    /// The cell whose centre it is walking to: the cell it is in, or a neighbour of it
    /// through an open side.
    MazeCell target;

    /// Seconds it still stands still (ShadeSettings::graceSeconds at the start).
    float graceLeft = 0.0F;

    /// Whether the flashlight was on it in the last step (shadeLit). Never in the grace
    /// time and never while it is quiet after a banish.
    bool lit = false;

    /// Seconds it still stands still because it was lit a moment ago
    /// (ShadeSettings::thawSeconds from the last step in which it was lit).
    float thawLeft = 0.0F;

    /// What it is after, and the cell it walks to for that: a cell of its own choice
    /// (wandering), the cell a noise came from (investigating) or the cell it saw the
    /// player in last (chasing). A goal that is no cell (-1, -1) means none yet: the
    /// next step chooses one.
    ShadeHunt hunt = ShadeHunt::Wandering;
    MazeCell goal{.x = -1, .z = -1};

    /// Seconds it still waits at the goal of a noise or a chase before it gives up
    /// (ShadeSettings::searchSeconds from the last moment it heard or saw the player).
    /// It only runs down while the shade stands at that goal.
    float searchLeft = 0.0F;

    /// The last noise it heard and the cell it came from, for the debug window.
    Noise lastHeard = Noise::None;
    MazeCell lastHeardCell{.x = -1, .z = -1};

    /// The burn clock: seconds of light it has taken (burnAfter). At
    /// ShadeSettings::burnSeconds it is banished.
    float burnSeconds = 0.0F;

    /// Seconds it is still quiet after a banish (ShadeSettings::quietSeconds).
    float quietLeft = 0.0F;

    /// Where the figure dissolves after a banish, and for how long it still does
    /// (SHADE_DISSOLVE_SECONDS). Drawing only: the shade itself is in its new cell from
    /// the step of the banish on.
    glm::vec3 banishedFrom{0.0F};
    float dissolveLeft = 0.0F;

    /// The direction it walked in last, as the yaw of its model in degrees
    /// (shadeYawDegrees). Drawing only: a shade that wanders looks where it goes.
    float headingDegrees = 0.0F;

    /// What its own choices are drawn from: the seed of the maze, the exit cell (never
    /// a goal: it lies behind the gate), and how many cells it has chosen to wander to
    /// and how often it was banished. The same round always makes the same choices
    /// (shadeWanderCell, shadeBanishCell).
    std::uint32_t seed = 0;
    MazeCell exit{.x = -1, .z = -1};
    int wanderCount = 0;
    int banishCount = 0;

    /// How far it would have to WALK to the player, in metres, along the passages
    /// (advanceShade keeps it up to date, also while the shade stands still).
    /// SHADE_NO_WAY before the first step, when there is no way and while it is quiet
    /// after a banish. The hum of the shade and its hearing are told this number and not
    /// the straight distance: a shade behind the next wall can be a long walk away.
    float wayMetres = SHADE_NO_WAY;

    /// The ways, kept from step to step: for every cell the number of passages to the
    /// cell of the player (playerDistances, for hearing and the hum) and to the goal
    /// (goalDistances, for walking), both from game::passageDistances. Each is searched
    /// again only when its cell is another one or another wall has opened (the two
    /// openings count the pulled levers), not in every step.
    std::vector<int> playerDistances;
    MazeCell playerCell{.x = -1, .z = -1};
    int playerOpenings = -1;
    std::vector<int> goalDistances;
    MazeCell pathGoal{.x = -1, .z = -1};
    int pathOpenings = -1;
};

/// The flashlight as the shade needs it.
struct ShadeLamp {
    /// True when it gives light: the switch is on and the battery is not empty.
    bool on = false;
    /// Where the light is and the axis of its cone, with length 1 (game::flashlightPose).
    glm::vec3 position{0.0F};
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
    /// The half angle of the cone in degrees and how far the light reaches in metres
    /// (LightingSettings::flashlightOuterDegrees and flashlightRange).
    float outerDegrees = 0.0F;
    float range = 0.0F;
};

/// The cell a shade starts in. The rule:
///   - never the start cell and never the exit cell (the exit lies behind the gate),
///   - only cells that can be reached and are far from the start: at least
///     SHADE_START_FAR_TENTHS tenths as many passages away as the farthest cell,
///   - of those cells, listed row after row, the seed of the maze chooses one. The shade
///     has a random generator of its own, so it moves nothing else of a seed.
/// The same maze and seed always give the same cell. False when the maze has no such
/// cell (a maze of one or two cells): cell is left as it was.
/// Throws std::out_of_range when start is not a cell of the maze.
bool shadeStartCell(const Maze& maze, std::uint32_t seed, MazeCell start, MazeCell exit,
                    MazeCell& cell);

/// The cell a wandering shade walks to next, as its choice number pick (0 for the first).
/// The rule of shadeStartCell, measured from the cell the shade is in: a cell that can be
/// reached, at least SHADE_WANDER_FAR_TENTHS tenths as far away as the farthest one, and
/// never the exit cell. The seed and the number choose among them, so the same round
/// always wanders the same way. False when the maze has no such cell: cell is left as it
/// was. Throws std::out_of_range when from is not a cell of the maze.
bool shadeWanderCell(const Maze& maze, std::uint32_t seed, int pick, MazeCell from, MazeCell exit,
                     MazeCell& cell);

/// The cell a banished shade reappears in, as its banish number count (0 for the first).
/// The rule of shadeStartCell, measured from the cell of the player this time: far from
/// the player (SHADE_START_FAR_TENTHS), and never the exit cell. False when the maze has
/// no such cell. Throws std::out_of_range when playerCell is not a cell of the maze.
bool shadeBanishCell(const Maze& maze, std::uint32_t seed, int count, MazeCell playerCell,
                     MazeCell exit, MazeCell& cell);

/// The shade at the start of a round: standing in the middle of its start cell
/// (shadeStartCell), on the ground of terrain, with the whole grace time ahead of it and
/// nothing to hunt. Not present when settings.enabled is false or the maze has no cell
/// for it.
Shade startShade(const Maze& maze, std::uint32_t seed, MazeCell start, MazeCell exit,
                 const Terrain& terrain, const ShadeSettings& settings);

/// How far a noise carries, in metres along the passages: the numbers of the settings
/// (SHADE_HEAR_SPRINT_METRES and the three after it). 0 for no noise.
float noiseReach(Noise noise, const ShadeSettings& settings);

/// What the noise of a player is decided from, for one fixed step.
struct NoiseSources {
    /// How far the feet moved over the ground in this step, in metres, and the length
    /// of the step in seconds: together the speed the feet really had.
    float metres = 0.0F;
    float stepSeconds = 0.0F;
    /// Player::walkSpeed and Player::sprintSpeed (game::sprintedStep).
    float walkSpeed = 0.0F;
    float sprintSpeed = 0.0F;
    /// True in noclip mode: a player who flies makes no sound of feet.
    bool flying = false;
    /// A crystal or a flask was picked up in this step.
    bool pickedUp = false;
    /// A lever was pulled since the step before.
    bool pulledLever = false;
};

/// The noise the player makes in a step: of everything that happened, the one that
/// carries farthest (noiseReach). Feet that moved are a sprint or a walk, by the speed
/// they really had, so sliding slowly along a wall with the sprint key held is a walk.
/// A player who stands still makes no noise, and neither does one who reads the map,
/// because reading the map is standing still (game::movementInput).
Noise playerNoise(const NoiseSources& sources, const ShadeSettings& settings);

/// True when the shade hears a noise made wayMetres away from it along the passages
/// (Shade::wayMetres): the noise carries at least that far.
bool shadeHears(float wayMetres, Noise noise, const ShadeSettings& settings);

/// True when somebody at shadeFeet sees somebody at playerFeet: the two are in the same
/// cell, or in the same row or column of the maze with no wall on the straight line of
/// cells between them, and not farther apart than rangeMetres on the ground. It is the
/// line of sight of the minimap (game::discoverFrom) with a range: down a corridor, never
/// around a corner. The shade has no front and no back: it sees in all four directions.
/// False when one of the two is outside the maze.
bool shadeSees(const Maze& maze, const glm::vec3& shadeFeet, const glm::vec3& playerFeet,
               float rangeMetres);

/// The burn clock after one more step of stepSeconds seconds: it runs up while the shade
/// is lit, and falls back by ShadeSettings::burnRecoverRate of that while it is not,
/// never below 0. The shade is banished when it reaches ShadeSettings::burnSeconds.
float burnAfter(float burnSeconds, bool lit, const ShadeSettings& settings, float stepSeconds);

/// How many times faster than usual the battery drains: ShadeSettings::burnBatteryFactor
/// while the beam is on the shade (Shade::lit), otherwise 1. The beam costs to burn.
float batteryDrainFactor(const Shade& shade, const ShadeSettings& settings);

/// What the shade is doing (ShadeState): the first state of that list that holds.
ShadeState shadeState(const Shade& shade);

/// How fast the shade walks for what it is after, in metres per second: the three
/// speeds of the settings. Never negative.
float shadeSpeed(ShadeHunt hunt, const ShadeSettings& settings);

/// True when the flashlight is on the shade. All of these have to hold:
///   - the lamp gives light (ShadeLamp::on),
///   - one of the points of the shade (SHADE_LIT_HEIGHTS above its feet) is inside the
///     cone of the lamp and not farther away than its range,
///   - no obstacle stands on the straight line from the lamp to that point. obstacles
///     are the boxes of the round (game::roundObstacles): walls, pillars and the closed
///     gate. A wall that a lever has opened is not among them.
bool shadeLit(const Shade& shade, const ShadeLamp& lamp, std::span<const scene::Aabb> obstacles);

/// How far the shade is from the player, in metres, measured on the ground (x and z).
float shadeDistance(const Shade& shade, const glm::vec3& playerFeet);

/// The yaw that turns the model of the shade towards the player, in degrees, for
/// scene::Transform::rotationDegrees.y. The model looks along +Z, and a turn by this
/// angle around the vertical axis makes it look from position towards playerFeet. 0 when
/// the two stand on the same spot.
float shadeYawDegrees(const glm::vec3& position, const glm::vec3& playerFeet);

/// The yaw the figure is drawn with. A shade that knows where the player is turns its
/// front to them: while it chases, while it is lit and right after. Otherwise it looks
/// the way it walked last (Shade::headingDegrees), so a wandering shade does not give
/// away that it is only a figure that always faces the camera.
float shadeFacingDegrees(const Shade& shade, const glm::vec3& playerFeet);

/// How much of the height of the figure is still there while it dissolves, from 1 (all
/// of it, at the moment of the banish) to 0 (gone), on a smooth curve. dissolveLeft is
/// Shade::dissolveLeft. The figure sinks into the ground where it stood: the model is
/// scaled, so no shader has to know about it.
float shadeDissolveHeight(float dissolveLeft);

/// What the shade is told about one fixed step.
struct ShadeStep {
    /// The feet of the player after the movement of this step.
    glm::vec3 playerFeet{0.0F};
    /// The flashlight in this step.
    ShadeLamp lamp;
    /// What blocks the light (game::roundObstacles).
    std::span<const scene::Aabb> obstacles;
    /// How many walls levers have opened so far (game::pulledLeverCount): when it
    /// changes, the ways are searched again.
    int openedWalls = 0;
    /// The noise the player made in this step (playerNoise), at playerFeet.
    Noise noise = Noise::None;
};

/// What happened to the shade in one fixed step. The application plays a sound for each.
struct ShadeEvents {
    /// It has reached the player: the round starts again.
    bool caught = false;
    /// It has noticed the player: it was wandering, and now it heard a noise or saw them.
    bool alerted = false;
    /// The beam has burned it away: it is in its new cell and quiet.
    bool banished = false;
};

/// Advances the shade by one fixed step of stepSeconds seconds and tells what happened.
/// In this order:
///
///   - Switched off in the settings: the shade is gone.
///   - Quiet after a banish: it stands, and nothing below happens.
///   - While the grace time runs it stands still, cannot catch, and nothing below
///     happens either.
///   - The light. While it is lit (shadeLit) it stands still and cannot catch: a lit
///     shade is only a dark place on the ground, and the player may walk past it. It
///     goes on standing still, and still cannot catch, for ShadeSettings::thawSeconds
///     after the light has left it. The burn clock runs (burnAfter). When it is full
///     the shade is banished: it stands in shadeBanishCell, forgets what it was after,
///     and is quiet for ShadeSettings::quietSeconds.
///   - Its senses, while it is not lit. It sees the player (shadeSees): it chases, and
///     its goal is the cell of the player. Otherwise it hears a noise (shadeHears, with
///     the way along the passages): its goal is the cell the noise came from, and
///     a wandering shade now investigates. Either one starts the wait of
///     ShadeSettings::searchSeconds anew. A shade that was wandering is alerted.
///   - Its walk, while nothing holds it: shadeSpeed metres per second along the shortest
///     way to its goal through the passages of maze, from cell centre to cell centre,
///     so it never crosses a wall. maze is the maze of the ROUND (game::roundMaze):
///     a wall sunk by a lever is a way for the shade too. In the cell of a player it
///     sees it walks straight at the player. At the goal of a noise or a chase it waits,
///     and when the wait is over it wanders again. A wandering shade that has reached
///     its goal chooses the next one (shadeWanderCell).
///   - It has caught the player when, after a movement, it is not farther away than
///     ShadeSettings::catchDistance and no wall stands between the two.
///
/// A player outside the maze (flying in noclip mode) is neither heard, seen nor caught.
/// The feet of the shade are put on the ground of terrain.
ShadeEvents advanceShade(Shade& shade, const ShadeSettings& settings, const Maze& maze,
                         const Terrain& terrain, const ShadeStep& step, float stepSeconds);

/// What the drawn shade does on top of its place: a rise and two turns around its feet.
struct ShadePose {
    /// Metres up from the ground.
    float riseMetres = 0.0F;
    /// Degrees of leaning forward (towards the player) and to the side.
    float forwardLeanDegrees = 0.0F;
    float sideLeanDegrees = 0.0F;
};

/// How the shade is drawn seconds into the round. walkAmount is 0 for a figure that
/// stands (lit, waiting, in grace) and 1 for one that walks; the values between blend the
/// two, so the drawing can change from one to the other without a jump. speed is the
/// walking speed in metres per second (shadeSpeed). Pure: the same numbers
/// always give the same pose, and the logical shade is not involved.
ShadePose shadeSwayPose(float walkAmount, float speed, float seconds,
                        const ShadeSwaySettings& sway);

/// True when the shade walked in its last fixed step: it is present and its feet moved
/// over the ground. A shade that is lit, waits, searches at its goal or is quiet stands.
/// The drawing uses it to choose the stand or the walk pose.
bool shadeWalking(const Shade& shade);

/// How the picture goes to black when the shade catches the player, before the round
/// starts again: CATCH_FADE_OUT_SECONDS of falling brightness, then black. The input of
/// the player is frozen and the shade stands still in that time.
constexpr float CATCH_FADE_OUT_SECONDS = 0.6F;

/// The two phases of a catch before the round starts again.
enum class CatchPhase {
    FadingOut, ///< the picture is getting darker
    Black,     ///< fully dark: the round starts again now
};

/// The phase secondsSinceCatch seconds after the shade reached the player.
CatchPhase catchPhase(float secondsSinceCatch);

/// How bright the picture is in that time, as a factor from 1 (full) to 0 (black), on the
/// same smooth curve as caughtBrightness runs the other way.
float catchFadeBrightness(float secondsSinceCatch);

/// What the player reads after being caught: one of these lines, shown over the play
/// view for CAUGHT_LINE_SECONDS seconds. The picture comes back from black in the first
/// CAUGHT_FADE_SECONDS of them.
constexpr int CAUGHT_LINE_COUNT = 5;
constexpr float CAUGHT_LINE_SECONDS = 5.0F;
constexpr float CAUGHT_FADE_SECONDS = 1.2F;

/// The number that means "no caught line": before the first catch, and in a round the
/// player was not carried into.
constexpr int NO_CAUGHT_LINE = -1;

/// One of the caught lines. index runs from 0 to CAUGHT_LINE_COUNT - 1.
/// Throws std::out_of_range for another index.
std::string_view caughtLine(int index);

/// The line after previous, in the order of the table and round and round, so the same
/// line never shows twice in a row. NO_CAUGHT_LINE (nothing was shown yet) gives line 0.
int nextCaughtLine(int previous);

/// How bright the picture is secondsSinceCaught seconds after a catch, as a factor from
/// 0 (black) to 1: black at the moment of the catch, back to full along a smooth curve
/// after CAUGHT_FADE_SECONDS.
float caughtBrightness(float secondsSinceCaught);

} // namespace game
