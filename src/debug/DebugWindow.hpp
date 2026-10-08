// Debug window: the one window that holds every debug control, in seven categories.
#pragma once

#include "debug/Categories.hpp"
#include "debug/RawTextureSampler.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace debug {

struct DebugContext;
class Page;

/// The debug window: an icon rail with the seven categories on the left, and beside it
/// the header, the tabs and the cards of the category that is chosen. A small strip
/// above it shows the frame rate, the seed and the screen the game is on.
///
/// Two things make it a tool. The search box in the header shows the matching rows of
/// all categories at once. The pin button swaps the window for a small panel with one
/// category in it, which stays out of the way while playing: that panel is an ordinary
/// ImGui window, so it can be moved, resized and docked.
///
/// The window stands at the right edge of the game window, so the middle of the
/// picture stays free. It cannot be moved, resized or docked: its place and its size
/// are computed from the size of the game window in every frame, so it always fits.
/// (Only the pinned panel keeps a place of its own, in imgui.ini.)
///
/// The object keeps what the user chose (the category, the tab of every category) and
/// one OpenGL object, the sampler of its texture previews. Everything it shows comes
/// from the DebugContext of the frame.
class DebugWindow {
public:
    /// Builds the window and the status strip for this frame. Call it inside an ImGui
    /// frame, and only while the debug UI is shown.
    void draw(const DebugContext& context);

private:
    // Room for the text of the search box, the closing zero included.
    static constexpr std::size_t SEARCH_TEXT_SIZE = 64;

    // The strip at the top right corner: name of the game, screen, frame rate, seed.
    void drawStatusStrip(const DebugContext& context) const;
    // The window itself: the rail, the header, the tabs and the cards.
    void drawMainWindow(const DebugContext& context);
    // The small panel that takes the place of the window while a category is pinned.
    void drawPinnedPanel(const DebugContext& context);
    // The column of icons at the left edge of the window: one button per category.
    void drawRail();
    // The name of the chosen category and the number of its controls, the search box
    // and the pin button.
    void drawHeader();
    // The header of the pinned panel: the arrows to the neighbouring categories and the
    // button that brings the window back.
    void drawPinnedHeader();
    // The tabs of the chosen category, when it has any.
    void drawTabs();
    // The cards, in a part of the window that scrolls: the ones of the chosen category,
    // or, while search has a word in it, the matching rows of all categories. Returns
    // how many rows were shown.
    int drawCards(const DebugContext& context, std::string_view search) const;
    // Calls the function that draws one category onto the page.
    void drawCategory(Category category, Page& page, const DebugContext& context) const;

    // The tab that is chosen in the given category.
    int& tabOf(Category category) { return m_tabs[static_cast<std::size_t>(category)]; }
    int tabOf(Category category) const { return m_tabs[static_cast<std::size_t>(category)]; }

    // The sampler the Diagnostics category shows sRGB textures with. It owns an OpenGL
    // object, so this window has to be destroyed while the OpenGL context exists.
    RawTextureSampler m_rawTextureSampler;

    Category m_category = Category::Render;
    // The chosen tab of every category, so a category opens on the tab it was left on.
    std::array<int, CATEGORY_COUNT> m_tabs{};

    // The text of the search box: characters ended by a zero, as ImGui edits them in
    // place. Empty while nothing is searched for.
    std::array<char, SEARCH_TEXT_SIZE> m_search{};
    // How many rows the search showed in the frame before: the header says the number.
    int m_matchCount = 0;
    // True while one category is pinned as a small panel in place of the window.
    bool m_pinned = false;
};

} // namespace debug
