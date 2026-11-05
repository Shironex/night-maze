// What more than one of the Hud*.cpp files uses: the canvas the HUD draws on, its sizes and the
// helpers that measure and draw text, and the parts of the HUD that drawHud puts together.
#pragma once

#include "game/Interaction.hpp"
#include "game/KeyBindings.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"

#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace debug {

// Every size below is a number of pixels in a window 720 pixels high
// (HUD_REFERENCE_HEIGHT). It is multiplied by the scale of the HUD (debug::hudScale), so
// the HUD takes the same part of a window of any size. Places are parts of the window.

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

// Lines of text that stand above each other are this many em apart, measured from top
// to top.
constexpr float LINE_HEIGHT = 1.2F;

// Room for a time written as text: minutes, a colon, two digits of seconds and the
// closing zero. Far more than a round can last.
constexpr std::size_t TIME_TEXT_SIZE = 16;

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
inline ImU32 ink(const Canvas& canvas, const ImVec4& color, float opacity = 1.0F) {
    return ImGui::ColorConvertFloat4ToU32(
        {color.x, color.y, color.z, color.w * opacity * canvas.opacity});
}

/// A time in seconds as text, minutes and seconds ("1:35").
std::array<char, TIME_TEXT_SIZE> timeText(float seconds);
/// The point of the game window that lies at a part of it (0 to 1 in x and y).
ImVec2 windowPoint(const ImVec2& part);
/// The height of a line of text of a size in em on the screen, in whole pixels.
float fontPixels(const Canvas& canvas, float size);
/// How wide a text of a size in em is on the screen, in pixels.
float textWidth(const Canvas& canvas, float size, std::string_view text);
/// How far below the top of a text its base line stands, in pixels.
float baseLine(const Canvas& canvas, float size);
/// How far below the top of its line a text starts, in pixels.
float lineInset(const Canvas& canvas, float size);
/// Draws a text with its top left corner at position, with its dark copy below it.
void drawText(const Canvas& canvas, const ImVec2& position, float size, const ImVec4& color,
              std::string_view text);
/// How wide a label with extra room between its letters is, in pixels.
float spacedTextWidth(const Canvas& canvas, float size, std::string_view text);
/// Draws such a label with its top left corner at position.
void drawSpacedText(const Canvas& canvas, ImVec2 position, float size, const ImVec4& color,
                    std::string_view text);

/// The lamp gauge at the bottom left.
void drawLampGauge(Canvas canvas, const game::Round& round, const game::GameplaySettings& settings);
/// The crystal counter at the top right.
void drawCrystalCounter(const Canvas& canvas, const game::Round& round);
/// The stamina line at the bottom, in the middle.
void drawStaminaLine(Canvas canvas, const game::Stamina& stamina, float flaskFraction,
                     float heavyFraction, float animationSeconds);
/// The loudness ticks under the stamina line.
void drawNoiseTicks(const Canvas& canvas, const game::Round& round,
                    const game::GameplaySettings& settings);
/// The sentence that tells the player what has just happened.
void drawHintLine(Canvas canvas, const game::Round& round, const game::Player& player);
/// The name of the night at the top left.
void drawNightName(Canvas canvas, const game::Round& round, int playedNight,
                   std::uint32_t playedDaily);

/// The card in the middle of the window, shown once the round is won.
void drawWinCard(const game::Round& round, const game::KeyBindings& keys, float scale,
                 float fontSize);
/// The crosshair in the middle of the window.
void drawCrosshair(const Canvas& canvas, const game::PickState& pick);
/// The prompt near the bottom of the window, while there is something to use.
void drawPrompt(const Canvas& canvas, const game::PickState& pick, const game::KeyBindings& keys);
/// The card of the note that is being read.
void drawNoteCard(const game::MazeWorld& world, const game::Round& round,
                  const game::KeyBindings& keys, float scale);
/// The line the player reads after the shade carried them back.
void drawCaughtLine(const game::Round& round);

} // namespace debug
