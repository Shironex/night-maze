// The Night Maze application: the maze being built, the start of a round and its fixed step.
#include "game/NightMazeApp.hpp"

#include "core/Log.hpp"
#include "game/Exit.hpp"
#include "game/GateLamp.hpp"
#include "game/Interactables.hpp"
#include "game/Puddles.hpp"
#include "game/Shade.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace game {

namespace {

// The keys of the player (walk, sprint, use, flashlight, map, restart) are not named
// here: the player chooses them on the settings screen (game/KeyBindings.hpp), and they
// are read through actionDown and actionPressed. What follows are the keys of the
// tools. They are fixed, and game::isFixedKey lists them so that no action can be put
// on one of them.

// The two keys that fly up and down in noclip. They belong to that tool only, so the
// player may put an action on them: Left Shift is the sprint of a game without
// a settings file.
constexpr int NOCLIP_UP_KEY = GLFW_KEY_SPACE;
constexpr int NOCLIP_DOWN_KEY = GLFW_KEY_LEFT_SHIFT;

// Key that switches the menu camera on and off: the game then shows itself.
constexpr int MENU_CAMERA_KEY = GLFW_KEY_F2;

// The name of the difficulty of a maze that no level describes: one the debug UI built.
constexpr const char* CUSTOM_DIFFICULTY_NAME = "Custom";

// Pitch of a level look, in degrees: how the player looks at the start.
constexpr float LEVEL_PITCH_DEGREES = 0.0F;

// The smallest height scale of the terrain: a flat world.
constexpr float MIN_HEIGHT_SCALE = 0.0F;

} // namespace

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
    // The still layer of the shadow map of the moon holds the terrain, the walls and
    // the pillars as they were. It is not drawn here: the number goes into the key the
    // next shadow pass compares (drawMoonShadowMap), also when the shadows are off now.
    ++m_groundVersion;
    // The boxes the culling tests: the walls, the pillars and the stone sheep stand
    // where the new world has put them, so their boxes are computed again, once, here.
    m_mazeBounds = m_mazeRenderer.bounds(m_mazeWorld);
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
    // The shade itself is part of the round: startRound has put it back in its start
    // cell, with its grace time ahead of it.
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

    // The step itself, in the order that matters: the player moves, the rules of the
    // round see where the player is now, and the shade comes last, because it needs the
    // battery of this step.
    const float walkedMetres = movePlayer(fixedDt);
    const bool pickedUp = stepRoundRules(fixedDt);
    const bool caught = stepShade(fixedDt, walkedMetres, pickedUp);

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
        // And a moment later the sting of a won night (updateRoundMusic).
        armNightEndSting(m_nightEndSting);
        tollBellSwing(m_bellSwing);
        finishRound();
    }
}

float NightMazeApp::movePlayer(double fixedDt) {
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
    return walkedMetres;
}

bool NightMazeApp::stepRoundRules(double fixedDt) {
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
    // Whether a crystal or a flask was picked up in this step: the shade may hear it.
    return m_round.collectedCount > soundBefore.collectedCount ||
           m_round.flasksCollected > flasksBefore;
}

bool NightMazeApp::stepShade(double fixedDt, float walkedMetres, bool pickedUp) {
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
    const Noise noise = playerNoise({.metres = walkedMetres,
                                     .stepSeconds = static_cast<float>(fixedDt),
                                     .walkSpeed = m_player.walkSpeed,
                                     .sprintSpeed = m_player.sprintSpeed,
                                     .flying = m_player.noclip,
                                     .pickedUp = pickedUp,
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
    // Its hum is a loop, said in every frame (updateAmbience).
    // Its steps, while it walks and is near: quieter the farther it still has to go.
    CuePlay step;
    if (advanceShadeSteps(m_shadeSteps, m_round, step)) {
        playCue(step.cue, step.volume);
    }
    return caught;
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

} // namespace game
