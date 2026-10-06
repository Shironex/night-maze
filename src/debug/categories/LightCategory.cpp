// "Light" category of the debug window: ambient light, the moon, the flashlight, the
// lights of the crystals and the highlight, and the shadows of the moon and of the
// flashlight with a picture of each shadow map.
// See docs/modules/scene/lights.md and docs/modules/renderer/shadows.md
#include "debug/categories/LightCategory.hpp"

#include "debug/DebugContext.hpp"
#include "debug/Pictures.hpp"
#include "debug/Widgets.hpp"
#include "game/Lighting.hpp"
#include "game/Round.hpp"
#include "game/ShadowMap.hpp"
#include "game/Shadows.hpp"
#include "gfx/Framebuffer.hpp"
#include "scene/Light.hpp"
#include "scene/LightSpace.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <algorithm>

namespace debug {

namespace {

// ---- Lights ----------------------------------------------------------------------------

// The moon light may come from any side of the compass.
constexpr float MIN_MOON_YAW_DEGREES = 0.0F;
constexpr float MAX_MOON_YAW_DEGREES = 360.0F;

// Its pitch is the angle of the travelling light against the ground: -90 shines straight
// down, close to 0 the light only grazes the ground. Above 0 the moon would shine from
// below the ground.
constexpr float MIN_MOON_PITCH_DEGREES = -90.0F;
constexpr float MAX_MOON_PITCH_DEGREES = -5.0F;

// Intensities start at 0 (off). The upper limits are well above the defaults, enough to
// overexpose the scene and see the colours clip.
constexpr float MIN_INTENSITY = 0.0F;
constexpr float MAX_MOON_INTENSITY = 2.0F;
constexpr float MAX_FLASHLIGHT_INTENSITY = 10.0F;
constexpr float MAX_POINT_INTENSITY = 10.0F;

// Half angles of the flashlight cone. Below 90 degrees, where a cone stops being a cone.
constexpr float MIN_CONE_DEGREES = 1.0F;
constexpr float MAX_CONE_DEGREES = 60.0F;
// How much a cone angle changes for one pixel of dragging, in degrees.
constexpr float CONE_DRAG_SPEED = 0.1F;
// Width of the two fields of the cone together, in pixels at 100 % display scaling.
constexpr float CONE_FIELDS_WIDTH = 142.0F;

// Reach of the flashlight and of a point light, in metres. The lower limits keep the
// radius away from 0, which scene::attenuationForRadius cannot divide by.
constexpr float MIN_FLASHLIGHT_RANGE = 2.0F;
constexpr float MAX_FLASHLIGHT_RANGE = 60.0F;
constexpr float MIN_POINT_RADIUS = 0.5F;
constexpr float MAX_POINT_RADIUS = 12.0F;

// Where the hand holds the flashlight, in metres from the eye. 0 for both puts the light
// back at the eye, where its shadows hide exactly behind the things that cast them.
// To the right the limit is game::MAX_FLASHLIGHT_HAND_RIGHT (0.25 m): half of the body
// of the player (0.3 m) minus the near plane of the shadow map, so the light never
// leaves the body box and cannot get into a wall the player stands sideways against.
// Downwards the limit is the height of a hand held at the hip.
constexpr float MIN_HAND_OFFSET = 0.0F;
constexpr float MAX_HAND_DOWN = 0.5F;

// The distance in front of the eye at which the beam crosses the line of view, in
// metres. The upper end is past the default reach of the beam: there the beam runs
// almost parallel to the view.
constexpr float MAX_CONVERGE_DISTANCE = 20.0F;

constexpr float MIN_SPECULAR_STRENGTH = 0.0F;
constexpr float MAX_SPECULAR_STRENGTH = 2.0F;

// The exponent of the highlight. 1 is a very wide highlight, 256 a small sharp dot.
// Below 1 the formula stops looking like a highlight, and pow(0, 0) is not defined.
constexpr float MIN_SHININESS = 1.0F;
constexpr float MAX_SHININESS = 256.0F;

// ---- Shadows ---------------------------------------------------------------------------

// The entries of the resolution list, in the order of the enum game::ShadowResolution:
// the number of the chosen entry is the value of the enum. ImGui wants the entries in
// one string, each ended by a zero character.
constexpr const char* RESOLUTION_ITEMS = "1024 x 1024\0"
                                         "2048 x 2048\0";

// The entries of the kernel list: entry number i is the radius i + game::MIN_PCF_RADIUS.
constexpr const char* PCF_KERNEL_ITEMS = "3 x 3\0"
                                         "5 x 5\0"
                                         "7 x 7\0";

// Range of the two bias sliders, in metres. At 0 the surfaces shade themselves (shadow
// acne): in stripes with both filters off, as an overall darkening with them on. The
// upper ends are far more than the 0.2 m of a wall, enough to see a shadow come loose
// from the wall that casts it (peter panning).
constexpr float MIN_BIAS = 0.0F;
constexpr float MAX_CONSTANT_BIAS = 0.5F;
constexpr float MAX_SLOPE_BIAS = 1.0F;

// Range of the strength slider: the share of the light a shadow takes away.
constexpr float MIN_STRENGTH = 0.0F;
constexpr float MAX_STRENGTH = 1.0F;

// Centimetres in a metre, for the size of a texel.
constexpr float CENTIMETRES_PER_METRE = 100.0F;

// The texels of a perspective map grow with the distance from the light, so their size
// is shown at this distance from it, in metres. At 1 m that size is also what a texel
// grows by with every metre.
constexpr float TEXEL_REFERENCE_DISTANCE = 1.0F;

// The picture of a shadow map is at most this wide, in pixels at 100 % display scaling.
constexpr float MAX_SHADOW_PICTURE_WIDTH = 240.0F;

// One light that casts shadows, as the Shadows tab sees it: what it edits and what it
// shows. A struct of references, built for one frame.
struct ShadowMapView {
    // The title of the card with the settings and of the card with the picture.
    const char* settingsTitle;
    const char* pictureTitle;
    // The caption and the tooltip of the picture.
    const char* pictureCaption;
    const char* pictureTooltip;
    // Editable: the switch of the shadows, the resolution of the map, the two parts of
    // the bias, the hardware filter, the PCF kernel and the strength.
    game::ShadowSettings& settings;
    // Read only: the size and the format of the map, and its picture.
    const game::ShadowMap& map;
    // Read only: how much the map covers and how large its texels are.
    const scene::LightSpace& lightSpace;
    // Whether the map is drawn in the frames the game draws now. False: a note stands
    // in place of the picture, which would be an old one.
    bool drawn;
};

void drawAmbient(Page& page, game::LightingSettings& lighting) {
    page.beginCard("Ambient");
    // value_ptr gives the address of the three floats of a glm::vec3.
    page.color("Ambient", glm::value_ptr(lighting.ambient),
               "The light that reaches every surface, also where no lamp shines and in "
               "every shadow.");
    page.endCard();
}

// The moon: a directional light.
void drawMoon(Page& page, game::LightingSettings& lighting) {
    page.beginCard("Moon");
    // The two angles say which way the light travels (scene::directionFromAngles).
    page.slider("Moon yaw", &lighting.moonYawDegrees, MIN_MOON_YAW_DEGREES, MAX_MOON_YAW_DEGREES,
                "%.0f deg",
                "The side of the compass the moon light comes from. It also turns the "
                "shadows of the moon. The painted moon of the sky does not follow it.");
    page.slider("Moon pitch", &lighting.moonPitchDegrees, MIN_MOON_PITCH_DEGREES,
                MAX_MOON_PITCH_DEGREES, "%.0f deg",
                "The angle of the moon light against the ground: -90 shines straight "
                "down, close to 0 the light only grazes the ground.");
    page.color("Moon colour", glm::value_ptr(lighting.moonColor), "The colour of the moon light.");
    page.slider("Moon intensity", &lighting.moonIntensity, MIN_INTENSITY, MAX_MOON_INTENSITY,
                "%.2f", "How strong the moon light is. 0 switches it off.");
    page.endCard();
}

// The flashlight: a spot light in the hand of the player.
void drawFlashlight(Page& page, game::LightingSettings& lighting, const game::Round& round) {
    page.beginCard("Flashlight");

    // The same switch as the F key. With an empty battery the game turns it off again
    // in its next step, so the switch cannot stay on: the tooltip then says why.
    page.toggle("Flashlight on (key F)", &lighting.flashlightOn,
                round.battery <= 0.0F ? "The battery is empty: collect a crystal first."
                                      : "Switches the flashlight, like the F key. With an "
                                        "empty battery it goes off again at once.");
    page.color("Beam colour", glm::value_ptr(lighting.flashlightColor),
               "The colour of the light of the flashlight.");
    page.slider("Beam intensity", &lighting.flashlightIntensity, MIN_INTENSITY,
                MAX_FLASHLIGHT_INTENSITY, "%.2f", "How strong the flashlight is.");

    // One widget for the two half angles of the cone: two fields that are dragged with
    // the mouse (or typed into after a double click). DragFloatRange2 keeps the first
    // value at or below the second, so the inner cone is never wider than the outer one.
    const float coneWidth = CONE_FIELDS_WIDTH * displayScale();
    if (page.beginRow("Cone",
                      "The two half angles of the cone of light, in degrees: inside the "
                      "inner one (left field) the light has its full strength, at the "
                      "outer one (right field) it has faded to nothing. Drag a field, or "
                      "double click it to type.",
                      coneWidth)) {
        ImGui::SetNextItemWidth(coneWidth);
        ImGui::DragFloatRange2("##cone", &lighting.flashlightInnerDegrees,
                               &lighting.flashlightOuterDegrees, CONE_DRAG_SPEED, MIN_CONE_DEGREES,
                               MAX_CONE_DEGREES, "%.1f deg", "%.1f deg",
                               ImGuiSliderFlags_AlwaysClamp);
        page.endRow();
    }
    page.slider("Beam range", &lighting.flashlightRange, MIN_FLASHLIGHT_RANGE, MAX_FLASHLIGHT_RANGE,
                "%.1f m",
                "How far the light reaches. It is also the far plane of the shadow map of "
                "the flashlight.");

    // The hand: where the light stands, measured from the eye (game::flashlightPose).
    page.slider("Hand right", &lighting.flashlightHandRight, MIN_HAND_OFFSET,
                game::MAX_FLASHLIGHT_HAND_RIGHT, "%.2f m",
                "How far to the right of the eye the flashlight is held. At the eye (0 "
                "here and below) every shadow of the flashlight hides behind the thing "
                "that casts it. The limit keeps the light inside the body of the player, "
                "which is 0.6 m wide, so it cannot get into a wall.");
    page.slider("Hand down", &lighting.flashlightHandDown, MIN_HAND_OFFSET, MAX_HAND_DOWN, "%.2f m",
                "How far below the eye the flashlight is held: straight down in the "
                "world, whatever the camera looks at.");
    page.slider("Converge at", &lighting.flashlightConvergeDistance,
                game::MIN_FLASHLIGHT_CONVERGE_DISTANCE, MAX_CONVERGE_DISTANCE, "%.1f m",
                "The beam points from the hand at the point this far in front of the eye. "
                "On a wall at this distance the spot is in the middle of the screen. "
                "Nearer walls show it to the right and below, farther ones a little to "
                "the left and above.");

    page.endCard();
}

// The point lights of the crystals. They are edited as a group: one colour, one
// intensity and one radius for all of them. The colour is also the colour the crystals
// glow in.
void drawCrystalLights(Page& page, game::LightingSettings& lighting, const game::Round& round) {
    page.beginCard("Crystal lights");
    // Every crystal that is not collected yet carries one light.
    const int crystalCount = static_cast<int>(round.crystals.size());
    page.stat("Lit", "%d of %d crystals (at most %d)", crystalCount - round.collectedCount,
              crystalCount, scene::MAX_POINT_LIGHTS);
    page.color("Point colour", glm::value_ptr(lighting.pointColor),
               "The colour of the lights of the crystals, and the colour the crystals "
               "glow in.");
    page.slider("Point intensity", &lighting.pointIntensity, MIN_INTENSITY, MAX_POINT_INTENSITY,
                "%.2f", "How strong the light of a crystal is.");
    page.slider("Point radius", &lighting.pointRadius, MIN_POINT_RADIUS, MAX_POINT_RADIUS, "%.1f m",
                "How far the light of a crystal reaches.");
    page.endCard();
}

// The highlight: the two numbers of the material that the lit shaders use.
void drawHighlight(Page& page, game::LightingSettings& lighting) {
    page.beginCard("Highlight");
    page.slider("Strength", &lighting.specularStrength, MIN_SPECULAR_STRENGTH,
                MAX_SPECULAR_STRENGTH, "%.2f",
                "How bright the shiny highlight (the specular part of the light) is.");
    // Logarithmic: half of the slider covers the small exponents, where one step changes
    // the size of the highlight the most.
    page.slider("Shininess", &lighting.shininess, MIN_SHININESS, MAX_SHININESS, "%.0f",
                "The exponent of the highlight: 1 is a very wide highlight, 256 a small "
                "sharp dot. Logarithmic: half of the bar covers the small exponents.",
                true);
    page.endCard();
}

void drawLightsTab(Page& page, const DebugContext& context) {
    game::LightingSettings& lighting = context.lighting;
    page.setPlace("Light / Lights");
    page.beginColumns();
    drawAmbient(page, lighting);
    drawMoon(page, lighting);
    drawCrystalLights(page, lighting, context.round);
    drawHighlight(page, lighting);
    page.nextColumn();
    drawFlashlight(page, lighting, context.round);
    page.endColumns();
}

// The rows of one shadow map and what the map is and covers.
void drawShadowSettings(Page& page, const ShadowMapView& view) {
    game::ShadowSettings& settings = view.settings;
    page.beginCard(view.settingsTitle);

    page.toggle("Shadows", &settings.enabled,
                "Off: the shadow map of this light is not drawn and nothing is in its "
                "shadow. The shadows of the other light stay.");

    int resolutionIndex = static_cast<int>(settings.resolution);
    if (page.combo("Resolution", &resolutionIndex, RESOLUTION_ITEMS,
                   "The size of the shadow map in texels. The map covers the same area at "
                   "every size, so a smaller map has larger texels: coarser shadow edges, "
                   "and more bias is needed.")) {
        settings.resolution = static_cast<game::ShadowResolution>(resolutionIndex);
    }

    page.slider("Constant bias", &settings.constantBias, MIN_BIAS, MAX_CONSTANT_BIAS, "%.3f m",
                "Every surface is compared with the shadow map as if it were this much "
                "nearer to the light, in metres for both lights. With both parts at 0 the "
                "surfaces shade themselves (shadow acne): stripes with both filters off, "
                "an overall darkening with them on.");
    page.slider("Slope bias", &settings.slopeBias, MIN_BIAS, MAX_SLOPE_BIAS, "%.3f m",
                "Added on top for surfaces the light only grazes: bias = constant + slope "
                "* (1 - cos of the angle between the normal and the light). Too much bias "
                "lets a shadow come loose from the wall that casts it (peter panning).");

    page.toggle("Hardware 2 x 2 filter", &settings.hardwareFilter,
                "The graphics card compares the four texels around a place and blends the "
                "four answers (a linear comparison sampler). Off: one texel, and the "
                "edges of the shadows show steps.");
    page.toggle("PCF", &settings.pcf,
                "Percentage closer filtering: the comparison is made for a square of "
                "texels and the answers are averaged, which makes the edge of a shadow "
                "soft.");
    // The list shows sizes and the settings hold a radius: entry 0 is radius 1.
    int kernelIndex = std::clamp(settings.pcfRadius, game::MIN_PCF_RADIUS, game::MAX_PCF_RADIUS) -
                      game::MIN_PCF_RADIUS;
    if (page.combo("Kernel", &kernelIndex, PCF_KERNEL_ITEMS,
                   "How many texels the PCF square has. A larger kernel is softer and "
                   "costs more lookups per pixel: 9, 25 or 49.")) {
        settings.pcfRadius = kernelIndex + game::MIN_PCF_RADIUS;
    }

    page.slider("Strength", &settings.strength, MIN_STRENGTH, MAX_STRENGTH, "%.2f",
                "The share of the light of this lamp a shadow takes away. 1: none of it "
                "is left in a shadow. The ambient light and the other lights are never "
                "darkened.");

    // What the map is.
    const gfx::Framebuffer& target = view.map.target();
    if (view.drawn && target.isValid()) {
        page.stat("Map", "%d x %d, %s", target.width(), target.height(),
                  gfx::depthFormatName(target.depthFormat()));
    } else {
        page.stat("Map", "not drawn");
    }

    // How much it covers. The size of a texel is given at the resolution that is
    // chosen, also while the map is off.
    const scene::LightSpace& lightSpace = view.lightSpace;
    const int mapSize = game::shadowMapSize(settings.resolution);
    if (lightSpace.kind == scene::LightProjection::Perspective) {
        // A pyramid: what the map covers and the size of its texels grow in proportion
        // to the distance from the light. So the covered area is given at the far
        // plane, where it is largest, and the texel as its size 1 m from the light,
        // which is also how much it grows with every metre.
        page.stat("Covers", "%.1f x %.1f m at %.1f m", lightSpace.extent.x, lightSpace.extent.y,
                  lightSpace.farPlane);
        const float texelPerMetre =
            game::shadowTexelSizeAt(lightSpace, mapSize, TEXEL_REFERENCE_DISTANCE);
        page.stat("One texel", "%.2f cm per metre away", texelPerMetre * CENTIMETRES_PER_METRE);
    } else {
        // A box: the same everywhere.
        page.stat("Covers", "%.1f x %.1f m, %.1f m deep", lightSpace.extent.x, lightSpace.extent.y,
                  lightSpace.extent.z);
        const float texelSize = game::shadowTexelSize(lightSpace, mapSize);
        page.stat("One texel", "%.1f cm", texelSize * CENTIMETRES_PER_METRE);
    }

    page.endCard();
}

// The picture of one shadow map: a plain GL_RGBA8 texture the game draws for it. The
// depth texture itself is never handed to ImGui.
void drawShadowPicture(Page& page, const ShadowMapView& view) {
    page.beginCard(view.pictureTitle);
    if (page.beginBlock("shadow map picture depth preview")) {
        // The picture is asked for only while this block is drawn. The game reads the
        // flag in its next frame, and DebugUI::draw clears it before every frame, so
        // the picture of a card nobody looks at is not drawn.
        view.settings.preview = true;
        const float width =
            std::min(ImGui::GetContentRegionAvail().x, MAX_SHADOW_PICTURE_WIDTH * displayScale());
        drawFramebufferPicture(view.pictureCaption, view.pictureTooltip, view.map.preview(), width,
                               view.drawn);
    }
    page.endCard();
}

void drawShadowsTab(Page& page, const DebugContext& context) {
    // One more light with a shadow map is one more view here and one more pair of
    // calls below.
    const ShadowMapView moon{
        .settingsTitle = "Moon shadows",
        .pictureTitle = "Moon shadow map",
        .pictureCaption = "Depth seen from the moon",
        .pictureTooltip = "The shadow map: black is near the moon, white is far from it or "
                          "empty. The walls are the dark lines.",
        .settings = context.moonShadowSettings,
        .map = context.moonShadowMap,
        .lightSpace = context.moonLightSpace,
        .drawn = context.moonShadowSettings.enabled,
    };
    const ShadowMapView flashlight{
        .settingsTitle = "Flashlight shadows",
        .pictureTitle = "Flashlight shadow map",
        .pictureCaption = "Distance seen from the flashlight",
        .pictureTooltip = "The shadow map of the flashlight: black is at the hand, white is "
                          "as far as the beam reaches, or empty. The map has a perspective "
                          "projection, so its stored depth is turned back into metres for "
                          "this picture: shown as it is, the picture would be almost white. "
                          "No picture while the flashlight is off.",
        .settings = context.flashlightShadowSettings,
        .map = context.flashlightShadowMap,
        .lightSpace = context.flashlightLightSpace,
        .drawn = context.flashlightShadowDrawn,
    };

    // The moon in the left column, the flashlight in the right one, with the same rows:
    // the two lights can be compared line by line.
    page.setPlace("Light / Shadows");
    page.beginColumns();
    drawShadowSettings(page, moon);
    drawShadowPicture(page, moon);
    page.nextColumn();
    drawShadowSettings(page, flashlight);
    drawShadowPicture(page, flashlight);
    page.endColumns();
}

} // namespace

void drawLightCategory(Page& page, const DebugContext& context, LightTab tab) {
    // While the user searches, the rows of both tabs can be found.
    const bool all = page.searching();
    if (all || tab == LightTab::Lights) {
        drawLightsTab(page, context);
    }
    if (all || tab == LightTab::Shadows) {
        drawShadowsTab(page, context);
    }
}

} // namespace debug
