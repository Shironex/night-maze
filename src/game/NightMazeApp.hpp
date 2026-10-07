// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#pragma once

#include "assets/AssetCache.hpp"
#include "audio/AudioEngine.hpp"
#include "core/Application.hpp"
#include "game/ColliderLines.hpp"
#include "game/Difficulty.hpp"
#include "game/EnvironmentMapping.hpp"
#include "game/GameState.hpp"
#include "game/GameplayRenderer.hpp"
#include "game/Grass.hpp"
#include "game/GrassRenderer.hpp"
#include "game/InteractableRenderer.hpp"
#include "game/Interaction.hpp"
#include "game/LightRig.hpp"
#include "game/Lighting.hpp"
#include "game/MazeRenderer.hpp"
#include "game/MazeWorld.hpp"
#include "game/MenuBackgroundRenderer.hpp"
#include "game/MenuCamera.hpp"
#include "game/Minimap.hpp"
#include "game/MinimapRenderer.hpp"
#include "game/Player.hpp"
#include "game/PostProcess.hpp"
#include "game/PuddleRenderer.hpp"
#include "game/Round.hpp"
#include "game/Settings.hpp"
#include "game/ShadowMap.hpp"
#include "game/Shadows.hpp"
#include "game/Skybox.hpp"
#include "game/SoundCues.hpp"
#include "game/StartOptions.hpp"
#include "game/Terrain.hpp"
#include "game/TerrainRenderer.hpp"
#include "gfx/Shader.hpp"
#include "scene/Camera.hpp"
#include "scene/Collider.hpp"
#include "scene/LightSpace.hpp"
#include "ui/UiLayer.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace game {

/// The game itself: a generated maze of textured walls and pillars at night, standing
/// on gently uneven ground that rises into hills around it (a heightmap terrain), and
/// a player who walks through it in first person without passing through the walls.
/// The mouse turns the camera, the keyboard moves the player. The maze is lit by the
/// moon, by the flashlight of the player and by the glowing crystals. Grass grows along
/// the walls, and above them is the night sky, a skybox.
///
/// The moon and the flashlight cast shadows. Before the scene, everything that casts
/// one is drawn twice into a depth texture (game::ShadowMap): from the direction of the
/// moon and from the hand that holds the flashlight. The lit programs look every
/// fragment up in both.
///
/// The scene is not drawn into the window directly. It is drawn into an HDR framebuffer
/// (game::PostProcess). Its bright parts are blurred into a glow (bloom), and a last
/// pass brings the picture to the window with fog near the ground, that glow, exposure,
/// tone mapping, a vignette and gamma correction.
///
/// On top of that finished picture comes the minimap (game::MinimapRenderer): a schematic
/// of the corridors the player has discovered, drawn into a framebuffer of its own and
/// shown in a corner of the window.
///
/// A round: the player collects crystals, each one charges the battery of the
/// flashlight, and when enough of them are collected the gate of the exit opens.
/// Walking through it wins the round. The rules are in game/Round.hpp, this class feeds
/// them the position of the player and draws their state.
///
/// On the walls hang levers and notes. In every frame a ray is cast from the eye through
/// the middle of the picture (or through the free cursor), and the lever or the note it
/// hits is highlighted: object picking by ray casting. The key E or a click pulls the
/// lever, which lowers a wall somewhere in the maze, or opens the card of the note. The
/// rules are in game/Interaction.hpp, this class builds the ray and reads the key.
///
/// The game can also show itself: with the menu camera switched on (key F2, the Camera
/// panel or the switch --menu-camera) the picture is taken by a camera that travels
/// through the maze alone (game/MenuCamera.hpp). The round stands still meanwhile, and
/// the keys and the mouse of the round are ignored.
///
/// The game is always on one screen (game/GameState.hpp): the main menu, a round being
/// played, the pause menu, the result of a won round or the settings. The round runs
/// only while it is played. The menus are documents drawn by RmlUi (ui::UiLayer) on top
/// of the finished frame. Their buttons and the Escape key are events, and
/// game::nextMode says which screen follows.
///
/// Behind the main menu plays a recorded video loop of the game
/// (game::MenuBackgroundRenderer), or a still picture when the video cannot be played.
/// Such a background covers the whole window, so in those frames the scene is not drawn
/// at all (game::drawsScene): no shadow maps, no HDR picture, no bloom.
///
/// A new game has a difficulty (game/Difficulty.hpp), which decides the size of the
/// maze, its crystals, the gate and the battery, and a seed, which decides the maze.
/// What the player sets on the settings screen (game/Settings.hpp) is used at once and
/// kept in a small text file in the working directory.
///
/// The game has short sounds (the click of the flashlight, a crystal, a lever, the
/// gate, the warning of a low battery). WHICH sound belongs to what happened is decided
/// by the rules in game/SoundCues.hpp. This class asks them at the few places where
/// something happens and hands the answer to audio::AudioEngine (playCue).
///
/// It knows nothing about the debug UI: main.cpp derives from this class and draws the
/// debug panels and the HUD on top of the frame.
class NightMazeApp : public core::Application {
public:
    /// Creates the window (through core::Application), loads the shaders and the models,
    /// generates the first maze and starts the first round in it. options comes from
    /// the command line (game::parseStartOptions): the seed of that first maze, the
    /// settings the menu camera starts with, whether the main menu is skipped and what
    /// is behind it. Left out, the game starts in the main menu with its video.
    explicit NightMazeApp(const StartOptions& options = {});

