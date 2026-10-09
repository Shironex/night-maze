// Tests of game/GameState: the screens of the game and the rules between them.
#include "game/GameState.hpp"

#include <doctest/doctest.h>

#include <array>
#include <initializer_list>
#include <vector>

namespace {

using game::GameEvent;
using game::GameMode;

// Every screen and every event, for the tests that go through all of them.
constexpr std::array<GameMode, 17> ALL_MODES = {GameMode::MainMenu,
                                                GameMode::Playing,
                                                GameMode::Paused,
                                                GameMode::RoundEnd,
                                                GameMode::Quitting,
                                                GameMode::SettingsFromMenu,
                                                GameMode::SettingsFromPause,
                                                GameMode::Intro,
                                                GameMode::FreePlay,
                                                GameMode::Nights,
                                                GameMode::NewCampaign,
                                                GameMode::NightCard,
                                                GameMode::EndingCard,
                                                GameMode::CampaignIntro,
                                                GameMode::LedgerFromMenu,
                                                GameMode::LedgerFromPause,
                                                GameMode::VillageBeat};
constexpr std::array<GameEvent, 20> ALL_EVENTS = {
    GameEvent::Play,          GameEvent::Resume,       GameEvent::Restart,
    GameEvent::BackToMenu,    GameEvent::Quit,         GameEvent::Escape,
    GameEvent::RoundWon,      GameEvent::OpenSettings, GameEvent::CloseSettings,
    GameEvent::NewMaze,       GameEvent::FocusLost,    GameEvent::IntroFinished,
    GameEvent::OpenFreePlay,  GameEvent::OpenNights,   GameEvent::AskNewCampaign,
    GameEvent::StartNight,    GameEvent::CardFinished, GameEvent::CampaignWon,
    GameEvent::BeginCampaign, GameEvent::OpenLedger};

// The three screens that are opened from the main menu and lead back to it.
constexpr std::array<GameMode, 3> MENU_PAGES = {GameMode::FreePlay, GameMode::Nights,
                                                GameMode::NewCampaign};

} // namespace

TEST_CASE("the main menu opens its screens and leaves the program with Quit") {
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::OpenFreePlay) == GameMode::FreePlay);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::OpenNights) == GameMode::Nights);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::AskNewCampaign) == GameMode::NewCampaign);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Quit) == GameMode::Quitting);
    // The button that starts a game of free play is on the screen of free play.
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Play) == GameMode::MainMenu);
}

TEST_CASE("in the main menu Escape and the buttons of other screens do nothing") {
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Escape) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Resume) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Restart) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::BackToMenu) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::RoundWon) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::NewMaze) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::CloseSettings) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::FocusLost) == GameMode::MainMenu);
}

TEST_CASE("the settings open from the main menu and from the pause menu, and go back there") {
    const GameMode fromMenu = game::nextMode(GameMode::MainMenu, GameEvent::OpenSettings);
    CHECK(fromMenu == GameMode::SettingsFromMenu);
    CHECK(game::nextMode(fromMenu, GameEvent::CloseSettings) == GameMode::MainMenu);
    CHECK(game::nextMode(fromMenu, GameEvent::Escape) == GameMode::MainMenu);

    const GameMode fromPause = game::nextMode(GameMode::Paused, GameEvent::OpenSettings);
    CHECK(fromPause == GameMode::SettingsFromPause);
    CHECK(game::nextMode(fromPause, GameEvent::CloseSettings) == GameMode::Paused);
    CHECK(game::nextMode(fromPause, GameEvent::Escape) == GameMode::Paused);

    // Nowhere else: not from a running round and not from the result screen.
    CHECK(game::nextMode(GameMode::Playing, GameEvent::OpenSettings) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::OpenSettings) == GameMode::RoundEnd);
}

TEST_CASE("the ledger opens from the main menu and from the pause menu, and goes back there") {
    const GameMode fromMenu = game::nextMode(GameMode::MainMenu, GameEvent::OpenLedger);
    CHECK(fromMenu == GameMode::LedgerFromMenu);
    CHECK(game::nextMode(fromMenu, GameEvent::CloseSettings) == GameMode::MainMenu);
    CHECK(game::nextMode(fromMenu, GameEvent::Escape) == GameMode::MainMenu);

    const GameMode fromPause = game::nextMode(GameMode::Paused, GameEvent::OpenLedger);
    CHECK(fromPause == GameMode::LedgerFromPause);
    CHECK(game::nextMode(fromPause, GameEvent::CloseSettings) == GameMode::Paused);
    CHECK(game::nextMode(fromPause, GameEvent::Escape) == GameMode::Paused);

    // Nowhere else, and on the page only Back and Escape do something.
    for (const GameMode mode : ALL_MODES) {
        if (mode != GameMode::MainMenu && mode != GameMode::Paused) {
            CHECK(game::nextMode(mode, GameEvent::OpenLedger) == mode);
        }
    }
    for (const GameMode ledger : {fromMenu, fromPause}) {
        for (const GameEvent event : ALL_EVENTS) {
            if (event != GameEvent::CloseSettings && event != GameEvent::Escape) {
                CHECK(game::nextMode(ledger, event) == ledger);
            }
        }
    }
    GameEvent event = GameEvent::Escape;
    CHECK(game::eventForAction("ledger", event));
    CHECK(event == GameEvent::OpenLedger);
}

