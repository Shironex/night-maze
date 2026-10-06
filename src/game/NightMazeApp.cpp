// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#include "game/NightMazeApp.hpp"

#include "assets/ImageLoader.hpp"
#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "game/Crystals.hpp"
#include "game/Interactables.hpp"
#include "game/Puddles.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "scene/Raycast.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <span>
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

// Key that shows and hides the minimap.
constexpr int MINIMAP_KEY = GLFW_KEY_M;

// Key that uses what the picking ray points at: pulls a lever, reads a note, closes the
// card of a note. A left click does the same.
constexpr int INTERACT_KEY = GLFW_KEY_E;

// Key that switches the menu camera on and off: the game then shows itself.
constexpr int MENU_CAMERA_KEY = GLFW_KEY_F2;

// The documents of the menus, relative to the assets directory, and the two elements
// of the result screen the code writes into (their id attributes).
constexpr const char* MAIN_MENU_DOCUMENT_FILE = "ui/main_menu.rml";
constexpr const char* PAUSE_DOCUMENT_FILE = "ui/pause.rml";
constexpr const char* ROUND_END_DOCUMENT_FILE = "ui/round_end.rml";
constexpr const char* ROUND_END_TIME_ID = "time";
constexpr const char* ROUND_END_CRYSTALS_ID = "crystals";

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
      // The seed of the first maze, from the command line. The size and the numbers of
      // levers and notes keep their defaults.
      m_mazeSettings{.seed = options.seed},
      m_heightmap(loadHeightmap()),
      m_mazeWorld(buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed,
                                 m_heightmap, m_terrainSettings.heightScale,
                                 m_mazeSettings.interactables)),
      m_menuCamera(options.menuCamera),
      // The menu camera is a tool for recording the game: with it the main menu is
      // skipped like with --play, so no menu ever lies over the recorded picture.
      m_mode(options.play || options.menuCamera.enabled ? GameMode::Playing : GameMode::MainMenu),
      m_newGame{.seed = options.seed},
      m_ui(window()) {
    // The two lit programs and the grass program read the lights from the uniform buffer
    // of m_lightRig. Each program is told once: the shader repeats it by itself after
    // a reload.
    m_lightRig.connect(m_litShader);
    m_lightRig.connect(m_gouraudShader);
    m_lightRig.connect(m_grassShader);
    // The reflect program lights the crystals and the puddles with the same lights.
    m_lightRig.connect(m_reflectShader);

    // The first maze was built in the initializer list, because MazeWorld cannot be
    // created empty. What is left is the same as after every later regeneration.
    uploadGround();
    beginRound();
    m_menuCameraPath = buildMenuCameraPath(m_mazeWorld);

    // The documents of the three screens with a menu. They are loaded once and stay
    // hidden until their screen comes up. An error is in the log (ui::UiLayer).
    m_mainMenuDocument = m_ui.loadDocument(MAIN_MENU_DOCUMENT_FILE);
    m_pauseDocument = m_ui.loadDocument(PAUSE_DOCUMENT_FILE);
    m_roundEndDocument = m_ui.loadDocument(ROUND_END_DOCUMENT_FILE);
    m_menusLoaded = m_mainMenuDocument != ui::NO_DOCUMENT && m_pauseDocument != ui::NO_DOCUMENT &&
                    m_roundEndDocument != ui::NO_DOCUMENT;
    if (!m_menusLoaded) {
        core::logError("The menus cannot be shown: the game starts straight in a round");
        m_mode = GameMode::Playing;
    }
    showScreen();
}

void NightMazeApp::onEscapePressed() {
    handleGameEvent(GameEvent::Escape);
}

void NightMazeApp::handleGameEvent(GameEvent event) {
    const GameMode before = m_mode;
    // Asked before the screen changes: the answer depends on the screen the event
    // was sent on.
    const bool newRound = startsRound(before, event);
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
    if (event == GameEvent::Play) {
        startNewGame(m_newGame);
    } else if (newRound || roundInsteadOfMenu) {
        beginRound();
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
            handleGameEvent(event);
        } else {
            core::logWarn("A menu button has an unknown action: " + action);
        }
    }
}

void NightMazeApp::showScreen() {
    ui::DocumentId document = ui::NO_DOCUMENT;
    if (m_mode == GameMode::MainMenu) {
        document = m_mainMenuDocument;
    } else if (m_mode == GameMode::Paused) {
        document = m_pauseDocument;
    } else if (m_mode == GameMode::RoundEnd) {
        document = m_roundEndDocument;
        fillRoundEndDocument();
    }
    m_ui.show(document);

    // The cursor follows the screen: captured for mouse look while a round is played,
    // free for the buttons of a menu. The menu camera does not turn with the mouse, so
    // it leaves the cursor free too.
    input().setCursorCaptured(updatesRound(m_mode) && !m_menuCamera.enabled);
}