    /// Writes the settings file when a setting changed and was not written yet: the
    /// last chance when the window is closed while the settings screen is open.
    ~NightMazeApp() override;

protected:
    void onUpdate(double fixedDt) override;
    void onRender(double alpha) override;

    /// Escape goes one screen back (game::nextMode): out of the game into the pause
    /// menu, out of the pause menu back into the game.
    void onEscapePressed() override;

    /// The screen the game is on, for the debug UI.
    GameMode gameMode() const { return m_mode; }

    /// Whether the HUD of the round is to be drawn in this frame: only while a round is
    /// played and the menu camera is off. The HUD is drawn by the debug UI layer
    /// (main.cpp), which asks here.
    bool hudVisible() const { return showsHud(m_mode) && !m_menuCamera.enabled; }

    /// The layer that draws the menu documents, exposed so main.cpp can keep the mouse
    /// away from it while a debug panel is under the cursor and can ask whether a text
    /// field of a menu has the keyboard.
    ui::UiLayer& menuUi() { return m_ui; }

    /// Clear color (red, green, blue, as sRGB values), exposed so the debug UI can edit
    /// it live. It is the background only where the sky is not drawn: with the skybox
    /// switched off.
    std::array<float, 3>& clearColor() { return m_clearColor; }

    /// Shader program of the scene without lighting and of its debug views (textured
    /// models), exposed so the debug UI can reload it live.
    gfx::Shader& texturedShader() { return m_texturedShader; }

    /// Shader program of the lines of the collision boxes and spheres, exposed for the
    /// same reason.
    gfx::Shader& colorShader() { return m_colorShader; }

    /// Shader program of the lit scene with lighting per fragment (Phong and
    /// Blinn-Phong), exposed for the same reason.
    gfx::Shader& litShader() { return m_litShader; }

    /// Shader program of the lit scene with lighting per vertex (Gouraud), exposed for
    /// the same reason.
    gfx::Shader& gouraudShader() { return m_gouraudShader; }

    /// Shader program of the sky, exposed for the same reason.
    gfx::Shader& skyboxShader() { return m_skyboxShader; }

    /// Shader program of the grass (with a geometry stage), exposed for the same reason.
    gfx::Shader& grassShader() { return m_grassShader; }

    /// Shader program of the composite pass (the HDR picture to the window), exposed
    /// for the same reason.
    gfx::Shader& compositeShader() { return m_compositeShader; }

    /// Shader program of the attachment previews of the debug UI, exposed for the same
    /// reason.
    gfx::Shader& previewShader() { return m_previewShader; }

    /// Shader program of the bright pass of the bloom, exposed for the same reason.
    gfx::Shader& brightPassShader() { return m_brightPassShader; }

    /// Shader program of the blur passes of the bloom, exposed for the same reason.
    gfx::Shader& blurShader() { return m_blurShader; }

    /// Shader program of the depth pass of the shadow maps, exposed for the same reason.
    gfx::Shader& shadowDepthShader() { return m_shadowDepthShader; }

    /// Shader program that draws the minimap into its framebuffer, exposed for the same
    /// reason.
    gfx::Shader& minimapShader() { return m_minimapShader; }

    /// Shader program that puts the picture of the minimap into the window, exposed for
    /// the same reason.
    gfx::Shader& minimapOverlayShader() { return m_minimapOverlayShader; }

