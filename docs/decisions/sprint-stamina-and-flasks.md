# Sprint costs stamina, and flasks of tea hidden in dead ends refill it
Status: accepted (2026-10-08)
Code: src/game/Player.hpp, src/game/Player.cpp, src/game/Flasks.hpp, src/game/Flasks.cpp, src/game/MazeWorld.cpp

Context: Testers held sprint all the time, so walking never mattered.
Decision: A full bar drains in 6 s of sprint, refills after 1 s of rest in 4 s, and an empty bar leaves the player winded until it is half full. A flask of tea, a thing from the story's world and not a brand, fills the bar and makes sprint free for 20 s. Flasks lie in dead ends, three of which are kept free of crystals.
Why: A bar costs sprint without banning it. Unlimited sprint was rejected, since it made walking pointless.
Cost and revisit: The numbers are untuned. Reserving dead ends moved crystals, notes, levers and puddles for every seed. I reopen it if sprint feels dull.
