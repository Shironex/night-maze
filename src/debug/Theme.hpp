// Theme of the debug panels: night colours, soft metrics and the panel font.
// See docs/modules/debug-ui.md
#pragma once

// This header shows ImGui types (ImVec4), so it includes imgui.h. Only the .cpp files of
// src/debug include it, so the rest of the project still does not depend on ImGui.
#include <imgui.h>

#include <vector>

namespace debug {

/// Makes an ImGui colour from red, green and blue written as whole numbers from 0 to 255,
/// the way colours are usually written down. ImGui wants floats from 0 to 1.
/// alpha is the opacity: 1 hides what is behind, 0 is invisible.
constexpr ImVec4 colorFromBytes(int red, int green, int blue, float alpha = 1.0F) {
    constexpr float BYTE_MAX = 255.0F;
    return {static_cast<float>(red) / BYTE_MAX, static_cast<float>(green) / BYTE_MAX,
            static_cast<float>(blue) / BYTE_MAX, alpha};
}

// The colours below have a meaning of their own, so the panels name them directly
// (ImGui::PushStyleColor, draw lists). All other colours of the theme are in Theme.cpp.

/// Text of a failed load (Shaders and Assets panels): a soft red that stays readable on
/// the dark panel background.
inline constexpr ImVec4 ERROR_TEXT_COLOR = colorFromBytes(255, 150, 138);

/// Walls on the plan of the Maze panel: pale stone in moonlight.
inline constexpr ImVec4 PLAN_WALL_COLOR = colorFromBytes(176, 190, 216);

/// The player on the plan of the Maze panel: the warm light of the flashlight.
inline constexpr ImVec4 PLAN_PLAYER_COLOR = colorFromBytes(255, 184, 84);

/// Crystals on the plan of the Maze panel, and everything about crystals in the HUD: the
/// cyan the crystals glow in.
inline constexpr ImVec4 PLAN_CRYSTAL_COLOR = colorFromBytes(86, 214, 202);
inline constexpr ImVec4 HUD_CRYSTAL_COLOR = PLAN_CRYSTAL_COLOR;

/// A crystal that is already collected, on the plan: only a dim trace of where it was.
inline constexpr ImVec4 PLAN_COLLECTED_COLOR = colorFromBytes(44, 104, 112);

/// The gate on the plan while it is closed: the brown of its wood, made light enough to
/// stand out from the walls. An open gate is drawn like a collected crystal.
inline constexpr ImVec4 PLAN_GATE_COLOR = colorFromBytes(214, 142, 82);

/// The exit zone on the plan: a soft green, the colour of "this way out".
inline constexpr ImVec4 PLAN_EXIT_COLOR = colorFromBytes(132, 220, 140);

/// The battery bar of the HUD: the warm light of the flashlight while there is charge,
/// the soft red of an error once it is low.
inline constexpr ImVec4 HUD_BATTERY_COLOR = PLAN_PLAYER_COLOR;
inline constexpr ImVec4 HUD_BATTERY_LOW_COLOR = ERROR_TEXT_COLOR;

/// A lever on the plan of the Maze panel while it can still be pulled: the red of
/// a switch. A pulled lever is drawn like a collected crystal.
inline constexpr ImVec4 PLAN_LEVER_COLOR = colorFromBytes(238, 92, 62);

/// A note on the plan of the Maze panel, and the title of the note card in the HUD: the
/// pale yellow of old paper.
inline constexpr ImVec4 PLAN_NOTE_COLOR = colorFromBytes(230, 220, 178);
inline constexpr ImVec4 HUD_NOTE_COLOR = PLAN_NOTE_COLOR;

/// The crosshair in the middle of the screen: a quiet grey white while it points at
/// nothing, and the warm colour of the flashlight when the player can use what it
/// points at. The same warm colour writes the prompt (the line that names the key).
inline constexpr ImVec4 HUD_CROSSHAIR_COLOR = colorFromBytes(214, 220, 232);
inline constexpr ImVec4 HUD_CROSSHAIR_ACTIVE_COLOR = PLAN_PLAYER_COLOR;

/// Sets the colours and the metrics (padding, spacing, rounding, font size) of all panels.
///
/// scale is the content scale of the display: 1 at 100 %, 1.5 at 150 % display scaling on
/// Windows. All metrics and the font are multiplied by it. A value that is not positive
/// counts as 1.
/// Call it once, after ImGui::CreateContext and before the first frame.
void applyTheme(float scale);

/// Loads the panel font from assets/fonts into ImGui.
///
/// The bytes of the file are stored in fontBytes and ImGui keeps a pointer to them, so
/// the vector must stay alive and unchanged until ImGui::DestroyContext.
/// When the file cannot be read or is not a font, one error is logged, fontBytes is left
/// empty and the font built into ImGui is used instead.
/// Call it once, after ImGui::CreateContext and before the first frame.
void loadFont(std::vector<unsigned char>& fontBytes);

} // namespace debug
