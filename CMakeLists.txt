cmake_minimum_required(VERSION 3.24)

project(NightMaze VERSION 0.17.0 LANGUAGES C CXX)

# Objective-C and Objective-C++, the two languages of Apple that C and C++ code is mixed
# with on macOS. Objective-C++ is the language of the one file of ours that talks to the
# video decoder of macOS (src/video/VideoDecoderApple.mm). Objective-C is the language of
# the macOS files of GLFW (.m). Both are switched on here, right after project() and
# before any target or downloaded library exists, and only on macOS, so Windows never
# looks for a compiler of them. Two things went wrong before, both seen on a GitHub
# runner on 2026-10-07, the first builds on macOS with this code:
#   - Objective-C++ switched on further down, next to the video library: CMake found the
#     compiler and then stopped while generating, with "Missing variable is:
#     CMAKE_OBJCXX_COMPILE_OBJECT".
#   - Objective-C++ alone switched on here: the .m files of GLFW were then compiled as
#     Objective-C++, which is stricter about pointer types than Objective-C, and failed
#     with dozens of errors. With Objective-C switched on too, a .m file is Objective-C.
if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/glad)
include(cmake/Dependencies.cmake)

# Strict warnings for our own targets only (third-party code is built with its defaults).
function(night_maze_enable_warnings target)
    if(MSVC)
        # /w14062 switches on warning C4062, which /W4 leaves off: a switch over an
        # enum that has no "default" and misses one of the enumerators. Clang and GCC
        # report the same with -Wall (-Wswitch), so both systems now notice a new entry
        # of an enum that a switch forgot (the name of a screen in the debug window,
        # for example).
        target_compile_options(${target} PRIVATE /W4 /w14062 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# The same floats on every system, for every target from here on (our own code; the
# third-party libraries above keep their defaults). Clang and GCC may fuse a * b + c
# into one instruction (FMA) that rounds once instead of twice, and Clang does so by
# default on Apple silicon; MSVC does not. A seed must build the same world on Windows
# and on macOS (Tonight's hedge is one maze for every player), and the tests pin
# checksums of float results: with fusing, the grass of a seed sat a few bits apart on
# macOS (seen in the macos job of the CI on 2026-10-16). -ffp-contract=off keeps the two
# roundings, as MSVC does.
if(NOT MSVC)
    add_compile_options(-ffp-contract=off)
endif()

# ---- the targets ------------------------------------------------------------------------
# Made here without sources: each folder under src/ and tests/ adds its own files in its own
# CMakeLists.txt (target_sources, sorted, one file per line), see add_subdirectory below.
# tools/move_module.py moves a module between folders and keeps the lists.

# ---- engine: reusable layers (src/assets, src/core, src/gfx, src/scene) ------------------
add_library(engine STATIC)
# Includes are written relative to src/, for example #include "core/Window.hpp".
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# GLM is PUBLIC because headers of engine (gfx/Shader.hpp, gfx/Vertex.hpp,
# scene/Transform.hpp, scene/Camera.hpp, scene/Collider.hpp, scene/Light.hpp,
# scene/LightBlock.hpp and others) expose GLM types, so every target that includes them
# needs the GLM include path too.
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
# The rule files of src/game: the maze and its world, the round, the player, the settings
# of the look, the screens of the game, the sound cues and the settings file are plain
# data and math. They live in a library of their own, and not in the night_maze
# executable, so that the test program can link them too: a test cannot link code that is
# inside another executable.
add_library(game_logic STATIC)
# PUBLIC: the headers of this library include headers of engine (scene/Collider.hpp,
# scene/Light.hpp, gfx/Vertex.hpp, assets/ImageLoader.hpp) and GLM, so whoever includes
# them needs the include paths of engine. The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)

# ---- ui: the layer that owns RmlUi and draws the documents of the menu ----------------
# In the order of the layers it stands beside gfx and scene, below game: it needs the
# window, the log and the asset paths of engine, and it knows nothing about the game
# (the game tells it which document to show and takes the names of the clicked
# buttons). It is a library of its own and not a part of engine, so that game_logic and
# the test program, which link engine, do not have to link RmlUi and FreeType.
add_library(ui STATIC)
# engine is PUBLIC: ui/UiLayer.hpp includes core/Window.hpp. RmlUi is PRIVATE: UiLayer.hpp
# only names its classes (forward declarations), so the code that uses the layer needs
# neither the headers of RmlUi nor its include paths.
target_link_libraries(ui PUBLIC engine)
target_link_libraries(ui PRIVATE rmlui_backend)
night_maze_enable_warnings(ui)

# ---- video: plays a video file into a texture with the decoder of the operating system --
# In the order of the layers it stands beside ui, above gfx: it needs the textures of
# engine and knows nothing about the game (the game tells it which file to play and
# draws its texture). It is a library of its own and not a part of engine for the same
# reason as ui: game_logic and the test program link engine, and they should link neither
# the media libraries of an operating system nor a file in another language.
#
# The decoder is the only code of the project that is written twice, once per system,
# behind one interface (video/VideoDecoder.hpp). src/video/CMakeLists.txt lists the
# Windows file on every system and the macOS file (Objective-C++) only on macOS.
add_library(video STATIC)
# engine is PUBLIC: video/VideoPlayer.hpp includes gfx/FrameTexture.hpp.
target_link_libraries(video PUBLIC engine)
# The player decodes on a thread of its own. Threads::Threads is whatever the system
# needs for std::thread: nothing on Windows and macOS, -pthread elsewhere.
find_package(Threads REQUIRED)
target_link_libraries(video PUBLIC Threads::Threads)
if(WIN32)
    # Media Foundation, the part of Windows that decodes video files, and COM, which it
    # is built on. They are libraries of Windows itself: nothing is downloaded and
    # nothing is added to the folder of the game.
    target_link_libraries(video PRIVATE mfplat mfreadwrite mfuuid ole32)
    # The N editions of Windows have no Media Foundation until the Media Feature Pack
    # is installed. A program that needs a missing DLL does not start at all, so the
    # two DLLs are "delay loaded": Windows loads them at the first call into them and
    # not at the start of the program. The decoder asks whether they exist before that
    # first call (VideoDecoderWindows.cpp), and without them the menu shows its still
    # picture. delayimp is the small library of the linker that does the late loading.
    # INTERFACE: a static library is not linked itself, so the switches are passed on
    # to the program that links it.
    target_link_options(video INTERFACE "/DELAYLOAD:mfplat.dll" "/DELAYLOAD:mfreadwrite.dll")
    target_link_libraries(video INTERFACE delayimp)
elseif(APPLE)
    # AVFoundation, the part of macOS that decodes video files, is used from
    # Objective-C++: C++ that can also call the Objective-C classes of Apple. The
    # language itself is switched on at the top of this file, the source file and its
    # compile option are in src/video/CMakeLists.txt.
    # The same C++ standard as the rest of the project, said again for this language.
    set_target_properties(video PROPERTIES
        OBJCXX_STANDARD 20
        OBJCXX_STANDARD_REQUIRED ON
        OBJCXX_EXTENSIONS OFF
    )
    # Frameworks are the libraries of macOS. They are part of the system.
    target_link_libraries(video PRIVATE
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework Foundation"
    )
endif()
night_maze_enable_warnings(video)

# ---- audio: plays short sounds through the sound device, with miniaudio ---------------
# In the order of the layers it stands beside ui and video: it needs the log, the file
# reader and the paths of engine, and it knows nothing about the game (the game hands it a
# list of files and later the number of the sound to play; which sound belongs to what is a
# rule of game_logic, game/sound/SoundCues.hpp). It is a library of its own and not a part
# of engine for the same reason as ui and video: game_logic links engine, and it should not
# link miniaudio, which opens the sound device of the computer. The game links it, and so
# does the test program, which makes the audio engine without a device
# (audio::AudioOutput::None): no test needs a sound card, and none is heard.
add_library(audio STATIC)
# engine is PUBLIC only for the src/ include root, which comes from it: the header of
# audio includes nothing but the standard library. miniaudio is PRIVATE:
# audio/AudioEngine.hpp hides it behind a pointer to a struct that only the .cpp file
# knows, so the code that uses the library needs neither the header of miniaudio nor
# its include path. What miniaudio itself links on each system is said at its target
# (cmake/Dependencies.cmake).
target_link_libraries(audio PUBLIC engine)
target_link_libraries(audio PRIVATE miniaudio)
night_maze_enable_warnings(audio)

# ---- night_maze: the application, its HUD and its debug UI -----------------------------
# src/main.cpp, the files of src/game that draw (NightMazeApp, the renderers and the
# others), the pass helpers of src/render (PostProcess and the others), src/hud and
# src/debug: they need a window and an OpenGL context, so they are not something a test can
# run.
add_executable(night_maze)
target_link_libraries(night_maze PRIVATE engine game_logic ui video audio imgui)
night_maze_enable_warnings(night_maze)

# ---- night_maze_tests: unit tests of the code that runs without a window --------------
# enable_testing() makes CMake write the list of tests into the build directory, where
# the ctest program finds it. It has to be called in this top-level file.
enable_testing()

# The tests in tests/, plus the few files of src/hud, src/debug and src/video that need
# neither ImGui nor a decoder, which their folders add to this target.
add_executable(night_maze_tests)
# game_logic brings engine with it (scene/Collider is part of engine). audio is linked
# for the tests of the audio engine, which make it without a sound device
# (audio::AudioOutput::None): nothing is opened and nothing is heard.
target_link_libraries(night_maze_tests PRIVATE game_logic audio doctest::doctest)
night_maze_enable_warnings(night_maze_tests)
# The loader tests (of images and of OBJ models) read the real files of the game, and the
# layering test reads the sources. A test must not depend on the directory it is started
# from, so the absolute path of assets/ in the repository is compiled in as a string: the
# macro NIGHT_MAZE_ASSETS_DIR.
target_compile_definitions(night_maze_tests PRIVATE
    NIGHT_MAZE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# One CTest test: it runs the whole test program and passes when the program exits with
# code 0. The program is part of the default build, so the tests always compile.
add_test(NAME night_maze_tests COMMAND night_maze_tests)

add_subdirectory(src)
add_subdirectory(tests)

# ---- assets: shaders, models and textures next to the executable -----------------------
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
