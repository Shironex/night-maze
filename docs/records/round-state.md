# A round keeps its own copy of the maze, and an opened wall stops blocking at the pull
Status: accepted (2026-10-06)
Code: src/game/Round.hpp, src/game/Round.cpp, src/game/Minimap.cpp, tests/RoundTests.cpp

Context: A lever removes a wall. Discovery, the map and the shade must see that, yet the world is built once and a new round must restore every wall.
Decision: The round copies the maze at its start and removes the wall from the copy on a pull. The world's maze never changes. The wall's collision box leaves the obstacle list at the pull, not after the 1.5 s sink, like the gate.
Why: Restart then costs nothing. Filtering opened walls in every reader was rejected, because one forgotten reader shows a wall that is gone.
Cost and revisit: Two sources of wall truth, and for 1.5 s a visible wall can be walked through. I reopen it if that looks wrong.
