# Randomness comes from mt19937 and my own randomBelow, never standard distributions
Status: accepted (2026-10-05)
Code: src/game/MazeGenerator.hpp, src/game/MazeGenerator.cpp, src/game/Crystals.cpp, tests/MazeGeneratorTests.cpp

Context: The same seed and size must give the same maze on macOS and Windows, or bugs and demos cannot move between my machines.
Decision: Numbers come from std::mt19937 with one 32-bit seed, and a short function maps them to a range with rejection. Crystals, flasks, levers and notes use the same rule with generators of their own and a hand-written shuffle.
Why: The standard fixes the engine output bit for bit but leaves distributions and std::shuffle to each library. With the MSVC distribution seed 1 gave another maze. A plain remainder was rejected as biased.
Cost and revisit: Any added random call changes every maze. I reopen it if tests on macOS show a different golden maze.
