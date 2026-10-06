// Tests of game/GameState: the screens of the game and the rules between them.
#include "game/GameState.hpp"

#include <doctest/doctest.h>

#include <array>

namespace {

using game::GameEvent;
using game::GameMode;

// Every screen and every event, for the tests that go through all of them.
constexpr std::array<GameMode, 5> ALL_MODES = {GameMode::MainMenu, GameMode::Playing,
                                               GameMode::Paused, GameMode::RoundEnd,
                                               GameMode::Quitting};
constexpr std::array<GameEvent, 7> ALL_EVENTS = {
    GameEvent::Play, GameEvent::Resume, GameEvent::Restart, GameEvent::BackToMenu,
    GameEvent::Quit, GameEvent::Escape, GameEvent::RoundWon};

} // namespace

TEST_CASE("the main menu starts a game with Play and leaves the program with Quit") {
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Play) == GameMode::Playing);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Quit) == GameMode::Quitting);
}

TEST_CASE("in the main menu Escape and the buttons of other screens do nothing") {
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Escape) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Resume) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::Restart) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::BackToMenu) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::MainMenu, GameEvent::RoundWon) == GameMode::MainMenu);
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
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::BackToMenu) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Escape) == GameMode::MainMenu);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Resume) == GameMode::RoundEnd);
    CHECK(game::nextMode(GameMode::RoundEnd, GameEvent::Quit) == GameMode::RoundEnd);
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

TEST_CASE("a round starts from the beginning with Play in the menu and with Restart") {
    CHECK(game::startsRound(GameMode::MainMenu, GameEvent::Play));
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
}

TEST_CASE("every event that starts a round also leads into the game") {
    for (const GameMode mode : ALL_MODES) {
        for (const GameEvent event : ALL_EVENTS) {
            if (game::startsRound(mode, event)) {
                CHECK(game::nextMode(mode, event) == GameMode::Playing);
            }
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
}

TEST_CASE("an unknown button name gives no event") {
    GameEvent event = GameEvent::Resume;
    CHECK_FALSE(game::eventForAction("", event));
    CHECK_FALSE(game::eventForAction("Play", event));
    CHECK_FALSE(game::eventForAction("escape", event));
    CHECK_FALSE(game::eventForAction("settings", event));
    // The event is left as it was.
    CHECK(event == GameEvent::Resume);
}

TEST_CASE("the round runs only while playing") {
    CHECK(game::updatesRound(GameMode::Playing));
    CHECK_FALSE(game::updatesRound(GameMode::MainMenu));
    CHECK_FALSE(game::updatesRound(GameMode::Paused));
    CHECK_FALSE(game::updatesRound(GameMode::RoundEnd));
    CHECK_FALSE(game::updatesRound(GameMode::Quitting));
}

TEST_CASE("the scene stops moving only in the pause menu") {
    CHECK_FALSE(game::animatesScene(GameMode::Paused));
    CHECK(game::animatesScene(GameMode::MainMenu));
    CHECK(game::animatesScene(GameMode::Playing));
    CHECK(game::animatesScene(GameMode::RoundEnd));
}

TEST_CASE("a menu is open on every screen but the game itself") {
    CHECK(game::isMenuOpen(GameMode::MainMenu));
    CHECK(game::isMenuOpen(GameMode::Paused));
    CHECK(game::isMenuOpen(GameMode::RoundEnd));
    CHECK_FALSE(game::isMenuOpen(GameMode::Playing));
    CHECK_FALSE(game::isMenuOpen(GameMode::Quitting));
}

TEST_CASE("a screen never runs the round under an open menu") {
    for (const GameMode mode : ALL_MODES) {
        CHECK_FALSE((game::isMenuOpen(mode) && game::updatesRound(mode)));
    }
}

TEST_CASE("the HUD belongs to the running game, the minimap also to the pause") {
    CHECK(game::showsHud(GameMode::Playing));
    CHECK_FALSE(game::showsHud(GameMode::Paused));
    CHECK_FALSE(game::showsHud(GameMode::MainMenu));
    CHECK_FALSE(game::showsHud(GameMode::RoundEnd));

    CHECK(game::showsMinimap(GameMode::Playing));
    CHECK(game::showsMinimap(GameMode::Paused));
    CHECK_FALSE(game::showsMinimap(GameMode::MainMenu));
    CHECK_FALSE(game::showsMinimap(GameMode::RoundEnd));
}

TEST_CASE("only the main menu is shown through the menu camera") {
    CHECK(game::usesMenuCamera(GameMode::MainMenu));
    CHECK_FALSE(game::usesMenuCamera(GameMode::Playing));
    CHECK_FALSE(game::usesMenuCamera(GameMode::Paused));
    CHECK_FALSE(game::usesMenuCamera(GameMode::RoundEnd));
}

TEST_CASE("the scene is left out only behind a main menu with a full screen background") {
    CHECK_FALSE(game::drawsScene(GameMode::MainMenu, true));
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
