// The Night Maze application: the change from one screen to the next, the intro and the
// cards of text.
#include "game/NightMazeApp.hpp"

#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "game/NightMazeAppShared.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <string>

namespace game {

namespace {

constexpr double MILLISECONDS_PER_SECOND = 1000.0;

} // namespace

void NightMazeApp::onEscapePressed() {
    handleGameEvent(GameEvent::Escape);
}

void NightMazeApp::handleGameEvent(GameEvent event) {
    const GameMode before = m_mode;
    // Asked before the screen changes: the answers depend on the screen the event
    // was sent on.
    const bool newRound = startsRound(before, event);
    const bool newGame = startsNewGame(before, event);
    m_mode = nextMode(before, event);

    // Without the documents only the pause can be entered: it shows nothing, but Escape
    // leaves it again. The main menu and the result screen have no key that leaves
    // them, so a round is started in their place.
    bool roundInsteadOfMenu = false;
    if (!m_menusLoaded && (m_mode == GameMode::MainMenu || m_mode == GameMode::RoundEnd)) {
        m_mode = GameMode::Playing;
        roundInsteadOfMenu = true;
    }

    if (m_mode == before && !roundInsteadOfMenu) {
        return;
    }
    // The intro is over: at its end, skipped by a key or left with Escape. All three
    // arrive here. The intro of a campaign leads on to the first night.
    if (isIntro(before)) {
        leaveIntro(before == GameMode::CampaignIntro);
    }
    // The ending card is over, at its end or skipped: its bell must not ring on over
    // the main menu.
    if (before == GameMode::EndingCard) {
        // The sting of the last night is faded, not cut: stopAll leaves a fading sound.
        fadeNightEnd();
        m_audio.stopAll();
    }
    // After a film that ends in black the main menu comes in out of black.
    m_menuAfterIntro = before == GameMode::Intro || before == GameMode::EndingCard;
    // A round was just won: the reveal of the village starts from the camera of the
    // player as it is now (villageRevealShown decides whether it is shown).
    if (before == GameMode::Playing &&
        (m_mode == GameMode::RoundEnd || m_mode == GameMode::VillageBeat)) {
        m_revealStart = {.eye = m_camera.position,
                         .yawDegrees = m_camera.yawDegrees,
                         .pitchDegrees = m_camera.pitchDegrees,
                         .fovDegrees = m_camera.fovDegrees};
        m_revealSeconds = 0.0;
    }
    if (newGame) {
        // "New maze" on the result screen: the same difficulty, another seed. "Play"
        // in free play starts the seed its field shows (handleMenuActions has read
        // it).
        if (event == GameEvent::NewMaze) {
            m_newGame.seed = randomSeed();
        }
        startNewGame(m_newGame);
    } else if (newRound || roundInsteadOfMenu) {
        beginRound();
    }

    // The settings screen was left: what was changed there goes into the file.
    const bool wasSettings =
        before == GameMode::SettingsFromMenu || before == GameMode::SettingsFromPause;
    if (wasSettings) {
        saveSettings();
    }
    // Back in the main menu after a game: the next game of free play gets a fresh seed.
    // Coming back from the settings, from free play or from the list of nights the seed
    // stays, with whatever was typed into its field. After the intro it stays too: that
    // is the first seed the menu offers, and it may be the one of the command line.
    const bool afterGame = before == GameMode::Paused || before == GameMode::RoundEnd ||
                           before == GameMode::EndingCard;
    if (m_mode == GameMode::MainMenu && afterGame) {
        rollSeed();
    }

    if (m_mode == GameMode::Quitting) {
        // The main loop ends after this frame (core::Application::run).
        window().requestClose();
    }
    showScreen();
    // Back from the ledger: the keyboard stands on the entry it was opened with, not on
    // the first entry of the menu.
    if (isLedger(before)) {
        m_ui.focus(m_mode == GameMode::Paused ? m_pauseDocument : m_mainMenuDocument,
                   LEDGER_ENTRY_ID);
    }
    // The title card of a night or the ending card has just come up: its clock starts.
    // Last, because a card that cannot be shown is over at once, which is an event again.
    if (m_mode != before && (m_mode == GameMode::NightCard || m_mode == GameMode::EndingCard)) {
        startStoryCard();
    }
    if (m_mode != before && m_mode == GameMode::VillageBeat) {
        startVillageBeat();
    }
}

void NightMazeApp::showScreen() {
    // A row of the controls that waited for a key waits no longer, and the line under
    // the rows starts empty when the settings screen comes up again.
    stopKeyCapture();
    m_controlsNote.clear();
    // The document of the screen, filled with what it shows at this moment.
    ui::DocumentId document = ui::NO_DOCUMENT;
    if (m_mode == GameMode::MainMenu) {
        document = m_mainMenuDocument;
        fillMainMenuDocument();
    } else if (m_mode == GameMode::Paused) {
        document = m_pauseDocument;
        fillPauseDocument();
    } else if (m_mode == GameMode::RoundEnd) {
        document = m_roundEndDocument;
        fillRoundEndDocument();
    } else if (m_mode == GameMode::SettingsFromMenu || m_mode == GameMode::SettingsFromPause) {
        document = m_settingsDocument;
        fillSettingsDocument();
    } else if (m_mode == GameMode::FreePlay) {
        document = m_freePlayDocument;
        fillFreePlayDocument();
    } else if (m_mode == GameMode::Nights) {
        document = m_nightsDocument;
        fillNightsDocument();
    } else if (isLedger(m_mode)) {
        document = m_ledgerDocument;
        fillLedgerDocument();
    } else if (m_mode == GameMode::NewCampaign) {
        // Nothing to fill: the question is always the same.
        document = m_newCampaignDocument;
    } else if (isFilm(m_mode)) {
        // The card is not filled here: updateIntro and updateStoryCard write it in
        // every frame.
        document = m_cardDocument;
    }
    m_ui.show(document);
    // The list of nights opens with the keyboard on the night the menu offers: the next
    // one. Which row that is changes, so the document cannot name it (autofocus).
    if (m_mode == GameMode::Nights) {
        m_ui.focus(m_nightsDocument, std::string(NIGHT_ID_PREFIX) +
                                         std::to_string(nightToOffer(m_settings.campaignNight)));
    }
    // The result screen of a night opens with the keyboard on its first button, so Enter
    // carries on. The document names "Play again" (autofocus), which is gone there.
    if (m_mode == GameMode::RoundEnd && m_playedNight != 0) {
        m_ui.focus(m_roundEndDocument,
                   nightEndOffer() == NightEndOffer::NextNight ? NEXT_NIGHT_ID : BACK_TO_NIGHTS_ID);
    }

    // The cursor follows the screen: captured for mouse look while a round is played,
    // free for the buttons of a menu. The menu camera does not turn with the mouse, so
    // it leaves the cursor free too. The intro and the cards capture it for another
    // reason: a film has no cursor in its picture, and there is nothing to click.
    input().setCursorCaptured((updatesRound(m_mode) && !m_menuCamera.enabled) || isFilm(m_mode));
}

void NightMazeApp::startIntro() {
    // Not without the card, not twice and not while the program is closing.
    if (m_cardDocument == ui::NO_DOCUMENT || isIntro(m_mode) || m_mode == GameMode::Quitting) {
        return;
    }
    // The maze of the intro, built like a new game of its level: the shots were picked
    // there. This does not go through game::nextMode: no event leads into the intro,
    // the application puts the game there.
    startNewGame({.difficulty = INTRO_DIFFICULTY, .seed = INTRO_MAZE_SEED});
    m_mode = GameMode::Intro;
    beginIntroClock();
    showScreen();
}

void NightMazeApp::beginIntroClock() {
    m_introSeconds = 0.0;
    m_introCardShown = INTRO_CARD_COUNT;
    m_storybookPlaying =
        m_storybook && m_storybookRenderer.load(*m_storybook, core::assetPath(STORYBOOK_FOLDER));
    if (m_storybook && !m_storybookPlaying) {
        core::logWarn("Storybook: its pictures cannot be loaded: the classic intro plays");
    }
}

bool NightMazeApp::anyKeyPressed() {
    // GLFW numbers its keys from GLFW_KEY_SPACE to GLFW_KEY_LAST, with gaps that are
    // never pressed, and its mouse buttons from 0. wasKeyPressed is true for one frame.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        if (input().wasKeyPressed(key)) {
            return true;
        }
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        if (input().wasMouseButtonPressed(button)) {
            return true;
        }
    }
    return false;
}

