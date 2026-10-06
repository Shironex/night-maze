// GameState: which screen the game is on (menu, playing, paused, round end) and the
// rules for going from one to the next.
#pragma once

#include "game/MazeWorld.hpp"

#include <cstdint>
#include <string_view>

namespace game {

// Plain data and pure functions without a window, without OpenGL and without the menu
// library, like the rest of the game_logic library, so tests can check every rule. The
// application (NightMazeApp) holds one GameMode, feeds it events and asks the questions
// below instead of deciding by itself what a screen allows.

/// The screen the game is on. Exactly one at a time.
enum class GameMode {
    MainMenu = 0, ///< the main menu, the first thing after the start
    Playing,      ///< a round is being played
    Paused,       ///< a round is stopped, the pause menu is shown over it
    RoundEnd,     ///< the round is won, its result is shown
    Quitting,     ///< the player asked to leave: the program closes its window
};

/// Something that can change the screen: a button of a menu, the Escape key, or the
/// round itself.
enum class GameEvent {
    Play = 0,   ///< button "Play" of the main menu: start a new game
    Resume,     ///< button "Resume" of the pause menu
    Restart,    ///< button "Restart": the same maze again, from the start
    BackToMenu, ///< button "Back to menu"
    Quit,       ///< button "Quit" of the main menu
    Escape,     ///< the Escape key
    RoundWon,   ///< the player walked through the open gate (RoundState::Won)
};

/// How hard a new game is. The numbers behind the three levels (size of the maze,
/// crystals, battery) are not chosen yet: until they are, every level plays the same.
enum class Difficulty {
    Easy = 0,
    Normal,
    Hard,
};

/// What the button "Play" asks for: the game that is started next.
struct NewGame {
    Difficulty difficulty = Difficulty::Normal;

    /// The seed of the maze (MazeSettings::seed).
    std::uint32_t seed = DEFAULT_MAZE_SEED;
};

/// The screen after an event. An event that means nothing on a screen leaves the
/// screen as it is, so a caller can send any event at any time.
///
///     screen     event        next screen
///     MainMenu   Play         Playing
///     MainMenu   Quit         Quitting
///     Playing    Escape       Paused
///     Playing    RoundWon     RoundEnd
///     Paused     Escape       Playing
///     Paused     Resume       Playing
///     Paused     Restart      Playing
///     Paused     BackToMenu   MainMenu
///     RoundEnd   Restart      Playing
///     RoundEnd   BackToMenu   MainMenu
///     RoundEnd   Escape       MainMenu
///
/// Escape goes one screen back: out of the game into the pause menu, out of the pause
/// menu back into the game, out of the result to the main menu. In the main menu there
/// is nothing to go back to, and leaving the program is a button, not a key.
GameMode nextMode(GameMode mode, GameEvent event);

/// True when the event, sent on this screen, starts a round from the beginning: Play in
/// the main menu, and Restart in the pause menu or on the result screen.
bool startsRound(GameMode mode, GameEvent event);

/// The event behind the name a menu button carries in its document (the attribute
/// data-action): "play", "resume", "restart", "menu" or "quit". False for any other
/// name: event is left as it was. The Escape key and the won round are not buttons and
/// have no name.
bool eventForAction(std::string_view action, GameEvent& event);

/// True while the rules of the round run: the player moves, the battery drains,
/// crystals are collected and the time counts. Only while playing.
bool updatesRound(GameMode mode);

/// True while the things that move by themselves keep moving (bobbing crystals, pulsing
/// lights). They stand still only in the pause menu: a pause stops everything.
bool animatesScene(GameMode mode);

/// True while a menu document is shown. The cursor is then free and the mouse and the
/// keys of the round do not reach the game.
bool isMenuOpen(GameMode mode);

/// True while the HUD of the round is drawn (counter, battery, crosshair, cards): only
/// while playing. It is drawn after the menu documents, so in the pause menu it would
/// lie on top of the buttons.
bool showsHud(GameMode mode);

/// True while the minimap is drawn: while playing, and in the pause menu, where it is
/// part of the stopped picture under the menu. It is drawn before the menu documents.
bool showsMinimap(GameMode mode);

/// True while the picture is taken by the menu camera and not from the eyes of the
/// player: in the main menu, which has no round to show.
bool usesMenuCamera(GameMode mode);

/// True when the scene has to be drawn. fullscreenBackground tells whether the menu
/// has a picture of its own that covers the whole window (a video). The main menu with
/// such a background hides the scene completely, so its passes can be left out. Every
/// other screen shows the scene, also behind a menu.
bool drawsScene(GameMode mode, bool fullscreenBackground);

} // namespace game
