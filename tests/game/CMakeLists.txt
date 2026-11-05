target_sources(night_maze_tests PRIVATE
    BloomTests.cpp
    EnvironmentMappingTests.cpp
    FogTests.cpp
    LightingTests.cpp
    ShadowTests.cpp
    SkyboxTests.cpp
    VignetteTests.cpp
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
