# The rules: plain data and math that need no window and no OpenGL, so the test program
# can link them.
target_sources(game_logic PRIVATE
    Bloom.cpp
    Bloom.hpp
    EnvironmentMapping.cpp
    EnvironmentMapping.hpp
    Fog.cpp
    Fog.hpp
    GameState.cpp
    GameState.hpp
    Interactables.cpp
    Interactables.hpp
    Interaction.cpp
    Interaction.hpp
    Intro.cpp
    Intro.hpp
    KeyBindings.cpp
    KeyBindings.hpp
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
    Player.cpp
    Player.hpp
    RoundMusic.cpp
    RoundMusic.hpp
    Settings.cpp
    Settings.hpp
    Shade.cpp
    Shade.hpp
    Shadows.cpp
    Shadows.hpp
    SoundCues.cpp
    SoundCues.hpp
    StartOptions.cpp
    StartOptions.hpp
    Storybook.cpp
    Storybook.hpp
    Vignette.cpp
    Vignette.hpp
)

# The application and the classes that draw: they need a window and an OpenGL context.
target_sources(night_maze PRIVATE
    InteractableRenderer.cpp
    InteractableRenderer.hpp
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
    StorybookRenderer.cpp
    StorybookRenderer.hpp
)

add_subdirectory(land)
add_subdirectory(maze)
add_subdirectory(round)
