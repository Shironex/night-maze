# Sound has looping wind, footsteps on distance walked and volume groups
Status: accepted (2026-10-08)
Code: src/audio/AudioEngine.hpp, src/game/SoundCues.hpp, src/game/SoundCues.cpp, src/game/Settings.hpp, tools/make_sounds.py

Context: single sounds were not enough. The maze needed wind that never stops, and steps that match how fast the player moves.
Decision: loops are switched on and off with a fade. Footsteps follow a clock of distance walked, 1.5 m a step walking and 2.0 m sprinting, not time. Sounds belong to effects, ambient or music, each with its own volume under the master.
Why: a clock in seconds would keep the wrong pace after a sprint. A single master volume lost because players turn the wind down and lose the cues. Each loop file starts at a zero crossing, after I measured a tick at the seam.
Cost and revisit: three step variants repeat. I add variants if players notice. Earlier: two groups, effects and ambient, until 2026-10-09 (see menu-music).