TEST_CASE("the ledger is a menu that keeps the picture of the screen it was opened from") {
    const std::array<std::array<GameMode, 2>, 2> pairs = {
        {{GameMode::MainMenu, GameMode::LedgerFromMenu},
         {GameMode::Paused, GameMode::LedgerFromPause}}};
    for (const std::array<GameMode, 2>& pair : pairs) {
        CHECK(game::isMenuOpen(pair[1]));
        CHECK_FALSE(game::updatesRound(pair[1]));
        CHECK_FALSE(game::showsHud(pair[1]));
        CHECK_FALSE(game::isFilm(pair[1]));
        CHECK(game::usesMenuCamera(pair[0]) == game::usesMenuCamera(pair[1]));
        CHECK(game::animatesScene(pair[0]) == game::animatesScene(pair[1]));
        CHECK(game::drawsScene(pair[0], true) == game::drawsScene(pair[1], true));
    }
}

TEST_CASE("on the settings screen only Back and Escape do something") {
    for (const GameMode settings : {GameMode::SettingsFromMenu, GameMode::SettingsFromPause}) {
        for (const GameEvent event : ALL_EVENTS) {
            if (event == GameEvent::CloseSettings || event == GameEvent::Escape) {
                continue;
            }
            CHECK(game::nextMode(settings, event) == settings);
        }
    }
}

TEST_CASE("losing the focus pauses a running round and changes no other screen") {
    CHECK(game::nextMode(GameMode::Playing, GameEvent::FocusLost) == GameMode::Paused);
    for (const GameMode mode : ALL_MODES) {
        if (mode != GameMode::Playing) {
            CHECK(game::nextMode(mode, GameEvent::FocusLost) == mode);
        }
    }
}

TEST_CASE("Escape pauses the game and Escape again resumes it") {
    const GameMode paused = game::nextMode(GameMode::Playing, GameEvent::Escape);
    CHECK(paused == GameMode::Paused);
    CHECK(game::nextMode(paused, GameEvent::Escape) == GameMode::Playing);
}

TEST_CASE("the pause menu resumes, restarts or goes back to the main menu") {
    CHECK(game::nextMode(GameMode::Paused, GameEvent::Resume) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::Paused, GameEvent::Restart) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::Paused, GameEvent::BackToMenu) == GameMode::MainMenu);
    // Leaving the program is a button of the main menu only.
    CHECK(game::nextMode(GameMode::Paused, GameEvent::Quit) == GameMode::Paused);
    CHECK(game::nextMode(GameMode::Paused, GameEvent::Play) == GameMode::Paused);
}

TEST_CASE("a won round goes to the result screen, and only while playing") {
    CHECK(game::nextMode(GameMode::Playing, GameEvent::RoundWon) == GameMode::RoundEnd);
    CHECK(game::nextMode(GameMode::Paused, GameEvent::RoundWon) == GameMode::Paused);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::RoundWon) == GameMode::RoundEnd);
}

TEST_CASE("while playing the menu buttons do nothing") {
    CHECK(game::nextMode(GameMode::Playing, GameEvent::Play) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::Playing, GameEvent::Resume) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::Playing, GameEvent::Restart) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::Playing, GameEvent::BackToMenu) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::Playing, GameEvent::Quit) == GameMode::Playing);
}

TEST_CASE("the result screen restarts or goes back to the main menu, also with Escape") {
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Restart) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::NewMaze) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::BackToMenu) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Escape) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Resume) == GameMode::RoundEnd);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Quit) == GameMode::RoundEnd);
}

TEST_CASE("the result screen of a night leads to the next title card or to the list of nights") {
    // "Next night": the title card, and from there into the round.
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::StartNight) == GameMode::NightCard);
    CHECK(game::nextMode(GameMode::NightCard, GameEvent::CardFinished) == GameMode::Playing);
    // "Back to nights".
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::OpenNights) == GameMode::Nights);
    // Neither starts a round on the maze in play, and the intro is not played.
    CHECK_FALSE(game::startsRound(GameMode::RoundEnd, GameEvent::StartNight));
    CHECK_FALSE(game::startsNewGame(GameMode::RoundEnd, GameEvent::StartNight));
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::BeginCampaign) == GameMode::RoundEnd);
    // The pause menu has no such way out.
    CHECK(game::nextMode(GameMode::Paused, GameEvent::StartNight) == GameMode::Paused);
    CHECK(game::nextMode(GameMode::Paused, GameEvent::OpenNights) == GameMode::Paused);
}

