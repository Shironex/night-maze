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
    /// True: the panel starts folded to its title bar and opens with a click on the arrow
    /// in that bar. For a panel the window has no free room for.
    bool collapsed = false;
    /// How many rows of folded title bars stand between the offset and this panel. The
    /// panel is moved away from its corner by that many bars, each with the gap after
    /// it. The height of a bar depends on the size of the font, so it is not part of
    /// the offset: it is asked from ImGui when the panel is placed.
    int foldedRowsBefore = 0;
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

// Left column: Renderer above Lights. Together they fill the height of the window. The
// Renderer panel is exactly as tall as its contents. The Lights panel gets what is left,
// which is a little less than its contents (with its Moon group folded), so it scrolls.
inline constexpr float LEFT_COLUMN_WIDTH = 336.0F;
inline constexpr float RENDERER_HEIGHT = 284.0F;
inline constexpr float LIGHTS_HEIGHT = REFERENCE_HEIGHT - RENDERER_HEIGHT - 3.0F * PANEL_GAP;

// Right column: Maze above Assets. Together they fill the height of the window. The
// lists of the Assets panel are long, so that panel always scrolls: it gets what is left.
inline constexpr float RIGHT_COLUMN_WIDTH = 300.0F;
inline constexpr float MAZE_HEIGHT = 480.0F;
inline constexpr float ASSETS_HEIGHT = REFERENCE_HEIGHT - MAZE_HEIGHT - 3.0F * PANEL_GAP;

// Bottom edge between the two columns: Collision and Shaders side by side, equally tall.
// Together they are as wide as the space between the columns. The scene stays visible
// above them, with the middle of the window, where the flashlight shines, well clear.
inline constexpr float BOTTOM_ROW_LEFT = LEFT_COLUMN_WIDTH + 2.0F * PANEL_GAP;
inline constexpr float BOTTOM_ROW_HEIGHT = 280.0F;
inline constexpr float COLLISION_WIDTH = 312.0F;
inline constexpr float SHADERS_LEFT = BOTTOM_ROW_LEFT + COLLISION_WIDTH + PANEL_GAP;
inline constexpr float SHADERS_WIDTH =
    REFERENCE_WIDTH - SHADERS_LEFT - RIGHT_COLUMN_WIDTH - 2.0F * PANEL_GAP;

// Camera and Gameplay: the seventh and the eighth panel. Two columns and a bottom row
// have room for six, so these two start folded to their title bars, side by side at the
// top edge between the columns. Unfolded each reaches down to the bottom row and covers
// its part of the scene and the folded bars under it (Terrain or Grass, and its part of
// the Framebuffers bar, see below), but no open panel. Camera is a little shorter than
// its contents, so it scrolls. Gameplay may start folded because the HUD shows the state of
// the round all the time: the panel is for changing the rules.
inline constexpr float CAMERA_WIDTH = 280.0F;
inline constexpr float CAMERA_HEIGHT = REFERENCE_HEIGHT - BOTTOM_ROW_HEIGHT - 3.0F * PANEL_GAP;
inline constexpr float GAMEPLAY_LEFT = BOTTOM_ROW_LEFT + CAMERA_WIDTH + PANEL_GAP;
inline constexpr float GAMEPLAY_WIDTH =
    REFERENCE_WIDTH - GAMEPLAY_LEFT - RIGHT_COLUMN_WIDTH - 2.0F * PANEL_GAP;

// Terrain and Grass: the ninth and the tenth panel. They start folded too, in a second
// row of title bars right under Camera and Gameplay, each as wide as the bar above it
// (PanelPlacement::foldedRowsBefore is 1). Both panels are short: unfolded they cover
// a strip of the scene below their bar and their part of the Framebuffers bar in the
// third row, but no open panel. An unfolded Camera or Gameplay panel does cover the
// bars under it.
inline constexpr float TERRAIN_HEIGHT = 170.0F;
inline constexpr float GRASS_HEIGHT = 190.0F;

