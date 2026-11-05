// Theme of the debug window: night colours, calm metrics and the panel font.
#pragma once

#include "debug/HudColors.hpp"

// This header shows ImGui types (ImVec4), so it includes imgui.h. Only the .cpp files of
// src/debug include it, so the rest of the project still does not depend on ImGui.
#include <imgui.h>

#include <vector>

namespace debug {

// The colours below have a meaning of their own, so the code of the debug window names
// them directly (ImGui::PushStyleColor, draw lists). All other colours of the theme are
// in Theme.cpp. The ones it shares with the HUD are made in HudColors.hpp
// (colorFromBytes too), and named here again by what they do in the window.

// ---- Tokens of the debug window --------------------------------------------------------
// The widgets drawn by hand (Widgets.cpp, Icons.cpp, DebugWindow.cpp) take every colour
// from this list, so the window has one look: the amber of the flashlight for what acts
// or is switched on, the teal of the crystals for what is only shown, on night navy.

/// Height of the text of the debug window in pixels at 100 % display scaling. One size
/// for everything. The built-in font of ImGui is 13 pixels high.
inline constexpr float FONT_SIZE = 14.0F;

/// Text: the pale, slightly blue white of moonlight.
inline constexpr ImVec4 TEXT_COLOR = HUD_TEXT_COLOR;
/// Quieter text (labels of read only lines, hints, units): moonlight behind a cloud.
inline constexpr ImVec4 TEXT_DIM_COLOR = HUD_TEXT_DIM_COLOR;
/// The quietest text that is still meant to be read (the path of a card in the search
/// results, an icon of the rail at rest).
inline constexpr ImVec4 TEXT_FAINT_COLOR = colorFromBytes(104, 118, 150);

/// Background of a card: one step lighter than the night navy of the window.
inline constexpr ImVec4 CARD_COLOR = colorFromBytes(19, 27, 51);
/// Background of a widget on a card (number field, list, button): one more step lighter.
inline constexpr ImVec4 CONTROL_COLOR = colorFromBytes(26, 36, 65);
/// The thin outline of windows, cards and widgets: slate, mostly transparent.
inline constexpr ImVec4 LINE_COLOR = HUD_LINE_COLOR;

/// The action colour: the amber of the flashlight. A switch that is on, the filled part
/// of a slider, the chosen category, the chosen tab.
inline constexpr ImVec4 ACCENT_COLOR = HUD_BATTERY_COLOR;
/// A faint amber wash behind something chosen (the icon of the current category).
inline constexpr ImVec4 ACCENT_SOFT_COLOR = colorFromBytes(255, 184, 84, 0.14F);
/// Text and marks drawn on top of the action colour: dark, because pale text on bright
/// amber cannot be read.
inline constexpr ImVec4 ON_ACCENT_COLOR = colorFromBytes(14, 20, 38);
/// The second colour: the teal of the crystals. Values that are only shown, and the
/// frame of the keyboard focus.
inline constexpr ImVec4 SECONDARY_COLOR = HUD_CRYSTAL_COLOR;

/// The empty part of a slider and a switch that is off.
inline constexpr ImVec4 TRACK_COLOR = colorFromBytes(46, 61, 104);
/// The same under the mouse.
inline constexpr ImVec4 TRACK_HOVER_COLOR = colorFromBytes(66, 87, 143);

// ---- Colours with a meaning in the game ------------------------------------------------

/// Text of a failed load (the lists of shader programs and of assets in the Diagnostics
/// category): a soft red that stays readable on the dark background of a card.
inline constexpr ImVec4 ERROR_TEXT_COLOR = HUD_BATTERY_LOW_COLOR;

/// Walls on the plan of the maze (MazePlan.hpp): pale stone in moonlight.
inline constexpr ImVec4 PLAN_WALL_COLOR = colorFromBytes(176, 190, 216);

/// The player on the plan of the maze: the warm light of the flashlight.
inline constexpr ImVec4 PLAN_PLAYER_COLOR = HUD_BATTERY_COLOR;

/// Crystals on the plan of the maze: the cyan the crystals glow in, as in the HUD.
inline constexpr ImVec4 PLAN_CRYSTAL_COLOR = HUD_CRYSTAL_COLOR;

/// A crystal that is already collected, on the plan: only a dim trace of where it was.
inline constexpr ImVec4 PLAN_COLLECTED_COLOR = colorFromBytes(44, 104, 112);

/// The gate on the plan while it is closed: the brown of its wood, made light enough to
/// stand out from the walls. An open gate is drawn like a collected crystal.
inline constexpr ImVec4 PLAN_GATE_COLOR = colorFromBytes(214, 142, 82);

/// The exit zone on the plan: a soft green, the colour of "this way out".
inline constexpr ImVec4 PLAN_EXIT_COLOR = colorFromBytes(132, 220, 140);

/// A lever on the plan of the maze while it can still be pulled: the red of
/// a switch. A pulled lever is drawn like a collected crystal.
inline constexpr ImVec4 PLAN_LEVER_COLOR = colorFromBytes(238, 92, 62);

/// A note on the plan of the maze: the pale yellow of old paper, as the title of the
/// note card in the HUD.
inline constexpr ImVec4 PLAN_NOTE_COLOR = HUD_NOTE_COLOR;

/// Sets the colours and the metrics (padding, spacing, rounding, font size) of the debug
/// UI.
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
