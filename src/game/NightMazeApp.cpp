// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#include "game/NightMazeApp.hpp"

#include "assets/ImageLoader.hpp"
#include "core/Files.hpp"
#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "game/Crystals.hpp"
#include "game/Interactables.hpp"
#include "game/Puddles.hpp"
#include "game/Shade.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "scene/Raycast.hpp"
#include "scene/Transform.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <optional>
#include <random>
#include <span>
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

// The names of the uniforms (MODEL_UNIFORM, VIEW_UNIFORM, PROJECTION_UNIFORM and the
// others) are in game/ShaderUniforms.hpp, shared with the classes that draw the maze.

// Colours of the collision lines (red, green, blue): the boxes of the maze in yellow,
// the box and the reach of the player in green, the box of the gate in orange, the
// pickup spheres of the crystals in cyan and the exit zone in magenta.
constexpr glm::vec3 MAZE_COLLIDER_COLOR{1.0F, 0.85F, 0.1F};
constexpr glm::vec3 PLAYER_COLLIDER_COLOR{0.2F, 1.0F, 0.4F};
constexpr glm::vec3 GATE_COLLIDER_COLOR{1.0F, 0.45F, 0.1F};
constexpr glm::vec3 PICKUP_COLLIDER_COLOR{0.2F, 0.9F, 1.0F};
constexpr glm::vec3 EXIT_ZONE_COLOR{1.0F, 0.3F, 0.9F};

// Colours of the debug view of the picking: the pick boxes of the levers in red and of
// the notes in white, the picking ray and the box it hit in bright green, and a ray that
// hit nothing in grey.
constexpr glm::vec3 LEVER_PICK_COLOR{1.0F, 0.35F, 0.25F};
constexpr glm::vec3 NOTE_PICK_COLOR{0.95F, 0.95F, 0.85F};
constexpr glm::vec3 PICK_HIT_COLOR{0.3F, 1.0F, 0.3F};
constexpr glm::vec3 PICK_MISS_COLOR{0.6F, 0.6F, 0.6F};

// Radius of the small sphere that marks the end of the drawn picking ray, in metres.
constexpr float PICK_MARKER_RADIUS = 0.04F;

// Key that switches between walking and noclip (free flight).
constexpr int NOCLIP_KEY = GLFW_KEY_N;

// Key that switches the flashlight on and off.
constexpr int FLASHLIGHT_KEY = GLFW_KEY_F;

// Key that starts the round again on the same maze.
constexpr int RESTART_KEY = GLFW_KEY_R;

// Key that shows the map for as long as it is held.
constexpr int MINIMAP_KEY = GLFW_KEY_M;

// Key that uses what the picking ray points at: pulls a lever, reads a note, closes the
// card of a note. A left click does the same.
constexpr int INTERACT_KEY = GLFW_KEY_E;

// Key that switches the menu camera on and off: the game then shows itself.
constexpr int MENU_CAMERA_KEY = GLFW_KEY_F2;

// The documents of the menus, relative to the assets directory.
constexpr const char* MAIN_MENU_DOCUMENT_FILE = "ui/main_menu.rml";
constexpr const char* PAUSE_DOCUMENT_FILE = "ui/pause.rml";
constexpr const char* ROUND_END_DOCUMENT_FILE = "ui/round_end.rml";
constexpr const char* SETTINGS_DOCUMENT_FILE = "ui/settings.rml";
// The card of text the intro is told with. It has no buttons.
constexpr const char* CARD_DOCUMENT_FILE = "ui/card.rml";

// The elements of the documents the code writes into or reads: their id attributes.
// The result screen and the pause menu use the same four names.
constexpr const char* TIME_ID = "time";
constexpr const char* CRYSTALS_ID = "crystals";
constexpr const char* DIFFICULTY_ID = "difficulty";
constexpr const char* SEED_ID = "seed";
// The main menu: the hint next to the seed field, the three numbers of the info block
// and the three difficulty buttons, whose ids are this prefix and the key of a level.
constexpr const char* SEED_HINT_ID = "seed-hint";
constexpr const char* INFO_MAZE_ID = "info-maze";
constexpr const char* INFO_CRYSTALS_ID = "info-crystals";
constexpr const char* INFO_BATTERY_ID = "info-battery";
constexpr const char* DIFFICULTY_ID_PREFIX = "difficulty-";
// The switch "Calm night" of the main menu.
constexpr const char* CALM_NIGHT_ID = "calm-night";
// The settings screen: the two sliders (their ids are the names of their settings), the
// text next to each control and the row of the window size.
constexpr const char* TEXT_ID_SUFFIX = "-text";
constexpr const char* FULLSCREEN_ID = "fullscreen";
constexpr const char* WINDOW_SIZE_ID = "window-size";
constexpr const char* WINDOW_SIZE_ROW_ID = "window-size-row";
// The card: the black over the picture, the card with its two lines, and the hint.
constexpr const char* CARD_BLACK_ID = "black";
constexpr const char* CARD_ID = "card";
constexpr const char* CARD_FIRST_LINE_ID = "line-first";
constexpr const char* CARD_SECOND_LINE_ID = "line-second";
constexpr const char* CARD_HINT_ID = "hint";
// The main menu after the intro: the document itself (a name RmlUi knows, see
// ui::UiLayer::setClass) and the black it comes out of.
constexpr const char* DOCUMENT_ID = "#document";
constexpr const char* CURTAIN_ID = "curtain";

// The classes the code sets on elements. The style sheet says what they look like.
constexpr const char* CHOSEN_CLASS = "chosen";
constexpr const char* ON_CLASS = "on";
constexpr const char* OFF_CLASS = "off";
// The main menu after the intro: no fade of the whole document, and the curtain is
// there.
constexpr const char* CUT_CLASS = "cut";
constexpr const char* DRAWN_CLASS = "drawn";

// The buttons that do not change the screen: their data-action names. A difficulty
// button is named by the same prefix as its id.
constexpr const char* NEW_SEED_ACTION = "new-seed";
constexpr const char* TOGGLE_FULLSCREEN_ACTION = "toggle-fullscreen";
constexpr const char* TOGGLE_CALM_NIGHT_ACTION = "toggle-calm-night";
constexpr const char* WINDOW_SIZE_SMALLER_ACTION = "window-size-smaller";
constexpr const char* WINDOW_SIZE_LARGER_ACTION = "window-size-larger";
constexpr const char* RESET_SETTINGS_ACTION = "reset-settings";
// The button that starts a game: its seed is read from the seed field first.
constexpr const char* PLAY_ACTION = "play";

// While the volume slider of the settings screen is moved, a short click lets the
// player hear the new loudness: at most one in this many seconds. A dragged slider
// reports a new value in almost every frame, and a click per frame would be a buzz.
constexpr double VOLUME_SAMPLE_SECONDS = 0.2;

// What the hint next to the seed field says when the field cannot be read.
constexpr const char* SEED_HINT_TEXT = "Digits only, up to 4294967295";

// The name of the difficulty of a maze that no level describes: one the debug UI built.
constexpr const char* CUSTOM_DIFFICULTY_NAME = "Custom";

// A random seed of the menu is below this number: at most six digits, short enough to
// read out to a friend. A typed seed can be any number a seed can be.
constexpr std::uint32_t RANDOM_SEED_LIMIT = 1000000;

constexpr int SECONDS_PER_MINUTE = 60;
// Below this number of seconds a leading zero is written: 1:05 and not 1:5.
constexpr int TWO_DIGITS = 10;

// A time as minutes and seconds, for example "1:05". Parts of a second are cut off.
std::string timeText(float seconds) {
    const int whole = static_cast<int>(seconds);
    const int minutes = whole / SECONDS_PER_MINUTE;
    const int rest = whole % SECONDS_PER_MINUTE;
    return std::to_string(minutes) + (rest < TWO_DIGITS ? ":0" : ":") + std::to_string(rest);
}

