// The Night Maze application: game state and rendering of a frame.
#include "game/NightMazeApp.hpp"

#include "assets/ImageLoader.hpp"
#include "core/Files.hpp"
#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "game/Crystals.hpp"
#include "game/Exit.hpp"
#include "game/GateLamp.hpp"
#include "game/Interactables.hpp"
#include "game/Ledger.hpp"
#include "game/ModelDraw.hpp"
#include "game/Puddles.hpp"
#include "game/Shade.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "scene/Raycast.hpp"
#include "scene/Transform.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
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

// The keys of the player (walk, sprint, use, flashlight, map, restart) are not named
// here: the player chooses them on the settings screen (game/KeyBindings.hpp), and they
// are read through actionDown and actionPressed. What follows are the keys of the
// tools. They are fixed, and game::isFixedKey lists them so that no action can be put
// on one of them.

// Key that switches between walking and noclip (free flight).
constexpr int NOCLIP_KEY = GLFW_KEY_N;

// The two keys that fly up and down in noclip. They belong to that tool only, so the
// player may put an action on them: Left Shift is the sprint of a game without
// a settings file.
constexpr int NOCLIP_UP_KEY = GLFW_KEY_SPACE;
constexpr int NOCLIP_DOWN_KEY = GLFW_KEY_LEFT_SHIFT;

// Key that switches the menu camera on and off: the game then shows itself.
constexpr int MENU_CAMERA_KEY = GLFW_KEY_F2;

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

// The name of the difficulty of a maze that no level describes: one the debug UI built.
constexpr const char* CUSTOM_DIFFICULTY_NAME = "Custom";

// A random seed of the menu is below this number: at most six digits, short enough to
// read out to a friend. A typed seed can be any number a seed can be.
constexpr std::uint32_t RANDOM_SEED_LIMIT = 1000000;

// A seed nobody can predict, from 1 to RANDOM_SEED_LIMIT - 1. std::random_device asks
// the operating system for a random number. It only picks the seed: the maze itself is
// built by the seeded generator, so the same seed always brings the same maze back.
std::uint32_t randomSeed() {
    std::random_device device;
    return 1 + static_cast<std::uint32_t>(device()) % (RANDOM_SEED_LIMIT - 1);
}

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
// True for the two screens that show the ledger.
bool isLedger(GameMode mode) {
    return mode == GameMode::LedgerFromMenu || mode == GameMode::LedgerFromPause;
}

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

// The face of the shade is the opposite: all of its colour is the sky
// (uEnvironmentStrength), all of that the picture seen through the surface
// (uReflectShare), and the ray into it is not bent (uRefractionRatio), so the sky is read
// along the look of the eye. That sky is turned by this angle around the vertical axis,
// in radians (uSkyTurn): 20 degrees, a little, but enough that no star in the hood sits
// where the sky behind the figure has it.
constexpr float SKY_ONLY = 1.0F;
constexpr float SEEN_THROUGH_ONLY = 0.0F;
constexpr float NO_REFRACTION = 1.0F;
constexpr float SHADE_HOOD_SKY_TURN = 0.35F;
// The face shows a wider piece of sky than it covers (uSkySpread, per metre from the
// middle of the face, which is this point of the model shade_hollow.obj): the face is
// 16 cm wide, and with a spread of 1 its rim reads the sky about 5 degrees further out
// than the look of the eye alone would.
constexpr float SHADE_HOOD_SKY_SPREAD = 1.0F;
constexpr glm::vec3 SHADE_HOOD_CENTRE{0.0F, 1.81F, 0.15F};

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

// How long the drawn shade takes to change between the standing and the walking pose.
constexpr float SHADE_SWAY_BLEND_SECONDS = 0.3F;

// The smallest height the dissolving figure is drawn with, as a part of its own: a model
// matrix must not scale by 0 (it could not be inverted for the normals).
constexpr float MIN_SHADE_DISSOLVE_SCALE = 0.02F;

