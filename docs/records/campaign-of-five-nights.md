# The campaign is five nights with a table of its own
Status: accepted (2026-10-08)
Code: src/game/Campaign.hpp, src/game/Campaign.cpp, src/game/NightMazeApp.cpp

Context: I wanted the story told in order, in mazes that grow and get darker, without changing the three levels of free play.
Decision: the nights are their own table, from 10 x 10 to 22 x 22. The mazes come from one campaign seed, so each night keeps its maze within a campaign and a new campaign gets new ones. Progress is saved only when a night is won. Calm night is ignored, and night 1 has no shade.
Why: extra rows in the difficulty table would mix the story with free play and break the picker. A separate table lets nights 2 and 4 sit between the levels.
Cost and revisit: the numbers in the table are not balanced by many players. I revisit them when friends report a night as too hard.
