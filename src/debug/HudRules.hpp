// The rules of the HUD that are plain arithmetic: how large it is drawn and how long
// a sentence stays. They need neither ImGui nor a window, so the tests can ask them.
#pragma once

namespace debug {

/// The height of the window the sizes of the HUD are written for, in pixels: every size
/// in Hud.cpp is a number of pixels of a window that is 720 pixels high.
constexpr float HUD_REFERENCE_HEIGHT = 720.0F;

/// The one scale of the HUD: every size and every distance of it is multiplied by this.
/// It is the larger of two numbers. The window height as a part of
/// HUD_REFERENCE_HEIGHT: the HUD takes the same part of a small and of a large window
/// (1 at 720, 2 at 1440, 3 at 2160). And displayScale, the scaling of the display (1 at
/// 100 %, 1.5 at 150 %): in a small window on a dense display the text stays as large
/// as the rest of the desktop.
float hudScale(float displayScale, float windowHeight);

/// How much of a sentence that is shown for a while can be seen, seconds after its
/// moment: 1 (fully there) for holdSeconds, then it fades evenly for fadeSeconds, then 0
/// (gone). 0 also before its moment, for a negative time.
float hintOpacity(float seconds, float holdSeconds, float fadeSeconds);

/// The sentence the player reads after taking the heartstone, while it cannot sprint. It
/// is at most 60 printable ASCII characters, so it fits the line of the hints.
inline constexpr const char* HEAVY_HINT = "Heavy. No running for a while.";

/// The one sentence for the step in which the heartstone opened the gate: the sentence of
/// the gate and the one above would start in the same moment, and the line of the hints
/// has room for one. The same limit of 60 characters.
inline constexpr const char* HEAVY_GATE_HINT = "The gate is open. Heavy, no running for a while.";

/// True when two sentences started in the same step: their ages, in seconds, are less
/// than a quarter of a second apart. The two ages are counted by two clocks (one counts
/// up, one down from 30), so they are never equal to the last digit.
bool sameMoment(float secondsA, float secondsB);

/// The seconds a clock that runs DOWN from wholeSeconds has already run: what the hints
/// of the tea and of the heartstone count from. Never below 0, also when the clock holds
/// more than wholeSeconds (the debug UI changed the length while it ran).
float secondsSince(float wholeSeconds, float secondsLeft);

} // namespace debug
