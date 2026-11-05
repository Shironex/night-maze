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
    MenuBackground.cpp
    MenuBackground.hpp
    MenuCamera.cpp
    MenuCamera.hpp
    MenuCameraPath.cpp
    MenuCameraShared.hpp
    Minimap.cpp
    Minimap.hpp
    RoundMusic.cpp
    RoundMusic.hpp
    Shadows.cpp
    Shadows.hpp
    SoundCues.cpp
    SoundCues.hpp
    Vignette.cpp
    Vignette.hpp
)

# The application and the classes that draw: they need a window and an OpenGL context.
target_sources(night_maze PRIVATE
    MenuBackgroundRenderer.cpp
    MenuBackgroundRenderer.hpp
    MinimapRenderer.cpp
    MinimapRenderer.hpp
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
add_subdirectory(player)
add_subdirectory(round)
add_subdirectory(screens)
add_subdirectory(shade)
