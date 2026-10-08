// Game HUD: the crystal counter, the battery bar, the stamina bar, hints, the crosshair
// with its prompt, the card of a note and the "You escaped" card.
// See docs/modules/game/gameplay.md
#include "debug/Hud.hpp"

#include "debug/Theme.hpp"
#include "game/Interaction.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"

#include <glm/gtc/constants.hpp>

#include <imgui.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace debug {

namespace {

// Sizes in pixels at 100 % display scaling. They are multiplied by the display scale
// (ImGuiStyle::FontScaleDpi, set by applyTheme), like the sizes of the debug window.

// The distance of the strip from the top edge of the window. It is the same whether the
// debug UI is shown or hidden: the debug window starts below the strip
// (hudReservedHeight), so nothing has to make room for the other.
constexpr float HUD_TOP_OFFSET = 16.0F;

// Height of the text of the HUD in pixels. The HUD is read while playing, from further
// away than the debug window is worked with, so it keeps a larger text than FONT_SIZE
// of the theme.
constexpr float HUD_FONT_SIZE = 16.0F;

// Width of the battery bar, which is also what makes the HUD as wide as it is.
constexpr float BATTERY_BAR_WIDTH = 230.0F;

// Height of the stamina bar: a thin line under the battery bar, as wide as it. The two
// bars differ in height and in colour, so they are told apart at a glance.
constexpr float STAMINA_BAR_HEIGHT = 5.0F;

// How many times per second the stamina bar of a winded player goes faint and back,
// and its opacity at the faintest moment (1 hides what is behind, 0 is invisible).
constexpr float WINDED_PULSES_PER_SECOND = 2.0F;
constexpr float WINDED_FAINTEST = 0.3F;

// The empty part of the bar of a winded player has this share of the opacity of the
// filled part, so the two stay apart while both pulse.
constexpr float WINDED_TRACK_OPACITY = 0.4F;

// How much of the scene shows through the HUD and through the card. The card hides
// more: it is meant to be read, and the round behind it is over.
constexpr float HUD_OPACITY = 0.72F;
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

// The prompt ("E: pull lever") stands near the bottom edge of the window, in the
// middle. The thing the player points at is around the crosshair, so down there the
// prompt never covers it. It is also below the card of a note (BELOW_CENTER). While
// the map is held nothing can be used, so the prompt and the map never meet.
constexpr ImVec2 PROMPT_PLACE{MIDDLE, 0.9F};

// How much of the scene shows through the prompt: more than through the HUD, so the
// ground in front of the player stays visible behind it.
constexpr float PROMPT_OPACITY = 0.55F;

constexpr int SECONDS_PER_MINUTE = 60;

// Room for a time written as text: minutes, a colon, two digits of seconds and the
// closing zero. Far more than a round can last.
constexpr std::size_t TIME_TEXT_SIZE = 16;

// The flags of both HUD windows. Together they make a window that is only a picture:
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

// The strip at the top also stays behind the debug UI (NoBringToFrontOnFocus): a pinned
// panel that is dragged over it is being worked with, and the strip must not cover its
// widgets. The card has no such flag: ImGui puts a new window in front of the ones that
// are already there, so the card starts on top of the debug window. A window that is
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

// The bar of the stamina, or the empty room where it would be. flaskFraction is how much
// of the effect of a flask is left (game::flaskEffectFraction).
void drawStaminaBar(const game::Stamina& stamina, float flaskFraction, float animationSeconds,
                    float scale) {
    const ImVec2 size{BATTERY_BAR_WIDTH * scale, STAMINA_BAR_HEIGHT * scale};
    // The tea of a flask works: the stamina is full and stays full, so the bar shows
    // something else, in another colour: the time that is left, running down.
    if (stamina.noDrainSecondsLeft > 0.0F) {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, HUD_FLASK_COLOR);
        ImGui::ProgressBar(flaskFraction, size, "");
        ImGui::PopStyleColor();
        return;
    }
    // A full bar that nobody uses is hidden. Dummy takes the same room without drawing
    // anything, so the strip keeps its height and the hints below do not jump.
    if (!game::staminaBarVisible(stamina)) {
        ImGui::Dummy(size);
        return;
    }

