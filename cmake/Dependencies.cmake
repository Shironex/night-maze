# Third-party dependencies, downloaded at configure time and pinned to release tags.
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
