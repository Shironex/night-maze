// Game HUD: the crystal counter, the battery bar, hints, the crosshair with its prompt,
// the card of a note and the "You escaped" card.
// See docs/modules/game/gameplay.md
#pragma once

namespace game {
struct GameplaySettings;
struct MazeWorld;
struct PickState;
struct Round;
} // namespace game

namespace debug {

/// Draws the HUD of the game on top of the scene. Called by DebugUI::draw, inside the
/// ImGui frame, in every frame: unlike the debug panels it is not hidden by the panel
/// toggle key, because it is part of the game and not a tool.
///
/// It lives in src/debug only because this is where ImGui is: the game itself must not
/// include ImGui. world, round, settings and pick are read only.
///
/// What it shows:
///   - at the top of the window: the crystals collected, needed and existing, the time
///     of the round, the charge of the battery as a bar that changes colour when it is
///     low, and a line of hint for each thing there is to say (the gate has opened,
///     the battery is empty),
///   - in the middle of the window, while the cursor is captured: a small crosshair,
///     the point the picking ray goes through. It changes when the ray points at a lever
///     or a note the player can use, and a line near the bottom of the window then
///     names the key,
///   - below the middle, while a note is being read: a card with its text
///     (game::openNoteText),
///   - in the middle of the window, once the round is won: a card with the time, the
///     crystals and the key that starts a new round.
///
/// The HUD takes no input: the mouse and the keyboard pass through it to the game.
void drawHud(const game::MazeWorld& world, const game::Round& round,
             const game::GameplaySettings& settings, const game::PickState& pick);

} // namespace debug