    // FrameBg is the colour of the empty part of a progress bar, its track.
    ImVec4 color = HUD_STAMINA_COLOR;
    ImVec4 track = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
    if (stamina.winded) {
        // Winded: a dimmer colour whose opacity pulses. The sine swings from -1 to 1,
        // and the line below brings that to the range from WINDED_FAINTEST to 1. The
        // clock is the animation clock of the round, which stands still in the pause.
        const float wave =
            std::sin(animationSeconds * WINDED_PULSES_PER_SECOND * glm::two_pi<float>());
        const float pulse = WINDED_FAINTEST + (1.0F - WINDED_FAINTEST) * (0.5F + 0.5F * wave);
        color = HUD_STAMINA_WINDED_COLOR;
        color.w = pulse;
        // The track pulses along, fainter: right after the bar ran empty there is no
        // filled part, and the player must still see that something is wrong.
        track = HUD_STAMINA_WINDED_COLOR;
        track.w = pulse * WINDED_TRACK_OPACITY;
    }
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, track);
    ImGui::ProgressBar(stamina.level, size, "");
    // The argument is how many colours are given back.
    ImGui::PopStyleColor(2);
}

// Up to three lines that tell the player what is going on, or nothing.
void drawHint(const game::Round& round, const game::Stamina& stamina) {
    if (round.state != game::RoundState::Playing) {
        return;
    }
    // The tea, first: it explains the bar right above it. ceil rounds up, so the line
    // counts 20, 19 ... 1 and is gone at 0.
    if (stamina.noDrainSecondsLeft > 0.0F) {
        ImGui::TextColored(HUD_FLASK_COLOR, "Warm tea. Sprinting costs nothing for %.0f s.",
                           std::ceil(stamina.noDrainSecondsLeft));
    }
    if (round.gateOpen) {
        ImGui::TextColored(HUD_CRYSTAL_COLOR, "The gate is open. Find the exit.");
    }
    if (round.battery <= 0.0F) {
        ImGui::TextColored(HUD_BATTERY_LOW_COLOR, "Battery empty. Find a crystal.");
    }
}

