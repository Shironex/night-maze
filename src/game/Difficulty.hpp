// Difficulty: the three levels of a new game as one table of numbers.
#pragma once

#include <array>
#include <string_view>

namespace game {

// Plain data and pure functions without a window, like the rest of the game_logic
// library, so tests can check the table.

/// How hard a new game is. The numbers behind each level are in difficultyLevel.
enum class Difficulty {
    Easy = 0,
    Normal,
    Hard,
};

/// Every level, in the order the main menu shows them.
constexpr std::array<Difficulty, 3> ALL_DIFFICULTIES = {Difficulty::Easy, Difficulty::Normal,
                                                        Difficulty::Hard};

/// The numbers of one level: everything that differs between an easy and a hard game.
/// A new game copies them into the request for the maze (MazeSettings) and into the
/// rules of the round (GameplaySettings).
struct DifficultyLevel {
    /// The name the menus show: "Easy".
    const char* name;

    /// The name in the settings file and in the documents of the menu: "easy". Lower
    /// case and without spaces.
    const char* key;

    /// The size of the maze in cells: columns and rows.
    int mazeWidth;
    int mazeHeight;

    /// How many crystals float in the maze (MazeSettings::crystalCount).
    int crystalCount;

    /// How many flasks of tea lie in the maze (GameplaySettings::flaskCount). A larger
    /// maze has more of them, because it has more ground to run over.
    int flaskCount;

    /// The part of the crystals that opens the gate (GameplaySettings::requiredFraction).
    float requiredFraction;

    /// How long a full battery lasts with the flashlight on, in seconds
    /// (GameplaySettings::batteryLifetimeSeconds). A collected crystal gives a quarter of
    /// it back, so a shorter battery also makes every crystal worth less light.
    float batteryLifetimeSeconds;
};

/// The numbers of a level. The table, with what was measured over 50 mazes of each
/// size (seeds 1 to 50). "Shortest round" is the shortest walk that collects enough
/// crystals and ends in the exit, for a player who knows the whole maze, at walking
/// speed (3 m/s). A player who has to search needs a multiple of it.
///
///     level   maze     crystals  gate opens at  battery  way to the exit  shortest round
///     Easy    10 x 10  13        70 % (10)      180 s    135 m            211 m, 1:10
///     Normal  16 x 16  26        70 % (19)      150 s    312 m            538 m, 2:59
///     Hard    22 x 22  40        80 % (32)      120 s    527 m            1081 m, 6:00
///
/// The levels also have 1, 2 and 3 flasks of tea. Those numbers are a first guess and
/// were not measured.
///
/// Easy is the game as it was before the levels existed. Each step up makes the maze
/// larger (more to search), asks for more crystals and shortens the battery, so on Hard
/// the light is something to plan with: a full battery plus every needed crystal gives
/// 18 minutes of light for a round whose shortest walk already takes 6.
const DifficultyLevel& difficultyLevel(Difficulty difficulty);

/// The level with this key ("easy", "normal" or "hard"). False for any other text:
/// difficulty is left as it was.
bool difficultyFromKey(std::string_view key, Difficulty& difficulty);

} // namespace game
