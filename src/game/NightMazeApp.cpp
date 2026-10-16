// The Night Maze application: how it is put together at the start, its settings and its
// sounds.
#include "game/NightMazeApp.hpp"

#include "assets/ImageLoader.hpp"
#include "core/Files.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "game/ModelDraw.hpp"
#include "game/NightMazeAppShared.hpp"
#include "game/Puddles.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
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

// How long the wind of the maze is heard after the button of the debug window asked for
// it, in seconds: long enough to hear it move.
constexpr double WIND_PREVIEW_SECONDS = 5.0;

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

// The manifest of the storybook intro (assets/intro/storybook.txt), or nothing: the
// classic intro is told then. Its absence is no error, a manifest that cannot be read is.
std::optional<Storybook> loadStorybook() {
    const std::filesystem::path path = core::assetPath(STORYBOOK_FOLDER) / STORYBOOK_MANIFEST_FILE;
    if (!std::filesystem::exists(path)) {
        core::logInfo("Intro: classic (no storybook at " + core::pathText(path) + ")");
        return std::nullopt;
    }
    std::string text;
    if (!core::readTextFile(path, text)) {
        core::logError("Intro: classic (the storybook cannot be read: " + core::pathText(path) +
                       ")");
        return std::nullopt;
    }
    StorybookResult result = parseStorybook(text);
    if (!result.error.empty()) {
        core::logError("Intro: classic (" + core::pathText(path) + ", " + result.error + ")");
        return std::nullopt;
    }
    core::logInfo("Intro: storybook of " + std::to_string(result.book.beats.size()) + " pictures");
    return std::move(result.book);
}

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
    // of its own (applyAudioSettings). After the cues comes the music, in the music
    // group: the theme of the menu (game::MENU_THEME_SOUND, the one --menu-theme names)
    // and the three pieces of a round (game/RoundMusic.hpp).
    std::vector<audio::SoundFile> soundFiles(FIXED_SOUND_COUNT);
    for (std::size_t i = 0; i < SOUND_CUE_COUNT; ++i) {
        const auto cue = static_cast<SoundCue>(i);
        soundFiles.at(i) = {.path = core::assetPath(soundCueFile(cue)),
                            .group = soundCueIsAmbient(cue) ? audio::SoundGroup::Ambient
                                                            : audio::SoundGroup::Effects};
    }
    const char* const themeFile =
        options.menuTheme == MenuTheme::V2 ? MENU_THEME_V2_FILE : MENU_THEME_FILE;
    soundFiles.at(MENU_THEME_SOUND) = {.path = core::assetPath(themeFile),
                                       .group = audio::SoundGroup::Music};
    core::logInfo(std::string("Music: the theme of the menu is ") + themeFile);
    // ponytail: decoded into memory like the rest, about 23 MB per minute of stereo:
    // 49.9 MB for the bed (48 s), the chase (52 s) and the sting (30 s) together. Stream
    // them if more music comes (docs/decisions/round-music.md).
    soundFiles.at(MAZE_AMBIENT_SOUND) = {.path = core::assetPath(MAZE_AMBIENT_FILE),
                                         .group = audio::SoundGroup::Music};
    soundFiles.at(SHADE_CHASE_SOUND) = {.path = core::assetPath(SHADE_CHASE_FILE),
                                        .group = audio::SoundGroup::Music};
    soundFiles.at(NIGHT_END_SOUND) = {.path = core::assetPath(NIGHT_END_FILE),
                                      .group = audio::SoundGroup::Music};
    // Then the music and the spoken lines of the storybook intro, when it is to be told
    // (loadStorybook). They are loaded with the rest, because load replaces the list.
    // ponytail: every sound is decoded into memory at the start, the music of the
    // storybook too (about 23 MB per minute of stereo). Stream it if it gets long.
    if (options.introStyle == IntroStyle::Storybook) {
        m_storybook = loadStorybook();
    }
    if (m_storybook) {
        const std::filesystem::path folder = core::assetPath(STORYBOOK_FOLDER);
        const auto addSound = [&soundFiles, &folder](const std::string& file,
                                                     audio::SoundGroup group) {
            if (file.empty()) {
                return NO_STORYBOOK_SOUND;
            }
            soundFiles.push_back({.path = folder / file, .group = group});
            return soundFiles.size() - 1;
        };
        m_storybookMusicSound = addSound(m_storybook->music, audio::SoundGroup::Music);
        // The spoken lines go with the effects: they tell something, like the cues.
        for (const StorybookBeat& beat : m_storybook->beats) {
            m_storybookVoiceSounds.push_back(addSound(beat.voice, audio::SoundGroup::Effects));
        }
    }
    m_audio.load(soundFiles);
    // A picture stays at least until its spoken line has ended. Without sound the
    // lines have no length, and the pictures keep their own times.
    if (m_storybook) {
        for (std::size_t i = 0; i < m_storybook->beats.size(); ++i) {
            const std::size_t voice = m_storybookVoiceSounds.at(i);
            m_storybookBeatSeconds.push_back(storybookBeatSeconds(
                m_storybook->beats.at(i),
                voice == NO_STORYBOOK_SOUND ? 0.0F : m_audio.lengthSeconds(voice)));
        }
    }

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
    // An intro of the command line begins here; the other ways in begin it themselves.
    if (m_mode == GameMode::Intro) {
        beginIntroClock();
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
    updateRoundMusic(windowFocused);
}