    /// The settings of the minimap (switch, reveal all, size, margin, corner, opacity),
    /// exposed so the debug UI can edit them live.
    MinimapSettings& minimapSettings() { return m_minimapSettings; }

    /// The minimap, read only: the debug UI shows the size, the format and the picture
    /// of its framebuffer.
    const MinimapRenderer& minimapRenderer() const { return m_minimapRenderer; }
    /// Shader program of the surfaces that show the sky (crystals and puddles), exposed
    /// for the same reason.
    gfx::Shader& reflectShader() { return m_reflectShader; }

    /// The settings of the environment mapping (the sky on the crystals and on the
    /// puddles), exposed so the debug UI can edit them live.
    EnvironmentSettings& environmentSettings() { return m_environment; }

    /// How many puddles lie in the maze, for the debug UI.
    std::size_t puddleCount() const { return m_puddleRenderer.puddleCount(); }

    /// The settings of the shadows of the moon (switch, resolution, bias, PCF,
    /// strength), exposed so the debug UI can edit them live.
    ShadowSettings& moonShadowSettings() { return m_moonShadow; }

    /// The shadow map of the moon, read only: the debug UI shows its size, its format
    /// and its preview picture.
    const ShadowMap& moonShadowMap() const { return m_moonShadowMap; }

    /// The view and the projection the shadow map of the moon was drawn with in the
    /// last frame, read only: the debug UI shows how much ground the map covers.
    const scene::LightSpace& moonLightSpace() const { return m_moonLightSpace; }

    /// The settings of the shadows of the flashlight, exposed like the ones of the
    /// moon.
    ShadowSettings& flashlightShadowSettings() { return m_flashlightShadow; }

    /// The shadow map of the flashlight, read only, like the one of the moon.
    const ShadowMap& flashlightShadowMap() const { return m_flashlightShadowMap; }

    /// The view and the projection the shadow map of the flashlight was drawn with in
    /// the last frame, read only: the debug UI shows how much the map covers.
    const scene::LightSpace& flashlightLightSpace() const { return m_flashlightLightSpace; }

    /// Whether the shadow map of the flashlight was drawn in the last frame. It is not
    /// with its shadows switched off, and not while the flashlight itself is off (key
    /// F or an empty battery). The debug UI then shows no picture of the map.
    bool flashlightShadowDrawn() const { return m_flashlightShadowDrawn; }

    /// The exposure, the tone mapping and the preview switch of the composite pass and
    /// the settings of the bloom, the fog and the vignette, exposed so the debug UI can
    /// edit them live.
    PostProcessSettings& postProcessSettings() { return m_postProcessSettings; }

    /// The framebuffers of the frame, read only: the debug UI shows their sizes, their
    /// formats and pictures of their attachments.
    const PostProcess& postProcess() const { return m_postProcess; }

    /// The height scale and the wireframe switch of the terrain, exposed so the debug UI
    /// can edit them live.
    TerrainSettings& terrainSettings() { return m_terrainSettings; }

    /// The switch, the density, the blade height and the wind of the grass, exposed so
    /// the debug UI can edit them live.
    GrassSettings& grassSettings() { return m_grassSettings; }

    /// How many tufts of grass are on the graphics card, for the debug UI.
    std::size_t grassTuftCount() const { return m_grassRenderer.tuftCount(); }

    /// The switch and the brightness of the sky, exposed so the debug UI can edit them
    /// live.
    SkyboxSettings& skyboxSettings() { return m_skyboxSettings; }

    /// The settings of the lighting (mode, moon, flashlight, point lights, highlight,
    /// normal mapping), exposed so the debug UI can edit them live.
    LightingSettings& lighting() { return m_lighting; }

    /// The camera, exposed so the debug UI can show and edit its angles and projection
    /// live. Its position follows the eyes of the player, see player().
    scene::Camera& camera() { return m_camera; }

    /// Mouse look sensitivity in degrees per screen coordinate unit of mouse movement,
    /// exposed so the debug UI can edit it live.
    float& mouseSensitivity() { return m_mouseSensitivity; }

    /// The player, exposed so the debug UI can show and edit its position, its speeds
    /// and the noclip mode live.
    Player& player() { return m_player; }

    /// The request for the next maze, exposed so the debug UI can ask for a new one.
    MazeSettings& mazeSettings() { return m_mazeSettings; }

    /// The maze in play, read only: the debug UI draws its plan and counts its boxes.
    const MazeWorld& mazeWorld() const { return m_mazeWorld; }

