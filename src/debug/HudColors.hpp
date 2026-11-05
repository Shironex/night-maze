// Colours of the HUD: the night palette it shares with the debug window, and the colours
// that mean something in a round.
#pragma once

// This header shows ImGui types (ImVec4), so it includes imgui.h. Only the files that draw
// with ImGui include it, so the rest of the project still does not depend on ImGui.
#include <imgui.h>

namespace debug {

/// Makes an ImGui colour from red, green and blue written as whole numbers from 0 to 255,
/// the way colours are usually written down. ImGui wants floats from 0 to 1.
/// alpha is the opacity: 1 hides what is behind, 0 is invisible.
constexpr ImVec4 colorFromBytes(int red, int green, int blue, float alpha = 1.0F) {
    constexpr float BYTE_MAX = 255.0F;
    return {static_cast<float>(red) / BYTE_MAX, static_cast<float>(green) / BYTE_MAX,
            static_cast<float>(blue) / BYTE_MAX, alpha};
}

// The HUD and the debug window share these colours, so a round and the window over it
// look like one game. The debug window names them through its own tokens (Theme.hpp).

/// Text: the pale, slightly blue white of moonlight.
inline constexpr ImVec4 HUD_TEXT_COLOR = colorFromBytes(226, 234, 246);
/// Quieter text (units, labels, hints): moonlight behind a cloud.
inline constexpr ImVec4 HUD_TEXT_DIM_COLOR = colorFromBytes(140, 154, 182);
/// The thin outline of a pill or a card: slate, mostly transparent.
inline constexpr ImVec4 HUD_LINE_COLOR = colorFromBytes(92, 112, 156, 0.3F);

/// Everything about crystals in the HUD: the cyan the crystals glow in.
inline constexpr ImVec4 HUD_CRYSTAL_COLOR = colorFromBytes(86, 214, 202);

/// The inside of the little crystal in front of the crystal counter of the HUD: a deep
/// teal. Its outline is the colour of the crystals.
inline constexpr ImVec4 HUD_GEM_COLOR = colorFromBytes(18, 68, 76);

/// The dark things of the HUD: the copy under every text that keeps it readable over
/// a bright wall, the disc behind the ring of the lamp and the pill of the prompt.
/// Nearly black, with a little of the blue of the night. Each use sets its own opacity.
inline constexpr ImVec4 HUD_SHADOW_COLOR = colorFromBytes(3, 5, 11);

/// The lamp gauge of the HUD: the warm light of the flashlight while there is charge,
/// a soft red once it is low (the red the debug window writes a failed load in).
inline constexpr ImVec4 HUD_BATTERY_COLOR = colorFromBytes(255, 184, 84);
inline constexpr ImVec4 HUD_BATTERY_LOW_COLOR = colorFromBytes(255, 150, 138);

/// The stamina line of the HUD: the teal of the crystals, so one look tells it from the
/// amber lamp gauge. While the player is winded it is drawn in a dimmer teal (and
/// pulses, see Hud.cpp): the stamina is there, but it cannot be used.
inline constexpr ImVec4 HUD_STAMINA_COLOR = HUD_CRYSTAL_COLOR;
inline constexpr ImVec4 HUD_STAMINA_WINDED_COLOR = colorFromBytes(58, 138, 140);

/// The stamina line while the tea of a flask works, and the sentence that says so: the
/// copper of strong tea. Warm, so one look tells it from the teal line of the stamina
/// it replaces, and darker and redder than the amber of the lamp gauge.
inline constexpr ImVec4 HUD_FLASK_COLOR = colorFromBytes(226, 124, 58);

/// The title of the note card in the HUD: the pale yellow of old paper.
inline constexpr ImVec4 HUD_NOTE_COLOR = colorFromBytes(230, 220, 178);

/// The crosshair in the middle of the screen: a quiet grey white while it points at
/// nothing, and the warm colour of the flashlight when the player can use what it
/// points at. The same warm colour writes the prompt (the line that names the key).
inline constexpr ImVec4 HUD_CROSSHAIR_COLOR = colorFromBytes(214, 220, 232);
inline constexpr ImVec4 HUD_CROSSHAIR_ACTIVE_COLOR = HUD_BATTERY_COLOR;

} // namespace debug
