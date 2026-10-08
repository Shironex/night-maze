# Escape pauses and goes back one screen, and leaving the program is a menu button
Status: accepted (2026-10-06)
Code: src/game/GameState.hpp, src/game/GameState.cpp, src/game/NightMazeApp.cpp, src/core/Application.cpp, tests/GameStateTests.cpp

Context: Menus needed a predictable rule for the one key everyone presses. The framework closed the window on Escape.
Decision: Escape goes one screen back: round to pause, pause to round, result to main menu, settings to where they were opened. In the main menu it does nothing. Quit is a button.
Why: Pausing needs the key. Keeping Escape as quit was rejected, since a stray press would end the program. Earlier: four screens and three levels. Free play, nights, cards and the intro now follow the same rule.
Cost and revisit: The main menu has no keyboard exit. I reopen it if players ask for one.