    /// The numbers of the rules of a round and the request for a restart, exposed so
    /// the debug UI can edit them live.
    GameplaySettings& gameplaySettings() { return m_gameplay; }

    /// The round in play, exposed so the HUD can show it and the debug UI can set the
    /// charge of the battery.
    Round& round() { return m_round; }

    /// The loaded models and textures, exposed so the debug UI can list them and change
    /// the texture filtering live.
    assets::AssetCache& assets() { return m_assets; }

    /// What the textured shader shows, exposed so the debug UI can switch it live.
    ViewMode& viewMode() { return m_viewMode; }

    /// Whether the collision boxes and spheres are drawn as lines, exposed so the debug
    /// UI can switch it live.
    bool& drawColliders() { return m_drawColliders; }

    /// The picking of the last frame (the ray, what it hit and what the interaction key
    /// does), read only: the HUD shows the crosshair and the prompt from it, the debug UI
    /// its numbers.
    const PickState& pick() const { return m_pick; }

    /// The switches of the debug view of the picking (draw the pick boxes and the ray,
    /// freeze the drawn ray), exposed so the debug UI can switch them live.
    PickDebugSettings& pickDebug() { return m_pickDebug; }

    /// The settings of the menu camera (switch, shot, speed, eye height, time offset),
    /// exposed so the debug UI can edit them live and can hide itself while the camera
    /// runs.
    MenuCameraSettings& menuCameraSettings() { return m_menuCamera; }

    /// How long one loop of the menu camera takes with the present maze and settings,
    /// in seconds (game::menuCameraLoopSeconds), for the debug UI.
    float menuCameraLoopSeconds() const {
        return game::menuCameraLoopSeconds(m_menuCameraPath, m_mazeWorld, m_menuCamera);
    }

    /// The sound device and the loaded sounds, exposed so the debug UI can show their
    /// status and set the master volume.
    audio::AudioEngine& audio() { return m_audio; }

    /// The name of the cue that was played last (game::soundCueName), "none" before the
    /// first one, and how many cues were played since the start, for the debug UI.
    const char* lastCueName() const { return m_lastCueName; }
    int cuesPlayed() const { return m_cuesPlayed; }

private:
    // Camera turn for one screen coordinate unit of mouse movement, in degrees. The mouse
    // is measured in the units of the window size, not in framebuffer pixels, so the same
    // hand movement turns the camera equally on a Retina display.
    static constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1F;

    /// Builds the maze described by m_mazeSettings on its terrain and starts a round in
    /// it (beginRound).
    void regenerateMaze();

    /// Builds the terrain of the maze in play again with the height scale of
    /// m_terrainSettings and puts everything back on it: the walls, the gate, the
    /// crystals of the round, the obstacle list and the walking player. The round goes
    /// on: nothing is collected or reset.
    void rebuildTerrain();

    /// Copies what stands on the ground of m_mazeWorld to the graphics card: the mesh of
    /// the terrain and the points of the grass (plantGrass).
    void uploadGround();

    /// Chooses the places of the grass tufts again (game::placeGrass) and copies them to
    /// the graphics card.
    void plantGrass();

    /// Chooses the puddles again (game::puddlesOnGround) and hands them to the class
    /// that draws them. The same seed gives the same puddles, so after a new height
    /// scale they only move up or down with the ground.
    void layPuddles();

    /// Reads the key of the menu camera and notices when the camera was switched on or
    /// off, by that key, by the debug UI or by the command line. Switching it on starts
    /// its shot from the beginning and gives the cursor back. Called once per frame.
    void updateMenuCameraSwitch();

    /// Sends an event to the screen of the game (game::nextMode) and does what the new
    /// screen needs: a round from the beginning, a new maze for a new game, the
    /// settings file written, a fresh seed for the main menu, the window closed. Then
    /// showScreen.
    void handleGameEvent(GameEvent event);

    /// Takes the names of the menu buttons that were clicked since the last frame
    /// (ui::UiLayer::takeActions). A name that changes the screen is sent as its event
    /// (game::eventForAction), every other name is a command of the screen it is on
    /// (handleMenuCommand).
    void handleMenuActions();

    /// Does what a button asks for that does not change the screen: choose a difficulty
    /// or roll a new seed in the main menu, switch fullscreen, step the window size or
    /// reset the settings on the settings screen. False for a name it does not know.
    bool handleMenuCommand(const std::string& action);

    /// Takes the controls of the settings screen that were moved since the last frame
    /// (ui::UiLayer::takeChanges), writes their values into m_settings
    /// (game::applySetting) and uses them at once.
    void handleControlChanges();

