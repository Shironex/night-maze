// The Night Maze application: game state and rendering of a frame.
#include "game/NightMazeApp.hpp"

#include "assets/ImageLoader.hpp"
#include "core/Files.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "game/Interactables.hpp"
#include "game/Ledger.hpp"
#include "game/ModelDraw.hpp"
#include "game/NightMazeAppShared.hpp"
#include "game/Puddles.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace game {

namespace {

constexpr int INITIAL_WIDTH = 1280;
constexpr int INITIAL_HEIGHT = 720;

// Shader files, relative to the assets directory. The scene without lighting is drawn
// with the first pair, the lines of the collision boxes and spheres with the second, the
// scene with lighting per fragment with the third, with lighting per vertex with the
// fourth and the sky with the fifth. The grass has three files: between its vertex and
// its fragment shader runs a geometry shader. The four programs after it do not draw
// the scene: they draw one triangle over the whole target and share its vertex shader. The
// composite program brings the HDR picture of the scene to the window, the preview
// program makes the pictures of the framebuffer attachments for the debug UI, and the
// bright pass and blur programs are the two steps of the bloom. The shadow depth
// program draws the scene from a light into a shadow map: positions only, no colours.
constexpr const char* TEXTURED_VERTEX_SHADER_FILE = "shaders/textured.vert";
constexpr const char* TEXTURED_FRAGMENT_SHADER_FILE = "shaders/textured.frag";
constexpr const char* COLOR_VERTEX_SHADER_FILE = "shaders/color.vert";
constexpr const char* COLOR_FRAGMENT_SHADER_FILE = "shaders/color.frag";
constexpr const char* LIT_VERTEX_SHADER_FILE = "shaders/lit.vert";
constexpr const char* LIT_FRAGMENT_SHADER_FILE = "shaders/lit.frag";
constexpr const char* GOURAUD_VERTEX_SHADER_FILE = "shaders/gouraud.vert";
constexpr const char* GOURAUD_FRAGMENT_SHADER_FILE = "shaders/gouraud.frag";
constexpr const char* SKYBOX_VERTEX_SHADER_FILE = "shaders/skybox.vert";
constexpr const char* SKYBOX_FRAGMENT_SHADER_FILE = "shaders/skybox.frag";
constexpr const char* GRASS_VERTEX_SHADER_FILE = "shaders/grass.vert";
constexpr const char* GRASS_GEOMETRY_SHADER_FILE = "shaders/grass.geom";
constexpr const char* GRASS_FRAGMENT_SHADER_FILE = "shaders/grass.frag";
constexpr const char* FULLSCREEN_VERTEX_SHADER_FILE = "shaders/post/composite.vert";
constexpr const char* COMPOSITE_FRAGMENT_SHADER_FILE = "shaders/post/composite.frag";
constexpr const char* PREVIEW_FRAGMENT_SHADER_FILE = "shaders/post/preview.frag";
constexpr const char* BRIGHT_PASS_FRAGMENT_SHADER_FILE = "shaders/post/bright.frag";
constexpr const char* BLUR_FRAGMENT_SHADER_FILE = "shaders/post/blur.frag";
constexpr const char* SHADOW_DEPTH_VERTEX_SHADER_FILE = "shaders/shadow_depth.vert";
constexpr const char* SHADOW_DEPTH_FRAGMENT_SHADER_FILE = "shaders/shadow_depth.frag";
// The minimap has two programs. The first draws its flat shapes into the framebuffer of
// the minimap. The second puts that picture into the window: one more program that
// draws the triangle of FULLSCREEN_VERTEX_SHADER_FILE.
constexpr const char* MINIMAP_VERTEX_SHADER_FILE = "shaders/post/minimap.vert";
constexpr const char* MINIMAP_FRAGMENT_SHADER_FILE = "shaders/post/minimap.frag";
constexpr const char* MINIMAP_OVERLAY_FRAGMENT_SHADER_FILE = "shaders/post/minimap_overlay.frag";
// The reflect program draws the crystals and the puddles with the sky on them.
constexpr const char* REFLECT_VERTEX_SHADER_FILE = "shaders/reflect.vert";
constexpr const char* REFLECT_FRAGMENT_SHADER_FILE = "shaders/reflect.frag";

// The heightmap of the terrain, relative to the assets directory: a grey picture made
// by tools/blender/make_heightmap.py.
constexpr const char* HEIGHTMAP_FILE = "textures/heightmap.png";

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

// How long the wind of the maze is heard after the button of the debug window asked for
// it, in seconds: long enough to hear it move.
constexpr double WIND_PREVIEW_SECONDS = 5.0;

// The part "Controls" of the settings screen: what the button of an action says while
// it waits for a key, and the line under the rows while there is nothing else to say.
constexpr const char* PRESS_A_KEY_TEXT = "Press a key";
constexpr const char* CONTROLS_RULE_TEXT = "A key already in use swaps the two.";
constexpr const char* CONTROLS_RESET_TEXT = "The default keys are back.";

// What the hint next to the seed field says when the field cannot be read.
constexpr const char* SEED_HINT_TEXT = "Digits only, up to 4294967295";

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

// Reads the settings file from the working directory. Without a file (the first start)
// and with a file that cannot be read the game starts with its defaults: parseSettings
// skips everything it does not understand.
GameSettings loadSettings() {
    std::string text;
    if (!core::readTextFile(SETTINGS_FILE_NAME, text)) {
        core::logInfo(std::string("No settings file (") + SETTINGS_FILE_NAME +
                      "): starting with the defaults");
        return {};
    }
    core::logInfo(std::string("Loaded settings: ") + SETTINGS_FILE_NAME);
    return parseSettings(text);
}

// The request for the maze of a new game: the size and the crystals of its difficulty
// level and its seed. The numbers of levers and notes keep their defaults, and the story
// notes start with the line the settings file says is the next unread one. shadeLines is
// false for a calm night: the notes then leave out the lines about the shadow.
MazeSettings mazeSettingsFor(Difficulty difficulty, std::uint32_t seed, int nextStoryLine,
                             bool shadeLines) {
    const DifficultyLevel& level = difficultyLevel(difficulty);
    MazeSettings settings;
    settings.width = level.mazeWidth;
    settings.height = level.mazeHeight;
    settings.crystalCount = level.crystalCount;
    settings.seed = seed;
    settings.interactables.firstStoryLine = nextStoryLine;
    settings.interactables.shadeLines = shadeLines;
    return settings;
}

// The request for the first maze. A game that opens with the intro (game::startMode,
// the switch --intro) builds the maze of the intro, whatever the settings and the
// command line say: the shots of the intro were picked in that maze. The maze of the
// player is built when the intro is over. Every other start builds the maze behind the
// main menu, which is also the maze of the round --play starts in: the seed of the
// command line and the level of the settings.
MazeSettings firstMazeSettings(const StartOptions& options, const GameSettings& settings,
                               bool shadeLines) {
    if (startMode(options) == GameMode::Intro) {
        return mazeSettingsFor(INTRO_DIFFICULTY, INTRO_MAZE_SEED, settings.nextStoryLine,
                               shadeLines);
    }
    return mazeSettingsFor(settings.difficulty, options.seed, settings.nextStoryLine, shadeLines);
}

constexpr double MILLISECONDS_PER_SECOND = 1000.0;

// Reads the heightmap picture. When it cannot be loaded the ground is flat: the error is
// in the log and the game is still playable.
Heightmap loadHeightmap() {
    const std::filesystem::path path = core::assetPath(HEIGHTMAP_FILE);
    assets::Image image;
    std::string error;
    // RowOrder::TopFirst: no row flip. The top row of the picture is the north edge of
    // the land (game::Heightmap), so it has to come first.
    if (!assets::loadImage(path, image, error, assets::RowOrder::TopFirst)) {
        // loadImage has logged which file failed and why.
        return {};
    }
    core::logInfo("Loaded heightmap: " + core::pathText(path));
    return heightmapFromImage(image);
}

} // namespace

