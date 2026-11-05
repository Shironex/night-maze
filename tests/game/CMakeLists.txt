target_sources(night_maze_tests PRIVATE
    BloomTests.cpp
    EnvironmentMappingTests.cpp
    FogTests.cpp
    GameStateTests.cpp
    IntroTests.cpp
    LightingTests.cpp
    MenuBackgroundTests.cpp
    MenuCameraTests.cpp
    MinimapTests.cpp
    RoundMusicTests.cpp
    SettingsTests.cpp
    ShadowTests.cpp
    SkyboxTests.cpp
    SoundCueTests.cpp
    StartOptionsTests.cpp
    StorybookTests.cpp
    VignetteTests.cpp
)

add_subdirectory(land)
add_subdirectory(maze)
add_subdirectory(player)
add_subdirectory(round)
add_subdirectory(shade)
