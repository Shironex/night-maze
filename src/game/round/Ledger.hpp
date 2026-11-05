// Ledger: the lamplighter's ledger, the lines of the story the player has read.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace game {

// Plain data and pure functions without a window and without files, like the rest of
// the game_logic library, so tests can check every rule. The application (NightMazeApp)
// marks a line when the card of its note opens, keeps the set in the settings file
// (GameSettings::storyRead) and writes the page of the ledger from ledgerGroups.

/// The lines that were read: bit number i is line number i of the ledger. The lines of
/// the ledger are the story lines of the notes (game::flavourLine), with the same
/// numbers. A second group of lines would take the numbers after them.
using StoryReadSet = std::uint64_t;

/// How many lines the ledger has: every story line of the notes.
int ledgerLineCount();

/// True when the line is in the set. False for a number that is no line.
bool isLineRead(StoryReadSet read, int line);

/// The set with the line in it. A number that is no line changes nothing.
StoryReadSet withLineRead(StoryReadSet read, int line);

/// How many lines of the ledger are in the set.
int readLineCount(StoryReadSet read);

/// The name of a line in the settings file, the code the story draft gives it: "N01"
/// for line 0, "N24" for line 23. Throws std::out_of_range for a number that is no line.
std::string storyLineCode(int line);

/// The set as the settings file holds it: the codes of its lines in their order, with
/// one space between two, "N01 N04 N22". Empty for an empty set.
std::string formatReadSet(StoryReadSet read);

/// Reads that text. The codes may stand in any order, with any spaces between them, and
/// a code may stand twice. A word that is no code of a line is skipped: a file written by
/// a newer version of the game, with lines this one does not know, still loads.
StoryReadSet parseReadSet(std::string_view text);

/// The set of a settings file that was written before the ledger existed, from what its
/// two counters prove:
///
///   - nextStoryLine, the story line counter of free play: every line before it that is
///     not about the shadow. The counter also moves past the lines about the shadow
///     without showing them (in a calm night, and when a file from before those lines
///     was carried over), so they are not proven by it.
///   - campaignNight, the next night of the campaign: every line of every night before
///     it. A night shows all of its lines, also the ones about the shadow.
///
/// A counter only says that a maze with these lines was finished, not that every note
/// in it was opened: this is the one place where the ledger takes a line on trust.
StoryReadSet readSetFromOldCounters(int nextStoryLine, int campaignNight);

/// How many widths the ruled blank of an unread line can have (LedgerLine::blankWidth).
constexpr int LEDGER_BLANK_WIDTH_COUNT = 3;

/// One line of the page of the ledger.
struct LedgerLine {
    /// The number of the line (game::flavourLine).
    int line = 0;

    /// True when the line is read: the page shows its text. False: a ruled blank.
    bool read = false;

    /// Which of the fixed widths the blank has, from 0 to LEDGER_BLANK_WIDTH_COUNT - 1.
    /// It follows from the number of the line and never from its text, so a blank tells
    /// nothing about the line behind it.
    int blankWidth = 0;
};

/// A part of the page with a heading of its own: the lines one night tells.
struct LedgerGroup {
    /// "Night 2: The Shepherds' Gates".
    std::string title;

    std::vector<LedgerLine> lines;
};

/// The page of the ledger: one group per night of the campaign, in their order, each
/// with the lines the night tells (CampaignNight::firstStoryLine and storyLineCount).
/// Every line of the ledger is in exactly one group.
std::vector<LedgerGroup> ledgerGroups(StoryReadSet read);

/// The counter of the page: "11 of 24".
std::string ledgerCounterText(StoryReadSet read);

} // namespace game
