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
    src/assets/AssetCache.cpp
    src/assets/AssetCache.hpp
    src/assets/ImageLoader.cpp
    src/assets/ImageLoader.hpp
    src/assets/ObjLoader.cpp
    src/assets/ObjLoader.hpp
    src/assets/Tangents.cpp
    src/assets/Tangents.hpp
    src/core/Application.cpp
    src/core/Application.hpp
    src/core/Files.cpp
    src/core/Files.hpp
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
    src/gfx/ColorSpace.cpp
    src/gfx/ColorSpace.hpp
    src/gfx/ComparisonSampler.cpp
    src/gfx/ComparisonSampler.hpp
    src/gfx/CoverFit.cpp
    src/gfx/CoverFit.hpp
    src/gfx/Cubemap.cpp
    src/gfx/Cubemap.hpp
    src/gfx/Extensions.cpp
    src/gfx/Extensions.hpp
    src/gfx/FrameTexture.cpp
    src/gfx/FrameTexture.hpp
    src/gfx/Framebuffer.cpp
    src/gfx/Framebuffer.hpp
    src/gfx/Mesh.cpp
    src/gfx/Mesh.hpp
    src/gfx/Shader.cpp
    src/gfx/Shader.hpp
    src/gfx/ShaderSource.cpp
    src/gfx/ShaderSource.hpp
    src/gfx/Texture2D.cpp
    src/gfx/Texture2D.hpp
    src/gfx/UniformBuffer.cpp
    src/gfx/UniformBuffer.hpp
    src/gfx/Vertex.hpp
    src/gfx/VertexArray.cpp
    src/gfx/VertexArray.hpp
    src/scene/Camera.cpp
    src/scene/Camera.hpp
    src/scene/Collider.cpp
    src/scene/Collider.hpp
    src/scene/Light.cpp
    src/scene/Light.hpp
    src/scene/LightBlock.cpp
    src/scene/LightBlock.hpp
    src/scene/LightSpace.cpp
    src/scene/LightSpace.hpp
    src/scene/Raycast.cpp
    src/scene/Raycast.hpp
    src/scene/Transform.cpp
    src/scene/Transform.hpp
)
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
# The maze, its generator, its layout in the world, the terrain it stands on, the places
# of the grass, the player, the settings of the lighting, of the bloom, of the fog, of
# the vignette and of the shadows, the rules of a round (exit, crystals, battery), the
# levers and the notes with the picking of a frame, the sound cues (which short sound
# belongs to what happened, and the clock of the low battery pulse), what the minimap
# shows (the discovered cells and its shapes), the path of the menu camera, the screens
# of the game (menu, playing, paused, round end, settings), the rule that chooses what is
# behind the main menu, the three difficulty levels, the settings of the player with
# the text of their file and the switches of the command line are plain data and math.
# They live in a library of their own, and not in the night_maze executable, so that
# the test program can link them too: a test cannot link code that is inside another
# executable.
add_library(game_logic STATIC
    src/game/Bloom.cpp
    src/game/Bloom.hpp
    src/game/Crystals.cpp
    src/game/Crystals.hpp
    src/game/Difficulty.cpp
    src/game/Difficulty.hpp
    src/game/Discovery.cpp
    src/game/Discovery.hpp
    src/game/EnvironmentMapping.cpp
    src/game/EnvironmentMapping.hpp
    src/game/Exit.cpp
    src/game/Exit.hpp
    src/game/Fog.cpp
    src/game/Fog.hpp
    src/game/GameState.cpp
    src/game/GameState.hpp
    src/game/Grass.cpp
    src/game/Grass.hpp
    src/game/Interactables.cpp
    src/game/Interactables.hpp
    src/game/Interaction.cpp
    src/game/Interaction.hpp
    src/game/Lighting.cpp
    src/game/Lighting.hpp
    src/game/Maze.cpp
    src/game/Maze.hpp
    src/game/MazeGenerator.cpp
    src/game/MazeGenerator.hpp
    src/game/MazeLayout.cpp
    src/game/MazeLayout.hpp
    src/game/MazeWorld.cpp
    src/game/MazeWorld.hpp
    src/game/MenuBackground.cpp
    src/game/MenuBackground.hpp
    src/game/MenuCamera.cpp
    src/game/MenuCamera.hpp
    src/game/Minimap.cpp
    src/game/Minimap.hpp
    src/game/Player.cpp
    src/game/Player.hpp
    src/game/Puddles.cpp
    src/game/Puddles.hpp
    src/game/Round.cpp
    src/game/Round.hpp
    src/game/Settings.cpp
    src/game/Settings.hpp
    src/game/Shadows.cpp
    src/game/Shadows.hpp
    src/game/SoundCues.cpp
    src/game/SoundCues.hpp
    src/game/StartOptions.cpp
    src/game/StartOptions.hpp
    src/game/Terrain.cpp
    src/game/Terrain.hpp
    src/game/Vignette.cpp
    src/game/Vignette.hpp
)
# PUBLIC: the headers of this library (Crystals.hpp, Exit.hpp, Lighting.hpp,
# MazeLayout.hpp, MazeWorld.hpp, Player.hpp, Round.hpp, Shadows.hpp, Terrain.hpp) include
# headers of engine (scene/Collider.hpp, scene/Light.hpp, gfx/Vertex.hpp, assets/ImageLoader.hpp)
# and GLM, so whoever includes them needs the include paths of engine.
# The src/ include root comes from engine as well.
target_link_libraries(game_logic PUBLIC engine)
night_maze_enable_warnings(game_logic)

