// Game HUD: the lamp gauge, the crystal counter, the stamina line, the loudness ticks,
// sentences that come and go, the crosshair with its prompt, the name of the night, the
// card of a note and the "You escaped" card.
#include "debug/Hud.hpp"

#include "debug/HudRules.hpp"
#include "debug/HudShared.hpp"
#include "debug/Theme.hpp"
#include "game/Campaign.hpp"
#include "game/Daily.hpp"
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
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace debug {

namespace {

// Height of the text of the cards and of the line after a catch, the way ImGui
// measures a text: the height of its line.
constexpr float CARD_FONT_SIZE = 16.0F;

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
        ImGui::Text("Crystals: %d of %d", round.collectedCount, game::crystalTotal(round));
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
             int playedNight, std::uint32_t playedDaily) {
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
    drawStaminaLine(
        canvas, player.stamina, game::flaskEffectFraction(player.stamina, player.staminaSettings),
        game::heavyFraction(player.stamina, player.staminaSettings), round.animationSeconds);
    drawNoiseTicks(canvas, round, settings);
    drawNightName(canvas, round, playedNight, playedDaily);
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
