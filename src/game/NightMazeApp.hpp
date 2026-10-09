// The Night Maze application: game state and rendering of a frame.
#pragma once

#include "assets/AssetCache.hpp"
#include "audio/AudioEngine.hpp"
#include "core/Application.hpp"
#include "game/Campaign.hpp"
#include "game/ColliderLines.hpp"
#include "game/Daily.hpp"
#include "game/Difficulty.hpp"
#include "game/EnvironmentMapping.hpp"
#include "game/GameState.hpp"
#include "game/GameplayRenderer.hpp"
#include "game/GateLamp.hpp"
#include "game/Grass.hpp"
#include "game/GrassRenderer.hpp"
#include "game/InteractableRenderer.hpp"
#include "game/Interaction.hpp"
#include "game/Intro.hpp"
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
#include <optional>
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
/// Through the same maze walks the shade (game/Shade.hpp), which moves only while the
/// flashlight is not on it and carries the player back to the start when it gets there.
/// A calm night has none.
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
/// shown in the middle of the window while the player holds the map key.
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
/// The game is always on one screen (game/GameState.hpp): the intro, the main menu, free
/// play, the list of campaign nights, a title card, a round being played, the pause
/// menu, the result of a won round, the ending card or the settings. The round runs
/// only while it is played. The menus are documents drawn by RmlUi (ui::UiLayer) on top
/// of the finished frame. Their buttons and the Escape key are events, and
/// game::nextMode says which screen follows.
///
/// Behind the main menu plays a recorded video loop of the game
/// (game::MenuBackgroundRenderer), or a still picture when the video cannot be played.
/// Such a background covers the whole window, so in those frames the scene is not drawn
/// at all (game::drawsScene): no shadow maps, no HDR picture, no bloom.
///
/// A campaign begins with the intro (game/Intro.hpp): five cards of text, the first on
/// black and the others over pictures the menu camera takes in one fixed maze, with
/// a wind and a few sounds under them. It is played live, by the same code that draws
/// a round. Any key ends it, and the title card of the first night follows. The game
/// itself opens with the main menu.
///
/// The game is played as a campaign of five nights (game/Campaign.hpp): the first entry
/// of the main menu starts the next night, each night begins with a title card and has
/// a maze, notes and numbers of its own, and the last one ends with the ending card. The
/// progress is part of the settings file. Next to it is free play: one maze of a chosen
/// difficulty and seed.
///
/// A game of free play has a difficulty (game/Difficulty.hpp), which decides the size of the
/// maze, its crystals, the gate and the battery, and a seed, which decides the maze.
/// What the player sets on the settings screen (game/Settings.hpp) is used at once and
/// kept in a small text file in the working directory.
///
/// The game has short sounds (the click of the flashlight, a crystal, a lever, the
/// gate, the warning of a low battery). WHICH sound belongs to what happened is decided
/// by the rules in game/SoundCues.hpp. This class asks them at the few places where
/// something happens and hands the answer to audio::AudioEngine (playCue). The steps
/// of the player and of the shade are such sounds too, and under all of them lies the
/// wind of the maze, a loop that is heard while a round is played (updateAmbience).
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

    /// True when the map was on the screen in the last frame drawn. The HUD asks it
    /// (main.cpp passes it on): while the map covers the middle of the window, the timed
    /// sentence of the HUD waits, because its place is where the bottom edge of the map
    /// is. Everything else of the HUD stands in the corners and stays.
    bool mapOnScreen() const { return m_mapOnScreen; }

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

    /// The settings of the minimap (pin, reveal all, size, opacity), exposed so the debug
    /// UI can edit them live.
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

    /// The sound device and the loaded sounds, read only: the debug UI shows their
    /// status and the loudness in use.
    const audio::AudioEngine& audio() const { return m_audio; }

    /// The master volume of the settings, 0 to 100, for the debug UI.
    float masterVolumeSetting() const { return m_settings.masterVolume; }

    /// The effects volume and the ambient volume of the settings, 0 to 100, for the
    /// debug UI.
    float effectsVolumeSetting() const { return m_settings.effectsVolume; }
    float ambientVolumeSetting() const { return m_settings.ambientVolume; }

    /// The name of the cue that was played last (game::soundCueName), "none" before the
    /// first one, and how many cues were played since the start, for the debug UI.
    const char* lastCueName() const { return m_lastCueName; }
    int cuesPlayed() const { return m_cuesPlayed; }

    /// How loud the last cue was played, from 0 to 1 (game::CuePlay::volume), for the
    /// debug UI: the steps of the shade are quieter the farther away it is.
    float lastCueVolume() const { return m_lastCueVolume; }

    /// The request to hear the wind of the maze for a few seconds, exposed so the debug
    /// UI can ask for it on any screen but the intro: set it to true.
    bool& windSampleRequest() { return m_windSampleRequested; }

    /// True when the mazes of free play have a shade in this run: its switch "Calm night"
    /// is off and the command line did not ask for a calm run (--calm). Texts that differ
    /// between a night with and without the shadow ask here. A night of the campaign says
    /// itself whether it has a shade (startNight).
    bool shadeInGame() const { return !m_settings.calmNight && !m_calmRun; }

    /// True when a maze of free play built now has a shade. daily is the day of a maze
    /// of the day (game/Daily.hpp), which always has one, whatever the switch "Calm
    /// night" says: it is the same maze for every player. Only a calm run of the command
    /// line takes it away. NO_DAILY_DATE: any other maze of free play (shadeInGame).
    bool shadeInMaze(std::uint32_t daily) const {
        return daily == NO_DAILY_DATE ? shadeInGame() : !m_calmRun;
    }

    /// The request to play the intro again, exposed so the debug UI can ask for it: set
    /// it to true, and the next frame starts the intro, on whatever screen the game is.
    /// A round that is being played is given up for it, and this intro ends in the main
    /// menu. Besides the switch --intro and the beginning of a campaign this is the only
    /// way to see the intro.
    bool& introRequest() { return m_introRequested; }

    /// The key of every action of the player, as the settings hold them, for the HUD:
    /// it names the keys in its prompts.
    const KeyBindings& keyBindings() const { return m_settings.keys; }

    /// The campaign as the settings file has it (the next night, the seed, the best
    /// times) and the night that is in play (0 in free play), for the debug UI.
    const GameSettings& settings() const { return m_settings; }
    int playedNight() const { return m_playedNight; }

    /// The day of the maze of the day that is in play (game/Daily.hpp), or NO_DAILY_DATE,
    /// for the HUD: it names the maze in the first seconds of a round.
    std::uint32_t playedDaily() const { return m_playedDaily; }

    /// What the debug UI asks of the campaign (game::CampaignRequest): it writes, and the
    /// next frame does it.
    CampaignRequest& campaignRequest() { return m_campaignRequest; }

