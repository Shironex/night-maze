# The intro is five cards played live in the engine
Status: accepted (2026-10-08)
Code: src/game/Intro.hpp, src/game/Intro.cpp, src/game/GameState.cpp

Context: the game needed a short opening that tells the story, and it must not go out of date when the look of the game changes.
Decision: the intro is a script of five cards laid over pictures that the menu camera takes in one fixed maze. It plays once, on the first start, and any key skips it. The tool switches on the command line never start it.
Why: a recorded video shows an old look of the game after every visual change and needs a file. Playing live costs no file and always matches the game.
Cost and revisit: the shot moments were picked by eye in one seed, so a change to maze generation can break them. I reopen this if the intro needs something the engine cannot show.
