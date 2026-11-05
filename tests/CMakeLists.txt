# The unit tests, one program. Each folder mirrors its folder in src/ and lists its own
# files, sorted, one file per line.

target_sources(night_maze_tests PRIVATE
    LayeringTests.cpp
    main.cpp
)

add_subdirectory(assets)
add_subdirectory(audio)
add_subdirectory(core)
add_subdirectory(debug)
add_subdirectory(game)
add_subdirectory(gfx)
add_subdirectory(scene)
add_subdirectory(video)
