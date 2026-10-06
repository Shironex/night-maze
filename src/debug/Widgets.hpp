// Widget kit of the debug window: cards, setting rows and the few widgets drawn by hand.
// See docs/modules/debug-ui.md
#pragma once

#include "debug/Icons.hpp"

// This header shows ImGui types, so it includes imgui.h. Only the .cpp files of
// src/debug include it, so the rest of the project still does not depend on ImGui.
#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <span>
#include <string_view>

namespace debug {

// Every size in this kit is written in pixels at 100 % display scaling and multiplied
// by the display scale when it is used, like the sizes of the theme.

/// The display scale: 1 at 100 %, 1.5 at 150 % display scaling (ImGuiStyle::FontScaleDpi,
/// set by applyTheme).
float displayScale();

/// A switch in the shape of a pill with a knob: amber with the knob on the right when on,
/// grey with the knob on the left when off. A click flips *value. Returns true in the
/// frame in which it was flipped. id tells it apart from the other widgets of its window
/// and is never shown.
bool pillToggle(const char* id, bool* value);

/// A square button that shows an icon and no text, size pixels wide. selected draws it
/// in the action colour on a faint amber patch (the current category, a pin that is
/// set). Returns true in the frame in which it was clicked.
bool iconButton(const char* id, Icon icon, bool selected, float size);

/// A row of buttons of which exactly one is chosen, like tabs. names are the texts of the
/// buttons, *current the number of the chosen one. A click on another button writes its
/// number to *current and returns true.
bool segmentedButtons(const char* id, std::span<const char* const> names, int* current);

/// A tooltip in the shape of a small card for the widget that was drawn last: the title
/// in the text colour and, under it, the explanation in the quieter colour, with line
/// breaks where the card ends. It appears when the mouse rests on the widget. text may
/// be nullptr: then the card is only the title.
void tooltipCard(const char* title, const char* text);

/// One page of the debug window: cards with setting rows in them.
///
/// A category draws itself by calling the functions of a page, top to bottom:
///
///     page.beginCard("Fog");
///     page.toggle("Fog", &fog.enabled, "Far and low surfaces fade into the fog colour.");
///     page.slider("Density", &fog.density, 0.0F, 0.5F, "%.3f /m");
///     page.endCard();
///
/// A row is a label on the left and a control on the right. The page lays the row out,
/// shows its tooltip and decides whether it is drawn at all: while the user searches,
/// only the rows that match the search text are (matchesSearch in Search.hpp), and
/// a card with no matching row is not drawn either. That is why a card is opened lazily,
/// by its first row that is shown, and why every row function can be called at any time.
///
/// A page object lives for one frame and one window. It keeps no ImGui state.
class Page {
public:
    /// search is the text of the search box: empty when nothing is searched for. It has
    /// to stay alive as long as the page. wide tells whether the window has room for
    /// two columns of cards.
    Page(std::string_view search, bool wide);

    /// True while the user searches. A category then shows the rows of all its tabs.
    bool searching() const { return m_searching; }

    /// How many rows have been shown so far: the number of matches of a search.
    int shownRows() const { return m_shownRows; }

    /// Names where the cards that follow belong, for example "Light / Shadows". The
    /// search looks into this name too, and the search results show it in the header
    /// of a card. The text has to stay alive as long as the page (a string literal).
    void setPlace(const char* place);

    /// Starts two columns of cards. The cards up to nextColumn stand in the left one,
    /// the cards after it in the right one. In a narrow window and in the search
    /// results there is one column and the three functions do nothing.
    void beginColumns();
    void nextColumn();
    void endColumns();

    /// Starts a card with the given title. Its title has to be unique in its place.
    /// Every beginCard needs an endCard.
    void beginCard(const char* title);
    void endCard();

    /// A switch. help is the text of the tooltip, or nullptr for none. Like every row
    /// function below it returns true in the frame in which the user changed the value.
    bool toggle(const char* label, bool* value, const char* help = nullptr);

