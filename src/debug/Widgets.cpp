// Widget kit of the debug window: cards, setting rows and the few widgets drawn by hand.
// See docs/modules/debug-ui.md
#include "debug/Widgets.hpp"

#include "debug/Search.hpp"
#include "debug/Theme.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace debug {

namespace {

// ---- Sizes, in pixels at 100 % display scaling -----------------------------------------

// The pill of a switch, and the free space between the pill and its knob.
constexpr float TOGGLE_WIDTH = 32.0F;
constexpr float TOGGLE_HEIGHT = 18.0F;
constexpr float TOGGLE_KNOB_GAP = 3.0F;

// A slider: the column its value is written in, the bar itself and its ticks.
constexpr float SLIDER_VALUE_WIDTH = 56.0F;
constexpr float SLIDER_BAR_WIDTH = 60.0F;
constexpr int SLIDER_TICK_COUNT = 12;
constexpr float SLIDER_TICK_HEIGHT = 12.0F;
constexpr float SLIDER_TICK_GAP = 2.0F;
constexpr float SLIDER_TICK_ROUNDING = 2.0F;

// Width of a list (Combo): room for its longest entry, "Normals as colour".
constexpr float COMBO_WIDTH = 142.0F;

// Width of the column the hexadecimal value of a colour is written in.
constexpr float COLOR_TEXT_WIDTH = 58.0F;

// Free space between the edge of a card and its rows.
constexpr ImVec2 CARD_PADDING{12.0F, 10.0F};
// Free space between two cards that stand one below the other.
constexpr float CARD_GAP = 10.0F;

// A tooltip card: its padding and the width its text is wrapped at.
constexpr ImVec2 TOOLTIP_PADDING{12.0F, 10.0F};
constexpr float TOOLTIP_TEXT_WIDTH = 290.0F;

// The icon of an icon button takes this part of the side of the button.
constexpr float ICON_SHARE = 0.5F;
// Radius of the corners of an icon button, as a part of its side.
constexpr float ICON_BUTTON_ROUNDING_SHARE = 0.28F;

// Free space between two buttons of a segmented row.
constexpr float SEGMENT_GAP = 2.0F;

// ---- Colours only this file uses -------------------------------------------------------

// A filled accent button under the mouse and while it is pressed: lighter and darker
// than ACCENT_COLOR, so the dark text on it stays readable.
constexpr ImVec4 ACCENT_HOVER_COLOR = colorFromBytes(255, 204, 128);
constexpr ImVec4 ACCENT_ACTIVE_COLOR = colorFromBytes(232, 160, 60);
// Nothing at all: for the parts of a stock widget that are drawn by hand instead.
constexpr ImVec4 INVISIBLE = colorFromBytes(0, 0, 0, 0.0F);

// ---- Text buffers ----------------------------------------------------------------------

// Room for the value of a slider as text ("0.020 m") and for a colour ("#1b2239").
constexpr std::size_t VALUE_TEXT_SIZE = 32;
// Room for the title of a card in capital letters.
constexpr std::size_t TITLE_TEXT_SIZE = 64;
// Room for the line of text the search looks into: place, card, label and help text.
// A help text is at most a few hundred characters long.
constexpr std::size_t SEARCH_TEXT_SIZE = 1024;

constexpr float BYTE_MAX = 255.0F;

// A colour of the theme packed into the 32 bit number a draw list wants. GetColorU32
// also applies the opacity of the style, so a widget inside ImGui::BeginDisabled is
// drawn faded like the stock widgets.
ImU32 packed(const ImVec4& color) {
    return ImGui::GetColorU32(color);
}

// The part of the range from min to max that lies below value, from 0 to 1: how much of
// a slider bar is filled. On a logarithmic slider the position follows the logarithm of
// the value, which is how ImGui places the handle for a range above 0.
float filledPart(float value, float min, float max, bool logarithmic) {
    if (max <= min) {
        return 0.0F;
    }
    const float part = logarithmic && min > 0.0F ? std::log(value / min) / std::log(max / min)
                                                 : (value - min) / (max - min);
    return std::clamp(part, 0.0F, 1.0F);
}

// Reserves the column the value of a slider is written in and returns where it starts.
// The text itself is drawn after the bar, when the value of this frame is known.
ImVec2 reserveValueColumn(float scale) {
    const ImVec2 position = ImGui::GetCursorScreenPos();
    // Dummy is an invisible widget of the given size. SameLine keeps the next widget
    // on this line: its second argument is the gap, here the inner spacing of the style.
    ImGui::Dummy({SLIDER_VALUE_WIDTH * scale, ImGui::GetFrameHeight()});
    ImGui::SameLine(0.0F, ImGui::GetStyle().ItemInnerSpacing.x);
    return position;
}

// Writes text into a column that starts at position and is width pixels wide, flush
// with its right edge and at the height of the text of a widget on the same line.
void drawRightAlignedText(const ImVec2& position, float width, const char* text) {
    const float textWidth = ImGui::CalcTextSize(text).x;
    const ImVec2 textPosition{position.x + width - textWidth,
                              position.y + ImGui::GetStyle().FramePadding.y};
    ImGui::GetWindowDrawList()->AddText(textPosition, packed(TEXT_DIM_COLOR), text);
}

// The stock slider of ImGui with nothing of it drawn: it only handles the mouse, the
// keyboard, the limits and typing (Ctrl and click turns it into a text field). The bar
// is drawn over it by drawTickBar. Returns true when the value changed. *typing is set
// while the slider is a text field: then the field has to stay visible and no bar is
// drawn over it.
bool hiddenSlider(ImGuiDataType dataType, void* value, const void* min, const void* max,
                  const char* format, bool logarithmic, float width, bool* typing) {
    // Whether the slider was a text field in the frame before. ImGui only tells after
    // the widget is drawn, and the colours have to be chosen before, so the answer is
    // kept from one frame to the next in the storage of the window: a small table of
    // values that ImGui keeps for every window, looked up by an id.
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID typingKey = ImGui::GetID("typing");
    *typing = storage->GetBool(typingKey);

    // PushStyleColor changes a colour of the style until the matching PopStyleColor.
    constexpr int HIDDEN_COLOR_COUNT = 7;
    if (!*typing) {
        ImGui::PushStyleColor(ImGuiCol_Text, INVISIBLE);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, INVISIBLE);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, INVISIBLE);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, INVISIBLE);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, INVISIBLE);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, INVISIBLE);
        ImGui::PushStyleColor(ImGuiCol_Border, INVISIBLE);
    }

    // AlwaysClamp: a typed value outside the limits is forced back between them.
    ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
    if (logarithmic) {
        flags |= ImGuiSliderFlags_Logarithmic;
    }
    ImGui::SetNextItemWidth(width);
    // SliderScalar is the slider for a number of any type: the type is named by the
    // second argument and the value and the limits are passed as plain pointers.
    const bool changed = ImGui::SliderScalar("##bar", dataType, value, min, max, format, flags);

    if (!*typing) {
        ImGui::PopStyleColor(HIDDEN_COLOR_COUNT);
    }
    // WantTextInput is true while a text field is being edited. Together with "this
    // widget is the active one" it means: this slider is the text field.
    storage->SetBool(typingKey, ImGui::IsItemActive() && ImGui::GetIO().WantTextInput);
    return changed;
}

