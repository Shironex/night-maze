// Tests of game/Ledger: the set of story lines that were read, its text in the settings
// file, the set of a file from before the ledger, and the page with its groups.
#include "game/Ledger.hpp"

#include "game/Campaign.hpp"
#include "game/Interactables.hpp"
#include "game/Settings.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using game::GameSettings;
using game::StoryReadSet;

// The set with exactly these lines.
StoryReadSet setOf(std::initializer_list<int> lines) {
    StoryReadSet read = 0;
    for (const int line : lines) {
        read = game::withLineRead(read, line);
    }
    return read;
}

// The lines about the shadow are N10 to N17 of the story draft.
constexpr int FIRST_SHADE_LINE = 9;
constexpr int LAST_SHADE_LINE = 16;

} // namespace

TEST_CASE("the ledger has the 24 story lines, and the shadow lines are N10 to N17") {
    CHECK(game::ledgerLineCount() == 24);
    for (int line = 0; line < game::ledgerLineCount(); ++line) {
        CHECK(game::flavourLineNeedsShade(line) ==
              (line >= FIRST_SHADE_LINE && line <= LAST_SHADE_LINE));
    }
}

TEST_CASE("a line is read once it was marked, and marking it again changes nothing") {
    StoryReadSet read = 0;
    CHECK(game::readLineCount(read) == 0);
    CHECK_FALSE(game::isLineRead(read, 3));

    read = game::withLineRead(read, 3);
    CHECK(game::isLineRead(read, 3));
    CHECK_FALSE(game::isLineRead(read, 2));
    CHECK_FALSE(game::isLineRead(read, 4));
    CHECK(game::readLineCount(read) == 1);
    CHECK(game::withLineRead(read, 3) == read);

    read = game::withLineRead(read, 23);
    read = game::withLineRead(read, 0);
    CHECK(game::readLineCount(read) == 3);
}

TEST_CASE("a number that is no line is never read and marks nothing") {
    for (const int line : {-1, 24, 63, 64, 1000}) {
        CHECK(game::withLineRead(0, line) == 0);
        CHECK_FALSE(game::isLineRead(~StoryReadSet{0}, line));
    }
    // Bits past the last line do not count.
    CHECK(game::readLineCount(~StoryReadSet{0}) == 24);
}

TEST_CASE("the settings file names the read lines by their codes") {
    CHECK(game::storyLineCode(0) == "N01");
    CHECK(game::storyLineCode(9) == "N10");
    CHECK(game::storyLineCode(23) == "N24");
    CHECK_THROWS_AS(game::storyLineCode(24), std::out_of_range);
    CHECK_THROWS_AS(game::storyLineCode(-1), std::out_of_range);

    CHECK(game::formatReadSet(0).empty());
    CHECK(game::formatReadSet(setOf({21, 0, 3})) == "N01 N04 N22");
}

TEST_CASE("every set is read back from its text") {
    CHECK(game::parseReadSet("") == 0);
    for (int line = 0; line < game::ledgerLineCount(); ++line) {
        const StoryReadSet one = setOf({line});
        CHECK(game::parseReadSet(game::formatReadSet(one)) == one);
    }
    const StoryReadSet all = game::readSetFromOldCounters(0, game::CAMPAIGN_FINISHED);
    CHECK(game::readLineCount(all) == 24);
    CHECK(game::parseReadSet(game::formatReadSet(all)) == all);
}

TEST_CASE("codes are read in any order, and a word that is no code is skipped") {
    CHECK(game::parseReadSet("N22  N01\tN04 N01") == setOf({0, 3, 21}));
    // A line of a later version of the game (a second hand), a number that is no line
    // and plain rubbish: the lines this game knows still count.
    CHECK(game::parseReadSet("M03 N02 N25 N00 n03 N4 N004 hello N24") == setOf({1, 23}));
    CHECK(game::parseReadSet("   ") == 0);
}

TEST_CASE("a file from before the ledger: the story counter proves the lines before it") {
    CHECK(game::readSetFromOldCounters(0, 1) == 0);
    CHECK(game::readSetFromOldCounters(3, 1) == setOf({0, 1, 2}));
    // The counter moves past the lines about the shadow without showing them (a calm
    // night, a file carried over from the shorter table), so it does not prove them.
    CHECK(game::readSetFromOldCounters(12, 1) == setOf({0, 1, 2, 3, 4, 5, 6, 7, 8}));
    CHECK(game::readSetFromOldCounters(19, 1) == setOf({0, 1, 2, 3, 4, 5, 6, 7, 8, 17, 18}));
    // The last place of the counter: every line but the last one and the eight of the
    // shadow.
    CHECK(game::readLineCount(game::readSetFromOldCounters(23, 1)) == 23 - 8);
    // A counter outside the table marks no line that does not exist.
    CHECK(game::readLineCount(game::readSetFromOldCounters(1000, 1)) == 24 - 8);
    CHECK(game::readSetFromOldCounters(-5, 1) == 0);
}

