// Ledger: the lamplighter's ledger, the lines of the story the player has read.
#include "game/Ledger.hpp"

#include "game/Campaign.hpp"
#include "game/Interactables.hpp"

#include <bit>
#include <cstddef>
#include <stdexcept>

namespace game {

namespace {

// A code is this letter and the number of the line, counted from 1, with two digits.
constexpr char CODE_LETTER = 'N';
constexpr std::size_t CODE_LENGTH = 3;
constexpr int DECIMAL_BASE = 10;

// Between two codes.
constexpr std::string_view BLANKS = " \t";

// The bit of a line.
StoryReadSet lineBit(int line) {
    return StoryReadSet{1} << static_cast<unsigned>(line);
}

bool isLine(int line) {
    return line >= 0 && line < ledgerLineCount();
}

// The line a code names, or -1 for a word that is no code of a line.
int lineOfCode(std::string_view code) {
    if (code.size() != CODE_LENGTH || code[0] != CODE_LETTER || code[1] < '0' || code[1] > '9' ||
        code[2] < '0' || code[2] > '9') {
        return -1;
    }
    const int line = (code[1] - '0') * DECIMAL_BASE + (code[2] - '0') - 1;
    return isLine(line) ? line : -1;
}

} // namespace

int ledgerLineCount() {
    return flavourLineCount();
}

bool isLineRead(StoryReadSet read, int line) {
    return isLine(line) && (read & lineBit(line)) != 0;
}

StoryReadSet withLineRead(StoryReadSet read, int line) {
    return isLine(line) ? read | lineBit(line) : read;
}

int readLineCount(StoryReadSet read) {
    // Only the bits of lines: lineBit(count) - 1 has exactly those set.
    return std::popcount(read & (lineBit(ledgerLineCount()) - 1));
}

std::string storyLineCode(int line) {
    if (!isLine(line)) {
        throw std::out_of_range("storyLineCode: no such line");
    }
    const int number = line + 1;
    return {CODE_LETTER, static_cast<char>('0' + number / DECIMAL_BASE),
            static_cast<char>('0' + number % DECIMAL_BASE)};
}

std::string formatReadSet(StoryReadSet read) {
    std::string text;
    for (int line = 0; line < ledgerLineCount(); ++line) {
        if (isLineRead(read, line)) {
            text += (text.empty() ? "" : " ") + storyLineCode(line);
        }
    }
    return text;
}

StoryReadSet parseReadSet(std::string_view text) {
    StoryReadSet read = 0;
    // Word after word: the text up to the next blank, then the rest.
    while (!text.empty()) {
        const std::size_t end = text.find_first_of(BLANKS);
        read = withLineRead(read, lineOfCode(text.substr(0, end)));
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
    }
    return read;
}

StoryReadSet readSetFromOldCounters(int nextStoryLine, int campaignNight) {
    StoryReadSet read = 0;
    for (int line = 0; line < nextStoryLine && line < ledgerLineCount(); ++line) {
        if (!flavourLineNeedsShade(line)) {
            read = withLineRead(read, line);
        }
    }
    for (int night = 1; night <= CAMPAIGN_NIGHT_COUNT; ++night) {
        if (nightStatus(campaignNight, night) != NightStatus::Finished) {
            continue;
        }
        const CampaignNight& told = game::campaignNight(night);
        for (int i = 0; i < told.storyLineCount; ++i) {
            read = withLineRead(read, told.firstStoryLine + i);
        }
    }
    return read;
}

std::vector<LedgerGroup> ledgerGroups(StoryReadSet read) {
    std::vector<LedgerGroup> groups;
    for (int night = 1; night <= CAMPAIGN_NIGHT_COUNT; ++night) {
        const CampaignNight& told = campaignNight(night);
        LedgerGroup group{.title = campaignNightName(night), .lines = {}};
        for (int i = 0; i < told.storyLineCount; ++i) {
            const int line = told.firstStoryLine + i;
            group.lines.push_back({.line = line,
                                   .read = isLineRead(read, line),
                                   .blankWidth = line % LEDGER_BLANK_WIDTH_COUNT});
        }
        groups.push_back(std::move(group));
    }
    return groups;
}

std::string ledgerCounterText(StoryReadSet read) {
    return std::to_string(readLineCount(read)) + " of " + std::to_string(ledgerLineCount());
}

} // namespace game