    /// Reads the seed field of the main menu into m_newGame.seed. An empty field takes
    /// a random seed. False when the field holds something that is not a seed: a hint
    /// is then shown next to it, and the game is not started.
    bool readSeedField();

    /// Chooses a random seed for the next game and writes it into the seed field of
    /// the main menu.
    void rollSeed();

    /// Makes the window match m_mode: shows the document of the screen (or none while
    /// playing), filled with what it shows, and captures the cursor for the game or
    /// gives it back to the menu.
    void showScreen();

    /// Starts a new game: the numbers of its difficulty level go into the request for
    /// the maze and into the rules of the round (they overwrite what the debug UI set
    /// there), the maze is built from the seed and its round starts.
    void startNewGame(const NewGame& newGame);

    /// The four functions that write what a screen shows into its document. The main
    /// menu: the chosen difficulty and its numbers. The pause menu: the difficulty and
    /// the seed of the round. The result screen: the time, the crystals, the
    /// difficulty and the seed. The settings screen: where its controls stand.
    void fillMainMenuDocument();
    void fillPauseDocument();
    void fillRoundEndDocument();
    void fillSettingsDocument();

    /// Uses the mouse sensitivity and the field of view of m_settings: they are copied
    /// to m_mouseSensitivity and to the camera. Called when a setting changed, not in
    /// every frame, so the debug UI can still edit both numbers by itself.
    void applyViewSettings();

    /// Gives the window the size and the fullscreen state of m_settings.
    void applyWindowSettings();

    /// Writes m_settings into the settings file, unless they are what the file holds
    /// already. An error is in the log, and the game goes on.
    void saveSettings();

    /// Plays the sound of a cue and remembers it for the debug UI. Without a sound
    /// device nothing is heard, and the cue is counted all the same.
    void playCue(SoundCue cue);

    /// Starts a round on the maze m_mazeWorld holds: every crystal back in its place,
    /// a full battery with the flashlight on, the gate closed, no lever pulled, every
    /// wall standing and the player at the start, looking down an open passage. It runs for the
    /// first maze, after every regeneration and when the round is restarted (key R or the debug
    /// UI).
    void beginRound();

    /// The picking ray of this frame and what it hits (game::pickInRound). The ray goes
    /// through the middle of the picture while the cursor is captured and through the
    /// cursor while it is free, and it is built from view and projection, the two
    /// matrices the frame is drawn with, and from eye, the blended eye of the frame.
    /// Without a ray (a window without a size, a cursor over a debug panel or outside
    /// the window) the result is game::pickNothing.
    PickState pickForFrame(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& eye,
                           bool cursorCaptured);

    /// Reads the interaction key and the left mouse button of this frame and does what
    /// m_pick says (game::interact). A click with the free cursor that hits nothing to
    /// interact with captures the cursor, as before. cursorCaptured is the state at the
    /// start of the frame.
    void handleInteraction(bool cursorCaptured);

    /// The shadow pass of the moon, the first pass of a frame. It fits the box of the
    /// moon to the land (m_moonLightSpace), draws the shadow casters into the shadow
    /// map (drawShadowCasters), binds the map for the lit programs and, while the
    /// debug UI asks for it, draws its preview picture. With the shadows switched off
    /// it only computes the box. It leaves a framebuffer of the shadow map bound, so
    /// the scene framebuffer has to be bound after it.
    void drawMoonShadowMap();

    /// The shadow pass of the flashlight, right after the one of the moon. It builds
    /// the pyramid the flashlight looks along (m_flashlightLightSpace) from flashlight,
    /// the place and the direction of the light in this frame, and from the cone and
    /// the range of frameLighting, the lighting of this frame (game::lightingForFrame).
    /// Then it draws the shadow casters into the shadow map of the flashlight, binds
    /// the map for the lit programs and, while the debug UI asks for it, draws its
    /// preview picture. With the shadows of the flashlight switched off, or with the
    /// flashlight itself off in this frame, it only computes the pyramid. Like
    /// drawMoonShadowMap it leaves a framebuffer of its own bound.
    void drawFlashlightShadowMap(const LightingSettings& frameLighting,
                                 const FlashlightPose& flashlight);

