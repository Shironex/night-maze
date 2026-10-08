# The shade moves only while the flashlight is not on it
Status: accepted (2026-10-08)
Code: src/game/Shade.hpp, src/game/Shade.cpp, assets/audio/shade_near.wav

Context: the round could not be lost, and I wanted an enemy that makes the flashlight the tool of defence without adding a loss screen.
Decision: one shade per maze walks at 4 m/s, between walking (3.0) and sprinting (5.5). It stands still only while the lit flashlight cone reaches it with no wall between. Looking at it with the lamp off does not stop it.
Why: after the light leaves it the shade waits 2 s, because otherwise a lit shade in a corridor would catch you the moment you passed it. Being caught resets the round like a restart. A hum along the walking path is the tell. The code says Shade, since shadow already means shadow mapping.
Cost and revisit: I reopen the speed if players cannot escape or never feel pressed.
