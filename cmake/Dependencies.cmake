# Third-party dependencies, downloaded at configure time and pinned to release tags
# (stb, which has no tags, to a commit).
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
