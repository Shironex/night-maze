// "Gameplay" debug panel: the state of the round, the battery and the numbers of the rules.
// See docs/modules/game/gameplay.md
#include "debug/panels/GameplayPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/Round.hpp"

#include <imgui.h>

namespace debug {

namespace {

// The charge of the battery: from empty to full.
constexpr float MIN_BATTERY = 0.0F;
constexpr float MAX_BATTERY = 1.0F;

// The part of the crystals that opens the gate. Not down to 0: at least one crystal is
// always needed (game::requiredCrystalCount), so smaller values would change nothing.
constexpr float MIN_REQUIRED_FRACTION = 0.05F;
constexpr float MAX_REQUIRED_FRACTION = 1.0F;

// How long a full battery lasts, in seconds: from a few seconds, to see it run out
// while watching, to ten minutes.
constexpr float MIN_BATTERY_LIFETIME = 5.0F;
constexpr float MAX_BATTERY_LIFETIME = 600.0F;

// How much of a full battery one crystal gives back.
constexpr float MIN_BATTERY_PER_CRYSTAL = 0.0F;
constexpr float MAX_BATTERY_PER_CRYSTAL = 1.0F;

// The charge below which the flashlight flickers. 0 switches the flicker off.
constexpr float MIN_LOW_BATTERY_THRESHOLD = 0.0F;
constexpr float MAX_LOW_BATTERY_THRESHOLD = 0.5F;

// The radius of the pickup sphere of a crystal, in metres. At 2 m a crystal is collected
// from the neighbouring cell, through the wall.
constexpr float MIN_PICKUP_RADIUS = 0.1F;
constexpr float MAX_PICKUP_RADIUS = 2.0F;

constexpr float PERCENT = 100.0F;

// What the round is doing, in words.
const char* stateName(game::RoundState state) {
    return state == game::RoundState::Won ? "won" : "playing";
}

// The round as it is: nothing here can be edited.
void drawRoundState(const game::Round& round) {
    ImGui::Text("Round: %s, %.1f s", stateName(round.state), round.elapsedSeconds);
    ImGui::Text("Crystals: %d collected, %d needed, %d in the maze", round.collectedCount,
                round.requiredCount, static_cast<int>(round.crystals.size()));

    if (!round.gateOpen) {
        ImGui::TextUnformatted("Gate: closed");
    } else if (round.gateProgress < 1.0F) {
        ImGui::Text("Gate: opening, %.0f%%", round.gateProgress * PERCENT);
    } else {
        ImGui::TextUnformatted("Gate: open");
    }

    // The levers of the maze and how many of them are pulled. Each pulled lever is one
    // wall that is open. The state has one entry per lever.
    ImGui::Text("Levers: %d pulled of %d", game::pulledLeverCount(round),
                static_cast<int>(round.interactables.leverPulled.size()));
    ImGui::Text("Note card: %s", round.noteOpen ? "open" : "closed");
}

// The battery: its charge can be dragged, to look at a low and at an empty battery.
void drawBattery(game::Round& round, game::GameplaySettings& settings) {
    ImGui::SliderFloat("Battery", &round.battery, MIN_BATTERY, MAX_BATTERY, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp);
    // Checkbox reads and writes a bool through the pointer. Without the drain the
    // charge stays where the slider put it.
    ImGui::Checkbox("Battery drains", &settings.batteryDrains);
}

// The numbers of the rules (game::GameplaySettings).
void drawRules(game::GameplaySettings& settings) {
    ImGui::SliderFloat("Crystals needed", &settings.requiredFraction, MIN_REQUIRED_FRACTION,
                       MAX_REQUIRED_FRACTION, "%.2f of all", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Battery lifetime", &settings.batteryLifetimeSeconds, MIN_BATTERY_LIFETIME,
                       MAX_BATTERY_LIFETIME, "%.0f s", ImGuiSliderFlags_AlwaysClamp);
    // What one collected crystal gives back, as a part of a full battery.
    ImGui::SliderFloat("Recharge", &settings.batteryPerCrystal, MIN_BATTERY_PER_CRYSTAL,
                       MAX_BATTERY_PER_CRYSTAL, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Flicker below", &settings.lowBatteryThreshold, MIN_LOW_BATTERY_THRESHOLD,
                       MAX_LOW_BATTERY_THRESHOLD, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat("Pickup radius", &settings.pickupRadius, MIN_PICKUP_RADIUS,
                       MAX_PICKUP_RADIUS, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
}

} // namespace

void drawGameplayPanel(game::Round& round, game::GameplaySettings& settings) {
    // First run only: the top edge of the window, right of the Camera panel and folded
    // to its title bar like it (the constant is in PanelLayout.hpp). Later ImGui
    // remembers the panel in imgui.ini.
    placePanelOnFirstUse(GAMEPLAY_PLACEMENT);
    if (ImGui::Begin("Gameplay")) {
        drawRoundState(round);

        // Button returns true only in the frame in which it was clicked. The panel only
        // asks: the game starts the round at the start of its next frame.
        if (ImGui::Button("Restart round (key R)")) {
            settings.restart = true;
        }
        // A switch for testing: opens every shortcut without walking to the levers. The
        // panel only asks here too. A restart closes the walls again.
        ImGui::SameLine();
        if (ImGui::Button("Pull all levers")) {
            settings.pullAllLevers = true;
        }

        ImGui::Separator();
        drawBattery(round, settings);

        ImGui::Separator();
        drawRules(settings);
    }
    ImGui::End();
}

} // namespace debug