    /// A slider for a number from min to max. format writes the value next to the bar
    /// (printf style, "%.2f m"). The value is always kept inside the range, also when
    /// it is typed (Ctrl and click on the bar). logarithmic gives the small values half
    /// of the bar: min has to be above 0 for it.
    bool slider(const char* label, float* value, float min, float max, const char* format,
                const char* help = nullptr, bool logarithmic = false);

    /// A slider for a whole number from min to max.
    bool sliderInt(const char* label, int* value, int min, int max, const char* format,
                   const char* help = nullptr);

    /// A list to choose one entry from. items holds the entries in one string, each
    /// ended by a zero character ("Nearest\0Bilinear\0"). *index is the number of the
    /// chosen entry.
    bool combo(const char* label, int* index, const char* items, const char* help = nullptr);

    /// A colour: its value written as hexadecimal sRGB and a swatch that opens a colour
    /// picker. rgb points to three floats from 0 to 1.
    bool color(const char* label, float* rgb, const char* help = nullptr);

    /// One or two buttons side by side, the first one in the action colour. label is
    /// not drawn: it is the title of the tooltip and what the search looks for, together
    /// with the texts of the buttons. second may be nullptr. Returns 1 or 2 in the
    /// frame in which that button was clicked, 0 otherwise.
    int buttons(const char* label, const char* first, const char* second,
                const char* help = nullptr);

    /// A read only line: the label on the left, a value on the right. The value is
    /// written like with printf: stat("Triangles", "%d", count). A value that does not
    /// fit beside its label moves to the lines below it.
    template <typename... Arguments>
    void stat(const char* label, const char* format, Arguments... arguments) {
        std::array<char, STAT_TEXT_SIZE> value{};
        // snprintf never writes more than the size it is given and always ends the
        // text with a zero. The three dots hand every argument on to it.
        std::snprintf(value.data(), value.size(), format, arguments...);
        statText(label, value.data());
    }

    /// A paragraph of explanation in the quieter text colour. Not shown in the search
    /// results.
    void note(const char* text);

    /// Starts a row with a control of the caller: the label is drawn and the cursor
    /// stands where the control belongs, controlWidth pixels before the right edge.
    /// Returns false when the row is not shown: then nothing may be drawn and endRow
    /// must not be called. Give the control a label that starts with "##", so ImGui
    /// draws no second label.
    ///
    ///     if (page.beginRow("Seed", "The maze is built from this number.", width)) {
    ///         ImGui::SetNextItemWidth(width);
    ///         ImGui::InputScalar("##seed", ImGuiDataType_U32, &seed, &step);
    ///         page.endRow();
    ///     }
    bool beginRow(const char* label, const char* help, float controlWidth);
    void endRow();

    /// Asks whether a block that is not a row (a picture, a list, a plan) is to be
    /// drawn now, and opens the card for it. keywords is what the search finds the
    /// block by. Returns false when the block is not shown.
    bool beginBlock(const char* keywords);

private:
    // Room for the value of a read only line as text. Longer values are cut off.
    static constexpr std::size_t STAT_TEXT_SIZE = 256;

    // True when a row with this label and this help text is to be shown: always
    // without a search, and with one when its words are found in the place, the title
    // of the card, the label or the help text.
    bool passes(const char* label, const char* help) const;

    // Opens the card if this is its first row. Returns false when the card cannot be
    // seen (it is scrolled out of the window): then nothing may be drawn into it.
    bool openCard();

    void statText(const char* label, const char* value);

    std::string_view m_search;
    bool m_searching;
    bool m_wide;
    int m_shownRows = 0;

    const char* m_place = "";
    const char* m_cardTitle = "";
    // Whether BeginChild was called for the card, and what it returned.
    bool m_cardOpen = false;
    bool m_cardVisible = false;
    bool m_columnsOpen = false;

    // The label and the help text of the row between beginRow and endRow.
    const char* m_rowLabel = "";
    const char* m_rowHelp = nullptr;
};

} // namespace debug