void NightMazeApp::writeCardLines(const std::array<const char*, STORY_CARD_LINE_COUNT>& lines,
                                  bool middle) {
    for (std::size_t line = 0; line < STORY_CARD_LINE_COUNT; ++line) {
        m_ui.setText(m_cardDocument, CARD_LINE_IDS.at(line), lines.at(line));
    }
    m_ui.setClass(m_cardDocument, CARD_ID, MIDDLE_CLASS, middle);
}

void NightMazeApp::showCardFrame(float black, float card,
                                 const std::array<float, STORY_CARD_LINE_COUNT>& lines,
                                 float hint) {
    m_ui.setOpacity(m_cardDocument, CARD_BLACK_ID, black);
    m_ui.setOpacity(m_cardDocument, CARD_ID, card);
    for (std::size_t line = 0; line < STORY_CARD_LINE_COUNT; ++line) {
        m_ui.setOpacity(m_cardDocument, CARD_LINE_IDS.at(line), lines.at(line));
    }
    m_ui.setOpacity(m_cardDocument, CARD_HINT_ID, hint);
}

void NightMazeApp::startStoryCard() {
    m_cardSeconds = 0.0;
    // A card that cannot be shown would be seconds of the scene with nothing on it: it
    // is left at once, like at its end.
    if (m_cardDocument == ui::NO_DOCUMENT) {
        handleGameEvent(GameEvent::CardFinished);
        return;
    }
    if (m_mode == GameMode::NightCard && m_playedDaily != NO_DAILY_DATE) {
        // The maze of the day: its name and its day.
        const std::string day = dailyDateText(m_playedDaily);
        writeCardLines({DAILY_NAME, day.c_str(), "", ""}, true);
    } else if (m_mode == GameMode::NightCard) {
        // "Night 2" and the name of the night.
        const std::string label = campaignNightLabel(m_playedNight);
        writeCardLines({label.c_str(), campaignNight(m_playedNight).title, "", ""}, true);
    } else {
        // The ending card: its third line says whether a crystal was left in the maze.
        const bool crystalsLeft = m_round.collectedCount < crystalTotal(m_round);
        writeCardLines(endingLines(crystalsLeft), true);
    }
    // Black from the first frame, and no text yet: the lines come up by themselves.
    showCardFrame(1.0F, 1.0F, {}, 0.0F);
}

