// What more than one of the NightMazeApp*.cpp files uses: the names the code shares with
// the documents of the menus, and two small helpers.
#pragma once

#include "game/Campaign.hpp"
#include "game/GameState.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

namespace game {

// The documents of the menus, relative to the assets directory.
constexpr const char* MAIN_MENU_DOCUMENT_FILE = "ui/main_menu.rml";
constexpr const char* PAUSE_DOCUMENT_FILE = "ui/pause.rml";
constexpr const char* ROUND_END_DOCUMENT_FILE = "ui/round_end.rml";
constexpr const char* SETTINGS_DOCUMENT_FILE = "ui/settings.rml";
// The screens the main menu opens: free play, the list of the nights of the campaign and
// the question before a new campaign.
constexpr const char* FREE_PLAY_DOCUMENT_FILE = "ui/free_play.rml";
constexpr const char* NIGHTS_DOCUMENT_FILE = "ui/nights.rml";
constexpr const char* NEW_CAMPAIGN_DOCUMENT_FILE = "ui/new_campaign.rml";
// The lamplighter's ledger, opened from the main menu and from the pause menu.
constexpr const char* LEDGER_DOCUMENT_FILE = "ui/ledger.rml";
// The card of text the intro is told with, and the title card of a night and the ending
// card after it. It has no buttons.
constexpr const char* CARD_DOCUMENT_FILE = "ui/card.rml";

// The elements of the documents the code writes into or reads: their id attributes.
// The result screen and the pause menu use the same four names.
constexpr const char* TIME_ID = "time";
constexpr const char* CRYSTALS_ID = "crystals";
constexpr const char* DIFFICULTY_ID = "difficulty";
constexpr const char* SEED_ID = "seed";
// The main menu: the label of its first entry and the night beside it. Its info block
// has the three numbers named below, like the one of free play.
constexpr const char* CAMPAIGN_LABEL_ID = "campaign-label";
constexpr const char* CAMPAIGN_NIGHT_ID = "campaign-night";
// What stands beside its entry "Tonight's hedge": the day, and the best time of the day.
constexpr const char* DAILY_DATE_ID = "daily-date";
// The list of nights: the id of a row is this prefix and the number of the night
// ("night-2"), and its title and its state are that id with one of the two endings.
constexpr const char* NIGHT_ID_PREFIX = "night-";
constexpr const char* NIGHT_TITLE_ID_SUFFIX = "-title";
constexpr const char* NIGHT_STATE_ID_SUFFIX = "-state";
// The result screen: the line of the story under its title, the name of its third row
// ("Difficulty" or "Night 2"), the two buttons of free play ("Play again", "New maze"),
// the two of the campaign ("Next night", "Back to nights"), the row of the seed and the
// line of keys.
constexpr const char* NIGHT_LINE_ID = "night-line";
constexpr const char* LEVEL_NAME_ID = "level-name";
constexpr const char* PLAY_AGAIN_ID = "play-again";
constexpr const char* NEXT_NIGHT_ID = "next-night";
constexpr const char* BACK_TO_NIGHTS_ID = "back-to-nights";
constexpr const char* NEW_MAZE_ID = "new-maze";
constexpr const char* SEED_ROW_ID = "seed-row";
constexpr const char* KEYS_ID = "keys";
// Free play: the hint next to the seed field, the three numbers of the info block
// and the three difficulty buttons, whose ids are this prefix and the key of a level.
constexpr const char* SEED_HINT_ID = "seed-hint";
constexpr const char* INFO_MAZE_ID = "info-maze";
constexpr const char* INFO_CRYSTALS_ID = "info-crystals";
constexpr const char* INFO_BATTERY_ID = "info-battery";
constexpr const char* DIFFICULTY_ID_PREFIX = "difficulty-";
// The switch "Calm night" of free play.
constexpr const char* CALM_NIGHT_ID = "calm-night";
// The settings screen: the two sliders (their ids are the names of their settings), the
// text next to each control and the row of the window size.
constexpr const char* TEXT_ID_SUFFIX = "-text";
constexpr const char* FULLSCREEN_ID = "fullscreen";
constexpr const char* WINDOW_SIZE_ID = "window-size";
constexpr const char* WINDOW_SIZE_ROW_ID = "window-size-row";
// Its part "Controls": the button of an action has the settings name of the action as
// its id and as its data-action ("key_sprint"), the name of the action stands in the
// element with that id and this suffix, and one line of text stands under the rows.
constexpr const char* KEY_NAME_ID_SUFFIX = "-name";
constexpr const char* CONTROLS_NOTE_ID = "controls-note";
// The card: the black over the picture, the card with its two lines, and the hint.
constexpr const char* CARD_BLACK_ID = "black";
constexpr const char* CARD_ID = "card";
constexpr std::array<const char*, STORY_CARD_LINE_COUNT> CARD_LINE_IDS = {
    "line-first", "line-second", "line-third", "line-fourth"};
constexpr const char* CARD_HINT_ID = "hint";
// The main menu after the intro: the document itself (a name RmlUi knows, see
// ui::UiLayer::setClass) and the black it comes out of.
constexpr const char* DOCUMENT_ID = "#document";
constexpr const char* CURTAIN_ID = "curtain";

// The ledger: its counter, its two pages, the part that scrolls and its entry in the
// main menu and in the pause menu (the same id in both documents).
constexpr const char* LEDGER_COUNT_ID = "ledger-count";
constexpr std::array<const char*, 2> LEDGER_PAGE_IDS = {"ledger-page-1", "ledger-page-2"};
constexpr const char* LEDGER_SCROLL_ID = "ledger-scroll";
constexpr const char* LEDGER_ENTRY_ID = "open-ledger";
// How many groups (nights) stand on its first page: three with five lines each. The
// second page has the other two, and room for a group that is added later.
constexpr std::size_t LEDGER_FIRST_PAGE_GROUPS = 3;
// How far its keys scroll, in heights of the scrolling part (ui::UiLayer::scrollBy): an
// arrow key per second it is held, Page Up and Page Down per press (a little less than
// a page, so the eye keeps a line), Home and End to the end.
constexpr float LEDGER_ARROW_PAGES_PER_SECOND = 1.2F;
constexpr float LEDGER_PAGE_KEY_PAGES = 0.85F;
constexpr float LEDGER_END_PAGES = 1000.0F;

// The classes the code sets on elements. The style sheet says what they look like.
constexpr const char* CHOSEN_CLASS = "chosen";
constexpr const char* ON_CLASS = "on";
constexpr const char* OFF_CLASS = "off";
// The button of the action that waits for a key.
constexpr const char* WAITING_CLASS = "waiting";
// The main menu after the intro: no fade of the whole document, and the curtain is
// there.
constexpr const char* CUT_CLASS = "cut";
constexpr const char* DRAWN_CLASS = "drawn";
// A row of the list of nights carries one of these three.
constexpr const char* FINISHED_CLASS = "finished";
constexpr const char* NEXT_CLASS = "open";
constexpr const char* LOCKED_CLASS = "locked";
// An element that is not shown at the moment, and a card of text on black.
constexpr const char* GONE_CLASS = "gone";
constexpr const char* MIDDLE_CLASS = "middle";
// The body of the result screen, and the class it carries after a night of the campaign.
constexpr const char* ROUND_END_BODY_ID = "round-end";
constexpr const char* REVEAL_CLASS = "reveal";

// The buttons that do not change the screen: their data-action names. A difficulty
// button is named by the same prefix as its id.
constexpr const char* NEW_SEED_ACTION = "new-seed";
constexpr const char* TOGGLE_FULLSCREEN_ACTION = "toggle-fullscreen";
constexpr const char* TOGGLE_CALM_NIGHT_ACTION = "toggle-calm-night";
constexpr const char* WINDOW_SIZE_SMALLER_ACTION = "window-size-smaller";
constexpr const char* WINDOW_SIZE_LARGER_ACTION = "window-size-larger";
constexpr const char* RESET_SETTINGS_ACTION = "reset-settings";
constexpr const char* RESET_CONTROLS_ACTION = "reset-controls";
// The button that starts a game: its seed is read from the seed field first.
constexpr const char* PLAY_ACTION = "play";
// The first entry of the main menu, and the answer "yes" to a new campaign. A row of the
// list of nights is named like its id: NIGHT_ID_PREFIX and its number.
constexpr const char* CAMPAIGN_ACTION = "campaign";
constexpr const char* NEW_CAMPAIGN_ACTION = "new-campaign";
// The button "Next night" of the result screen.
constexpr const char* NEXT_NIGHT_ACTION = "next-night";
// The entry "Tonight's hedge" of the main menu: the maze of the day.
constexpr const char* DAILY_ACTION = "daily";

// A random seed of the menu is below this number: at most six digits, short enough to
// read out to a friend. A typed seed can be any number a seed can be.
constexpr std::uint32_t RANDOM_SEED_LIMIT = 1000000;

// A seed nobody can predict, from 1 to RANDOM_SEED_LIMIT - 1. std::random_device asks
// the operating system for a random number. It only picks the seed: the maze itself is
// built by the seeded generator, so the same seed always brings the same maze back.
inline std::uint32_t randomSeed() {
    std::random_device device;
    return 1 + static_cast<std::uint32_t>(device()) % (RANDOM_SEED_LIMIT - 1);
}

// True for the two screens that show the ledger.
inline bool isLedger(GameMode mode) {
    return mode == GameMode::LedgerFromMenu || mode == GameMode::LedgerFromPause;
}

} // namespace game