// Draws the bar of a slider over the widget that was drawn last: a row of ticks, the
// first ones in the action colour. part (0 to 1) says how many.
void drawTickBar(float part, float scale) {
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const bool lively = ImGui::IsItemHovered() || ImGui::IsItemActive();

    const float gap = SLIDER_TICK_GAP * scale;
    const float tickWidth = (max.x - min.x - gap * static_cast<float>(SLIDER_TICK_COUNT - 1)) /
                            static_cast<float>(SLIDER_TICK_COUNT);
    const float tickHeight = SLIDER_TICK_HEIGHT * scale;
    // The ticks stand in the middle of the height of the widget.
    const float top = (min.y + max.y - tickHeight) / 2.0F;
    // lround rounds to the nearest whole number: 0.5 of twelve ticks lights six.
    const int filledTicks =
        static_cast<int>(std::lround(part * static_cast<float>(SLIDER_TICK_COUNT)));

    const ImU32 filledColor = packed(ACCENT_COLOR);
    const ImU32 emptyColor = packed(lively ? TRACK_HOVER_COLOR : TRACK_COLOR);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (int tick = 0; tick < SLIDER_TICK_COUNT; ++tick) {
        const float left = min.x + static_cast<float>(tick) * (tickWidth + gap);
        drawList->AddRectFilled({left, top}, {left + tickWidth, top + tickHeight},
                                tick < filledTicks ? filledColor : emptyColor,
                                SLIDER_TICK_ROUNDING * scale);
    }
}

