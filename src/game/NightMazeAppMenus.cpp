// The Night Maze application: the documents of the menus, their buttons and their controls.
#include "game/NightMazeApp.hpp"

#include "core/Log.hpp"
#include "game/Interactables.hpp"
#include "game/Ledger.hpp"
#include "game/NightMazeAppShared.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace game {

namespace {

// What the first entry of the main menu reads (game::campaignStage).
constexpr const char* BEGIN_LABEL = "Begin";
constexpr const char* CONTINUE_LABEL = "Continue";
constexpr const char* NEW_CAMPAIGN_LABEL = "New campaign";
// The state of a row of the list of nights. A finished night shows its best time after
// BEST_TIME_PREFIX, or NIGHT_FINISHED_TEXT when it has none (the debug UI can move the
// campaign on without a win).
constexpr const char* NIGHT_NEXT_TEXT = "Next";
constexpr const char* NIGHT_LOCKED_TEXT = "Locked";
constexpr const char* NIGHT_FINISHED_TEXT = "Finished";
constexpr const char* BEST_TIME_PREFIX = "Best ";
// The result screen: the name of its third row in free play, and its line of keys in
// free play and after a night, for each of the two offers (game::nightEndOffer).
constexpr const char* DIFFICULTY_ROW_NAME = "Difficulty";
constexpr const char* ROUND_END_KEYS_TEXT =
    "Play again: the same maze | New maze: another seed | Esc: back to menu";
constexpr const char* NEXT_NIGHT_KEYS_TEXT = "Enter: the next night | Esc: back to menu";
constexpr const char* BACK_TO_NIGHTS_KEYS_TEXT = "Enter: the list of nights | Esc: back to menu";
// The same two after the maze of the day: its third row names the day, and it has no
// other maze to offer.
constexpr const char* DATE_ROW_NAME = "Date";
constexpr const char* DAILY_KEYS_TEXT = "Play again: the same maze | Esc: back to menu";

// While the volume slider of the settings screen is moved, a short click lets the
// player hear the new loudness: at most one in this many seconds. A dragged slider
// reports a new value in almost every frame, and a click per frame would be a buzz.
constexpr double VOLUME_SAMPLE_SECONDS = 0.2;

// The part "Controls" of the settings screen: what the button of an action says while
// it waits for a key, and the line under the rows while there is nothing else to say.
constexpr const char* PRESS_A_KEY_TEXT = "Press a key";
constexpr const char* CONTROLS_RULE_TEXT = "A key already in use swaps the two.";
constexpr const char* CONTROLS_RESET_TEXT = "The default keys are back.";

// What the hint next to the seed field says when the field cannot be read.
constexpr const char* SEED_HINT_TEXT = "Digits only, up to 4294967295";

} // namespace

void NightMazeApp::handleMenuActions() {
    for (const std::string& action : m_ui.takeActions()) {
        // A button was clicked while a row of the controls waited for a key: the wait
        // is over, and the button does what it always does.
        stopKeyCapture();
        GameEvent event = GameEvent::Escape;
        if (eventForAction(action, event)) {
            // Play in free play starts the seed of the seed field. A field that cannot
            // be read keeps the screen open.
            if (action == PLAY_ACTION && m_mode == GameMode::FreePlay && !readSeedField()) {
                continue;
            }
            handleGameEvent(event);
        } else if (!handleMenuCommand(action)) {
            core::logWarn("A menu button has an unknown action: " + action);
        }
    }
}

