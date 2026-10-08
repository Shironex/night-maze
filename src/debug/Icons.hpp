// Icons of the debug window: a handful of small line drawings made with the draw list.
#pragma once

// This header shows ImGui types (ImDrawList, ImVec2), so it includes imgui.h. Only the
// .cpp files of src/debug include it, so the rest of the project still does not depend
// on ImGui.
#include <imgui.h>

namespace debug {

/// The pictures the debug window uses in place of words. They are drawn with lines and
/// circles (drawIcon), so the game needs no icon font and no picture files for them.
enum class Icon {
    Render,      ///< a screen on its stand
    Light,       ///< a flashlight
    PostProcess, ///< two sliders
    World,       ///< a small maze
    Player,      ///< a head and shoulders
    Gameplay,    ///< a crystal
    Diagnostics, ///< the line of a heartbeat
    Search,      ///< a magnifying glass
    Pin,         ///< a pin
    Expand,      ///< four corners pointing outwards
    Previous,    ///< an arrow head to the left
    Next,        ///< an arrow head to the right
};

/// Draws one icon into a square. topLeft is the top left corner of the square in screen
/// coordinates and size the length of its side in pixels. The lines get thicker with
/// the size, so an icon looks the same at every display scale.
void drawIcon(ImDrawList* drawList, Icon icon, const ImVec2& topLeft, float size, ImU32 color);

/// Draws the mark of the game, a crystal, into a square like drawIcon does.
void drawLogo(ImDrawList* drawList, const ImVec2& topLeft, float size, ImU32 color);

} // namespace debug
