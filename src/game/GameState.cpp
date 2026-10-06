// GameState: which screen the game is on (menu, playing, paused, round end, settings)
// and the rules for going from one to the next.
#include "game/GameState.hpp"

namespace game {

namespace {

// The names of the menu buttons, as the documents in assets/ui write them.
constexpr std::string_view PLAY_ACTION = "play";
constexpr std::string_view RESUME_ACTION = "resume";
constexpr std::string_view RESTART_ACTION = "restart";
constexpr std::string_view BACK_TO_MENU_ACTION = "menu";
constexpr std::string_view QUIT_ACTION = "quit";
constexpr std::string_view SETTINGS_ACTION = "settings";
constexpr std::string_view BACK_ACTION = "back";
constexpr std::string_view NEW_MAZE_ACTION = "new-maze";

} // namespace

GameMode nextMode(GameMode mode, GameEvent event) {
    // One block per screen: the events that mean something there. Everything else
    // falls through to the last line and changes nothing.
    switch (mode) {
    case GameMode::MainMenu:
        if (event == GameEvent::Play) {
            return GameMode::Playing;
        }
        if (event == GameEvent::Quit) {
            return GameMode::Quitting;
        }
        if (event == GameEvent::OpenSettings) {
            return GameMode::SettingsFromMenu;
        }
        break;
    case GameMode::Playing:
        // A player who switches to another program does not want the round to go on
        // without them: losing the focus pauses like the Escape key.
        if (event == GameEvent::Escape || event == GameEvent::FocusLost) {
            return GameMode::Paused;
        }
        if (event == GameEvent::RoundWon) {
            return GameMode::RoundEnd;
        }
        break;
    case GameMode::Paused:
        if (event == GameEvent::Escape || event == GameEvent::Resume ||
            event == GameEvent::Restart) {
            return GameMode::Playing;
        }
        if (event == GameEvent::BackToMenu) {
            return GameMode::MainMenu;
        }
        if (event == GameEvent::OpenSettings) {
            return GameMode::SettingsFromPause;
        }
        break;
    case GameMode::RoundEnd:
        if (event == GameEvent::Restart || event == GameEvent::NewMaze) {
            return GameMode::Playing;
        }
        if (event == GameEvent::BackToMenu || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::SettingsFromMenu:
        if (event == GameEvent::CloseSettings || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::SettingsFromPause:
        if (event == GameEvent::CloseSettings || event == GameEvent::Escape) {
            return GameMode::Paused;
        }
        break;
    case GameMode::Quitting:
        // The program is closing: nothing brings it back.
        break;
    }
    return mode;
}

bool startsRound(GameMode mode, GameEvent event) {
    if (event == GameEvent::Restart) {
        return mode == GameMode::Paused || mode == GameMode::RoundEnd;
    }
    return false;
}

bool startsNewGame(GameMode mode, GameEvent event) {
    if (event == GameEvent::Play) {
        return mode == GameMode::MainMenu;
    }
    if (event == GameEvent::NewMaze) {
        return mode == GameMode::RoundEnd;
    }
    return false;
}

bool eventForAction(std::string_view action, GameEvent& event) {
    if (action == PLAY_ACTION) {
        event = GameEvent::Play;
    } else if (action == RESUME_ACTION) {
        event = GameEvent::Resume;
    } else if (action == RESTART_ACTION) {
        event = GameEvent::Restart;
    } else if (action == BACK_TO_MENU_ACTION) {
        event = GameEvent::BackToMenu;
    } else if (action == QUIT_ACTION) {
        event = GameEvent::Quit;
    } else if (action == SETTINGS_ACTION) {
        event = GameEvent::OpenSettings;
    } else if (action == BACK_ACTION) {
        event = GameEvent::CloseSettings;
    } else if (action == NEW_MAZE_ACTION) {
        event = GameEvent::NewMaze;
    } else {
        return false;
    }
    return true;
}

bool updatesRound(GameMode mode) {
    return mode == GameMode::Playing;
}

bool animatesScene(GameMode mode) {
    return mode != GameMode::Paused && mode != GameMode::SettingsFromPause;
}

bool isMenuOpen(GameMode mode) {
    return mode == GameMode::MainMenu || mode == GameMode::Paused || mode == GameMode::RoundEnd ||
           mode == GameMode::SettingsFromMenu || mode == GameMode::SettingsFromPause;
}

bool showsHud(GameMode mode) {
    return mode == GameMode::Playing;
}

bool showsMinimap(GameMode mode) {
    return mode == GameMode::Playing || mode == GameMode::Paused ||
           mode == GameMode::SettingsFromPause;
}

bool usesMenuCamera(GameMode mode) {
    return mode == GameMode::MainMenu || mode == GameMode::SettingsFromMenu;
}

bool drawsScene(GameMode mode, bool fullscreenBackground) {
    return !(usesMenuCamera(mode) && fullscreenBackground);
}

} // namespace game