bool NightMazeApp::handleMenuCommand(const std::string& action) {
    // The first entry of the main menu: the intro and the first night of a campaign
    // that has not been started, the next night of one that runs, or the question about
    // a new campaign once the last night is won.
    if (action == CAMPAIGN_ACTION) {
        enterCampaign();
        return true;
    }
    // "Tonight's hedge": the maze of the day.
    if (action == DAILY_ACTION) {
        beginDaily();
        return true;
    }
    // "Yes" to a new campaign: the finished one is forgotten, and the new one begins
    // like any campaign that has not been started, with mazes of its own.
    if (action == NEW_CAMPAIGN_ACTION) {
        if (m_mode == GameMode::NewCampaign) {
            forgetCampaign();
            enterCampaign();
        }
        return true;
    }
    // "Next night" on the result screen of a night: the title card of the night after
    // the one that was just won. The button is only shown when that night is the next
    // one of the campaign (fillRoundEndDocument), and the same rule is asked again here.
    if (action == NEXT_NIGHT_ACTION) {
        if (m_mode == GameMode::RoundEnd && nightEndOffer() == NightEndOffer::NextNight) {
            beginCampaignNight(m_playedNight + 1);
        }
        return true;
    }
    // A row of the list of nights: "night-" and the number of the night, one digit. A
    // night that is still locked does nothing.
    const std::string nightPrefix = NIGHT_ID_PREFIX;
    if (action.starts_with(nightPrefix)) {
        const std::string number = action.substr(nightPrefix.size());
        if (number.size() != 1 || number.front() < '1' ||
            number.front() > '0' + CAMPAIGN_NIGHT_COUNT) {
            return false;
        }
        const int night = number.front() - '0';
        if (nightStatus(m_settings.campaignNight, night) != NightStatus::Locked) {
            beginCampaignNight(night);
        }
        return true;
    }

    // The three difficulty buttons of free play: "difficulty-" and the key of
    // a level. The choice is a setting too, so the next start of the game shows it.
    const std::string difficultyPrefix = DIFFICULTY_ID_PREFIX;
    if (action.starts_with(difficultyPrefix)) {
        if (!difficultyFromKey(action.substr(difficultyPrefix.size()), m_newGame.difficulty)) {
            return false;
        }
        m_settings.difficulty = m_newGame.difficulty;
        fillFreePlayDocument();
        return true;
    }
    if (action == NEW_SEED_ACTION) {
        rollSeed();
        return true;
    }
    // The switch "Calm night" of free play. It is a setting like the difficulty: the
    // next game of free play uses it, and it is written to the file when that game starts.
    // The campaign does not ask it: its nights say themselves whether they have a shade.
    if (action == TOGGLE_CALM_NIGHT_ACTION) {
        m_settings.calmNight = !m_settings.calmNight;
        fillFreePlayDocument();
        return true;
    }

    // The buttons of the settings screen. Each one changes m_settings, uses the new
    // value at once and shows it. The file is written when the screen is left.
    if (action == TOGGLE_FULLSCREEN_ACTION) {
        m_settings.fullscreen = !m_settings.fullscreen;
        applyWindowSettings();
        fillSettingsDocument();
        return true;
    }
    if (action == WINDOW_SIZE_SMALLER_ACTION || action == WINDOW_SIZE_LARGER_ACTION) {
        // While the game is fullscreen the size of the window means nothing: the row
        // is shown dimmed and its buttons do nothing.
        if (!m_settings.fullscreen) {
            const core::Size desktop = window().desktopSize();
            const std::vector<WindowSize> choices = windowSizeChoices(
                {.width = desktop.width, .height = desktop.height}, m_settings.windowSize);
            const int step = action == WINDOW_SIZE_LARGER_ACTION ? 1 : -1;
            m_settings.windowSize = steppedWindowSize(choices, m_settings.windowSize, step);
            applyWindowSettings();
            fillSettingsDocument();
        }
        return true;
    }
    // The part "Controls". A row: its action waits for the next key press
    // (handleKeyCapture). From now on the UI layer keeps every key from the menu, and
    // main.cpp blocks the keyboard for the game, because the layer says it wants it.
    if (KeyAction keyAction = KeyAction::Forward; keyActionFromSetting(action, keyAction)) {
        m_keyCaptureAction = keyAction;
        m_controlsNote.clear();
        m_ui.setKeyCapture(true);
        fillControls();
        return true;
    }
    if (action == RESET_CONTROLS_ACTION) {
        m_settings.keys = defaultKeyBindings();
        m_controlsNote = CONTROLS_RESET_TEXT;
        fillControls();
        return true;
    }
    if (action == RESET_SETTINGS_ACTION) {
        // Everything this screen shows goes back to its default, the keys too. What is
        // chosen or earned somewhere else stays (game::resetSettings).
        m_settings = resetSettings(m_settings);
        applyViewSettings();
        applyWindowSettings();
        applyAudioSettings();
        m_controlsNote.clear();
        fillSettingsDocument();
        return true;
    }
    return false;
}

