// Theme of the debug window: night colours, calm metrics and the panel font.
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

// The colours below have a meaning of their own, so the code of the debug window names
// them directly (ImGui::PushStyleColor, draw lists). All other colours of the theme are
// in Theme.cpp.

// ---- Tokens of the debug window --------------------------------------------------------
// The widgets drawn by hand (Widgets.cpp, Icons.cpp, DebugWindow.cpp) take every colour
// from this list, so the window has one look: the amber of the flashlight for what acts
// or is switched on, the teal of the crystals for what is only shown, on night navy.

/// Height of the text of the debug window in pixels at 100 % display scaling. One size
/// for everything. The built-in font of ImGui is 13 pixels high.
inline constexpr float FONT_SIZE = 14.0F;

/// Text: the pale, slightly blue white of moonlight.
inline constexpr ImVec4 TEXT_COLOR = colorFromBytes(226, 234, 246);
/// Quieter text (labels of read only lines, hints, units): moonlight behind a cloud.
inline constexpr ImVec4 TEXT_DIM_COLOR = colorFromBytes(140, 154, 182);
/// The quietest text that is still meant to be read (the path of a card in the search
/// results, an icon of the rail at rest).
inline constexpr ImVec4 TEXT_FAINT_COLOR = colorFromBytes(104, 118, 150);

/// Background of a card: one step lighter than the night navy of the window.
inline constexpr ImVec4 CARD_COLOR = colorFromBytes(19, 27, 51);
/// Background of a widget on a card (number field, list, button): one more step lighter.
inline constexpr ImVec4 CONTROL_COLOR = colorFromBytes(26, 36, 65);
/// The thin outline of windows, cards and widgets: slate, mostly transparent.
inline constexpr ImVec4 LINE_COLOR = colorFromBytes(92, 112, 156, 0.3F);

/// The action colour: the amber of the flashlight. A switch that is on, the filled part
/// of a slider, the chosen category, the chosen tab.
inline constexpr ImVec4 ACCENT_COLOR = colorFromBytes(255, 184, 84);
/// A faint amber wash behind something chosen (the icon of the current category).
inline constexpr ImVec4 ACCENT_SOFT_COLOR = colorFromBytes(255, 184, 84, 0.14F);
/// Text and marks drawn on top of the action colour: dark, because pale text on bright
/// amber cannot be read.
inline constexpr ImVec4 ON_ACCENT_COLOR = colorFromBytes(14, 20, 38);
/// The second colour: the teal of the crystals. Values that are only shown, and the
/// frame of the keyboard focus.
inline constexpr ImVec4 SECONDARY_COLOR = colorFromBytes(86, 214, 202);

/// The empty part of a slider and a switch that is off.
inline constexpr ImVec4 TRACK_COLOR = colorFromBytes(46, 61, 104);
/// The same under the mouse.
inline constexpr ImVec4 TRACK_HOVER_COLOR = colorFromBytes(66, 87, 143);

// ---- Colours with a meaning in the game ------------------------------------------------

/// Text of a failed load (the lists of shader programs and of assets in the Diagnostics
/// category): a soft red that stays readable on the dark background of a card.
inline constexpr ImVec4 ERROR_TEXT_COLOR = colorFromBytes(255, 150, 138);

/// Walls on the plan of the maze (MazePlan.hpp): pale stone in moonlight.
inline constexpr ImVec4 PLAN_WALL_COLOR = colorFromBytes(176, 190, 216);

/// The player on the plan of the maze: the warm light of the flashlight.
inline constexpr ImVec4 PLAN_PLAYER_COLOR = colorFromBytes(255, 184, 84);

/// Crystals on the plan of the maze, and everything about crystals in the HUD: the
/// cyan the crystals glow in.
inline constexpr ImVec4 PLAN_CRYSTAL_COLOR = colorFromBytes(86, 214, 202);
inline constexpr ImVec4 HUD_CRYSTAL_COLOR = PLAN_CRYSTAL_COLOR;

/// The inside of the little crystal in front of the crystal counter of the HUD: a deep
/// teal. Its outline is the colour of the crystals.
inline constexpr ImVec4 HUD_GEM_COLOR = colorFromBytes(18, 68, 76);

/// The dark things of the HUD: the copy under every text that keeps it readable over
/// a bright wall, the disc behind the ring of the lamp and the pill of the prompt.
/// Nearly black, with a little of the blue of the night. Each use sets its own opacity.
inline constexpr ImVec4 HUD_SHADOW_COLOR = colorFromBytes(3, 5, 11);

/// A crystal that is already collected, on the plan: only a dim trace of where it was.
inline constexpr ImVec4 PLAN_COLLECTED_COLOR = colorFromBytes(44, 104, 112);

/// The gate on the plan while it is closed: the brown of its wood, made light enough to
/// stand out from the walls. An open gate is drawn like a collected crystal.
inline constexpr ImVec4 PLAN_GATE_COLOR = colorFromBytes(214, 142, 82);

/// The exit zone on the plan: a soft green, the colour of "this way out".
inline constexpr ImVec4 PLAN_EXIT_COLOR = colorFromBytes(132, 220, 140);

/// The lamp gauge of the HUD: the warm light of the flashlight while there is charge,
/// the soft red of an error once it is low.
inline constexpr ImVec4 HUD_BATTERY_COLOR = PLAN_PLAYER_COLOR;
inline constexpr ImVec4 HUD_BATTERY_LOW_COLOR = ERROR_TEXT_COLOR;

/// The stamina line of the HUD: the teal of the crystals, so one look tells it from the
/// amber lamp gauge. While the player is winded it is drawn in a dimmer teal (and
/// pulses, see Hud.cpp): the stamina is there, but it cannot be used.
inline constexpr ImVec4 HUD_STAMINA_COLOR = SECONDARY_COLOR;
inline constexpr ImVec4 HUD_STAMINA_WINDED_COLOR = colorFromBytes(58, 138, 140);

/// The stamina line while the tea of a flask works, and the sentence that says so: the
/// copper of strong tea. Warm, so one look tells it from the teal line of the stamina
/// it replaces, and darker and redder than the amber of the lamp gauge.
inline constexpr ImVec4 HUD_FLASK_COLOR = colorFromBytes(226, 124, 58);

/// A lever on the plan of the maze while it can still be pulled: the red of
/// a switch. A pulled lever is drawn like a collected crystal.
inline constexpr ImVec4 PLAN_LEVER_COLOR = colorFromBytes(238, 92, 62);

/// A note on the plan of the maze, and the title of the note card in the HUD: the
/// pale yellow of old paper.
inline constexpr ImVec4 PLAN_NOTE_COLOR = colorFromBytes(230, 220, 178);
inline constexpr ImVec4 HUD_NOTE_COLOR = PLAN_NOTE_COLOR;

/// The crosshair in the middle of the screen: a quiet grey white while it points at
/// nothing, and the warm colour of the flashlight when the player can use what it
/// points at. The same warm colour writes the prompt (the line that names the key).
inline constexpr ImVec4 HUD_CROSSHAIR_COLOR = colorFromBytes(214, 220, 232);
inline constexpr ImVec4 HUD_CROSSHAIR_ACTIVE_COLOR = PLAN_PLAYER_COLOR;

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