// The width of a button with this text, as ImGui::Button makes it: the text plus the
// frame padding on both sides.
float buttonWidth(const char* text) {
    return ImGui::CalcTextSize(text).x + 2.0F * ImGui::GetStyle().FramePadding.x;
}

} // namespace

float displayScale() {
    return ImGui::GetStyle().FontScaleDpi;
}

bool pillToggle(const char* id, bool* value) {
    const float scale = displayScale();
    const float width = TOGGLE_WIDTH * scale;
    const float height = TOGGLE_HEIGHT * scale;
    // The widget is as high as every other widget of a row, so the rows line up. The
    // pill is drawn in the middle of that height.
    const float rowHeight = ImGui::GetFrameHeight();
    const ImVec2 position = ImGui::GetCursorScreenPos();

    // InvisibleButton handles the mouse and the keyboard for an area and draws nothing.
    // It returns true in the frame in which it was clicked. EnableNav: it can also be
    // reached with the keyboard, which an invisible button cannot by default.
    const bool clicked = ImGui::InvisibleButton(id, {width, rowHeight}, ImGuiButtonFlags_EnableNav);
    if (clicked) {
        *value = !*value;
    }

    const ImVec2 pillMin{position.x, position.y + (rowHeight - height) / 2.0F};
    const ImVec2 pillMax{pillMin.x + width, pillMin.y + height};
    ImVec4 pillColor = TRACK_COLOR;
    if (*value) {
        pillColor = ACCENT_COLOR;
    } else if (ImGui::IsItemHovered()) {
        pillColor = TRACK_HOVER_COLOR;
    }
    // A rounding of half the height turns the ends of the rectangle into half circles.
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(pillMin, pillMax, packed(pillColor), height / 2.0F);

    // The knob: on the right and dark when on (it lies on amber), on the left when off.
    const float knobRadius = height / 2.0F - TOGGLE_KNOB_GAP * scale;
    const float knobOffset = TOGGLE_KNOB_GAP * scale + knobRadius;
    const ImVec2 knobCenter{*value ? pillMax.x - knobOffset : pillMin.x + knobOffset,
                            (pillMin.y + pillMax.y) / 2.0F};
    drawList->AddCircleFilled(knobCenter, knobRadius,
                              packed(*value ? ON_ACCENT_COLOR : TEXT_DIM_COLOR));
    return clicked;
}

bool iconButton(const char* id, Icon icon, bool selected, float size) {
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(id, {size, size}, ImGuiButtonFlags_EnableNav);
    const bool hovered = ImGui::IsItemHovered();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 corner{position.x + size, position.y + size};
    const float rounding = size * ICON_BUTTON_ROUNDING_SHARE;
    if (selected) {
        drawList->AddRectFilled(position, corner, packed(ACCENT_SOFT_COLOR), rounding);
    } else if (hovered) {
        drawList->AddRectFilled(position, corner, packed(CONTROL_COLOR), rounding);
    }

    ImVec4 color = TEXT_DIM_COLOR;
    if (selected) {
        color = ACCENT_COLOR;
    } else if (hovered) {
        color = TEXT_COLOR;
    }
    // The icon stands in the middle of the button.
    const float iconSize = size * ICON_SHARE;
    const float inset = (size - iconSize) / 2.0F;
    drawIcon(drawList, icon, {position.x + inset, position.y + inset}, iconSize, packed(color));
    return clicked;
}

bool segmentedButtons(const char* id, std::span<const char* const> names, int* current) {
    bool changed = false;
    // PushID adds a name to the ids of the widgets that follow, so two rows of buttons
    // with the same texts do not get the same ids.
    ImGui::PushID(id);
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        if (i > 0) {
            ImGui::SameLine(0.0F, SEGMENT_GAP * displayScale());
        }
        const bool chosen = i == *current;
        // The chosen button is filled with the action colour and gets dark text. The
        // others look like every other button, with quieter text.
        constexpr int PUSHED_COLOR_COUNT = 4;
        ImGui::PushStyleColor(ImGuiCol_Button, chosen ? ACCENT_COLOR : CONTROL_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, chosen ? ACCENT_COLOR : TRACK_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, chosen ? ACCENT_COLOR : TRACK_HOVER_COLOR);
        ImGui::PushStyleColor(ImGuiCol_Text, chosen ? ON_ACCENT_COLOR : TEXT_DIM_COLOR);
        if (ImGui::Button(names[static_cast<std::size_t>(i)]) && !chosen) {
            *current = i;
            changed = true;
        }
        ImGui::PopStyleColor(PUSHED_COLOR_COUNT);
    }
    ImGui::PopID();
    return changed;
}

