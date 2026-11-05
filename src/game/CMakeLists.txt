# The game, one folder per domain. Each folder lists its rules (plain data and math, in
# game_logic, so the test program can link them) and its classes that draw (in night_maze).
# The application class that holds them together is in src/app.

add_subdirectory(land)
add_subdirectory(lighting)
add_subdirectory(maze)
add_subdirectory(menu)
add_subdirectory(minimap)
add_subdirectory(player)
add_subdirectory(round)
add_subdirectory(screens)
add_subdirectory(shade)
add_subdirectory(sound)
