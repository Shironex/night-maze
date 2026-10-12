// The Night Maze application: the start of a game, of a night and of the maze of the day,
// and the round that was won.
#include "game/NightMazeApp.hpp"

#include "game/Interactables.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <random>

namespace game {

namespace {

// The years of std::tm are counted from this one.
constexpr int TM_FIRST_YEAR = 1900;

// The day of the calendar at this moment, where the player is: the local time of the
// system (game::dailyDate). This is the one place the game asks the clock for a day, and
// a run a tool drives never comes here (game::fixedDailyDate).
std::uint32_t todayLocal() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // Not std::chrono::current_zone: the standard library of Apple clang does not have
    // it yet. The two functions below are the ones that are safe with threads.
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return dailyDate(std::chrono::year{local.tm_year + TM_FIRST_YEAR} /
                     std::chrono::month{static_cast<unsigned>(local.tm_mon + 1)} /
                     std::chrono::day{static_cast<unsigned>(local.tm_mday)});
}

} // namespace

void NightMazeApp::startNewGame(const NewGame& newGame, std::uint32_t daily) {
    // The numbers of the level: the size of the maze and its crystals go into the
    // request for the maze, the gate, the battery and the flasks into the rules of the
    // round. They
    // overwrite what the debug UI may have set in the same fields, so every new game of
    // a level is the same game. The numbers of levers and notes are not part of a
    // level and stay as they are.
    const DifficultyLevel& level = difficultyLevel(newGame.difficulty);
    m_mazeSettings.width = level.mazeWidth;
    m_mazeSettings.height = level.mazeHeight;
    m_mazeSettings.crystalCount = level.crystalCount;
    m_gameplay.requiredFraction = level.requiredFraction;
    m_gameplay.batteryLifetimeSeconds = level.batteryLifetimeSeconds;
    m_gameplay.flaskCount = level.flaskCount;
    // The shade: in the game unless this is a calm night. Like the numbers above it
    // overwrites what the debug UI may have switched. The maze of the day does not ask
    // the switch "Calm night": it is the same maze for every player.
    m_gameplay.shade.enabled = shadeInMaze(daily);

    // The maze is always built again, also for the seed that is in play: the level may
    // be another one, and the numbers of levers and notes may have been changed.
    m_mazeSettings.seed = newGame.seed;
    m_mazeSettings.regenerate = false;
    regenerateMaze(0, daily);
    m_playedDifficultyName = daily == NO_DAILY_DATE ? level.name : DAILY_NAME;

    // The difficulty that was just played is the one free play starts with next time,
    // so it is written to the settings file now.
    saveSettings();
}

void NightMazeApp::startNight(int night, std::uint32_t campaignSeed, bool counts) {
    // The numbers of the night, where startNewGame puts the numbers of a level: they
    // overwrite what the debug UI or a game of free play left there.
    const CampaignNight& level = campaignNight(night);
    m_mazeSettings.width = level.mazeWidth;
    m_mazeSettings.height = level.mazeHeight;
    m_mazeSettings.crystalCount = level.crystalCount;
    m_gameplay.requiredFraction = level.requiredFraction;
    m_gameplay.batteryLifetimeSeconds = level.batteryLifetimeSeconds;
    m_gameplay.flaskCount = level.flaskCount;
    // The shade: the night says whether it has one. The switch "Calm night" of free play
    // is not asked. Only a calm run of the command line (--calm) still has no shade in
    // any maze: that switch is a promise to the tool that gave it.
    m_gameplay.shade.enabled = level.shade && !m_calmRun;

    // The maze of the night: its seed follows from the seed of the campaign, so the
    // night keeps its maze for as long as the campaign lasts.
    m_mazeSettings.seed = campaignNightSeed(campaignSeed, night);
    m_mazeSettings.regenerate = false;
    regenerateMaze(night);
    m_playedDifficultyName = campaignNightName(night);
    m_nightCounts = counts;
}

void NightMazeApp::beginCampaignNight(int night) {
    // A click that arrives on a screen that starts no night (the screen changed in the
    // same frame) builds nothing.
    if (nextMode(m_mode, GameEvent::StartNight) != GameMode::NightCard) {
        return;
    }
    drawCampaignSeed();
    startNight(night, m_settings.campaignSeed, true);
    // The seed is written now: a player who leaves the night and comes back another day
    // finds the same maze. The progress itself is written when a night is won.
    saveSettings();
    handleGameEvent(GameEvent::StartNight);
}

void NightMazeApp::drawCampaignSeed() {
    // The first night that is started draws the seed of the campaign: its five mazes are
    // fixed from here on. std::random_device asks the operating system for a number
    // nobody can predict, like for a seed of free play.
    if (m_settings.campaignSeed == NO_CAMPAIGN_SEED) {
        std::random_device device;
        m_settings.campaignSeed = drawnCampaignSeed(static_cast<std::uint32_t>(device()));
    }
}

void NightMazeApp::enterCampaign() {
    // Without the card the intro cannot be shown: the campaign then begins with its
    // first night, like in a run that skips the intro.
    const bool introPlays = m_campaignIntroPlays && m_cardDocument != ui::NO_DOCUMENT;
    const GameEvent event = campaignEntryEvent(campaignStage(m_settings.campaignNight), introPlays);
    if (event == GameEvent::AskNewCampaign) {
        handleGameEvent(event);
    } else if (event == GameEvent::BeginCampaign) {
        beginCampaignIntro();
    } else {
        beginCampaignNight(nightToOffer(m_settings.campaignNight));
    }
}