void NightMazeApp::startNewGame(const NewGame& newGame) {
    // newGame.difficulty is not read yet: the three levels have no numbers so far.
    // The maze is always built again, also for the seed that is in play: the size and
    // the numbers of levers and notes in m_mazeSettings may have been changed since.
    m_mazeSettings.seed = newGame.seed;
    m_mazeSettings.regenerate = false;
    regenerateMaze();
}

void NightMazeApp::fillRoundEndDocument() {
    m_ui.setText(m_roundEndDocument, ROUND_END_TIME_ID, timeText(m_round.elapsedSeconds));
    m_ui.setText(m_roundEndDocument, ROUND_END_CRYSTALS_ID,
                 std::to_string(m_round.collectedCount) + " / " +
                     std::to_string(m_round.crystals.size()));
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

    // The height scale can be typed into its slider too.
    m_terrainSettings.heightScale =
        std::clamp(m_terrainSettings.heightScale, MIN_HEIGHT_SCALE, MAX_HEIGHT_SCALE);

    // Replaces the maze, its terrain, the model matrices, the collision boxes, the exit,
    // the crystals, the levers and the notes in one assignment. A new maze is a new
    // round.
    m_mazeWorld = buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed,
                                 m_heightmap, m_terrainSettings.heightScale, interactables);
    // The button "Play" starts the maze that is in play again, also when this one was
    // asked for by the debug UI with another seed.
    m_newGame.seed = m_mazeSettings.seed;
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
    // The state of the round: every crystal back, a full battery, the gate closed, no
    // lever pulled and so every wall back in the obstacle list.
    m_round = startRound(m_mazeWorld, m_gameplay);
    m_obstacles = roundObstacles(m_mazeWorld, m_round);
    // The picking of the round before may name a lever the new maze does not have.
    m_pick = pickNothing(m_round);
    m_shownPick = m_pick;
    // A round starts with the light on, also after one that ended in the dark.
    m_lighting.flashlightOn = true;

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

    // The keys reach the player only while the cursor is captured: one click in the scene
    // switches on both mouse look and movement, Escape switches both off. Without the
    // capture the struct stays as it is created: nothing is held.
    PlayerInput wanted;
    if (input().isCursorCaptured()) {
        wanted.forward = input().isKeyDown(GLFW_KEY_W);
        wanted.backward = input().isKeyDown(GLFW_KEY_S);
        wanted.left = input().isKeyDown(GLFW_KEY_A);
        wanted.right = input().isKeyDown(GLFW_KEY_D);
        wanted.up = input().isKeyDown(GLFW_KEY_SPACE);
        // Left Shift has one meaning per mode: sprint when walking, down when flying.
        // The player uses the field that belongs to its mode and ignores the other.
        wanted.down = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
        wanted.sprint = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
    }

    // The step runs also with nothing held: it is what brings the feet back to the
    // ground after noclip was switched off in a panel.
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
    updateRound(m_round, m_mazeWorld, m_gameplay, m_player.position, m_lighting.flashlightOn,
                static_cast<float>(fixedDt));
    // The gate has just opened (the only change a step can make here): its box leaves
    // the obstacle list, and the way into the exit cell is free.
    if (gateBlocks(m_mazeWorld, m_round) != gateBlockedBefore) {
        m_obstacles = roundObstacles(m_mazeWorld, m_round);
    }

    // This step took the player through the open gate: the result screen comes up. The
    // steps that may follow in the same frame then find the round stopped.
    if (m_round.state == RoundState::Won) {
        handleGameEvent(GameEvent::RoundWon);
    }
}