    /// Draws everything that casts a shadow with the depth program, as the light with
    /// the given view and projection sees it: the terrain, the walls (a wall opened by
    /// a lever as far as it has sunk) and the pillars, the gate (as far as it has sunk),
    /// the crystals, the levers and the notes. The grass casts no shadow.
    /// The target (a shadow map) must be bound already.
    void drawShadowCasters(const scene::LightSpace& lightSpace) const;

    /// Sets the uniforms of both shadow maps (the moon and the flashlight) in the
    /// program shader, which must be in use and must include common/shadows.glsl:
    /// game::setShadowUniforms once for each map. Called in every frame for the lit
    /// program in use and for the grass program, also with the shadows switched off.
    void setShadowUniformsOf(const gfx::Shader& shader) const;

    /// The parts of a frame. Each one selects its own shader program and sets its
    /// uniforms. drawMaze draws the terrain and the maze together with the crystals,
    /// the gate, the levers and the notes, and has two ways to do it: without lighting (the
    /// textured program, also used for the debug views of the normals and the texture coordinates)
    /// and with lighting. drawGrass draws the grass with its own program.
    void drawMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawLitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawGrass(const glm::mat4& view, const glm::mat4& projection) const;
    void drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const;

    /// The debug view of the picking, drawn with the program of the collider lines: the
    /// pick box of every lever and note, the box the ray hit in a colour of its own, and
    /// the ray of m_shownPick as a line with a small sphere at its end.
    void drawPickLines(const glm::mat4& view, const glm::mat4& projection) const;

    /// Draws the levers and the notes with the given program of the maze, the picked
    /// one highlighted (game::highlightGlow).
    void drawInteractables(const gfx::Shader& shader) const;

    /// The reflection pass, after the maze and the grass and before the sky: the
    /// crystals and the puddles, drawn with the reflect program, which shows the sky on
    /// them (environment mapping). It handles all four lighting modes itself, like
    /// drawGrass. In the two debug views it only draws the puddles, as data, with the
    /// textured program. With the environment mapping switched off it draws nothing.
    void drawReflections(const glm::mat4& view, const glm::mat4& projection) const;

    /// True when the crystals of this frame are drawn by drawReflections. False when
    /// drawMaze draws them with the program of the walls: with the environment mapping
    /// switched off, in the two debug views and when the reflect program failed to load.
    bool crystalsReflect() const;

    /// Draws the gate with the given program of the maze, and the crystals too unless
    /// drawReflections draws them in this frame (crystalsReflect).
    void drawGateAndCrystals(const gfx::Shader& shader) const;

    /// The light the crystals give off by themselves at this moment, as a linear colour
    /// for the uniform uEmissive: game::crystalGlow of the colour of the crystal lights.
    glm::vec3 crystalEmissive() const;

    /// The minimap, the last thing of a frame the game draws: after the composite pass,
    /// so the fog, the bloom and the tone mapping do not touch it. It builds the shapes
    /// of the map (game::buildMinimapVertices), draws them into the framebuffer of the
    /// minimap and puts that picture into its corner of the window. framebuffer is the
    /// framebuffer size of the window and feet the place the frame is drawn from (the
    /// feet of the player, blended between two fixed steps). With the minimap switched
    /// off it does nothing. It leaves the window bound, with the viewport over all of it.
    void drawMinimap(core::Size framebuffer, const glm::vec3& feet);

    // The colour every frame starts with: a very dark blue, darker than the ambient
    // light on the stone. The sky is drawn over it wherever no wall is, so it shows
    // only when the skybox is switched off or its pictures could not be loaded. It is
    // close to the colour of the sky straight above, so switching the skybox off does
    // not change the mood of the scene. An sRGB value, as the colour picker of the
    // debug UI shows it: onRender converts it to linear for the HDR buffer.
    std::array<float, 3> m_clearColor{0.022F, 0.033F, 0.088F};

