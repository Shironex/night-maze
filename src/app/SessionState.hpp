// What is in play (the game of free play, the night or the day) and what the command line
// said about the run.
#pragma once

#include "game/maze/MazeWorld.hpp"
#include "game/round/Campaign.hpp"
#include "game/round/Daily.hpp"
#include "game/screens/GameState.hpp"

#include <cstdint>
#include <string>

namespace game {

/// Plain data of the session. The constructor of the application fills the fields of the
/// run from game::StartOptions; the functions of NightMazeAppCampaign.cpp and
/// NightMazeAppRound.cpp set the fields of the game in play whenever a maze is built.
struct SessionState {
    /// The game the button "Play" starts: the difficulty chosen on the free play screen
    /// and the seed its seed field shows.
    NewGame newGame;
    /// The night of the campaign that is in play, 1 to CAMPAIGN_NIGHT_COUNT, or 0 for
    /// a maze of free play and for one the debug UI asked for. Set whenever a maze is
    /// built (regenerateMaze).
    int playedNight = 0;
    /// True when winning the night in play changes the campaign of the settings file.
    /// False for a night started from the command line (--night), which has a campaign
    /// seed of its own, and after the debug UI changed the campaign under a running night.
    bool nightCounts = false;
    /// The day of the maze of the day that is in play (game/round/Daily.hpp), or NO_DAILY_DATE
    /// for every other maze. Set whenever a maze is built (regenerateMaze).
    std::uint32_t playedDaily = NO_DAILY_DATE;
    /// The last win of a maze of the day: the record after it and whether it was a new
    /// best, for the result screen (finishRound).
    DailyWin dailyWin;
    /// What the debug UI asks of the campaign (handleCampaignRequest).
    CampaignRequest campaignRequest;
    /// The seed of the command line (StartOptions::seed): the maze behind the main menu
    /// is built from it again when the intro, which has a maze of its own, is over.
    std::uint32_t startSeed = DEFAULT_MAZE_SEED;
    /// StartOptions::toolSwitch: a run driven by a tool shows the village dark unless it
    /// names its night (game::villageNightsLit).
    bool toolRun = false;
    /// True when a campaign that is begun in this run plays the intro first
    /// (game::campaignIntroPlays of the command line).
    bool campaignIntroPlays = true;
    /// The day of "Tonight's hedge" in a run that must not ask the clock
    /// (game::fixedDailyDate), or NO_DAILY_DATE. Such a run never writes a best time.
    std::uint32_t fixedDailyDate = NO_DAILY_DATE;
    /// True when a note that is read in this run goes into the ledger of the settings
    /// file. False in a run with a tool switch (StartOptions::toolSwitch): its notes are
    /// read by scripts.
    bool ledgerCounts = true;
    /// The name of the difficulty of the game in play, for the pause menu and the
    /// result screen: the name of a level, or "Custom" for a maze the debug UI asked for.
    std::string playedDifficultyName;
};

} // namespace game