private:
    // Camera turn for one screen coordinate unit of mouse movement, in degrees. The mouse
    // is measured in the units of the window size, not in framebuffer pixels, so the same
    // hand movement turns the camera equally on a Retina display.
    static constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1F;

    /// Builds the maze described by m_mazeSettings on its terrain and starts a round in
    /// it (beginRound). night is the night of the campaign the maze is built for: its
    /// notes are then the ones of that night (game::campaignInteractables). 0 is a maze
    /// of free play or one the debug UI asked for, with the notes of m_mazeSettings.
    /// daily is the day of a maze of the day (m_playedDaily), or NO_DAILY_DATE.
    void regenerateMaze(int night = 0, std::uint32_t daily = NO_DAILY_DATE);

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

    /// Does what a button asks for that is no event by itself: choose a difficulty or
    /// roll a new seed in free play, switch fullscreen, step the window size or reset the
    /// settings on the settings screen, and the buttons that start a night of the
    /// campaign (the first entry of the main menu, a row of the list of nights, the
    /// answer to a new campaign). False for a name it does not know.
    bool handleMenuCommand(const std::string& action);

    /// The two questions every action of the player is read with, and the one place
    /// that knows which key an action is on (m_settings.keys). actionDown: is its key
    /// held? actionPressed: did its key go down in this frame? The second one is for
    /// code that runs once per frame, like core::Input::wasKeyPressed.
    bool actionDown(KeyAction action) {
        return input().isKeyDown(boundKey(m_settings.keys, action));
    }
    bool actionPressed(KeyAction action) {
        return input().wasKeyPressed(boundKey(m_settings.keys, action));
    }

    /// The part "Controls" of the settings screen while it waits for a key: takes the
    /// key that was pressed (ui::UiLayer::takeCapturedKey). Escape leaves the action as
    /// it is, a key that cannot be bound is refused with a line on the screen and the
    /// wait goes on, every other key becomes the key of the action (game::bindKey).
    /// Called once per frame.
    void handleKeyCapture();

    /// Ends the wait for a key, when there is one: the row shows its key again.
    void stopKeyCapture();

    /// Writes the part "Controls" of the settings screen: the name and the key of
    /// every action ("Press a key" for the one that waits) and the line under them.
    void fillControls();

    /// Takes the controls of the settings screen that were moved since the last frame
    /// (ui::UiLayer::takeChanges), writes their values into m_settings
    /// (game::applySetting) and uses them at once.
    void handleControlChanges();

    /// Reads the seed field of the free play screen into m_newGame.seed. An empty field takes
    /// a random seed. False when the field holds something that is not a seed: a hint
    /// is then shown next to it, and the game is not started.
    bool readSeedField();

    /// Chooses a random seed for the next game and writes it into the seed field of
    /// the free play screen.
    void rollSeed();

    /// Makes the window match m_mode: shows the document of the screen (or none while
    /// playing), filled with what it shows, and captures the cursor for the game or
    /// gives it back to the menu.
    void showScreen();

    /// Starts a new game of free play: the numbers of its difficulty level go into the
    /// request for the maze and into the rules of the round (they overwrite what the
    /// debug UI set there), the maze is built from the seed and its round starts.
    /// The maze of the day is such a game too: daily is its day (game/Daily.hpp), and it
    /// then has the shade and the name "Tonight's hedge". NO_DAILY_DATE: free play.
    void startNewGame(const NewGame& newGame, std::uint32_t daily = NO_DAILY_DATE);

    /// The day "Tonight's hedge" has at this moment: the day of the clock, or the fixed
    /// day of a run a tool drives (game::fixedDailyDate).
    std::uint32_t dailyToday() const;

    /// Starts the maze of the day from the main menu: asks for the day once, builds the
    /// maze (startNewGame) and sends GameEvent::StartNight, which brings up the title
    /// card. Nothing happens on another screen.
    void beginDaily();

    /// Builds the maze of a night of the campaign and starts its round: the numbers of
    /// the night (game::campaignNight) go into the request for the maze and into the
    /// rules of the round, like the numbers of a level do in startNewGame. The seed of
    /// the maze follows from campaignSeed (game::campaignNightSeed). counts says whether
    /// winning the night changes the campaign in the settings file: true for a night
    /// started from the menus, false for one of the command line (--night).
    void startNight(int night, std::uint32_t campaignSeed, bool counts);

    /// Starts a night of the campaign of the settings file, from a menu: draws the seed
    /// of the campaign when it has none yet, builds the maze (startNight), writes the
    /// settings file and sends GameEvent::StartNight, which brings up the title card.
    /// Nothing happens on a screen that cannot start a night.
    void beginCampaignNight(int night);

    /// The functions that write what a screen shows into its document. The main menu:
    /// the label of its first entry, the night beside it and the numbers of that night.
    /// Free play: the chosen difficulty and its numbers, and the switch "Calm night".
    /// The list of nights: the title and the state of every row. The pause menu: the
    /// difficulty or the night and the seed of the round. The result screen: the time,
    /// the crystals, the difficulty or the night, the seed and, after a night, its line of
    /// the story. The settings screen: where its controls stand.
    void fillMainMenuDocument();
    void fillFreePlayDocument();
    void fillNightsDocument();
    void fillPauseDocument();
    void fillRoundEndDocument();
    void fillSettingsDocument();

    /// Uses the mouse sensitivity and the field of view of m_settings: they are copied
    /// to m_mouseSensitivity and to the camera. Called when a setting changed, not in
    /// every frame, so the debug UI can still edit both numbers by itself.
    void applyViewSettings();

    /// Gives the window the size and the fullscreen state of m_settings.
    void applyWindowSettings();

    /// Gives the audio engine the three volumes of m_settings: the master volume and
    /// the volumes of the effects and of the ambient sounds (game::masterVolumeGain of
    /// each).
    void applyAudioSettings();

    /// Switches the wind of the maze on or off for the screen the game is on
    /// (game::mazeWindPlays): on while a round is played, off under every menu, in the
    /// intro and while the window is not the active one (windowFocused), and on for
    /// a moment when a sample was asked for. The audio engine fades it. Called once
    /// per frame.
    void updateAmbience(bool windowFocused);

    /// Writes m_settings into the settings file, unless they are what the file holds
    /// already. An error is in the log, and the game goes on.
    void saveSettings();

    /// Plays the sound of a cue and remembers it for the debug UI. volume is how loud
    /// this one play is, from 0 to 1: 1 is the file as it is. Without a sound device
    /// nothing is heard, and the cue is counted all the same.
    void playCue(SoundCue cue, float volume = 1.0F);

    /// Starts the intro from its beginning, on whatever screen the game is: builds the
    /// maze of the intro and shows the card. For the request of the debug UI
    /// (introRequest). A start with the switch --intro needs no call: the constructor
    /// builds that maze and begins on the screen of the intro (game::startMode).
    void startIntro();

    /// What the first entry of the main menu does, and the answer "yes" to a new
    /// campaign after it has forgotten the old one (game::campaignEntryEvent): the
    /// question about a new campaign, the intro and then the first night, or the next
    /// night at once.
    void enterCampaign();

    /// Begins a campaign with the intro: draws the seed of the campaign, builds the maze
    /// of the intro and goes to the screen GameMode::CampaignIntro. The first night is
    /// started when the intro is left (leaveIntro).
    void beginCampaignIntro();

    /// Draws the seed of the campaign when it has none yet: its five mazes are fixed
    /// from here on.
    void drawCampaignSeed();

    /// True when the intro that is playing tells of the shadow on its fourth card
    /// (game::introLines, game::introShadeCell). The intro of a campaign always does,
    /// because the campaign has the shadow from its second night on. The intro played by
    /// itself follows free play (shadeInGame). A calm run of the command line never does.
    bool introTellsOfShade() const;

    /// One frame of the intro, called once per frame while the game is on its screen.
    /// A key or a mouse button ends it. Otherwise its clock moves on by the time of the
    /// frame, the sounds whose moments were passed are played (game::introCuesBetween)
    /// and the card document gets its two lines and its three opacities
    /// (game::introFrame). At the end of the script it sends GameEvent::IntroFinished.
    void updateIntro();

    /// True when any key or any mouse button was pressed in this frame: what skips the
    /// intro and the cards.
    bool anyKeyPressed();

    /// Writes the lines of the card of text (assets/ui/card.rml): up to
    /// STORY_CARD_LINE_COUNT of them, an empty text for a line that is not used. middle
    /// puts them in the middle of the window, for a card on black.
    void writeCardLines(const std::array<const char*, STORY_CARD_LINE_COUNT>& lines, bool middle);

    /// Sets how much of each part of the card of text is there in this frame: the black
    /// over the picture, the card as a whole, each of its lines and the hint.
    void showCardFrame(float black, float card,
                       const std::array<float, STORY_CARD_LINE_COUNT>& lines, float hint);

    /// Starts the card of the screen the game has just come to: the title card of the
    /// night in play (GameMode::NightCard) or the ending card (GameMode::EndingCard).
    /// Its clock starts at 0 and its lines are written. Without the document of the card
    /// it is over at once.
    void startStoryCard();

    /// One frame of the title card or of the ending card, called once per frame while
    /// the game is on one of the two screens. A key or a mouse button ends it. Otherwise
    /// its clock moves on by the time of the frame, the bell of the ending card rings when
    /// its moment is passed, and the card gets its opacities (game::nightCardFrame,
    /// game::endingCardFrame). At its end it sends GameEvent::CardFinished.
    void updateStoryCard();

    /// Does what the debug UI asked of the campaign (m_campaignRequest) and clears the
    /// request. Called once per frame.
    void handleCampaignRequest();

    /// The round is won: called by the fixed step that took the player through the gate.
    /// In free play the story line counter moves on. In a night of the campaign the best
    /// time and the next night are written to the settings file (unless the night came
    /// from the command line). Then the result screen comes up, or the ending card after
    /// the last night.
    void finishRound();

    /// What the first button of the result screen offers after the night in play was
    /// won: game::nightEndOffer for the campaign of the settings as it is after the win.
    NightEndOffer nightEndOffer() const;

    /// Forgets the campaign of the settings: no night is won, no seed, no best time. For
    /// a new campaign and for the debug UI. The file is not written here.
    void forgetCampaign();

    /// What has to happen when the intro is left, at its end, by a key or by Escape:
    /// called by handleGameEvent, the one place all three come through. Every sound
    /// stops (the wind is as long as the whole intro), the settings file remembers that
    /// the intro was seen, and the maze of the intro makes room for the next one: the
    /// maze of the first night when intoNight is true (the intro of a campaign),
    /// otherwise the one behind the main menu.
    void leaveIntro(bool intoNight);

    /// Starts a round on the maze m_mazeWorld holds: every crystal back in its place,
    /// a full battery with the flashlight on, the gate closed, no lever pulled, every
    /// wall standing and the player at the start, looking down an open passage. It runs for the
    /// first maze, after every regeneration and when the round is restarted (key R or the debug
    /// UI).
    void beginRound();

    /// The shade has reached the player: the picture fades to black first
    /// (m_catchSeconds, game::catchFadeBrightness), then the round starts again in the
    /// same maze, the way the restart key does it (beginRound), and the next caught line
    /// is shown. The sound of the catch plays when the fade starts. The story line counter does not
    /// move and nothing is written to the settings file.
    void carryPlayerBack();

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

    /// Counts the passages from the exit cell to every cell again, in the maze of the
    /// round (m_exitDistances): when a round begins, and when a lever has opened a wall,
    /// which can make a way shorter.
    void measureExitDistances();

    /// How loud a toll of the bell of the gate is where the player stands now
    /// (game::gateBellVolume of the passages to the exit). A player outside the maze
    /// (noclip) hears the far volume.
    float gateBellVolumeHere() const;

    /// The minimap, the last thing of a frame the game draws: after the composite pass,
    /// so the fog, the bloom and the tone mapping do not touch it. It builds the shapes
    /// of the map (game::buildMinimapVertices), draws them into the framebuffer of the
    /// minimap and puts that picture into the middle of the window. framebuffer is the
    /// framebuffer size of the window and feet the place the frame is drawn from (the
    /// feet of the player, blended between two fixed steps). It is called only while
    /// the map is shown (mapShown). It leaves the window bound, with the viewport over
    /// all of it.
    void drawMinimap(core::Size framebuffer, const glm::vec3& feet);

    /// True while the map is on the screen: game::showsMap for the screen the game is
    /// on, the map key as it is held in this moment, the debug pin, the card of a note
    /// and the menu camera. onUpdate asks it to stop the player, onRender to stop the
    /// mouse look and the picking and to draw the map.
    bool mapShown();

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
    // True when the command line asked for a calm run (--calm): no maze of this run has
    // a shade, whatever m_settings says, and the settings are not changed by it.
    // Declared before m_mazeSettings, which asks shadeInGame for the first maze.
    bool m_calmRun = false;

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

    // Whether the shade is drawn in the frame that is being drawn, and its model matrix:
    // where it stands between two fixed steps, turned towards the player. Set once per
    // frame in onRender, like m_wallMatrices, and used by the shadow passes and by the
    // scene pass. The shade is drawn only while a round is played: never under a menu,
    // behind the main menu or in the picture of the menu camera. The one exception is
    // the card of the intro that shows it standing in a corridor (game::introShadeCell).
    bool m_shadeDrawn = false;
    glm::mat4 m_shadeMatrix{1.0F};
    // How much the drawn shade walks, from 0 (stands) to 1 (walks). It follows the state
    // of the shade over a third of a second, so the pose never jumps (game::shadeSwayPose).
    float m_shadeWalkAmount = 0.0F;
    // Seconds since the shade reached the player, or -1 when no catch is going on. While
    // it counts, the round stands still and the player cannot act: the picture fades to
    // black, and at black the round starts again (carryPlayerBack). It counts in fixed
    // steps of a played round, so it stands still in the pause.
    float m_catchSeconds = -1.0F;

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

    // Whether the map was drawn in the last frame (mapOnScreen). Set in onRender.
    bool m_mapOnScreen = false;

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

    // The map: whether the debug UI pins it open, whether it shows the whole maze, and
    // its size and its opacity.
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
    // The game the button "Play" starts: the difficulty chosen on the free play screen
    // and the seed its seed field shows.
    NewGame m_newGame;
    // The night of the campaign that is in play, 1 to CAMPAIGN_NIGHT_COUNT, or 0 for
    // a maze of free play and for one the debug UI asked for. Set whenever a maze is
    // built (regenerateMaze).
    int m_playedNight = 0;
    // True when winning the night in play changes the campaign of the settings file.
    // False for a night started from the command line (--night), which has a campaign
    // seed of its own, and after the debug UI changed the campaign under a running night.
    bool m_nightCounts = false;
    // The day of the maze of the day that is in play (game/Daily.hpp), or NO_DAILY_DATE
    // for every other maze. Set whenever a maze is built (regenerateMaze).
    std::uint32_t m_playedDaily = NO_DAILY_DATE;
    // The last win of a maze of the day: the record after it and whether it was a new
    // best, for the result screen (finishRound).
    DailyWin m_dailyWin;
    // What the debug UI asks of the campaign (handleCampaignRequest).
    CampaignRequest m_campaignRequest;
    // The seed of the command line (StartOptions::seed): the maze behind the main menu
    // is built from it again when the intro, which has a maze of its own, is over.
    std::uint32_t m_startSeed = DEFAULT_MAZE_SEED;
    // True when a campaign that is begun in this run plays the intro first
    // (game::campaignIntroPlays of the command line).
    bool m_campaignIntroPlays = true;
    // The day of "Tonight's hedge" in a run that must not ask the clock
    // (game::fixedDailyDate), or NO_DAILY_DATE. Such a run never writes a best time.
    std::uint32_t m_fixedDailyDate = NO_DAILY_DATE;
    // The name of the difficulty of the game in play, for the pause menu and the
    // result screen: the name of a level, or "Custom" for a maze the debug UI asked for.
    std::string m_playedDifficultyName;

    // Whether the window was the active one in the frame before: the frame in which it
    // stops being active pauses a running round.
    bool m_windowWasFocused = true;

    // The menu: RmlUi and its documents, one per screen with a menu. Each DocumentId is
    // ui::NO_DOCUMENT when its file could not be loaded.
    ui::UiLayer m_ui;
    ui::DocumentId m_mainMenuDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_freePlayDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_nightsDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_newCampaignDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_pauseDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_roundEndDocument = ui::NO_DOCUMENT;
    ui::DocumentId m_settingsDocument = ui::NO_DOCUMENT;
    // True when all of these documents are loaded. Without them a menu screen would show
    // nothing and could not be left, so the game then never enters one.
    bool m_menusLoaded = false;

    // The intro. Its card is a fifth document, without buttons. The clock counts the
    // seconds since the intro began, in the real time of the frames like the clock of
    // the menu camera: what is shown is a function of it (game::introFrame).
    ui::DocumentId m_cardDocument = ui::NO_DOCUMENT;
    double m_introSeconds = 0.0;
    // The card whose two lines stand in the document: they are written when the card
    // changes, not in every frame. INTRO_CARD_COUNT means none yet.
    std::size_t m_introCardShown = INTRO_CARD_COUNT;
    // The clock of the title card of a night and of the ending card: seconds since the
    // card came up (game::nightCardFrame, game::endingCardFrame). The two cards use the
    // document of the intro.
    double m_cardSeconds = 0.0;
    // Set by the debug UI: play the intro again (introRequest).
    bool m_introRequested = false;
    // True while the main menu is the one that followed the intro or the ending card: it
    // then comes in out of black (fillMainMenuDocument).
    bool m_menuAfterIntro = false;

    // The sound device with the sounds of the cues, loaded once in the constructor in
    // the order of game::SoundCue. Without a device it does nothing (audio::AudioEngine).
    audio::AudioEngine m_audio;
    // The clock of the low battery pulse. It is simulation state like the round:
    // onUpdate advances it in fixed steps, and beginRound starts it anew.
    LowBatteryPulse m_lowBatteryPulse;
    // The clock of the breathing of a winded player: the same kind of state.
    WindedBreath m_windedBreath;
    // The clock of the hum of the shade: the same kind of state.
    ShadeHum m_shadeHum;
    // The clock of the bell of the open gate: the same kind of state.
    GateBell m_gateBell;
    // Where the bell of the gatehouse is in its swing (game::advanceBellSwing): pushed by
    // every toll, advanced in fixed steps wherever the picture moves, drawn by every pass.
    BellSwing m_bellSwing;
    // For every cell of the maze, the passages from it to the exit cell
    // (game::passageDistances, row after row): how loud a toll of the bell is played.
    // Counted once per round and again when a lever opens a wall (measureExitDistances).
    std::vector<int> m_exitDistances;
    // The glow of the lanterns of the gate in the frame that is being drawn, as a linear
    // colour for the uniform uEmissive: the colour of the lamp times its strength
    // (game::gateLampColor, game::gateLampStrength). onRender sets it before the shadow
    // passes, and every pass of the frame draws the lanterns with it.
    glm::vec3 m_gateLampGlow{0.0F};
    // True when the player has pulled a lever since the last fixed step: the next step
    // tells the shade, which may hear it (game::playerNoise). Keys are read once per
    // frame and the shade moves in fixed steps, so the pull waits here in between.
    bool m_leverPulled = false;
    // The clocks of the steps of the player and of the shade. They count metres and
    // not seconds, and are the same kind of state too.
    StepClock m_footsteps;
    StepClock m_shadeSteps;
    // The caught line that was shown last (game::caughtLine), so the next catch shows
    // the next one. It lives as long as the program: a new round does not reset it, and
    // it is not saved.
    int m_lastCaughtLine = NO_CAUGHT_LINE;
    // What playCue remembers for the debug UI: the name of the last cue (a text of the
    // cue table, which lives as long as the program) and the number of cues so far.
    const char* m_lastCueName = "none";
    int m_cuesPlayed = 0;
    // How loud that last cue was played (playCue).
    float m_lastCueVolume = 1.0F;
    // Seconds for which a sample of the wind of the maze is still heard on a screen
    // that has no wind (updateAmbience): set when the ambient slider of the settings
    // screen moves. 0: no sample runs.
    double m_windSampleLeft = 0.0;
    // Set by the debug UI: play a sample of the wind (windSampleRequest).
    bool m_windSampleRequested = false;
    // Seconds until the volume slider of the settings screen may play its next sample
    // click (handleControlChanges). 0: the next change is heard at once.
    double m_volumeSampleWait = 0.0;

    // The action whose row of the settings screen waits for a key, or nothing
    // (handleKeyCapture). While it is set the UI layer captures the keyboard.
    std::optional<KeyAction> m_keyCaptureAction;
    // The line under the rows of the part "Controls": what the last change did (a swap,
    // a key that was refused). Empty: the line says the rule of the swap.
    std::string m_controlsNote;
};

} // namespace game