NightMazeApp::NightMazeApp(const StartOptions& options)
    : core::Application(INITIAL_WIDTH, INITIAL_HEIGHT, "Night Maze"),
      m_texturedShader(core::assetPath(TEXTURED_VERTEX_SHADER_FILE),
                       core::assetPath(TEXTURED_FRAGMENT_SHADER_FILE)),
      m_colorShader(core::assetPath(COLOR_VERTEX_SHADER_FILE),
                    core::assetPath(COLOR_FRAGMENT_SHADER_FILE)),
      m_litShader(core::assetPath(LIT_VERTEX_SHADER_FILE),
                  core::assetPath(LIT_FRAGMENT_SHADER_FILE)),
      m_gouraudShader(core::assetPath(GOURAUD_VERTEX_SHADER_FILE),
                      core::assetPath(GOURAUD_FRAGMENT_SHADER_FILE)),
      m_skyboxShader(core::assetPath(SKYBOX_VERTEX_SHADER_FILE),
                     core::assetPath(SKYBOX_FRAGMENT_SHADER_FILE)),
      // The geometry shader is the third argument, although it runs second: it is the
      // optional one.
      m_grassShader(core::assetPath(GRASS_VERTEX_SHADER_FILE),
                    core::assetPath(GRASS_FRAGMENT_SHADER_FILE),
                    core::assetPath(GRASS_GEOMETRY_SHADER_FILE)),
      m_compositeShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                        core::assetPath(COMPOSITE_FRAGMENT_SHADER_FILE)),
      m_previewShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                      core::assetPath(PREVIEW_FRAGMENT_SHADER_FILE)),
      m_brightPassShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                         core::assetPath(BRIGHT_PASS_FRAGMENT_SHADER_FILE)),
      m_blurShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                   core::assetPath(BLUR_FRAGMENT_SHADER_FILE)),
      m_shadowDepthShader(core::assetPath(SHADOW_DEPTH_VERTEX_SHADER_FILE),
                          core::assetPath(SHADOW_DEPTH_FRAGMENT_SHADER_FILE)),
      m_minimapShader(core::assetPath(MINIMAP_VERTEX_SHADER_FILE),
                      core::assetPath(MINIMAP_FRAGMENT_SHADER_FILE)),
      m_minimapOverlayShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                             core::assetPath(MINIMAP_OVERLAY_FRAGMENT_SHADER_FILE)),
      m_reflectShader(core::assetPath(REFLECT_VERTEX_SHADER_FILE),
                      core::assetPath(REFLECT_FRAGMENT_SHADER_FILE)),
      m_mazeRenderer(m_assets),
      m_gameplayRenderer(m_assets),
      m_villageRenderer(m_assets),
      m_interactableRenderer(m_assets),
      m_terrainRenderer(m_assets),
      m_puddleRenderer(m_assets),
      // The background of the main menu: the video unless the command line asked for
      // something else. Opening the video waits for its first frame.
      m_menuBackground(options.menuBackground),
      // The settings file is read before the first maze is built: the maze has the size
      // of the difficulty that was chosen last.
      m_settings(loadSettings()),
      m_savedSettings(m_settings),
      m_calmRun(options.calm),
      // The first maze: the one of the intro, or the one behind the main menu.
      m_mazeSettings(firstMazeSettings(options, m_settings, shadeInGame())),
      m_heightmap(loadHeightmap()),
      m_mazeWorld(buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed,
                                 m_heightmap, m_terrainSettings.heightScale,
                                 m_mazeSettings.interactables, m_mazeSettings.crystalCount)),
      m_menuCamera(options.menuCamera),
      // The first screen: the main menu, a round for the tools that record the game,
      // or the intro when the command line asked for it.
      m_mode(startMode(options)),
      // The game "Play" starts: the difficulty of the settings, and the seed of the
      // command line when one was named there. Otherwise the menu rolls one (below).
      m_newGame{.difficulty = m_settings.difficulty, .seed = options.seed},
      m_startSeed(options.seed),
      m_toolRun(options.toolSwitch),
      m_campaignIntroPlays(campaignIntroPlays(options)),
      m_fixedDailyDate(fixedDailyDate(options)),
      m_ledgerCounts(!options.toolSwitch),
      m_playedDifficultyName(difficultyLevel(m_settings.difficulty).name),
      m_ui(window()) {
    // The two lit programs and the grass program read the lights from the uniform buffer
    // of m_lightRig. Each program is told once: the shader repeats it by itself after
    // a reload.
    m_lightRig.connect(m_litShader);
    m_lightRig.connect(m_gouraudShader);
    m_lightRig.connect(m_grassShader);
    // The reflect program lights the crystals and the puddles with the same lights.
    m_lightRig.connect(m_reflectShader);

    // The rules of the first round are the ones of the same difficulty level as the
    // first maze (the gate and the battery). The number of flasks is among them, and
    // flasks are seen: the pictures of the intro need the level of the intro.
    const DifficultyLevel& level =
        difficultyLevel(m_mode == GameMode::Intro ? INTRO_DIFFICULTY : m_settings.difficulty);
    m_gameplay.requiredFraction = level.requiredFraction;
    m_gameplay.batteryLifetimeSeconds = level.batteryLifetimeSeconds;
    m_gameplay.flaskCount = level.flaskCount;
    // A calm night has no shade: the first round is started with that already known.
    m_gameplay.shade.enabled = shadeInGame();

    // The first maze was built in the initializer list, because MazeWorld cannot be
    // created empty. What is left is the same as after every later regeneration.
    if (options.night != 0) {
        // A night of the campaign named on the command line (--night): its maze takes the
        // place of the first one. The campaign of such a run is one of its own, with the
        // seed of the command line or a fixed one, and winning the night does not touch
        // the settings file.
        // ponytail: a tool run builds two mazes at its start, the first one for nothing.
        // Build the night in the initializer list if that half second ever matters.
        startNight(options.night, options.seedGiven ? options.seed : TOOL_CAMPAIGN_SEED, false);
    } else if (options.daily != NO_DAILY_DATE) {
        // The maze of a day named on the command line (--daily), in the same way.
        startNewGame({.difficulty = DAILY_DIFFICULTY, .seed = options.daily}, options.daily);
    } else {
        uploadGround();
        beginRound();
        m_menuCameraPath = buildMenuCameraPath(m_mazeWorld);
    }

    // The switches for captures change the first round only: beginRound above is the
    // normal start, and R and "Play again" call it again, so they go back to it.
    if (options.collectAll) {
        // The crystals are picked up by the rules of the round: one step of no length
        // with the player standing in each of them, which also opens the gate. The step
        // would also discover the cells around the crystals and pick up the flasks near
        // them, which the player has not earned, so those two are put back as they were.
        const Discovery discoveredBefore = m_round.discovery;
        const std::vector<RoundFlask> flasksBefore = m_round.flasks;
        const int flasksCollectedBefore = m_round.flasksCollected;
        for (const RoundCrystal& crystal : m_round.crystals) {
            const glm::vec3 feet =
                crystal.restPosition - glm::vec3(0.0F, PLAYER_REACH_HEIGHT, 0.0F);
            updateRound(m_round, m_mazeWorld, m_gameplay, feet, m_lighting.flashlightOn, 0.0F);
        }
        // The heartstone too, in the same way. Only the round takes it: nobody carried
        // it, so the player does not start heavy.
        if (m_mazeWorld.heartstone) {
            const MazeCell cell = *m_mazeWorld.heartstone;
            const glm::vec3 feet =
                heartstoneCenter(heartstoneRestPosition(cell, groundHeightAt(m_mazeWorld, cell))) -
                glm::vec3(0.0F, PLAYER_REACH_HEIGHT, 0.0F);
            updateRound(m_round, m_mazeWorld, m_gameplay, feet, m_lighting.flashlightOn, 0.0F);
        }
        // The gate has opened: its box leaves the obstacle list, like in a step that
        // opens it (onUpdate).
        m_obstacles = roundObstacles(m_mazeWorld, m_round);
        m_round.discovery = discoveredBefore;
        m_round.flasks = flasksBefore;
        m_round.flasksCollected = flasksCollectedBefore;
    }
    if (options.startCell) {
        // The maze is known only now, so this is where a cell that is not in it is
        // refused: main prints the message and ends the program.
        const MazeCell cell = *options.startCell;
        if (!m_mazeWorld.maze.contains(cell.x, cell.z)) {
            throw std::invalid_argument(
                "The switch --start-cell does not accept " + std::to_string(cell.x) + "," +
                std::to_string(cell.z) + ": the maze has " +
                std::to_string(m_mazeWorld.maze.width()) + " columns and " +
                std::to_string(m_mazeWorld.maze.height()) + " rows, counted from 0");
        }
        // The middle of the cell, feet on the ground there, like beginRound does it for
        // the start cell.
        m_player.position = cellCenter(cell.x, cell.z);
        m_player.position.y =
            m_mazeWorld.terrain.heightAt(m_player.position.x, m_player.position.z);
        m_previousPlayerPosition = m_player.position;
        m_camera.position = m_player.eyePosition();
        // What the player sees from the new place is on the minimap from the first frame.
        discoverAround(m_round.discovery, roundMaze(m_mazeWorld, m_round), m_player.position);
    }
    if (options.startYawDegrees) {
        m_camera.yawDegrees = *options.startYawDegrees;
    }

    // What the settings file says about the mouse, the camera, the window and the
    // loudness of the sound.
    applyViewSettings();
    applyWindowSettings();
    applyAudioSettings();
    // A window that starts in the background must not pause the game by "losing" a
    // focus it never had.
    m_windowWasFocused = window().isFocused();

    // The documents of the screens with a menu. They are loaded once and stay hidden
    // until their screen comes up. An error is in the log (ui::UiLayer).
    m_mainMenuDocument = m_ui.loadDocument(MAIN_MENU_DOCUMENT_FILE);
    m_freePlayDocument = m_ui.loadDocument(FREE_PLAY_DOCUMENT_FILE);
    m_nightsDocument = m_ui.loadDocument(NIGHTS_DOCUMENT_FILE);
    m_newCampaignDocument = m_ui.loadDocument(NEW_CAMPAIGN_DOCUMENT_FILE);
    m_ledgerDocument = m_ui.loadDocument(LEDGER_DOCUMENT_FILE);
    m_pauseDocument = m_ui.loadDocument(PAUSE_DOCUMENT_FILE);
    m_roundEndDocument = m_ui.loadDocument(ROUND_END_DOCUMENT_FILE);
    m_settingsDocument = m_ui.loadDocument(SETTINGS_DOCUMENT_FILE);
    m_menusLoaded = m_mainMenuDocument != ui::NO_DOCUMENT &&
                    m_freePlayDocument != ui::NO_DOCUMENT && m_nightsDocument != ui::NO_DOCUMENT &&
                    m_newCampaignDocument != ui::NO_DOCUMENT &&
                    m_ledgerDocument != ui::NO_DOCUMENT && m_pauseDocument != ui::NO_DOCUMENT &&
                    m_roundEndDocument != ui::NO_DOCUMENT && m_settingsDocument != ui::NO_DOCUMENT;
    // The card of the intro and of the nights: one more document, not one of the menus.
    m_cardDocument = m_ui.loadDocument(CARD_DOCUMENT_FILE);
    if (!m_menusLoaded) {
        core::logError("The menus cannot be shown: the game starts straight in a round");
        m_mode = GameMode::Playing;
    }
    // The settings screen is filled once now, while it is hidden. Its switch slides
    // when its class changes: filled only when the screen comes up, a fullscreen that
    // was saved as on would slide from off to on before the eyes of the player.
    fillSettingsDocument();
    // The same for the free play screen and its switch "Calm night".
    fillFreePlayDocument();

    // The sounds: one file per cue, in the order of the enum, so the number of a cue is
    // the number of its sound (game::soundCueIndex). A missing file is in the log and
    // its cue is silent (audio::AudioEngine). The winds go into the ambient group of
    // the engine and everything else into the effects group: each group has a volume
    // of its own (applyAudioSettings). After the cues comes the one piece of music, the
    // theme of the menu, in the music group (game::MENU_THEME_SOUND).
    std::array<audio::SoundFile, SOUND_CUE_COUNT + 1> soundFiles;
    for (std::size_t i = 0; i < SOUND_CUE_COUNT; ++i) {
        const auto cue = static_cast<SoundCue>(i);
        soundFiles.at(i) = {.path = core::assetPath(soundCueFile(cue)),
                            .group = soundCueIsAmbient(cue) ? audio::SoundGroup::Ambient
                                                            : audio::SoundGroup::Effects};
    }
    soundFiles.at(MENU_THEME_SOUND) = {.path = core::assetPath(MENU_THEME_FILE),
                                       .group = audio::SoundGroup::Music};
    m_audio.load(soundFiles);

    // The seed free play offers for the first game: the one of the command line, or
    // a random one.
    if (options.seedGiven) {
        m_ui.setValue(m_freePlayDocument, SEED_ID, std::to_string(m_newGame.seed));
    } else {
        rollSeed();
    }
    // An intro without its card would be half a minute of pictures without a word: it
    // is left at once, like at its end, and the main menu comes up with its own maze.
    if (m_mode == GameMode::Intro && m_cardDocument == ui::NO_DOCUMENT) {
        core::logError("The card of the intro cannot be shown: the intro is left out");
        handleGameEvent(GameEvent::IntroFinished);
    }
    showScreen();
}