TEST_CASE("nothing leaves the quitting state") {
    for (const GameEvent event : ALL_EVENTS) {
        CHECK(game::nextMode(GameMode::Quitting, event) == GameMode::Quitting);
    }
}

TEST_CASE("only the main menu can reach the quitting state") {
    for (const GameMode mode : ALL_MODES) {
        for (const GameEvent event : ALL_EVENTS) {
            const bool quits = game::nextMode(mode, event) == GameMode::Quitting;
            const bool expected = mode == GameMode::Quitting ||
                                  (mode == GameMode::MainMenu && event == GameEvent::Quit);
            CHECK(quits == expected);
        }
    }
}

TEST_CASE("a round starts from the beginning on the same maze with Restart") {
    CHECK(game::startsRound(GameMode::Paused, GameEvent::Restart));
    CHECK(game::startsRound(GameMode::RoundEnd, GameEvent::Restart));

    // Resuming goes on with the round that was paused.
    CHECK_FALSE(game::startsRound(GameMode::Paused, GameEvent::Resume));
    CHECK_FALSE(game::startsRound(GameMode::Paused, GameEvent::Escape));
    // An event that does not change the screen starts nothing.
    CHECK_FALSE(game::startsRound(GameMode::Playing, GameEvent::Play));
    CHECK_FALSE(game::startsRound(GameMode::Playing, GameEvent::Restart));
    CHECK_FALSE(game::startsRound(GameMode::MainMenu, GameEvent::Restart));
    CHECK_FALSE(game::startsRound(GameMode::Paused, GameEvent::Play));
    // A new game is more than a new round: it has a function of its own.
    CHECK_FALSE(game::startsRound(GameMode::MainMenu, GameEvent::Play));
    CHECK_FALSE(game::startsRound(GameMode::RoundEnd, GameEvent::NewMaze));
}

TEST_CASE("a new game starts with Play in free play and with New maze after a round") {
    CHECK(game::startsNewGame(GameMode::FreePlay, GameEvent::Play));
    CHECK(game::startsNewGame(GameMode::RoundEnd, GameEvent::NewMaze));
    CHECK_FALSE(game::startsNewGame(GameMode::MainMenu, GameEvent::Play));

    // The same maze again is not a new game.
    CHECK_FALSE(game::startsNewGame(GameMode::RoundEnd, GameEvent::Restart));
    CHECK_FALSE(game::startsNewGame(GameMode::Paused, GameEvent::Restart));
    // An event that does not change the screen starts nothing.
    CHECK_FALSE(game::startsNewGame(GameMode::Playing, GameEvent::Play));
    CHECK_FALSE(game::startsNewGame(GameMode::Paused, GameEvent::NewMaze));
    CHECK_FALSE(game::startsNewGame(GameMode::MainMenu, GameEvent::NewMaze));
}

TEST_CASE("every event that starts a round or a game also leads into the game") {
    for (const GameMode mode : ALL_MODES) {
        for (const GameEvent event : ALL_EVENTS) {
            if (game::startsRound(mode, event) || game::startsNewGame(mode, event)) {
                CHECK(game::nextMode(mode, event) == GameMode::Playing);
            }
            // Never both: a new game starts its own round.
            CHECK_FALSE((game::startsRound(mode, event) && game::startsNewGame(mode, event)));
        }
    }
}

TEST_CASE("the names of the menu buttons give their events") {
    GameEvent event = GameEvent::Escape;
    CHECK(game::eventForAction("play", event));
    CHECK(event == GameEvent::Play);
    CHECK(game::eventForAction("resume", event));
    CHECK(event == GameEvent::Resume);
    CHECK(game::eventForAction("restart", event));
    CHECK(event == GameEvent::Restart);
    CHECK(game::eventForAction("menu", event));
    CHECK(event == GameEvent::BackToMenu);
    CHECK(game::eventForAction("quit", event));
    CHECK(event == GameEvent::Quit);
    CHECK(game::eventForAction("settings", event));
    CHECK(event == GameEvent::OpenSettings);
    CHECK(game::eventForAction("back", event));
    CHECK(event == GameEvent::CloseSettings);
    CHECK(game::eventForAction("new-maze", event));
    CHECK(event == GameEvent::NewMaze);
    CHECK(game::eventForAction("free-play", event));
    CHECK(event == GameEvent::OpenFreePlay);
    CHECK(game::eventForAction("nights", event));
    CHECK(event == GameEvent::OpenNights);
}

TEST_CASE("an unknown button name gives no event") {
    GameEvent event = GameEvent::Resume;
    CHECK_FALSE(game::eventForAction("", event));
    CHECK_FALSE(game::eventForAction("Play", event));
    CHECK_FALSE(game::eventForAction("escape", event));
    CHECK_FALSE(game::eventForAction("difficulty-easy", event));
    // The event is left as it was.
    CHECK(event == GameEvent::Resume);
}

