// Difficulty: the three levels of a new game as one table of numbers.
#include "game/Difficulty.hpp"

#include <cstddef>

namespace game {

namespace {

// One row per level, in the order of the enum: the number of a level is its row.
constexpr std::array<DifficultyLevel, ALL_DIFFICULTIES.size()> LEVELS = {{
    {.name = "Easy",
     .key = "easy",
     .mazeWidth = 10,
     .mazeHeight = 10,
     .crystalCount = 13,
     .requiredFraction = 0.7F,
     .batteryLifetimeSeconds = 180.0F},
    {.name = "Normal",
     .key = "normal",
     .mazeWidth = 16,
     .mazeHeight = 16,
     .crystalCount = 26,
     .requiredFraction = 0.7F,
     .batteryLifetimeSeconds = 150.0F},
    {.name = "Hard",
     .key = "hard",
     .mazeWidth = 22,
     .mazeHeight = 22,
     .crystalCount = 40,
     .requiredFraction = 0.8F,
     .batteryLifetimeSeconds = 120.0F},
}};

} // namespace

const DifficultyLevel& difficultyLevel(Difficulty difficulty) {
    return LEVELS.at(static_cast<std::size_t>(difficulty));
}

bool difficultyFromKey(std::string_view key, Difficulty& difficulty) {
    for (const Difficulty candidate : ALL_DIFFICULTIES) {
        if (key == difficultyLevel(candidate).key) {
            difficulty = candidate;
            return true;
        }
    }
    return false;
}

} // namespace game