void tooltipCard(const char* title, const char* text) {
    // ForTooltip: the mouse has to rest on the widget for a moment, so the cards do not
    // flash up while the mouse only passes over the rows.
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
        return;
    }
    const float scale = displayScale();
    // PushStyleVar changes a metric of the style until the matching PopStyleVar. It has
    // to be set before BeginTooltip, which reads the padding.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        {TOOLTIP_PADDING.x * scale, TOOLTIP_PADDING.y * scale});
    if (ImGui::BeginTooltip()) {
        // Text that reaches this distance from the left edge continues on the next line.
        ImGui::PushTextWrapPos(TOOLTIP_TEXT_WIDTH * scale);
        ImGui::TextUnformatted(title);
        if (text != nullptr) {
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM_COLOR);
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::PopStyleVar();
}

Page::Page(std::string_view search, bool wide)
    : m_search(search), m_searching(hasSearchWords(search)), m_wide(wide) {}

void Page::setPlace(const char* place) {
    m_place = place;
}

void Page::beginColumns() {
    if (!m_wide || m_searching) {
        return;
    }
    // A table with two columns of equal width and one row. BeginTable returns false
    // when no part of the table can be seen: EndTable must not be called then.
    constexpr int COLUMN_COUNT = 2;
    m_columnsOpen = ImGui::BeginTable("columns", COLUMN_COUNT, ImGuiTableFlags_SizingStretchSame);
    if (m_columnsOpen) {
        // TableNextColumn moves on to the next cell: here into the first one.
        ImGui::TableNextColumn();
    }
}

void Page::nextColumn() {
    if (m_columnsOpen) {
        ImGui::TableNextColumn();
    }
}

void Page::endColumns() {
    if (m_columnsOpen) {
        ImGui::EndTable();
    }
    m_columnsOpen = false;
}

void Page::beginCard(const char* title) {
    // Nothing is drawn yet: the first row that is shown opens the card (openCard).
    m_cardTitle = title;
    m_cardOpen = false;
    m_cardVisible = false;
}

void Page::endCard() {
    if (!m_cardOpen) {
        return;
    }
    // EndChild must be called whatever BeginChild returned. The gap to the next card
    // is the item spacing at this moment, so it is set around the call.
    const float scale = displayScale();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        {ImGui::GetStyle().ItemSpacing.x, CARD_GAP * scale});
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopID();
    m_cardOpen = false;
}

bool Page::passes(const char* label, const char* help) const {
    if (!m_searching) {
        return true;
    }
    // Everything the row can be found by, in one line of text.
    std::array<char, SEARCH_TEXT_SIZE> text{};
    std::snprintf(text.data(), text.size(), "%s %s %s %s", m_place, m_cardTitle, label,
                  help != nullptr ? help : "");
    return matchesSearch(text.data(), m_search);
}

bool Page::openCard() {
    if (m_cardOpen) {
        return m_cardVisible;
    }
    m_cardOpen = true;
    const float scale = displayScale();

    // The place is part of the id of the card, so cards with the same title in two
    // places are still two cards for ImGui. That matters in the search results, where
    // the cards of all places stand in one window.
    ImGui::PushID(m_place);
    // A card is a child window: a window inside a window, with its own background and
    // outline. Size 0 means "as wide as the room that is left", and AutoResizeY makes
    // it as high as its contents. NavFlattened lets the keyboard move from the rows of
    // one card to the rows of the next, as if they were in one window. Borders draws
    // the outline and gives the card its padding.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, CARD_COLOR);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        {CARD_PADDING.x * scale, CARD_PADDING.y * scale});
    m_cardVisible = ImGui::BeginChild(m_cardTitle, {0.0F, 0.0F},
                                      ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY |
                                          ImGuiChildFlags_NavFlattened);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    if (!m_cardVisible) {
        return false;
    }

    // The header: the title in capital letters and the quieter colour.
    std::array<char, TITLE_TEXT_SIZE> title{};
    std::snprintf(title.data(), title.size(), "%s", m_cardTitle);
    for (char& character : title) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }
    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM_COLOR);
    ImGui::TextUnformatted(title.data());
    ImGui::PopStyleColor();

    // In the search results the cards of all categories stand below each other, so the
    // header also says where the card comes from, at its right edge.
    if (m_searching) {
        const float placeWidth = ImGui::CalcTextSize(m_place).x;
        ImGui::SameLine();
        const float room = ImGui::GetContentRegionAvail().x;
        if (placeWidth <= room) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + room - placeWidth);
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_FAINT_COLOR);
            ImGui::TextUnformatted(m_place);
            ImGui::PopStyleColor();
        } else {
            // No room beside the title: the place is left out. NewLine ends the line
            // that SameLine kept open.
            ImGui::NewLine();
        }
    }
    return true;
}

