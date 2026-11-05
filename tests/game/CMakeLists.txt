target_sources(night_maze_tests PRIVATE
    BloomTests.cpp
    EnvironmentMappingTests.cpp
    FogTests.cpp
    GameStateTests.cpp
    InteractablesTests.cpp
    InteractionTests.cpp
    IntroTests.cpp
    KeyBindingsTests.cpp
    LightingTests.cpp
    MenuBackgroundTests.cpp
    MenuCameraTests.cpp
    MinimapTests.cpp
    PlayerTests.cpp
    RoundMusicTests.cpp
    SettingsTests.cpp
    ShadeTests.cpp
    ShadowTests.cpp
    SkyboxTests.cpp
    SoundCueTests.cpp
    StartOptionsTests.cpp
    StorybookTests.cpp
    VignetteTests.cpp
)

add_subdirectory(land)
add_subdirectory(maze)
add_subdirectory(round)
