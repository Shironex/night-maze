// Campaign: the five nights of the story as one table of numbers, and the rules of the
// progress through them.
#pragma once

#include "game/Interactables.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace game {

// Plain data and pure functions without a window and without files, like the rest of
// the game_logic library, so tests can check every rule. The application
// (NightMazeApp) keeps the progress in the settings file (GameSettings::campaignNight,
// campaignSeed and campaignBestSeconds), asks the questions below and builds the maze
// of a night from the numbers of the table.
//
// The campaign is the story told in order: five nights, each in a larger maze with
// less light. It is not a difficulty of free play (game/Difficulty.hpp): the table
// here is its own, and the picker of free play keeps its three levels.

/// How many nights the campaign has. The nights are counted from 1.
constexpr int CAMPAIGN_NIGHT_COUNT = 5;

/// The "next night" of a campaign whose last night is won: one past the last.
constexpr int CAMPAIGN_FINISHED = CAMPAIGN_NIGHT_COUNT + 1;

/// The campaign seed of a game that has not started a campaign yet. A seed that is
/// drawn is never this number (drawnCampaignSeed).
constexpr std::uint32_t NO_CAMPAIGN_SEED = 0;

/// The campaign seed of a night that is started from the command line (--night)
/// without a seed of its own (--seed): tools and tests then always get the same maze.
constexpr std::uint32_t TOOL_CAMPAIGN_SEED = 1;

/// A drawn campaign seed and the seed of the maze of a night are below this number: at
/// most six digits, like the random seeds of free play.
constexpr std::uint32_t CAMPAIGN_SEED_LIMIT = 1000000;

/// The numbers of one night: everything that differs between the first and the last.
struct CampaignNight {
    /// The name of the night: "First Frost".
    const char* title;

    /// The size of the maze in cells: columns and rows.
    int mazeWidth;
    int mazeHeight;

    /// How many crystals float in the maze, and how many flasks of tea lie in it.
    int crystalCount;
    int flaskCount;

    /// The part of the crystals that opens the gate.
    float requiredFraction;

    /// How long a full battery lasts with the flashlight on, in seconds.
    float batteryLifetimeSeconds;

    /// True when the shade walks in the maze. The first night has none, whatever the
    /// switch "Calm night" of free play says: it is the night the player learns the walk.
    bool shade;

    /// How many notes tell the story, and how many are hints (to the exit or to
    /// a crystal).
    int storyNotes;
    int hintNotes;

    /// The story lines of the night: the first one (a number of game::flavourLine) and
    /// how many belong to the night. The story notes show them in this order, the note
    /// nearest to the start first.
    int firstStoryLine;
    int storyLineCount;

    /// The line under the title of the result screen when the night is won. Empty for
    /// the last night: the ending card comes in its place (endingLines).
    const char* endLine;
};

/// The numbers of a night, 1 to CAMPAIGN_NIGHT_COUNT. The table:
///
///     night  maze     crystals  gate opens at  battery  flasks  shade  notes
///     1      10 x 10  13        70 % (10)      180 s    1       no     5 story + 2 hint
///     2      13 x 13  19        70 % (14)      165 s    1       yes    5 story + 3 hint
///     3      16 x 16  26        70 % (19)      150 s    2       yes    5 story + 4 hint
///     4      19 x 19  33        80 % (27)      135 s    2       yes    5 story + 5 hint
///     5      22 x 22  40        80 % (32)      120 s    3       yes    4 story + 6 hint
///
/// Their titles: First Frost, The Shepherds' Gates, Lamp's Back, What the Moon Misses and
/// The Last Lamp.
///
/// Nights 1, 3 and 5 have the numbers of the levels Easy, Normal and Hard of free play.
/// Nights 2 and 4 lie between them. The story lines are the 24 lines of the table of
/// notes in their order: five for each of the first four nights and four for the last.
///
/// Throws std::out_of_range for a number that is no night.
const CampaignNight& campaignNight(int night);

/// A night as the menus name it: "Night 2: The Shepherds' Gates". Throws like
/// campaignNight.
std::string campaignNightName(int night);

/// A night as a short label, without its title: "Night 2".
std::string campaignNightLabel(int night);

/// The seed of the maze of a night: it follows from the seed of the campaign and the
/// number of the night and from nothing else, so every night of a campaign keeps its
/// maze, and a night that is played again is the same maze. From 1 to
/// CAMPAIGN_SEED_LIMIT - 1. Whole number arithmetic only, so it is the same on every
/// compiler.
std::uint32_t campaignNightSeed(std::uint32_t campaignSeed, int night);

/// A campaign seed made from any random number: from 1 to CAMPAIGN_SEED_LIMIT - 1, so
/// never NO_CAMPAIGN_SEED.
std::uint32_t drawnCampaignSeed(std::uint32_t randomNumber);