void NightMazeApp::handleKeyCapture() {
    if (!m_keyCaptureAction.has_value()) {
        return;
    }
    const int key = m_ui.takeCapturedKey();
    if (key == ui::NO_CAPTURED_KEY) {
        return;
    }
    // Escape: the action keeps its key. The game does not see this Escape (its keyboard
    // is blocked while the layer captures), so the screen stays.
    if (key == GLFW_KEY_ESCAPE) {
        m_controlsNote.clear();
        stopKeyCapture();
        return;
    }
    const KeyAction action = *m_keyCaptureAction;
    const BindResult result = bindKey(m_settings.keys, action, key);
    if (!result.accepted) {
        // A fixed key or a key without a name: the row goes on waiting.
        m_controlsNote = keyRefusedLine(key);
        fillControls();
        return;
    }
    // The file is written when the screen is left, like every other setting.
    m_controlsNote = result.swappedWith.has_value()
                         ? keySwappedLine(m_settings.keys, *result.swappedWith)
                         : std::string();
    stopKeyCapture();
}

void NightMazeApp::stopKeyCapture() {
    if (!m_keyCaptureAction.has_value()) {
        return;
    }
    m_keyCaptureAction.reset();
    m_ui.setKeyCapture(false);
    fillControls();
}

void NightMazeApp::fillControls() {
    for (const KeyActionInfo& info : keyActions()) {
        const std::string id(info.settingName);
        const bool waiting = m_keyCaptureAction == info.action;
        m_ui.setText(m_settingsDocument, id + KEY_NAME_ID_SUFFIX, std::string(info.label));
        m_ui.setText(m_settingsDocument, id,
                     waiting ? PRESS_A_KEY_TEXT : boundKeyName(m_settings.keys, info.action));
        m_ui.setClass(m_settingsDocument, id, WAITING_CLASS, waiting);
    }
    m_ui.setText(m_settingsDocument, CONTROLS_NOTE_ID,
                 m_controlsNote.empty() ? CONTROLS_RULE_TEXT : m_controlsNote);
}

void NightMazeApp::handleControlChanges() {
    const std::vector<ui::ControlChange> changes = m_ui.takeChanges();
    // The wait of the sample click runs down with the real time of the frames. This
    // function is called once per frame.
    m_volumeSampleWait = std::max(m_volumeSampleWait - time().deltaSeconds(), 0.0);
    // Only the settings screen has controls that report: a change that arrives on
    // another screen is a late echo of fillSettingsDocument and is dropped.
    if (m_mode != GameMode::SettingsFromMenu && m_mode != GameMode::SettingsFromPause) {
        return;
    }
    for (const ui::ControlChange& change : changes) {
        GameSettings changed = m_settings;
        // A slider also reports the value the code has just given it. Then nothing is
        // different and nothing is done.
        if (!applySetting(changed, change.name, change.value) || changed == m_settings) {
            continue;
        }
        // Asked before the new settings are taken over: was it the volume that moved?
        // The master volume and the effects volume are both heard through an effect.
        const bool volumeChanged = changed.masterVolume != m_settings.masterVolume ||
                                   changed.effectsVolume != m_settings.effectsVolume;
        const bool ambientChanged = changed.ambientVolume != m_settings.ambientVolume;
        m_settings = changed;
        applyViewSettings();
        applyAudioSettings();
        // The new loudness is heard at once, as the click of the flashlight. The UI
        // layer reports every value of a dragged slider and not its release, so the
        // click is held back to a few per second.
        if (volumeChanged && m_volumeSampleWait <= 0.0) {
            playCue(SoundCue::FlashlightOn);
            m_volumeSampleWait = VOLUME_SAMPLE_SECONDS;
        }
        // The ambient volume is heard as what it changes: the wind of the maze comes up
        // on this screen and stays for a moment after the last change (updateAmbience).
        if (ambientChanged) {
            m_windSampleLeft = MAZE_WIND_SAMPLE_SECONDS;
        }
        // Only the numbers next to the sliders: the sliders themselves already stand
        // where the player put them.
        m_ui.setText(m_settingsDocument, std::string(MOUSE_SENSITIVITY_SETTING) + TEXT_ID_SUFFIX,
                     mouseSensitivityLabel(m_settings.mouseSensitivity));
        m_ui.setText(m_settingsDocument, std::string(FIELD_OF_VIEW_SETTING) + TEXT_ID_SUFFIX,
                     fieldOfViewLabel(m_settings.fieldOfViewDegrees));
        m_ui.setText(m_settingsDocument, std::string(MASTER_VOLUME_SETTING) + TEXT_ID_SUFFIX,
                     masterVolumeLabel(m_settings.masterVolume));
        m_ui.setText(m_settingsDocument, std::string(EFFECTS_VOLUME_SETTING) + TEXT_ID_SUFFIX,
                     masterVolumeLabel(m_settings.effectsVolume));
        m_ui.setText(m_settingsDocument, std::string(AMBIENT_VOLUME_SETTING) + TEXT_ID_SUFFIX,
                     masterVolumeLabel(m_settings.ambientVolume));
        // The music volume needs no sample: on the settings screen of the main menu the
        // theme itself is playing, and its loudness follows the slider at once.
        m_ui.setText(m_settingsDocument, std::string(MUSIC_VOLUME_SETTING) + TEXT_ID_SUFFIX,
                     masterVolumeLabel(m_settings.musicVolume));
    }
}