    // OpenGL objects. They are members of a class derived from core::Application, so they
    // are created after the window and its OpenGL context, and destroyed before them.
    //
    // Order: members are constructed top to bottom. The renderers of the maze, the
    // round and the terrain ask m_assets for their models and textures in their
    // constructors, so they come after it.
    gfx::Shader m_texturedShader;
    gfx::Shader m_colorShader;
    gfx::Shader m_litShader;
    gfx::Shader m_gouraudShader;
    gfx::Shader m_skyboxShader;
    gfx::Shader m_grassShader;
    gfx::Shader m_compositeShader;
    gfx::Shader m_previewShader;
    gfx::Shader m_brightPassShader;
    gfx::Shader m_blurShader;
    // Draws depth only, from the view of a light: the program of the shadow pass.
    gfx::Shader m_shadowDepthShader;
    // Draws the flat shapes of the minimap into its framebuffer, and puts the finished
    // picture of the minimap into the window.
    gfx::Shader m_minimapShader;
    gfx::Shader m_minimapOverlayShader;
    // Draws the crystals and the puddles with the sky on them: the reflection pass.
    gfx::Shader m_reflectShader;
    assets::AssetCache m_assets;
    MazeRenderer m_mazeRenderer;
    GameplayRenderer m_gameplayRenderer;
    InteractableRenderer m_interactableRenderer;
    TerrainRenderer m_terrainRenderer;
    GrassRenderer m_grassRenderer;
    PuddleRenderer m_puddleRenderer;
    ColliderLines m_colliderLines;
    LightRig m_lightRig;
    Skybox m_skybox;
    // The HDR framebuffer of the scene and the passes after the scene.
    PostProcess m_postProcess;
    // The depth texture the scene is drawn into from the direction of the moon.
    ShadowMap m_moonShadowMap;
    // The depth texture the scene is drawn into from the flashlight.
    ShadowMap m_flashlightShadowMap;
    // The framebuffer the minimap is drawn into and the buffer of its triangles.
    MinimapRenderer m_minimapRenderer;
    // What is behind the main menu: the video loop, its still picture, or nothing of
    // its own when the live scene is shown there.
    MenuBackgroundRenderer m_menuBackground;

    // What the player has set: read from the settings file at start-up (the defaults
    // without a file) and written back when it changed. m_savedSettings is what the
    // file holds. Both are declared before m_mazeSettings, because the first maze has
    // the size of the difficulty the settings name.
    GameSettings m_settings;
    GameSettings m_savedSettings;

    // The request for the next maze (edited by the debug UI, and written by a new game).
    MazeSettings m_mazeSettings;

    // The heightmap of the terrain, read from its picture once at start-up, and the
    // settings of the terrain (edited by the debug UI). Both are declared before
    // m_mazeWorld, because the first maze is built from them in the initializer list.
    Heightmap m_heightmap;
    TerrainSettings m_terrainSettings;

    // The maze in play, standing on its terrain.
    MazeWorld m_mazeWorld;

    // The settings of the grass (edited by the debug UI). The tufts themselves are not
    // kept: they are placed, copied to the graphics card and forgotten.
    GrassSettings m_grassSettings;

    // The numbers of the rules (edited by the debug UI) and the round in play. The round
    // is simulation state: onUpdate advances it in fixed steps. It starts empty and is
    // filled by beginRound in the constructor.
    GameplaySettings m_gameplay;
    Round m_round;

    // What the player cannot walk through in this round: the boxes of the maze without
    // the walls that levers have opened, plus the box of the gate while it is closed
    // (game::roundObstacles). The picking ray is stopped by the same boxes. A copy that
    // is rebuilt only when it changes: at the start of a round, when the gate opens, when
    // a lever is pulled and when the terrain is built again with another height scale
    // (the boxes move up or down).
    std::vector<scene::Aabb> m_obstacles;

    // The model matrices of the walls for the frame that is being drawn
    // (game::roundWallMatrices): a wall that a lever has opened is lower in every frame
    // while it sinks. Built once per frame in onRender and used by the shadow passes and
    // by the scene pass, so the shadow of a wall always fits the wall.
    std::vector<glm::mat4> m_wallMatrices;

    // The picking of this frame: the ray, what it hits and what the interaction key
    // does. Built in onRender, read by the drawing (the highlight), the HUD and the
    // debug UI.
    PickState m_pick;
    // The switches of the debug view of the picking (edited by the debug UI), and the
    // picking that view draws: a copy of m_pick that stops following it while the
    // "freeze" switch is set.
    PickDebugSettings m_pickDebug;
    PickState m_shownPick;

    // The player is simulation state: onUpdate moves it in fixed steps.
    Player m_player;
    // Position of the player before the last fixed step. onRender draws from a point
    // between this one and m_player.position. It starts equal to the position of the
    // player, so the frames before the first step are drawn from where the player stands.
    // Declared after m_player, because members are initialized top to bottom.
    glm::vec3 m_previousPlayerPosition = m_player.position;
    // Whether the player was in noclip mode in the last fixed step. The step in which
    // a flying player starts to walk drops the feet to the ground, and that drop is the
    // one change of height that must not be blended (see onUpdate).
    bool m_playerWasFlying = false;