NightMazeApp::~NightMazeApp() {
    saveSettings();
}

// A cue that was started plays to its end, also when the pause menu comes up or the
// round is left in that moment (back to the main menu, a restart, a new maze). Nothing
// stops it on purpose: every cue of a round is a one shot of under three seconds, so the
// longest thing that can be heard over a menu is the tail of the bell, and stopping sounds for
// the pause would need a way to go on with them afterwards. No new cue OF THE ROUND
// starts under a menu: those come from onUpdate, which returns early there, and the
// keys of the round are not read (roundInput in onRender). The one cue a menu itself
// plays is the sample click of the volume slider on the settings screen, which is
// there to be heard (handleControlChanges).
//
// The wind of the maze is not played here. It is a loop, and it does stop for a menu:
// updateAmbience fades it out and in, and the theme of the menu the other way round.
void NightMazeApp::playCue(SoundCue cue, float volume) {
    m_audio.play(soundCueIndex(cue), volume);
    m_lastCueName = soundCueName(cue);
    m_lastCueVolume = volume;
    ++m_cuesPlayed;
}

void NightMazeApp::updateAmbience(bool windowFocused) {
    // A sample of the wind: asked for by the debug window, or still running after the
    // ambient slider was moved (handleControlChanges). Its time runs down with the real
    // time of the frames.
    if (m_windSampleRequested) {
        m_windSampleRequested = false;
        m_windSampleLeft = WIND_PREVIEW_SECONDS;
    }
    const bool sample = m_windSampleLeft > 0.0;
    m_windSampleLeft = std::max(m_windSampleLeft - time().deltaSeconds(), 0.0);

    // Said in every frame: the engine does something only in the frame the answer
    // changes, and then fades the loop in or out (audio::AudioEngine::setLoop). A sample
    // comes in fast, because it is short.
    const bool wind = mazeWindPlays({.mode = m_mode,
                                     .windowFocused = windowFocused,
                                     .menuCamera = m_menuCamera.enabled,
                                     .sample = sample});
    const float fadeIn = sample ? MAZE_WIND_SAMPLE_FADE_IN_SECONDS : MAZE_WIND_FADE_IN_SECONDS;
    m_audio.setLoop(soundCueIndex(SoundCue::MazeWind), wind,
                    wind ? fadeIn : MAZE_WIND_FADE_OUT_SECONDS);
    // The wind follows the fade to black of a catch and the way back up from it. Only the
    // ambient group is touched, so the other sounds keep their level.
    m_audio.setGroupVolume(audio::SoundGroup::Ambient,
                           masterVolumeGain(m_settings.ambientVolume) *
                               pictureBrightness(m_catchSeconds, m_round));

    // The theme of the menu, said in every frame like the wind. The loop keeps its
    // place while it is off, so the piece goes on after a night where it was left. The
    // first call of the program comes before the first picture: the theme waits for it.
    const bool theme = menuThemePlays(
        {.mode = m_mode, .windowFocused = windowFocused, .pictureShown = m_pictureShown});
    m_pictureShown = true;
    m_audio.setLoop(MENU_THEME_SOUND, theme,
                    theme ? MENU_THEME_FADE_IN_SECONDS : MENU_THEME_FADE_OUT_SECONDS);
    if (theme != m_menuThemeOn) {
        m_menuThemeOn = theme;
        core::logInfo(theme ? "Music: the theme of the menu fades in"
                            : "Music: the theme of the menu fades out");
    }
}

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
    m_introSeconds = 0.0;
    m_introCardShown = INTRO_CARD_COUNT;
    showScreen();
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

