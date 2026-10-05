// First-use layout of the debug panels: where each panel appears when there is no imgui.ini.
// See docs/modules/debug-ui.md
#pragma once

// This header shows ImGui types (ImVec2), so it includes imgui.h. Only the .cpp files of
// src/debug include it, so the rest of the project still does not depend on ImGui.
#include <imgui.h>

namespace debug {

/// Where one panel appears and how big it is the first time the program runs.
///
/// A panel sticks to one corner of the window. All numbers are in pixels of a
/// REFERENCE_WIDTH x REFERENCE_HEIGHT window at 100 % display scaling.
struct PanelPlacement {
    /// The corner of the window the panel sticks to: one of the four constants below.
    ImVec2 corner;
    /// Distance from that corner of the window to the same corner of the panel.
    ImVec2 offset;
    /// Width and height of the panel.
    ImVec2 size;
};

// The corners of the window: x is 0 at the left edge and 1 at the right edge, y is 0 at
// the top edge and 1 at the bottom edge. All four are named, although the present layout
// puts no panel in the bottom right corner.
inline constexpr ImVec2 TOP_LEFT{0.0F, 0.0F};
inline constexpr ImVec2 TOP_RIGHT{1.0F, 0.0F};
inline constexpr ImVec2 BOTTOM_LEFT{0.0F, 1.0F};
inline constexpr ImVec2 BOTTOM_RIGHT{1.0F, 1.0F};

// The layout is drawn up for the window the game starts with (INITIAL_WIDTH and
// INITIAL_HEIGHT in game/NightMazeApp.cpp).
inline constexpr float REFERENCE_WIDTH = 1280.0F;
inline constexpr float REFERENCE_HEIGHT = 720.0F;

// Free space between a panel and the edge of the window, and between two panels.
inline constexpr float PANEL_GAP = 8.0F;

// Left column: Renderer above Camera. Together they fill the height of the window. The
// Renderer panel has room for one more line, for a longer name of the graphics card.
inline constexpr float LEFT_COLUMN_WIDTH = 336.0F;
inline constexpr float RENDERER_HEIGHT = 240.0F;
inline constexpr float CAMERA_HEIGHT = REFERENCE_HEIGHT - RENDERER_HEIGHT - 3.0F * PANEL_GAP;

// Right column: Maze above Assets. Together they fill the height of the window. The
// lists of the Assets panel are long, so that panel always scrolls: it gets what is left.
inline constexpr float RIGHT_COLUMN_WIDTH = 300.0F;
inline constexpr float MAZE_HEIGHT = 480.0F;
inline constexpr float ASSETS_HEIGHT = REFERENCE_HEIGHT - MAZE_HEIGHT - 3.0F * PANEL_GAP;

// Bottom edge between the two columns: Collision and Shaders side by side, each as tall
// as its contents. Together they are as wide as the space between the columns. The scene
// stays visible above them.
inline constexpr float BOTTOM_ROW_LEFT = LEFT_COLUMN_WIDTH + 2.0F * PANEL_GAP;
inline constexpr float COLLISION_WIDTH = 312.0F;
inline constexpr float COLLISION_HEIGHT = 272.0F;
inline constexpr float SHADERS_LEFT = BOTTOM_ROW_LEFT + COLLISION_WIDTH + PANEL_GAP;
inline constexpr float SHADERS_WIDTH =
    REFERENCE_WIDTH - SHADERS_LEFT - RIGHT_COLUMN_WIDTH - 2.0F * PANEL_GAP;
inline constexpr float SHADERS_HEIGHT = 336.0F;

// The six panels. No two rectangles overlap in a window of the reference size.
inline constexpr PanelPlacement RENDERER_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {PANEL_GAP, PANEL_GAP},
    .size = {LEFT_COLUMN_WIDTH, RENDERER_HEIGHT},
};
inline constexpr PanelPlacement CAMERA_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {PANEL_GAP, RENDERER_HEIGHT + 2.0F * PANEL_GAP},
    .size = {LEFT_COLUMN_WIDTH, CAMERA_HEIGHT},
};
inline constexpr PanelPlacement MAZE_PLACEMENT{
    .corner = TOP_RIGHT,
    .offset = {PANEL_GAP, PANEL_GAP},
    .size = {RIGHT_COLUMN_WIDTH, MAZE_HEIGHT},
};
inline constexpr PanelPlacement ASSETS_PLACEMENT{
    .corner = TOP_RIGHT,
    .offset = {PANEL_GAP, MAZE_HEIGHT + 2.0F * PANEL_GAP},
    .size = {RIGHT_COLUMN_WIDTH, ASSETS_HEIGHT},
};
inline constexpr PanelPlacement COLLISION_PLACEMENT{
    .corner = BOTTOM_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {COLLISION_WIDTH, COLLISION_HEIGHT},
};
inline constexpr PanelPlacement SHADERS_PLACEMENT{
    .corner = BOTTOM_LEFT,
    .offset = {SHADERS_LEFT, PANEL_GAP},
    .size = {SHADERS_WIDTH, SHADERS_HEIGHT},
};

/// Sets the position and the size of the panel that the next ImGui::Begin opens, but only
/// when ImGui has no data for that panel in imgui.ini (ImGuiCond_FirstUseEver).
///
/// The panel is placed at its corner of the window as it is at that moment, so in a
/// larger window the panels still stand at the edges. The numbers of the placement are
/// multiplied by the display scale, so that the bigger font fits, but never by more than
/// the window has room for compared with the reference size.
void placePanelOnFirstUse(const PanelPlacement& placement);

} // namespace debug