// A seed nobody can predict, from 1 to RANDOM_SEED_LIMIT - 1. std::random_device asks
// the operating system for a random number. It only picks the seed: the maze itself is
// built by the seeded generator, so the same seed always brings the same maze back.
std::uint32_t randomSeed() {
    std::random_device device;
    return 1 + static_cast<std::uint32_t>(device()) % (RANDOM_SEED_LIMIT - 1);
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

// The request for the first maze. A game that opens with the intro (game::startMode)
// builds the maze of the intro, whatever the settings and the command line say: the
// shots of the intro were picked in that maze. The maze of the player is built when the
// intro is over. Every other start builds the maze behind the main menu, which is also
// the maze of the round --play starts in: the seed of the command line and the level of
// the settings.
MazeSettings firstMazeSettings(const StartOptions& options, const GameSettings& settings,
                               bool shadeLines) {
    if (startMode(options, settings.introSeen) == GameMode::Intro) {
        return mazeSettingsFor(INTRO_DIFFICULTY, INTRO_MAZE_SEED, settings.nextStoryLine,
                               shadeLines);
    }
    return mazeSettingsFor(settings.difficulty, options.seed, settings.nextStoryLine, shadeLines);
}

constexpr double MILLISECONDS_PER_SECOND = 1000.0;

// Pitch of a level look, in degrees: how the player looks at the start.
constexpr float LEVEL_PITCH_DEGREES = 0.0F;

// The smallest height scale of the terrain: a flat world.
constexpr float MIN_HEIGHT_SCALE = 0.0F;

// An exposure that changes nothing: the composite pass multiplies the colours by it.
constexpr float NEUTRAL_EXPOSURE = 1.0F;

// The highlight of a puddle. Water is smooth: its highlight is as bright as the light
// that makes it (strength 1) and small and sharp (a large exponent), unlike the weak,
// wide highlight of the rough stone (LightingSettings::specularStrength and shininess).
constexpr float PUDDLE_SPECULAR_STRENGTH = 1.0F;
constexpr float PUDDLE_SHININESS = 128.0F;

// The highlight of the cloak of the shade: dull cloth, far weaker than the stone.
constexpr float SHADE_SPECULAR_STRENGTH = 0.03F;

// A puddle only mirrors the sky: all of what it shows is the reflected picture, none
// the refracted one (the uniform uReflectShare of reflect.frag).
constexpr float MIRROR_ONLY = 1.0F;

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
      // The first screen: the intro on the very first start, a round for the tools
      // that record the game, and otherwise the main menu.
      m_mode(startMode(options, m_settings.introSeen)),
      // The game "Play" starts: the difficulty of the settings, and the seed of the
      // command line when one was named there. Otherwise the menu rolls one (below).
      m_newGame{.difficulty = m_settings.difficulty, .seed = options.seed},
      m_startSeed(options.seed),
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
    uploadGround();
    beginRound();
    m_menuCameraPath = buildMenuCameraPath(m_mazeWorld);

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

    // The documents of the four screens with a menu. They are loaded once and stay
    // hidden until their screen comes up. An error is in the log (ui::UiLayer).
    m_mainMenuDocument = m_ui.loadDocument(MAIN_MENU_DOCUMENT_FILE);
    m_pauseDocument = m_ui.loadDocument(PAUSE_DOCUMENT_FILE);
    m_roundEndDocument = m_ui.loadDocument(ROUND_END_DOCUMENT_FILE);
    m_settingsDocument = m_ui.loadDocument(SETTINGS_DOCUMENT_FILE);
    m_menusLoaded = m_mainMenuDocument != ui::NO_DOCUMENT && m_pauseDocument != ui::NO_DOCUMENT &&
                    m_roundEndDocument != ui::NO_DOCUMENT && m_settingsDocument != ui::NO_DOCUMENT;
    // The card of the intro: a fifth document, not one of the menus.
    m_cardDocument = m_ui.loadDocument(CARD_DOCUMENT_FILE);
    if (!m_menusLoaded) {
        core::logError("The menus cannot be shown: the game starts straight in a round");
        m_mode = GameMode::Playing;
    }
    // The settings screen is filled once now, while it is hidden. Its switch slides
    // when its class changes: filled only when the screen comes up, a fullscreen that
    // was saved as on would slide from off to on before the eyes of the player.
    fillSettingsDocument();

    // The sounds: one file per cue, in the order of the enum, so the number of a cue is
    // the number of its sound (game::soundCueIndex). A missing file is in the log and
    // its cue is silent (audio::AudioEngine).
    std::array<std::filesystem::path, SOUND_CUE_COUNT> soundFiles;
    for (std::size_t i = 0; i < SOUND_CUE_COUNT; ++i) {
        soundFiles.at(i) = core::assetPath(soundCueFile(static_cast<SoundCue>(i)));
    }
    m_audio.load(soundFiles);

    // The seed the main menu offers for the first game: the one of the command line, or
    // a random one.
    if (options.seedGiven) {
        m_ui.setValue(m_mainMenuDocument, SEED_ID, std::to_string(m_newGame.seed));
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
// stops it on purpose: every cue is a one shot of under two seconds, so the longest
// thing that can be heard over a menu is the tail of the gate, and stopping sounds for
// the pause would need a way to go on with them afterwards. No new cue OF THE ROUND
// starts under a menu: those come from onUpdate, which returns early there, and the
// keys of the round are not read (roundInput in onRender). The one cue a menu itself
// plays is the sample click of the volume slider on the settings screen, which is
// there to be heard (handleControlChanges).
void NightMazeApp::playCue(SoundCue cue) {
    m_audio.play(soundCueIndex(cue));
    m_lastCueName = soundCueName(cue);
    ++m_cuesPlayed;
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
    // arrive here.
    if (before == GameMode::Intro) {
        leaveIntro();
    }
    m_menuAfterIntro = before == GameMode::Intro;
    if (newGame) {
        // "New maze" on the result screen: the same difficulty, another seed. "Play"
        // in the main menu starts the seed its field shows (handleMenuActions has read
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
    // Back in the main menu after a game: the next game gets a fresh seed. Coming back
    // from the settings the seed stays, with whatever was typed into its field. After
    // the intro it stays too: that is the first seed the menu offers, and it may be the
    // one of the command line.
    if (m_mode == GameMode::MainMenu && !wasSettings && before != GameMode::Intro) {
        rollSeed();
    }

    if (m_mode == GameMode::Quitting) {
        // The main loop ends after this frame (core::Application::run).
        window().requestClose();
    }
    showScreen();
}

void NightMazeApp::handleMenuActions() {
    for (const std::string& action : m_ui.takeActions()) {
        GameEvent event = GameEvent::Escape;
        if (eventForAction(action, event)) {
            // Play in the main menu starts the seed of the seed field. A field that
            // cannot be read keeps the menu open.
            if (action == PLAY_ACTION && m_mode == GameMode::MainMenu && !readSeedField()) {
                continue;
            }
            handleGameEvent(event);
        } else if (!handleMenuCommand(action)) {
            core::logWarn("A menu button has an unknown action: " + action);
        }
    }
}

bool NightMazeApp::handleMenuCommand(const std::string& action) {
    // The three difficulty buttons of the main menu: "difficulty-" and the key of
    // a level. The choice is a setting too, so the next start of the game shows it.
    const std::string difficultyPrefix = DIFFICULTY_ID_PREFIX;
    if (action.starts_with(difficultyPrefix)) {
        if (!difficultyFromKey(action.substr(difficultyPrefix.size()), m_newGame.difficulty)) {
            return false;
        }
        m_settings.difficulty = m_newGame.difficulty;
        fillMainMenuDocument();
        return true;
    }
    if (action == NEW_SEED_ACTION) {
        rollSeed();
        return true;
    }
    // The switch "Calm night" of the main menu. It is a setting like the difficulty: the
    // next game uses it, and it is written to the file when that game starts.
    if (action == TOGGLE_CALM_NIGHT_ACTION) {
        m_settings.calmNight = !m_settings.calmNight;
        fillMainMenuDocument();
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
    if (action == RESET_SETTINGS_ACTION) {
        // Everything this screen shows goes back to its default. The difficulty and the
        // calm night are chosen in the main menu and stay, and so does the story line
        // counter.
        const Difficulty difficulty = m_settings.difficulty;
        const int nextStoryLine = m_settings.nextStoryLine;
        const bool calmNight = m_settings.calmNight;
        // That the intro was seen is no setting of this screen either: a reset does not
        // bring the intro back.
        const bool introSeen = m_settings.introSeen;
        m_settings = GameSettings{};
        m_settings.difficulty = difficulty;
        m_settings.nextStoryLine = nextStoryLine;
        m_settings.calmNight = calmNight;
        m_settings.introSeen = introSeen;
        applyViewSettings();
        applyWindowSettings();
        applyAudioSettings();
        fillSettingsDocument();
        return true;
    }
    return false;
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
        const bool volumeChanged = changed.masterVolume != m_settings.masterVolume;
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
        // Only the numbers next to the sliders: the sliders themselves already stand
        // where the player put them.
        m_ui.setText(m_settingsDocument, std::string(MOUSE_SENSITIVITY_SETTING) + TEXT_ID_SUFFIX,
                     mouseSensitivityLabel(m_settings.mouseSensitivity));
        m_ui.setText(m_settingsDocument, std::string(FIELD_OF_VIEW_SETTING) + TEXT_ID_SUFFIX,
                     fieldOfViewLabel(m_settings.fieldOfViewDegrees));
        m_ui.setText(m_settingsDocument, std::string(MASTER_VOLUME_SETTING) + TEXT_ID_SUFFIX,
                     masterVolumeLabel(m_settings.masterVolume));
    }
}

bool NightMazeApp::readSeedField() {
    const std::string text = m_ui.value(m_mainMenuDocument, SEED_ID);
    if (text.empty()) {
        // Nothing typed: any maze will do.
        rollSeed();
        return true;
    }
    if (!parseSeed(text, m_newGame.seed)) {
        m_ui.setText(m_mainMenuDocument, SEED_HINT_ID, SEED_HINT_TEXT);
        return false;
    }
    m_ui.setText(m_mainMenuDocument, SEED_HINT_ID, "");
    return true;
}

void NightMazeApp::rollSeed() {
    m_newGame.seed = randomSeed();
    m_ui.setValue(m_mainMenuDocument, SEED_ID, std::to_string(m_newGame.seed));
    m_ui.setText(m_mainMenuDocument, SEED_HINT_ID, "");
}

void NightMazeApp::showScreen() {
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
    } else if (m_mode == GameMode::Intro) {
        // The card is not filled here: updateIntro writes it in every frame.
        document = m_cardDocument;
    }
    m_ui.show(document);

    // The cursor follows the screen: captured for mouse look while a round is played,
    // free for the buttons of a menu. The menu camera does not turn with the mouse, so
    // it leaves the cursor free too. The intro captures it for another reason: a film
    // has no cursor in its picture, and there is nothing to click.
    input().setCursorCaptured((updatesRound(m_mode) && !m_menuCamera.enabled) ||
                              m_mode == GameMode::Intro);
}

void NightMazeApp::startIntro() {
    // Not without the card, not twice and not while the program is closing.
    if (m_cardDocument == ui::NO_DOCUMENT || m_mode == GameMode::Intro ||
        m_mode == GameMode::Quitting) {
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

void NightMazeApp::updateIntro() {
    // Any key and any mouse button skip the whole intro. Escape is one of them, and it
    // also arrives as an event of its own before this function runs (onEscapePressed).
    // wasKeyPressed is true for one frame, and this function runs once per frame.
    if (introFrame(static_cast<float>(m_introSeconds)).skippable) {
        bool pressed = false;
        // GLFW numbers its keys from GLFW_KEY_SPACE to GLFW_KEY_LAST, with gaps that
        // are never pressed, and its mouse buttons from 0.
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST && !pressed; ++key) {
            pressed = input().wasKeyPressed(key);
        }
        for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST && !pressed; ++button) {
            pressed = input().wasMouseButtonPressed(button);
        }
        if (pressed) {
            handleGameEvent(GameEvent::IntroFinished);
            return;
        }
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
        const IntroLines lines = introLines(frame.card, shadeInGame());
        m_ui.setText(m_cardDocument, CARD_FIRST_LINE_ID, lines.first);
        m_ui.setText(m_cardDocument, CARD_SECOND_LINE_ID, lines.second);
    }
    m_ui.setOpacity(m_cardDocument, CARD_BLACK_ID, frame.blackOpacity);
    m_ui.setOpacity(m_cardDocument, CARD_ID, frame.textOpacity);
    m_ui.setOpacity(m_cardDocument, CARD_HINT_ID, frame.hintOpacity);
}

void NightMazeApp::leaveIntro() {
    // The wind is one sound of half a minute: without this it would go on over the
    // main menu after a skip.
    m_audio.stopAll();
    // Seen, to its end or to the key that skipped it: the next start opens with the
    // main menu. An intro that could not be shown at all was not seen.
    if (m_cardDocument != ui::NO_DOCUMENT) {
        m_settings.introSeen = true;
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

void NightMazeApp::startNewGame(const NewGame& newGame) {
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
    // overwrites what the debug UI may have switched.
    m_gameplay.shade.enabled = shadeInGame();

    // The maze is always built again, also for the seed that is in play: the level may
    // be another one, and the numbers of levers and notes may have been changed.
    m_mazeSettings.seed = newGame.seed;
    m_mazeSettings.regenerate = false;
    regenerateMaze();
    m_playedDifficultyName = level.name;

    // The difficulty that was just played is the one the main menu starts with next
    // time, so it is written to the settings file now.
    saveSettings();
}

void NightMazeApp::fillMainMenuDocument() {
    // The chosen one of the three difficulty buttons carries a class.
    for (const Difficulty difficulty : ALL_DIFFICULTIES) {
        m_ui.setClass(m_mainMenuDocument,
                      std::string(DIFFICULTY_ID_PREFIX) + difficultyLevel(difficulty).key,
                      CHOSEN_CLASS, difficulty == m_newGame.difficulty);
    }

    // The switch "Calm night": a class moves its knob.
    m_ui.setClass(m_mainMenuDocument, CALM_NIGHT_ID, ON_CLASS, m_settings.calmNight);

    // The info block: what the chosen level means in numbers.
    const DifficultyLevel& level = difficultyLevel(m_newGame.difficulty);
    m_ui.setText(m_mainMenuDocument, INFO_MAZE_ID,
                 std::to_string(level.mazeWidth) + " x " + std::to_string(level.mazeHeight));
    m_ui.setText(m_mainMenuDocument, INFO_CRYSTALS_ID,
                 std::to_string(requiredCrystalCount(level.crystalCount, level.requiredFraction)) +
                     " of " + std::to_string(level.crystalCount));
    m_ui.setText(m_mainMenuDocument, INFO_BATTERY_ID, timeText(level.batteryLifetimeSeconds));

    // The menu that follows the intro comes in out of black. Every other time the two
    // classes are taken away, and the menu fades in like every screen.
    m_ui.setClass(m_mainMenuDocument, DOCUMENT_ID, CUT_CLASS, m_menuAfterIntro);
    m_ui.setClass(m_mainMenuDocument, CURTAIN_ID, DRAWN_CLASS, m_menuAfterIntro);
}

void NightMazeApp::fillPauseDocument() {
    m_ui.setText(m_pauseDocument, DIFFICULTY_ID, m_playedDifficultyName);
    m_ui.setText(m_pauseDocument, SEED_ID, std::to_string(m_mazeWorld.seed));
}

void NightMazeApp::fillRoundEndDocument() {
    m_ui.setText(m_roundEndDocument, TIME_ID, timeText(m_round.elapsedSeconds));
    m_ui.setText(m_roundEndDocument, CRYSTALS_ID,
                 std::to_string(m_round.collectedCount) + " of " +
                     std::to_string(m_round.crystals.size()));
    // The difficulty and the seed together name the maze: with both, a friend plays
    // the same one.
    m_ui.setText(m_roundEndDocument, DIFFICULTY_ID, m_playedDifficultyName);
    m_ui.setText(m_roundEndDocument, SEED_ID, std::to_string(m_mazeWorld.seed));
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
}

void NightMazeApp::applyViewSettings() {
    m_mouseSensitivity = mouseDegreesPerUnit(m_settings.mouseSensitivity);
    m_camera.fovDegrees = m_settings.fieldOfViewDegrees;
}

void NightMazeApp::applyAudioSettings() {
    m_audio.setMasterVolume(masterVolumeGain(m_settings.masterVolume));
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

void NightMazeApp::regenerateMaze() {
    // generateMaze throws for a size outside 1 to Maze::MAX_SIZE. The request comes from
    // a panel, where any number can be typed, so it is brought into the range here and
    // written back for the panel to show.
    m_mazeSettings.width = std::clamp(m_mazeSettings.width, 1, Maze::MAX_SIZE);
    m_mazeSettings.height = std::clamp(m_mazeSettings.height, 1, Maze::MAX_SIZE);
    // The same for the wanted numbers of levers and notes. placeInteractables would
    // bring them into the range itself, here they are written back for the panel.
    InteractableSettings& interactables = m_mazeSettings.interactables;
    interactables.leverCount = std::clamp(interactables.leverCount, 0, MAX_LEVER_COUNT);
    interactables.noteCount = std::clamp(interactables.noteCount, 0, MAX_NOTE_COUNT);
    // Every maze starts its story notes at the next unread line, also a maze built again
    // for the same seed (the counter only moves when a maze is finished).
    interactables.firstStoryLine = m_settings.nextStoryLine;
    // A maze without a shade does not talk about one.
    interactables.shadeLines = shadeInGame();

    // The height scale can be typed into its slider too.
    m_terrainSettings.heightScale =
        std::clamp(m_terrainSettings.heightScale, MIN_HEIGHT_SCALE, MAX_HEIGHT_SCALE);

    // Replaces the maze, its terrain, the model matrices, the collision boxes, the exit,
    // the crystals, the levers and the notes in one assignment. A new maze is a new
    // round.
    m_mazeWorld = buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed,
                                 m_heightmap, m_terrainSettings.heightScale, interactables,
                                 m_mazeSettings.crystalCount);
    // Until a new game says otherwise (startNewGame), this is a maze of no level.
    m_playedDifficultyName = CUSTOM_DIFFICULTY_NAME;
    uploadGround();
    beginRound();
    // The menu camera walks the corridors of the maze, so a new maze is a new path.
    m_menuCameraPath = buildMenuCameraPath(m_mazeWorld);
}

void NightMazeApp::rebuildTerrain() {
    m_terrainSettings.heightScale =
        std::clamp(m_terrainSettings.heightScale, MIN_HEIGHT_SCALE, MAX_HEIGHT_SCALE);

    // The world: a new terrain, and the walls, the gate, the start, the exit, the levers
    // and the notes on it.
    placeOnTerrain(m_mazeWorld, m_heightmap, m_terrainSettings.heightScale);

    // What copied heights out of the world. The crystals keep their state (collected or
    // not), only their resting places move. The obstacle list is a copy of the boxes.
    restCrystalsOnGround(m_round, m_mazeWorld);
    m_obstacles = roundObstacles(m_mazeWorld, m_round);

    // A walking player stands on the new ground at once, in both positions, so the next
    // frame is not drawn from a point between the old height and the new one. A flying
    // player is left where it is.
    if (!m_player.noclip) {
        m_player.position.y =
            m_mazeWorld.terrain.heightAt(m_player.position.x, m_player.position.z);
        m_previousPlayerPosition.y = m_player.position.y;
        m_camera.position = m_player.eyePosition();
    }

    uploadGround();
    // The path of the menu camera lies on the ground, so it moves up or down with it.
    m_menuCameraPath = buildMenuCameraPath(m_mazeWorld);
}

void NightMazeApp::updateMenuCameraSwitch() {
    // wasKeyPressed is true for one frame, so the key is read once per frame. Like the
    // other keys of a round it works only while a round is played, not under a menu.
    if (updatesRound(m_mode) && input().wasKeyPressed(MENU_CAMERA_KEY)) {
        m_menuCamera.enabled = !m_menuCamera.enabled;
    }

    // The numbers can be typed into the sliders of the debug UI, so they are brought
    // into their ranges here.
    m_menuCamera.speed =
        std::clamp(m_menuCamera.speed, MIN_MENU_CAMERA_SPEED, MAX_MENU_CAMERA_SPEED);
    m_menuCamera.eyeHeight =
        std::clamp(m_menuCamera.eyeHeight, MIN_MENU_CAMERA_EYE_HEIGHT, MAX_MENU_CAMERA_EYE_HEIGHT);

    if (m_menuCamera.enabled == m_menuCameraWasEnabled) {
        return;
    }
    m_menuCameraWasEnabled = m_menuCamera.enabled;
    if (m_menuCamera.enabled) {
        // Every run of the menu camera starts at the beginning of its shot, so the same
        // maze and settings always show the same pictures.
        m_menuCameraSeconds = 0.0;
        // For whoever records the picture: after this many seconds it repeats.
        core::logInfo("Menu camera on, one loop takes " +
                      std::to_string(static_cast<int>(menuCameraLoopSeconds())) + " s");
    }
    // The cursor: free while the menu camera runs (the mouse does not turn it), and
    // captured again for the round when it is switched off.
    input().setCursorCaptured(updatesRound(m_mode) && !m_menuCamera.enabled);
}

void NightMazeApp::uploadGround() {
    // The triangles are built on the CPU (plain data, covered by tests) and copied to
    // the graphics card in one piece.
    m_terrainRenderer.upload(buildTerrainMesh(m_mazeWorld.terrain));
    // The grass stands on the terrain, so new ground means new places for it.
    plantGrass();
    // The puddles lie on the terrain too: new ground means new water levels, and a new
    // maze new cells.
    layPuddles();
}

void NightMazeApp::layPuddles() {
    m_environment.puddleShare = std::clamp(m_environment.puddleShare, 0.0F, MAX_PUDDLE_SHARE);
    const std::vector<Puddle> puddles = puddlesOnGround(m_mazeWorld, m_environment.puddleShare);
    m_puddleRenderer.upload(m_mazeWorld.terrain, puddles);
}

void NightMazeApp::plantGrass() {
    m_grassSettings.density = std::clamp(m_grassSettings.density, 0.0F, MAX_GRASS_DENSITY);
    const std::vector<GrassTuft> tufts = placeGrass(m_mazeWorld, m_grassSettings.density);
    m_grassRenderer.upload(tufts);
}

void NightMazeApp::beginRound() {
    // The state of the round: every crystal and every flask back, a full battery, the
    // gate closed, no lever pulled and so every wall back in the obstacle list.
    m_round = startRound(m_mazeWorld, m_gameplay);
    m_obstacles = roundObstacles(m_mazeWorld, m_round);
    // The picking of the round before may name a lever the new maze does not have.
    m_pick = pickNothing(m_round);
    m_shownPick = m_pick;
    // A round starts with the light on, also after one that ended in the dark.
    m_lighting.flashlightOn = true;
    // The low battery pulse of the round before must not go on in this one. The cues of
    // a step need no reset: onUpdate compares every step with the moment right before
    // it, so a new round, also one that starts with its gate open, fires nothing.
    m_lowBatteryPulse = {};
    // The same for the breathing, and the stamina it follows: a round starts with a full
    // bar and a player who is not winded, and the tea of a flask of the round before
    // works no longer. Only the state is new. The numbers of the rule
    // (Player::staminaSettings), which the debug UI may have changed, stay.
    m_windedBreath = {};
    m_player.stamina = {};
    // The hum of the shade starts anew as well. The shade itself is part of the round:
    // startRound has put it back in its start cell, with its grace time ahead of it.
    m_shadeHum = {};

    // The player goes to the start, feet on the ground there. After a regeneration the
    // old position may be inside a wall of the new maze, or outside of it.
    m_player.position = m_mazeWorld.startPosition;
    // Both positions at once: otherwise the next frame would be drawn from a point
    // between the old place and the new one, a visible swoop through the walls.
    m_previousPlayerPosition = m_player.position;

    m_camera.position = m_player.eyePosition();
    m_camera.yawDegrees = m_mazeWorld.startYawDegrees;
    m_camera.pitchDegrees = LEVEL_PITCH_DEGREES;
}

void NightMazeApp::carryPlayerBack() {
    // The same call the restart key makes: everything of the round is as it was at its
    // start (crystals, battery, levers, discovery, stamina, flasks, the shade).
    beginRound();
    // The next line of the five, so the same one never shows twice in a row.
    m_lastCaughtLine = nextCaughtLine(m_lastCaughtLine);
    showCaughtLine(m_round, m_lastCaughtLine);
    playCue(SoundCue::Caught);
}

void NightMazeApp::onUpdate(double fixedDt) {
    // Remember where the player was before this step. It is done in every step, also
    // when the player does not move, so that onRender never blends with an old position.
    m_previousPlayerPosition = m_player.position;

    // The round runs only while it is played (game::updatesRound). Under a menu, and
    // while the menu camera runs, it stands still: the player does not move, the
    // battery does not drain, nothing is collected and the time of the round does not
    // count. Only the animation clock goes on, so the crystals keep bobbing and their
    // lights keep pulsing in the picture. The pause menu stops that clock too.
    if (!updatesRound(m_mode) || m_menuCamera.enabled) {
        if (animatesScene(m_mode)) {
            m_round.animationSeconds += static_cast<float>(fixedDt);
        }
        return;
    }

    // The keys reach the player only while the cursor is captured. It is captured when
    // a round starts or goes on (showScreen). Showing the debug panels gives it back, and
    // one click in the scene then switches on both mouse look and movement again.
    // Without the capture the struct stays as it is created: nothing is held.
    PlayerInput wanted;
    if (input().isCursorCaptured()) {
        wanted.forward = input().isKeyDown(GLFW_KEY_W);
        wanted.backward = input().isKeyDown(GLFW_KEY_S);
        wanted.left = input().isKeyDown(GLFW_KEY_A);
        wanted.right = input().isKeyDown(GLFW_KEY_D);
        wanted.up = input().isKeyDown(GLFW_KEY_SPACE);
        // Left Shift has one meaning per mode: sprint when walking, down when flying.
        // The player uses the field that belongs to its mode and ignores the other.
        // Whether the sprint really happens is decided by the stamina (Player::update).
        wanted.down = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
        wanted.sprint = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
    }
    // A player who reads the map stands still: the keys are dropped for this step
    // (game::movementInput). The step below still runs, and so does the rest of the
    // round: the battery drains, the crystals bob and the stamina refills.
    wanted = movementInput(wanted, mapShown());

    // The step runs also with nothing held: it is what brings the feet back to the
    // ground after noclip was switched off in a panel. It also drains and refills the
    // stamina, so the stamina stands still wherever the round does: no step gets here
    // under a menu, in the pause, on the result screen or while the menu camera runs.
    m_player.update(wanted, m_camera.yawDegrees, m_camera.pitchDegrees, static_cast<float>(fixedDt),
                    m_obstacles, m_mazeWorld.terrain);

    // Walking changes the height all the time, because the ground is uneven, and that
    // change is blended in onRender like the movement itself: the eyes then glide over
    // the ground instead of moving up and down in steps. One change must not be
    // blended: the step right after noclip was switched off in mid-air, which drops the
    // feet to the ground. That is a jump and not a movement. Without these lines one
    // frame would be drawn from a point part of the way down.
    if (m_playerWasFlying && !m_player.noclip) {
        m_previousPlayerPosition.y = m_player.position.y;
    }
    m_playerWasFlying = m_player.noclip;

    // The camera stands where the eyes of the player are. onRender does not draw from
    // this position directly (it blends two steps), but the debug UI shows it.
    m_camera.position = m_player.eyePosition();

    // The rules of the round, with the position the player has after this step: the
    // battery, the crystals within reach, the gate and the exit. The switch of the
    // flashlight goes in by reference, because an empty battery turns it off.
    const bool gateBlockedBefore = gateBlocks(m_mazeWorld, m_round);
    // What the round looked like before the step, to hear afterwards what the step did.
    const RoundSoundSnapshot soundBefore = soundSnapshot(m_round, m_lighting.flashlightOn);
    const int flasksBefore = m_round.flasksCollected;
    updateRound(m_round, m_mazeWorld, m_gameplay, m_player.position, m_lighting.flashlightOn,
                static_cast<float>(fixedDt));
    // A flask was picked up in this step: the player drinks it at once. The round only
    // counts the flasks. What the tea does belongs to the stamina (game::drinkFlask).
    if (m_round.flasksCollected > flasksBefore) {
        drinkFlask(m_player.stamina, m_player.staminaSettings);
    }
    // The sounds of this step: a crystal, the gate, the battery that ran out
    // (game::roundStepCues), and the beat of a low battery when one is due. Both are
    // counted in fixed steps and only here, below the early return above: under a menu,
    // in the pause and while the menu camera runs no step gets this far, so nothing
    // sounds there and the clock of the pulse stands still.
    for (const SoundCue cue : roundStepCues(soundBefore, m_round, m_lighting.flashlightOn)) {
        playCue(cue);
    }
    if (advanceLowBatteryPulse(m_lowBatteryPulse, m_round, m_lighting.flashlightOn, m_gameplay,
                               static_cast<float>(fixedDt))) {
        playCue(SoundCue::LowBatteryPulse);
    }
    // The heavy breathing of a winded player: one breath, again and again on a clock
    // of the same kind, for as long as the stamina says winded.
    if (advanceWindedBreath(m_windedBreath, m_player.stamina.winded, static_cast<float>(fixedDt))) {
        playCue(SoundCue::WindedBreath);
    }
    // The gate has just opened (the only change a step can make here): its box leaves
    // the obstacle list, and the way into the exit cell is free.
    if (gateBlocks(m_mazeWorld, m_round) != gateBlockedBefore) {
        m_obstacles = roundObstacles(m_mazeWorld, m_round);
    }

    // The shade, after the rules of the round: it needs the battery of this step (an
    // empty one gives no light). The flashlight is where the hand holds it now, built
    // from the eyes of this step and the angles of the camera, the same way onRender
    // builds the one that is drawn. Like everything above it runs only in the steps of
    // a round that is played, so the shade stands still under a menu and in the pause,
    // and goes on while the map or a note card is on the screen.
    const FlashlightPose lampPose =
        flashlightPose(m_lighting, m_player.eyePosition(), m_camera.forward(), m_camera.right());
    const bool caught = updateRoundShade(m_round, m_mazeWorld, m_gameplay, m_player.position,
                                         roundShadeLamp(m_lighting, m_round, lampPose), m_obstacles,
                                         static_cast<float>(fixedDt));
    // Its hum, on a clock like the pulse: more often the nearer the shade is.
    if (advanceShadeHum(m_shadeHum, m_round, static_cast<float>(fixedDt))) {
        playCue(SoundCue::ShadeNear);
    }
    // Caught: back to the start of the same maze. A player who flies (noclip, a tool for
    // looking around) is left alone. The steps that may follow in the same frame play
    // the new round.
    if (caught && !m_player.noclip) {
        carryPlayerBack();
        return;
    }

    // This step took the player through the open gate: the result screen comes up. The
    // steps that may follow in the same frame then find the round stopped.
    if (m_round.state == RoundState::Won) {
        // The maze is finished: the story goes on after the lines this maze showed. The
        // counter is written now, once, and not at every frame (saveSettings writes only
        // when something changed). The counter is moved on from the line THIS maze started
        // with, so winning the same maze twice ("Play again") does not skip a line. The
        // maze keeps its lines until a new one is built.
        // Whether the maze skipped the lines about the shadow is asked from the request
        // it was built with, not from the settings of this moment.
        m_settings.nextStoryLine = advanceStoryLine(m_mazeSettings.interactables.firstStoryLine,
                                                    storyNoteCount(m_mazeWorld.interactables),
                                                    m_mazeSettings.interactables.shadeLines);
        saveSettings();
        handleGameEvent(GameEvent::RoundWon);
    }
}

void NightMazeApp::onRender(double alpha) {
    // The menu buttons that were clicked since the last frame, and the controls of the
    // settings screen that were moved. The clicks arrived while the events of this
    // frame were read, so the screen they lead to is drawn in this very frame.
    handleMenuActions();
    handleControlChanges();

    // The intro: the request of the debug UI to play it again, and its frame. Both come
    // before everything below, because the frame may end the intro: the rest of this
    // function then already draws the main menu.
    if (m_introRequested) {
        m_introRequested = false;
        startIntro();
    }
    if (m_mode == GameMode::Intro) {
        updateIntro();
    }

    // A player who switches to another program does not want the round to go on: the
    // frame in which the window stops being the active one pauses it (game::nextMode,
    // FocusLost). Asked once per frame and compared with the frame before, so it
    // happens once. The menu camera is a recording tool, and the program that records
    // it is the active one then: it is left alone.
    const bool windowFocused = window().isFocused();
    if (m_windowWasFocused && !windowFocused && !m_menuCamera.enabled) {
        if (updatesRound(m_mode)) {
            core::logInfo("The window is no longer the active one: the round is paused");
        }
        handleGameEvent(GameEvent::FocusLost);
    }
    m_windowWasFocused = windowFocused;

    // A new maze asked for by the debug UI is built here, at the start of a frame and
    // outside of the fixed steps, so no step ever sees a half replaced maze. When that
    // happens on the result screen, the screen is left: its numbers belong to the
    // round that is gone. Restart is the event that leads from there into the game.
    if (m_mazeSettings.regenerate) {
        m_mazeSettings.regenerate = false;
        regenerateMaze();
        if (m_mode == GameMode::RoundEnd) {
            handleGameEvent(GameEvent::Restart);
        }
    }

    // The menu camera: its key, and what has to happen when it was just switched on.
    updateMenuCameraSwitch();
    // Two questions the rest of the frame asks. roundInput: do the keys and the mouse
    // of the round work? Only while a round is played and the menu camera is off, so
    // under a menu no key of the round does anything. menuCamera: is the picture taken
    // by the menu camera? While it is switched on, and in the main menu when the live
    // scene is its background (--menu-background scene, or neither the video nor the
    // still could be loaded).
    const bool roundInput = updatesRound(m_mode) && !m_menuCamera.enabled;
    // The intro takes its pictures with the menu camera too, but it says itself which
    // shot, at which moment and with which light (game::introCamera).
    const bool intro = m_mode == GameMode::Intro;
    const bool menuCamera = m_menuCamera.enabled || usesMenuCamera(m_mode) || intro;
    // The settings the menu camera uses in this frame. The main menu always shows the
    // high glide over the maze, whatever shot the recording tool is set to.
    MenuCameraSettings menuCameraSettings = m_menuCamera;
    if (!m_menuCamera.enabled) {
        menuCameraSettings.shot = MenuShot::HighGlide;
    }
    const IntroFrame introMoment = introFrame(static_cast<float>(m_introSeconds));
    const IntroCamera introShot = introCamera(introMoment);
    if (intro) {
        menuCameraSettings = introShot.settings;
    }

    // A new round on the same maze, asked for with the restart key or by the debug UI.
    // It is started here for the same reason: between two fixed steps, never inside one.
    // wasKeyPressed is true for one frame, so the key is read once per frame.
    if (m_gameplay.restart || (roundInput && input().wasKeyPressed(RESTART_KEY))) {
        m_gameplay.restart = false;
        if (m_mode == GameMode::RoundEnd) {
            // The debug UI asked on the result screen: the button "Play again" of that
            // screen does the same, a new round and the screen left.
            handleGameEvent(GameEvent::Restart);
        } else {
            beginRound();
        }
    }

    // Every lever at once, asked for by a button of the debug UI. Handled here like the
    // restart: between two fixed steps. Open walls leave the obstacle list. One sound
    // for all of them, because the walls open in the same moment, and none under a menu
    // or the menu camera (the debug window is open there too): no cue of the round
    // sounds while the round stands still.
    if (m_gameplay.pullAllLevers) {
        m_gameplay.pullAllLevers = false;
        if (pullAllLevers(m_round, m_mazeWorld) > 0) {
            m_obstacles = roundObstacles(m_mazeWorld, m_round);
            if (roundInput) {
                playCue(SoundCue::LeverPull);
            }
        }
    }

    // A new height scale of the terrain or a new density of the grass, asked for by the
    // debug UI. Both are handled here for the same reason as a new maze. A maze that
    // was regenerated in this frame is already built with the new numbers: doing it
    // again costs a little time once and changes nothing.
    if (m_terrainSettings.rebuild) {
        m_terrainSettings.rebuild = false;
        rebuildTerrain();
    }
    if (m_grassSettings.replant) {
        m_grassSettings.replant = false;
        plantGrass();
    }
    // A new share of cells with a puddle, asked for by the debug UI in the same way.
    if (m_environment.replacePuddles) {
        m_environment.replacePuddles = false;
        layPuddles();
    }

    // The noclip key. wasKeyPressed is true for one frame, so it is read here, once per
    // frame, and not in onUpdate, which runs zero or more times per frame.
    if (roundInput && input().wasKeyPressed(NOCLIP_KEY)) {
        m_player.noclip = !m_player.noclip;
    }

    // The flashlight key, read once per frame for the same reason. Like the noclip key
    // it works whether or not the cursor is captured. With an empty battery the key
    // still sets the switch, but the next fixed step turns it off again
    // (game::updateRound), and no frame is drawn with the light of an empty battery
    // (game::lightingForFrame). The sound of the key is chosen before the switch moves:
    // a click on, a click off, or the dull click of an empty battery
    // (game::flashlightKeyCue).
    if (roundInput && input().wasKeyPressed(FLASHLIGHT_KEY)) {
        playCue(flashlightKeyCue(m_round.battery, m_lighting.flashlightOn));
        m_lighting.flashlightOn = !m_lighting.flashlightOn;
    }

    // The map: on the screen while its key is held (mapShown). Asked once here for the
    // whole frame. While it is shown the mouse does not turn the camera and nothing can
    // be used: looking at the map is a stop, and the round goes on behind it.
    const bool map = mapShown();
    m_mapOnScreen = map;

    // Mouse look. It runs here, once per frame, and not in onUpdate: a mouse delta
    // describes one frame, and onUpdate runs zero or more times per frame. It comes
    // before the view matrix is built, so the picture and the picking ray of this frame
    // already use the new angles. The click that captures a free cursor is read further
    // down (handleInteraction), because it first has to be known what the click hit.
    const bool cursorCaptured = input().isCursorCaptured();
    if (cursorCaptured && roundInput && !map) {
        // Mouse movement to the right is positive and positive yaw turns right, so x is
        // used as it is. Screen y grows downwards while pitch grows upwards, hence the
        // minus sign: moving the mouse up (negative y) looks up.
        const float yawDelta = static_cast<float>(input().mouseDeltaX()) * m_mouseSensitivity;
        const float pitchDelta = -static_cast<float>(input().mouseDeltaY()) * m_mouseSensitivity;
        m_camera.rotate(yawDelta, pitchDelta);
    }

    // Everything is measured in pixels of the framebuffer of the window, which differs
    // from the window size on Retina displays. Querying it every frame also handles
    // window resizing.
    const core::Size framebuffer = window().framebufferSize();

    // A minimized window can have a framebuffer of size 0 x 0. There is nothing to draw
    // then, and nothing to draw into: a texture of size 0 cannot be attached to
    // a framebuffer. The aspect ratio would be 0 / 0, which is NaN (not a number):
    // glm::perspective stops the program with an assert in a Debug build and returns
    // a matrix with NaN in it in a Release build. So the whole frame is skipped. The
    // scene framebuffer keeps its last size and is used again when the window is back.
    if (framebuffer.width == 0 || framebuffer.height == 0) {
        return;
    }

    // The main menu (and the settings opened from it) with its video or its still
    // picture: that background covers the whole window, so nothing of the scene could
    // be seen and none of it is drawn (game::drawsScene). The frame is the picture and
    // the menu on top, and it ends here: no shadow maps, no scene, no bloom, no
    // composite pass, no minimap. The video moves on by the real time of the frame, and
    // only here, so it stands still while a round is played. update comes before the
    // question: a video whose decoder gave up is replaced by the still or by the live
    // scene there, and the live scene is drawn below like any other frame.
    if (usesMenuCamera(m_mode)) {
        m_menuBackground.update(time().deltaSeconds());
    }
    if (!drawsScene(m_mode, coversWindow(m_menuBackground.background()))) {
        // Nothing is picked under a menu, as in the frames that draw the scene.
        m_pick = pickNothing(m_round);
        // The picture goes straight into the window, like the composite pass does.
        gfx::Framebuffer::bindDefault(framebuffer.width, framebuffer.height);
        m_menuBackground.draw(framebuffer);
        m_ui.draw(framebuffer);
        return;
    }

    // The simulation moves the player in fixed steps, and this frame is drawn at some
    // moment between two of them: alpha (0 to 1) tells how far. Drawing from a point
    // between the position before the last step and the position after it keeps the
    // movement smooth at any frame rate, in all three directions: the height of the
    // feet follows the ground from step to step and is blended like x and z.
    // m_player.position itself is not changed. The eyes are a fixed height above the
    // feet, so blending the feet and then going up gives the same point as blending the
    // eyes.
    const glm::vec3 feet =
        glm::mix(m_previousPlayerPosition, m_player.position, static_cast<float>(alpha));
    glm::vec3 eye = feet + glm::vec3{0.0F, Player::EYE_HEIGHT, 0.0F};

    // The camera this frame is drawn with: the camera of the player, or a copy of it
    // that stands and looks where the menu camera does. A copy, so the angles of the
    // player are still there when the menu camera is switched off, and the field of
    // view and the two planes are the same in both.
    scene::Camera frameCamera = m_camera;
    if (intro) {
        // The moment of the shot comes from the script and not from the clock of the
        // menu camera, which stands still meanwhile. The field of view is the default
        // one, not the one of the settings: the shots were picked with it.
        const MenuCameraPose pose = menuCameraPose(m_menuCameraPath, m_mazeWorld,
                                                   menuCameraSettings, introShot.menuSeconds);
        eye = pose.eye;
        frameCamera.yawDegrees = pose.yawDegrees;
        frameCamera.pitchDegrees = pose.pitchDegrees;
        frameCamera.fovDegrees = DEFAULT_FIELD_OF_VIEW_DEGREES;
    } else if (menuCamera) {
        // The clock of the menu camera follows the real time of the frames, not the
        // fixed steps: the pose is a function of time, so it can be asked for the exact
        // moment of every frame and needs no blending. It is kept inside one loop of
        // the shot, so the number stays small however long the menu is open.
        m_menuCameraSeconds += time().deltaSeconds();
        const double loopSeconds =
            game::menuCameraLoopSeconds(m_menuCameraPath, m_mazeWorld, menuCameraSettings);
        if (loopSeconds > 0.0) {
            m_menuCameraSeconds = std::fmod(m_menuCameraSeconds, loopSeconds);
        }
        const MenuCameraPose pose =
            menuCameraPose(m_menuCameraPath, m_mazeWorld, menuCameraSettings,
                           static_cast<float>(m_menuCameraSeconds));
        eye = pose.eye;
        frameCamera.yawDegrees = pose.yawDegrees;
        frameCamera.pitchDegrees = pose.pitchDegrees;
    }

    // Width divided by height of the same pixels the viewport covers. The casts make it
    // a division of floats: 1280 / 720 as integers would be 1.
    const float aspectRatio =
        static_cast<float>(framebuffer.width) / static_cast<float>(framebuffer.height);

    // The two matrices that are the same for everything drawn in this frame, built from
    // the blended eye. They are needed this early for the picking ray, which has to go
    // through the picture exactly as it will be drawn.
    const glm::mat4 view = frameCamera.viewMatrix(eye);
    const glm::mat4 projection = frameCamera.projectionMatrix(aspectRatio);

    // Object picking: one ray per frame, what it hits, and then the key or the click
    // that uses it. This runs once per frame like the other keys. A lever that is
    // pulled here changes the round between two fixed steps, never inside one.
    // Under a menu and in the picture of the menu camera nothing is picked and nothing
    // is used: no lever is highlighted, and a click or the key does not reach the round.
    // The same while the map is shown: it covers the middle of the picture, so there is
    // no crosshair, no prompt, and the interaction key does nothing.
    if (!roundInput || map) {
        m_pick = pickNothing(m_round);
    } else {
        m_pick = pickForFrame(view, projection, eye, cursorCaptured);
        handleInteraction(cursorCaptured);
    }
    // The debug view draws this copy, which stands still while "freeze" is set.
    if (!m_pickDebug.freezeRay) {
        m_shownPick = m_pick;
    }

    // Where the walls stand in this frame: a wall that a lever has opened is on its way
    // into the ground. One list for the shadow passes and for the scene pass.
    m_wallMatrices = roundWallMatrices(m_mazeWorld, m_round);

    // The shade of this frame: between its place before the last fixed step and after
    // it, like the player, and turned towards the place the frame is drawn from. It is
    // part of the picture only while a round is played and the player is the one who
    // looks: not under a menu, not behind the main menu and not for the menu camera.
    m_shadeDrawn = m_mode == GameMode::Playing && !m_menuCamera.enabled && m_round.shade.present;
    if (m_shadeDrawn) {
        scene::Transform shade;
        shade.position = glm::mix(m_round.shade.previousPosition, m_round.shade.position,
                                  static_cast<float>(alpha));
        shade.rotationDegrees = {0.0F, shadeYawDegrees(shade.position, feet), 0.0F};
        m_shadeMatrix = shade.matrix();
    }
    // The one picture outside a round that has a shade in it: the card of the intro that
    // warns of it. It stands still in the cell the script names, on the ground, turned
    // towards the camera, and the flashlight of the camera is on it. It is a prop: the
    // shade of the round is not moved or asked, so nothing hums and nobody is caught.
    // A calm night has none (game::introShadeCell).
    if (intro) {
        if (const std::optional<MazeCell> cell = introShadeCell(introMoment.card, shadeInGame())) {
            scene::Transform shade;
            shade.position = cellCenter(cell->x, cell->z);
            shade.position.y = m_mazeWorld.terrain.heightAt(shade.position.x, shade.position.z);
            shade.rotationDegrees = {0.0F, shadeYawDegrees(shade.position, eye), 0.0F};
            m_shadeMatrix = shade.matrix();
            m_shadeDrawn = true;
        }
    }

    // The lighting of this frame. The round changes two things for this frame only:
    // a low battery dims the flashlight (an empty one switches it off) and the crystal
    // lights pulse. That happens in a copy, so the settings the debug UI shows stay as
    // they were set.
    LightingSettings frameLighting = lightingForFrame(m_lighting, m_round, m_gameplay);
    if (menuCamera) {
        // The flashlight of the menu camera is not the one of the round: it does not
        // depend on the switch or on the battery, and it never flickers. In the
        // corridors it is on, at the brightness of the settings: the cone of warm light
        // on the stone is the look of the game. High above the maze it is off: its
        // light does not reach the ground from there, and the moon and the crystals
        // are what that shot shows.
        frameLighting.flashlightOn = menuCameraSettings.shot == MenuShot::CorridorWalk;
        frameLighting.flashlightIntensity = m_lighting.flashlightIntensity;
        // The script of the intro names the light of every card itself.
        if (intro) {
            frameLighting.flashlightOn = introShot.flashlightOn;
        }
    }

    // Where the flashlight is and where it points in this frame: in the hand, a little
    // to the right of the eye and below it, aimed at a point in front of the eye. It is
    // computed here, after the mouse has turned the camera and from the same eye the
    // view matrix uses below. From m_camera.position (the last fixed step) the cone
    // would trail behind the picture while the player moves. It is computed ONCE: the
    // shadow pass of the flashlight and the lights of the frame both get this result,
    // so the shadows always belong to the light that is drawn.
    const FlashlightPose flashlight =
        flashlightPose(frameLighting, eye, frameCamera.forward(), frameCamera.right());

    // The shadow passes come first: the scene as the moon sees it and then as the
    // flashlight sees it, depths only, each into its own shadow map. The lit programs
    // of the scene pass read those maps, so they have to be complete before they draw.
    // Each pass binds a framebuffer and a viewport of its own (the size of its map),
    // and beginScene below binds the scene framebuffer with its viewport again.
    drawMoonShadowMap();
    drawFlashlightShadowMap(frameLighting, flashlight);

    // From here on the draw calls do not land in the window. They land in the HDR
    // framebuffer of the scene, which is created again here when the size of the window
    // has changed. The viewport is set to its size by the same call. Without
    // a framebuffer (the driver refused it, the error is in the log) nothing is drawn.
    // The shadow passes above may have left their own framebuffer bound, so the window
    // is bound again first: the debug UI is drawn after this function and must land
    // there.
    if (!m_postProcess.beginScene(framebuffer)) {
        gfx::Framebuffer::bindDefault(framebuffer.width, framebuffer.height);
        return;
    }

    // Depth test: a fragment is kept only if it is nearer to the camera than what is
    // already drawn at that pixel, so the near walls hide the far ones in whatever order
    // the triangles are drawn. It is switched on every frame, next to the other state
    // this frame relies on, instead of once at start-up: the frame then does not depend
    // on other code (the debug UI changes this state) leaving it switched on.
    GL_CHECK(glEnable(GL_DEPTH_TEST));

    // The depth buffer has to be cleared together with the color, otherwise the depths of
    // the previous frame would hide the new one. The clear colour is what stays on the
    // pixels nothing is drawn on: with the skybox on there are none, the sky fills them.
    // Both buffers are the two textures of the scene framebuffer now. The clear colour
    // is an sRGB value and the buffer holds linear colours, so it is converted first.
    const glm::vec3 clearColor =
        gfx::srgbToLinear(glm::vec3{m_clearColor[0], m_clearColor[1], m_clearColor[2]});
    GL_CHECK(glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0F));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // The lights of this frame, from the lighting and the flashlight pose computed
    // before the shadow passes. The point lights hang above the crystals that are still
    // there: the ones nearest to the eye of this frame, because a large maze has more
    // crystals than the shaders have point lights (game::nearestPointLights). The copy
    // to the graphics card happens once, and the two lit programs and the grass program
    // read it. It takes the EYE, not the hand: the highlights are computed for the place
    // the picture is taken from.
    const std::vector<PointLightSpot> crystalLights =
        nearestPointLights(crystalLightPositions(m_round), eye);
    const scene::LightSet lights = buildLightSet(frameLighting, flashlight, crystalLights);
    m_lightRig.upload(lights, eye);

    drawMaze(view, projection);
    drawGrass(view, projection);
    // The reflection pass: the crystals and the puddles, which show the sky. The
    // crystals are opaque and write depth like the walls. The puddles are blended over
    // the ground that is already drawn and write no depth (game::PuddleRenderer), so
    // they need the ground first. The sky comes after both: the ground under a puddle
    // has written its depth, so the sky does not show through.
    drawReflections(view, projection);
    if (m_drawColliders) {
        drawColliderLines(view, projection);
    }
    if (m_pickDebug.drawShapes) {
        drawPickLines(view, projection);
    }

    // The sky comes LAST, after everything that writes depth. It is drawn at the largest
    // depth and passes the depth test only where nothing else was drawn. With the walls
    // and hills already in the depth buffer, the graphics card can reject the hidden sky
    // fragments before it runs skybox.frag for them (the early depth test, which it may
    // use here because the shader neither discards nor writes depth). Drawn first, the
    // whole screen would be shaded and then mostly painted over. The sky does not depend
    // on the lighting mode: it is not lit, it is the same picture in all four. The debug
    // views change it, see Skybox::draw. The picture would be the same with the sky
    // drawn earlier (the depth test sorts opaque things out). Only something that does
    // not write depth, like a transparent effect, would have to come after the sky.
    if (m_skyboxSettings.enabled) {
        m_skybox.draw(m_skyboxShader, view, projection, m_skyboxSettings, m_viewMode);
    }

    // The scene is complete: its colours and its depth are in the two textures of the
    // scene framebuffer. What follows reads those textures. Passes that are added
    // later (effects computed from the finished scene) belong here, before the
    // composite pass.

    // The pictures of the attachments, only while the debug UI shows them.
    if (m_postProcessSettings.previews) {
        m_postProcess.drawPreviews(m_previewShader, m_postProcessSettings, frameCamera.nearPlane,
                                   frameCamera.farPlane);
    }

    // The two debug views show data as colours (a normal, a texture coordinate), not
    // light. An exposure or a tone mapping curve would change those numbers, and
    // a bloom would make the bright ones glow, a fog would mix its colour into them and
    // a vignette would darken them towards the corners, so all five are switched off
    // for them, in a copy: the settings the debug UI shows stay.
    PostProcessSettings compositeSettings = m_postProcessSettings;
    if (m_viewMode != ViewMode::Textured) {
        compositeSettings.exposure = NEUTRAL_EXPOSURE;
        compositeSettings.toneMapping = ToneMapping::None;
        compositeSettings.bloom.enabled = false;
        compositeSettings.fog.enabled = false;
        compositeSettings.vignette.enabled = false;
    }
    // Right after the shade carried the player back the picture comes up from black: the
    // exposure multiplies every colour, so a factor of 0 is a black picture.
    compositeSettings.exposure *= roundBrightness(m_round);

    // The bloom: the bright parts of the finished scene, blurred in targets of half the
    // size. It is called in every frame, also with the bloom switched off: it then
    // draws nothing and tells the composite pass so.
    m_postProcess.drawBloom(m_brightPassShader, m_blurShader, m_previewShader, compositeSettings);

    // The fog of the last pass finds the place in the world every pixel shows. For that
    // it needs the way back from the screen to the world: the inverse of the two
    // matrices the scene was drawn with, multiplied in the order a vertex shader applies
    // them (the view first, then the projection), and the eye they were built for.
    const SceneView sceneView{.inverseViewProjection = glm::inverse(projection * view), .eye = eye};

    // The last pass: back to the window, and the HDR picture goes into it with the fog,
    // the bloom, exposure, tone mapping, the vignette and the sRGB encoding. The debug
    // UI is drawn after this function returns (main.cpp), straight into the window.
    m_postProcess.composite(m_compositeShader, compositeSettings, framebuffer, sceneView);

    // The map comes after the composite pass, on top of the finished picture: it is
    // a schematic, and the fog, the bloom and the tone mapping must not touch it. It is
    // shown in the two debug views too, like the HUD. feet is the blended position the
    // scene was drawn from. Only while the key is held in a round (game::showsMap): the
    // pause menu, the result screen and the menu camera have no map.
    if (map) {
        drawMinimap(framebuffer, feet);
    }

    // The menu comes last in this function, on top of the finished frame. The debug UI
    // is drawn after this function returns (main.cpp), so its panels stay usable on top
    // of a menu. While a round is played no document is shown and this call does
    // nothing.
    m_ui.draw(framebuffer);
}

PickState NightMazeApp::pickForFrame(const glm::mat4& view, const glm::mat4& projection,
                                     const glm::vec3& eye, bool cursorCaptured) {
    // The cursor is measured in the units of the WINDOW size (screen coordinates), so
    // that size is its partner here and not the size of the framebuffer: on a Retina
    // display the framebuffer has twice as many pixels. A window without a size has no
    // point to cast a ray through (scene::screenPointRay would throw).
    const core::Size windowSize = window().windowSize();
    if (windowSize.width <= 0 || windowSize.height <= 0) {
        return pickNothing(m_round);
    }
    const glm::vec2 size{static_cast<float>(windowSize.width),
                         static_cast<float>(windowSize.height)};

    // With the cursor captured the player aims with the camera: the ray goes through
    // the middle of the picture, where the crosshair is. With a free cursor it goes
    // through the cursor.
    glm::vec2 point = size / 2.0F;
    if (!cursorCaptured) {
        // Not valid while the debug UI uses the mouse: a cursor over a panel points at
        // the panel, not at the scene behind it.
        const core::CursorPosition cursor = input().cursorPosition();
        if (!cursor.valid) {
            return pickNothing(m_round);
        }
        point = {static_cast<float>(cursor.x), static_cast<float>(cursor.y)};
        // A cursor that has left the window points at nothing in the picture.
        if (point.x < 0.0F || point.y < 0.0F || point.x > size.x || point.y > size.y) {
            return pickNothing(m_round);
        }
    }

    // From the point of the picture back into the world: the inverse of the two
    // matrices the frame is drawn with. The ray then starts in the eye, so the reach
    // is measured from the eye (game::rayFromEye).
    const scene::Ray screenRay =
        scene::screenPointRay(point, size, glm::inverse(projection * view));
    // The obstacles of the round are what hides a lever or a note: walls, pillars and
    // the closed gate. A wall that a lever has opened is not in the list.
    return pickInRound(rayFromEye(screenRay, eye), cursorCaptured, m_mazeWorld, m_round,
                       m_obstacles);
}

void NightMazeApp::handleInteraction(bool cursorCaptured) {
    // wasKeyPressed and wasMouseButtonPressed are true for one frame. A click on
    // a debug panel does not arrive here: main.cpp blocks the mouse for the game while
    // the debug UI is using it.
    const bool keyPressed = input().wasKeyPressed(INTERACT_KEY);
    const bool clicked = input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

    if ((keyPressed || clicked) && m_pick.action != Interaction::None) {
        // There is something to do: pull the lever, read the note or close the card.
        // A wall that opened is no obstacle any more. interact is true only then: for
        // the first pull of a lever, so that is the moment of its sound.
        if (interact(m_round, m_mazeWorld, m_pick)) {
            m_obstacles = roundObstacles(m_mazeWorld, m_round);
            playCue(SoundCue::LeverPull);
        }
        // The round has changed, so the action is asked again: the lever that was just
        // pulled is not highlighted in this frame, and an opened card can be closed.
        m_pick.action = interactionFor(m_round, m_pick.picked);
    } else if (clicked && !cursorCaptured) {
        // A click into the scene that hit nothing to use: it captures the cursor, which
        // switches on mouse look and movement. So a click with the free cursor ON
        // a lever or a note uses it, and any other click captures.
        input().setCursorCaptured(true);
    }
}

bool NightMazeApp::mapShown() {
    return showsMap(m_mode, {.keyHeld = input().isKeyDown(MINIMAP_KEY),
                             .pinned = m_minimapSettings.pinned,
                             .noteOpen = m_round.noteOpen,
                             .menuCamera = m_menuCamera.enabled});
}

void NightMazeApp::drawMinimap(core::Size framebuffer, const glm::vec3& feet) {
    // Where the map stands, in framebuffer pixels: a square in the middle. Its picture
    // is drawn at exactly that size, so the framebuffer of the minimap follows the
    // window and the size setting. A size of 0: the window has no room for a map.
    const MinimapRect rect = minimapRect(framebuffer.width, framebuffer.height, m_minimapSettings);
    if (rect.size < 1) {
        return;
    }

    // The shapes of the map are built again in every frame, on the CPU (plain data,
    // covered by tests), and copied to the graphics card in one piece. Every frame and
    // not only after a change, because the arrow of the player moves all the time, and
    // because then nothing can be forgotten: a cell that was discovered, a crystal that
    // was collected, the gate, a new maze or a wall that a lever has opened all show
    // up by themselves. The default maze is about 1400 vertices of 20 bytes when all
    // of it is shown.
    const Maze& maze = m_mazeWorld.maze;
    const MinimapPlayer player{.position = feet, .yawDegrees = m_camera.yawDegrees};
    const std::vector<MinimapVertex> vertices =
        buildMinimapVertices(m_mazeWorld, m_round, m_minimapSettings.revealAll, player,
                             minimapMetresPerPixel(maze, rect.size), m_gameplay.shade.showOnMap);

    // The offscreen pass, and then the picture into the middle of the window.
    if (m_minimapRenderer.drawMap(m_minimapShader, vertices, minimapProjection(maze), rect.size)) {
        m_minimapRenderer.drawOverlay(m_minimapOverlayShader, rect, m_minimapSettings.opacity,
                                      framebuffer);
    }
}

void NightMazeApp::drawMoonShadowMap() {
    // Where the moon looks: an orthographic box around the whole land, seen from the
    // direction of its light. It is computed again in every frame, from the terrain
    // and the two angles of the moon alone: a few dozen multiplications, and nothing
    // that could be forgotten when a new maze is built, the height scale changes or
    // the Lights panel moves the moon. The camera is not part of it, so the map covers
    // the same ground in every frame and the shadows stand still when the player
    // moves.
    m_moonLightSpace = scene::directionalLightSpace(shadowCasterBounds(m_mazeWorld.terrain),
                                                    moonDirection(m_lighting));

    // Until the pass below has run, this frame has no shadows.
    m_moonShadowDrawn = false;
    if (!m_moonShadow.enabled || !m_shadowDepthShader.isValid()) {
        return;
    }
    if (!m_moonShadowMap.beginDepthPass(shadowMapSize(m_moonShadow.resolution))) {
        return;
    }
    drawShadowCasters(m_moonLightSpace);
    m_moonShadowDrawn = true;

    // The map goes to its texture unit once, and stays there while the scene is drawn.
    m_moonShadowMap.bindForSampling(MOON_SHADOW_TEXTURE_UNIT, m_moonShadow.hardwareFilter);

    // The picture of the map, only while the debug UI shows it.
    if (m_moonShadow.preview) {
        m_moonShadowMap.drawPreview(m_previewShader, m_moonLightSpace);
    }
}

void NightMazeApp::drawFlashlightShadowMap(const LightingSettings& frameLighting,
                                           const FlashlightPose& flashlight) {
    // Where the flashlight looks: a pyramid with its tip in the hand, along the beam,
    // a little wider than the cone of light and as deep as the light reaches. It is
    // computed again in every frame, also with the shadows switched off: the debug UI
    // shows its size. The position and the direction are the ones the spot light of
    // this frame is built with (onRender), the cone and the range come from the same
    // settings of the frame.
    m_flashlightLightSpace =
        scene::spotLightSpace(flashlight.position, flashlight.direction,
                              frameLighting.flashlightOuterDegrees, frameLighting.flashlightRange);

    // Until the pass below has run, this frame has no flashlight shadows.
    m_flashlightShadowDrawn = false;
    // A flashlight that is off gives no light, so there is nothing its shadows could
    // take away: the pass is skipped. frameLighting is asked and not m_lighting,
    // because an empty battery switches the light off for the frame.
    if (!m_flashlightShadow.enabled || !frameLighting.flashlightOn ||
        !m_shadowDepthShader.isValid()) {
        return;
    }
    if (!m_flashlightShadowMap.beginDepthPass(shadowMapSize(m_flashlightShadow.resolution))) {
        return;
    }
    drawShadowCasters(m_flashlightLightSpace);
    m_flashlightShadowDrawn = true;

    // The map goes to its own texture unit, next to the one of the moon.
    m_flashlightShadowMap.bindForSampling(FLASHLIGHT_SHADOW_TEXTURE_UNIT,
                                          m_flashlightShadow.hardwareFilter);

    // The picture of the map, only while the debug UI shows it.
    if (m_flashlightShadow.preview) {
        m_flashlightShadowMap.drawPreview(m_previewShader, m_flashlightLightSpace);
    }
}

void NightMazeApp::setShadowUniformsOf(const gfx::Shader& shader) const {
    // The shadow map of the moon: where it is bound, the matrix it was drawn with and
    // the numbers of the comparison.
    setShadowUniforms(shader, MOON_SHADOW_UNIFORMS, MOON_SHADOW_TEXTURE_UNIT, m_moonShadowDrawn,
                      m_moonShadow, m_moonLightSpace);
    // The same for the shadow map of the flashlight.
    setShadowUniforms(shader, FLASHLIGHT_SHADOW_UNIFORMS, FLASHLIGHT_SHADOW_TEXTURE_UNIT,
                      m_flashlightShadowDrawn, m_flashlightShadow, m_flashlightLightSpace);
}

void NightMazeApp::drawShadowCasters(const scene::LightSpace& lightSpace) const {
    // The classes that draw the scene are used as they are, with another program and
    // the matrices of the light in place of the ones of the camera. So everything
    // stands in the shadow map exactly where it stands in the picture: the gate and
    // the walls of pulled levers as far as they have sunk, every crystal where it
    // floats at this moment. The depth program
    // has no samplers, no tint and no glow: those uniforms are set all the same and
    // ignored, as every uniform a program does not have.
    m_shadowDepthShader.use();
    m_shadowDepthShader.setMat4(VIEW_UNIFORM, lightSpace.view);
    m_shadowDepthShader.setMat4(PROJECTION_UNIFORM, lightSpace.projection);

    // The terrain is always drawn filled here: the wireframe switch is a way to look
    // at the ground, and a ground of lines would cast a shadow of lines.
    constexpr bool NO_WIREFRAME = false;
    m_terrainRenderer.draw(m_shadowDepthShader, NO_WIREFRAME);
    m_mazeRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_wallMatrices,
                        m_mazeSettings.wallVariants);
    m_gameplayRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_round, crystalEmissive());
    m_gameplayRenderer.drawFlasks(m_shadowDepthShader, m_mazeWorld, m_round);
    // The shade casts a shadow like everything that stands in the maze.
    if (m_shadeDrawn) {
        m_gameplayRenderer.drawShade(m_shadowDepthShader, m_shadeMatrix);
    }
    // The levers and the notes cast shadows too. An empty PickState: nothing is
    // highlighted, the depth program has no colours.
    m_interactableRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_round, PickState{},
                                glm::vec3{0.0F});
    // The grass is left out. A blade is 4 cm wide at its root and thinner above, and
    // a texel of the map of the moon is about 3 cm, so its shadow would be a flicker of
    // single texels that moves with the wind, on ground the tuft itself hides. The map
    // of the flashlight has finer texels, but the grass is left out of it too: one rule
    // for both lights, and no shadows that sway on every wall the beam passes. The
    // grass still RECEIVES shadows.
}

