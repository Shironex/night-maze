// Shade: the enemy of a round. A dark figure that walks towards the player whenever the
// flashlight is not on it, and carries the player back to the start when it gets there.
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
// The rule in one sentence: the shade moves only while it is NOT LIT, and "lit" means
// exactly that the flashlight is on, the shade is inside its cone and its range, and no
// wall stands between the two (shadeLit). Looking at the shade with the lamp off does
// not stop it, and neither do the moon or the glow of the crystals.
//
// The code says "shade", because "shadow" already means shadow mapping here. Text the
// player reads says "the shadow".

/// How fast the shade walks, in metres per second: faster than the player walks (3.0)
/// and slower than the player sprints (5.5). So walking away from it never works, and
/// the stamina and the flasks decide whether running does.
constexpr float SHADE_SPEED = 4.0F;

/// How long the shade stands still after a round starts or starts again, in seconds. It
/// neither moves nor catches in that time, so a player who was just carried back is
/// never caught again at once.
constexpr float SHADE_GRACE_SECONDS = 8.0F;

/// The shade has reached the player when the two are this close, in metres, measured on
/// the ground from the middle of one to the middle of the other. The player is 0.6
/// m wide and the shade about 0.8 m, so at 0.9 m they almost touch.
constexpr float SHADE_CATCH_DISTANCE = 0.9F;

/// The height of the figure, in metres (the model shade.obj).
constexpr float SHADE_HEIGHT = 2.1F;

/// The heights above its feet at which the shade is tested for light, in metres: the
/// knees, the chest and the hood. The shade is lit when the flashlight reaches ANY of
/// them, so a beam that only catches its head over a rise of the ground still stops it.
constexpr std::array<float, 3> SHADE_LIT_HEIGHTS = {0.5F, 1.2F, 1.9F};

/// The length of a way that does not exist, in metres: farther than any maze is long.
constexpr float SHADE_NO_WAY = 1.0e9F;

/// Where a shade may start: a cell counts as far from the start of the maze when it is
/// at least this many tenths of the way to the farthest cell, counted in passages.
constexpr int SHADE_START_FAR_TENTHS = 6;

/// The numbers of the shade that can be changed while the game runs. The debug UI edits
/// them. The values here are the defaults.
struct ShadeSettings {
    /// False: the round has no shade. A new game writes "not a calm night" here. Switched
    /// off in the middle of a round, the shade is gone at once. Switched on, it comes
    /// with the next round.
    bool enabled = true;

    /// See SHADE_SPEED, SHADE_GRACE_SECONDS and SHADE_CATCH_DISTANCE.
    float speed = SHADE_SPEED;
    float graceSeconds = SHADE_GRACE_SECONDS;
    float catchDistance = SHADE_CATCH_DISTANCE;

    /// Debug switch: draw the shade on the map. The map of the game never shows it.
    bool showOnMap = false;
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

    /// Whether the flashlight was on it in the last step (shadeLit).
    bool lit = false;

    /// How far it still has to WALK to the player, in metres, along the passages
    /// (advanceShade keeps it up to date, also while the shade stands still).
    /// SHADE_NO_WAY before the first step and when there is no way. The hum of the shade
    /// is told this number and not the straight distance: a shade behind the next wall
    /// can be a long walk away.
    float wayMetres = SHADE_NO_WAY;

    /// The way to the player, kept from step to step: for every cell the number of
    /// passages to pathGoal (game::passageDistances from the cell of the player). It is
    /// searched again only when the player is in another cell or another wall has
    /// opened (pathOpenings counts the pulled levers), not in every step.
    std::vector<int> pathDistances;
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

/// The shade at the start of a round: standing in the middle of its start cell
/// (shadeStartCell), on the ground of terrain, with the whole grace time ahead of it.
/// Not present when settings.enabled is false or the maze has no cell for it.
Shade startShade(const Maze& maze, std::uint32_t seed, MazeCell start, MazeCell exit,
                 const Terrain& terrain, const ShadeSettings& settings);

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

/// What the shade is told about one fixed step.
struct ShadeStep {
    /// The feet of the player after the movement of this step.
    glm::vec3 playerFeet{0.0F};
    /// The flashlight in this step.
    ShadeLamp lamp;
    /// What blocks the light (game::roundObstacles).
    std::span<const scene::Aabb> obstacles;
    /// How many walls levers have opened so far (game::pulledLeverCount): when it
    /// changes, the way to the player is searched again.
    int openedWalls = 0;
};

/// Advances the shade by one fixed step of stepSeconds seconds and returns true when it
/// has caught the player in this step.
///
///   - Switched off in the settings: the shade is gone.
///   - While the grace time runs it stands still and cannot catch.
///   - While it is lit (shadeLit) it stands still and cannot catch: a lit shade is only
///     a dark place on the ground, and the player may walk past it.
///   - Otherwise it walks ShadeSettings::speed metres per second along the shortest way
///     through the passages of maze, from cell centre to cell centre, so it never
///     crosses a wall. maze is the maze of the ROUND (game::roundMaze): a wall sunk by
///     a lever is a way for the shade too. In the cell of the player it walks straight
///     at the player. A player outside the maze (flying in noclip mode) is not followed.
///   - It has caught the player when, after its movement, it is not farther away than
///     ShadeSettings::catchDistance and no wall stands between the two.
///
/// The feet of the shade are put on the ground of terrain.
bool advanceShade(Shade& shade, const ShadeSettings& settings, const Maze& maze,
                  const Terrain& terrain, const ShadeStep& step, float stepSeconds);

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