bool NightMazeApp::readSeedField() {
    const std::string text = m_ui.value(m_freePlayDocument, SEED_ID);
    if (text.empty()) {
        // Nothing typed: any maze will do.
        rollSeed();
        return true;
    }
    if (!parseSeed(text, m_newGame.seed)) {
        m_ui.setText(m_freePlayDocument, SEED_HINT_ID, SEED_HINT_TEXT);
        return false;
    }
    m_ui.setText(m_freePlayDocument, SEED_HINT_ID, "");
    return true;
}

void NightMazeApp::rollSeed() {
    m_newGame.seed = randomSeed();
    m_ui.setValue(m_freePlayDocument, SEED_ID, std::to_string(m_newGame.seed));
    m_ui.setText(m_freePlayDocument, SEED_HINT_ID, "");
}

void NightMazeApp::fillMainMenuDocument() {
    // The first entry: what it reads follows from how far the campaign is, and while
    // the campaign runs the next night stands beside it.
    const CampaignStage stage = campaignStage(m_settings.campaignNight);
    const char* label = BEGIN_LABEL;
    if (stage == CampaignStage::Running) {
        label = CONTINUE_LABEL;
    } else if (stage == CampaignStage::Finished) {
        label = NEW_CAMPAIGN_LABEL;
    }
    m_ui.setText(m_mainMenuDocument, CAMPAIGN_LABEL_ID, label);
    m_ui.setText(m_mainMenuDocument, CAMPAIGN_NIGHT_ID,
                 stage == CampaignStage::Running ? campaignNightName(m_settings.campaignNight)
                                                 : std::string());

    // "Tonight's hedge": the day beside it, and the best time once the day has one.
    m_ui.setText(m_mainMenuDocument, DAILY_DATE_ID, dailyMenuLine(m_settings.daily, dailyToday()));

    // The info block: the numbers of the night the first entry starts.
    const CampaignNight& level = campaignNight(nightToOffer(m_settings.campaignNight));
    m_ui.setText(m_mainMenuDocument, INFO_MAZE_ID,
                 std::to_string(level.mazeWidth) + " x " + std::to_string(level.mazeHeight));
    m_ui.setText(m_mainMenuDocument, INFO_CRYSTALS_ID,
                 std::to_string(requiredCrystalCount(level.crystalCount, level.requiredFraction)) +
                     " of " + std::to_string(level.crystalCount));
    m_ui.setText(m_mainMenuDocument, INFO_BATTERY_ID, timeText(level.batteryLifetimeSeconds));

    // The menu that follows the intro or the ending card comes in out of black. Every
    // other time the two classes are taken away, and the menu fades in like every screen.
    m_ui.setClass(m_mainMenuDocument, DOCUMENT_ID, CUT_CLASS, m_menuAfterIntro);
    m_ui.setClass(m_mainMenuDocument, CURTAIN_ID, DRAWN_CLASS, m_menuAfterIntro);
}

