// Debug window: the one window that holds every debug control, in seven categories.
// See docs/modules/debug-ui.md
#pragma once

#include "debug/Categories.hpp"

#include <array>
#include <cstddef>

namespace debug {

struct DebugContext;
class Page;

/// The debug window: an icon rail with the seven categories on the left, and beside it
/// the header, the tabs and the cards of the category that is chosen. A small strip
/// above it shows the frame rate, the seed and the screen the game is on.
///
/// The window stands at the right edge of the game window, so the middle of the
/// picture stays free. It cannot be moved, resized or docked: its place and its size
/// are computed from the size of the game window in every frame, so it always fits.
///
/// The object only keeps what the user chose (the category, the tab of every
/// category). Everything it shows comes from the DebugContext of the frame.
class DebugWindow {
public:
    /// Builds the window and the status strip for this frame. Call it inside an ImGui
    /// frame, and only while the debug UI is shown.
    void draw(const DebugContext& context);

private:
    // The strip at the top right corner: name of the game, screen, frame rate, seed.
    void drawStatusStrip(const DebugContext& context) const;
    // The column of icons at the left edge of the window: one button per category.
    void drawRail();
    // The name of the chosen category and the number of its controls.
    void drawHeader() const;
    // The tabs of the chosen category, when it has any.
    void drawTabs();
    // The cards of the chosen category, in a part of the window that scrolls.
    void drawBody(const DebugContext& context);
    // Calls the function that draws one category onto the page.
    void drawCategory(Category category, Page& page, const DebugContext& context) const;

    // The tab that is chosen in the given category.
    int& tabOf(Category category) { return m_tabs[static_cast<std::size_t>(category)]; }
    int tabOf(Category category) const { return m_tabs[static_cast<std::size_t>(category)]; }

    Category m_category = Category::Render;
    // The chosen tab of every category, so a category opens on the tab it was left on.
    std::array<int, CATEGORY_COUNT> m_tabs{};
};

} // namespace debug
