# The rules: plain data and math that need no window and no OpenGL, so the test program
# can link them.
target_sources(game_logic PRIVATE
    Bloom.cpp
    Bloom.hpp
    EnvironmentMapping.cpp
    EnvironmentMapping.hpp
    Fog.cpp
    Fog.hpp
    Lighting.cpp
    Lighting.hpp
    Shadows.cpp
    Shadows.hpp
    Vignette.cpp
    Vignette.hpp
)

# The application and the classes that draw: they need a window and an OpenGL context.
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
add_subdirectory(maze)
add_subdirectory(menu)
add_subdirectory(minimap)
add_subdirectory(player)
add_subdirectory(round)
add_subdirectory(screens)
add_subdirectory(shade)
add_subdirectory(sound)
