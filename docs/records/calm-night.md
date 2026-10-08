# Calm night is a switch in free play, not a fourth difficulty
Status: accepted (2026-10-08)
Code: src/game/Settings.hpp, src/game/NightMazeApp.cpp, assets/ui/free_play.rml

Context: some players want the mazes without an enemy, and the difficulty table has three rows that set maze size and crystal count.
Decision: calm night is a saved on/off setting on the free play screen. It removes the shade on every difficulty and leaves out the notes about it. The campaign ignores it.
Why: a fourth difficulty row would force a size and a crystal count onto a choice that is only about the enemy. A switch combines with any of the three levels.
Cost and revisit: the campaign has the shade from night 2 on, so a player who wants no enemy cannot play it. I reopen this if friends ask for a calm campaign.