glm::vec3 NightMazeApp::crystalEmissive() const {
    // The colour of the crystal lights is an sRGB value, like every colour of the
    // lighting settings. It is converted here the way buildLightSet converts it for
    // the lights, so the mesh glows in the colour of the light around it.
    return crystalGlow(gfx::srgbToLinear(m_lighting.pointColor), m_round.animationSeconds);
}

void NightMazeApp::drawMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // The two debug views (normals and texture coordinates as colours) only exist in the
    // textured program, and they show data, not light. So they are drawn without
    // lighting whatever the lighting mode is. The view of the normals still follows the
    // lighting in one thing: it shows the normals the chosen mode shades with.
    if (m_lighting.mode == LightingMode::Unlit || m_viewMode != ViewMode::Textured) {
        drawUnlitMaze(view, projection);
    } else {
        drawLitMaze(view, projection);
    }
}

void NightMazeApp::drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // Without a shader program there is nothing to draw with. The load error was logged
    // once, when the shader was created, and the rest of the frame is still drawn.
    if (!m_texturedShader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    m_texturedShader.use();
    m_texturedShader.setMat4(VIEW_UNIFORM, view);
    m_texturedShader.setMat4(PROJECTION_UNIFORM, projection);
    // The enum values are the numbers textured.frag compares uViewMode with.
    m_texturedShader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));
    // Only the view of the normals reads it: that view shows the normals the lighting
    // would use, so with normal mapping the ones from the normal maps.
    m_texturedShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);

    // The ground first, then what stands on it. The order does not change the picture
    // (the depth test sorts it out), it only follows the way the scene is built.
    m_terrainRenderer.draw(m_texturedShader, m_terrainSettings.wireframe);
    m_mazeRenderer.draw(m_texturedShader, m_mazeWorld, m_wallMatrices, m_mazeSettings.wallVariants);
    // The crystals and the gate, with the same program: they show up in the debug
    // views like the walls do. (In the picture without lighting the crystals may be
    // left to the reflection pass, see drawGateAndCrystals.)
    drawGateAndCrystals(m_texturedShader);
    // The levers and the notes. The highlight of the picked one shows in the picture
    // without lighting. The two debug views show data and ignore it.
    drawInteractables(m_texturedShader);
}