// Puts the pose of the shade (game::shadeSwayPose) on its transform: a rise, and two
// turns around the feet, the forward lean after the turn to the player (Transform turns
// y, then x, then z).
void applyShadeSway(scene::Transform& transform, const ShadePose& pose) {
    transform.position.y += pose.riseMetres;
    transform.rotationDegrees.x = pose.forwardLeanDegrees;
    transform.rotationDegrees.z = pose.sideLeanDegrees;
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

void NightMazeApp::regenerateMaze(int night, std::uint32_t daily) {
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
    interactables.shadeLines = shadeInMaze(daily);
    // A night of the campaign has a mix of notes and story lines of its own. They go
    // into a copy: the request above stays the one of free play, with its counter, so
    // a game of free play after a night is what it was before.
    m_playedNight = night;
    m_playedDaily = daily;
    const InteractableSettings built =
        night == 0 ? interactables : campaignInteractables(night, interactables);

    // The height scale can be typed into its slider too.
    m_terrainSettings.heightScale =
        std::clamp(m_terrainSettings.heightScale, MIN_HEIGHT_SCALE, MAX_HEIGHT_SCALE);

    // Replaces the maze, its terrain, the model matrices, the collision boxes, the exit,
    // the crystals, the levers and the notes in one assignment. A new maze is a new
    // round.
    m_mazeWorld = buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed,
                                 m_heightmap, m_terrainSettings.heightScale, built,
                                 m_mazeSettings.crystalCount);
    // Until a new game or a night says otherwise (startNewGame, startNight), this is
    // a maze of no level.
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
    // The bell of the gate is silent until the gate opens again, and every wall a lever
    // had opened is back, so the ways to the exit are counted anew.
    m_gateBell = {};
    m_bellSwing = {};
    measureExitDistances();
    // A lever pulled in the round before is not a noise of this one.
    m_leverPulled = false;
    // A restart in the middle of a fade to black ends the fade.
    m_catchSeconds = -1.0F;
    // The steps of the player and of the shade count their metres from the beginning.
    m_footsteps = {};
    m_shadeSteps = {};

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
}

