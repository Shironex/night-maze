# The intro can be told as a storybook of painted layers
Status: accepted (2026-10-15). Earlier: the live intro of [intro-played-live](intro-played-live.md), which stays as the fallback.
Code: src/game/Storybook.hpp, src/game/Storybook.cpp, src/game/StorybookRenderer.cpp, src/game/NightMazeAppScreens.cpp, assets/intro/storybook.txt

Context: the five cards over camera shots read as weak, and I want to compare richer intros in the game before choosing one.
Decision: when `assets/intro/storybook.txt` can be read, the intro shows its beats: two to four PNG layers per beat that drift and zoom at their own speeds, a crossfade between beats, the same card for the text, an optional spoken line per beat and optional music. `--intro-style classic` plays the live intro instead, and a manifest or a layer that fails falls back to it. The switch is not a tool switch, so a new campaign can be compared in both styles.
Why: a text manifest lets illustrations, voice and music be swapped without a build. A settings field was rejected: the comparison is temporary and must not touch the player's settings file.
Cost and revisit: all layers are decoded when the intro starts and all sounds when the game starts, which is fine for six beats. The calm-night line of the classic intro has no storybook variant. I reopen this once one intro is chosen: the other one goes.