/// The request for the levers and notes of a night. The mix of the notes and the story
/// line they start with are the ones of the night, and no line about the shadow is
/// skipped: the lines of a night are chosen for it. Everything else (the number of
/// levers) is taken from settings. The story line counter of free play is neither read
/// nor moved by a night. Throws like campaignNight.
InteractableSettings campaignInteractables(int night, const InteractableSettings& settings);

/// How far a campaign is, for the first entry of the main menu.
enum class CampaignStage {
    NotStarted = 0, ///< no night is won: the entry reads "Begin"
    Running,        ///< at least one night is won, and one is left: "Continue"
    Finished,       ///< the last night is won: "New campaign"
};

/// The stage of a campaign whose next night is campaignNight (1 to CAMPAIGN_FINISHED).
CampaignStage campaignStage(int campaignNight);

/// A night in the list of nights.
enum class NightStatus {
    Finished = 0, ///< won: it shows its best time and can be played again
    Open,         ///< the next night to play
    Locked,       ///< a later night: it cannot be started
};

/// The status of a night in a campaign whose next night is campaignNight.
NightStatus nightStatus(int campaignNight, int night);

/// The night the first entry of the main menu starts, and whose numbers the facts of
/// the main menu show: the next night, or the first one of a new campaign when the
/// campaign is finished.
int nightToOffer(int campaignNight);

/// The next night of the campaign after wonNight was won. Winning the next night moves
/// the campaign on by one. Winning a finished night again changes nothing.
int nightAfterWin(int campaignNight, int wonNight);

/// "No best time yet" in GameSettings::campaignBestSeconds.
constexpr int NO_BEST_TIME = 0;

/// The longest time that is kept: 99 minutes and 59 seconds, the most the list shows.
constexpr int MAX_BEST_SECONDS = 5999;

/// The best time of a night after it was won in elapsedSeconds: the shorter of the two,
/// in whole seconds (parts of a second are cut off, like on the result screen), at
/// least 1 and at most MAX_BEST_SECONDS. bestSeconds is the time so far, or
/// NO_BEST_TIME.
int bestAfterWin(int bestSeconds, float elapsedSeconds);

/// The most lines a card of text shows (assets/ui/card.rml has that many).
constexpr std::size_t STORY_CARD_LINE_COUNT = 4;

/// The four lines of the ending card, shown after the last night. The third one depends
/// on whether the player left at least one crystal in the maze: the lamplighters leave
/// a few on purpose.
std::array<const char*, STORY_CARD_LINE_COUNT> endingLines(bool crystalsLeft);

/// What a card of text on black shows at one moment: the title card of a night and the
/// ending card.
struct StoryCardFrame {
    /// True when the moment lies at or past the end of the card.
    bool finished = false;

    /// How much of each line is there: 0 is none, 1 is all of it.
    std::array<float, STORY_CARD_LINE_COUNT> lineOpacity{};

    /// How much of the hint "Press any key to skip" is there.
    float hintOpacity = 0.0F;

    /// True when a key may skip the card at this moment: not in its first half second
    /// (INTRO_SKIP_DELAY_SECONDS), because the key that started the night is still
    /// down then.
    bool skippable = false;
};

/// How long the title card of a night takes, in seconds.
constexpr float NIGHT_CARD_SECONDS = 3.0F;

/// The title card of a night, seconds after it came up: its two lines come and go
/// together, the way the lines of a card of the intro do (game::cardTextOpacity). It
/// has no hint: it is over before one could be read.
StoryCardFrame nightCardFrame(float seconds);

/// The ending card: its four lines come up one after another, ENDING_LINE_SECONDS
/// apart, stay together and leave together. The bell of the intro rings once, at
/// ENDING_BELL_SECONDS: early enough to ring out before the card ends.
constexpr float ENDING_LINE_SECONDS = 2.5F;
constexpr float ENDING_CARD_SECONDS = 15.0F;
constexpr float ENDING_BELL_SECONDS = 0.5F;
StoryCardFrame endingCardFrame(float seconds);

/// What the debug window asks of the campaign, for testing. It only writes this data: the
/// application reads it at the start of its next frame, does what is asked and clears
/// it, like MazeSettings::regenerate.
struct CampaignRequest {
    /// The next night the campaign should have, 1 to CAMPAIGN_FINISHED: the nights before
    /// it then count as won (without a best time). 0: nothing is asked.
    int setNight = 0;

    /// True: forget the campaign. No night is won, no seed, no best time.
    bool clear = false;

    /// True: the round that is being played is won at once, with the crystals it has.
    bool winRound = false;
};

} // namespace game