bool NightMazeApp::introTellsOfShade() const {
    return m_mode == GameMode::CampaignIntro ? !m_calmRun : shadeInGame();
}

void NightMazeApp::leaveIntro(bool intoNight) {
    // The wind is one sound of half a minute: without this it would go on over the
    // main menu or the title card of the night after a skip.
    m_audio.stopAll();
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

NightEndOffer NightMazeApp::nightEndOffer() const {
    // The campaign of the settings is already the one after the win (finishRound).
    return game::nightEndOffer(m_settings.campaignNight, m_playedNight, m_nightCounts);
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

void NightMazeApp::applyViewSettings() {
    m_mouseSensitivity = mouseDegreesPerUnit(m_settings.mouseSensitivity);
    m_camera.fovDegrees = m_settings.fieldOfViewDegrees;
}

void NightMazeApp::applyAudioSettings() {
    m_audio.setMasterVolume(masterVolumeGain(m_settings.masterVolume));
    // The three groups under it, each with the same curve from its own slider.
    m_audio.setGroupVolume(audio::SoundGroup::Effects, masterVolumeGain(m_settings.effectsVolume));
    m_audio.setGroupVolume(audio::SoundGroup::Ambient, masterVolumeGain(m_settings.ambientVolume));
    m_audio.setGroupVolume(audio::SoundGroup::Music, masterVolumeGain(m_settings.musicVolume));
}

void NightMazeApp::applyWindowSettings() {
    // The size first: while the window is fullscreen it is only remembered, and
    // switching fullscreen off then goes back to a window of that size.
    window().setWindowedSize(
        {.width = m_settings.windowSize.width, .height = m_settings.windowSize.height});
    window().setFullscreen(m_settings.fullscreen);
}

void NightMazeApp::saveSettings() {
    if (m_settings == m_savedSettings) {
        return;
    }
    if (core::writeTextFile(SETTINGS_FILE_NAME, formatSettings(m_settings))) {
        m_savedSettings = m_settings;
        core::logInfo(std::string("Saved settings: ") + SETTINGS_FILE_NAME);
    } else {
        // The game goes on with the settings it has. They are tried again at the next
        // change.
        core::logError(std::string("The settings file cannot be written: ") + SETTINGS_FILE_NAME);
    }
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