void NightMazeApp::onUpdate(double fixedDt) {
    // Remember where the player was before this step. It is done in every step, also
    // when the player does not move, so that onRender never blends with an old position.
    m_previousPlayerPosition = m_player.position;

    // The bell of the gatehouse swings on wherever the picture moves, like the crystals
    // bob: also behind the result screen, where the toll for walking through has just
    // pushed it. The pause stops it.
    if (animatesScene(m_mode)) {
        advanceBellSwing(m_bellSwing, static_cast<float>(fixedDt));
    }

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

    // A catch in progress: the picture fades to black and nothing else happens. The
    // player does not move, nothing is collected, the shade does not step and cannot catch
    // again, the battery does not drain. Only the animation clock goes on. At black the
    // round starts again. The pause stops this clock like the rest of the round.
    if (m_catchSeconds >= 0.0F) {
        m_round.animationSeconds += static_cast<float>(fixedDt);
        m_catchSeconds += static_cast<float>(fixedDt);
        if (catchPhase(m_catchSeconds) == CatchPhase::Black) {
            carryPlayerBack();
        }
        return;
    }

    // The keys reach the player only while the cursor is captured. It is captured when
    // a round starts or goes on (showScreen). Showing the debug panels gives it back, and
    // one click in the scene then switches on both mouse look and movement again.
    // Without the capture the struct stays as it is created: nothing is held.
    PlayerInput wanted;
    if (input().isCursorCaptured()) {
        wanted.forward = actionDown(KeyAction::Forward);
        wanted.backward = actionDown(KeyAction::Back);
        wanted.left = actionDown(KeyAction::Left);
        wanted.right = actionDown(KeyAction::Right);
        // Up and down are for flying (noclip) and sprint is for walking: the player
        // uses the fields that belong to its mode and ignores the others. So with the
        // default keys Left Shift has one meaning per mode: sprint when walking, down
        // when flying. Whether the sprint really happens is decided by the stamina
        // (Player::update).
        wanted.up = input().isKeyDown(NOCLIP_UP_KEY);
        wanted.down = input().isKeyDown(NOCLIP_DOWN_KEY);
        wanted.sprint = actionDown(KeyAction::Sprint);
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

    // The steps of the player, counted in the metres the feet really moved over the
    // ground in this step (m_previousPlayerPosition is the position before it, set at
    // the top). A player who stands, holds the map or pushes against a wall moved
    // nothing, and a flying one is silent (game::advanceFootsteps).
    const glm::vec3 walked = m_player.position - m_previousPlayerPosition;
    const float walkedMetres = glm::length(glm::vec2{walked.x, walked.z});
    CuePlay step;
    if (advanceFootsteps(m_footsteps,
                         {.metres = walkedMetres,
                          .stepSeconds = static_cast<float>(fixedDt),
                          .walkSpeed = m_player.walkSpeed,
                          .sprintSpeed = m_player.sprintSpeed,
                          .flying = m_player.noclip},
                         step)) {
        playCue(step.cue, step.volume);
    }

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
    // The heartstone was taken in this step: from now on the player is heavy. Like the
    // tea, that belongs to the stamina (game::carryHeartstone).
    if (m_round.heartstoneTaken && !soundBefore.heartstoneTaken) {
        carryHeartstone(m_player.stamina, m_player.staminaSettings);
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
    // The bell of the open gate, on a clock of the same kind: a toll every few seconds,
    // louder the fewer passages lie between the player and the exit.
    if (advanceGateBell(m_gateBell, m_round, m_gameplay.gate.bellSeconds,
                        static_cast<float>(fixedDt))) {
        playCue(SoundCue::GateBell, gateBellVolumeHere());
        tollBellSwing(m_bellSwing);
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
    // What the shade can hear of this step: the feet by the speed they really had,
    // a crystal or a flask that was picked up, and a lever pulled since the step before
    // (game::playerNoise). A player who stands or reads the map is silent.
    const Noise noise =
        playerNoise({.metres = walkedMetres,
                     .stepSeconds = static_cast<float>(fixedDt),
                     .walkSpeed = m_player.walkSpeed,
                     .sprintSpeed = m_player.sprintSpeed,
                     .flying = m_player.noclip,
                     .pickedUp = m_round.collectedCount > soundBefore.collectedCount ||
                                 m_round.flasksCollected > flasksBefore,
                     .pulledLever = m_leverPulled},
                    m_gameplay.shade);
    m_leverPulled = false;
    const ShadeEvents shadeEvents =
        updateRoundShade(m_round, m_mazeWorld, m_gameplay, m_player.position,
                         roundShadeLamp(m_lighting, m_round, lampPose), m_obstacles,
                         static_cast<float>(fixedDt), noise);
    const bool caught = shadeEvents.caught;
    // The moment it noticed the player, and the moment the beam burned it away: one
    // sound each. A round without a shade has neither.
    if (shadeEvents.alerted) {
        playCue(SoundCue::ShadeAlert);
    }
    if (shadeEvents.banished) {
        playCue(SoundCue::ShadeBanish);
    }
    // Its hum, on a clock like the pulse: more often the nearer the shade is.
    if (advanceShadeHum(m_shadeHum, m_round, static_cast<float>(fixedDt))) {
        playCue(SoundCue::ShadeNear);
    }
    // Its steps, while it walks and is near: quieter the farther it still has to go.
    if (advanceShadeSteps(m_shadeSteps, m_round, step)) {
        playCue(step.cue, step.volume);
    }
    // Caught: back to the start of the same maze. A player who flies (noclip, a tool for
    // looking around) is left alone. The steps that may follow in the same frame play
    // the new round.
    if (caught && !m_player.noclip) {
        m_catchSeconds = 0.0F;
        // The shade stands still from now on, so the picture of it must too: its last step
        // was drawn between two places (the frames blend them), and without this the
        // figure would swing between them for the whole fade.
        m_round.shade.previousPosition = m_round.shade.position;
        playCue(SoundCue::Caught);
        return;
    }

    // This step took the player through the open gate: the result screen comes up. The
    // steps that may follow in the same frame then find the round stopped.
    if (m_round.state == RoundState::Won) {
        // One full toll for walking through, whatever the clock of the bell says.
        playCue(SoundCue::GateBell, GATE_BELL_NEAR_VOLUME);
        tollBellSwing(m_bellSwing);
        finishRound();
    }
}

void NightMazeApp::measureExitDistances() {
    m_exitDistances = passageDistances(roundMaze(m_mazeWorld, m_round), m_mazeWorld.exitCell);
}

float NightMazeApp::gateBellVolumeHere() const {
    const Maze& maze = roundMaze(m_mazeWorld, m_round);
    const MazeCell cell = cellAt(m_player.position);
    if (!maze.contains(cell.x, cell.z)) {
        return gateBellVolume(UNREACHABLE);
    }
    // The list holds the rows one after another (game::passageDistances).
    const std::size_t index =
        static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(maze.width()) +
        static_cast<std::size_t>(cell.x);
    return gateBellVolume(m_exitDistances.at(index));
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

void NightMazeApp::onRender(double alpha) {
    // The menu buttons that were clicked since the last frame, and the controls of the
    // settings screen that were moved. The clicks arrived while the events of this
    // frame were read, so the screen they lead to is drawn in this very frame.
    handleMenuActions();
    handleControlChanges();
    handleKeyCapture();
    if (isLedger(m_mode)) {
        scrollLedger();
    }

    // The intro: the request of the debug UI to play it again, and its frame. Both come
    // before everything below, because the frame may end the intro: the rest of this
    // function then already draws the main menu.
    if (m_introRequested) {
        m_introRequested = false;
        startIntro();
    }
    if (isIntro(m_mode)) {
        updateIntro();
    }
    // What the debug UI asked of the campaign, and the frame of the title card of a night
    // or of the ending card: like the intro, a card may end in this frame.
    handleCampaignRequest();
    if (m_mode == GameMode::NightCard || m_mode == GameMode::EndingCard) {
        updateStoryCard();
    } else if (m_mode == GameMode::VillageBeat) {
        updateVillageBeat();
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
    // The wind of the maze: on or off for the screen the game is on now.
    updateAmbience(windowFocused);

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
    const bool roundInput = updatesRound(m_mode) && !m_menuCamera.enabled && m_catchSeconds < 0.0F;
    // The intro takes its pictures with the menu camera too, but it says itself which
    // shot, at which moment and with which light (game::introCamera).
    const bool intro = isIntro(m_mode);
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
    if (m_gameplay.restart || (roundInput && actionPressed(KeyAction::Restart))) {
        m_gameplay.restart = false;
        if (m_mode == GameMode::RoundEnd) {
            // The debug UI asked on the result screen: the button "Play again" of free
            // play does the same, a new round and the screen left.
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
            measureExitDistances();
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
    if (roundInput && actionPressed(KeyAction::Flashlight)) {
        playCue(flashlightKeyCue(m_round.battery, m_lighting.flashlightOn));
        m_lighting.flashlightOn = !m_lighting.flashlightOn;
    }

    // The map: on the screen while its key is held (mapShown). Asked once here for the
    // whole frame. While it is shown the mouse does not turn the camera and nothing can
    // be used: looking at the map is a stop, and the round goes on behind it.
    const bool map = mapShown() && m_catchSeconds < 0.0F;
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
    // The reveal of the village: after a night of the campaign is won the camera leaves
    // the eyes of the player, rises over the exit cell and turns to the ridge, with
    // a longer lens. Like the menu camera it changes the copy only. The eyes of the
    // player are kept: the flashlight stays in the hand down in the cell.
    const glm::vec3 playerEye = eye;
    const bool reveal = villageRevealShown() && !menuCamera;
    if (reveal) {
        m_revealSeconds += time().deltaSeconds();
        // Beside the card of the result, or in the middle of a picture without one.
        const float aside =
            m_mode == GameMode::RoundEnd ? VILLAGE_REVEAL_BESIDE_CARD_DEGREES : 0.0F;
        const VillageRevealPose pose =
            villageRevealPose(m_revealStart, m_mazeWorld.exitPosition, villageSide(m_mazeWorld),
                              aside, static_cast<float>(m_revealSeconds));
        eye = pose.eye;
        frameCamera.yawDegrees = pose.yawDegrees;
        frameCamera.pitchDegrees = pose.pitchDegrees;
        frameCamera.fovDegrees = pose.fovDegrees;
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
        // It looks at the player it knows of, and otherwise the way it walks.
        shade.rotationDegrees = {0.0F, shadeFacingDegrees(m_round.shade, feet), 0.0F};
        // Right after a banish the figure is still drawn where it was burned away, and
        // sinks into the ground there: its height is scaled, which needs no shader. The
        // shade of the rules is already in its new cell.
        if (m_round.shade.dissolveLeft > 0.0F) {
            shade.position = m_round.shade.banishedFrom;
            shade.scale.y =
                std::max(shadeDissolveHeight(m_round.shade.dissolveLeft), MIN_SHADE_DISSOLVE_SCALE);
        }
        // The sway is drawing only: it moves the picture of the figure, not the shade of
        // the round. It walks more or less as the state says, blended over a third of a
        // second (the frame time, as the pose is a picture and not a rule).
        const float blendStep =
            static_cast<float>(time().deltaSeconds()) / SHADE_SWAY_BLEND_SECONDS;
        const float target = shadeWalking(m_round.shade) && m_catchSeconds < 0.0F ? 1.0F : 0.0F;
        m_shadeWalkAmount += std::clamp(target - m_shadeWalkAmount, -blendStep, blendStep);
        applyShadeSway(shade, shadeSwayPose(m_shadeWalkAmount,
                                            shadeSpeed(m_round.shade.hunt, m_gameplay.shade),
                                            m_round.animationSeconds, m_gameplay.shade.sway));
        m_shadeMatrix = shade.matrix();
        // The stars in its hood get brighter as the beam burns it: the burn clock of the
        // rules, shown. No clock of the drawing is involved.
        const float burn = shadeBurnProgress(m_round.shade, m_gameplay.shade);
        m_shadeHoodBrightness = shadeHoodBrightness(burn);
        m_shadeHoodStarBoost = shadeHoodStarBoost(burn);
    }
    // The one picture outside a round that has a shade in it: the card of the intro that
    // warns of it. It stands still in the cell the script names, on the ground, turned
    // towards the camera, and the flashlight of the camera is on it. It is a prop: the
    // shade of the round is not moved or asked, so nothing hums and nobody is caught.
    // A calm night has none (game::introShadeCell).
    if (intro) {
        if (const std::optional<MazeCell> cell =
                introShadeCell(introMoment.card, introTellsOfShade())) {
            scene::Transform shade;
            shade.position = cellCenter(cell->x, cell->z);
            shade.position.y = m_mazeWorld.terrain.heightAt(shade.position.x, shade.position.z);
            shade.rotationDegrees = {0.0F, shadeYawDegrees(shade.position, eye), 0.0F};
            applyShadeSway(shade, shadeSwayPose(0.0F, 0.0F, static_cast<float>(m_introSeconds),
                                                m_gameplay.shade.sway));
            m_shadeMatrix = shade.matrix();
            m_shadeHoodBrightness = SHADE_HOOD_REST_BRIGHTNESS;
            m_shadeHoodStarBoost = 0.0F;
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

    // The lamp of the gate in this frame: how bright and in which colour, by the rule of
    // the round (game::gateLampStrength: a cold ember while the gate is closed, amber once
    // it is open). The menu camera and the live scene behind the main menu show it fully
    // lit, whatever the round says, so the glide over the maze has its one warm light.
    // The intro does not: it tells that the lamps of the village have gone out, so its
    // pictures keep the cold ember of a night that has not begun.
    const bool lampShownLit = menuCamera && !intro;
    const float lampProgress = lampShownLit ? 1.0F : m_round.gateProgress;
    const float lampStrength = lampShownLit
                                   ? 1.0F
                                   : gateLampStrength(m_round.collectedCount, m_round.requiredCount,
                                                      m_round.gateProgress, m_gameplay.gate);
    // The colour is an sRGB value and the glow a linear colour, like the one of the
    // crystals (crystalEmissive).
    m_gateLampGlow = gfx::srgbToLinear(gateLampColor(lampProgress)) *
                     (m_gameplay.gate.glowStrength * lampStrength);

    // Where the flashlight is and where it points in this frame: in the hand, a little
    // to the right of the eye and below it, aimed at a point in front of the eye. It is
    // computed here, after the mouse has turned the camera and from the same eye the
    // view matrix uses below. From m_camera.position (the last fixed step) the cone
    // would trail behind the picture while the player moves. It is computed ONCE: the
    // shadow pass of the flashlight and the lights of the frame both get this result,
    // so the shadows always belong to the light that is drawn.
    // During the reveal of the village the lamp stays where the player stands, aimed as
    // it was: the camera has left the hand that holds it.
    const FlashlightPose flashlight =
        reveal ? flashlightPose(frameLighting, playerEye, m_camera.forward(), m_camera.right())
               : flashlightPose(frameLighting, eye, frameCamera.forward(), frameCamera.right());

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
    //
    // The lamp of the gate is one more light in the same list, with a colour and
    // a radius of its own. So it takes one of the places only while it is among the
    // nearest: at the gate it lights the stone, and far away it leaves its place to
    // a crystal while its glass still glows.
    std::vector<PointLightSpot> pointLights;
    for (const glm::vec3& position : crystalLightPositions(m_round)) {
        pointLights.push_back({.position = position});
    }
    // The heartstone has a light of its own look too: the colour of the crystals,
    // brighter and reaching farther, pulsing with them (frameLighting is already dimmed
    // by the pulse).
    if (const std::optional<glm::vec3> base = heartstoneBase(m_mazeWorld, m_round)) {
        pointLights.push_back(
            {.position = heartstoneLightPosition(*base),
             .ownLook = true,
             .color = frameLighting.pointColor,
             .radius = frameLighting.pointRadius * HEARTSTONE_LIGHT_RADIUS_FACTOR,
             .intensity = frameLighting.pointIntensity * HEARTSTONE_LIGHT_INTENSITY_FACTOR});
    }
    if (m_mazeWorld.hasGate) {
        pointLights.push_back(gateLampLight(gateScenery(m_mazeWorld).lightPosition, lampStrength,
                                            lampProgress, m_gameplay.gate));
    }
    const std::vector<PointLightSpot> frameLights = nearestPointLightSpots(pointLights, eye);
    const scene::LightSet lights = buildLightSet(frameLighting, flashlight, frameLights);
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

    // The village on the ridge, before the sky and after everything of the maze: it lies
    // near the far plane, so the maze has already filled most of the pixels it would
    // cover. How many nights of it are lit is a rule of the campaign. The live scene
    // behind a menu and a maze of free play show the progress of the campaign, a night
    // shows itself, and the intro, which tells that the lamps have gone out, shows none.
    const bool ownNight = m_playedNight != 0 && !usesMenuCamera(m_mode);
    const bool nightWon = m_mode == GameMode::RoundEnd || m_mode == GameMode::VillageBeat ||
                          m_mode == GameMode::EndingCard;
    const int nightsLit = intro ? 0
                                : villageNightsLit(ownNight ? m_playedNight : 0, nightWon,
                                                   m_settings.campaignNight, m_toolRun);
    // The lights of the night that was just won come on while the reveal looks at them.
    const float newestLight =
        reveal ? villageNewLightStrength(static_cast<float>(m_revealSeconds)) : 1.0F;
    drawVillage(view, projection, frameCamera.farPlane, nightsLit, newestLight);

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
    compositeSettings.exposure *= pictureBrightness(m_catchSeconds, m_round);

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
    const bool keyPressed = actionPressed(KeyAction::Use);
    const bool clicked = input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);

    if ((keyPressed || clicked) && m_pick.action != Interaction::None) {
        // There is something to do: pull the lever, read the note or close the card.
        // A wall that opened is no obstacle any more. interact is true only then: for
        // the first pull of a lever, so that is the moment of its sound.
        if (interact(m_round, m_mazeWorld, m_pick)) {
            m_obstacles = roundObstacles(m_mazeWorld, m_round);
            // An open wall can be a shorter way to the exit: the bell follows it.
            measureExitDistances();
            playCue(SoundCue::LeverPull);
            // The shade may hear it, in the next fixed step.
            m_leverPulled = true;
        }
        // The round has changed, so the action is asked again: the lever that was just
        // pulled is not highlighted in this frame, and an opened card can be closed.
        m_pick.action = interactionFor(m_round, m_pick.picked);
        // The card of a story note has come up: its line is in the ledger from now on.
        markOpenNoteRead();
    } else if (clicked && !cursorCaptured) {
        // A click into the scene that hit nothing to use: it captures the cursor, which
        // switches on mouse look and movement. So a click with the free cursor ON
        // a lever or a note uses it, and any other click captures.
        input().setCursorCaptured(true);
    }
}

bool NightMazeApp::mapShown() {
    return showsMap(m_mode, {.keyHeld = actionDown(KeyAction::Map),
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
    m_gameplayRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_round, crystalEmissive(),
                            m_gateLampGlow, m_bellSwing.degrees);
    m_gameplayRenderer.drawFlasks(m_shadowDepthShader, m_mazeWorld, m_round);
    // The shade casts a shadow like everything that stands in the maze.
    if (m_shadeDrawn) {
        m_gameplayRenderer.drawShade(m_shadowDepthShader, m_shadeMatrix);
        m_gameplayRenderer.drawShadeHollow(m_shadowDepthShader, m_shadeMatrix);
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
    m_gameplayRenderer.drawGate(shader, m_mazeWorld, m_round, m_gateLampGlow, m_bellSwing.degrees);
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
        // The night in its hood and its cuffs are drawn exactly once per frame, like the
        // crystals: here as dark cloth, or later by drawReflections as the night sky.
        if (!crystalsReflect()) {
            m_gameplayRenderer.drawShadeHollow(shader, m_shadeMatrix);
        }
        shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    }
    // Every crystal is drawn exactly once per frame: here, with the program of the
    // walls, or later by drawReflections with the reflect program.
    if (!crystalsReflect()) {
        m_gameplayRenderer.drawCrystals(shader, m_mazeWorld, m_round, crystalEmissive());
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
    m_gameplayRenderer.drawCrystals(m_reflectShader, m_mazeWorld, m_round,
                                    crystalEmissive() * m_environment.crystalGlowShare);

    // The face in the hood of the shade and its cuffs: a piece of the night. All of
    // their colour is the sky (a strength of 1, so the beam of the flashlight does not
    // light them), read where the eye looks and not where a mirror would send the look:
    // the refracted picture alone, with a ratio that bends nothing. The sky is turned
    // by SHADE_HOOD_SKY_TURN, so these are not the stars behind the figure, and is as
    // bright as the burn clock says, its stars lifted most. The faces are flat: no normal
    // map. The turn, the star boost and the brightness are put back for the puddles. The strength,
    // the shares and the normal map they set themselves.
    if (m_shadeDrawn) {
        m_reflectShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, 0);
        m_reflectShader.setFloat(ENVIRONMENT_STRENGTH_UNIFORM, SKY_ONLY);
        m_reflectShader.setFloat(ENVIRONMENT_REFLECT_SHARE_UNIFORM, SEEN_THROUGH_ONLY);
        m_reflectShader.setFloat(ENVIRONMENT_REFRACTION_RATIO_UNIFORM, NO_REFRACTION);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_TURN_UNIFORM, SHADE_HOOD_SKY_TURN);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_BRIGHTNESS_UNIFORM,
                                 m_skyboxSettings.brightness * m_shadeHoodBrightness);
        m_reflectShader.setFloat(ENVIRONMENT_STAR_BOOST_UNIFORM, m_shadeHoodStarBoost);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_SPREAD_UNIFORM, SHADE_HOOD_SKY_SPREAD);
        m_reflectShader.setVec3(ENVIRONMENT_SKY_SPREAD_CENTRE_UNIFORM,
                                glm::vec3{m_shadeMatrix * glm::vec4{SHADE_HOOD_CENTRE, 1.0F}});
        m_gameplayRenderer.drawShadeHollow(m_reflectShader, m_shadeMatrix);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_TURN_UNIFORM, 0.0F);
        m_reflectShader.setFloat(ENVIRONMENT_STAR_BOOST_UNIFORM, 0.0F);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_SPREAD_UNIFORM, 0.0F);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_BRIGHTNESS_UNIFORM, m_skyboxSettings.brightness);
    }

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

void NightMazeApp::drawVillage(const glm::mat4& view, const glm::mat4& projection, float farPlane,
                               int nightsLit, float newestStrength) const {
    if (!m_villageSettings.enabled || !m_mazeWorld.hasGate || !m_texturedShader.isValid()) {
        return;
    }
    // The state this draw relies on, said here and not taken from the pass before it: the
    // puddles are blended and write no depth, and the village must write its depth, or
    // the sky, drawn next at the largest depth, would paint over it.
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDepthMask(GL_TRUE));
    GL_CHECK(glDisable(GL_BLEND));

    m_texturedShader.use();
    // The view matrix without its translation, as in skybox.vert: the village is centred
    // on the eye and never comes closer. It keeps its real depth, unlike the sky.
    m_texturedShader.setMat4(VIEW_UNIFORM, glm::mat4{glm::mat3{view}});
    m_texturedShader.setMat4(PROJECTION_UNIFORM, projection);
    m_texturedShader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));
    m_texturedShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    setModelSamplers(m_texturedShader);
    m_villageRenderer.draw(
        m_texturedShader,
        villageMatrix(villageSide(m_mazeWorld), farPlane, m_villageSettings.elevationDegrees),
        nightsLit, newestStrength);
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