void NightMazeApp::fillFreePlayDocument() {
    // The chosen one of the three difficulty buttons carries a class.
    for (const Difficulty difficulty : ALL_DIFFICULTIES) {
        m_ui.setClass(m_freePlayDocument,
                      std::string(DIFFICULTY_ID_PREFIX) + difficultyLevel(difficulty).key,
                      CHOSEN_CLASS, difficulty == m_newGame.difficulty);
    }

    // The switch "Calm night": a class moves its knob.
    m_ui.setClass(m_freePlayDocument, CALM_NIGHT_ID, ON_CLASS, m_settings.calmNight);

    // The info block: what the chosen level means in numbers.
    const DifficultyLevel& level = difficultyLevel(m_newGame.difficulty);
    m_ui.setText(m_freePlayDocument, INFO_MAZE_ID,
                 std::to_string(level.mazeWidth) + " x " + std::to_string(level.mazeHeight));
    m_ui.setText(m_freePlayDocument, INFO_CRYSTALS_ID,
                 std::to_string(requiredCrystalCount(level.crystalCount, level.requiredFraction)) +
                     " of " + std::to_string(level.crystalCount));
    m_ui.setText(m_freePlayDocument, INFO_BATTERY_ID, timeText(level.batteryLifetimeSeconds));
}

void NightMazeApp::fillNightsDocument() {
    for (int night = 1; night <= CAMPAIGN_NIGHT_COUNT; ++night) {
        const std::string id = std::string(NIGHT_ID_PREFIX) + std::to_string(night);
        m_ui.setText(m_nightsDocument, id + NIGHT_TITLE_ID_SUFFIX, campaignNight(night).title);

        // What the row says and looks like: a finished night shows its best time, the
        // next one is marked, a later one is locked.
        const NightStatus status = nightStatus(m_settings.campaignNight, night);
        std::string state = NIGHT_LOCKED_TEXT;
        if (status == NightStatus::Open) {
            state = NIGHT_NEXT_TEXT;
        } else if (status == NightStatus::Finished) {
            const int best = m_settings.campaignBestSeconds.at(static_cast<std::size_t>(night - 1));
            state = best == NO_BEST_TIME ? std::string(NIGHT_FINISHED_TEXT)
                                         : BEST_TIME_PREFIX + timeText(static_cast<float>(best));
        }
        m_ui.setText(m_nightsDocument, id + NIGHT_STATE_ID_SUFFIX, state);
        m_ui.setClass(m_nightsDocument, id, FINISHED_CLASS, status == NightStatus::Finished);
        m_ui.setClass(m_nightsDocument, id, NEXT_CLASS, status == NightStatus::Open);
        m_ui.setClass(m_nightsDocument, id, LOCKED_CLASS, status == NightStatus::Locked);
    }
}

void NightMazeApp::fillLedgerDocument() {
    m_ui.setText(m_ledgerDocument, LEDGER_COUNT_ID, ledgerCounterText(m_settings.storyRead));
    // The two pages, written as RML: a heading per night, then its lines. A line that
    // was read is its text, any other a ruled blank in one of three widths. The texts
    // hold no character that RML would read as a tag (tests/LedgerTests.cpp).
    std::array<std::string, LEDGER_PAGE_IDS.size()> pages;
    const std::vector<LedgerGroup> groups = ledgerGroups(m_settings.storyRead);
    for (std::size_t i = 0; i < groups.size(); ++i) {
        std::string& page = pages.at(i < LEDGER_FIRST_PAGE_GROUPS ? 0 : 1);
        page += R"(<p class="ledger-night">)" + groups[i].title + "</p>";
        for (const LedgerLine& line : groups[i].lines) {
            if (line.read) {
                page += R"(<p class="ledger-line">)" + std::string(flavourLine(line.line)) + "</p>";
            } else {
                page += R"(<div class="ledger-blank w)" + std::to_string(line.blankWidth) +
                        R"("><div class="rule"></div></div>)";
            }
        }
    }
    for (std::size_t i = 0; i < pages.size(); ++i) {
        m_ui.setText(m_ledgerDocument, LEDGER_PAGE_IDS.at(i), pages.at(i));
    }
    // The page opens at its top.
    m_ui.scrollBy(m_ledgerDocument, LEDGER_SCROLL_ID, -LEDGER_END_PAGES);
}