TEST_CASE("the round runs only while playing") {
    CHECK(game::updatesRound(GameMode::Playing));
    CHECK_FALSE(game::updatesRound(GameMode::MainMenu));
    CHECK_FALSE(game::updatesRound(GameMode::Paused));
    CHECK_FALSE(game::updatesRound(GameMode::RoundEnd));
    CHECK_FALSE(game::updatesRound(GameMode::Quitting));
    CHECK_FALSE(game::updatesRound(GameMode::SettingsFromMenu));
    CHECK_FALSE(game::updatesRound(GameMode::SettingsFromPause));
}

TEST_CASE("the scene stops moving only in the pause menu and in its settings") {
    CHECK_FALSE(game::animatesScene(GameMode::Paused));
    CHECK_FALSE(game::animatesScene(GameMode::SettingsFromPause));
    CHECK(game::animatesScene(GameMode::SettingsFromMenu));
    CHECK(game::animatesScene(GameMode::MainMenu));
    CHECK(game::animatesScene(GameMode::Playing));
    CHECK(game::animatesScene(GameMode::RoundEnd));
}

TEST_CASE("a menu is open on every screen but the game itself") {
    CHECK(game::isMenuOpen(GameMode::MainMenu));
    CHECK(game::isMenuOpen(GameMode::Paused));
    CHECK(game::isMenuOpen(GameMode::RoundEnd));
    CHECK(game::isMenuOpen(GameMode::SettingsFromMenu));
    CHECK(game::isMenuOpen(GameMode::SettingsFromPause));
    CHECK_FALSE(game::isMenuOpen(GameMode::Playing));
    CHECK_FALSE(game::isMenuOpen(GameMode::Quitting));
    for (const GameMode page : MENU_PAGES) {
        CHECK(game::isMenuOpen(page));
    }
}

TEST_CASE("a screen never runs the round under an open menu") {
    for (const GameMode mode : ALL_MODES) {
        CHECK_FALSE((game::isMenuOpen(mode) && game::updatesRound(mode)));
    }
}

TEST_CASE("the HUD belongs to the running game") {
    CHECK(game::showsHud(GameMode::Playing));
    CHECK_FALSE(game::showsHud(GameMode::Paused));
    CHECK_FALSE(game::showsHud(GameMode::MainMenu));
    CHECK_FALSE(game::showsHud(GameMode::RoundEnd));
    CHECK_FALSE(game::showsHud(GameMode::SettingsFromMenu));
    CHECK_FALSE(game::showsHud(GameMode::SettingsFromPause));
}

TEST_CASE("the map is shown while its key is held in a round, and gone when it is released") {
    constexpr game::MapRequest HELD{.keyHeld = true};
    CHECK(game::showsMap(GameMode::Playing, HELD));
    // Released: no map. There is no switch that remembers the key.
    CHECK_FALSE(game::showsMap(GameMode::Playing, game::MapRequest{}));
}

TEST_CASE("only a round that is being played shows the map") {
    // The pause (also the one of a window that lost the focus), the result screen and
    // the menus take the map away, with the key held and with the debug pin.
    constexpr game::MapRequest HELD_AND_PINNED{.keyHeld = true, .pinned = true};
    for (const GameMode mode : ALL_MODES) {
        CHECK(game::showsMap(mode, HELD_AND_PINNED) == (mode == GameMode::Playing));
    }
}

TEST_CASE("the debug pin shows the map without the key") {
    CHECK(game::showsMap(GameMode::Playing, {.pinned = true}));
}

TEST_CASE("an open note card and the menu camera keep the map away") {
    CHECK_FALSE(game::showsMap(GameMode::Playing, {.keyHeld = true, .noteOpen = true}));
    CHECK_FALSE(game::showsMap(GameMode::Playing, {.pinned = true, .noteOpen = true}));
    CHECK_FALSE(game::showsMap(GameMode::Playing, {.keyHeld = true, .menuCamera = true}));
    CHECK_FALSE(game::showsMap(GameMode::Playing, {.pinned = true, .menuCamera = true}));
}

TEST_CASE("only the main menu and the screens opened from it are shown through the menu camera") {
    CHECK(game::usesMenuCamera(GameMode::MainMenu));
    CHECK(game::usesMenuCamera(GameMode::SettingsFromMenu));
    for (const GameMode page : MENU_PAGES) {
        CHECK(game::usesMenuCamera(page));
        // And they keep the picture of the main menu: its video covers the scene.
        CHECK(game::drawsScene(page, true) == game::drawsScene(GameMode::MainMenu, true));
        CHECK(game::animatesScene(page));
    }
    CHECK_FALSE(game::usesMenuCamera(GameMode::Playing));
    CHECK_FALSE(game::usesMenuCamera(GameMode::Paused));
    CHECK_FALSE(game::usesMenuCamera(GameMode::SettingsFromPause));
    CHECK_FALSE(game::usesMenuCamera(GameMode::RoundEnd));
}

