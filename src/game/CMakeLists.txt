# The application class: it needs a window and an OpenGL context. Each domain folder below
# lists its rules (plain data and math, in game_logic, so the test program can link them)
# and its classes that draw (in night_maze).
target_sources(night_maze PRIVATE
    NightMazeApp.cpp
    NightMazeApp.hpp
    NightMazeAppCampaign.cpp
    NightMazeAppMenus.cpp
    NightMazeAppPasses.cpp
    NightMazeAppRender.cpp
    NightMazeAppRound.cpp
    NightMazeAppScreens.cpp
    NightMazeAppShared.hpp
)

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