void NightMazeApp::updateStoryCard() {
    const bool ending = m_mode == GameMode::EndingCard;
    const auto frameAt = [ending](double seconds) {
        return ending ? endingCardFrame(static_cast<float>(seconds))
                      : nightCardFrame(static_cast<float>(seconds));
    };
    // Any key and any mouse button skip the card, but not in its first half second:
    // the key that started the night is still down then (StoryCardFrame::skippable).
    if (frameAt(m_cardSeconds).skippable && anyKeyPressed()) {
        handleGameEvent(GameEvent::CardFinished);
        return;
    }

    // The clock follows the real time of the frames, like the one of the intro. The
    // bell of the ending card rings in the frame that passes its moment: once.
    const double before = m_cardSeconds;
    m_cardSeconds += time().deltaSeconds();
    const double bell = ENDING_BELL_SECONDS;
    if (ending && before <= bell && bell < m_cardSeconds) {
        playCue(SoundCue::IntroBell);
    }

    const StoryCardFrame frame = frameAt(m_cardSeconds);
    if (frame.finished) {
        handleGameEvent(GameEvent::CardFinished);
        return;
    }
    // A card on black: the black and the card are all there, the lines come and go.
    showCardFrame(1.0F, 1.0F, frame.lineOpacity, frame.hintOpacity);
}

void NightMazeApp::startVillageBeat() {
    m_cardSeconds = 0.0;
    if (m_cardDocument == ui::NO_DOCUMENT) {
        return;
    }
    // The document of the cards is on the screen (showScreen), with nothing of its
    // black and nothing of its card: the picture is the scene.
    writeCardLines({"", "", "", ""}, true);
    showCardFrame(0.0F, 0.0F, {}, 0.0F);
}

void NightMazeApp::updateVillageBeat() {
    // Any key and any mouse button end the look, but not in its first half second, like
    // a card: the key the player walked through the gate with is still down then.
    if (m_cardSeconds >= static_cast<double>(INTRO_SKIP_DELAY_SECONDS) && anyKeyPressed()) {
        handleGameEvent(GameEvent::CardFinished);
        return;
    }
    m_cardSeconds += time().deltaSeconds();
    if (m_cardSeconds >= static_cast<double>(VILLAGE_BEAT_SECONDS)) {
        handleGameEvent(GameEvent::CardFinished);
        return;
    }
    if (m_cardDocument != ui::NO_DOCUMENT) {
        showCardFrame(villageBeatBlack(static_cast<float>(m_cardSeconds)), 0.0F, {}, 0.0F);
    }
}

bool NightMazeApp::villageRevealShown() const {
    const bool won = m_mode == GameMode::RoundEnd || m_mode == GameMode::VillageBeat ||
                     m_mode == GameMode::EndingCard;
    return won && m_playedNight != 0 && m_mazeWorld.hasGate;
}