void NightMazeApp::scrollLedger() {
    // The arrows scroll for as long as they are held, the other keys once per press
    // (wasKeyPressed is true for one frame, so they are read here, once per frame).
    float pages = 0.0F;
    const auto held = static_cast<float>(time().deltaSeconds()) * LEDGER_ARROW_PAGES_PER_SECOND;
    pages += input().isKeyDown(GLFW_KEY_DOWN) ? held : 0.0F;
    pages -= input().isKeyDown(GLFW_KEY_UP) ? held : 0.0F;
    pages += input().wasKeyPressed(GLFW_KEY_PAGE_DOWN) ? LEDGER_PAGE_KEY_PAGES : 0.0F;
    pages -= input().wasKeyPressed(GLFW_KEY_PAGE_UP) ? LEDGER_PAGE_KEY_PAGES : 0.0F;
    pages += input().wasKeyPressed(GLFW_KEY_END) ? LEDGER_END_PAGES : 0.0F;
    pages -= input().wasKeyPressed(GLFW_KEY_HOME) ? LEDGER_END_PAGES : 0.0F;
    if (pages != 0.0F) {
        m_ui.scrollBy(m_ledgerDocument, LEDGER_SCROLL_ID, pages);
    }
}

void NightMazeApp::markOpenNoteRead() {
    // A run driven by a tool writes no progress, like its night and its maze of the day.
    if (!m_ledgerCounts || !m_round.noteOpen ||
        m_round.noteIndex >= m_mazeWorld.interactables.notes.size()) {
        return;
    }
    const Note& note = m_mazeWorld.interactables.notes[m_round.noteIndex];
    if (note.kind != NoteKind::Flavour) {
        return;
    }
    // Written at once: a line that was read stays read, also when the round is left
    // without a win. saveSettings writes only when the set has changed.
    m_settings.storyRead = withLineRead(m_settings.storyRead, note.flavourIndex);
    saveSettings();
}

void NightMazeApp::fillPauseDocument() {
    m_ui.setText(m_pauseDocument, DIFFICULTY_ID, m_playedDifficultyName);
    m_ui.setText(m_pauseDocument, SEED_ID, std::to_string(m_mazeWorld.seed));
}

void NightMazeApp::fillRoundEndDocument() {
    m_ui.setText(m_roundEndDocument, TIME_ID, timeText(m_round.elapsedSeconds));
    m_ui.setText(m_roundEndDocument, CRYSTALS_ID,
                 std::to_string(m_round.collectedCount) + " of " +
                     std::to_string(crystalTotal(m_round)));
    // The difficulty and the seed together name the maze: with both, a friend plays
    // the same one. After a night of the campaign the row names the night ("Night 2",
    // "The Shepherds' Gates"), and the line of the story of that night stands under the
    // title. In free play that line is empty and takes no room.
    // After the maze of the day the line says "New best" or names the time to beat, and
    // the row names the day.
    const bool night = m_playedNight != 0;
    const bool daily = m_playedDaily != NO_DAILY_DATE;
    std::string line;
    std::string levelName = DIFFICULTY_ROW_NAME;
    std::string level = m_playedDifficultyName;
    if (night) {
        line = campaignNight(m_playedNight).endLine;
        levelName = campaignNightLabel(m_playedNight);
        level = campaignNight(m_playedNight).title;
    } else if (daily) {
        line = dailyResultLine(m_dailyWin);
        levelName = DATE_ROW_NAME;
        level = dailyDateText(m_playedDaily);
    }
    m_ui.setText(m_roundEndDocument, NIGHT_LINE_ID, line);
    // After a night the camera shows the village beside the card (villageRevealShown):
    // the card stands on the right and the left of the picture is left clear.
    m_ui.setClass(m_roundEndDocument, ROUND_END_BODY_ID, REVEAL_CLASS, villageRevealShown());
    m_ui.setText(m_roundEndDocument, LEVEL_NAME_ID, levelName);
    m_ui.setText(m_roundEndDocument, DIFFICULTY_ID, level);
    m_ui.setText(m_roundEndDocument, SEED_ID, std::to_string(m_mazeWorld.seed));
    // "Play again" and "New maze" belong to free play. A night is not played again from
    // here (a finished night is replayed from the list of nights), and its maze is
    // fixed. In their place a night has one button: "Next night", or "Back to nights"
    // after a replay of an earlier night.
    const bool nextNight = night && nightEndOffer() == NightEndOffer::NextNight;
    m_ui.setClass(m_roundEndDocument, PLAY_AGAIN_ID, GONE_CLASS, night);
    // The maze of the day is played again from here, and there is no other one today.
    m_ui.setClass(m_roundEndDocument, NEW_MAZE_ID, GONE_CLASS, night || daily);
    m_ui.setClass(m_roundEndDocument, NEXT_NIGHT_ID, GONE_CLASS, !nextNight);
    m_ui.setClass(m_roundEndDocument, BACK_TO_NIGHTS_ID, GONE_CLASS, !night || nextNight);
    // The seed names a maze of free play for a friend. The maze of a night cannot be
    // entered anywhere, so its row is left out.
    // The seed of the maze of the day is its day, which the row above shows.
    m_ui.setClass(m_roundEndDocument, SEED_ROW_ID, GONE_CLASS, night || daily);
    const char* keys = daily ? DAILY_KEYS_TEXT : ROUND_END_KEYS_TEXT;
    if (night) {
        keys = nextNight ? NEXT_NIGHT_KEYS_TEXT : BACK_TO_NIGHTS_KEYS_TEXT;
    }
    m_ui.setText(m_roundEndDocument, KEYS_ID, keys);
}

