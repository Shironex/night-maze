// Game HUD: the lamp gauge, the crystal counter, the stamina line, the loudness ticks,
// sentences that come and go, the crosshair with its prompt, the name of the night, the
// card of a note and the "You escaped" card.
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
/// What it shows. Each reading has a place of its own, in a corner or at the bottom
/// edge, so the middle of the picture stays free and nothing has to move for the map:
///   - bottom left, the lamp gauge: a ring that empties with the battery, the charge as
///     a number and the label LAMP. Below GameplaySettings::lowBatteryThreshold the
///     ring and the label turn red, the label reads LOW and the gauge dips together
///     with the lamp (game::flashlightFlicker),
///   - top right, the crystal counter: the crystals collected against the number that
///     opens the gate, a thin bar of all crystals of the maze with a tick at that
///     number, and under it how many there are and the time of the round. Once the gate
///     is open the label GATE OPEN stands there,
///   - bottom centre, the stamina line, only while the stamina is in use
///     (game::staminaBarVisible): it gets shorter from both ends. It pulses dimly while
///     the player is winded. While the tea of a flask works it is copper, shrinks with
///     the seconds that are left (game::flaskEffectFraction) and has them beside it,
///   - under it, in a round with a shade, three ticks that say how loud the player is
///     (Round::noiseMeter, game::noiseTicks): what the shade hears,
///   - between the card of a note and the prompt, for a few seconds each: a sentence
///     when the tea starts to work, when the gate opens and when the battery is empty
///     (debug::hintOpacity). While the map is on the screen (mapOnScreen) the sentence
///     is left out: the map stands in the middle of the window and reaches down to it,
///   - top left, for the first seconds of a night of the campaign (playedNight, 0 in
///     free play): the number and the title of the night. The maze of the day
///     (playedDaily, its day or 0) has its name and its day there,
///   - in the middle of the window, while the cursor is captured: a small crosshair,
///     the point the picking ray goes through. It changes when the ray points at a lever
///     or a note the player can use, and the prompt near the bottom of the window then
///     names the key, in a small box, and what it does,
///   - below the middle, while a note is being read: a card with its text
///     (game::openNoteText),
///   - a little below the middle, for a few seconds after the shade carried the player
///     back to the start: one line that says so (Round::caughtLine),
///   - in the middle of the window, once the round is won: a card with the time, the
///     crystals and the key that starts a new round.
///
/// Every size and every distance follows one scale (debug::hudScale): the HUD takes
/// the same part of a small and of a large window.
///
/// The HUD takes no input: the mouse and the keyboard pass through it to the game.
/// Where it names a key it names the one the player has chosen (keys): the key of "use"
/// in the prompt and on the card of a note, the key of "restart" on the card of a won
/// round.
///
/// The instruments are drawn behind every ImGui window. The debug window, which is
/// a tool, may cover a corner of them while it is open.
void drawHud(const game::MazeWorld& world, const game::Round& round,
             const game::GameplaySettings& settings, const game::Player& player,
             const game::PickState& pick, bool mapOnScreen, const game::KeyBindings& keys,
             int playedNight, std::uint32_t playedDaily);

} // namespace debug
