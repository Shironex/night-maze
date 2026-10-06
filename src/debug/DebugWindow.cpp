// Debug window: the one window that holds every debug control, in seven categories.
// See docs/modules/debug-ui.md
#include "debug/DebugWindow.hpp"

#include "core/Time.hpp"
#include "debug/DebugContext.hpp"
#include "debug/Hud.hpp"
#include "debug/Theme.hpp"
#include "debug/Widgets.hpp"
#include "debug/categories/GameplayCategory.hpp"
#include "debug/categories/LightCategory.hpp"
#include "debug/categories/PlayerCategory.hpp"
#include "debug/categories/PostProcessCategory.hpp"
#include "debug/categories/RenderCategory.hpp"
#include "debug/categories/WorldCategory.hpp"
#include "game/GameState.hpp"
#include "game/MazeWorld.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstddef>
#include <span>

namespace debug {

namespace {

// ---- Sizes, in pixels at 100 % display scaling -----------------------------------------

// Free space between the debug window and the edges of the game window, and between
// the things the debug UI puts on the screen.
constexpr float WINDOW_MARGIN = 12.0F;

// The width the window would like to have: room for two columns of cards. It never
// takes more than this part of the width of the game window, so the left half of the
// picture always stays free.
constexpr float WINDOW_WIDTH = 620.0F;
constexpr float MAX_WINDOW_WIDTH_SHARE = 0.5F;

// The window is never made lower than this, however small the game window is: it then
// reaches below the bottom edge instead of collapsing into a line.
constexpr float MIN_WINDOW_HEIGHT = 160.0F;

// The icon rail: its width, the side of a category button, the gap between two buttons
// and the size of the mark of the game above them.
constexpr float RAIL_WIDTH = 56.0F;
constexpr float RAIL_BUTTON_SIZE = 36.0F;
constexpr float RAIL_BUTTON_GAP = 4.0F;
constexpr float RAIL_PADDING = 14.0F;
constexpr float LOGO_SIZE = 26.0F;
constexpr float LOGO_GAP = 10.0F;

// The name of the game written down the rail, one letter per line: the size of its
// letters, the height of a line and the height of the gap between its two words.
constexpr float WORDMARK_FONT_SIZE = 10.0F;
constexpr float WORDMARK_LINE_HEIGHT = 12.0F;
constexpr float WORDMARK_WORD_GAP = 10.0F;

// Height of the text of the title in the header: a little larger than FONT_SIZE.
constexpr float TITLE_FONT_SIZE = 16.0F;

// Free space between the edge of the right part of the window and its contents.
constexpr ImVec2 MAIN_PADDING{14.0F, 12.0F};

// Two columns of cards are used when each of them can be at least this wide. Below
// that the labels of the rows would be cut off, so the cards stand in one column.
constexpr float MIN_COLUMN_WIDTH = 250.0F;

// The status strip: its padding, the gap around a dot between two entries and the
// radius of such a dot.
constexpr ImVec2 STATUS_PADDING{14.0F, 7.0F};
constexpr float STATUS_DOT_GAP = 6.0F;
constexpr float STATUS_DOT_RADIUS = 1.5F;

// ---- Window flags ----------------------------------------------------------------------

// The debug window is a fixed part of the screen:
//   NoTitleBar, NoCollapse   it has a header of its own
//   NoMove, NoResize         its place and size are computed in every frame
//   NoDocking                it is not one of the panels that can be docked
//   NoSavedSettings          so nothing about it has to be written to imgui.ini
//   NoScrollbar, NoScrollWithMouse   only the part with the cards scrolls
constexpr ImGuiWindowFlags MAIN_WINDOW_FLAGS =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

// The status strip is only a picture, like the HUD: as large as its contents, no
// input, never focused, never docked (see the flags of the HUD in Hud.cpp).
constexpr ImGuiWindowFlags STATUS_WINDOW_FLAGS =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoBringToFrontOnFocus;

// The corner of the game window the debug UI hangs from: x is 0 at the left edge and
// 1 at the right edge, y is 0 at the top edge.
constexpr ImVec2 TOP_RIGHT{1.0F, 0.0F};

// The screen the game is on, in words.
const char* screenName(game::GameMode mode) {
    switch (mode) {
    case game::GameMode::MainMenu:
        return "Main menu";
    case game::GameMode::Playing:
        return "Playing";
    case game::GameMode::Paused:
        return "Paused";
    case game::GameMode::RoundEnd:
        return "Round end";
    case game::GameMode::Quitting:
        break;
    }
    return "Quitting";
}

// A small dot on the line of the status strip, with a gap on both sides: it separates
// two entries.
void drawStatusDot(float scale) {
    const float gap = STATUS_DOT_GAP * scale;
    ImGui::SameLine(0.0F, gap);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float radius = STATUS_DOT_RADIUS * scale;
    // The dot stands at half the height of a line of text. Dummy reserves its room.
    ImGui::GetWindowDrawList()->AddCircleFilled(
        {position.x + radius, position.y + ImGui::GetTextLineHeight() / 2.0F}, radius,
        ImGui::GetColorU32(TEXT_FAINT_COLOR));
    ImGui::Dummy({2.0F * radius, ImGui::GetTextLineHeight()});
    ImGui::SameLine(0.0F, gap);
}

// The title in the header of the window: the one text that is larger than the rest.
void drawTitle(const char* title) {
    // nullptr keeps the font. The size is given without the display scale: ImGui
    // multiplies it by FontScaleDpi itself.
    ImGui::PushFont(nullptr, TITLE_FONT_SIZE);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
}

// The name of the game down the rail, one letter below the other, ending at bottom.
// It is decoration: when the rail is too short for it, it is left out.
void drawWordmark(float centerX, float top, float bottom, float scale) {
    constexpr std::array<const char*, 2> WORDS = {"NIGHT", "MAZE"};
    constexpr std::size_t LETTER_COUNT = 9;

    const float lineHeight = WORDMARK_LINE_HEIGHT * scale;
    const float wordGap = WORDMARK_WORD_GAP * scale;
    const float height = static_cast<float>(LETTER_COUNT) * lineHeight + wordGap;
    if (bottom - height < top) {
        return;
    }

    // The same font as everything else, drawn smaller. GetFont is the font in use.
    ImFont* font = ImGui::GetFont();
    const float fontSize = WORDMARK_FONT_SIZE * scale;
    const ImU32 color = ImGui::GetColorU32(TEXT_FAINT_COLOR);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float y = bottom - height;
    for (const char* word : WORDS) {
        // A word is a list of characters that ends with a zero.
        for (const char* letter = word; *letter != '\0'; ++letter) {
            // AddText draws the characters from letter up to (not including) the
            // second pointer: exactly one letter. CalcTextSizeA measures the same.
            const float width = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0F, letter, letter + 1).x;
            drawList->AddText(font, fontSize, {centerX - width / 2.0F, y}, color, letter,
                              letter + 1);
            y += lineHeight;
        }
        y += wordGap;
    }
}

// The page of a category that has not moved into the window yet: its controls are
// still in the old panels.
void drawNotMovedYet(Page& page) {
    page.setPlace("");
    page.beginCard("Not here yet");
    page.note("The controls of this category are still in the old panels.");
    page.endCard();
}

} // namespace

