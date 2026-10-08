// Theme of the debug window: night colours, calm metrics and the panel font.
#include "debug/Theme.hpp"

#include "core/Files.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <limits>

namespace debug {

namespace {

// ---- Palette: a stone maze at night ---------------------------------------------------
// Red, green and blue from 0 to 255. The colours the widgets of the debug window name
// directly (text, card, accent, track) are in Theme.hpp. Here are the ones only the
// style table below uses. Text is always TEXT_COLOR, so every background that text is
// drawn on is dark. The one exception is a filled accent button, which gets dark text
// (ON_ACCENT_COLOR).

// How much of a panel hides the scene behind it. A little of the scene shows through.
constexpr float PANEL_OPACITY = 0.94F;
// Tooltips and opened lists lie on top of panels and must not mix with their text.
constexpr float POPUP_OPACITY = 0.97F;

// Background of a panel: deep night navy.
constexpr ImVec4 NIGHT = colorFromBytes(14, 20, 38, PANEL_OPACITY);
// Background of tooltips and of opened lists: a darker night.
constexpr ImVec4 NIGHT_DEEP = colorFromBytes(9, 13, 26, POPUP_OPACITY);
// Title bar of a panel that is not focused, and the tab of a hidden docked panel.
constexpr ImVec4 NIGHT_RAISED = colorFromBytes(22, 31, 56);

// Tabs of docked panels at rest: muted slate.
constexpr ImVec4 SLATE = colorFromBytes(36, 47, 74);
// Resting scrollbar grab and resize grip: lighter slate.
constexpr ImVec4 SLATE_LIGHT = colorFromBytes(48, 62, 94);
// Resting handle of a slider, hovered scrollbar grab: the brightest slate.
constexpr ImVec4 SLATE_BRIGHT = colorFromBytes(92, 112, 156);

// A button under the mouse: the dark, warm edge of the flashlight patch. It is dark,
// not bright, because the pale text is drawn on top of it.
constexpr ImVec4 EMBER = colorFromBytes(78, 54, 22);
// A button that is being pressed: a slightly brighter ember.
constexpr ImVec4 EMBER_BRIGHT = colorFromBytes(98, 66, 20);
// Small bright marks without text on them (check mark, line over the selected tab,
// dragged scrollbar): the amber of the flashlight, the action colour.
constexpr ImVec4 AMBER = ACCENT_COLOR;
// The handle of a slider while it is dragged: a darker amber, because the digits of the
// value are drawn across the handle and must stay readable.
constexpr ImVec4 AMBER_DEEP = colorFromBytes(172, 110, 28);

// Sparing second accent (keyboard focus frame, plots, links, dragged splitter): crystal
// teal.
constexpr ImVec4 CRYSTAL = SECONDARY_COLOR;
// Title bar of the focused panel, selected tab, ticked checkbox: deep crystal teal.
constexpr ImVec4 CRYSTAL_DEEP = colorFromBytes(18, 68, 76);
// Separator lines: dim crystal teal.
constexpr ImVec4 CRYSTAL_DIM = colorFromBytes(44, 104, 112);

// Overlays that tint a whole area: crystal over a place where a panel can be docked,
// crystal over selected text, night over everything behind a modal window.
constexpr ImVec4 CRYSTAL_OVERLAY = colorFromBytes(86, 214, 202, 0.35F);
constexpr ImVec4 NIGHT_OVERLAY = colorFromBytes(9, 13, 26, 0.55F);
// A barely visible lighter stripe for every second row of a table.
constexpr ImVec4 MOON_STRIPE = colorFromBytes(226, 234, 246, 0.04F);
// Nothing at all: for parts that should not be drawn.
constexpr ImVec4 INVISIBLE = colorFromBytes(0, 0, 0, 0.0F);

// ---- Metrics, in pixels at 100 % display scaling ---------------------------------------

// The height of the text is FONT_SIZE in Theme.hpp.

// Free space between the edge of a panel and its contents.
constexpr ImVec2 WINDOW_PADDING{12.0F, 12.0F};
// Free space between the edge of a widget (button, number field) and its text. With the
// font it makes a widget 22 pixels high.
constexpr ImVec2 FRAME_PADDING{8.0F, 4.0F};
// Distance between two widgets: to the next one in a line, and to the next line. With
// the height of a widget it makes a setting row 28 pixels high.
constexpr ImVec2 ITEM_SPACING{8.0F, 6.0F};
// Distance between the parts of one widget, for example the two fields of a range.
constexpr ImVec2 ITEM_INNER_SPACING{6.0F, 4.0F};

// Radius of the corners of the debug window and of a pinned panel.
constexpr float WINDOW_ROUNDING = 14.0F;
// Radius of the corners of a card (a child window inside the debug window).
constexpr float CARD_ROUNDING = 12.0F;
// Radius of the corners of tooltips and of opened lists.
constexpr float POPUP_ROUNDING = 9.0F;
// Radius of the corners of widgets, tabs and scrollbars.
constexpr float WIDGET_ROUNDING = 7.0F;
// Thickness of every outline: panels, cards, popups and widgets. One thin line.
constexpr float BORDER_SIZE = 1.0F;
// Width of a scrollbar: thin, it only hints at where the page is.
constexpr float SCROLLBAR_SIZE = 8.0F;
// Smallest width of the handle of a slider.
constexpr float GRAB_MIN_SIZE = 10.0F;

// Opacity of widgets inside ImGui::BeginDisabled: low enough to tell them apart at once.
constexpr float DISABLED_ALPHA = 0.45F;

// ---- Font ------------------------------------------------------------------------------

// The font file is core::TEXT_FONT_FILE: the menu of the game uses the same one.

// ImGui refuses (with an assertion) data of 100 bytes or less: no font file is that small.
constexpr std::size_t SMALLEST_FONT_FILE = 101;

// The first four bytes of every TrueType font file: the number 1.0 written as two
// 16 bit halves.
constexpr std::array<unsigned char, 4> TRUETYPE_SIGNATURE{0x00, 0x01, 0x00, 0x00};

// Size 0 tells ImGui to take the size of the text from the style (FontSizeBase).
constexpr float SIZE_FROM_STYLE = 0.0F;

// True when the bytes can be handed to ImGui: their number is above its lower limit and
// fits the int it takes the size in, and they start like a TrueType font. In a debug
// build ImGui stops the program with an assertion when it is given something else, for
// example an empty file or a text file saved under the name of the font.
bool looksLikeFont(const std::vector<unsigned char>& bytes) {
    if (bytes.size() < SMALLEST_FONT_FILE ||
        bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    // std::equal compares the four bytes of the signature with the first four of the file.
    return std::equal(TRUETYPE_SIGNATURE.begin(), TRUETYPE_SIGNATURE.end(), bytes.begin());
}

// Fills the colour table of the style. Every entry is set, so that no colour of the
// stock style is left over.
void applyColors(ImGuiStyle& style) {
    // Colors is an array with one ImVec4 for every value of the enum ImGuiCol_.
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = TEXT_COLOR;
    colors[ImGuiCol_TextDisabled] = TEXT_DIM_COLOR;
    colors[ImGuiCol_TextLink] = CRYSTAL;
    colors[ImGuiCol_TextSelectedBg] = CRYSTAL_OVERLAY;
    colors[ImGuiCol_InputTextCursor] = TEXT_COLOR;

    // Backgrounds and outlines. A child window has no background of its own: a card
    // pushes CARD_COLOR for itself (Widgets.cpp).
    colors[ImGuiCol_WindowBg] = NIGHT;
    colors[ImGuiCol_ChildBg] = INVISIBLE;
    colors[ImGuiCol_PopupBg] = NIGHT_DEEP;
    colors[ImGuiCol_MenuBarBg] = NIGHT_RAISED;
    colors[ImGuiCol_Border] = LINE_COLOR;
    colors[ImGuiCol_BorderShadow] = INVISIBLE;

    // Title bars: only the focused panel gets the crystal accent.
    colors[ImGuiCol_TitleBg] = NIGHT_RAISED;
    colors[ImGuiCol_TitleBgActive] = CRYSTAL_DEEP;
    colors[ImGuiCol_TitleBgCollapsed] = NIGHT_RAISED;

    // Widgets with a frame (number fields, lists, the search box): one step lighter than
    // the card they lie on at rest, the two colours of a slider track under the mouse
    // and while they are used. All three are dark, because the pale text is drawn on them.
    colors[ImGuiCol_FrameBg] = CONTROL_COLOR;
    colors[ImGuiCol_FrameBgHovered] = TRACK_COLOR;
    colors[ImGuiCol_FrameBgActive] = TRACK_HOVER_COLOR;
    colors[ImGuiCol_CheckboxSelectedBg] = CRYSTAL_DEEP;
    colors[ImGuiCol_CheckMark] = AMBER;
    colors[ImGuiCol_SliderGrab] = SLATE_BRIGHT;
    colors[ImGuiCol_SliderGrabActive] = AMBER_DEEP;

    // A button does something, so it answers in the warm colour: ember under the mouse.
    colors[ImGuiCol_Button] = CONTROL_COLOR;
    colors[ImGuiCol_ButtonHovered] = EMBER;
    colors[ImGuiCol_ButtonActive] = EMBER_BRIGHT;

    // Header colours are used by the entries of an opened list (Combo).
    colors[ImGuiCol_Header] = TRACK_COLOR;
    colors[ImGuiCol_HeaderHovered] = TRACK_HOVER_COLOR;
    colors[ImGuiCol_HeaderActive] = EMBER_BRIGHT;

    colors[ImGuiCol_ScrollbarBg] = INVISIBLE;
    colors[ImGuiCol_ScrollbarGrab] = SLATE_LIGHT;
    colors[ImGuiCol_ScrollbarGrabHovered] = SLATE_BRIGHT;
    colors[ImGuiCol_ScrollbarGrabActive] = AMBER;

    // Lines. A separator can be dragged where two docked panels meet.
    colors[ImGuiCol_Separator] = CRYSTAL_DIM;
    colors[ImGuiCol_SeparatorHovered] = CRYSTAL;
    colors[ImGuiCol_SeparatorActive] = AMBER;
    colors[ImGuiCol_TreeLines] = CRYSTAL_DIM;

    // The corner of a panel that resizes it.
    colors[ImGuiCol_ResizeGrip] = SLATE_LIGHT;
    colors[ImGuiCol_ResizeGripHovered] = CRYSTAL;
    colors[ImGuiCol_ResizeGripActive] = AMBER;

    // Tabs of docked panels. "Dimmed" is a tab bar whose panel is not focused.
    colors[ImGuiCol_Tab] = SLATE;
    colors[ImGuiCol_TabHovered] = EMBER;
    colors[ImGuiCol_TabSelected] = CRYSTAL_DEEP;
    colors[ImGuiCol_TabSelectedOverline] = AMBER;
    colors[ImGuiCol_TabDimmed] = NIGHT_RAISED;
    colors[ImGuiCol_TabDimmedSelected] = SLATE;
    colors[ImGuiCol_TabDimmedSelectedOverline] = CRYSTAL_DIM;

    // Docking: the place a dragged panel would take, and a dock area with nothing in it.
    // The middle of our dock area is not painted at all (PassthruCentralNode).
    colors[ImGuiCol_DockingPreview] = CRYSTAL_OVERLAY;
    colors[ImGuiCol_DockingEmptyBg] = NIGHT;

    colors[ImGuiCol_PlotLines] = CRYSTAL;
    colors[ImGuiCol_PlotLinesHovered] = AMBER;
    colors[ImGuiCol_PlotHistogram] = CRYSTAL;
    colors[ImGuiCol_PlotHistogramHovered] = AMBER;

    colors[ImGuiCol_TableHeaderBg] = NIGHT_RAISED;
    colors[ImGuiCol_TableBorderStrong] = SLATE_LIGHT;
    colors[ImGuiCol_TableBorderLight] = SLATE;
    colors[ImGuiCol_TableRowBg] = INVISIBLE;
    colors[ImGuiCol_TableRowBgAlt] = MOON_STRIPE;

    colors[ImGuiCol_DragDropTarget] = AMBER;
    colors[ImGuiCol_DragDropTargetBg] = INVISIBLE;
    colors[ImGuiCol_UnsavedMarker] = AMBER;

    // Keyboard navigation: the frame around the focused widget, the Ctrl+Tab window list.
    colors[ImGuiCol_NavCursor] = CRYSTAL;
    colors[ImGuiCol_NavWindowingHighlight] = CRYSTAL;
    colors[ImGuiCol_NavWindowingDimBg] = NIGHT_OVERLAY;
    colors[ImGuiCol_ModalWindowDimBg] = NIGHT_OVERLAY;
}

// Sets paddings, spacings, roundings and sizes: rounder and roomier than stock, with one
// thin outline around every panel, card, popup and widget.
void applyMetrics(ImGuiStyle& style) {
    style.WindowPadding = WINDOW_PADDING;
    style.FramePadding = FRAME_PADDING;
    style.ItemSpacing = ITEM_SPACING;
    style.ItemInnerSpacing = ITEM_INNER_SPACING;

    style.WindowRounding = WINDOW_ROUNDING;
    style.ChildRounding = CARD_ROUNDING;
    style.PopupRounding = POPUP_ROUNDING;
    style.FrameRounding = WIDGET_ROUNDING;
    style.GrabRounding = WIDGET_ROUNDING;
    style.TabRounding = WIDGET_ROUNDING;
    style.ScrollbarRounding = WIDGET_ROUNDING;

    style.WindowBorderSize = BORDER_SIZE;
    style.ChildBorderSize = BORDER_SIZE;
    style.PopupBorderSize = BORDER_SIZE;
    style.FrameBorderSize = BORDER_SIZE;
    style.ScrollbarSize = SCROLLBAR_SIZE;
    style.GrabMinSize = GRAB_MIN_SIZE;

    style.DisabledAlpha = DISABLED_ALPHA;
}

} // namespace

void applyTheme(float scale) {
    // A display that reports no usable scale is treated as 100 %.
    const float usedScale = scale > 0.0F ? scale : 1.0F;

    // The style holds everything about the look: a table of colours and the metrics.
    ImGuiStyle& style = ImGui::GetStyle();
    // Start from the stock dark style, so that an entry added by a later version of
    // ImGui gets a sensible dark colour and not black.
    ImGui::StyleColorsDark(&style);
    applyColors(style);
    applyMetrics(style);

    // ScaleAllSizes multiplies every metric set above by the scale. It does not touch the
    // font: FontScaleDpi does that. The final text height is FontSizeBase * FontScaleDpi.
    style.ScaleAllSizes(usedScale);
    style.FontSizeBase = FONT_SIZE;
    style.FontScaleDpi = usedScale;
}

void loadFont(std::vector<unsigned char>& fontBytes) {
    ImFontAtlas* fonts = ImGui::GetIO().Fonts;
    const std::filesystem::path path = core::assetPath(core::TEXT_FONT_FILE);

    if (core::readBinaryFile(path, fontBytes) && looksLikeFont(fontBytes)) {
        // By default ImGui takes the bytes over and frees them with its own allocator.
        // They belong to a std::vector, so ImGui must only use them, not free them.
        ImFontConfig config;
        config.FontDataOwnedByAtlas = false;
        // No glyph ranges: since ImGui 1.92 a letter is drawn into the font texture the
        // first time it is used, so the Polish letters need no extra setup.
        const ImFont* font = fonts->AddFontFromMemoryTTF(
            fontBytes.data(), static_cast<int>(fontBytes.size()), SIZE_FROM_STYLE, &config);
        if (font != nullptr) {
            return;
        }
    }

    core::logError("Panel font cannot be loaded, using the built-in font: " + core::pathText(path));
    // ImGui holds no pointer to the bytes after a failure, so they can be dropped.
    fontBytes.clear();
    // The built-in font that can be scaled, so the display scale still works.
    fonts->AddFontDefaultVector();
}

} // namespace debug