void NightMazeApp::beginCampaignIntro() {
    // A click that arrives on a screen that begins no campaign builds nothing.
    if (nextMode(m_mode, GameEvent::BeginCampaign) != GameMode::CampaignIntro) {
        return;
    }
    // The seed is drawn and written now, like when a night is started without the
    // intro: a player who leaves during the intro finds the same mazes another day.
    drawCampaignSeed();
    // The maze of the intro, built like a new game of its level (startIntro). It also
    // writes the settings file. The maze of the first night follows when the intro is
    // left (leaveIntro).
    startNewGame({.difficulty = INTRO_DIFFICULTY, .seed = INTRO_MAZE_SEED});
    m_introSeconds = 0.0;
    m_introCardShown = INTRO_CARD_COUNT;
    handleGameEvent(GameEvent::BeginCampaign);
}

std::uint32_t NightMazeApp::dailyToday() const {
    return m_fixedDailyDate == NO_DAILY_DATE ? todayLocal() : m_fixedDailyDate;
}

void NightMazeApp::beginDaily() {
    // The day is asked once, now: a maze started before midnight stays the maze of that
    // day, and its result belongs to that day (finishRound). A click that arrives on
    // another screen builds nothing, and neither does a clock that names no day.
    const std::uint32_t date = dailyToday();
    if (m_mode != GameMode::MainMenu || date == NO_DAILY_DATE) {
        return;
    }
    startNewGame({.difficulty = DAILY_DIFFICULTY, .seed = date}, date);
    handleGameEvent(GameEvent::StartNight);
}

void NightMazeApp::forgetCampaign() {
    m_settings.campaignNight = 1;
    m_settings.campaignSeed = NO_CAMPAIGN_SEED;
    m_settings.campaignBestSeconds = {};
}

NightEndOffer NightMazeApp::nightEndOffer() const {
    // The campaign of the settings is already the one after the win (finishRound).
    return game::nightEndOffer(m_settings.campaignNight, m_playedNight, m_nightCounts);
}

void NightMazeApp::finishRound() {
    if (m_playedNight == 0) {
        // A maze of free play is finished: the story goes on after the lines this maze
        // showed. The counter is written now, once, and not at every frame (saveSettings
        // writes only when something changed). The counter is moved on from the line THIS
        // maze started with, so winning the same maze twice ("Play again") does not skip
        // a line. The maze keeps its lines until a new one is built.
        // Whether the maze skipped the lines about the shadow is asked from the request
        // it was built with, not from the settings of this moment.
        m_settings.nextStoryLine = advanceStoryLine(m_mazeSettings.interactables.firstStoryLine,
                                                    storyNoteCount(m_mazeWorld.interactables),
                                                    m_mazeSettings.interactables.shadeLines);
        // The maze of the day: its best time, for the result screen and for the settings
        // file. A day that did not come from the clock (a tool run) was not played: its
        // record is read and never written.
        if (m_playedDaily != NO_DAILY_DATE) {
            m_dailyWin = dailyAfterWin(m_settings.daily, m_playedDaily, m_round.elapsedSeconds);
            if (m_fixedDailyDate == NO_DAILY_DATE) {
                m_settings.daily = m_dailyWin.record;
            }
        }
        saveSettings();
        handleGameEvent(GameEvent::RoundWon);
        return;
    }

    // A night of the campaign is won. The story line counter of free play is left alone:
    // a night has its own lines. What is written is the progress: the best time of the
    // night, and the next night when this one was the next. A finished night that is won
    // again can only improve its time. This is the one place the progress is written:
    // being caught, a restart and leaving the night change nothing.
    if (m_nightCounts) {
        int& best = m_settings.campaignBestSeconds.at(static_cast<std::size_t>(m_playedNight - 1));
        best = bestAfterWin(best, m_round.elapsedSeconds);
        m_settings.campaignNight = nightAfterWin(m_settings.campaignNight, m_playedNight);
        saveSettings();
    }
    // The last night ends with the ending card, every other one with the result screen.
    handleGameEvent(m_playedNight == CAMPAIGN_NIGHT_COUNT ? GameEvent::CampaignWon
                                                          : GameEvent::RoundWon);
}

void NightMazeApp::handleCampaignRequest() {
    const CampaignRequest request = m_campaignRequest;
    m_campaignRequest = {};

    // The round that is being played is won as it stands: the fixed step that follows
    // finds it won and does what a walk through the gate does (finishRound).
    if (request.winRound && m_mode == GameMode::Playing) {
        m_round.state = RoundState::Won;
    }
    if (!request.clear && request.setNight == 0) {
        return;
    }
    if (request.clear) {
        forgetCampaign();
    }
    if (request.setNight != 0) {
        m_settings.campaignNight = std::clamp(request.setNight, 1, CAMPAIGN_FINISHED);
        // A night that is not won any more has no best time, like after reading a file.
        for (int night = 1; night <= CAMPAIGN_NIGHT_COUNT; ++night) {
            if (nightStatus(m_settings.campaignNight, night) != NightStatus::Finished) {
                m_settings.campaignBestSeconds.at(static_cast<std::size_t>(night - 1)) =
                    NO_BEST_TIME;
            }
        }
    }
    // A night that is in play belongs to the campaign as it was: winning it now must
    // not write into the one that was just set.
    m_nightCounts = false;
    saveSettings();
    // A menu that shows the campaign shows the new one at once.
    if (isMenuOpen(m_mode)) {
        showScreen();
    }
}

} // namespace game