void DebugWindow::draw(const DebugContext& context) {
    drawStatusStrip(context);

    const float scale = displayScale();
    const float margin = WINDOW_MARGIN * scale;
    // The main viewport is the window of the game. WorkPos is its top left corner and
    // WorkSize its size, in the same units as the mouse position.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float right = viewport->WorkPos.x + viewport->WorkSize.x - margin;
    const float bottom = viewport->WorkPos.y + viewport->WorkSize.y - margin;

    // In a narrow game window the debug window reaches the middle of the picture, where
    // the HUD of the round stands at the top edge. So it starts below the room the HUD
    // can take, always: a window that moved whenever the HUD grew by a line of hint
    // would be hard to work with.
    const float top = viewport->WorkPos.y + hudReservedHeight() + margin;
    const float width =
        std::min(WINDOW_WIDTH * scale, viewport->WorkSize.x * MAX_WINDOW_WIDTH_SHARE);
    const float height = std::max(bottom - top, MIN_WINDOW_HEIGHT * scale);

    // ImGuiCond_Always: the place and the size are set again in every frame, so the
    // window follows when the game window is resized.
    ImGui::SetNextWindowPos({right - width, top}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({width, height}, ImGuiCond_Always);
    // No padding: the rail reaches the edges of the window. The right part has its own.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0F, 0.0F});
    // Begin returns false when the window cannot be seen. End must be called in both
    // cases. The name is never shown: ImGui tells windows apart by it.
    const bool open = ImGui::Begin("Debug window", nullptr, MAIN_WINDOW_FLAGS);
    ImGui::PopStyleVar();
    if (open) {
        drawRail();
        // The rail and the right part stand side by side without a gap.
        ImGui::SameLine(0.0F, 0.0F);

        // The right part is a child window with its own padding. Size 0 means "all the
        // room that is left". AlwaysUseWindowPadding: a child window without an outline
        // would otherwise get no padding.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            {MAIN_PADDING.x * scale, MAIN_PADDING.y * scale});
        const bool mainOpen =
            ImGui::BeginChild("main", {0.0F, 0.0F}, ImGuiChildFlags_AlwaysUseWindowPadding,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        if (mainOpen) {
            drawHeader();
            drawTabs();
            drawBody(context);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void DebugWindow::drawStatusStrip(const DebugContext& context) const {
    const float scale = displayScale();
    const float margin = WINDOW_MARGIN * scale;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // The third argument is the pivot: the point of the strip that is put at the given
    // position. (1, 0) is its top right corner, so the strip grows to the left.
    ImGui::SetNextWindowPos(
        {viewport->WorkPos.x + viewport->WorkSize.x - margin, viewport->WorkPos.y + margin},
        ImGuiCond_Always, TOP_RIGHT);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        {STATUS_PADDING.x * scale, STATUS_PADDING.y * scale});

    if (ImGui::Begin("Debug status", nullptr, STATUS_WINDOW_FLAGS)) {
        ImGui::TextUnformatted("NIGHT MAZE");
        drawStatusDot(scale);
        ImGui::TextDisabled("%s", screenName(context.gameMode));
        drawStatusDot(scale);
        // The numbers that change are in the second colour, like every value that is
        // only shown.
        ImGui::TextColored(SECONDARY_COLOR, "%.0f FPS", context.time.fps());
        drawStatusDot(scale);
        ImGui::TextColored(SECONDARY_COLOR, "%.1f ms", context.time.frameTimeMs());
        drawStatusDot(scale);
        ImGui::TextDisabled("seed %u", static_cast<unsigned int>(context.mazeWorld.seed));
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void DebugWindow::drawRail() {
    const float scale = displayScale();
    const float railWidth = RAIL_WIDTH * scale;
    const float buttonSize = RAIL_BUTTON_SIZE * scale;

    // A child window of a fixed width and the full height of the debug window.
    if (ImGui::BeginChild("rail", {railWidth, 0.0F}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        const ImVec2 railMin = ImGui::GetWindowPos();
        const ImVec2 railSize = ImGui::GetWindowSize();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // The thin line between the rail and the right part of the window.
        drawList->AddLine({railMin.x + railSize.x - 1.0F, railMin.y},
                          {railMin.x + railSize.x - 1.0F, railMin.y + railSize.y},
                          ImGui::GetColorU32(LINE_COLOR));

        // The mark of the game at the top, in the action colour.
        const float logoSize = LOGO_SIZE * scale;
        drawLogo(drawList,
                 {railMin.x + (railSize.x - logoSize) / 2.0F, railMin.y + RAIL_PADDING * scale},
                 logoSize, ImGui::GetColorU32(ACCENT_COLOR));

        // One button per category, below each other in the middle of the rail.
        // SetCursorPos takes a position counted from the top left corner of the rail.
        float y = (RAIL_PADDING + LOGO_SIZE + LOGO_GAP) * scale;
        for (int i = 0; i < CATEGORY_COUNT; ++i) {
            const auto category = static_cast<Category>(i);
            const CategoryInfo& info = categoryInfo(category);
            ImGui::SetCursorPos({(railSize.x - buttonSize) / 2.0F, y});
            if (iconButton(info.name, info.icon, category == m_category, buttonSize)) {
                m_category = category;
            }
            tooltipCard(info.name, info.description);
            y += buttonSize + RAIL_BUTTON_GAP * scale;
        }

        drawWordmark(railMin.x + railSize.x / 2.0F, railMin.y + y,
                     railMin.y + railSize.y - RAIL_PADDING * scale, scale);
    }
    ImGui::EndChild();
}

void DebugWindow::drawHeader() const {
    const CategoryInfo& info = categoryInfo(m_category);
    // AlignTextToFramePadding moves the text down to where the text of a button on
    // the same line stands, so the header keeps its height when buttons join it.
    ImGui::AlignTextToFramePadding();
    drawTitle(info.name);
    ImGui::SameLine();
    ImGui::TextDisabled("%d controls", info.controlCount);
}

void DebugWindow::drawTabs() {
    const CategoryInfo& info = categoryInfo(m_category);
    if (info.tabCount == 0) {
        return;
    }
    // A span is a view of the first tabCount names of the array: no copy is made.
    const std::span<const char* const> names(info.tabs.data(),
                                             static_cast<std::size_t>(info.tabCount));
    segmentedButtons("tabs", names, &tabOf(m_category));
}

void DebugWindow::drawBody(const DebugContext& context) {
    // The cards stand in a child window that takes the rest of the height and scrolls
    // when they are taller than it. NavFlattened: the keyboard moves between the header
    // and the cards as if they were in one window.
    if (ImGui::BeginChild("body", {0.0F, 0.0F}, ImGuiChildFlags_NavFlattened)) {
        const float scale = displayScale();
        // Room for two columns? Each needs MIN_COLUMN_WIDTH, and the table that holds
        // them puts its cell padding on both sides of each column.
        const float columnWidth =
            ImGui::GetContentRegionAvail().x / 2.0F - 2.0F * ImGui::GetStyle().CellPadding.x;
        const bool wide = columnWidth >= MIN_COLUMN_WIDTH * scale;

        Page page("", wide);
        drawCategory(m_category, page, context);
    }
    ImGui::EndChild();
}

void DebugWindow::drawCategory(Category category, Page& page, const DebugContext& context) const {
    switch (category) {
    case Category::Render:
        drawRenderCategory(page, context);
        break;
    case Category::Light:
        drawLightCategory(page, context, static_cast<LightTab>(tabOf(category)));
        break;
    case Category::PostProcess:
        drawPostProcessCategory(page, context);
        break;
    case Category::World:
        drawWorldCategory(page, context, static_cast<WorldTab>(tabOf(category)));
        break;
    case Category::Player:
        drawPlayerCategory(page, context);
        break;
    case Category::Gameplay:
        drawGameplayCategory(page, context);
        break;
    case Category::Diagnostics:
        drawNotMovedYet(page);
        break;
    }
}

} // namespace debug
