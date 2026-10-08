# The map is held with M, large in the centre, and the player stands still while it is up
Status: accepted (2026-10-08)
Code: src/game/Minimap.hpp, src/game/Minimap.cpp, src/game/GameState.hpp, src/game/Player.hpp, src/game/NightMazeApp.cpp

Context: Friends who played said the map in the screen corner made the game too easy.
Decision: The map shows only while the key is held, as a square of 0.7 of the window height in the middle. While it is up, movement and looking are off and nothing can be used. The round goes on, so the battery and the shade keep running.
Why: Standing still gives the map a price. Keeping the corner map was rejected, because a glance at it was free.
Cost and revisit: A held key is awkward for long study,. Earlier: a map in the bottom left corner. I reopen it if holding the key tires players.
