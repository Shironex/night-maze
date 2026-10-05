// "Gameplay" debug panel: the state of the round, the battery and the numbers of the rules.
// See docs/modules/game/gameplay.md
#pragma once

namespace game {
struct GameplaySettings;
struct Round;
} // namespace game

namespace debug {

/// Draws the "Gameplay" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// round is editable in one thing: the charge of the battery, so the flicker of a low
/// battery and the darkness of an empty one can be looked at without waiting for them.
/// Everything else of the round is only shown. settings is editable: the numbers of the
/// rules (they apply from the next fixed step on), the switch that stops the battery
/// from draining and the request for a new round, which the game reads at the start of
/// the next frame.
void drawGameplayPanel(game::Round& round, game::GameplaySettings& settings);

} // namespace debug