# ---- ui: the layer that owns RmlUi and draws the documents of the menu ----------------
# In the order of the layers it stands beside gfx and scene, below game: it needs the
# window, the log and the asset paths of engine, and it knows nothing about the game
# (the game tells it which document to show and takes the names of the clicked
# buttons). It is a library of its own and not a part of engine, so that game_logic and
# the test program, which link engine, do not have to link RmlUi and FreeType.
add_library(ui STATIC
    src/ui/AssetFileInterface.cpp
    src/ui/AssetFileInterface.hpp
    src/ui/UiLayer.cpp
    src/ui/UiLayer.hpp
)
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
# behind one interface (video/VideoDecoder.hpp). VideoDecoderWindows.cpp is in the list
# on every system and empty outside of Windows, so the checks that run over all .cpp
# files find it in the list of compiled files everywhere. The macOS file is
# Objective-C++, a language CMake has to be told about, and is added below.
add_library(video STATIC
    src/video/VideoClock.cpp
    src/video/VideoClock.hpp
    src/video/VideoDecoder.hpp
    src/video/VideoDecoderWindows.cpp
    src/video/VideoPlayer.cpp
    src/video/VideoPlayer.hpp
)
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
    # language is switched on here and only here, so Windows never looks for a compiler
    # of it.
    enable_language(OBJCXX)
    target_sources(video PRIVATE src/video/VideoDecoderApple.mm)
    # The same C++ standard as the rest of the project, said again for this language.
    set_target_properties(video PROPERTIES
        OBJCXX_STANDARD 20
        OBJCXX_STANDARD_REQUIRED ON
        OBJCXX_EXTENSIONS OFF
    )
    # Automatic reference counting: the compiler releases the Objective-C objects the
    # file holds, there is no retain and no release in it.
    set_source_files_properties(src/video/VideoDecoderApple.mm PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc"
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
# reader and the paths of engine, and it knows nothing about the game (the game hands it
# a list of files and later the number of the sound to play; which sound belongs to
# what is a rule of game_logic, game/SoundCues.hpp). It is a library of its own and not
# a part of engine for the same reason as ui and video: game_logic and the test program
# link engine, and they should not link miniaudio, which opens the sound device of the
# computer. So it is linked into night_maze only, and no test needs a sound card.
add_library(audio STATIC
    src/audio/AudioEngine.cpp
    src/audio/AudioEngine.hpp
)
# engine is PUBLIC only for the src/ include root, which comes from it: the header of
# audio includes nothing but the standard library. miniaudio is PRIVATE:
# audio/AudioEngine.hpp hides it behind a pointer to a struct that only the .cpp file
# knows, so the code that uses the library needs neither the header of miniaudio nor
# its include path. What miniaudio itself links on each system is said at its target
# (cmake/Dependencies.cmake).
target_link_libraries(audio PUBLIC engine)
target_link_libraries(audio PRIVATE miniaudio)
night_maze_enable_warnings(audio)

# ---- night_maze: the application and its debug UI -------------------------------------
# NightMazeApp and the classes that draw (MazeRenderer, GameplayRenderer,
# InteractableRenderer, TerrainRenderer, GrassRenderer, ColliderLines, LightRig, Skybox,
# PostProcess, ShadowMap, MinimapRenderer, MenuBackgroundRenderer,
# with the shared ModelDraw) stay in the executable: they need a window and an OpenGL
# context, so they are not something a test can run.
add_executable(night_maze
    src/main.cpp
    src/game/ColliderLines.cpp
    src/game/ColliderLines.hpp
    src/game/GameplayRenderer.cpp
    src/game/GameplayRenderer.hpp
    src/game/GrassRenderer.cpp
    src/game/GrassRenderer.hpp
    src/game/InteractableRenderer.cpp
    src/game/InteractableRenderer.hpp
    src/game/LightRig.cpp
    src/game/LightRig.hpp
    src/game/MazeRenderer.cpp
    src/game/MazeRenderer.hpp
    src/game/MenuBackgroundRenderer.cpp
    src/game/MenuBackgroundRenderer.hpp
    src/game/MinimapRenderer.cpp
    src/game/MinimapRenderer.hpp
    src/game/ModelDraw.cpp
    src/game/ModelDraw.hpp
    src/game/NightMazeApp.cpp
    src/game/NightMazeApp.hpp
    src/game/PostProcess.cpp
    src/game/PostProcess.hpp
    src/game/PuddleRenderer.cpp
    src/game/PuddleRenderer.hpp
    src/game/ShaderUniforms.hpp
    src/game/ShadowMap.cpp
    src/game/ShadowMap.hpp
    src/game/Skybox.cpp
    src/game/Skybox.hpp
    src/game/TerrainRenderer.cpp
    src/game/TerrainRenderer.hpp
    src/debug/Categories.hpp
    src/debug/DebugContext.hpp
    src/debug/DebugUI.cpp
    src/debug/DebugUI.hpp
    src/debug/DebugWindow.cpp
    src/debug/DebugWindow.hpp
    src/debug/Hud.cpp
    src/debug/Hud.hpp
    src/debug/Icons.cpp
    src/debug/Icons.hpp
    src/debug/MazePlan.cpp
    src/debug/MazePlan.hpp
    src/debug/Pictures.cpp
    src/debug/Pictures.hpp
    src/debug/RawTextureSampler.cpp
    src/debug/RawTextureSampler.hpp
    src/debug/Search.cpp
    src/debug/Search.hpp
    src/debug/Theme.cpp
    src/debug/Theme.hpp
    src/debug/Widgets.cpp
    src/debug/Widgets.hpp
    src/debug/categories/DiagnosticsCategory.cpp
    src/debug/categories/DiagnosticsCategory.hpp
    src/debug/categories/GameplayCategory.cpp
    src/debug/categories/GameplayCategory.hpp
    src/debug/categories/LightCategory.cpp
    src/debug/categories/LightCategory.hpp
    src/debug/categories/PlayerCategory.cpp
    src/debug/categories/PlayerCategory.hpp
    src/debug/categories/PostProcessCategory.cpp
    src/debug/categories/PostProcessCategory.hpp
    src/debug/categories/RenderCategory.cpp
    src/debug/categories/RenderCategory.hpp
    src/debug/categories/WorldCategory.cpp
    src/debug/categories/WorldCategory.hpp
)
target_link_libraries(night_maze PRIVATE engine game_logic ui video audio imgui)
night_maze_enable_warnings(night_maze)

# ---- night_maze_tests: unit tests of the code that runs without a window --------------
# enable_testing() makes CMake write the list of tests into the build directory, where
# the ctest program finds it. It has to be called in this top-level file.
enable_testing()

add_executable(night_maze_tests
    tests/main.cpp
    tests/BloomTests.cpp
    tests/ColliderTests.cpp
    tests/ColorSpaceTests.cpp
    tests/CoverFitTests.cpp
    tests/CrystalTests.cpp
    tests/DifficultyTests.cpp
    tests/DiscoveryTests.cpp
    tests/EnvironmentMappingTests.cpp
    tests/ExitTests.cpp
    tests/FogTests.cpp
    tests/FramebufferTests.cpp
    tests/GameStateTests.cpp
    tests/GrassTests.cpp
    tests/ImageLoaderTests.cpp
    tests/InteractablesTests.cpp
    tests/InteractionTests.cpp
    tests/LightTests.cpp
    tests/LightingTests.cpp
    tests/MazeGeneratorTests.cpp
    tests/MazeLayoutTests.cpp
    tests/MazeTests.cpp
    tests/MazeWorldTests.cpp
    tests/MenuBackgroundTests.cpp
    tests/MenuCameraTests.cpp
    tests/MinimapTests.cpp
    tests/ObjLoaderTests.cpp
    tests/PlayerTests.cpp
    tests/PuddleTests.cpp
    tests/RaycastTests.cpp
    tests/RoundTests.cpp
    tests/SearchTests.cpp
    tests/SettingsTests.cpp
    tests/ShaderSourceTests.cpp
    tests/ShadowTests.cpp
    tests/SkyboxTests.cpp
    tests/SoundCueTests.cpp
    tests/StartOptionsTests.cpp
    tests/TangentTests.cpp
    tests/TerrainTests.cpp
    tests/TransformTests.cpp
    tests/VideoClockTests.cpp
    tests/VignetteTests.cpp
    # The one file of the debug UI that needs neither ImGui nor a window: the text
    # search of the debug window. It is compiled into the test program directly.
    src/debug/Search.cpp
    src/debug/Search.hpp
    # The same for the video library: the arithmetic of its clock needs neither
    # a decoder nor OpenGL. Linking the library itself would bring Media Foundation
    # into the test program.
    src/video/VideoClock.cpp
    src/video/VideoClock.hpp
)
# game_logic brings engine with it (scene/Collider is part of engine).
target_link_libraries(night_maze_tests PRIVATE game_logic doctest::doctest)
night_maze_enable_warnings(night_maze_tests)
# The loader tests (of images and of OBJ models) read the real files of the game. A test
# must not depend on the directory it is started from, so the absolute path of assets/ in
# the repository is compiled in as a string: the macro NIGHT_MAZE_ASSETS_DIR.
target_compile_definitions(night_maze_tests PRIVATE
    NIGHT_MAZE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

# One CTest test: it runs the whole test program and passes when the program exits with
# code 0. The program is part of the default build, so the tests always compile.
add_test(NAME night_maze_tests COMMAND night_maze_tests)

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