bool Page::beginRow(const char* label, const char* help, float controlWidth) {
    if (!passes(label, help)) {
        return false;
    }
    ++m_shownRows;
    if (!openCard()) {
        return false;
    }
    m_rowLabel = label;
    m_rowHelp = help;
    // The label is part of the id of everything in the row, so the controls of two rows
    // can have the same hidden name ("##bar").
    ImGui::PushID(label);

    const ImGuiStyle& style = ImGui::GetStyle();
    const float rowWidth = ImGui::GetContentRegionAvail().x;
    const float rowHeight = ImGui::GetFrameHeight();
    // The label gets what the control leaves. A label that is too long is cut off at
    // the end of its column (PushClipRect): its tooltip still shows all of it.
    const float labelWidth = std::max(rowWidth - controlWidth - style.ItemSpacing.x, 0.0F);
    const ImVec2 position = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(position, {position.x + labelWidth, position.y + rowHeight}, true);
    drawList->AddText({position.x, position.y + style.FramePadding.y}, packed(TEXT_COLOR), label);
    drawList->PopClipRect();
    // The draw list does not move the cursor. Dummy reserves the column of the label
    // and is the widget the tooltip of the label belongs to.
    ImGui::Dummy({labelWidth, rowHeight});
    tooltipCard(label, help);
    ImGui::SameLine();
    return true;
}

void Page::endRow() {
    // The same card for the control as for the label: it belongs to the widget that
    // was drawn last, which is the control.
    tooltipCard(m_rowLabel, m_rowHelp);
    ImGui::PopID();
}

bool Page::toggle(const char* label, bool* value, const char* help) {
    if (!beginRow(label, help, TOGGLE_WIDTH * displayScale())) {
        return false;
    }
    const bool changed = pillToggle("##toggle", value);
    endRow();
    return changed;
}

bool Page::slider(const char* label, float* value, float min, float max, const char* format,
                  const char* help, bool logarithmic) {
    const float scale = displayScale();
    const float barWidth = SLIDER_BAR_WIDTH * scale;
    const float valueWidth = SLIDER_VALUE_WIDTH * scale;
    if (!beginRow(label, help, valueWidth + ImGui::GetStyle().ItemInnerSpacing.x + barWidth)) {
        return false;
    }
    const ImVec2 valuePosition = reserveValueColumn(scale);
    bool typing = false;
    const bool changed = hiddenSlider(ImGuiDataType_Float, value, &min, &max, format, logarithmic,
                                      barWidth, &typing);
    if (!typing) {
        drawTickBar(filledPart(*value, min, max, logarithmic), scale);
    }
    // The value as text, written with the format of the caller, after the slider has
    // changed it.
    std::array<char, VALUE_TEXT_SIZE> text{};
    std::snprintf(text.data(), text.size(), format, static_cast<double>(*value));
    drawRightAlignedText(valuePosition, valueWidth, text.data());
    endRow();
    return changed;
}

bool Page::sliderInt(const char* label, int* value, int min, int max, const char* format,
                     const char* help) {
    const float scale = displayScale();
    const float barWidth = SLIDER_BAR_WIDTH * scale;
    const float valueWidth = SLIDER_VALUE_WIDTH * scale;
    if (!beginRow(label, help, valueWidth + ImGui::GetStyle().ItemInnerSpacing.x + barWidth)) {
        return false;
    }
    const ImVec2 valuePosition = reserveValueColumn(scale);
    bool typing = false;
    const bool changed =
        hiddenSlider(ImGuiDataType_S32, value, &min, &max, format, false, barWidth, &typing);
    if (!typing) {
        drawTickBar(filledPart(static_cast<float>(*value), static_cast<float>(min),
                               static_cast<float>(max), false),
                    scale);
    }
    std::array<char, VALUE_TEXT_SIZE> text{};
    std::snprintf(text.data(), text.size(), format, *value);
    drawRightAlignedText(valuePosition, valueWidth, text.data());
    endRow();
    return changed;
}