TEST_CASE("the settings screen keeps the picture of the screen it was opened from") {
    // Opening and closing the settings must not change what is behind the menu: the
    // same camera, and a scene that moves or stands as before.
    const std::array<std::array<GameMode, 2>, 2> pairs = {
        {{GameMode::MainMenu, GameMode::SettingsFromMenu},
         {GameMode::Paused, GameMode::SettingsFromPause}}};
    for (const std::array<GameMode, 2>& pair : pairs) {
        CHECK(game::usesMenuCamera(pair[0]) == game::usesMenuCamera(pair[1]));
        CHECK(game::animatesScene(pair[0]) == game::animatesScene(pair[1]));
        CHECK(game::drawsScene(pair[0], true) == game::drawsScene(pair[1], true));
    }
}

TEST_CASE("the scene is left out only behind a main menu with a full screen background") {
    CHECK_FALSE(game::drawsScene(GameMode::MainMenu, true));
    CHECK_FALSE(game::drawsScene(GameMode::SettingsFromMenu, true));
    CHECK(game::drawsScene(GameMode::MainMenu, false));
    // The pause menu and the result show the stopped scene behind them, whatever the
    // main menu has.
    CHECK(game::drawsScene(GameMode::Paused, true));
    CHECK(game::drawsScene(GameMode::RoundEnd, true));
    CHECK(game::drawsScene(GameMode::Playing, true));
}

TEST_CASE("a new game asks for the normal difficulty and the default seed") {
    const game::NewGame newGame;
    CHECK(newGame.difficulty == game::Difficulty::Normal);
    CHECK(newGame.seed == game::DEFAULT_MAZE_SEED);
}

TEST_CASE("the intro played by itself ends in the main menu, also skipped and with Escape") {
    CHECK(game::nextMode(GameMode::Intro, GameEvent::IntroFinished) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::Intro, GameEvent::Escape) == GameMode::MainMenu);
}

TEST_CASE("the intro of a campaign leads to the title card, also skipped and with Escape") {
    CHECK(game::nextMode(GameMode::CampaignIntro, GameEvent::IntroFinished) == GameMode::NightCard);
    CHECK(game::nextMode(GameMode::CampaignIntro, GameEvent::Escape) == GameMode::NightCard);
    // The title card then leads into the round: the whole way of "Begin".
    CHECK(game::nextMode(GameMode::NightCard, GameEvent::CardFinished) == GameMode::Playing);
}

TEST_CASE("a campaign begins with the intro, from the main menu and from the question") {
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::BeginCampaign) == GameMode::CampaignIntro);
    CHECK(game::nextMode(GameMode::NewCampaign, GameEvent::BeginCampaign) ==
          GameMode::CampaignIntro);
    // From nowhere else. The list of nights never plays the intro, and neither does
    // a result screen or a pause.
    for (const GameMode mode : ALL_MODES) {
        if (mode != GameMode::MainMenu && mode != GameMode::NewCampaign) {
            CHECK(game::nextMode(mode, GameEvent::BeginCampaign) == mode);
        }
        // No other event leads into it.
        for (const GameEvent event : ALL_EVENTS) {
            if (event != GameEvent::BeginCampaign && mode != GameMode::CampaignIntro) {
                CHECK(game::nextMode(mode, event) != GameMode::CampaignIntro);
            }
        }
    }
}

TEST_CASE("the first entry of the main menu plays the intro only for a campaign not started") {
    using game::CampaignStage;
    // "Begin": the intro, then the first night.
    CHECK(game::campaignEntryEvent(CampaignStage::NotStarted, true) == GameEvent::BeginCampaign);
    // "Continue" never plays it.
    CHECK(game::campaignEntryEvent(CampaignStage::Running, true) == GameEvent::StartNight);
    // "New campaign" asks first. After "yes" the campaign is one that has not been
    // started, so the first row decides.
    CHECK(game::campaignEntryEvent(CampaignStage::Finished, true) == GameEvent::AskNewCampaign);
    CHECK(game::campaignStage(1) == CampaignStage::NotStarted);

    // A run that does not play the intro begins with the title card of the night.
    CHECK(game::campaignEntryEvent(CampaignStage::NotStarted, false) == GameEvent::StartNight);
    CHECK(game::campaignEntryEvent(CampaignStage::Running, false) == GameEvent::StartNight);
    CHECK(game::campaignEntryEvent(CampaignStage::Finished, false) == GameEvent::AskNewCampaign);
}

