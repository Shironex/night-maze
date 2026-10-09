# Tonight's hedge is free play on Normal with the date as its seed
Status: accepted (2026-10-09)
Code: src/game/Daily.hpp, src/game/Daily.cpp, src/game/Settings.cpp, src/game/StartOptions.cpp, src/game/NightMazeApp.cpp, assets/ui/main_menu.rml, tests/DailyTests.cpp

Context: the story says the hedge "still grows a new way each day", and a seed already decides everything in a maze. I wanted one maze a day that every player shares.
Decision: the main menu entry "Tonight's hedge" plays free play on Normal with the seed year, month, day in a row (8 October 2026 is 20261008), always with the shade. The day is the local date, read once when the entry is chosen. The settings file keeps the best time of one day, that day and a count of days won.
Why: reusing free play, the title card and the result screen needed no new screen and no new event. A table of best times by date was rejected for now: it grows for ever and no screen shows it yet.
Cost and revisit: the best time of yesterday is gone after the first win of today. A run with a tool switch has a fixed day (`--daily`, otherwise 8 October 2026) and never writes a best time, so pictures stay the same. I add a history when a screen wants to show it.