bool Page::combo(const char* label, int* index, const char* items, const char* help) {
    const float width = COMBO_WIDTH * displayScale();
    if (!beginRow(label, help, width)) {
        return false;
    }
    ImGui::SetNextItemWidth(width);
    // Combo works on the number of the chosen entry and returns true in the frame in
    // which the user picked another one.
    const bool changed = ImGui::Combo("##combo", index, items);
    endRow();
    return changed;
}

bool Page::color(const char* label, float* rgb, const char* help) {
    const float scale = displayScale();
    const float textWidth = COLOR_TEXT_WIDTH * scale;
    // With NoInputs the colour widget is only its swatch: a square as high as a row.
    const float swatchWidth = ImGui::GetFrameHeight();
    const float gap = ImGui::GetStyle().ItemInnerSpacing.x;
    if (!beginRow(label, help, textWidth + gap + swatchWidth)) {
        return false;
    }
    const ImVec2 textPosition = ImGui::GetCursorScreenPos();
    ImGui::Dummy({textWidth, ImGui::GetFrameHeight()});
    ImGui::SameLine(0.0F, gap);
    // ColorEdit3 reads and writes three floats through the pointer. A click on the
    // swatch opens the colour picker, where the three numbers can also be typed.
    const bool changed = ImGui::ColorEdit3(
        "##color", rgb, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    // The value as it is usually written down: two hexadecimal digits per channel.
    const auto toByte = [](float channel) {
        return static_cast<int>(std::lround(std::clamp(channel, 0.0F, 1.0F) * BYTE_MAX));
    };
    std::array<char, VALUE_TEXT_SIZE> text{};
    std::snprintf(text.data(), text.size(), "#%02x%02x%02x", toByte(rgb[0]), toByte(rgb[1]),
                  toByte(rgb[2]));
    drawRightAlignedText(textPosition, textWidth, text.data());
    endRow();
    return changed;
}

int Page::buttons(const char* label, const char* first, const char* second, const char* help) {
    // The search finds the row by its label and by the texts on its buttons.
    std::array<char, STAT_TEXT_SIZE> searchText{};
    std::snprintf(searchText.data(), searchText.size(), "%s %s %s", label, first,
                  second != nullptr ? second : "");
    if (!passes(searchText.data(), help)) {
        return 0;
    }
    ++m_shownRows;
    if (!openCard()) {
        return 0;
    }

    int clicked = 0;
    // The first button is the main action: filled with the action colour, dark text.
    constexpr int PUSHED_COLOR_COUNT = 4;
    ImGui::PushStyleColor(ImGuiCol_Button, ACCENT_COLOR);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ACCENT_HOVER_COLOR);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACCENT_ACTIVE_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Text, ON_ACCENT_COLOR);
    // Button returns true only in the frame in which it was clicked.
    if (ImGui::Button(first)) {
        clicked = 1;
    }
    ImGui::PopStyleColor(PUSHED_COLOR_COUNT);
    tooltipCard(label, help);

    if (second != nullptr) {
        // Beside the first button when the card is wide enough, below it otherwise.
        ImGui::SameLine();
        if (buttonWidth(second) > ImGui::GetContentRegionAvail().x) {
            ImGui::NewLine();
        }
        if (ImGui::Button(second)) {
            clicked = 2;
        }
        tooltipCard(label, help);
    }
    return clicked;
}

void Page::statText(const char* label, const char* value) {
    // The value is part of what the search finds: "OpenGL" also finds the line by the
    // name of the graphics card.
    if (!passes(label, value)) {
        return;
    }
    ++m_shownRows;
    if (!openCard()) {
        return;
    }
    const float rowWidth = ImGui::GetContentRegionAvail().x;
    const float labelWidth = ImGui::CalcTextSize(label).x;
    const float valueWidth = ImGui::CalcTextSize(value).x;

    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM_COLOR);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, SECONDARY_COLOR);
    if (labelWidth + ImGui::GetStyle().ItemSpacing.x + valueWidth <= rowWidth) {
        // Beside the label, flush with the right edge of the card.
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x -
                             valueWidth);
        ImGui::TextUnformatted(value);
    } else {
        // Too long for one line: below the label, broken at the edge of the card.
        ImGui::TextWrapped("%s", value);
    }
    ImGui::PopStyleColor();
}

void Page::note(const char* text) {
    if (m_searching || !openCard()) {
        return;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM_COLOR);
    // "%s" and the text as an argument: the text itself is never read as a format.
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

bool Page::beginBlock(const char* keywords) {
    if (!passes(keywords, nullptr)) {
        return false;
    }
    return openCard();
}

} // namespace debug
