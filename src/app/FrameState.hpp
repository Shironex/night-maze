// What the drawing of one frame shares between its parts, and what it keeps from one frame
// to the next for the passes, the HUD and the debug UI.
#pragma once

#include "game/lighting/Lighting.hpp"
#include "game/menu/MenuCamera.hpp"
#include "game/player/Interaction.hpp"
#include "game/screens/Intro.hpp"
#include "game/shade/Shade.hpp"
#include "scene/Camera.hpp"
#include "scene/LightSpace.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace game {

/// What onRender knows about the screen before it draws: asked once, after the menus
/// and the menu camera switch of the frame, and used by the rest of the frame.
struct FrameScreen {
    /// The keys and the mouse of the round work: a round is played, the menu camera
    /// is off and no catch is going on.
    bool roundInput = false;
    /// The game is on the screen of the intro.
    bool intro = false;
    /// The picture is taken by the menu camera: it is switched on, the live scene is
    /// behind the main menu, or the intro plays.
    bool menuCamera = false;
    /// The settings the menu camera uses in this frame: the ones of the intro, the
    /// high glide of the main menu, or what the recording tool is set to.
    MenuCameraSettings menuCameraSettings;
    /// Where the script of the intro is (game::introFrame) and its shot
    /// (game::introCamera). Used only while intro is true.
    IntroFrame introMoment;
    IntroCamera introShot;
};

/// Where a frame is seen from: built once per frame (frameView) and given to the
/// parts that draw.
struct FrameView {
    /// The feet of the player, blended between two fixed steps.
    glm::vec3 feet{0.0F};
    /// The place the picture is taken from: the eyes of the player, the menu camera
    /// or the camera of the village reveal.
    glm::vec3 eye{0.0F};
    /// The eyes of the player (or of the menu camera) before the reveal moved the
    /// camera away: the flashlight stays there.
    glm::vec3 playerEye{0.0F};
    /// The camera of the frame: a copy of the camera of the player with the angles and the lens of
    /// whoever takes the picture.
    scene::Camera camera;
    /// The camera of the village reveal takes the picture.
    bool reveal = false;
    /// The two matrices everything of the frame is drawn with.
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
};

/// The lamp of the gate in a frame: how far its colour has turned from the cold
/// ember to amber, and how bright it is (game::gateLampStrength).
struct GateLampFrame {
    float progress = 0.0F;
    float strength = 0.0F;
};

/// How many nights of the village and of the land are lit in this frame, and how far
/// the newest of them has come on: the rule of the campaign (game::villageNightsLit)
/// and the reveal (game::villageNewLightStrength).
struct NightLights {
    int nightsLit = 0;
    float newestStrength = 1.0F;
};

/// The state of the drawing that outlives one part of onRender: set while a frame is drawn
/// and read by the passes of the same frame, by the HUD and the debug UI after it, and in
/// two cases (shadeWalkAmount, heldLampSway) carried on into the next frame.
struct FrameState {
    /// The model matrices of the walls for the frame that is being drawn
    /// (game::roundWallMatrices): a wall that a lever has opened is lower in every frame
    /// while it sinks. Built once per frame in onRender and used by the shadow passes and
    /// by the scene pass, so the shadow of a wall always fits the wall.
    std::vector<glm::mat4> wallMatrices;
    /// Whether the shade is drawn in the frame that is being drawn, and its model matrix:
    /// where it stands between two fixed steps, turned towards the player. Set once per
    /// frame in onRender, like wallMatrices, and used by the shadow passes and by the
    /// scene pass. The shade is drawn only while a round is played: never under a menu,
    /// behind the main menu or in the picture of the menu camera. The one exception is
    /// the card of the intro that shows it standing in a corridor (game::introShadeCell).
    bool shadeDrawn = false;
    glm::mat4 shadeMatrix{1.0F};
    /// How bright the night in the hood of that shade is: the brightness of the sky is
    /// multiplied by it (game::shadeHoodBrightness), and how much its stars are lifted
    /// above that (game::shadeHoodStarBoost). Both follow the burn clock of the round,
    /// and are the values of a shade at rest for the one of the intro.
    float shadeHoodBrightness = SHADE_HOOD_REST_BRIGHTNESS;
    float shadeHoodStarBoost = 0.0F;
    /// How much the drawn shade walks, from 0 (stands) to 1 (walks). It follows the state
    /// of the shade over a third of a second, so the pose never jumps (game::shadeSwayPose).
    float shadeWalkAmount = 0.0F;
    /// The sway of the lamp in the hand: drawing only, advanced with the frame time while
    /// the round runs (drawHeldLamp).
    HeldLampSway heldLampSway;
    /// The picking of this frame: the ray, what it hits and what the interaction key
    /// does. Built in onRender, read by the drawing (the highlight), the HUD and the
    /// debug UI.
    PickState pick;
    /// The picking the debug view draws: a copy of pick that stops following it while
    /// the "freeze" switch of RenderSettings::pickDebug is set.
    PickState shownPick;

    /// The shadows of the moon: the view and the projection of the moon in this frame,
    /// and whether the shadow pass has filled the map in this frame. Both are set by
    /// drawMoonShadowMap in every frame.
    scene::LightSpace moonLightSpace;
    bool moonShadowDrawn = false;

    /// The lit nights of this frame (nightLightsFor), set before the scene is drawn: the
    /// windows of the land in the scene pass and the village after it show the same.
    NightLights frameNightLights;

    /// Whether the map was drawn in the last frame. Set in onRender. The HUD asks it
    /// (main.cpp passes it on): while the map covers the middle of the window, the timed
    /// sentence of the HUD waits, because its place is where the bottom edge of the map
    /// is. Everything else of the HUD stands in the corners and stays.
    bool mapOnScreen = false;

    /// The same two for the shadows of the flashlight, set by drawFlashlightShadowMap
    /// in every frame.
    scene::LightSpace flashlightLightSpace;
    bool flashlightShadowDrawn = false;

    /// The glow of the lanterns of the gate in the frame that is being drawn, as a linear
    /// colour for the uniform uEmissive: the colour of the lamp times its strength
    /// (game::gateLampColor, game::gateLampStrength). onRender sets it before the shadow
    /// passes, and every pass of the frame draws the lanterns with it.
    glm::vec3 gateLampGlow{0.0F};
};

} // namespace game
