cmake_minimum_required(VERSION 3.24)

project(NightMaze VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/glad)
include(cmake/Dependencies.cmake)

# Strict warnings for our own targets only (third-party code is built with its defaults).
function(night_maze_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---- engine: reusable layers (assets, core, gfx and scene now; renderer later) --------
add_library(engine STATIC
    src/assets/ImageLoader.cpp
    src/assets/ImageLoader.hpp
    src/assets/ObjLoader.cpp
    src/assets/ObjLoader.hpp
    src/core/Application.cpp
    src/core/Application.hpp
    src/core/GlCheck.cpp
    src/core/GlCheck.hpp
    src/core/Input.cpp
    src/core/Input.hpp
    src/core/Log.cpp
    src/core/Log.hpp
    src/core/Paths.cpp
    src/core/Paths.hpp
    src/core/Time.cpp
    src/core/Time.hpp
    src/core/Window.cpp
    src/core/Window.hpp
    src/gfx/Buffer.cpp
    src/gfx/Buffer.hpp
    src/gfx/Mesh.cpp
    src/gfx/Mesh.hpp
    src/gfx/Shader.cpp
    src/gfx/Shader.hpp
    src/gfx/Texture2D.cpp
    src/gfx/Texture2D.hpp
    src/gfx/Vertex.hpp
    src/gfx/VertexArray.cpp
    src/gfx/VertexArray.hpp
    src/scene/Camera.cpp
    src/scene/Camera.hpp
    src/scene/Collider.cpp
    src/scene/Collider.hpp
    src/scene/Transform.cpp
    src/scene/Transform.hpp
)
# Includes are written relative to src/, for example #include "core/Window.hpp".
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# GLM is PUBLIC because headers of engine (gfx/Shader.hpp, scene/Transform.hpp,
# scene/Camera.hpp, scene/Collider.hpp) expose GLM types, so every target that includes
# them needs the GLM include path too.
target_link_libraries(engine PUBLIC glad glfw glm::glm-header-only)
# stb_image is PRIVATE: only assets/ImageLoader.cpp includes its header, no header of
# engine does, so the targets that use engine do not need its include path.
target_link_libraries(engine PRIVATE stb_image)
target_compile_definitions(engine PUBLIC
    GLFW_INCLUDE_NONE      # GLFW must not include an OpenGL header, GLAD provides it
    GL_SILENCE_DEPRECATION # macOS marks all of OpenGL as deprecated
)
night_maze_enable_warnings(engine)

# ---- game_logic: the rules of Night Maze that need no window and no OpenGL -------------
# The maze, its generator and its layout in the world are plain data and math. They live
# in a library of their own, and not in the night_maze executable, so that the test
# program can link them too: a test cannot link code that is inside another executable.
add_library(game_logic STATIC
    src/game/Maze.cpp
    src/game/Maze.hpp
    src/game/MazeGenerator.cpp
    src/game/MazeGenerator.hpp
    src/game/MazeLayout.cpp
    src/game/MazeLayout.hpp
)
# PUBLIC: game/MazeLayout.hpp includes scene/Collider.hpp and GLM, so whoever includes it
# needs the include paths of engine. The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)

# ---- night_maze: the application and its debug UI -------------------------------------
# NightMazeApp stays in the executable: it needs a window and an OpenGL context, so it is
# not something a test can run.
add_executable(night_maze
    src/main.cpp
    src/game/NightMazeApp.cpp
    src/game/NightMazeApp.hpp
    src/debug/DebugContext.hpp
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/panels/CameraPanel.cpp
    src/debug/panels/CameraPanel.hpp
    src/debug/panels/RendererPanel.cpp
    src/debug/panels/RendererPanel.hpp
    src/debug/panels/ShadersPanel.cpp
    src/debug/panels/ShadersPanel.hpp
)
target_link_libraries(night_maze PRIVATE engine game_logic imgui)
night_maze_enable_warnings(night_maze)

# ---- night_maze_tests: unit tests of the code that runs without a window --------------
# enable_testing() makes CMake write the list of tests into the build directory, where
# the ctest program finds it. It has to be called in this top-level file.
enable_testing()

add_executable(night_maze_tests
    tests/main.cpp
    tests/ColliderTests.cpp
    tests/ImageLoaderTests.cpp
    tests/MazeGeneratorTests.cpp
    tests/MazeLayoutTests.cpp
    tests/MazeTests.cpp
    tests/ObjLoaderTests.cpp
)
# game_logic brings engine with it (scene/Collider is part of engine).
target_link_libraries(night_maze_tests PRIVATE game_logic doctest::doctest)
night_maze_enable_warnings(night_maze_tests)
# The loader tests read the real models and textures. A test must not depend on the
# directory it is started from, so the absolute path of assets/ in the repository is
# compiled in as a string: the macro NIGHT_MAZE_ASSETS_DIR.
target_compile_definitions(night_maze_tests PRIVATE
    NIGHT_MAZE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# One CTest test: it runs the whole test program and passes when the program exits with
# code 0. The program is part of the default build, so the tests always compile.
add_test(NAME night_maze_tests COMMAND night_maze_tests)

# ---- assets: shaders (later models and textures) next to the executable ---------------
# The program looks for assets/ in the directory of its own executable (core::assetPath).
# $<TARGET_FILE_DIR:night_maze> is that directory. It is a generator expression, filled in
# when the build runs, because the Visual Studio generator adds a Debug/ or Release/ level.
if(WIN32)
    # Windows: copy the directory. Symbolic links need Developer Mode or administrator
    # rights there. copy_assets is a custom target without output files, so its command
    # runs on every build of the default (ALL) target: "cmake --build --preset debug"
    # refreshes the copy even when no C++ file changed. Building only the night_maze
    # target does not run it.
    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:night_maze>/assets
        COMMENT "Copying assets next to the executable"
        VERBATIM
    )
    # copy_assets does not depend on night_maze (copy_directory creates the destination
    # directory itself), so "cmake --build --preset debug --target copy_assets" refreshes
    # the copy without building the program. That matters while the program is running:
    # Windows locks a running .exe and the Visual Studio generator then fails to link it,
    # also when no C++ file changed.
else()
    # macOS: a symbolic link to the directory in the repository, made each time
    # night_maze has been linked (POST_BUILD). A shader edited in assets/ is seen by the
    # next reload, without building.
    add_custom_command(TARGET night_maze POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:night_maze>/assets
        COMMENT "Linking assets next to the executable"
        VERBATIM
    )
endif()
