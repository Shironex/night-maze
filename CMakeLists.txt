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

# ---- engine: reusable layers (core now; gfx, assets, renderer, scene later) -----------
add_library(engine STATIC
    src/core/Application.cpp
    src/core/Application.hpp
    src/core/GlCheck.cpp
    src/core/GlCheck.hpp
    src/core/Input.cpp
    src/core/Input.hpp
    src/core/Log.cpp
    src/core/Log.hpp
    src/core/Time.cpp
    src/core/Time.hpp
    src/core/Window.cpp
    src/core/Window.hpp
)
# Includes are written relative to src/, for example #include "core/Window.hpp".
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# GLM is linked ahead of its first users (Shader, Transform, Camera). It is PUBLIC because
# their headers will expose GLM types to every target that includes them.
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
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/panels/RendererPanel.cpp
    src/debug/panels/RendererPanel.hpp
)
target_link_libraries(night_maze PRIVATE engine imgui)
night_maze_enable_warnings(night_maze)
