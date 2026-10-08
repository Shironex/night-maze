// Game HUD: the crystal counter, the battery bar, the stamina bar, hints, the crosshair
// with its prompt, the card of a note and the "You escaped" card.
#pragma once

#include "game/KeyBindings.hpp"

namespace game {
struct GameplaySettings;
struct MazeWorld;
struct PickState;
struct Round;
struct Player;
} // namespace game

namespace debug {

/// Draws the HUD of the game on top of the scene. Called by DebugUI::draw, inside the
/// ImGui frame, in every frame: unlike the debug window it is not hidden by the debug
/// key, because it is part of the game and not a tool.
///
/// It lives in src/debug only because this is where ImGui is: the game itself must not
/// include ImGui. world, round, settings, player and pick are read only.
///
/// What it shows:
///   - at the top of the window: the crystals collected, needed and existing, the time
///     of the round, the charge of the battery as a bar that changes colour when it is
///     low, below it the stamina as a thin bar (only while it is not full and for
///     a moment after, game::staminaBarVisible; it pulses dimly while the player is
///     winded; while the tea of a flask works it is a copper bar that shrinks with the
///     seconds that are left, game::flaskEffectFraction), and a line of hint for each
///     thing there is to say (the tea works and for how long, the gate has opened, the
///     battery is empty). While the map is on the screen (mapOnScreen) the hint lines
///     are left out: the map stands in the middle of the window and the lines would
///     reach into its top edge,
///   - in the middle of the window, while the cursor is captured: a small crosshair,
///     the point the picking ray goes through. It changes when the ray points at a lever
///     or a note the player can use, and a line near the bottom of the window then
///     names the key,
///   - below the middle, while a note is being read: a card with its text
///     (game::openNoteText),
///   - a little below the middle, for a few seconds after the shade carried the player
///     back to the start: one line that says so (Round::caughtLine),
///   - in the middle of the window, once the round is won: a card with the time, the
///     crystals and the key that starts a new round.
///
/// The HUD takes no input: the mouse and the keyboard pass through it to the game.
/// Where it names a key it names the one the player has chosen (keys): the key of "use"
/// in the prompt and on the card of a note, the key of "restart" on the card of a won
/// round.
///
/// The strip always stands at the top edge of the window, in the middle. The debug
/// window keeps clear of it (hudReservedHeight), so the strip does not move when the
/// debug UI is shown or hidden.
void drawHud(const game::MazeWorld& world, const game::Round& round,
             const game::GameplaySettings& settings, const game::Player& player,
             const game::PickState& pick, bool mapOnScreen, const game::KeyBindings& keys);

/// The height of the room at the top edge of the window that the strip of the HUD can
/// take, in pixels of the screen: its distance from the edge plus its height with all
/// three lines of hint shown. The debug window starts below it, so the two never overlap.
/// Call it inside an ImGui frame.
float hudReservedHeight();

} // namespace debug