void NightMazeApp::drawLitMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // Gouraud has a program of its own (the light is computed in its vertex shader).
    // Phong and Blinn-Phong share the other one and differ in one uniform.
    const gfx::Shader& shader =
        m_lighting.mode == LightingMode::Gouraud ? m_gouraudShader : m_litShader;
    if (!shader.isValid()) {
        return;
    }

    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
    // The material of the stone, which the ground shares. The lights themselves are not
    // set here: they are in the uniform buffer that onRender filled before this call.
    // The enum values are the numbers common/lighting.glsl compares uSpecularModel with.
    shader.setInt(SPECULAR_MODEL_UNIFORM, static_cast<int>(specularModelOf(m_lighting.mode)));
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    shader.setFloat(SHININESS_UNIFORM, m_lighting.shininess);
    // Normal mapping, the switch of the lit program (1 on, 0 off). The Gouraud program
    // has no such uniform, and usesNormalMap is false for it anyway.
    shader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    // The two shadow maps (the moon and the flashlight). Set in every frame, also with
    // the shadows off.
    setShadowUniformsOf(shader);

    // The ground first, then what stands on it, as in drawUnlitMaze.
    m_terrainRenderer.draw(shader, m_terrainSettings.wireframe);
    m_mazeRenderer.draw(shader, m_mazeWorld, m_wallMatrices, m_mazeSettings.wallVariants);
    // The crystals and the gate, with the same program and so the same lighting mode.
    // The crystals glow in the colour of their lights. (The crystals may be left to the
    // reflection pass, see drawGateAndCrystals.)
    drawGateAndCrystals(shader);
    // The levers and the notes, lit like the walls they hang on.
    drawInteractables(shader);
}