TEST_CASE("a file from before the ledger: a won night proves all of its lines") {
    // Night 1 is won: N01 to N05.
    CHECK(game::readSetFromOldCounters(0, 2) == setOf({0, 1, 2, 3, 4}));
    // Nights 1 to 3: also the lines about the shadow, which a night always shows.
    const StoryReadSet three = game::readSetFromOldCounters(0, 4);
    CHECK(game::readLineCount(three) == 15);
    CHECK(game::isLineRead(three, FIRST_SHADE_LINE));
    CHECK(game::isLineRead(three, 14));
    CHECK_FALSE(game::isLineRead(three, 15));
    // The whole campaign: all 24.
    CHECK(game::readLineCount(game::readSetFromOldCounters(0, game::CAMPAIGN_FINISHED)) == 24);
    // Both counters together: the lines of either.
    CHECK(game::readSetFromOldCounters(19, 2) == setOf({0, 1, 2, 3, 4, 5, 6, 7, 8, 17, 18}));
    CHECK(game::readSetFromOldCounters(2, 3) == setOf({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST_CASE("the page groups the lines by the night that tells them") {
    const std::vector<game::LedgerGroup> groups = game::ledgerGroups(0);
    REQUIRE(groups.size() == 5);
    CHECK(groups[0].title == "Night 1: First Frost");
    CHECK(groups[1].title == "Night 2: The Shepherds' Gates");
    CHECK(groups[2].title == "Night 3: Lamp's Back");
    CHECK(groups[3].title == "Night 4: What the Moon Misses");
    CHECK(groups[4].title == "Night 5: The Last Lamp");
    CHECK(groups[0].lines.size() == 5);
    CHECK(groups[4].lines.size() == 4);

    // Every line of the ledger once, in its order.
    int next = 0;
    for (const game::LedgerGroup& group : groups) {
        for (const game::LedgerLine& line : group.lines) {
            CHECK(line.line == next);
            CHECK_FALSE(line.read);
            ++next;
        }
    }
    CHECK(next == game::ledgerLineCount());
}

TEST_CASE("the page shows a line as read exactly when it is in the set") {
    const StoryReadSet read = setOf({0, 6, 23});
    for (const game::LedgerGroup& group : game::ledgerGroups(read)) {
        for (const game::LedgerLine& line : group.lines) {
            CHECK(line.read == game::isLineRead(read, line.line));
        }
    }
}

TEST_CASE("the blank of an unread line has one of three widths and tells nothing") {
    std::vector<int> used(game::LEDGER_BLANK_WIDTH_COUNT, 0);
    for (const game::LedgerGroup& group : game::ledgerGroups(0)) {
        for (const game::LedgerLine& line : group.lines) {
            REQUIRE(line.blankWidth >= 0);
            REQUIRE(line.blankWidth < game::LEDGER_BLANK_WIDTH_COUNT);
            ++used[static_cast<std::size_t>(line.blankWidth)];
            // The width follows from the number of the line alone.
            CHECK(line.blankWidth == line.line % game::LEDGER_BLANK_WIDTH_COUNT);
        }
    }
    for (const int count : used) {
        CHECK(count == 8);
    }
}

TEST_CASE("the counter of the page counts the read lines of the 24") {
    CHECK(game::ledgerCounterText(0) == "0 of 24");
    CHECK(game::ledgerCounterText(setOf({0, 1, 2, 3, 4, 5, 6, 7, 8, 17, 18})) == "11 of 24");
    CHECK(game::ledgerCounterText(~StoryReadSet{0}) == "24 of 24");
}

TEST_CASE("a story line can be written into a menu document as it is") {
    // The page is filled as RML, where these two characters would start a tag or an
    // entity. The titles of the nights stand on the page too.
    for (int line = 0; line < game::ledgerLineCount(); ++line) {
        CHECK(game::flavourLine(line).find_first_of("<&") == std::string_view::npos);
    }
    for (const game::LedgerGroup& group : game::ledgerGroups(0)) {
        CHECK(group.title.find_first_of("<&") == std::string::npos);
    }
}

TEST_CASE("the ledger is written to the settings file and read back") {
    GameSettings settings;
    settings.storyRead = setOf({0, 3, 21});
    const std::string text = game::formatSettings(settings);
    CHECK(text.find("story_read = N01 N04 N22\n") != std::string::npos);
    CHECK(game::parseSettings(text) == settings);

    // The empty ledger has its line too: that line is what tells a file of this version
    // from an older one.
    CHECK(game::formatSettings(GameSettings{}).find("story_read = \n") != std::string::npos);

    CHECK(game::applySetting(settings, "story_read", "N02"));
    CHECK(settings.storyRead == setOf({1}));
    CHECK(game::applySetting(settings, "story_read", ""));
    CHECK(settings.storyRead == 0);
}

TEST_CASE("a file from before the ledger gets the lines its counters prove, once") {
    // Free play was at line 3 and two nights were won.
    const GameSettings old = game::parseSettings("story_line = 3\ncampaign_night = 3\n");
    CHECK(old.storyRead == game::readSetFromOldCounters(3, 3));
    CHECK(game::readLineCount(old.storyRead) == 10);
    // Nothing else of the file moves.
    CHECK(old.nextStoryLine == 3);
    CHECK(old.campaignNight == 3);

    // The counter of game 0.11, in the shorter table: carried over first, then asked.
    const GameSettings older = game::parseSettings("next_story_line = 12\n");
    CHECK(older.storyRead == game::readSetFromOldCounters(older.nextStoryLine, 1));
    CHECK(game::readLineCount(older.storyRead) == 12);

    // A new player: nothing is read.
    CHECK(game::parseSettings("").storyRead == 0);
}

TEST_CASE("a file with a ledger keeps it, whatever its counters say") {
    // The counters moved on without a note being opened: the ledger stays as it is.
    CHECK(game::parseSettings("story_line = 6\ncampaign_night = 4\nstory_read = \n").storyRead ==
          0);
    CHECK(game::parseSettings("story_read = N24\nstory_line = 6\n").storyRead == setOf({23}));
    // So a file that was written once is never migrated again.
    GameSettings settings;
    settings.nextStoryLine = 6;
    settings.campaignNight = 4;
    CHECK(game::parseSettings(game::formatSettings(settings)).storyRead == 0);
}

TEST_CASE("reset defaults leaves the ledger alone") {
    GameSettings settings;
    settings.storyRead = setOf({0, 1, 9, 23});
    CHECK(game::resetSettings(settings).storyRead == settings.storyRead);
}
