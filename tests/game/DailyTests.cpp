// Tests of game/Daily: the day as a number and as text, the seed of the maze of the day,
// the best time of a day and the lines the menus show.
#include "game/Daily.hpp"

#include "game/Campaign.hpp"
#include "game/Difficulty.hpp"
#include "game/MazeWorld.hpp"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace {

using game::DailyRecord;
using game::DailyWin;

// A day of the calendar from three plain numbers.
std::chrono::year_month_day day(int year, unsigned month, unsigned dayOfMonth) {
    return std::chrono::year{year} / std::chrono::month{month} / std::chrono::day{dayOfMonth};
}

} // namespace

TEST_CASE("the seed of the maze of the day is the date as it is written") {
    // The example of the design: 8 October 2026 is seed 20261008.
    CHECK(game::dailyDate(day(2026, 10, 8)) == 20261008);
    CHECK(game::dailyDate(day(2026, 1, 1)) == 20260101);
    CHECK(game::dailyDate(day(2026, 12, 31)) == 20261231);
    CHECK(game::TOOL_DAILY_DATE == 20261008);
    CHECK(game::isDailyDate(game::TOOL_DAILY_DATE));

    // Two days next to each other are two seeds, and the same day is the same seed.
    CHECK(game::dailyDate(day(2026, 10, 9)) == game::dailyDate(day(2026, 10, 8)) + 1);
    CHECK(game::dailyDate(day(2026, 10, 8)) == game::dailyDate(day(2026, 10, 8)));
}

TEST_CASE("the maze of the day is the maze of free play on Normal with that seed") {
    CHECK(game::DAILY_DIFFICULTY == game::Difficulty::Normal);
    const game::DifficultyLevel& level = game::difficultyLevel(game::DAILY_DIFFICULTY);
    const auto build = [&level](std::uint32_t seed) {
        return game::buildMazeWorld(level.mazeWidth, level.mazeHeight, seed, {},
                                    level.crystalCount);
    };
    // The same day gives the same maze, so every player walks the same one.
    const game::MazeWorld first = build(20261008);
    const game::MazeWorld again = build(game::dailyDate(day(2026, 10, 8)));
    CHECK(first.seed == 20261008);
    CHECK(first.maze.width() == level.mazeWidth);
    CHECK(first.exitCell.x == again.exitCell.x);
    CHECK(first.exitCell.z == again.exitCell.z);
    REQUIRE(first.crystals.size() == again.crystals.size());
    for (std::size_t i = 0; i < first.crystals.size(); ++i) {
        CHECK(first.crystals.at(i).cell.x == again.crystals.at(i).cell.x);
        CHECK(first.crystals.at(i).cell.z == again.crystals.at(i).cell.z);
    }
}

TEST_CASE("a day the calendar does not have is no day") {
    CHECK(game::dailyDate(day(2026, 2, 30)) == game::NO_DAILY_DATE);
    CHECK(game::dailyDate(day(2026, 13, 1)) == game::NO_DAILY_DATE);
    CHECK(game::dailyDate(day(2026, 4, 31)) == game::NO_DAILY_DATE);
    CHECK(game::dailyDate(day(2026, 0, 10)) == game::NO_DAILY_DATE);
    CHECK(game::dailyDate(day(2026, 10, 0)) == game::NO_DAILY_DATE);
    // A year is written with four digits, so the number always has eight.
    CHECK(game::dailyDate(day(999, 12, 31)) == game::NO_DAILY_DATE);
    CHECK(game::dailyDate(day(1000, 1, 1)) == 10000101);
    CHECK(game::dailyDate(day(9999, 12, 31)) == 99991231);
    CHECK(game::dailyDate(day(10000, 1, 1)) == game::NO_DAILY_DATE);
    CHECK(game::dailyDate(day(-5, 1, 1)) == game::NO_DAILY_DATE);

    CHECK(game::isDailyDate(20261009));
    CHECK_FALSE(game::isDailyDate(game::NO_DAILY_DATE));
    CHECK_FALSE(game::isDailyDate(20261332));
    CHECK_FALSE(game::isDailyDate(20261000));
    CHECK_FALSE(game::isDailyDate(20260010));
    CHECK_FALSE(game::isDailyDate(7));
    CHECK_FALSE(game::isDailyDate(9991231));
    // More than eight digits, up to the largest number a seed can be.
    CHECK_FALSE(game::isDailyDate(202610090));
    CHECK_FALSE(game::isDailyDate(4294967295U));
}

TEST_CASE("29 February is a day in a leap year only") {
    CHECK(game::isDailyDate(20280229));
    CHECK(game::isDailyDate(20240229));
    // Divisible by 400: a leap year. Divisible by 100 only: not one.
    CHECK(game::isDailyDate(20000229));
    CHECK_FALSE(game::isDailyDate(21000229));
    CHECK_FALSE(game::isDailyDate(20260229));
    CHECK_FALSE(game::isDailyDate(20270229));
    CHECK_FALSE(game::isDailyDate(20280230));
    CHECK(game::dailyDate(day(2028, 2, 29)) == 20280229);
    CHECK(game::dailyDateText(20280229) == "29 February");
    CHECK(game::dailyDateText(20260229).empty());
}

TEST_CASE("a day is read from exactly eight digits") {
    std::uint32_t date = 5;
    CHECK(game::parseDailyDate("20261009", date));
    CHECK(date == 20261009);
    CHECK(game::parseDailyDate("20280229", date));
    CHECK(date == 20280229);

    // Anything else is refused, and the number stays what it was.
    for (const char* text : {"", "2026109", "202610091", "2026-10-09", "20261332", "20260229",
                             "00000000", "2026100a", " 20261009", "20261009 ", "-2026100"}) {
        CAPTURE(text);
        date = 5;
        CHECK_FALSE(game::parseDailyDate(text, date));
        CHECK(date == 5);
    }
}

