# Crystals scale with the maze, the gate opens at a share of them, and an empty battery only darkens
Status: accepted (2026-10-05)
Code: src/game/Crystals.hpp, src/game/Crystals.cpp, src/game/Round.cpp, src/game/Difficulty.cpp, src/game/Campaign.cpp

Context: The PRD gave no counts, and an empty battery needed a meaning.
Decision: A maze gets one crystal per 8 cells, from 1 to 64, unless a level or night names its count. The gate opens at 70 percent, rounded up (80 on Hard and the last two nights). An empty battery only switches the flashlight off until a crystal charges it.
Why: Requiring every crystal makes the end a hunt for one missed item. A loss on an empty battery adds a state and an unfair ending. Earlier: the cap was 16.
Cost and revisit: The numbers are tuning values, not play-tested. The shade can carry the player back to the start, but there is still no losing screen.