void NightMazeApp::drawInteractables(const gfx::Shader& shader) const {
    // The highlight pulses on the animation clock of the round, which never stops.
    m_interactableRenderer.draw(shader, m_mazeWorld, m_round, m_pick,
                                highlightGlow(m_round.animationSeconds));
}

bool NightMazeApp::crystalsReflect() const {
    return m_environment.enabled && m_viewMode == ViewMode::Textured && m_reflectShader.isValid();
}

void NightMazeApp::drawGateAndCrystals(const gfx::Shader& shader) const {
    m_gameplayRenderer.drawGate(shader, m_mazeWorld, m_round);
    // The flasks, always with the program of the walls: they are brass, not glass, and
    // show no sky.
    m_gameplayRenderer.drawFlasks(shader, m_mazeWorld, m_round);
    // The shade, with the program of the walls too: it is lit like them, so the beam of
    // the flashlight shows it and the dark hides it. Its cloth has almost no highlight:
    // with the highlight of the stone the flashlight, which shines from where the player
    // looks, would paint a bright board on the dark figure. The strength of the stone is
    // put back for what is drawn next. (The textured program has no such uniform.)
    if (m_shadeDrawn) {
        shader.setFloat(SPECULAR_STRENGTH_UNIFORM, SHADE_SPECULAR_STRENGTH);
        m_gameplayRenderer.drawShade(shader, m_shadeMatrix);
        shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    }
    // Every crystal is drawn exactly once per frame: here, with the program of the
    // walls, or later by drawReflections with the reflect program.
    if (!crystalsReflect()) {
        m_gameplayRenderer.drawCrystals(shader, m_round, crystalEmissive());
    }
}