TEST_CASE("a day is shown as its number and the English name of its month") {
    const std::array<const char*, 12> months = {"January",   "February", "March",    "April",
                                                "May",       "June",     "July",     "August",
                                                "September", "October",  "November", "December"};
    for (std::uint32_t month = 1; month <= months.size(); ++month) {
        CAPTURE(month);
        CHECK(game::dailyDateText(20260000 + month * 100 + 15) ==
              std::string("15 ") + months.at(month - 1));
    }
    // No leading zero, and no year: the menu shows the day of today.
    CHECK(game::dailyDateText(20261009) == "9 October");
    CHECK(game::dailyDateText(20261008) == "8 October");
    CHECK(game::dailyDateText(20261231) == "31 December");
    CHECK(game::dailyDateText(20260101) == "1 January");
    // A number that is no day has no text.
    CHECK(game::dailyDateText(game::NO_DAILY_DATE).empty());
    CHECK(game::dailyDateText(20261332).empty());
}

TEST_CASE("the first win of a day is a new best and counts the day") {
    const DailyWin win = game::dailyAfterWin({}, 20261009, 252.9F);
    CHECK(win.newBest);
    CHECK(win.record.date == 20261009);
    // Whole seconds, parts cut off, like every best time (game::bestAfterWin).
    CHECK(win.record.bestSeconds == 252);
    CHECK(win.record.daysPlayed == 1);
    CHECK(game::dailyBestOn(win.record, 20261009) == 252);
}

TEST_CASE("on the same day only a shorter time is a new best") {
    const DailyRecord record{.date = 20261009, .bestSeconds = 252, .daysPlayed = 4};

    const DailyWin faster = game::dailyAfterWin(record, 20261009, 190.0F);
    CHECK(faster.newBest);
    CHECK(faster.record == DailyRecord{.date = 20261009, .bestSeconds = 190, .daysPlayed = 4});

    const DailyWin slower = game::dailyAfterWin(record, 20261009, 300.0F);
    CHECK_FALSE(slower.newBest);
    CHECK(slower.record == record);

    // The same whole second is no new best, and neither is a part of a second less.
    CHECK_FALSE(game::dailyAfterWin(record, 20261009, 252.0F).newBest);
    CHECK_FALSE(game::dailyAfterWin(record, 20261009, 252.9F).newBest);
    CHECK(game::dailyAfterWin(record, 20261009, 251.9F).newBest);
}

TEST_CASE("a win on another day replaces the best time and counts one more day") {
    const DailyRecord record{.date = 20261009, .bestSeconds = 100, .daysPlayed = 4};

    // Slower than the best of the day before: still the best of its own day.
    const DailyWin next = game::dailyAfterWin(record, 20261010, 400.0F);
    CHECK(next.newBest);
    CHECK(next.record == DailyRecord{.date = 20261010, .bestSeconds = 400, .daysPlayed = 5});

    // The time of another day is not the time to beat.
    CHECK(game::dailyBestOn(record, 20261010) == game::NO_BEST_TIME);
    CHECK(game::dailyBestOn(record, 20261009) == 100);
    CHECK(game::dailyBestOn({}, 20261009) == game::NO_BEST_TIME);

    // The limits of every best time, and of the count of days.
    CHECK(game::dailyAfterWin({}, 20261009, 0.2F).record.bestSeconds == 1);
    CHECK(game::dailyAfterWin({}, 20261009, 1.0e9F).record.bestSeconds == game::MAX_BEST_SECONDS);
    const DailyRecord many{
        .date = 20261009, .bestSeconds = 100, .daysPlayed = game::MAX_DAILY_DAYS};
    CHECK(game::dailyAfterWin(many, 20261010, 50.0F).record.daysPlayed == game::MAX_DAILY_DAYS);
}

TEST_CASE("the menu shows the day, and the best time once the day has one") {
    CHECK(game::dailyMenuLine({}, 20261009) == "9 October");
    const DailyRecord record{.date = 20261009, .bestSeconds = 252, .daysPlayed = 1};
    CHECK(game::dailyMenuLine(record, 20261009) == "9 October | best 4:12");
    // The next day the best time of yesterday is not shown any more.
    CHECK(game::dailyMenuLine(record, 20261010) == "10 October");
    CHECK(game::dailyMenuLine({.date = 20261009, .bestSeconds = 65, .daysPlayed = 1}, 20261009) ==
          "9 October | best 1:05");
}

TEST_CASE("the result screen says new best, or names the time to beat") {
    const DailyWin first = game::dailyAfterWin({}, 20261009, 252.0F);
    CHECK(game::dailyResultLine(first) == "New best");
    const DailyWin slower = game::dailyAfterWin(first.record, 20261009, 300.0F);
    CHECK(game::dailyResultLine(slower) == "Today's best 4:12");
    CHECK(std::string(game::DAILY_NAME) == "Tonight's hedge");
}

TEST_CASE("a time is written as minutes and seconds") {
    CHECK(game::timeText(0.0F) == "0:00");
    CHECK(game::timeText(5.9F) == "0:05");
    CHECK(game::timeText(65.0F) == "1:05");
    CHECK(game::timeText(252.0F) == "4:12");
    CHECK(game::timeText(600.0F) == "10:00");
    CHECK(game::timeText(static_cast<float>(game::MAX_BEST_SECONDS)) == "99:59");
}
