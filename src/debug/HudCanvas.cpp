// The canvas of the HUD: the measures of its text and the helpers that draw it.
#include "debug/HudShared.hpp"

#include "debug/Theme.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string_view>

namespace debug {

namespace {

// ImGui measures a text by the height of its line, from the top of the highest letter
// to the bottom of the lowest. In the font of the game (Atkinson Hyperlegible) that is
// 1.24 times the em, the size its letters are designed in: 950 units above the base
// line and 290 below it, of 1000. A size in em times this number is the size ImGui has
// to be asked for.
constexpr float FONT_LINE_PER_EM = 1.24F;

// A small label is written with this much extra room after every letter.
constexpr float LABEL_SPACING = 2.2F;

// The HUD has no panel behind its text. Every text gets a dark copy of itself this far
// below it, of this opacity, so it can be read over a bright wall too.
constexpr float TEXT_SHADOW_OFFSET = 1.0F;
constexpr float TEXT_SHADOW_OPACITY = 0.85F;

constexpr int SECONDS_PER_MINUTE = 60;

} // namespace

// A time in seconds as text, minutes and seconds: 95.4 becomes "1:35". The fraction of
// a second is dropped.
std::array<char, TIME_TEXT_SIZE> timeText(float seconds) {
    const int wholeSeconds = static_cast<int>(seconds);
    std::array<char, TIME_TEXT_SIZE> text{};
    // snprintf never writes more than the size it is given and always ends the text
    // with a zero. %02d: two digits, with a leading zero.
    std::snprintf(text.data(), text.size(), "%d:%02d", wholeSeconds / SECONDS_PER_MINUTE,
                  wholeSeconds % SECONDS_PER_MINUTE);
    return text;
}

// The point of the game window that lies at the given part of it (see TOP_CENTER).
ImVec2 windowPoint(const ImVec2& part) {
    // The main viewport is the window of the program. WorkPos is its top left corner and
    // WorkSize its size.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    return {viewport->WorkPos.x + part.x * viewport->WorkSize.x,
            viewport->WorkPos.y + part.y * viewport->WorkSize.y};
}

// The height of a line of text of the given size (in em) on the screen, in whole
// pixels: this is the size ImGui is asked for. A whole number of pixels, because
// letters of such a height are drawn sharp.
float fontPixels(const Canvas& canvas, float size) {
    return std::round(size * FONT_LINE_PER_EM * canvas.scale);
}

// How wide a text of the given size is on the screen, in pixels.
float textWidth(const Canvas& canvas, float size, std::string_view text) {
    // The two FLT_MAX and 0 say: no width to cut the text at and no line wrapping.
    // A string_view has no closing zero, so the end of the text is given too.
    return canvas.font
        ->CalcTextSizeA(fontPixels(canvas, size), FLT_MAX, 0.0F, text.data(),
                        text.data() + text.size())
        .x;
}

// How far below the top of a text of the given size its letters stand (their base
// line), in pixels of the screen. Two texts of different sizes that stand side by side
// are lined up with it.
float baseLine(const Canvas& canvas, float size) {
    // ImGui keeps the measures of a font once for every size it is drawn in.
    return canvas.font->GetFontBaked(fontPixels(canvas, size))->Ascent;
}

// How far below the top of its line a text of the given size starts, in pixels of the
// screen. The room of a line is LINE_HEIGHT em and the text ImGui draws is
// FONT_LINE_PER_EM em high: the text stands in the middle of its room. The two numbers
// are nearly the same, so this is a fraction of a pixel.
float lineInset(const Canvas& canvas, float size) {
    return size * (LINE_HEIGHT - FONT_LINE_PER_EM) / 2.0F * canvas.scale;
}

// Draws a text with its top left corner at position, with its dark copy below it.
void drawText(const Canvas& canvas, const ImVec2& position, float size, const ImVec4& color,
              std::string_view text) {
    const float pixels = fontPixels(canvas, size);
    // Whole pixels again: a text that starts between two pixels is blurred.
    const ImVec2 corner{std::floor(position.x), std::floor(position.y)};
    const float shadow = std::max(std::round(TEXT_SHADOW_OFFSET * canvas.scale), 1.0F);
    // A string_view has no closing zero, so the end of the text is given too.
    canvas.list->AddText(canvas.font, pixels, {corner.x, corner.y + shadow},
                         ink(canvas, HUD_SHADOW_COLOR, TEXT_SHADOW_OPACITY), text.data(),
                         text.data() + text.size());
    canvas.list->AddText(canvas.font, pixels, corner, ink(canvas, color), text.data(),
                         text.data() + text.size());
}

// A small label in capital letters is written with extra room between its letters, like
// the labels of the menus. ImGui cannot do that by itself, so these two go through the
// text letter by letter. The labels are plain English: one byte is one letter.

// How wide such a label is, in pixels of the screen. No room is counted after the last
// letter.
float spacedTextWidth(const Canvas& canvas, float size, std::string_view text) {
    float width = 0.0F;
    for (std::size_t i = 0; i < text.size(); ++i) {
        width += textWidth(canvas, size, text.substr(i, 1));
        if (i + 1 < text.size()) {
            width += LABEL_SPACING * canvas.scale;
        }
    }
    return width;
}

// Draws such a label with its top left corner at position.
void drawSpacedText(const Canvas& canvas, ImVec2 position, float size, const ImVec4& color,
                    std::string_view text) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        const std::string_view letter = text.substr(i, 1);
        drawText(canvas, position, size, color, letter);
        position.x += textWidth(canvas, size, letter) + LABEL_SPACING * canvas.scale;
    }
}

} // namespace debug
