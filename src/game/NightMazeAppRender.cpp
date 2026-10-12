// The Night Maze application: the frame, the picking and the minimap.
#include "game/NightMazeApp.hpp"

#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "game/GateLamp.hpp"
#include "game/NightMazeAppShared.hpp"
#include "game/Shade.hpp"
#include "gfx/ColorSpace.hpp"
#include "scene/Raycast.hpp"
#include "scene/Transform.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace game {

namespace {

// Key that switches between walking and noclip (free flight).
constexpr int NOCLIP_KEY = GLFW_KEY_N;

// An exposure that changes nothing: the composite pass multiplies the colours by it.
constexpr float NEUTRAL_EXPOSURE = 1.0F;

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

void NightMazeApp::onRender(double alpha) {
    // What the menus, the intro and the cards do in this frame, and the pause of a round
    // whose window is no longer the active one. All of it comes first, because it can
    // change the screen the rest of this function draws.
    updateScreens();
    updateFocus();
    buildRequestedMaze();

    // The menu camera: its key, and what has to happen when it was just switched on.
    updateMenuCameraSwitch();
    const FrameScreen screen = frameScreen();

    // What the debug UI and the keys of the round asked for: all of it is done here,
    // between two fixed steps and never inside one.
    handleRoundRequests(screen.roundInput);
    readRoundKeys(screen.roundInput);

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
    if (cursorCaptured && screen.roundInput && !map) {
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

    // The main menu with its video or its still picture is a frame of its own.
    if (drawMenuBackgroundFrame(framebuffer)) {
        return;
    }

    // Where this frame is seen from: the eye, the camera and the two matrices.
    const FrameView frame = frameView(alpha, screen, framebuffer);

    // Object picking: one ray per frame, what it hits, and then the key or the click
    // that uses it. This runs once per frame like the other keys. A lever that is
    // pulled here changes the round between two fixed steps, never inside one.
    // Under a menu and in the picture of the menu camera nothing is picked and nothing
    // is used: no lever is highlighted, and a click or the key does not reach the round.
    // The same while the map is shown: it covers the middle of the picture, so there is
    // no crosshair, no prompt, and the interaction key does nothing.
    if (!screen.roundInput || map) {
        m_pick = pickNothing(m_round);
    } else {
        m_pick = pickForFrame(frame.view, frame.projection, frame.eye, cursorCaptured);
        handleInteraction(cursorCaptured);
    }
    // The debug view draws this copy, which stands still while "freeze" is set.
    if (!m_pickDebug.freezeRay) {
        m_shownPick = m_pick;
    }

    // Where the walls stand in this frame: a wall that a lever has opened is on its way
    // into the ground. One list for the shadow passes and for the scene pass.
    m_wallMatrices = roundWallMatrices(m_mazeWorld, m_round);

    // The shade, the lighting and the lamp of the gate as this frame shows them.
    placeShade(alpha, screen, frame);
    const LightingSettings frameLighting = frameLightingFor(screen);
    const GateLampFrame lamp = gateLampForFrame(screen);

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
        frame.reveal
            ? flashlightPose(frameLighting, frame.playerEye, m_camera.forward(), m_camera.right())
            : flashlightPose(frameLighting, frame.eye, frame.camera.forward(),
                             frame.camera.right());

    // The shadow passes come first: the scene as the moon sees it and then as the
    // flashlight sees it, depths only, each into its own shadow map. The lit programs
    // of the scene pass read those maps, so they have to be complete before they draw.
    // Each pass binds a framebuffer and a viewport of its own (the size of its map),
    // and beginScene below binds the scene framebuffer with its viewport again.
    drawMoonShadowMap();
    drawFlashlightShadowMap(frameLighting, flashlight);

    // The HDR framebuffer of the scene, cleared, and the lights of the frame.
    if (!beginScenePass(framebuffer)) {
        return;
    }
    const scene::LightSet lights = uploadLights(frameLighting, flashlight, lamp, frame.eye);

    drawMaze(frame.view, frame.projection);
    drawGrass(frame.view, frame.projection);
    // The reflection pass: the crystals and the puddles, which show the sky. The
    // crystals are opaque and write depth like the walls. The puddles are blended over
    // the ground that is already drawn and write no depth (game::PuddleRenderer), so
    // they need the ground first. The sky comes after both: the ground under a puddle
    // has written its depth, so the sky does not show through.
    drawReflections(frame.view, frame.projection);
    if (m_drawColliders) {
        drawColliderLines(frame.view, frame.projection);
    }
    if (m_pickDebug.drawShapes) {
        drawPickLines(frame.view, frame.projection);
    }

    // The village on the ridge, before the sky and after everything of the maze.
    drawVillageForFrame(screen, frame);

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
        m_skybox.draw(m_skyboxShader, frame.view, frame.projection, m_skyboxSettings, m_viewMode);
    }

    // The lamp in the hand, on top of the scene and in front of every wall.
    drawHeldLamp(frame.view, frame.projection, frameLighting, lights);

    // The scene is complete. The passes that read it bring it to the window.
    finishPicture(framebuffer, frame);

    // The map comes after the composite pass, on top of the finished picture: it is
    // a schematic, and the fog, the bloom and the tone mapping must not touch it. It is
    // shown in the two debug views too, like the HUD. feet is the blended position the
    // scene was drawn from. Only while the key is held in a round (game::showsMap): the
    // pause menu, the result screen and the menu camera have no map.
    if (map) {
        drawMinimap(framebuffer, frame.feet);
    }

    // The menu comes last in this function, on top of the finished frame. The debug UI
    // is drawn after this function returns (main.cpp), so its panels stay usable on top
    // of a menu. While a round is played no document is shown and this call does
    // nothing.
    m_ui.draw(framebuffer);
}

void NightMazeApp::updateScreens() {
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
}

void NightMazeApp::updateFocus() {
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
}

void NightMazeApp::buildRequestedMaze() {
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
}

NightMazeApp::FrameScreen NightMazeApp::frameScreen() const {
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
    return {.roundInput = roundInput,
            .intro = intro,
            .menuCamera = menuCamera,
            .menuCameraSettings = menuCameraSettings,
            .introMoment = introMoment,
            .introShot = introShot};
}

void NightMazeApp::handleRoundRequests(bool roundInput) {
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
}

void NightMazeApp::readRoundKeys(bool roundInput) {
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
}

bool NightMazeApp::drawMenuBackgroundFrame(core::Size framebuffer) {
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
    if (drawsScene(m_mode, coversWindow(m_menuBackground.background()))) {
        return false;
    }
    // Nothing is picked under a menu, as in the frames that draw the scene.
    m_pick = pickNothing(m_round);
    // The picture goes straight into the window, like the composite pass does.
    gfx::Framebuffer::bindDefault(framebuffer.width, framebuffer.height);
    m_menuBackground.draw(framebuffer);
    m_ui.draw(framebuffer);
    return true;
}

NightMazeApp::FrameView NightMazeApp::frameView(double alpha, const FrameScreen& screen,
                                                core::Size framebuffer) {
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
    if (screen.intro) {
        // The moment of the shot comes from the script and not from the clock of the
        // menu camera, which stands still meanwhile. The field of view is the default
        // one, not the one of the settings: the shots were picked with it.
        const MenuCameraPose pose = menuCameraPose(
            m_menuCameraPath, m_mazeWorld, screen.menuCameraSettings, screen.introShot.menuSeconds);
        eye = pose.eye;
        frameCamera.yawDegrees = pose.yawDegrees;
        frameCamera.pitchDegrees = pose.pitchDegrees;
        frameCamera.fovDegrees = DEFAULT_FIELD_OF_VIEW_DEGREES;
    } else if (screen.menuCamera) {
        // The clock of the menu camera follows the real time of the frames, not the
        // fixed steps: the pose is a function of time, so it can be asked for the exact
        // moment of every frame and needs no blending. It is kept inside one loop of
        // the shot, so the number stays small however long the menu is open.
        m_menuCameraSeconds += time().deltaSeconds();
        const double loopSeconds =
            game::menuCameraLoopSeconds(m_menuCameraPath, m_mazeWorld, screen.menuCameraSettings);
        if (loopSeconds > 0.0) {
            m_menuCameraSeconds = std::fmod(m_menuCameraSeconds, loopSeconds);
        }
        const MenuCameraPose pose =
            menuCameraPose(m_menuCameraPath, m_mazeWorld, screen.menuCameraSettings,
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
    const bool reveal = villageRevealShown() && !screen.menuCamera;
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
    return {.feet = feet,
            .eye = eye,
            .playerEye = playerEye,
            .camera = frameCamera,
            .reveal = reveal,
            .view = view,
            .projection = projection};
}

void NightMazeApp::placeShade(double alpha, const FrameScreen& screen, const FrameView& frame) {
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
        shade.rotationDegrees = {0.0F, shadeFacingDegrees(m_round.shade, frame.feet), 0.0F};
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
    if (screen.intro) {
        if (const std::optional<MazeCell> cell =
                introShadeCell(screen.introMoment.card, introTellsOfShade())) {
            scene::Transform shade;
            shade.position = cellCenter(cell->x, cell->z);
            shade.position.y = m_mazeWorld.terrain.heightAt(shade.position.x, shade.position.z);
            shade.rotationDegrees = {0.0F, shadeYawDegrees(shade.position, frame.eye), 0.0F};
            applyShadeSway(shade, shadeSwayPose(0.0F, 0.0F, static_cast<float>(m_introSeconds),
                                                m_gameplay.shade.sway));
            m_shadeMatrix = shade.matrix();
            m_shadeHoodBrightness = SHADE_HOOD_REST_BRIGHTNESS;
            m_shadeHoodStarBoost = 0.0F;
            m_shadeDrawn = true;
        }
    }
}

LightingSettings NightMazeApp::frameLightingFor(const FrameScreen& screen) const {
    // The lighting of this frame. The round changes two things for this frame only:
    // a low battery dims the flashlight (an empty one switches it off) and the crystal
    // lights pulse. That happens in a copy, so the settings the debug UI shows stay as
    // they were set.
    LightingSettings frameLighting = lightingForFrame(m_lighting, m_round, m_gameplay);
    if (screen.menuCamera) {
        // The flashlight of the menu camera is not the one of the round: it does not
        // depend on the switch or on the battery, and it never flickers. In the
        // corridors it is on, at the brightness of the settings: the cone of warm light
        // on the stone is the look of the game. High above the maze it is off: its
        // light does not reach the ground from there, and the moon and the crystals
        // are what that shot shows.
        frameLighting.flashlightOn = screen.menuCameraSettings.shot == MenuShot::CorridorWalk;
        frameLighting.flashlightIntensity = m_lighting.flashlightIntensity;
        // The script of the intro names the light of every card itself.
        if (screen.intro) {
            frameLighting.flashlightOn = screen.introShot.flashlightOn;
        }
    }
    return frameLighting;
}

NightMazeApp::GateLampFrame NightMazeApp::gateLampForFrame(const FrameScreen& screen) {
    // The lamp of the gate in this frame: how bright and in which colour, by the rule of
    // the round (game::gateLampStrength: a cold ember while the gate is closed, amber once
    // it is open). The menu camera and the live scene behind the main menu show it fully
    // lit, whatever the round says, so the glide over the maze has its one warm light.
    // The intro does not: it tells that the lamps of the village have gone out, so its
    // pictures keep the cold ember of a night that has not begun.
    const bool lampShownLit = screen.menuCamera && !screen.intro;
    const float lampProgress = lampShownLit ? 1.0F : m_round.gateProgress;
    const float lampStrength = lampShownLit
                                   ? 1.0F
                                   : gateLampStrength(m_round.collectedCount, m_round.requiredCount,
                                                      m_round.gateProgress, m_gameplay.gate);
    // The colour is an sRGB value and the glow a linear colour, like the one of the
    // crystals (crystalEmissive).
    m_gateLampGlow = gfx::srgbToLinear(gateLampColor(lampProgress)) *
                     (m_gameplay.gate.glowStrength * lampStrength);
    return {.progress = lampProgress, .strength = lampStrength};
}

bool NightMazeApp::beginScenePass(core::Size framebuffer) {
    // From here on the draw calls do not land in the window. They land in the HDR
    // framebuffer of the scene, which is created again here when the size of the window
    // has changed. The viewport is set to its size by the same call. Without
    // a framebuffer (the driver refused it, the error is in the log) nothing is drawn.
    // The shadow passes above may have left their own framebuffer bound, so the window
    // is bound again first: the debug UI is drawn after this function and must land
    // there.
    if (!m_postProcess.beginScene(framebuffer)) {
        gfx::Framebuffer::bindDefault(framebuffer.width, framebuffer.height);
        return false;
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
    return true;
}

scene::LightSet NightMazeApp::uploadLights(const LightingSettings& frameLighting,
                                           const FlashlightPose& flashlight,
                                           const GateLampFrame& lamp, const glm::vec3& eye) {
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
        pointLights.push_back(gateLampLight(gateScenery(m_mazeWorld).lightPosition, lamp.strength,
                                            lamp.progress, m_gameplay.gate));
    }
    const std::vector<PointLightSpot> frameLights = nearestPointLightSpots(pointLights, eye);
    const scene::LightSet lights = buildLightSet(frameLighting, flashlight, frameLights);
    m_lightRig.upload(lights, eye);
    return lights;
}

void NightMazeApp::drawVillageForFrame(const FrameScreen& screen, const FrameView& frame) const {
    // The village on the ridge, before the sky and after everything of the maze: it lies
    // near the far plane, so the maze has already filled most of the pixels it would
    // cover. How many nights of it are lit is a rule of the campaign. The live scene
    // behind a menu and a maze of free play show the progress of the campaign, a night
    // shows itself, and the intro, which tells that the lamps have gone out, shows none.
    const bool ownNight = m_playedNight != 0 && !usesMenuCamera(m_mode);
    const bool nightWon = m_mode == GameMode::RoundEnd || m_mode == GameMode::VillageBeat ||
                          m_mode == GameMode::EndingCard;
    const int nightsLit = screen.intro ? 0
                                       : villageNightsLit(ownNight ? m_playedNight : 0, nightWon,
                                                          m_settings.campaignNight, m_toolRun);
    // The lights of the night that was just won come on while the reveal looks at them.
    const float newestLight =
        frame.reveal ? villageNewLightStrength(static_cast<float>(m_revealSeconds)) : 1.0F;
    drawVillage(frame.view, frame.projection, frame.camera.farPlane, nightsLit, newestLight);
}

void NightMazeApp::finishPicture(core::Size framebuffer, const FrameView& frame) {
    // The scene is complete: its colours and its depth are in the two textures of the
    // scene framebuffer. What follows reads those textures. Passes that are added
    // later (effects computed from the finished scene) belong here, before the
    // composite pass.

    // The pictures of the attachments, only while the debug UI shows them.
    if (m_postProcessSettings.previews) {
        m_postProcess.drawPreviews(m_previewShader, m_postProcessSettings, frame.camera.nearPlane,
                                   frame.camera.farPlane);
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
    const SceneView sceneView{.inverseViewProjection = glm::inverse(frame.projection * frame.view),
                              .eye = frame.eye};

    // The last pass: back to the window, and the HDR picture goes into it with the fog,
    // the bloom, exposure, tone mapping, the vignette and the sRGB encoding. The debug
    // UI is drawn after this function returns (main.cpp), straight into the window.
    m_postProcess.composite(m_compositeShader, compositeSettings, framebuffer, sceneView);
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

} // namespace game