void NightMazeApp::drawReflections(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_environment.enabled) {
        return;
    }
    const bool puddlesDrawn = m_environment.puddles && m_puddleRenderer.puddleCount() > 0;

    // The two debug views show data, not light or sky. The crystals were drawn by
    // drawUnlitMaze with the textured program. The puddles follow here with the same
    // program, whose uniforms of this frame (the matrices and the view mode) that
    // function has already set: a uniform keeps its value while other programs draw.
    if (m_viewMode != ViewMode::Textured) {
        if (puddlesDrawn && m_texturedShader.isValid()) {
            m_texturedShader.use();
            m_puddleRenderer.draw(m_texturedShader);
        }
        return;
    }

    if (!m_reflectShader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    m_reflectShader.use();
    m_reflectShader.setMat4(VIEW_UNIFORM, view);
    m_reflectShader.setMat4(PROJECTION_UNIFORM, projection);

    // The lighting, as drawLitMaze sets it for the lit program. The reflect program
    // computes the light per fragment in every lit mode, also in the mode Gouraud (the
    // sky has to be looked up per fragment anyway), and shows the surface at full
    // brightness in the mode Unlit, like the grass program.
    const bool lit = m_lighting.mode != LightingMode::Unlit;
    m_reflectShader.setInt(REFLECT_LIT_UNIFORM, lit ? 1 : 0);
    m_reflectShader.setInt(SPECULAR_MODEL_UNIFORM,
                           static_cast<int>(specularModelOf(m_lighting.mode)));
    m_reflectShader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    m_reflectShader.setFloat(SHININESS_UNIFORM, m_lighting.shininess);
    m_reflectShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    setShadowUniformsOf(m_reflectShader);

    // The sky the surfaces show: the cube map of the skybox on a texture unit of its
    // own, as bright as the skybox draws it. With the skybox switched off (or without
    // its pictures) the background of the frame is the clear colour, so that colour is
    // what a mirror shows. The sampler gets its unit in every case: left at 0 it would
    // share a unit with the colour texture, which OpenGL does not allow.
    const bool skyVisible = m_skyboxSettings.enabled && m_skybox.isValid();
    m_reflectShader.setInt(ENVIRONMENT_MAP_UNIFORM, static_cast<int>(ENVIRONMENT_TEXTURE_UNIT));
    m_reflectShader.setInt(ENVIRONMENT_SKY_VISIBLE_UNIFORM, skyVisible ? 1 : 0);
    m_reflectShader.setFloat(ENVIRONMENT_SKY_BRIGHTNESS_UNIFORM, m_skyboxSettings.brightness);
    m_reflectShader.setVec3(
        ENVIRONMENT_BACKGROUND_UNIFORM,
        gfx::srgbToLinear(glm::vec3{m_clearColor[0], m_clearColor[1], m_clearColor[2]}));
    if (m_skybox.isValid()) {
        m_skybox.cubemap().bind(ENVIRONMENT_TEXTURE_UNIT);
        // Blend the texels of two faces at the border between them, see Skybox::draw. It
        // is switched on here too: this pass reads the cube map before the sky is drawn.
        GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
    }

    // The crystals: a blend of the mirrored and of the refracted sky, the same share at
    // every angle. Their glow can be turned down for this pass, to see the sky on them.
    m_reflectShader.setFloat(ENVIRONMENT_STRENGTH_UNIFORM, m_environment.crystalStrength);
    m_reflectShader.setInt(ENVIRONMENT_FRESNEL_ENABLED_UNIFORM, 0);
    m_reflectShader.setFloat(ENVIRONMENT_REFLECT_SHARE_UNIFORM, m_environment.crystalReflectShare);
    m_reflectShader.setFloat(ENVIRONMENT_REFRACTION_RATIO_UNIFORM,
                             m_environment.crystalRefractionRatio);
    m_gameplayRenderer.drawCrystals(m_reflectShader, m_round,
                                    crystalEmissive() * m_environment.crystalGlowShare);

    // The puddles: level mirrors. Their normal points straight up (no normal map),
    // their highlight is the one of water, and they show the mirrored sky only, more of
    // it the flatter they are looked at. The refraction ratio stays as the crystals
    // left it: with a reflect share of 1 the refracted picture is not shown.
    if (puddlesDrawn) {
        m_reflectShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, 0);
        m_reflectShader.setFloat(SPECULAR_STRENGTH_UNIFORM, PUDDLE_SPECULAR_STRENGTH);
        m_reflectShader.setFloat(SHININESS_UNIFORM, PUDDLE_SHININESS);
        m_reflectShader.setFloat(ENVIRONMENT_STRENGTH_UNIFORM, m_environment.puddleReflectivity);
        m_reflectShader.setInt(ENVIRONMENT_FRESNEL_ENABLED_UNIFORM,
                               m_environment.puddleFresnel ? 1 : 0);
        m_reflectShader.setFloat(ENVIRONMENT_REFLECT_SHARE_UNIFORM, MIRROR_ONLY);
        m_puddleRenderer.draw(m_reflectShader);
    }

    // Binding the cube map made its texture unit the active one. The draws above put
    // unit 0 back, but with no crystal left and no puddle nothing was drawn, so it is
    // put back here: the rest of the frame expects unit 0 to be the active one.
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
}

