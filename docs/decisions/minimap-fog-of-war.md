# The map shows only the corridors the player has already seen
Status: accepted (2026-10-05)
Code: src/game/Discovery.hpp, src/game/Discovery.cpp, src/game/Minimap.cpp, src/game/Round.cpp, tests/DiscoveryTests.cpp

Context: Finding the way is the game. A map of the whole maze with the exit answers it for the player.
Decision: A cell is discovered when the player stands in it or sees it along a straight corridor up to the next wall. The state lives in the round.
Why: The map helps with going back but never shows the way out. Showing the whole maze was rejected. Discovery reads the round's maze copy, so a wall opened by a lever opens the view at once.
Cost and revisit: It needs a state grid and a rule. Earlier: a corner map always on screen; now it is held with M. I reopen it if the game turns out too hard or long corridors reveal too much.
