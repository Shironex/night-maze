// "Lights" debug panel: ambient light, the moon, the flashlight, the point lights, the highlight.
// See docs/modules/scene/lights.md
#include "debug/panels/LightsPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/Lighting.hpp"
#include "game/Round.hpp"
#include "scene/Light.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

namespace debug {

namespace {

// The moon light may come from any side of the compass.
constexpr float MIN_MOON_YAW_DEGREES = 0.0F;
constexpr float MAX_MOON_YAW_DEGREES = 360.0F;

// Its pitch is the angle of the travelling light against the ground: -90 shines straight
// down, close to 0 the light only grazes the floor. Above 0 the moon would shine from
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

// Reach of the flashlight and of a point light, in metres. The lower limits keep the
// radius away from 0, which scene::attenuationForRadius cannot divide by.
constexpr float MIN_FLASHLIGHT_RANGE = 2.0F;
constexpr float MAX_FLASHLIGHT_RANGE = 60.0F;
constexpr float MIN_POINT_RADIUS = 0.5F;
constexpr float MAX_POINT_RADIUS = 12.0F;

constexpr float MIN_SPECULAR_STRENGTH = 0.0F;
constexpr float MAX_SPECULAR_STRENGTH = 2.0F;

// The exponent of the highlight. 1 is a very wide highlight, 256 a small sharp dot.
// Below 1 the formula stops looking like a highlight, and pow(0, 0) is not defined.
constexpr float MIN_SHININESS = 1.0F;
constexpr float MAX_SHININESS = 256.0F;

// The moon: a directional light.
void drawMoon(game::LightingSettings& lighting) {
    // CollapsingHeader draws a title bar that folds its group away. It returns true
    // while the group is open. This group starts folded: the panel is exactly as tall
    // as the other three groups together, and the moon is the light that is changed
    // least often. A click on the bar opens it (the panel then scrolls).
    if (!ImGui::CollapsingHeader("Moon (directional)")) {
        return;
    }
    // The two angles say which way the light travels (scene::directionFromAngles).
    ImGui::SliderFloat("Moon yaw", &lighting.moonYawDegrees, MIN_MOON_YAW_DEGREES,
                       MAX_MOON_YAW_DEGREES, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Moon pitch", &lighting.moonPitchDegrees, MIN_MOON_PITCH_DEGREES,
                       MAX_MOON_PITCH_DEGREES, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
    // ColorEdit3 reads and writes three floats through the pointer. value_ptr gives the
    // address of the three floats of a glm::vec3.
    ImGui::ColorEdit3("Moon colour", glm::value_ptr(lighting.moonColor));
    ImGui::SliderFloat("Moon intensity", &lighting.moonIntensity, MIN_INTENSITY, MAX_MOON_INTENSITY,
                       "%.2f", ImGuiSliderFlags_AlwaysClamp);
}

// The flashlight: a spot light at the eye of the player.
void drawFlashlight(game::LightingSettings& lighting, const game::Round& round) {
    // DefaultOpen: the group is open the first time the program runs.
    if (!ImGui::CollapsingHeader("Flashlight (spot)", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    // The same switch as the F key. With an empty battery the game turns it off again
    // in its next step, so the box cannot stay ticked: the tooltip says why.
    ImGui::Checkbox("Flashlight on (key F)", &lighting.flashlightOn);
    if (round.battery <= 0.0F) {
        ImGui::SetItemTooltip("The battery is empty: collect a crystal first.");
    }
    ImGui::ColorEdit3("Beam colour", glm::value_ptr(lighting.flashlightColor));
    ImGui::SliderFloat("Beam intensity", &lighting.flashlightIntensity, MIN_INTENSITY,
                       MAX_FLASHLIGHT_INTENSITY, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    // One widget for the two half angles of the cone: two fields that are dragged with
    // the mouse (or typed into after a double click). DragFloatRange2 keeps the first
    // value at or below the second, so the inner cone is never wider than the outer one.
    ImGui::DragFloatRange2("Cone", &lighting.flashlightInnerDegrees,
                           &lighting.flashlightOuterDegrees, CONE_DRAG_SPEED, MIN_CONE_DEGREES,
                           MAX_CONE_DEGREES, "inner %.1f deg", "outer %.1f deg",
                           ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Beam range", &lighting.flashlightRange, MIN_FLASHLIGHT_RANGE,
                       MAX_FLASHLIGHT_RANGE, "%.1f m", ImGuiSliderFlags_AlwaysClamp);
}

// The point lights of the crystals. They are edited as a group: one colour, one
// intensity and one radius for all of them. The colour is also the colour the crystals
// glow in.
void drawPointLights(game::LightingSettings& lighting, const game::Round& round) {
    if (!ImGui::CollapsingHeader("Point lights (crystals)", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    // Every crystal that is not collected yet carries one light.
    const int crystalCount = static_cast<int>(round.crystals.size());
    ImGui::Text("Lit: %d of %d crystals (at most %d)", crystalCount - round.collectedCount,
                crystalCount, scene::MAX_POINT_LIGHTS);
    ImGui::ColorEdit3("Point colour", glm::value_ptr(lighting.pointColor));
    ImGui::SliderFloat("Point intensity", &lighting.pointIntensity, MIN_INTENSITY,
                       MAX_POINT_INTENSITY, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Point radius", &lighting.pointRadius, MIN_POINT_RADIUS, MAX_POINT_RADIUS,
                       "%.1f m", ImGuiSliderFlags_AlwaysClamp);
}

// The highlight: the two numbers of the material that the lit shaders use.
void drawHighlight(game::LightingSettings& lighting) {
    if (!ImGui::CollapsingHeader("Highlight (specular)", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    ImGui::SliderFloat("Strength", &lighting.specularStrength, MIN_SPECULAR_STRENGTH,
                       MAX_SPECULAR_STRENGTH, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    // Logarithmic: half of the slider covers the small exponents, where one step changes
    // the size of the highlight the most.
    ImGui::SliderFloat("Shininess", &lighting.shininess, MIN_SHININESS, MAX_SHININESS, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
}

} // namespace

void drawLightsPanel(game::LightingSettings& lighting, const game::Round& round) {
    // First run only: the left edge of the window, below the Renderer panel (the constant
    // is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(LIGHTS_PLACEMENT);
    if (ImGui::Begin("Lights")) {
        ImGui::ColorEdit3("Ambient", glm::value_ptr(lighting.ambient));
        drawMoon(lighting);
        drawFlashlight(lighting, round);
        drawPointLights(lighting, round);
        drawHighlight(lighting);
    }
    ImGui::End();
}

} // namespace debug
