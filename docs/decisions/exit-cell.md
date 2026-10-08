# The exit is the cell farthest from the start by passages, not the opposite corner
Status: accepted (2026-10-05)
Code: src/game/Exit.hpp, src/game/Exit.cpp, src/game/MazeWorld.cpp, tests/ExitTests.cpp

Context: One gate segment must close the exit, the walk should be long, and a seed must give the same exit everywhere.
Decision: A breadth-first search from the start counts passages. The cell with the largest count is the exit, ties going to the first in row order. The gate stands on its only open side.
Why: In a perfect maze the farthest cell is always a dead end, so one gate closes it. The opposite corner can have two open sides, and in the 4 by 4 test maze it is 6 passages away while the farthest cell is 14.
Cost and revisit: The exit can be near in a straight line. I reopen it if mazes gain loops or the start moves.
