target_sources(night_maze_tests PRIVATE
    BloomTests.cpp
    EnvironmentMappingTests.cpp
    FogTests.cpp
    LightingTests.cpp
    MenuBackgroundTests.cpp
    MenuCameraTests.cpp
    MinimapTests.cpp
    RoundMusicTests.cpp
    ShadowTests.cpp
    SkyboxTests.cpp
    SoundCueTests.cpp
    VignetteTests.cpp
)

add_subdirectory(land)
add_subdirectory(maze)
add_subdirectory(player)
add_subdirectory(round)
add_subdirectory(screens)
add_subdirectory(shade)
