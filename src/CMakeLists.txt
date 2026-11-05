# The sources of the program. Each folder lists its own files in its own CMakeLists.txt,
# sorted, one file per line, into the targets the root list makes. tools/move_module.py
# moves a module between folders and keeps the lists.

target_sources(night_maze PRIVATE
    main.cpp
)

add_subdirectory(assets)
add_subdirectory(audio)
add_subdirectory(core)
add_subdirectory(debug)
add_subdirectory(game)
add_subdirectory(gfx)
add_subdirectory(hud)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(ui)
add_subdirectory(video)
