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
constexpr std::string_view FREE_PLAY_ACTION = "free-play";
constexpr std::string_view NIGHTS_ACTION = "nights";

} // namespace

GameMode nextMode(GameMode mode, GameEvent event) {
    // One block per screen: the events that mean something there. Everything else
    // falls through to the last line and changes nothing.
    switch (mode) {
    case GameMode::MainMenu:
        if (event == GameEvent::BeginCampaign) {
            return GameMode::CampaignIntro;
        }
        if (event == GameEvent::StartNight) {
            return GameMode::NightCard;
        }
        if (event == GameEvent::AskNewCampaign) {
            return GameMode::NewCampaign;
        }
        if (event == GameEvent::OpenNights) {
            return GameMode::Nights;
        }
        if (event == GameEvent::OpenFreePlay) {
            return GameMode::FreePlay;
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
        // The last night of the campaign: its ending card in the place of the result.
        if (event == GameEvent::CampaignWon) {
            return GameMode::EndingCard;
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
        // After a night of the campaign: on to the next one, or to the list of nights.
        if (event == GameEvent::StartNight) {
            return GameMode::NightCard;
        }
        if (event == GameEvent::OpenNights) {
            return GameMode::Nights;
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
    case GameMode::Intro:
        // To the main menu, at its end or when it is skipped. FocusLost is not named
        // here on purpose: the intro goes on behind another program and ends by itself.
        if (event == GameEvent::IntroFinished || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::FreePlay:
        if (event == GameEvent::Play) {
            return GameMode::Playing;
        }
        if (event == GameEvent::BackToMenu || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::NewCampaign:
        // "Yes" to a new campaign: the intro comes before its first night.
        if (event == GameEvent::BeginCampaign) {
            return GameMode::CampaignIntro;
        }
        [[fallthrough]];
    case GameMode::Nights:
        // A night of the list, or a new campaign without the intro: a title card.
        if (event == GameEvent::StartNight) {
            return GameMode::NightCard;
        }
        if (event == GameEvent::BackToMenu || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::NightCard:
        // Into the round, at its end or when it is skipped. Escape skips like any key:
        // there is nothing to go back to that the player would expect.
        if (event == GameEvent::CardFinished || event == GameEvent::Escape) {
            return GameMode::Playing;
        }
        break;
    case GameMode::EndingCard:
        if (event == GameEvent::CardFinished || event == GameEvent::Escape) {
            return GameMode::MainMenu;
        }
        break;
    case GameMode::CampaignIntro:
        // On to the title card of the first night, at its end or when it is skipped.
        // FocusLost is not named, for the same reason as in the intro above.
        if (event == GameEvent::IntroFinished || event == GameEvent::Escape) {
            return GameMode::NightCard;
        }
        break;
    }
    return mode;
}

GameMode startMode(const StartOptions& options) {
    if (options.intro && !options.skipIntro) {
        return GameMode::Intro;
    }
    // The menu camera is a tool for recording the game: with it the main menu is
    // skipped like with --play, so no menu ever lies over the recorded picture.
    // A night of the campaign named on the command line starts in its round at once, and
    // so does the maze of a day.
    if (options.play || options.menuCamera.enabled || options.night != 0 ||
        options.daily != NO_DAILY_DATE) {
        return GameMode::Playing;
    }
    return GameMode::MainMenu;
}

bool campaignIntroPlays(const StartOptions& options) {
    return !options.skipIntro && !options.toolSwitch;
}

GameEvent campaignEntryEvent(CampaignStage stage, bool introPlays) {
    if (stage == CampaignStage::Finished) {
        return GameEvent::AskNewCampaign;
    }
    if (stage == CampaignStage::NotStarted && introPlays) {
        return GameEvent::BeginCampaign;
    }
    return GameEvent::StartNight;
}

bool startsRound(GameMode mode, GameEvent event) {
    if (event == GameEvent::Restart) {
        return mode == GameMode::Paused || mode == GameMode::RoundEnd;
    }
    return false;
}

bool startsNewGame(GameMode mode, GameEvent event) {
    if (event == GameEvent::Play) {
        return mode == GameMode::FreePlay;
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
    } else if (action == FREE_PLAY_ACTION) {
        event = GameEvent::OpenFreePlay;
    } else if (action == NIGHTS_ACTION) {
        event = GameEvent::OpenNights;
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
           mode == GameMode::SettingsFromMenu || mode == GameMode::SettingsFromPause ||
           mode == GameMode::FreePlay || mode == GameMode::Nights || mode == GameMode::NewCampaign;
}

bool isIntro(GameMode mode) {
    return mode == GameMode::Intro || mode == GameMode::CampaignIntro;
}

bool isFilm(GameMode mode) {
    return isIntro(mode) || mode == GameMode::NightCard || mode == GameMode::EndingCard;
}

bool showsHud(GameMode mode) {
    return mode == GameMode::Playing;
}

bool showsMap(GameMode mode, const MapRequest& request) {
    return mode == GameMode::Playing && (request.keyHeld || request.pinned) && !request.noteOpen &&
           !request.menuCamera;
}

bool usesMenuCamera(GameMode mode) {
    return mode == GameMode::MainMenu || mode == GameMode::SettingsFromMenu ||
           mode == GameMode::FreePlay || mode == GameMode::Nights || mode == GameMode::NewCampaign;
}

bool drawsScene(GameMode mode, bool fullscreenBackground) {
    return !(usesMenuCamera(mode) && fullscreenBackground);
}

} // namespace game