void NightMazeApp::updateRoundMusic(bool windowFocused) {
    // The bed and the chase layer: loops like the theme, said in every frame. Each goes
    // on from where it stopped, so a pause does not start the piece again.
    const RoundMusicRequest request{.mode = m_mode,
                                    .windowFocused = windowFocused,
                                    .menuCamera = m_menuCamera.enabled,
                                    .catching = m_catchSeconds >= 0.0F};
    const bool bed = mazeAmbientPlays(request, m_round);
    const bool chase = shadeChasePlays(request, m_round);
    m_audio.setLoop(MAZE_AMBIENT_SOUND, bed,
                    bed ? MAZE_AMBIENT_FADE_IN_SECONDS : MAZE_AMBIENT_FADE_OUT_SECONDS);
    m_audio.setLoop(SHADE_CHASE_SOUND, chase,
                    chase ? SHADE_CHASE_FADE_IN_SECONDS : SHADE_CHASE_FADE_OUT_SECONDS);
    // The bed steps aside under the chase, a little per frame.
    m_ambientDuck =
        advanceAmbientDuck(m_ambientDuck, chase, static_cast<float>(time().deltaSeconds()));
    m_audio.setLoopVolume(MAZE_AMBIENT_SOUND, m_ambientDuck);
    if (bed != m_mazeAmbientOn) {
        m_mazeAmbientOn = bed;
        core::logInfo(bed ? "Music: the bed of the round fades in"
                          : "Music: the bed of the round fades out");
    }
    if (chase != m_shadeChaseOn) {
        m_shadeChaseOn = chase;
        core::logInfo(chase ? "Music: the shade hunts, the chase layer fades in"
                            : "Music: the hunt is over, the chase layer fades out");
    }

    // The sting of a won night: once, a moment after the win, and faded when the screens
    // after the win are left (fadeNightEnd).
    if (advanceNightEndSting(m_nightEndSting, m_mode, static_cast<float>(time().deltaSeconds()))) {
        m_audio.play(NIGHT_END_SOUND);
        m_nightEndOn = true;
        core::logInfo("Music: the night is won, the sting plays");
    }
    if (!nightEndScreen(m_mode)) {
        fadeNightEnd();
    }
}

void NightMazeApp::fadeNightEnd() {
    if (m_nightEndOn) {
        m_nightEndOn = false;
        m_audio.fadeOut(NIGHT_END_SOUND, NIGHT_END_FADE_OUT_SECONDS);
        core::logInfo("Music: the sting fades out");
    }
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

} // namespace game
