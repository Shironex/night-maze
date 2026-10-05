// Theme of the debug panels: night colours, soft metrics and the panel font.
// See docs/modules/debug-ui.md
#include "debug/Theme.hpp"

#include "core/Log.hpp"
#include "core/Paths.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>

namespace debug {

namespace {

// ---- Palette: a stone maze at night ---------------------------------------------------
// Red, green and blue from 0 to 255. Text is always MOONLIGHT, so every background that
// text is drawn on is dark enough for a contrast of at least 7 to 1 (table in the
// document). The one exception is the small handle of a slider, which the digits of the
// value cross.

// Text: the pale, slightly blue white of moonlight.
constexpr ImVec4 MOONLIGHT = colorFromBytes(226, 234, 246);
// Greyed out text (hints, disabled entries): moonlight behind a cloud.
constexpr ImVec4 MOON_DIM = colorFromBytes(140, 154, 182);

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

// Resting widgets (sliders, number fields, checkboxes, tabs): muted slate.
constexpr ImVec4 SLATE = colorFromBytes(36, 47, 74);
// Resting things that are clicked (buttons, list entries, scrollbar grab): lighter slate.
constexpr ImVec4 SLATE_LIGHT = colorFromBytes(48, 62, 94);
// Resting handle of a slider, hovered scrollbar grab: the brightest slate.
constexpr ImVec4 SLATE_BRIGHT = colorFromBytes(92, 112, 156);
// Soft outline of panels and popups: the brightest slate, mostly transparent.
constexpr ImVec4 SOFT_BORDER = colorFromBytes(92, 112, 156, 0.45F);

// A widget under the mouse: the dark, warm edge of the flashlight patch. It is dark,
// not bright, because the pale text is drawn on top of it.
constexpr ImVec4 EMBER = colorFromBytes(78, 54, 22);
// A widget that is being pressed or dragged: a slightly brighter ember.
constexpr ImVec4 EMBER_BRIGHT = colorFromBytes(98, 66, 20);
// Small bright marks without text on them (check mark, line over the selected tab,
// dragged scrollbar): the amber of the flashlight.
constexpr ImVec4 AMBER = colorFromBytes(255, 184, 84);
// The handle of a slider while it is dragged: a darker amber, because the digits of the
// value are drawn across the handle and must stay readable.
constexpr ImVec4 AMBER_DEEP = colorFromBytes(172, 110, 28);

// Sparing accent (keyboard focus frame, plots, links, dragged splitter): crystal cyan.
constexpr ImVec4 CRYSTAL = colorFromBytes(86, 214, 202);
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

// Height of the text. The built-in font of ImGui is 13 pixels high.
constexpr float FONT_SIZE = 16.0F;

// Free space between the edge of a panel and its contents.
constexpr ImVec2 WINDOW_PADDING{10.0F, 10.0F};
// Free space between the edge of a widget (button, slider) and its text.
constexpr ImVec2 FRAME_PADDING{8.0F, 3.0F};
// Distance between two widgets: to the next one in a line, and to the next line.
constexpr ImVec2 ITEM_SPACING{8.0F, 5.0F};
// Distance between the parts of one widget, for example a slider and its label.
constexpr ImVec2 ITEM_INNER_SPACING{6.0F, 4.0F};

// Radius of the corners of panels.
constexpr float WINDOW_ROUNDING = 8.0F;
// Radius of the corners of widgets, popups, tabs and scrollbars.
constexpr float WIDGET_ROUNDING = 5.0F;
// Thickness of the outline of panels and popups.
constexpr float BORDER_SIZE = 1.0F;
// Width of a scrollbar.
constexpr float SCROLLBAR_SIZE = 12.0F;
// Smallest width of the handle of a slider.
constexpr float GRAB_MIN_SIZE = 12.0F;

// Opacity of widgets inside ImGui::BeginDisabled: low enough to tell them apart at once.
constexpr float DISABLED_ALPHA = 0.45F;

// ---- Font ------------------------------------------------------------------------------

// The font file, relative to the assets directory. Licence: assets/fonts/OFL.txt.
constexpr const char* FONT_FILE = "fonts/AtkinsonHyperlegible-Regular.ttf";

// ImGui refuses (with an assertion) data of 100 bytes or less: no font file is that small.
constexpr std::size_t SMALLEST_FONT_FILE = 101;

// The first four bytes of every TrueType font file: the number 1.0 written as two
// 16 bit halves.
constexpr std::array<unsigned char, 4> TRUETYPE_SIGNATURE{0x00, 0x01, 0x00, 0x00};

// Size 0 tells ImGui to take the size of the text from the style (FontSizeBase).
constexpr float SIZE_FROM_STYLE = 0.0F;

// Reads a whole file into bytes. Returns false when the file cannot be opened.
bool readBinaryFile(const std::filesystem::path& path, std::vector<unsigned char>& bytes) {
    // The stream takes the path object itself, so on Windows a letter outside the local
    // code page is not damaged (same reason as in assets/ImageLoader.cpp).
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

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

    colors[ImGuiCol_Text] = MOONLIGHT;
    colors[ImGuiCol_TextDisabled] = MOON_DIM;
    colors[ImGuiCol_TextLink] = CRYSTAL;
    colors[ImGuiCol_TextSelectedBg] = CRYSTAL_OVERLAY;
    colors[ImGuiCol_InputTextCursor] = MOONLIGHT;

    // Backgrounds and outlines.
    colors[ImGuiCol_WindowBg] = NIGHT;
    colors[ImGuiCol_ChildBg] = INVISIBLE;
    colors[ImGuiCol_PopupBg] = NIGHT_DEEP;
    colors[ImGuiCol_MenuBarBg] = NIGHT_RAISED;
    colors[ImGuiCol_Border] = SOFT_BORDER;
    colors[ImGuiCol_BorderShadow] = INVISIBLE;

    // Title bars: only the focused panel gets the crystal accent.
    colors[ImGuiCol_TitleBg] = NIGHT_RAISED;
    colors[ImGuiCol_TitleBgActive] = CRYSTAL_DEEP;
    colors[ImGuiCol_TitleBgCollapsed] = NIGHT_RAISED;

    // Widgets with a frame: slate at rest, ember under the mouse, brighter ember when used.
    colors[ImGuiCol_FrameBg] = SLATE;
    colors[ImGuiCol_FrameBgHovered] = EMBER;
    colors[ImGuiCol_FrameBgActive] = EMBER_BRIGHT;
    colors[ImGuiCol_CheckboxSelectedBg] = CRYSTAL_DEEP;
    colors[ImGuiCol_CheckMark] = AMBER;
    colors[ImGuiCol_SliderGrab] = SLATE_BRIGHT;
    colors[ImGuiCol_SliderGrabActive] = AMBER_DEEP;

    colors[ImGuiCol_Button] = SLATE_LIGHT;
    colors[ImGuiCol_ButtonHovered] = EMBER;
    colors[ImGuiCol_ButtonActive] = EMBER_BRIGHT;

    // Header colours are used by the entries of an opened list (Combo).
    colors[ImGuiCol_Header] = SLATE_LIGHT;
    colors[ImGuiCol_HeaderHovered] = EMBER;
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

// Sets paddings, spacings, roundings and sizes: softer and a little roomier than stock.
void applyMetrics(ImGuiStyle& style) {
    style.WindowPadding = WINDOW_PADDING;
    style.FramePadding = FRAME_PADDING;
    style.ItemSpacing = ITEM_SPACING;
    style.ItemInnerSpacing = ITEM_INNER_SPACING;

    style.WindowRounding = WINDOW_ROUNDING;
    style.ChildRounding = WIDGET_ROUNDING;
    style.PopupRounding = WIDGET_ROUNDING;
    style.FrameRounding = WIDGET_ROUNDING;
    style.GrabRounding = WIDGET_ROUNDING;
    style.TabRounding = WIDGET_ROUNDING;
    style.ScrollbarRounding = WIDGET_ROUNDING;

    style.WindowBorderSize = BORDER_SIZE;
    style.PopupBorderSize = BORDER_SIZE;
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
    const std::filesystem::path path = core::assetPath(FONT_FILE);

    if (readBinaryFile(path, fontBytes) && looksLikeFont(fontBytes)) {
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