// The strip at the top of the window.
void drawStatus(const game::Round& round, const game::GameplaySettings& settings,
                const game::Player& player, bool mapOnScreen, float scale) {
    // The third argument is the pivot: the point of the HUD that is put at the given
    // position. (0.5, 0) is the middle of its top edge, so the HUD is centred whatever
    // its width turns out to be.
    const ImVec2 top = windowPoint(TOP_CENTER);
    ImGui::SetNextWindowPos({top.x, top.y + HUD_TOP_OFFSET * scale}, ImGuiCond_Always, TOP_CENTER);
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
        drawStaminaBar(player.stamina,
                       game::flaskEffectFraction(player.stamina, player.staminaSettings),
                       round.animationSeconds, scale);
        // The hints are left out while the map is shown: the strip would grow down into
        // the top of the map, which is centred in the window.
        if (!mapOnScreen) {
            drawHint(round, player.stamina);
        }
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
        // The same font, larger than the rest of the HUD.
        ImGui::PushFont(nullptr, HUD_FONT_SIZE * CARD_TITLE_SCALE);
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

// True when the interaction key would use the thing the ray points at: pull that lever
// or read that note. (Closing an open card needs no aim, so it does not count here.)
bool pointsAtSomethingUsable(const game::PickState& pick) {
    return pick.action == game::Interaction::PullLever ||
           pick.action == game::Interaction::ReadNote;
}

// The crosshair in the middle of the window: the point the picking ray goes through
// while the cursor is captured. With a free cursor the cursor itself shows that point,
// so nothing is drawn.
void drawCrosshair(const game::PickState& pick, float scale) {
    if (!pick.hasRay || !pick.centered) {
        return;
    }
    // The background draw list takes shapes in screen coordinates and draws them
    // behind every ImGui window, so the crosshair never covers the debug window or
    // a card.
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 center = windowPoint(CENTER);

    if (pointsAtSomethingUsable(pick)) {
        const ImU32 color = ImGui::GetColorU32(HUD_CROSSHAIR_ACTIVE_COLOR);
        drawList->AddCircleFilled(center, CROSSHAIR_DOT_RADIUS * scale, color);
        // The fourth argument is the number of straight pieces: 0 lets ImGui choose.
        drawList->AddCircle(center, CROSSHAIR_RING_RADIUS * scale, color, 0,
                            CROSSHAIR_RING_THICKNESS * scale);
    } else {
        drawList->AddCircleFilled(center, CROSSHAIR_DOT_RADIUS * scale,
                                  ImGui::GetColorU32(HUD_CROSSHAIR_COLOR));
    }
}

// The line near the bottom of the window that names the key, while there is something
// to use.
void drawPrompt(const game::PickState& pick) {
    if (!pointsAtSomethingUsable(pick)) {
        return;
    }
    ImGui::SetNextWindowPos(windowPoint(PROMPT_PLACE), ImGuiCond_Always, CENTER);
    ImGui::SetNextWindowBgAlpha(PROMPT_OPACITY);

    if (ImGui::Begin("Interaction prompt", nullptr, STATUS_WINDOW_FLAGS)) {
        ImGui::TextColored(HUD_CROSSHAIR_ACTIVE_COLOR, "%s", game::interactionPrompt(pick.action));
    }
    ImGui::End();
}

// The card of the note that is being read: its text and the key that closes it.
void drawNoteCard(const game::MazeWorld& world, const game::Round& round, float scale) {
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
        ImGui::TextColored(HUD_BATTERY_COLOR, "%s",
                           game::interactionPrompt(game::Interaction::CloseNote));
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
    ImGui::SetNextWindowBgAlpha(PROMPT_OPACITY);

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
             const game::PickState& pick, bool mapOnScreen) {
    const float scale = ImGui::GetStyle().FontScaleDpi;

    // The same font as the debug window, in the size of the HUD, for everything below.
    // nullptr keeps the font. The size is given without the display scale: ImGui
    // multiplies it by FontScaleDpi itself.
    ImGui::PushFont(nullptr, HUD_FONT_SIZE);

    drawStatus(round, settings, player, mapOnScreen, scale);
    drawCrosshair(pick, scale);
    drawPrompt(pick);
    if (round.noteOpen) {
        drawNoteCard(world, round, scale);
    }
    drawCaughtLine(round);
    if (round.state == game::RoundState::Won) {
        drawWinCard(round, scale);
    }

    ImGui::PopFont();
}

float hudReservedHeight() {
    const float scale = ImGui::GetStyle().FontScaleDpi;
    const ImGuiStyle& style = ImGui::GetStyle();

    // The strip is a window with six lines in it: the counter, the battery bar, the
    // stamina bar and three hints. A line of text is as high as the font, and the battery
    // bar is a widget: the font plus the frame padding above and below. The stamina bar
    // has a height of its own, and keeps its room also while it is hidden.
    constexpr float TEXT_LINE_COUNT = 4.0F;
    constexpr float GAP_COUNT = 5.0F;
    const float textLine = HUD_FONT_SIZE * scale;
    const float bar = textLine + 2.0F * style.FramePadding.y;
    const float strip = 2.0F * style.WindowPadding.y + TEXT_LINE_COUNT * textLine + bar +
                        STAMINA_BAR_HEIGHT * scale + GAP_COUNT * style.ItemSpacing.y;
    return HUD_TOP_OFFSET * scale + strip;
}

} // namespace debug
