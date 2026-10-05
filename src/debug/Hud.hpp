// Game HUD: the crystal counter, the battery bar, hints and the "You escaped" card.
// See docs/modules/game/gameplay.md
#pragma once

namespace game {
struct Round;
struct GameplaySettings;
} // namespace game

namespace debug {

/// Draws the HUD of the game on top of the scene. Called by DebugUI::draw, inside the
/// ImGui frame, in every frame: unlike the debug panels it is not hidden by the panel
/// toggle key, because it is part of the game and not a tool.
///
/// It lives in src/debug only because this is where ImGui is: the game itself must not
/// include ImGui. round and settings are read only.
///
/// What it shows:
///   - at the top of the window: the crystals collected, needed and existing, the time
///     of the round, the charge of the battery as a bar that changes colour when it is
///     low, and a line of hint for each thing there is to say (the gate has opened,
///     the battery is empty),
///   - in the middle of the window, once the round is won: a card with the time, the
///     crystals and the key that starts a new round.
///
/// The HUD takes no input: the mouse and the keyboard pass through it to the game.
void drawHud(const game::Round& round, const game::GameplaySettings& settings);

} // namespace debug
