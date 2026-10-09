// Daily: "Tonight's hedge", the maze of the day, and the rules of its best time.
#pragma once

#include "game/Difficulty.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace game {

// Plain data and pure functions without a window, without files and without a clock,
// like the rest of the game_logic library, so tests can check every rule. The story says
// the hedge "still grows a new way each day": the main menu has an entry that plays the
// maze of the day, the same one for every player. The application (NightMazeApp) asks
// the clock for the day, builds the maze and keeps the record in the settings file
// (GameSettings::daily).
//
// A day is written as one number, year, month and day in a row: 8 October 2026 is
// 20261008. That number is also the seed of the maze of the day, as it stands.

/// The name of the maze of the day: the entry of the main menu, the title card, the HUD
/// and the pause menu all show it.
constexpr const char* DAILY_NAME = "Tonight's hedge";

/// The level whose numbers the maze of the day has: its size, its crystals, the gate, the
/// battery and the flasks. The shade is always in it: the switch "Calm night" of free
/// play is not asked.
constexpr Difficulty DAILY_DIFFICULTY = Difficulty::Normal;

/// "No day": no maze of the day is in play, or no day has a best time yet.
constexpr std::uint32_t NO_DAILY_DATE = 0;

/// The day of a run that a tool drives (game::fixedDailyDate): such a run never asks the
/// clock, so its pictures are the same on every day.
constexpr std::uint32_t TOOL_DAILY_DATE = 20261008;

/// The most days the count of days goes up to (DailyRecord::daysPlayed).
constexpr int MAX_DAILY_DAYS = 99999;

/// A day of the calendar as its number: 9 October 2026 is 20261009. NO_DAILY_DATE for
/// a day the calendar does not have (30 February) and for a year that has not four
/// digits (before 1000 or after 9999).
std::uint32_t dailyDate(std::chrono::year_month_day day);

/// True when the number is a day of the calendar written that way. 20280229 is one
/// (2028 is a leap year), 20260229 and 20261332 are not.
bool isDailyDate(std::uint32_t date);

/// Reads a day from text: exactly eight digits that are a day of the calendar
/// ("20261009"). False, and date left as it was, for anything else. The command line
/// (--daily) and the settings file both read their text with it.
bool parseDailyDate(std::string_view text, std::uint32_t& date);

/// A day as the menus show it, the day and the English name of the month: "9 October".
/// An empty text for a number that is no day.
std::string dailyDateText(std::uint32_t date);

/// What is kept of the mazes of the day: the best time of ONE day. A win on another day
/// replaces it. A history of the days is not kept.
struct DailyRecord {
    /// The day the best time belongs to, or NO_DAILY_DATE until a maze of the day is won.
    std::uint32_t date = NO_DAILY_DATE;

    /// The best time of that day in whole seconds (game::bestAfterWin), or NO_BEST_TIME.
    int bestSeconds = 0;

    /// On how many days a maze of the day was won, up to MAX_DAILY_DAYS.
    int daysPlayed = 0;

    bool operator==(const DailyRecord& other) const = default;
};

/// The best time the record has for this day, or NO_BEST_TIME: the time of another day
/// does not count.
int dailyBestOn(const DailyRecord& record, std::uint32_t date);

/// The record after a win, and whether the win was a new best.
struct DailyWin {
    DailyRecord record;

    /// True for the first win of the day and for a time that is shorter, in whole
    /// seconds, than the best of the day so far.
    bool newBest = false;
};

/// The record after the maze of date was won in elapsedSeconds. On the day of the record
/// the shorter time stays (game::bestAfterWin). A win on another day starts the record
/// anew with this time, and the count of days goes up by one.
DailyWin dailyAfterWin(const DailyRecord& record, std::uint32_t date, float elapsedSeconds);

/// What stands beside the entry of the main menu on the day date: the day, and the best
/// time of that day once it has one. "9 October" or "9 October | best 4:12".
std::string dailyMenuLine(const DailyRecord& record, std::uint32_t date);

/// The line under the title of the result screen after a win: "New best", or the time
/// to beat, "Today's best 4:12".
std::string dailyResultLine(const DailyWin& win);

} // namespace game
