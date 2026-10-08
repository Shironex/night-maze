// Game HUD: the lamp gauge, the crystal counter, the stamina line, the loudness ticks,
// sentences that come and go, the crosshair with its prompt, the name of the night, the
// card of a note and the "You escaped" card.
#include "debug/Hud.hpp"

#include "debug/HudRules.hpp"
#include "debug/Theme.hpp"
#include "game/Campaign.hpp"
#include "game/Interaction.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"

#include <glm/gtc/constants.hpp>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace debug {

namespace {

// Every size below is a number of pixels in a window 720 pixels high
// (HUD_REFERENCE_HEIGHT). It is multiplied by the scale of the HUD (debug::hudScale), so
// the HUD takes the same part of a window of any size. Places are parts of the window.

// Height of the text of the cards and of the line after a catch, the way ImGui
// measures a text: the height of its line.
constexpr float CARD_FONT_SIZE = 16.0F;

// The text sizes of everything that is drawn by hand: a sentence and the prompt, the
// two numbers that matter (the charge of the lamp, the crystals) and the name of the
// night, the small lines under them (the percent sign, "of 13", the time), the seconds
// of the tea, and the small labels in capital letters (LAMP, GATE OPEN, NIGHT 2).
// These are sizes of the letters themselves, the em of the font, which is how the
// design of the HUD was drawn (FONT_LINE_PER_EM).
constexpr float HUD_FONT_SIZE = 16.0F;
constexpr float NUMBER_FONT_SIZE = 20.0F;
constexpr float SMALL_FONT_SIZE = 12.0F;
constexpr float TEA_FONT_SIZE = 13.0F;
constexpr float LABEL_FONT_SIZE = 11.0F;

// ImGui measures a text by the height of its line, from the top of the highest letter
// to the bottom of the lowest. In the font of the game (Atkinson Hyperlegible) that is
// 1.24 times the em, the size its letters are designed in: 950 units above the base
// line and 290 below it, of 1000. A size in em times this number is the size ImGui has
// to be asked for.
constexpr float FONT_LINE_PER_EM = 1.24F;

// A small label is written with this much extra room after every letter.
constexpr float LABEL_SPACING = 2.2F;

// Lines of text that stand above each other are this many em apart, measured from top
// to top.
constexpr float LINE_HEIGHT = 1.2F;

// The HUD has no panel behind its text. Every text gets a dark copy of itself this far
// below it, of this opacity, so it can be read over a bright wall too.
constexpr float TEXT_SHADOW_OFFSET = 1.0F;
constexpr float TEXT_SHADOW_OPACITY = 0.85F;

// The empty part of every gauge (the ring, the two lines, a tick that is not lit) is the
// colour of the text, this faint.
constexpr float TRACK_OPACITY = 0.16F;

// ---- The lamp gauge, bottom left ----

// Where the bottom left corner of the gauge is.
constexpr ImVec2 LAMP_PLACE{0.02188F, 1.0F - 0.03333F};

// The ring: its outer radius and how thick its line is. A dark disc of the same radius
// lies behind it, and a dot of the colour of the ring marks its middle.
constexpr float LAMP_RING_RADIUS = 20.0F;
constexpr float LAMP_RING_THICKNESS = 3.5F;
constexpr float LAMP_DISC_OPACITY = 0.55F;
constexpr float LAMP_DOT_RADIUS = 4.5F;

// The text stands this far right of the ring: the number, and the label this far under
// it. The percent sign stands this far right of the number.
constexpr float LAMP_TEXT_GAP = 9.6F;
constexpr float LAMP_LINE_GAP = 1.6F;
constexpr float LAMP_PERCENT_GAP = 1.8F;

// A low lamp flickers (game::flashlightFlicker), and the gauge dips with it: this is its
// opacity when the lamp is all the way down.
constexpr float LAMP_DIP_FAINTEST = 0.4F;

constexpr float PERCENT = 100.0F;

// ---- The crystal counter, top right ----

// Where the top right corner of the counter is. Everything in it ends at that edge.
constexpr ImVec2 CRYSTALS_PLACE{1.0F - 0.02188F, 0.03056F};

// The little crystal in front of the numbers: a diamond, from its middle this far to
// the side and this far up and down, with an outline this thick. It stands this far
// left of the numbers.
constexpr float GEM_HALF_WIDTH = 5.5F;
constexpr float GEM_HALF_HEIGHT = 8.0F;
constexpr float GEM_OUTLINE = 2.0F;
constexpr float GEM_GAP = 16.0F;

// The bar of all crystals of the maze, and the tick on it where the gate opens: how
// wide the tick is and how far it sticks out above and below the bar.
constexpr float CRYSTAL_BAR_WIDTH = 96.0F;
constexpr float CRYSTAL_BAR_HEIGHT = 3.0F;
constexpr float CRYSTAL_TICK_WIDTH = 1.6F;
constexpr float CRYSTAL_TICK_OVERHANG = 3.2F;

// The three rows of the counter stand this far apart, and the two texts of the last row
// ("of 13" and the time) this far.
constexpr float CRYSTAL_ROW_GAP = 5.76F;
constexpr float CRYSTAL_SUB_GAP = 12.0F;

// ---- The stamina line, bottom centre ----

// Where the middle of the top edge of the line is.
constexpr ImVec2 STAMINA_PLACE{0.5F, 0.95556F};

// The line and the two marks at its ends, which show how long a full line is: how wide
// a mark is and how far it sticks out above and below the line.
constexpr float STAMINA_LINE_WIDTH = 220.0F;
constexpr float STAMINA_LINE_HEIGHT = 3.0F;
constexpr float STAMINA_MARK_WIDTH = 1.6F;
constexpr float STAMINA_MARK_OVERHANG = 3.0F;

// The seconds of the tea stand this far right of the line.
constexpr float TEA_TEXT_GAP = 10.4F;

// How many times per second the stamina line of a winded player goes faint and back,
// and its opacity at the faintest moment (1 hides what is behind, 0 is invisible).
constexpr float WINDED_PULSES_PER_SECOND = 2.0F;
constexpr float WINDED_FAINTEST = 0.3F;

// ---- The loudness ticks, under the stamina line ----

// Where the middle of the bottom edge of the three ticks is: in the middle like the
// stamina line, between it and the edge of the window. Sprinting drains the one and
// lights the other, so the two are read together.
constexpr ImVec2 NOISE_PLACE{0.5F, 0.98611F};

// The ticks: how wide one is, how far apart they stand, and how high each is. They grow
// from left to right, like the bars of a signal.
constexpr float NOISE_TICK_WIDTH = 2.56F;
constexpr float NOISE_TICK_GAP = 2.56F;
constexpr std::array<float, game::NOISE_TICK_COUNT> NOISE_TICK_HEIGHTS{4.8F, 8.4F, 12.0F};

// ---- Sentences that come and go ----

// Where the middle of a sentence is: between the place of a note card and the prompt.
constexpr ImVec2 HINT_PLACE{0.5F, 0.835F};

// How long each sentence is fully there, in seconds, and how long it takes to fade
// after that (debug::hintOpacity). What it says stays on the screen afterwards in
// another form: GATE OPEN under the counter, the copper line of the tea, the red gauge.
constexpr float GATE_HINT_SECONDS = 6.0F;
constexpr float TEA_HINT_SECONDS = 4.0F;
constexpr float BATTERY_HINT_SECONDS = 6.0F;
constexpr float HINT_FADE_SECONDS = 0.5F;

// Room for a sentence with a number in it, and for a number alone.
constexpr std::size_t HINT_TEXT_SIZE = 64;
constexpr std::size_t NUMBER_TEXT_SIZE = 16;

// ---- The name of the night, top left ----

// Where the top left corner of the name is, and how far its two lines stand apart.
constexpr ImVec2 NIGHT_NAME_PLACE{0.02188F, 0.03056F};
constexpr float NIGHT_NAME_LINE_GAP = 4.8F;

// The name is there for the first seconds of a night and then fades. Together the two
// times are not longer than game::CAUGHT_LINE_SECONDS: a round that starts again after
// a catch shows the caught line for that long, and the name must not come up behind it.
constexpr float NIGHT_NAME_SECONDS = 4.5F;
constexpr float NIGHT_NAME_FADE_SECONDS = 0.5F;

// ---- The cards ----

// How much of the scene shows through a card: little, it is meant to be read.
constexpr float CARD_OPACITY = 0.9F;

// The title of the card is drawn this much larger than the other text.
constexpr float CARD_TITLE_SCALE = 1.8F;

// Free space around the text of the card, wider than in the debug window.
constexpr ImVec2 CARD_PADDING{28.0F, 20.0F};

// Places on the screen as parts of the window: x is 0 at the left edge and 1 at the
// right edge, y is 0 at the top edge and 1 at the bottom edge.
constexpr float MIDDLE = 0.5F;
constexpr ImVec2 TOP_CENTER{MIDDLE, 0.0F};
constexpr ImVec2 CENTER{MIDDLE, MIDDLE};

// The card of a note stands below the middle of the window, so the crosshair and the
// scene in front of the player stay free.
constexpr ImVec2 BELOW_CENTER{MIDDLE, 0.74F};

// The crosshair: a dot in the middle of the window, and a ring around it while the
// player can use what it points at. Radii and the thickness of the ring in pixels.
constexpr float CROSSHAIR_DOT_RADIUS = 2.0F;
constexpr float CROSSHAIR_RING_RADIUS = 8.0F;
constexpr float CROSSHAIR_RING_THICKNESS = 1.5F;

// The line after a catch stands between the crosshair and the place of a note card. It
// is a line of text and not a card: the round goes on behind it.
constexpr ImVec2 CAUGHT_LINE_PLACE{MIDDLE, 0.62F};

// The prompt (the key in a small box, then "pull lever") stands near the bottom edge of
// the window, in the middle. The thing the player points at is around the crosshair, so
// down there the prompt never covers it. It is also below the card of a note
// (BELOW_CENTER). While the map is held nothing can be used, so the prompt and the map
// never meet.
constexpr ImVec2 PROMPT_PLACE{MIDDLE, 0.9F};

// The prompt lies on a dark pill with a thin outline: how much of the scene the pill
// hides, the free room inside it, and how round its corners are.
constexpr float PROMPT_PILL_OPACITY = 0.6F;
constexpr ImVec2 PROMPT_PADDING{12.8F, 6.4F};
constexpr float PROMPT_ROUNDING = 8.0F;
constexpr float PROMPT_OUTLINE = 1.0F;

// The key cap: a box with an outline around the name of the key. It is a square of
// this size for a name of one letter and grows for a longer one ("Left Shift"), which
// keeps this much room on each side. The words stand this far right of it.
constexpr float KEY_CAP_SIZE = 24.0F;
constexpr float KEY_CAP_PADDING = 4.8F;
constexpr float KEY_CAP_ROUNDING = 4.8F;
constexpr float KEY_CAP_OUTLINE = 1.44F;
constexpr float KEY_CAP_GAP = 8.8F;

// How much of the scene shows through the line after a catch, so the ground in front of
// the player stays visible behind it.
constexpr float CAUGHT_LINE_OPACITY = 0.55F;

constexpr int SECONDS_PER_MINUTE = 60;

// Room for a time written as text: minutes, a colon, two digits of seconds and the
// closing zero. Far more than a round can last.
constexpr std::size_t TIME_TEXT_SIZE = 16;

// The flags of the HUD windows. Together they make a window that is only a picture:
//   NoDecoration        no title bar, no resize grip, no scrollbar
//   AlwaysAutoResize    exactly as large as its contents, in every frame
//   NoInputs            the mouse passes through it, it is never hovered or clicked
//   NoNav               the keyboard navigation of ImGui skips it
//   NoFocusOnAppearing  appearing does not take the focus from the debug window
//   NoSavedSettings     nothing about it is written to imgui.ini
//   NoDocking           it cannot be docked into the dock area
//   NoMove              it stays where the code puts it
constexpr ImGuiWindowFlags PICTURE_WINDOW_FLAGS =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;

// The line after a catch also stays behind the debug UI (NoBringToFrontOnFocus):
// a pinned panel that is dragged over it is being worked with, and the line must not
// cover its widgets. The cards have no such flag: ImGui puts a new window in front of
// the ones that are already there, so a card starts on top of the debug window.
// A window that is clicked afterwards comes in front of it, like in front of any other
// window.
constexpr ImGuiWindowFlags STATUS_WINDOW_FLAGS =
    PICTURE_WINDOW_FLAGS | ImGuiWindowFlags_NoBringToFrontOnFocus;
constexpr ImGuiWindowFlags CARD_WINDOW_FLAGS = PICTURE_WINDOW_FLAGS;

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

// What every part of the HUD that is drawn by hand needs: the list of shapes it draws
// into, the font, the scale of the HUD and how much of the part can be seen (1 fully,
// 0 nothing). A part that fades or pulses draws with a copy that has a lower opacity.
struct Canvas {
    // The background draw list takes shapes in screen coordinates and draws them
    // behind every ImGui window, so the HUD never covers the debug window or a card.
    ImDrawList* list = nullptr;
    ImFont* font = nullptr;
    float scale = 1.0F;
    float opacity = 1.0F;
};

// A colour as the draw list wants it (one number), for something drawn on the canvas:
// opacity is multiplied with the opacity of the colour and with the one of the canvas.
ImU32 ink(const Canvas& canvas, const ImVec4& color, float opacity = 1.0F) {
    return ImGui::ColorConvertFloat4ToU32(
        {color.x, color.y, color.z, color.w * opacity * canvas.opacity});
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

// The lamp gauge at the bottom left: a ring that empties with the battery, the charge
// as a number and a label. Amber while there is charge, red once it is low.
void drawLampGauge(Canvas canvas, const game::Round& round,
                   const game::GameplaySettings& settings) {
    const bool low = round.battery < settings.lowBatteryThreshold;
    const ImVec4& color = low ? HUD_BATTERY_LOW_COLOR : HUD_BATTERY_COLOR;
    // A low lamp flickers, and the gauge dips with it: the same number dims both. An
    // empty lamp gives no light at all (the flicker is 0 then), but its gauge has to
    // stay readable, so it does not dip.
    if (low && round.battery > 0.0F) {
        const float flicker =
            game::flashlightFlicker(round.battery, round.animationSeconds, settings);
        canvas.opacity *= LAMP_DIP_FAINTEST + (1.0F - LAMP_DIP_FAINTEST) * flicker;
    }
    const float scale = canvas.scale;

    const ImVec2 corner = windowPoint(LAMP_PLACE);
    const float radius = LAMP_RING_RADIUS * scale;
    const ImVec2 center{corner.x + radius, corner.y - radius};
    canvas.list->AddCircleFilled(center, radius, ink(canvas, HUD_SHADOW_COLOR, LAMP_DISC_OPACITY));

    // A thick line is drawn half to each side of its path, so the path of the ring
    // lies half a thickness inside the outer radius. First the whole ring, faint, then
    // the part that is left of the charge on top of it.
    const float thickness = LAMP_RING_THICKNESS * scale;
    const float pathRadius = radius - thickness / 2.0F;
    // The fourth argument is the number of straight pieces: 0 lets ImGui choose.
    canvas.list->AddCircle(center, pathRadius, ink(canvas, TEXT_COLOR, TRACK_OPACITY), 0,
                           thickness);
    if (round.battery > 0.0F) {
        // Angles are in radians, 0 points to the right and they grow clockwise (y grows
        // downwards on the screen). The arc starts at the top, a quarter turn before 0,
        // and goes round by the charge: a full battery is a full turn.
        const float top = -glm::half_pi<float>();
        const float charge = std::min(round.battery, 1.0F);
        canvas.list->PathArcTo(center, pathRadius, top, top + charge * glm::two_pi<float>());
        canvas.list->PathStroke(ink(canvas, color), thickness);
    }
    canvas.list->AddCircleFilled(center, LAMP_DOT_RADIUS * scale, ink(canvas, color));

    // The two lines of text, as a block whose middle is level with the middle of the
    // ring.
    const float numberLine = NUMBER_FONT_SIZE * LINE_HEIGHT * scale;
    const float labelLine = LABEL_FONT_SIZE * LINE_HEIGHT * scale;
    const float blockHeight = numberLine + LAMP_LINE_GAP * scale + labelLine;
    const float left = corner.x + 2.0F * radius + LAMP_TEXT_GAP * scale;
    const float top = center.y - blockHeight / 2.0F;

    std::array<char, NUMBER_TEXT_SIZE> number{};
    // floor rounds down: a lamp just under the low mark reads 19 and not 20, so the
    // number never disagrees with the label LOW next to it.
    std::snprintf(number.data(), number.size(), "%.0f", std::floor(round.battery * PERCENT));
    const ImVec2 numberPlace{left, top + lineInset(canvas, NUMBER_FONT_SIZE)};
    drawText(canvas, numberPlace, NUMBER_FONT_SIZE, low ? color : TEXT_COLOR, number.data());
    // The percent sign is smaller and stands on the same base line as the number.
    const ImVec2 percentPlace{numberPlace.x + textWidth(canvas, NUMBER_FONT_SIZE, number.data()) +
                                  LAMP_PERCENT_GAP * scale,
                              numberPlace.y + baseLine(canvas, NUMBER_FONT_SIZE) -
                                  baseLine(canvas, SMALL_FONT_SIZE)};
    drawText(canvas, percentPlace, SMALL_FONT_SIZE, TEXT_DIM_COLOR, "%");

    const ImVec2 labelPlace{left, top + numberLine + LAMP_LINE_GAP * scale +
                                      lineInset(canvas, LABEL_FONT_SIZE)};
    drawSpacedText(canvas, labelPlace, LABEL_FONT_SIZE, low ? color : TEXT_DIM_COLOR,
                   low ? "LOW" : "LAMP");
}

// The crystal counter at the top right: the crystals collected against the number that
// opens the gate, a thin bar of all crystals of the maze with a tick at that number,
// and under it how many there are and the time of the round.
void drawCrystalCounter(const Canvas& canvas, const game::Round& round) {
    const float scale = canvas.scale;
    const ImVec2 corner = windowPoint(CRYSTALS_PLACE);
    const float right = corner.x;

    // The first row: "3" in the colour of the crystals, then " / 10".
    std::array<char, NUMBER_TEXT_SIZE> have{};
    std::array<char, NUMBER_TEXT_SIZE> need{};
    std::snprintf(have.data(), have.size(), "%d", round.collectedCount);
    std::snprintf(need.data(), need.size(), " / %d", round.requiredCount);
    const float haveWidth = textWidth(canvas, NUMBER_FONT_SIZE, have.data());
    const float needWidth = textWidth(canvas, NUMBER_FONT_SIZE, need.data());
    const float rowHeight = NUMBER_FONT_SIZE * LINE_HEIGHT * scale;
    const float textLeft = right - haveWidth - needWidth;
    const float textTop = corner.y + lineInset(canvas, NUMBER_FONT_SIZE);
    drawText(canvas, {textLeft, textTop}, NUMBER_FONT_SIZE, HUD_CRYSTAL_COLOR, have.data());
    drawText(canvas, {textLeft + haveWidth, textTop}, NUMBER_FONT_SIZE, TEXT_COLOR, need.data());

    // The little crystal in front of it: a diamond, level with the middle of the row.
    const ImVec2 gem{textLeft - GEM_GAP * scale, corner.y + rowHeight / 2.0F};
    const float halfWidth = GEM_HALF_WIDTH * scale;
    const float halfHeight = GEM_HALF_HEIGHT * scale;
    const ImVec2 gemTop{gem.x, gem.y - halfHeight};
    const ImVec2 gemRight{gem.x + halfWidth, gem.y};
    const ImVec2 gemBottom{gem.x, gem.y + halfHeight};
    const ImVec2 gemLeft{gem.x - halfWidth, gem.y};
    canvas.list->AddQuadFilled(gemTop, gemRight, gemBottom, gemLeft, ink(canvas, HUD_GEM_COLOR));
    canvas.list->AddQuad(gemTop, gemRight, gemBottom, gemLeft, ink(canvas, HUD_CRYSTAL_COLOR),
                         GEM_OUTLINE * scale);

    // The second row: the bar. Its whole width is every crystal of the maze.
    const float barLeft = right - CRYSTAL_BAR_WIDTH * scale;
    const float barTop = corner.y + rowHeight + CRYSTAL_ROW_GAP * scale;
    const float barBottom = barTop + CRYSTAL_BAR_HEIGHT * scale;
    canvas.list->AddRectFilled({barLeft, barTop}, {right, barBottom},
                               ink(canvas, TEXT_COLOR, TRACK_OPACITY));
    const int total = static_cast<int>(round.crystals.size());
    // A maze without crystals has nothing to fill the bar with and no place for a tick,
    // and it must not be divided by.
    if (total > 0) {
        const float pixelsPerCrystal = CRYSTAL_BAR_WIDTH * scale / static_cast<float>(total);
        const float filled =
            pixelsPerCrystal * static_cast<float>(std::min(round.collectedCount, total));
        canvas.list->AddRectFilled({barLeft, barTop}, {barLeft + filled, barBottom},
                                   ink(canvas, HUD_CRYSTAL_COLOR));
        // The tick where the gate opens. What is filled past it is left over.
        const float tick =
            barLeft + pixelsPerCrystal * static_cast<float>(std::min(round.requiredCount, total));
        const float overhang = CRYSTAL_TICK_OVERHANG * scale;
        canvas.list->AddRectFilled({tick, barTop - overhang},
                                   {tick + CRYSTAL_TICK_WIDTH * scale, barBottom + overhang},
                                   ink(canvas, TEXT_COLOR));
    }

    // The third row, dim: the time at the right edge, and left of it how many crystals
    // the maze has. Once the gate is open that number has done its work, and the label
    // GATE OPEN stands in its place for the rest of the round.
    const float subTop = barBottom + CRYSTAL_ROW_GAP * scale + lineInset(canvas, SMALL_FONT_SIZE);
    const std::array<char, TIME_TEXT_SIZE> time = timeText(round.elapsedSeconds);
    const float timeLeft = right - textWidth(canvas, SMALL_FONT_SIZE, time.data());
    drawText(canvas, {timeLeft, subTop}, SMALL_FONT_SIZE, TEXT_DIM_COLOR, time.data());
    const float subRight = timeLeft - CRYSTAL_SUB_GAP * scale;
    if (round.gateOpen) {
        constexpr std::string_view GATE_OPEN = "GATE OPEN";
        // The label is a little smaller than the time: both stand on one base line.
        const float labelTop =
            subTop + baseLine(canvas, SMALL_FONT_SIZE) - baseLine(canvas, LABEL_FONT_SIZE);
        drawSpacedText(canvas,
                       {subRight - spacedTextWidth(canvas, LABEL_FONT_SIZE, GATE_OPEN), labelTop},
                       LABEL_FONT_SIZE, HUD_CRYSTAL_COLOR, GATE_OPEN);
    } else {
        std::array<char, NUMBER_TEXT_SIZE> all{};
        std::snprintf(all.data(), all.size(), "of %d", total);
        drawText(canvas, {subRight - textWidth(canvas, SMALL_FONT_SIZE, all.data()), subTop},
                 SMALL_FONT_SIZE, TEXT_DIM_COLOR, all.data());
    }
}

// The stamina line at the bottom, in the middle: it gets shorter from both ends
// towards its middle, so its length is read without looking at it. Only there while
// the stamina is in use (game::staminaBarVisible). flaskFraction is how much of the
// effect of a flask is left (game::flaskEffectFraction).
void drawStaminaLine(Canvas canvas, const game::Stamina& stamina, float flaskFraction,
                     float animationSeconds) {
    if (!game::staminaBarVisible(stamina)) {
        return;
    }
    // The tea of a flask works: the stamina is full and stays full, so the line shows
    // something else, in another colour: the time that is left, running down.
    const bool tea = stamina.noDrainSecondsLeft > 0.0F;
    float fraction = stamina.level;
    ImVec4 color = HUD_STAMINA_COLOR;
    if (tea) {
        fraction = flaskFraction;
        color = HUD_FLASK_COLOR;
    } else if (stamina.winded) {
        // Winded: a dimmer colour, and the whole line pulses. The sine swings from -1
        // to 1, and the line below brings that to the range from WINDED_FAINTEST to 1.
        // The clock is the animation clock of the round, which stands still in the
        // pause. The marks at the ends pulse along: right after the line ran empty
        // there is nothing else left of it, and the player must still see that
        // something is wrong.
        const float wave =
            std::sin(animationSeconds * WINDED_PULSES_PER_SECOND * glm::two_pi<float>());
        canvas.opacity *= WINDED_FAINTEST + (1.0F - WINDED_FAINTEST) * (0.5F + 0.5F * wave);
        color = HUD_STAMINA_WINDED_COLOR;
    }
    const float scale = canvas.scale;

    const ImVec2 middle = windowPoint(STAMINA_PLACE);
    const float half = STAMINA_LINE_WIDTH * scale / 2.0F;
    const float top = middle.y;
    const float bottom = top + STAMINA_LINE_HEIGHT * scale;
    canvas.list->AddRectFilled({middle.x - half, top}, {middle.x + half, bottom},
                               ink(canvas, TEXT_COLOR, TRACK_OPACITY));

    // The two marks at the ends of a full line.
    const float mark = STAMINA_MARK_WIDTH * scale;
    const float overhang = STAMINA_MARK_OVERHANG * scale;
    const ImU32 markColor = ink(canvas, TEXT_DIM_COLOR);
    canvas.list->AddRectFilled({middle.x - half, top - overhang},
                               {middle.x - half + mark, bottom + overhang}, markColor);
    canvas.list->AddRectFilled({middle.x + half - mark, top - overhang},
                               {middle.x + half, bottom + overhang}, markColor);

    // What is left, the same length to both sides of the middle.
    const float filled = half * std::clamp(fraction, 0.0F, 1.0F);
    canvas.list->AddRectFilled({middle.x - filled, top}, {middle.x + filled, bottom},
                               ink(canvas, color));

    if (tea) {
        // The seconds beside the line. ceil rounds up, so they count 20, 19 ... 1 and
        // the line is gone at 0.
        std::array<char, NUMBER_TEXT_SIZE> seconds{};
        std::snprintf(seconds.data(), seconds.size(), "%.0f s",
                      std::ceil(stamina.noDrainSecondsLeft));
        const float lineMiddle = (top + bottom) / 2.0F;
        drawText(canvas,
                 {middle.x + half + TEA_TEXT_GAP * scale,
                  lineMiddle - fontPixels(canvas, TEA_FONT_SIZE) / 2.0F},
                 TEA_FONT_SIZE, color, seconds.data());
    }
}

// The loudness ticks under the stamina line: none lit while the player is silent, one
// while walking, all three while sprinting, and two for a moment after a pickup or
// a lever. They show Round::noiseMeter, the noise the shade was given, as
// game::noiseTicks counts it. A round without a shade has nobody who listens, so there
// the ticks are left out.
void drawNoiseTicks(const Canvas& canvas, const game::Round& round,
                    const game::GameplaySettings& settings) {
    if (!round.shade.present || round.state != game::RoundState::Playing) {
        return;
    }
    const int lit =
        game::noiseTicks(game::noiseReach(round.noiseMeter.noise, settings.shade), settings.shade);
    const float scale = canvas.scale;
    const ImVec2 bottomMiddle = windowPoint(NOISE_PLACE);
    const float step = (NOISE_TICK_WIDTH + NOISE_TICK_GAP) * scale;
    // All ticks and the gaps between them, to find the left edge of the first one.
    const float width = static_cast<float>(game::NOISE_TICK_COUNT) * step - NOISE_TICK_GAP * scale;
    float left = bottomMiddle.x - width / 2.0F;
    for (int i = 0; i < game::NOISE_TICK_COUNT; ++i) {
        const float height = NOISE_TICK_HEIGHTS.at(static_cast<std::size_t>(i)) * scale;
        const ImU32 color =
            i < lit ? ink(canvas, TEXT_COLOR) : ink(canvas, TEXT_COLOR, TRACK_OPACITY);
        canvas.list->AddRectFilled({left, bottomMiddle.y - height},
                                   {left + NOISE_TICK_WIDTH * scale, bottomMiddle.y}, color);
        left += step;
    }
}

// One sentence that tells the player what has just happened, for a few seconds: the tea
// works and for how long, the gate has opened, the battery is empty. Of several that
// would show at the same moment only the newest is drawn: there is room for one line
// between the card of a note and the prompt.
void drawHintLine(Canvas canvas, const game::Round& round, const game::Player& player) {
    if (round.state != game::RoundState::Playing) {
        return;
    }
    const game::Stamina& stamina = player.stamina;

    // The sentence that is drawn, how long ago its moment was and how much of it can
    // still be seen.
    std::array<char, HINT_TEXT_SIZE> text{};
    ImVec4 color = TEXT_COLOR;
    float newest = FLT_MAX;
    float opacity = 0.0F;
    // Takes a sentence when it can still be seen and is newer than the one so far. True
    // when it was taken: the caller then writes its words into text.
    const auto offer = [&](float seconds, float holdSeconds, const ImVec4& hintColor) {
        const float hintOpacityNow = hintOpacity(seconds, holdSeconds, HINT_FADE_SECONDS);
        if (hintOpacityNow <= 0.0F || seconds >= newest) {
            return false;
        }
        newest = seconds;
        opacity = hintOpacityNow;
        color = hintColor;
        return true;
    };

    // The tea counts down from the length of its effect, so what is gone of that length
    // is the time since the flask was drunk.
    const float sinceDrink =
        std::max(player.staminaSettings.flaskSeconds - stamina.noDrainSecondsLeft, 0.0F);
    if (stamina.noDrainSecondsLeft > 0.0F && offer(sinceDrink, TEA_HINT_SECONDS, HUD_FLASK_COLOR)) {
        // ceil rounds up, so the sentence counts 20, 19 ... like the seconds beside the
        // line.
        std::snprintf(text.data(), text.size(), "Warm tea. Sprinting costs nothing for %.0f s.",
                      std::ceil(stamina.noDrainSecondsLeft));
    }
    if (round.gateOpen && offer(round.gateOpenSeconds, GATE_HINT_SECONDS, HUD_CRYSTAL_COLOR)) {
        std::snprintf(text.data(), text.size(), "The gate is open. Find the exit.");
    }
    if (round.battery <= 0.0F &&
        offer(round.batteryEmptySeconds, BATTERY_HINT_SECONDS, HUD_BATTERY_LOW_COLOR)) {
        std::snprintf(text.data(), text.size(), "Battery empty. Find a crystal.");
    }
    if (opacity <= 0.0F) {
        return;
    }

    canvas.opacity *= opacity;
    const ImVec2 middle = windowPoint(HINT_PLACE);
    drawText(canvas,
             {middle.x - textWidth(canvas, HUD_FONT_SIZE, text.data()) / 2.0F,
              middle.y - fontPixels(canvas, HUD_FONT_SIZE) / 2.0F},
             HUD_FONT_SIZE, color, text.data());
}

// The name of the night at the top left, for the first seconds of a night of the
// campaign: "NIGHT 2" as a small label and its title under it. playedNight is the night
// in play, or 0 for free play, which has no name.
void drawNightName(Canvas canvas, const game::Round& round, int playedNight) {
    // A round that started again after a catch is not the start of the night: it shows
    // the caught line, and the name stays away (NIGHT_NAME_SECONDS).
    if (playedNight == 0 || round.caughtLine != game::NO_CAUGHT_LINE) {
        return;
    }
    // The time of the round stands still under the title card of the night, so it is
    // the time since that card left.
    canvas.opacity *=
        hintOpacity(round.elapsedSeconds, NIGHT_NAME_SECONDS, NIGHT_NAME_FADE_SECONDS);
    if (canvas.opacity <= 0.0F) {
        return;
    }

    // "Night 2" in capital letters, like the other small labels. toupper wants its
    // letter as an unsigned char.
    std::string label = game::campaignNightLabel(playedNight);
    for (char& letter : label) {
        letter = static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
    }

    const ImVec2 corner = windowPoint(NIGHT_NAME_PLACE);
    drawSpacedText(canvas, {corner.x, corner.y + lineInset(canvas, LABEL_FONT_SIZE)},
                   LABEL_FONT_SIZE, TEXT_DIM_COLOR, label);
    const float titleTop =
        corner.y + (LABEL_FONT_SIZE * LINE_HEIGHT + NIGHT_NAME_LINE_GAP) * canvas.scale;
    drawText(canvas, {corner.x, titleTop + lineInset(canvas, NUMBER_FONT_SIZE)}, NUMBER_FONT_SIZE,
             HUD_BATTERY_COLOR, game::campaignNight(playedNight).title);
}

// The card in the middle of the window, shown once the round is won. fontSize is the
// size the text of the HUD was pushed with (drawHud).
void drawWinCard(const game::Round& round, const game::KeyBindings& keys, float scale,
                 float fontSize) {
    ImGui::SetNextWindowPos(windowPoint(CENTER), ImGuiCond_Always, CENTER);
    ImGui::SetNextWindowBgAlpha(CARD_OPACITY);
    // PushStyleVar changes a metric of the style until the matching PopStyleVar. It
    // has to be set before Begin, which reads the padding.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        {CARD_PADDING.x * scale, CARD_PADDING.y * scale});

    if (ImGui::Begin("You escaped", nullptr, CARD_WINDOW_FLAGS)) {
        // The same font, larger than the rest of the HUD.
        ImGui::PushFont(nullptr, fontSize * CARD_TITLE_SCALE);
        ImGui::TextColored(HUD_CRYSTAL_COLOR, "You escaped");
        ImGui::PopFont();

        ImGui::Separator();
        ImGui::Text("Time: %s", timeText(round.elapsedSeconds).data());
        ImGui::Text("Crystals: %d of %d", round.collectedCount,
                    static_cast<int>(round.crystals.size()));
        ImGui::Spacing();
        // "%s" and the name as an argument: the name itself is never read as a format.
        ImGui::TextColored(HUD_BATTERY_COLOR, "%s: play again",
                           game::boundKeyName(keys, game::KeyAction::Restart).c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// True when the interaction key would use the thing the ray points at: pull that lever
// or read that note. (Closing an open card needs no aim, so it does not count here.)
bool pointsAtSomethingUsable(const game::PickState& pick) {
    return pick.action == game::Interaction::PullLever ||
           pick.action == game::Interaction::ReadNote;
}

// The crosshair in the middle of the window: the point the picking ray goes through
// while the cursor is captured. With a free cursor the cursor itself shows that point,
// so nothing is drawn.
void drawCrosshair(const Canvas& canvas, const game::PickState& pick) {
    if (!pick.hasRay || !pick.centered) {
        return;
    }
    const ImVec2 center = windowPoint(CENTER);
    const float scale = canvas.scale;

    if (pointsAtSomethingUsable(pick)) {
        const ImU32 color = ink(canvas, HUD_CROSSHAIR_ACTIVE_COLOR);
        canvas.list->AddCircleFilled(center, CROSSHAIR_DOT_RADIUS * scale, color);
        // The fourth argument is the number of straight pieces: 0 lets ImGui choose.
        canvas.list->AddCircle(center, CROSSHAIR_RING_RADIUS * scale, color, 0,
                               CROSSHAIR_RING_THICKNESS * scale);
    } else {
        canvas.list->AddCircleFilled(center, CROSSHAIR_DOT_RADIUS * scale,
                                     ink(canvas, HUD_CROSSHAIR_COLOR));
    }
}

// The prompt near the bottom of the window, while there is something to use: the name
// of the key in a small box (a key cap), then what the key does. The key is read first,
// because it is what the hand needs.
void drawPrompt(const Canvas& canvas, const game::PickState& pick, const game::KeyBindings& keys) {
    if (!pointsAtSomethingUsable(pick)) {
        return;
    }
    const float scale = canvas.scale;
    const std::string key = game::boundKeyName(keys, game::KeyAction::Use);
    const std::string_view words = game::interactionWords(pick.action);

    // The sizes of the parts, from the inside out: the key cap, the words, the pill
    // around both.
    const float keyWidth = textWidth(canvas, HUD_FONT_SIZE, key);
    const float capHeight = KEY_CAP_SIZE * scale;
    const float capWidth = std::max(capHeight, keyWidth + 2.0F * KEY_CAP_PADDING * scale);
    const float wordsWidth = textWidth(canvas, HUD_FONT_SIZE, words);
    const float width =
        capWidth + KEY_CAP_GAP * scale + wordsWidth + 2.0F * PROMPT_PADDING.x * scale;
    const float height = capHeight + 2.0F * PROMPT_PADDING.y * scale;

    const ImVec2 middle = windowPoint(PROMPT_PLACE);
    const ImVec2 pillMin{std::floor(middle.x - width / 2.0F), std::floor(middle.y - height / 2.0F)};
    const ImVec2 pillMax{pillMin.x + width, pillMin.y + height};
    // The fourth argument is how round the corners are. The pill is filled first and
    // then outlined.
    canvas.list->AddRectFilled(pillMin, pillMax, ink(canvas, HUD_SHADOW_COLOR, PROMPT_PILL_OPACITY),
                               PROMPT_ROUNDING * scale);
    canvas.list->AddRect(pillMin, pillMax, ink(canvas, LINE_COLOR), PROMPT_ROUNDING * scale,
                         PROMPT_OUTLINE * scale);

    const ImVec4& color = HUD_CROSSHAIR_ACTIVE_COLOR;
    const ImVec2 capMin{pillMin.x + PROMPT_PADDING.x * scale, pillMin.y + PROMPT_PADDING.y * scale};
    const ImVec2 capMax{capMin.x + capWidth, capMin.y + capHeight};
    canvas.list->AddRect(capMin, capMax, ink(canvas, color), KEY_CAP_ROUNDING * scale,
                         KEY_CAP_OUTLINE * scale);
    // Both texts are as high as the font and stand in the middle of the height of the
    // key cap. The name of the key is also in the middle of its width.
    const float textTop = capMin.y + (capHeight - fontPixels(canvas, HUD_FONT_SIZE)) / 2.0F;
    drawText(canvas, {capMin.x + (capWidth - keyWidth) / 2.0F, textTop}, HUD_FONT_SIZE, color, key);
    drawText(canvas, {capMax.x + KEY_CAP_GAP * scale, textTop}, HUD_FONT_SIZE, color, words);
}

// The card of the note that is being read: its text and the key that closes it.
void drawNoteCard(const game::MazeWorld& world, const game::Round& round,
                  const game::KeyBindings& keys, float scale) {
    // The text is asked for in every frame: a hint towards a crystal changes when that
    // crystal is collected.
    const std::string text = game::openNoteText(world, round);

    ImGui::SetNextWindowPos(windowPoint(BELOW_CENTER), ImGuiCond_Always, CENTER);
    ImGui::SetNextWindowBgAlpha(CARD_OPACITY);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        {CARD_PADDING.x * scale, CARD_PADDING.y * scale});

    if (ImGui::Begin("Note", nullptr, CARD_WINDOW_FLAGS)) {
        ImGui::TextColored(HUD_NOTE_COLOR, "Chalk on the stone");
        ImGui::Separator();
        // "%s" and the text as an argument: the text itself is never read as a format.
        ImGui::Text("%s", text.c_str());
        ImGui::Spacing();
        const std::string close = game::interactionPrompt(
            game::Interaction::CloseNote, game::boundKeyName(keys, game::KeyAction::Use));
        ImGui::TextColored(HUD_BATTERY_COLOR, "%s", close.c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// The line the player reads after the shade carried them back (game::caughtLine).
void drawCaughtLine(const game::Round& round) {
    if (round.caughtLine == game::NO_CAUGHT_LINE) {
        return;
    }
    ImGui::SetNextWindowPos(windowPoint(CAUGHT_LINE_PLACE), ImGuiCond_Always, CENTER);
    ImGui::SetNextWindowBgAlpha(CAUGHT_LINE_OPACITY);

    if (ImGui::Begin("Caught line", nullptr, STATUS_WINDOW_FLAGS)) {
        const std::string_view line = game::caughtLine(round.caughtLine);
        // A string_view has no closing zero: "%.*s" takes the length and the letters.
        ImGui::TextColored(HUD_NOTE_COLOR, "%.*s", static_cast<int>(line.size()), line.data());
    }
    ImGui::End();
}

} // namespace

void drawHud(const game::MazeWorld& world, const game::Round& round,
             const game::GameplaySettings& settings, const game::Player& player,
             const game::PickState& pick, bool mapOnScreen, const game::KeyBindings& keys,
             int playedNight) {
    // One scale for every size and every distance of the HUD: it grows with the window
    // (debug::hudScale). FontScaleDpi is the scaling of the display, set by applyTheme.
    const float displayScale = ImGui::GetStyle().FontScaleDpi;
    const float scale = hudScale(displayScale, ImGui::GetMainViewport()->WorkSize.y);

    // The cards and the line after a catch are ImGui windows, so their text needs
    // a font of the size of the HUD. nullptr keeps the font of the debug window. ImGui
    // multiplies the size it is given by FontScaleDpi itself, so the size is divided
    // by it first: what comes out is CARD_FONT_SIZE times the scale of the HUD. Since
    // ImGui 1.92 a font is drawn into the font texture in every size it is used in, so
    // the letters stay sharp in a window of any height.
    const float fontSize = CARD_FONT_SIZE * scale / displayScale;
    ImGui::PushFont(nullptr, fontSize);

    // Everything else is drawn by hand, as shapes and text on the background draw list.
    const Canvas canvas{.list = ImGui::GetBackgroundDrawList(),
                        .font = ImGui::GetFont(),
                        .scale = scale,
                        .opacity = 1.0F};
    drawLampGauge(canvas, round, settings);
    drawCrystalCounter(canvas, round);
    drawStaminaLine(canvas, player.stamina,
                    game::flaskEffectFraction(player.stamina, player.staminaSettings),
                    round.animationSeconds);
    drawNoiseTicks(canvas, round, settings);
    drawNightName(canvas, round, playedNight);
    // A sentence waits while the map is shown: its place is where the bottom edge of
    // the map is. Its time goes on, so a sentence whose time ran out under the map is
    // not shown late.
    if (!mapOnScreen) {
        drawHintLine(canvas, round, player);
    }
    drawCrosshair(canvas, pick);
    drawPrompt(canvas, pick, keys);

    if (round.noteOpen) {
        drawNoteCard(world, round, keys, scale);
    }
    drawCaughtLine(round);
    if (round.state == game::RoundState::Won) {
        drawWinCard(round, keys, scale, fontSize);
    }

    ImGui::PopFont();
}

} // namespace debug
