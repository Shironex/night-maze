// Game HUD: the crystal counter, the battery bar, hints and the "You escaped" card.
// See docs/modules/game/gameplay.md
#include "debug/Hud.hpp"

#include "debug/PanelLayout.hpp"
#include "debug/Theme.hpp"
#include "game/Round.hpp"

#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdio>

namespace debug {

namespace {

// Sizes in pixels at 100 % display scaling. They are multiplied by the display scale
// (ImGuiStyle::FontScaleDpi, set by applyTheme), like the sizes of the panels.

// The HUD stands below the rows of title bars of the panels that start folded at the top
// edge (Camera and Gameplay, Terrain and Grass). This is the free space above the first
// row plus the extra space between the last row and the HUD, which makes the HUD read as
// a thing of its own: two panel gaps.
constexpr float HUD_TOP_OFFSET = 2.0F * PANEL_GAP;

// Width of the battery bar, which is also what makes the HUD as wide as it is.
constexpr float BATTERY_BAR_WIDTH = 230.0F;

// How much of the scene shows through the HUD and through the card. The card hides
// more: it is meant to be read, and the round behind it is over.
constexpr float HUD_OPACITY = 0.72F;
constexpr float CARD_OPACITY = 0.9F;

// The title of the card is drawn this much larger than the other text.
constexpr float CARD_TITLE_SCALE = 1.8F;

// Free space around the text of the card, wider than in a panel.
constexpr ImVec2 CARD_PADDING{28.0F, 20.0F};

// Places on the screen as parts of the window: x is 0 at the left edge and 1 at the
// right edge, y is 0 at the top edge and 1 at the bottom edge.
constexpr float MIDDLE = 0.5F;
constexpr ImVec2 TOP_CENTER{MIDDLE, 0.0F};
constexpr ImVec2 CENTER{MIDDLE, MIDDLE};

constexpr int SECONDS_PER_MINUTE = 60;

// Room for a time written as text: minutes, a colon, two digits of seconds and the
// closing zero. Far more than a round can last.
constexpr std::size_t TIME_TEXT_SIZE = 16;

// The flags of both HUD windows. Together they make a window that is only a picture:
//   NoDecoration        no title bar, no resize grip, no scrollbar
//   AlwaysAutoResize    exactly as large as its contents, in every frame
//   NoInputs            the mouse passes through it, it is never hovered or clicked
//   NoNav               the keyboard navigation of ImGui skips it
//   NoFocusOnAppearing  appearing does not take the focus from a panel
//   NoSavedSettings     nothing about it is written to imgui.ini
//   NoDocking           it cannot be docked into the dock area
//   NoMove              it stays where the code puts it
constexpr ImGuiWindowFlags PICTURE_WINDOW_FLAGS =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;

// The strip at the top also stays behind the debug panels (NoBringToFrontOnFocus): a
// panel that is unfolded over it is being worked with, and the strip must not cover
// its widgets. The card has no such flag: ImGui puts a new window in front of the ones
// that are already there, so the card starts on top of the panels. A panel that is
// clicked afterwards comes in front of it, like in front of any other window.
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

// The bar of the battery: amber while there is charge, red once it is low.
void drawBatteryBar(const game::Round& round, const game::GameplaySettings& settings, float scale) {
    const bool low = round.battery < settings.lowBatteryThreshold;
    // A progress bar is drawn in the colour ImGui calls PlotHistogram. PushStyleColor
    // changes a colour of the style until the matching PopStyleColor.
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, low ? HUD_BATTERY_LOW_COLOR : HUD_BATTERY_COLOR);
    // The second argument is the size: a height of 0 means the height of a line of
    // text. The third is the text written on the bar: none, the label next to it says
    // the number.
    ImGui::ProgressBar(round.battery, {BATTERY_BAR_WIDTH * scale, 0.0F}, "");
    ImGui::PopStyleColor();

    constexpr float PERCENT = 100.0F;
    ImGui::SameLine();
    ImGui::Text("%.0f%%", round.battery * PERCENT);
}

// Up to two lines that tell the player what to do next, or nothing.
void drawHint(const game::Round& round) {
    if (round.state != game::RoundState::Playing) {
        return;
    }
    if (round.gateOpen) {
        ImGui::TextColored(HUD_CRYSTAL_COLOR, "The gate is open. Find the exit.");
    }
    if (round.battery <= 0.0F) {
        ImGui::TextColored(HUD_BATTERY_LOW_COLOR, "Battery empty. Find a crystal.");
    }
}

// The strip at the top of the window.
void drawStatus(const game::Round& round, const game::GameplaySettings& settings, float scale) {
    // The third argument is the pivot: the point of the HUD that is put at the given
    // position. (0.5, 0) is the middle of its top edge, so the HUD is centred whatever
    // its width turns out to be.
    const ImVec2 top = windowPoint(TOP_CENTER);
    // The rows of title bars are measured with the real height of a bar, which follows
    // the font (foldedRowsHeight).
    const float rowsAbove = foldedRowsHeight(FOLDED_ROW_COUNT, scale);
    ImGui::SetNextWindowPos({top.x, top.y + rowsAbove + HUD_TOP_OFFSET * scale}, ImGuiCond_Always,
                            TOP_CENTER);
    ImGui::SetNextWindowBgAlpha(HUD_OPACITY);

    // The name is never shown (there is no title bar). ImGui tells windows apart by it.
    if (ImGui::Begin("Game HUD", nullptr, STATUS_WINDOW_FLAGS)) {
        ImGui::TextColored(HUD_CRYSTAL_COLOR, "Crystals");
        ImGui::SameLine();
        ImGui::Text("%d / %d", round.collectedCount, round.requiredCount);
        ImGui::SameLine();
        ImGui::TextDisabled("(of %d)", static_cast<int>(round.crystals.size()));
        ImGui::SameLine();
        ImGui::TextDisabled("  %s", timeText(round.elapsedSeconds).data());

        drawBatteryBar(round, settings, scale);
        drawHint(round);
    }
    ImGui::End();
}

// The card in the middle of the window, shown once the round is won.
void drawWinCard(const game::Round& round, float scale) {
    ImGui::SetNextWindowPos(windowPoint(CENTER), ImGuiCond_Always, CENTER);
    ImGui::SetNextWindowBgAlpha(CARD_OPACITY);
    // PushStyleVar changes a metric of the style until the matching PopStyleVar. It
    // has to be set before Begin, which reads the padding.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        {CARD_PADDING.x * scale, CARD_PADDING.y * scale});

    if (ImGui::Begin("You escaped", nullptr, CARD_WINDOW_FLAGS)) {
        // The same font, larger. The size is given without the display scale: ImGui
        // multiplies it by FontScaleDpi itself.
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * CARD_TITLE_SCALE);
        ImGui::TextColored(HUD_CRYSTAL_COLOR, "You escaped");
        ImGui::PopFont();

        ImGui::Separator();
        ImGui::Text("Time: %s", timeText(round.elapsedSeconds).data());
        ImGui::Text("Crystals: %d of %d", round.collectedCount,
                    static_cast<int>(round.crystals.size()));
        ImGui::Spacing();
        ImGui::TextColored(HUD_BATTERY_COLOR, "R: play again");
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace

void drawHud(const game::Round& round, const game::GameplaySettings& settings) {
    const float scale = ImGui::GetStyle().FontScaleDpi;

    drawStatus(round, settings, scale);
    if (round.state == game::RoundState::Won) {
        drawWinCard(round, scale);
    }
}

} // namespace debug