TEST_CASE("--skip-intro and the switches of a tool keep the intro out of a campaign") {
    CHECK(game::campaignIntroPlays(game::StartOptions{}));

    game::StartOptions skip;
    skip.skipIntro = true;
    CHECK_FALSE(game::campaignIntroPlays(skip));

    game::StartOptions tool;
    tool.toolSwitch = true;
    CHECK_FALSE(game::campaignIntroPlays(tool));

    // --intro plays the intro at the start and changes nothing about the campaign.
    game::StartOptions intro;
    intro.intro = true;
    CHECK(game::campaignIntroPlays(intro));
    intro.skipIntro = true;
    CHECK_FALSE(game::campaignIntroPlays(intro));

    // The whole way, from the words of the command line.
    const auto plays = [](std::initializer_list<const char*> words) {
        const std::vector<const char*> list(words);
        return game::campaignIntroPlays(game::parseStartOptions(list).options);
    };
    CHECK(plays({}));
    CHECK(plays({"--intro"}));
    CHECK_FALSE(plays({"--skip-intro"}));
    CHECK_FALSE(plays({"--seed", "7"}));
    CHECK_FALSE(plays({"--menu-background", "scene"}));
    CHECK_FALSE(plays({"--calm"}));
}

TEST_CASE("nothing else leaves the intro, and losing the focus does not stop it") {
    for (const GameMode intro : {GameMode::Intro, GameMode::CampaignIntro}) {
        for (const GameEvent event : ALL_EVENTS) {
            if (event == GameEvent::IntroFinished || event == GameEvent::Escape) {
                continue;
            }
            CHECK(game::nextMode(intro, event) == intro);
        }
        // Named once more, because it is a decision: the intro goes on behind another
        // program and ends by itself. It is never paused, so it cannot be left stuck.
        CHECK(game::nextMode(intro, GameEvent::FocusLost) == intro);
    }
}

TEST_CASE("no event leads into the intro played by itself, and its end means nothing elsewhere") {
    for (const GameMode mode : ALL_MODES) {
        if (game::isIntro(mode)) {
            continue;
        }
        for (const GameEvent event : ALL_EVENTS) {
            CHECK(game::nextMode(mode, event) != GameMode::Intro);
        }
        CHECK(game::nextMode(mode, GameEvent::IntroFinished) == mode);
    }
}

TEST_CASE("the end of the intro starts no round and no game") {
    for (const GameEvent event : ALL_EVENTS) {
        for (const GameMode intro : {GameMode::Intro, GameMode::CampaignIntro}) {
            CHECK_FALSE(game::startsRound(intro, event));
            CHECK_FALSE(game::startsNewGame(intro, event));
        }
    }
    for (const GameMode mode : ALL_MODES) {
        CHECK_FALSE(game::startsRound(mode, GameEvent::IntroFinished));
        CHECK_FALSE(game::startsNewGame(mode, GameEvent::IntroFinished));
    }
}

TEST_CASE("the intro is a film: no round, no menu, no HUD, no map, a scene that moves") {
    for (const GameMode intro : {GameMode::Intro, GameMode::CampaignIntro}) {
        CHECK(game::isIntro(intro));
        CHECK_FALSE(game::updatesRound(intro));
        CHECK_FALSE(game::isMenuOpen(intro));
        CHECK_FALSE(game::showsHud(intro));
        CHECK_FALSE(game::showsMap(intro, {.keyHeld = true, .pinned = true}));
        CHECK(game::animatesScene(intro));
        // It places the camera itself, and the scene is drawn also when the main menu has
        // a video that covers the window.
        CHECK_FALSE(game::usesMenuCamera(intro));
        CHECK(game::drawsScene(intro, true));
        CHECK(game::drawsScene(intro, false));
    }
    // No other screen plays the intro.
    for (const GameMode mode : ALL_MODES) {
        CHECK(game::isIntro(mode) == (mode == GameMode::Intro || mode == GameMode::CampaignIntro));
    }
}

TEST_CASE("the game opens with the main menu, also on its very first start") {
    // Nothing but the command line decides: the settings file is not asked, so a fresh
    // folder and an old one start alike.
    const game::StartOptions none;
    CHECK(game::startMode(none) == GameMode::MainMenu);
}

TEST_CASE("a run driven by a tool starts in its round or in the main menu") {
    game::StartOptions play;
    play.play = true;
    play.toolSwitch = true;
    CHECK(game::startMode(play) == GameMode::Playing);

    game::StartOptions menuCamera;
    menuCamera.menuCamera.enabled = true;
    menuCamera.toolSwitch = true;
    CHECK(game::startMode(menuCamera) == GameMode::Playing);

    // A night of the campaign named on the command line starts in its round.
    game::StartOptions night;
    night.night = 3;
    night.toolSwitch = true;
    CHECK(game::startMode(night) == GameMode::Playing);

    // So does the maze of a day (--daily).
    game::StartOptions daily;
    daily.daily = 20261009;
    daily.toolSwitch = true;
    CHECK(game::startMode(daily) == GameMode::Playing);

    // --seed, --menu-shot, --menu-time and --menu-background leave the main menu as the
    // first screen.
    game::StartOptions tool;
    tool.toolSwitch = true;
    CHECK(game::startMode(tool) == GameMode::MainMenu);
}