// Framebuffers: the eleventh panel. It starts folded in a third row of title bars, one
// bar as wide as the two bars above it together (PanelPlacement::foldedRowsBefore is
// 2). It is that wide because it shows four pictures side by side, under widgets that
// stand in two columns. Unfolded it reaches down to just above the bottom row and
// covers the scene between the columns, but no other panel. Its contents need a little
// less than that height at that width.
inline constexpr float FRAMEBUFFERS_WIDTH = CAMERA_WIDTH + PANEL_GAP + GAMEPLAY_WIDTH;
inline constexpr float FRAMEBUFFERS_HEIGHT = 344.0F;

// The number of rows of folded title bars at the top edge. The HUD starts below them
// (Hud.cpp).
inline constexpr int FOLDED_ROW_COUNT = 3;

// The eleven panels. No two rectangles overlap in a window of the reference size, with
// the five folded panels counted as their title bars.
inline constexpr PanelPlacement RENDERER_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {PANEL_GAP, PANEL_GAP},
    .size = {LEFT_COLUMN_WIDTH, RENDERER_HEIGHT},
};
inline constexpr PanelPlacement LIGHTS_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {PANEL_GAP, RENDERER_HEIGHT + 2.0F * PANEL_GAP},
    .size = {LEFT_COLUMN_WIDTH, LIGHTS_HEIGHT},
};
inline constexpr PanelPlacement CAMERA_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {CAMERA_WIDTH, CAMERA_HEIGHT},
    .collapsed = true,
};
inline constexpr PanelPlacement GAMEPLAY_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {GAMEPLAY_LEFT, PANEL_GAP},
    .size = {GAMEPLAY_WIDTH, CAMERA_HEIGHT},
    .collapsed = true,
};
inline constexpr PanelPlacement TERRAIN_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {CAMERA_WIDTH, TERRAIN_HEIGHT},
    .collapsed = true,
    .foldedRowsBefore = 1,
};
inline constexpr PanelPlacement GRASS_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {GAMEPLAY_LEFT, PANEL_GAP},
    .size = {GAMEPLAY_WIDTH, GRASS_HEIGHT},
    .collapsed = true,
    .foldedRowsBefore = 1,
};
inline constexpr PanelPlacement FRAMEBUFFERS_PLACEMENT{
    .corner = TOP_LEFT,
    .offset = {BOTTOM_ROW_LEFT, PANEL_GAP},
    .size = {FRAMEBUFFERS_WIDTH, FRAMEBUFFERS_HEIGHT},
    .collapsed = true,
    .foldedRowsBefore = 2,
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
    .size = {COLLISION_WIDTH, BOTTOM_ROW_HEIGHT},
};
inline constexpr PanelPlacement SHADERS_PLACEMENT{
    .corner = BOTTOM_LEFT,
    .offset = {SHADERS_LEFT, PANEL_GAP},
    .size = {SHADERS_WIDTH, BOTTOM_ROW_HEIGHT},
};

/// Sets the position, the size and the folded state of the panel that the next
/// ImGui::Begin opens, but only when ImGui has no data for that panel in imgui.ini
/// (ImGuiCond_FirstUseEver).
///
/// The panel is placed at its corner of the window as it is at that moment, so in a
/// larger window the panels still stand at the edges. The numbers of the placement are
/// multiplied by the display scale, so that the bigger font fits, but never by more than
/// the window has room for compared with the reference size.
void placePanelOnFirstUse(const PanelPlacement& placement);

/// The height of count rows of folded title bars, each with the gap after it, in pixels
/// of the screen. gapScale is what PANEL_GAP is multiplied by. The height of a bar is the
/// real one: ImGui computes it from the font in use, so the rows fit at every display
/// scale. Call it inside an ImGui frame.
float foldedRowsHeight(int count, float gapScale);

} // namespace debug