void NightMazeApp::drawGrass(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_grassSettings.enabled) {
        return;
    }

    // The grass has one program for every lighting mode. It is lit per fragment in the
    // three lit modes (with Gouraud too: a tuft has no vertices in any buffer that
    // light could be computed at), and shown at full brightness in the mode Unlit.
    const bool lit = m_lighting.mode != LightingMode::Unlit;
    // The clock of the wind: the seconds since GLFW was started. It only has to keep
    // growing, so it does not stop when the round is won and does not jump when one is
    // restarted.
    const auto windSeconds = static_cast<float>(glfwGetTime());

    // The grass lies in the shadows of the moon and of the flashlight like the ground,
    // so its program gets the uniforms of the two shadow maps too. A uniform is written
    // into the program in use, hence use() here: GrassRenderer::draw calls it again,
    // which changes nothing.
    if (m_grassShader.isValid()) {
        m_grassShader.use();
        setShadowUniformsOf(m_grassShader);
    }
    m_grassRenderer.draw(m_grassShader, view, projection, m_grassSettings, windSeconds, lit,
                         m_viewMode);
}

void NightMazeApp::drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_colorShader.isValid()) {
        return;
    }

    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);

    // The depth test stays on: a line behind a wall is hidden by it, which shows where
    // each box really is. The box of the player is drawn at the simulation position (the
    // last fixed step), the camera at a blend of two steps, so while moving the box runs
    // ahead of the camera by a fraction of one step.
    //
    // The boxes of the maze come from the obstacle list of the round, so a wall that
    // a lever has opened is not drawn. While the gate blocks, its box is the last one of
    // that list (game::roundObstacles): it is left out here and drawn in its own colour
    // below.
    const std::size_t gateBoxes = gateBlocks(m_mazeWorld, m_round) ? 1 : 0;
    const std::span<const scene::Aabb> mazeBoxes =
        std::span<const scene::Aabb>(m_obstacles).first(m_obstacles.size() - gateBoxes);
    m_colliderLines.draw(m_colorShader, mazeBoxes, MAZE_COLLIDER_COLOR);
    // draw takes a list of boxes. A span made of a pointer and a count of 1 is a list
    // with this one box in it.
    const scene::Aabb playerBox = m_player.box();
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&playerBox, 1),
                         PLAYER_COLLIDER_COLOR);

    // The gate, while it is an obstacle, and the zone behind it that wins the round.
    if (gateBlocks(m_mazeWorld, m_round)) {
        m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.gateBox, 1),
                             GATE_COLLIDER_COLOR);
    }
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.exitZone, 1),
                         EXIT_ZONE_COLOR);

    // The spheres of the pickup test: the reach of the player and, around every crystal
    // that is still there, the sphere the reach has to overlap. They stay on the
    // resting place of the crystal while the crystal itself bobs.
    const scene::Sphere reach = playerReach(m_player.position);
    m_colliderLines.drawSpheres(m_colorShader, std::span<const scene::Sphere>(&reach, 1),
                                PLAYER_COLLIDER_COLOR);
    std::vector<scene::Sphere> pickupSpheres;
    for (const RoundCrystal& crystal : m_round.crystals) {
        if (!crystal.collected) {
            pickupSpheres.push_back(
                {.center = crystalCenter(crystal.restPosition), .radius = m_gameplay.pickupRadius});
        }
    }
    m_colliderLines.drawSpheres(m_colorShader, pickupSpheres, PICKUP_COLLIDER_COLOR);
}