TEST_CASE("--skip-intro never plays the intro and --intro always does") {
    game::StartOptions skip;
    skip.skipIntro = true;
    CHECK(game::startMode(skip) == GameMode::MainMenu);

    game::StartOptions intro;
    intro.intro = true;
    CHECK(game::startMode(intro) == GameMode::Intro);
    // Also next to the switches of a tool, and before --play.
    intro.toolSwitch = true;
    CHECK(game::startMode(intro) == GameMode::Intro);
    intro.play = true;
    intro.menuCamera.enabled = true;
    CHECK(game::startMode(intro) == GameMode::Intro);

    // Both together: "never" wins, and the rest of the line decides.
    intro.skipIntro = true;
    CHECK(game::startMode(intro) == GameMode::Playing);
    game::StartOptions both;
    both.intro = true;
    both.skipIntro = true;
    CHECK(game::startMode(both) == GameMode::MainMenu);
}

TEST_CASE("the switches of the command line give the start screen they describe") {
    // The whole way, from the words to the screen.
    const auto screen = [](std::initializer_list<const char*> words) {
        const std::vector<const char*> list(words);
        return game::startMode(game::parseStartOptions(list).options);
    };
    CHECK(screen({}) == GameMode::MainMenu);
    CHECK(screen({"--seed", "1"}) == GameMode::MainMenu);
    CHECK(screen({"--menu-background", "scene"}) == GameMode::MainMenu);
    CHECK(screen({"--menu-shot", "walk"}) == GameMode::MainMenu);
    CHECK(screen({"--menu-time", "14"}) == GameMode::MainMenu);
    CHECK(screen({"--play"}) == GameMode::Playing);
    CHECK(screen({"--night", "2"}) == GameMode::Playing);
    CHECK(screen({"--menu-camera"}) == GameMode::Playing);
    CHECK(screen({"--skip-intro"}) == GameMode::MainMenu);
    CHECK(screen({"--intro"}) == GameMode::Intro);
    CHECK(screen({"--intro", "--seed", "1", "--menu-background", "scene"}) == GameMode::Intro);
}

TEST_CASE("free play, the list of nights and the question lead back to the main menu") {
    for (const GameMode page : MENU_PAGES) {
        CHECK(game::nextMode(page, GameEvent::BackToMenu) == GameMode::MainMenu);
        CHECK(game::nextMode(page, GameEvent::Escape) == GameMode::MainMenu);
        // Nothing of the round and nothing of another screen happens there.
        CHECK(game::nextMode(page, GameEvent::Resume) == page);
        CHECK(game::nextMode(page, GameEvent::Restart) == page);
        CHECK(game::nextMode(page, GameEvent::RoundWon) == page);
        CHECK(game::nextMode(page, GameEvent::CampaignWon) == page);
        CHECK(game::nextMode(page, GameEvent::Quit) == page);
        CHECK(game::nextMode(page, GameEvent::OpenSettings) == page);
        CHECK(game::nextMode(page, GameEvent::FocusLost) == page);
        CHECK_FALSE(game::updatesRound(page));
        CHECK_FALSE(game::showsHud(page));
        CHECK_FALSE(game::isFilm(page));
    }
}

TEST_CASE("free play starts a game with Play, and only free play does") {
    CHECK(game::nextMode(GameMode::FreePlay, GameEvent::Play) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::Nights, GameEvent::Play) == GameMode::Nights);
    CHECK(game::nextMode(GameMode::NewCampaign, GameEvent::Play) == GameMode::NewCampaign);
    // A night is not started from there.
    CHECK(game::nextMode(GameMode::FreePlay, GameEvent::StartNight) == GameMode::FreePlay);
}

TEST_CASE("a night begins with its title card, from the menu, the list and the question") {
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::StartNight) == GameMode::NightCard);
    CHECK(game::nextMode(GameMode::Nights, GameEvent::StartNight) == GameMode::NightCard);
    CHECK(game::nextMode(GameMode::NewCampaign, GameEvent::StartNight) == GameMode::NightCard);
    // From nowhere else: not out of a round, a pause or a result.
    for (const GameMode mode : ALL_MODES) {
        const bool starts = game::nextMode(mode, GameEvent::StartNight) == GameMode::NightCard;
        const bool expected = mode == GameMode::MainMenu || mode == GameMode::Nights ||
                              mode == GameMode::NewCampaign || mode == GameMode::NightCard ||
                              mode == GameMode::RoundEnd;
        CHECK(starts == expected);
    }
}

