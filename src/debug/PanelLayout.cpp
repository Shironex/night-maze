// First-use layout of the debug panels: where each panel appears when there is no imgui.ini.
// See docs/modules/debug-ui.md
#include "debug/PanelLayout.hpp"

#include <algorithm>

namespace debug {

void placePanelOnFirstUse(const PanelPlacement& placement) {
    // The main viewport is the window of the program. WorkPos is its top left corner and
    // WorkSize its size, in the same units as the mouse position.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // How much bigger than the reference layout the panels are drawn. The display scale
    // (stored in the style by applyTheme) makes room for the bigger font. The two ratios
    // stop the layout from growing past the window: a 1280 x 720 window at 150 % scaling
    // keeps the layout of 100 %, and the panels scroll instead of covering each other.
    const float displayScale = ImGui::GetStyle().FontScaleDpi;
    const float layoutScale = std::min({displayScale, viewport->WorkSize.x / REFERENCE_WIDTH,
                                        viewport->WorkSize.y / REFERENCE_HEIGHT});

    // The corner of the window the panel sticks to, in screen coordinates.
    const ImVec2 windowCorner{viewport->WorkPos.x + placement.corner.x * viewport->WorkSize.x,
                              viewport->WorkPos.y + placement.corner.y * viewport->WorkSize.y};

    // The offset points into the window: to the right and down from a left or top edge
    // (corner 0, direction +1), to the left and up from a right or bottom edge (corner 1,
    // direction -1).
    const ImVec2 inwards{1.0F - 2.0F * placement.corner.x, 1.0F - 2.0F * placement.corner.y};
    const ImVec2 panelCorner{windowCorner.x + inwards.x * placement.offset.x * layoutScale,
                             windowCorner.y + inwards.y * placement.offset.y * layoutScale};

    // The third argument is the pivot: the point of the panel that is put at the given
    // position, with the same 0 to 1 meaning as the corner. With the pivot equal to the
    // corner, a panel at the right edge is placed by its right corner, so its width does
    // not have to be subtracted here.
    ImGui::SetNextWindowPos(panelCorner, ImGuiCond_FirstUseEver, placement.corner);
    ImGui::SetNextWindowSize({placement.size.x * layoutScale, placement.size.y * layoutScale},
                             ImGuiCond_FirstUseEver);
}

} // namespace debug