void NightMazeApp::onRender(double alpha) {
    // The menu buttons that were clicked since the last frame. The clicks arrived
    // while the events of this frame were read, so the screen they lead to is drawn
    // in this very frame.
    handleMenuActions();

    // A new maze asked for by the debug UI is built here, at the start of a frame and
    // outside of the fixed steps, so no step ever sees a half replaced maze.
    if (m_mazeSettings.regenerate) {
        m_mazeSettings.regenerate = false;
        regenerateMaze();
    }

    // The menu camera: its key, and what has to happen when it was just switched on.
    updateMenuCameraSwitch();
    // Two questions the rest of the frame asks. roundInput: do the keys and the mouse
    // of the round work? Only while a round is played and the menu camera is off, so
    // under a menu no key of the round does anything. menuCamera: is the picture taken
    // by the menu camera? While it is switched on, and in the main menu, where it
    // stands in for the background the menu will get later.
    const bool roundInput = updatesRound(m_mode) && !m_menuCamera.enabled;
    const bool menuCamera = m_menuCamera.enabled || usesMenuCamera(m_mode);
    // The settings the menu camera uses in this frame. The main menu always shows the
    // high glide over the maze, whatever shot the recording tool is set to.
    MenuCameraSettings menuCameraSettings = m_menuCamera;
    if (!m_menuCamera.enabled) {
        menuCameraSettings.shot = MenuShot::HighGlide;
    }

    // A new round on the same maze, asked for with the restart key or by the debug UI.
    // It is started here for the same reason: between two fixed steps, never inside one.
    // wasKeyPressed is true for one frame, so the key is read once per frame.
    if (m_gameplay.restart || (roundInput && input().wasKeyPressed(RESTART_KEY))) {
        m_gameplay.restart = false;
        beginRound();
    }

    // Every lever at once, asked for by a button of the debug UI. Handled here like the
    // restart: between two fixed steps. Open walls leave the obstacle list.
    if (m_gameplay.pullAllLevers) {
        m_gameplay.pullAllLevers = false;
        if (pullAllLevers(m_round, m_mazeWorld) > 0) {
            m_obstacles = roundObstacles(m_mazeWorld, m_round);
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
    // (game::lightingForFrame).
    if (roundInput && input().wasKeyPressed(FLASHLIGHT_KEY)) {
        m_lighting.flashlightOn = !m_lighting.flashlightOn;
    }

    // The minimap key, read once per frame like the two keys above.
    if (roundInput && input().wasKeyPressed(MINIMAP_KEY)) {
        m_minimapSettings.enabled = !m_minimapSettings.enabled;
    }

    // Mouse look. It runs here, once per frame, and not in onUpdate: a mouse delta
    // describes one frame, and onUpdate runs zero or more times per frame. It comes
    // before the view matrix is built, so the picture and the picking ray of this frame
    // already use the new angles. The click that captures a free cursor is read further
    // down (handleInteraction), because it first has to be known what the click hit.
    const bool cursorCaptured = input().isCursorCaptured();
    if (cursorCaptured && roundInput) {
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
    if (menuCamera) {
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
    if (!roundInput) {
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
    // there. The copy to the graphics card happens once, and the two lit programs and
    // the grass program read it. It takes the EYE, not the hand: the highlights are
    // computed for the place the picture is taken from.
    const std::vector<glm::vec3> crystalLights = crystalLightPositions(m_round);
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

    // The minimap comes after the composite pass, on top of the finished picture: it is
    // a schematic, and the fog, the bloom and the tone mapping must not touch it. It is
    // shown in the two debug views too, like the HUD. feet is the blended position the
    // scene was drawn from, so the arrow moves as smoothly as the camera. The picture of
    // the menu camera has no minimap: it is part of the HUD of a round. In the pause
    // menu it stays, as a part of the stopped picture (game::showsMinimap).
    if (showsMinimap(m_mode) && !m_menuCamera.enabled) {
        drawMinimap(framebuffer, feet);
    }

    // The menu comes last in this function, on top of the finished frame and of the
    // minimap: a menu covers the game and its map. The debug UI is drawn after this
    // function returns (main.cpp), so its panels stay usable on top of a menu. While
    // a round is played no document is shown and this call does nothing.
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
        // A wall that opened is no obstacle any more.
        if (interact(m_round, m_mazeWorld, m_pick)) {
            m_obstacles = roundObstacles(m_mazeWorld, m_round);
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

void NightMazeApp::drawMinimap(core::Size framebuffer, const glm::vec3& feet) {
    if (!m_minimapSettings.enabled) {
        return;
    }

    // Where the map stands, in framebuffer pixels. Its picture is drawn at exactly
    // that size, so the framebuffer of the minimap follows the window and the size
    // setting. A size of 0: the window has no room for a map.
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
                             minimapMetresPerPixel(maze, rect.size));

    // The offscreen pass, and then the picture into its corner of the window.
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
    m_mazeRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_wallMatrices);
    m_gameplayRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_round, crystalEmissive());
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
    m_mazeRenderer.draw(m_texturedShader, m_mazeWorld, m_wallMatrices);
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
    m_mazeRenderer.draw(shader, m_mazeWorld, m_wallMatrices);
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
