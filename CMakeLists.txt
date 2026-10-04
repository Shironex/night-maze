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

# ---- engine: reusable layers (core, gfx and scene now; assets and renderer later) -----
add_library(engine STATIC
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
    src/gfx/Shader.cpp
    src/gfx/Shader.hpp
    src/gfx/VertexArray.cpp
    src/gfx/VertexArray.hpp
    src/scene/Camera.cpp
    src/scene/Camera.hpp
    src/scene/Transform.cpp
    src/scene/Transform.hpp
)
# Includes are written relative to src/, for example #include "core/Window.hpp".
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# GLM is PUBLIC because headers of engine (gfx/Shader.hpp, scene/Transform.hpp,
# scene/Camera.hpp) expose GLM types, so every target that includes them needs the GLM
# include path too.
target_link_libraries(engine PUBLIC glad glfw glm::glm-header-only)
target_compile_definitions(engine PUBLIC
    GLFW_INCLUDE_NONE      # GLFW must not include an OpenGL header, GLAD provides it
    GL_SILENCE_DEPRECATION # macOS marks all of OpenGL as deprecated
)
night_maze_enable_warnings(engine)

# ---- night_maze: the game and its debug UI -------------------------------------------
add_executable(night_maze
    src/main.cpp
    src/game/NightMazeApp.cpp
    src/game/NightMazeApp.hpp
    src/debug/DebugContext.hpp
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/panels/RendererPanel.cpp
    src/debug/panels/RendererPanel.hpp
    src/debug/panels/ShadersPanel.cpp
    src/debug/panels/ShadersPanel.hpp
)
target_link_libraries(night_maze PRIVATE engine imgui)
night_maze_enable_warnings(night_maze)

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
    # copy_assets is built after night_maze, so the directory of the executable exists
    # before the copy. The dependency goes one way only: night_maze does not depend on
    # copy_assets.
    add_dependencies(copy_assets night_maze)
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