    // Where the scene is seen from (the view and projection matrices). The angles are
    // turned by the mouse. The position is not controlled directly: after every fixed
    // step it is set to the eyes of the player.
    scene::Camera m_camera;

    // The lighting: how the scene is shaded and the settings of every light. The lights
    // of a frame are built from it in onRender.
    LightingSettings m_lighting;

    // The shadows of the moon: their settings (edited by the debug UI), the view and
    // the projection of the moon in this frame, and whether the shadow pass has filled
    // the map in this frame. The last two are set by drawMoonShadowMap in every frame.
    ShadowSettings m_moonShadow;
    scene::LightSpace m_moonLightSpace;
    bool m_moonShadowDrawn = false;

    // The same three for the shadows of the flashlight, set by drawFlashlightShadowMap
    // in every frame. The settings start with a smaller map and a bias of their own
    // (game::flashlightShadowDefaults).
    ShadowSettings m_flashlightShadow = flashlightShadowDefaults();
    scene::LightSpace m_flashlightLightSpace;
    bool m_flashlightShadowDrawn = false;

    // What the textured shader shows: the picture, or one of the two debug views. A debug
    // view replaces the lighting: it is drawn with the textured program in every
    // lighting mode.
    ViewMode m_viewMode = ViewMode::Textured;
    // Whether the collision boxes and spheres are drawn as lines on top of the scene.
    bool m_drawColliders = false;

    // Whether the sky is drawn and how bright it is.
    SkyboxSettings m_skyboxSettings;

    // How the crystals and the puddles show the sky (edited by the debug UI). The
    // puddles themselves are not kept: they are placed, handed to m_puddleRenderer and
    // forgotten, like the tufts of the grass.
    EnvironmentSettings m_environment;

    // The exposure and the tone mapping of the composite pass and the settings of the
    // bloom, the fog and the vignette.
    PostProcessSettings m_postProcessSettings;

    // How the camera is turned. It belongs to the controls, not to the camera.
    float m_mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;

    // Whether the minimap is drawn, whether it shows the whole maze, and its size, its
    // corner and its opacity.
    MinimapSettings m_minimapSettings;

    // The menu camera: its settings (edited by the debug UI and set by the command
    // line) and the line its corridor walk follows through the maze in play. The line
    // is built again whenever the maze or the terrain changes.
    MenuCameraSettings m_menuCamera;
    MenuCameraPath m_menuCameraPath;
    // The clock of the menu camera: seconds since it was switched on, kept inside one
    // loop of the shot. A double, like the clock of the frame it is advanced by.
    double m_menuCameraSeconds = 0.0;
    // Whether the menu camera was on in the frame before: the frame in which the two
    // differ is the one that switches it.
    bool m_menuCameraWasEnabled = false;

    // The screen the game is on. It starts with the main menu, or straight in a round
    // when the command line asked for that.
    GameMode m_mode = GameMode::MainMenu;
    // The game the button "Play" starts: the difficulty chosen in the main menu and
    // the seed its seed field shows.
    NewGame m_newGame;
    // The name of the difficulty of the game in play, for the pause menu and the
    // result screen: the name of a level, or "Custom" for a maze the debug UI asked for.
    std::string m_playedDifficultyName;

    // Whether the window was the active one in the frame before: the frame in which it
    // stops being active pauses a running round.
    bool m_windowWasFocused = true;

    // The menu: RmlUi and the four documents, one per screen with a menu. Each
    // DocumentId is ui::NO_DOCUMENT when its file could not be loaded.
    ui::UiLayer m_ui;
    ui::DocumentId m_mainMenuDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_pauseDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_roundEndDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_settingsDocument = ui::NO_DOCUMENT;
    // True when all four documents are loaded. Without them a menu screen would show
    // nothing and could not be left, so the game then never enters one.
    bool m_menusLoaded = false;

    // The sound device with the sounds of the cues, loaded once in the constructor in
    // the order of game::SoundCue. Without a device it does nothing (audio::AudioEngine).
    audio::AudioEngine m_audio;
    // The clock of the low battery pulse. It is simulation state like the round:
    // onUpdate advances it in fixed steps, and beginRound starts it anew.
    LowBatteryPulse m_lowBatteryPulse;
    // What playCue remembers for the debug UI: the name of the last cue (a text of the
    // cue table, which lives as long as the program) and the number of cues so far.
    const char* m_lastCueName = "none";
    int m_cuesPlayed = 0;
};

} // namespace game
