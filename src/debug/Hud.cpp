// Game HUD: drawHud, which puts the parts of the HUD together and gives them the canvas.
#include "debug/Hud.hpp"

#include "debug/HudRules.hpp"
#include "debug/HudShared.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"

#include <imgui.h>

#include <cstdint>

namespace debug {

namespace {

// Height of the text of the cards and of the line after a catch, the way ImGui
// measures a text: the height of its line.
constexpr float CARD_FONT_SIZE = 16.0F;

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
