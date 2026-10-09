# Third-party dependencies, downloaded at configure time and pinned to release tags
# (stb, which has no tags, to a commit). The licences of FreeType and RmlUi ask for
# a notice in what is distributed, like the others: THIRD-PARTY-NOTICES.txt, which is
# generated (tools/build-notices.mjs) and has to be generated again when
# a version here changes.
# GLAD is not here: it is generated code that lives in external/glad.
include(FetchContent)

# ---- GLFW: window, OpenGL context, input ---------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# Treat the GLFW headers as system headers so they cannot produce warnings in our code.
get_target_property(glfw_include_dirs glfw INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(glfw PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${glfw_include_dirs}")

# ---- GLM: vector and matrix math ------------------------------------------------------
# GLM is header-only. By default its CMake build also compiles a static library that we do
# not need, so switch that off: only the header-only interface target is used.
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# Treat the GLM headers as system headers so they cannot produce warnings in our code.
# glm::glm-header-only is an alias, properties must be set on the real target name.
get_target_property(glm_include_dirs glm-header-only INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(glm-header-only PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${glm_include_dirs}"
)

# ---- Dear ImGui (docking branch): debug panels ---------------------------------------
# ImGui ships without a CMake build, so only download it and define the target ourselves.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b-docking
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# The OpenGL3 backend uses the small GL loader bundled with ImGui, so it does not need GLAD.
# The GLFW backend needs the GLFW headers.
target_link_libraries(imgui PUBLIC glfw)

# ---- doctest: unit tests ---------------------------------------------------------------
# doctest is a single header. Its CMake build can also compile a small static library that
# contains only main(): we write that one line ourselves in tests/main.cpp, so the library
# is switched off. So are the tests and examples of doctest itself and its install rules.
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

# The doctest target is an interface target that only carries the include path. Unlike
# GLFW and GLM it needs no extra step here: when doctest is not the main project, its own
# CMakeLists.txt already declares that path as SYSTEM, so the header cannot produce
# warnings in our tests.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.5.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(doctest)

# ---- stb_image: decoding image files (PNG and others) into pixels ----------------------
# The stb repository has no release tags, so the version is pinned to a commit hash (the
# commit that holds stb_image 2.30). GIT_SHALLOW is left out on purpose: a shallow clone
# can only fetch a branch or a tag by name, not an arbitrary commit, so the whole history
# is downloaded once. stb has no CMake build, so only download it and define the target
# ourselves, like ImGui.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
)
FetchContent_MakeAvailable(stb)

# stb_image.h is a single header that also contains its implementation. The implementation
# is compiled exactly once, in external/stb/stb_image.c, into this small library. It is a
# target of its own so that it is built with the compiler's default warnings and not with
# the strict ones of our code (night_maze_enable_warnings is never called for it).
add_library(stb_image STATIC ${CMAKE_CURRENT_LIST_DIR}/../external/stb/stb_image.c)
# SYSTEM: the header must not produce warnings in the files of ours that include it.
target_include_directories(stb_image SYSTEM PUBLIC ${stb_SOURCE_DIR})
# STBI_NO_STDIO removes every function of stb_image that opens a file by name. Our loader
# reads the file itself and hands stb the bytes (see src/assets/ImageLoader.cpp). PUBLIC,
# because the header has to see the same definition in the implementation file and in the
# files that include it.
target_compile_definitions(stb_image PUBLIC STBI_NO_STDIO)

# ---- FreeType: reads font files and draws their letters (needed by RmlUi) --------------
# RmlUi does not read font files itself: it asks FreeType. FreeType can use five other
# libraries when it finds them on the computer (zlib, bzip2, libpng, HarfBuzz, Brotli).
# All five are switched off, so the build is the same on every computer and links nothing
# that happens to be installed (Homebrew on macOS has all of them). Without the system
# zlib FreeType uses the small copy that is part of its own source. What is lost: fonts
# compressed with bzip2, colour emoji stored as PNG, WOFF2 files and automatic hinting
# through HarfBuzz. The game loads plain TrueType files and needs none of it.
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

# OVERRIDE_FIND_PACKAGE: RmlUi looks for FreeType with find_package(Freetype). With this
# word that call is answered by the source downloaded here, and never by a FreeType that
# is installed on the computer. The name has to be written the way RmlUi asks for it.
FetchContent_Declare(
    Freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-14-3
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(Freetype)

# RmlUi links the target Freetype::Freetype. An installed FreeType has a target of that
# name, but FreeType built from source as part of another project only has "freetype",
# so the second name is added here.
add_library(Freetype::Freetype ALIAS freetype)

# ---- RmlUi: the menu of the game (documents in RML and RCSS, like HTML and CSS) --------
# A static library, like every other dependency. RmlUi would build a shared one (a DLL)
# by default. Its samples, tests and Lua bindings are off by default and stay off.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    rmlui
    GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
    GIT_TAG 6.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rmlui)

# Treat the RmlUi headers as system headers so they cannot produce warnings in our code
# (and so that clang-tidy does not check them: its header filter matches every path with
# "src/" in it, and the downloaded source lies in a directory called rmlui-src).
get_target_property(rmlui_include_dirs rmlui_core INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(rmlui_core PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${rmlui_include_dirs}"
)

# RmlUi itself only produces lists of triangles and asks an interface to draw them. The
# code that does the drawing and the code that translates window events are "backends":
# files that come with RmlUi but are not part of its library, like the backends of ImGui.
# Two of them are compiled here, unchanged: the renderer for OpenGL 3.3 and later, and
# the platform helpers for GLFW. RmlUi_Backend_GLFW_GL3.cpp is NOT used: it creates its
# own window and its own main loop, and this program has both.
add_library(rmlui_backend STATIC
    ${rmlui_SOURCE_DIR}/Backends/RmlUi_Platform_GLFW.cpp
    ${rmlui_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.cpp
)
target_include_directories(rmlui_backend SYSTEM PUBLIC ${rmlui_SOURCE_DIR}/Backends)
target_link_libraries(rmlui_backend PUBLIC RmlUi::RmlUi glad glfw)
target_compile_definitions(rmlui_backend PRIVATE
    # By default the renderer brings its own copy of a GLAD loader for OpenGL 3.3. With
    # this macro it includes the named header instead: the GLAD of this project, which
    # core::Window has already loaded for OpenGL 4.1 Core. One loader, one set of
    # function pointers.
    "RMLUI_GL3_CUSTOM_LOADER=<glad/gl.h>"
    # GLFW must not include an OpenGL header of its own next to GLAD.
    GLFW_INCLUDE_NONE
)

# ---- miniaudio: opens the sound card and mixes the sounds of the game ------------------
# miniaudio is one header that also contains its implementation, like stb_image. Unlike
# stb its repository does bring a CMakeLists.txt, and that file must NOT run here:
#   - it defines a target that is itself called "miniaudio", the name of the small
#     library defined below, and two targets cannot share a name,
#   - it adds install rules (its option MINIAUDIO_INSTALL is ON by default) and looks on
#     the computer for libvorbis and libopus to build extra decoders around them, so the
#     build would depend on what happens to be installed.
# Switching its options off would still leave the name clash. So SOURCE_SUBDIR points at
# a directory the repository does not have: FetchContent then downloads the source and
# adds nothing of it to the build ("cmake" is only a name that does not exist there).
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(miniaudio)

# The implementation is compiled exactly once, in external/miniaudio/miniaudio.c, into
# this small library. It is a C file, and a target of its own for the same reason as
# stb_image: it is built with the compiler's default warnings and not with the strict
# ones of our code (night_maze_enable_warnings is never called for it).
add_library(miniaudio STATIC ${CMAKE_CURRENT_LIST_DIR}/../external/miniaudio/miniaudio.c)
# SYSTEM: the header must not produce warnings in the file of ours that includes it, and
# clang-tidy must not check it (its header filter matches every path with "src/" in it,
# and the downloaded source lies in a directory called miniaudio-src).
target_include_directories(miniaudio SYSTEM PUBLIC ${miniaudio_SOURCE_DIR})
# The parts of miniaudio the game does not use are left out of the build. PUBLIC, and
# that matters more than for stb: the macros change which fields the structs of
# miniaudio have, so the implementation file and the file of ours that includes the
# header must see the same set, or the two would disagree about the size of a struct.
target_compile_definitions(miniaudio PUBLIC
    # No writing of sound files: the game only plays.
    MA_NO_ENCODING
    # No built-in decoder for MP3. The decoders for WAV and FLAC stay: the short sounds
    # of the game are WAV files, and the theme of the menu is a FLAC file, which holds
    # the same samples in a quarter of the bytes (assets/audio/README.md). The FLAC
    # decoder is plain C inside miniaudio.h and needs no library.
    MA_NO_MP3
    # No generators of sine waves and noise: the sounds come from files.
    MA_NO_GENERATION
    # No resource manager, the part that opens sound files by name, decodes them on
    # a thread of its own and can stream long ones. The audio library reads each file
    # itself (core::readBinaryFile), like the image loader does for stb, and has
    # miniaudio decode the bytes in one go (see src/audio/AudioEngine.cpp). What stays is
    # the high level engine (ma_engine, ma_sound), the mixer under it and the device.
    MA_NO_RESOURCE_MANAGER
)
# What miniaudio has to be linked with, as its documentation says (miniaudio.h, section
# 2 "Building"):
#   - Windows: nothing. The sound libraries of Windows are loaded while the program runs.
#   - macOS: nothing either. miniaudio loads the Core Audio frameworks while the program
#     runs. The documentation names one catch: a program built this way may not pass the
#     notarization of Apple. The way around it is the macro MA_NO_RUNTIME_LINKING together
#     with the frameworks CoreFoundation, CoreAudio and AudioToolbox. It is not used
#     here, because the game is not notarized.
#   - Linux: dl (for loading ALSA or PulseAudio while the program runs), pthread and m
#     (the math library). CMAKE_DL_LIBS and Threads::Threads are the names CMake has for
#     the first two on whatever system it runs on.
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS} m)
endif()
