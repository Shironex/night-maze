# Story notes are read in order, nearest to the start first, from a saved place
Status: accepted (2026-10-07)
Code: src/game/Interactables.hpp, src/game/Interactables.cpp, src/game/Settings.hpp, src/game/NightMazeApp.cpp, src/game/Campaign.cpp

Context: Notes tell the lamplighter's story, so their order matters.
Decision: The story notes of a maze, ranked by passages from the start, show consecutive lines of a table of 24. The next unread line is saved in the settings and moves on when a maze is finished. In a calm night the 8 lines about the shadow are skipped.
Why: The notes nearest to the start come first, since that is the order the player walks. A first line chosen by the seed was the earlier plan, rejected for repeats and gaps.
Cost and revisit: The table is code, not data. Earlier: 16 lines, before the shadow lines. I reopen it if the story needs branches.
