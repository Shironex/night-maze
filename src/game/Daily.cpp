// Daily: "Tonight's hedge", the maze of the day, and the rules of its best time.
#include "game/Daily.hpp"

#include "game/Campaign.hpp"
#include "game/StartOptions.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace game {

namespace {

// The names of the months, January first.
constexpr std::array<const char*, 12> MONTH_NAMES = {
    "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",   "September", "October", "November", "December"};

// A day as a number: the year, then two digits of the month, then two of the day.
constexpr std::uint32_t YEAR_FACTOR = 10000;
constexpr std::uint32_t MONTH_FACTOR = 100;

// The years that are written with four digits.
constexpr int FIRST_YEAR = 1000;
constexpr int LAST_YEAR = 9999;

// How many characters a day has as text: "20261009".
constexpr std::size_t DATE_TEXT_LENGTH = 8;

// The three parts of a number that is read as a day. Whether the calendar has that day
// is not asked here (isDailyDate).
std::chrono::year_month_day calendarDay(std::uint32_t date) {
    return std::chrono::year{static_cast<int>(date / YEAR_FACTOR)} /
           std::chrono::month{date / MONTH_FACTOR % MONTH_FACTOR} /
           std::chrono::day{date % MONTH_FACTOR};
}

} // namespace

std::uint32_t dailyDate(std::chrono::year_month_day day) {
    const int year = static_cast<int>(day.year());
    if (!day.ok() || year < FIRST_YEAR || year > LAST_YEAR) {
        return NO_DAILY_DATE;
    }
    return static_cast<std::uint32_t>(year) * YEAR_FACTOR +
           static_cast<unsigned>(day.month()) * MONTH_FACTOR + static_cast<unsigned>(day.day());
}

bool isDailyDate(std::uint32_t date) {
    // There and back: a number the calendar does not have comes back as NO_DAILY_DATE,
    // and a number with more than eight digits comes back as another one.
    return date != NO_DAILY_DATE && dailyDate(calendarDay(date)) == date;
}

bool parseDailyDate(std::string_view text, std::uint32_t& date) {
    std::uint32_t number = 0;
    if (text.size() != DATE_TEXT_LENGTH || !parseSeed(text, number) || !isDailyDate(number)) {
        return false;
    }
    date = number;
    return true;
}

std::string dailyDateText(std::uint32_t date) {
    if (!isDailyDate(date)) {
        return {};
    }
    const std::size_t month = date / MONTH_FACTOR % MONTH_FACTOR;
    return std::to_string(date % MONTH_FACTOR) + " " + MONTH_NAMES.at(month - 1);
}

int dailyBestOn(const DailyRecord& record, std::uint32_t date) {
    return record.date == date ? record.bestSeconds : NO_BEST_TIME;
}

DailyWin dailyAfterWin(const DailyRecord& record, std::uint32_t date, float elapsedSeconds) {
    const int before = dailyBestOn(record, date);
    DailyWin win{.record = record, .newBest = false};
    win.record.date = date;
    win.record.bestSeconds = bestAfterWin(before, elapsedSeconds);
    if (record.date != date) {
        win.record.daysPlayed = std::min(record.daysPlayed + 1, MAX_DAILY_DAYS);
    }
    win.newBest = win.record.bestSeconds != before;
    return win;
}

std::string dailyMenuLine(const DailyRecord& record, std::uint32_t date) {
    const int best = dailyBestOn(record, date);
    const std::string day = dailyDateText(date);
    return best == NO_BEST_TIME ? day : day + " | best " + timeText(static_cast<float>(best));
}

std::string dailyResultLine(const DailyWin& win) {
    return win.newBest ? "New best"
                       : "Today's best " + timeText(static_cast<float>(win.record.bestSeconds));
}

} // namespace game