void NightMazeApp::fillSettingsDocument() {
    // The three sliders and the numbers next to them. The id of a slider is the name of
    // its setting, and the value it gets is the text the settings file would hold.
    const std::string sensitivityId(MOUSE_SENSITIVITY_SETTING);
    const std::string sensitivity = mouseSensitivityLabel(m_settings.mouseSensitivity);
    m_ui.setValue(m_settingsDocument, sensitivityId, sensitivity);
    m_ui.setText(m_settingsDocument, sensitivityId + TEXT_ID_SUFFIX, sensitivity);

    const std::string fieldOfViewId(FIELD_OF_VIEW_SETTING);
    m_ui.setValue(m_settingsDocument, fieldOfViewId,
                  std::to_string(std::lround(m_settings.fieldOfViewDegrees)));
    m_ui.setText(m_settingsDocument, fieldOfViewId + TEXT_ID_SUFFIX,
                 fieldOfViewLabel(m_settings.fieldOfViewDegrees));

    const std::string volumeId(MASTER_VOLUME_SETTING);
    const std::string volume = masterVolumeLabel(m_settings.masterVolume);
    m_ui.setValue(m_settingsDocument, volumeId, volume);
    m_ui.setText(m_settingsDocument, volumeId + TEXT_ID_SUFFIX, volume);

    // The three volumes under it, in the same way.
    const std::string effectsId(EFFECTS_VOLUME_SETTING);
    const std::string effects = masterVolumeLabel(m_settings.effectsVolume);
    m_ui.setValue(m_settingsDocument, effectsId, effects);
    m_ui.setText(m_settingsDocument, effectsId + TEXT_ID_SUFFIX, effects);

    const std::string ambientId(AMBIENT_VOLUME_SETTING);
    const std::string ambient = masterVolumeLabel(m_settings.ambientVolume);
    m_ui.setValue(m_settingsDocument, ambientId, ambient);
    m_ui.setText(m_settingsDocument, ambientId + TEXT_ID_SUFFIX, ambient);

    const std::string musicId(MUSIC_VOLUME_SETTING);
    const std::string music = masterVolumeLabel(m_settings.musicVolume);
    m_ui.setValue(m_settingsDocument, musicId, music);
    m_ui.setText(m_settingsDocument, musicId + TEXT_ID_SUFFIX, music);

    // The switch of the fullscreen: a class moves its knob.
    m_ui.setClass(m_settingsDocument, FULLSCREEN_ID, ON_CLASS, m_settings.fullscreen);
    m_ui.setText(m_settingsDocument, std::string(FULLSCREEN_ID) + TEXT_ID_SUFFIX,
                 m_settings.fullscreen ? "On" : "Off");

    // The size of the window. While the game is fullscreen it covers the screen, so
    // the row shows the resolution of the desktop and is dimmed.
    m_ui.setClass(m_settingsDocument, WINDOW_SIZE_ROW_ID, OFF_CLASS, m_settings.fullscreen);
    if (m_settings.fullscreen) {
        const core::Size desktop = window().desktopSize();
        m_ui.setText(m_settingsDocument, WINDOW_SIZE_ID,
                     windowSizeLabel({.width = desktop.width, .height = desktop.height}));
    } else {
        m_ui.setText(m_settingsDocument, WINDOW_SIZE_ID, windowSizeLabel(m_settings.windowSize));
    }

    fillControls();
}

} // namespace game