TEST_CASE("the title card leads into the round, at its end and when it is skipped") {
    CHECK(game::nextMode(GameMode::NightCard, GameEvent::CardFinished) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::NightCard, GameEvent::Escape) == GameMode::Playing);
    for (const GameEvent event : ALL_EVENTS) {
        if (event == GameEvent::CardFinished || event == GameEvent::Escape) {
            continue;
        }
        CHECK(game::nextMode(GameMode::NightCard, event) == GameMode::NightCard);
        // The round of the night was started when its maze was built: the card starts
        // neither a round nor a game.
        CHECK_FALSE(game::startsRound(GameMode::NightCard, event));
        CHECK_FALSE(game::startsNewGame(GameMode::NightCard, event));
    }
    CHECK_FALSE(game::startsRound(GameMode::NightCard, GameEvent::CardFinished));
    CHECK_FALSE(game::startsNewGame(GameMode::NightCard, GameEvent::CardFinished));
}

TEST_CASE("the last night ends with the village, the ending card and then the main menu") {
    CHECK(game::nextMode(GameMode::Playing, GameEvent::CampaignWon) == GameMode::VillageBeat);
    // The look at the village ends by itself or by a key, and never skips the card.
    CHECK(game::nextMode(GameMode::VillageBeat, GameEvent::CardFinished) == GameMode::EndingCard);
    CHECK(game::nextMode(GameMode::VillageBeat, GameEvent::Escape) == GameMode::EndingCard);
    for (const GameEvent event : ALL_EVENTS) {
        if (event == GameEvent::CardFinished || event == GameEvent::Escape) {
            continue;
        }
        CHECK(game::nextMode(GameMode::VillageBeat, event) == GameMode::VillageBeat);
    }
    // An ordinary night ends with its result screen and not with the village.
    CHECK(game::nextMode(GameMode::Playing, GameEvent::RoundWon) == GameMode::RoundEnd);
    CHECK(game::nextMode(GameMode::EndingCard, GameEvent::CardFinished) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::EndingCard, GameEvent::Escape) == GameMode::MainMenu);
    for (const GameEvent event : ALL_EVENTS) {
        if (event == GameEvent::CardFinished || event == GameEvent::Escape) {
            continue;
        }
        CHECK(game::nextMode(GameMode::EndingCard, event) == GameMode::EndingCard);
    }
    // Only a round that is being played can be won.
    for (const GameMode mode : ALL_MODES) {
        if (mode != GameMode::Playing) {
            CHECK(game::nextMode(mode, GameEvent::CampaignWon) == mode);
        }
    }
    // The end of a card means nothing on a screen that is no card.
    for (const GameMode mode : ALL_MODES) {
        if (mode != GameMode::NightCard && mode != GameMode::EndingCard &&
            mode != GameMode::VillageBeat) {
            CHECK(game::nextMode(mode, GameEvent::CardFinished) == mode);
        }
    }
}

TEST_CASE("the intro and the two cards are films: no round, no menu, no HUD, no map") {
    CHECK(game::isFilm(GameMode::Intro));
    CHECK(game::isFilm(GameMode::CampaignIntro));
    CHECK(game::isFilm(GameMode::NightCard));
    CHECK(game::isFilm(GameMode::EndingCard));
    CHECK(game::isFilm(GameMode::VillageBeat));
    for (const GameMode mode : ALL_MODES) {
        if (!game::isFilm(mode)) {
            continue;
        }
        CHECK_FALSE(game::updatesRound(mode));
        CHECK_FALSE(game::isMenuOpen(mode));
        CHECK_FALSE(game::showsHud(mode));
        CHECK_FALSE(game::showsMap(mode, {.keyHeld = true, .pinned = true}));
        // A lost focus does not stop a film: it runs on and ends by itself.
        CHECK(game::nextMode(mode, GameEvent::FocusLost) == mode);
    }
    CHECK_FALSE(game::isFilm(GameMode::MainMenu));
    CHECK_FALSE(game::isFilm(GameMode::Playing));
    CHECK_FALSE(game::isFilm(GameMode::RoundEnd));
    CHECK_FALSE(game::isFilm(GameMode::Paused));
}

TEST_CASE("the maze of the day goes from the main menu over a title card into its round") {
    // "Tonight's hedge" sends StartNight, like a night: the card, then the round.
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::StartNight) == GameMode::NightCard);
    CHECK(game::nextMode(GameMode::NightCard, GameEvent::CardFinished) == GameMode::Playing);
    // Its result screen is the one of free play: "Play again" starts the same maze
    // without a card, and "Back to menu" and Escape lead to the main menu.
    CHECK(game::nextMode(GameMode::Playing, GameEvent::RoundWon) == GameMode::RoundEnd);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Restart) == GameMode::Playing);
    CHECK(game::startsRound(GameMode::RoundEnd, GameEvent::Restart));
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::BackToMenu) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Escape) == GameMode::MainMenu);
    // The entry is a command of the application and no event of its own.
    GameEvent event = GameEvent::Quit;
    CHECK_FALSE(game::eventForAction("daily", event));
    CHECK(event == GameEvent::Quit);
}