void NightMazeApp::drawPickLines(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_colorShader.isValid()) {
        return;
    }

    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);

    // The pick boxes: what the ray has to hit. They are larger than the models on
    // purpose, and they reach out of the collision box of the wall they hang on.
    const Interactables& interactables = m_mazeWorld.interactables;
    std::vector<scene::Aabb> leverBoxes;
    leverBoxes.reserve(interactables.levers.size());
    for (const Lever& lever : interactables.levers) {
        leverBoxes.push_back(lever.box);
    }
    std::vector<scene::Aabb> noteBoxes;
    noteBoxes.reserve(interactables.notes.size());
    for (const Note& note : interactables.notes) {
        noteBoxes.push_back(note.box);
    }

    // The ray that is shown: the one of this frame, or the frozen one. The box it hit
    // is drawn FIRST, in the colour of a hit. The same box follows below in the colour
    // of its kind, exactly on top of these lines: the depth test keeps a fragment only
    // when it is nearer than what is there, so the lines drawn first stay. The index is
    // checked: the frozen ray may be older than the maze.
    const PickState& shown = m_shownPick;
    const PickedInteractable& picked = shown.picked;
    const bool hitLever =
        shown.hasRay && picked.kind == InteractableKind::Lever && picked.index < leverBoxes.size();
    const bool hitNote =
        shown.hasRay && picked.kind == InteractableKind::Note && picked.index < noteBoxes.size();
    if (hitLever) {
        m_colliderLines.draw(m_colorShader,
                             std::span<const scene::Aabb>(&leverBoxes[picked.index], 1),
                             PICK_HIT_COLOR);
    }
    if (hitNote) {
        m_colliderLines.draw(m_colorShader,
                             std::span<const scene::Aabb>(&noteBoxes[picked.index], 1),
                             PICK_HIT_COLOR);
    }
    m_colliderLines.draw(m_colorShader, leverBoxes, LEVER_PICK_COLOR);
    m_colliderLines.draw(m_colorShader, noteBoxes, NOTE_PICK_COLOR);
    if (!shown.hasRay) {
        return;
    }

    // The ray itself: from the eye to the point where it enters the box it hit, or as
    // far as the player can reach when it hit nothing. A small sphere marks its end.
    // Seen from the eye it started in, the line is a single point behind the crosshair:
    // freeze it and step aside to see it.
    const bool hit = hitLever || hitNote;
    const float length = hit ? picked.distance : INTERACTION_REACH;
    const glm::vec3 end = shown.ray.origin + shown.ray.direction * length;
    const glm::vec3 color = hit ? PICK_HIT_COLOR : PICK_MISS_COLOR;
    m_colliderLines.drawLine(m_colorShader, shown.ray.origin, end, color);
    const scene::Sphere marker{.center = end, .radius = PICK_MARKER_RADIUS};
    m_colliderLines.drawSpheres(m_colorShader, std::span<const scene::Sphere>(&marker, 1), color);
}

} // namespace game
