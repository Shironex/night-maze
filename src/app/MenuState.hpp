// The menus of the application: RmlUi with one document per screen, and the wait for a key
// on the settings screen.
#pragma once

#include "core/Window.hpp"
#include "game/player/KeyBindings.hpp"
#include "ui/UiLayer.hpp"

#include <optional>
#include <string>

namespace game {

/// What the menus keep between frames. The documents are loaded by the constructor of the
/// application and filled by the functions of NightMazeAppMenus.cpp.
struct MenuState {
    /// window is the window RmlUi draws into and reads its events from.
    explicit MenuState(const core::Window& window) : ui(window) {}

    /// RmlUi and its documents, one per screen with a menu. Each DocumentId is
    /// ui::NO_DOCUMENT when its file could not be loaded.
    ui::UiLayer ui;
    ui::DocumentId mainMenuDocument = ui::NO_DOCUMENT;
    ui::DocumentId freePlayDocument = ui::NO_DOCUMENT;
    ui::DocumentId nightsDocument = ui::NO_DOCUMENT;
    ui::DocumentId newCampaignDocument = ui::NO_DOCUMENT;
    ui::DocumentId ledgerDocument = ui::NO_DOCUMENT;
    ui::DocumentId pauseDocument = ui::NO_DOCUMENT;
    ui::DocumentId roundEndDocument = ui::NO_DOCUMENT;
    ui::DocumentId settingsDocument = ui::NO_DOCUMENT;
    /// True when all of these documents are loaded. Without them a menu screen would show
    /// nothing and could not be left, so the game then never enters one.
    bool menusLoaded = false;

    /// The card of text of the intro, of the title card of a night and of the ending card:
    /// one more document, without buttons, not one of the menus.
    ui::DocumentId cardDocument = ui::NO_DOCUMENT;

    /// True while the main menu is the one that followed the intro or the ending card: it
    /// then comes in out of black (fillMainMenuDocument).
    bool menuAfterIntro = false;

    /// The action whose row of the settings screen waits for a key, or nothing
    /// (handleKeyCapture). While it is set the UI layer captures the keyboard.
    std::optional<KeyAction> keyCaptureAction;
    /// The line under the rows of the part "Controls": what the last change did (a swap,
    /// a key that was refused). Empty: the line says the rule of the swap.
    std::string controlsNote;
};

} // namespace game