void NightMazeApp::updateIntro() {
    if (m_storybookPlaying) {
        updateStorybookIntro();
        return;
    }
    // Any key and any mouse button skip the whole intro. Escape is one of them, and it
    // also arrives as an event of its own before this function runs (onEscapePressed).
    // This function runs once per frame.
    if (introFrame(static_cast<float>(m_introSeconds)).skippable && anyKeyPressed()) {
        handleGameEvent(GameEvent::IntroFinished);
        return;
    }

    // The clock follows the real time of the frames. The sounds between the moment
    // before this frame and the moment after it are played now: each one once.
    const double before = m_introSeconds;
    m_introSeconds += time().deltaSeconds();
    for (const SoundCue cue :
         introCuesBetween(static_cast<float>(before), static_cast<float>(m_introSeconds))) {
        playCue(cue);
    }

    const IntroFrame frame = introFrame(static_cast<float>(m_introSeconds));
    if (frame.finished) {
        handleGameEvent(GameEvent::IntroFinished);
        return;
    }

    // The two lines, when the card has changed. The cut happens while no text is
    // shown (game::introFrame), so the new lines are never seen replacing the old ones.
    if (frame.card != m_introCardShown) {
        m_introCardShown = frame.card;
        // Whether the shadow, the enemy of the game, takes part in it: the fourth card
        // has another second line for a calm night (game::introLines).
        const IntroLines lines = introLines(frame.card, introTellsOfShade());
        writeCardLines({lines.first, lines.second, "", ""}, false);
    }
    // The two lines of a card of the intro come and go together, as the card: each line
    // by itself is all there.
    showCardFrame(frame.blackOpacity, frame.textOpacity, {1.0F, 1.0F, 1.0F, 1.0F},
                  frame.hintOpacity);
}

void NightMazeApp::updateStorybookIntro() {
    if (!m_storybook) {
        return;
    }
    // Set once in the constructor and never reset, so the reference holds all frame long.
    const Storybook& book = *m_storybook;
    const float crossfade = book.crossfadeSeconds;
    if (storybookFrame(m_storybookBeatSeconds, crossfade, static_cast<float>(m_introSeconds))
            .skippable &&
        anyKeyPressed()) {
        handleGameEvent(GameEvent::IntroFinished);
        return;
    }

    // The music starts with the first frame, each spoken line at its moment: once.
    const double before = m_introSeconds;
    m_introSeconds += time().deltaSeconds();
    if (before <= 0.0 && m_storybookMusicSound != NO_STORYBOOK_SOUND) {
        m_audio.play(m_storybookMusicSound);
    }
    for (const std::size_t beat :
         storybookVoicesBetween(m_storybookBeatSeconds, static_cast<float>(before),
                                static_cast<float>(m_introSeconds))) {
        if (m_storybookVoiceSounds.at(beat) != NO_STORYBOOK_SOUND) {
            m_audio.play(m_storybookVoiceSounds.at(beat));
        }
    }

    const StorybookFrame frame =
        storybookFrame(m_storybookBeatSeconds, crossfade, static_cast<float>(m_introSeconds));
    if (frame.finished) {
        handleGameEvent(GameEvent::IntroFinished);
        return;
    }
    // The lines change at the cut, where no text is shown (game::cardTextOpacity).
    if (frame.beat != m_introCardShown) {
        m_introCardShown = frame.beat;
        const StorybookBeat& beat = book.beats.at(frame.beat);
        writeCardLines({beat.lines.at(0).c_str(), beat.lines.at(1).c_str(), "", ""}, false);
    }
    showCardFrame(frame.blackOpacity, frame.textOpacity, {1.0F, 1.0F, 1.0F, 1.0F},
                  frame.hintOpacity);
}

bool NightMazeApp::introTellsOfShade() const {
    return m_mode == GameMode::CampaignIntro ? !m_calmRun : shadeInGame();
}

void NightMazeApp::leaveIntro(bool intoNight) {
    // The wind is one sound of half a minute: without this it would go on over the
    // main menu or the title card of the night after a skip. The same for the music
    // and the spoken lines of the storybook.
    m_audio.stopAll();
    // The pictures of the storybook are not needed until the next intro.
    m_storybookRenderer.unload();
    m_storybookPlaying = false;
    // Seen, to its end or to the key that skipped it. The settings file keeps the line,
    // so files of older versions stay valid, but nothing asks it any more: the intro
    // plays when a campaign begins. An intro that could not be shown at all was not seen.
    if (m_cardDocument != ui::NO_DOCUMENT) {
        m_settings.introSeen = true;
    }
    if (intoNight) {
        // The maze of the first night takes the place of the maze of the intro. The
        // seed of the campaign was drawn before the intro began (beginCampaignIntro).
        // startNight does not write the settings file, so it is written here.
        startNight(1, m_settings.campaignSeed, true);
        saveSettings();
        return;
    }
    // The maze of the player again: the level the main menu shows and the seed the
    // game was started with. It is built like a new game, which also writes the
    // settings file, with the line of the intro in it. The time is logged, because the
    // menu waits for it: it is the price of an intro with a maze of its own.
    const double started = glfwGetTime();
    startNewGame({.difficulty = m_newGame.difficulty, .seed = m_startSeed});
    core::logInfo("The intro is over: the maze of the main menu was built in " +
                  std::to_string(std::lround((glfwGetTime() - started) * MILLISECONDS_PER_SECOND)) +
                  " ms");
}

} // namespace game
