target_sources(night_maze PRIVATE
    Categories.hpp
    DebugContext.hpp
    DebugUI.cpp
    DebugUI.hpp
    DebugWindow.cpp
    DebugWindow.hpp
    Icons.cpp
    Icons.hpp
    MazePlan.cpp
    MazePlan.hpp
    PerfProbe.cpp
    PerfProbe.hpp
    Pictures.cpp
    Pictures.hpp
    RawTextureSampler.cpp
    RawTextureSampler.hpp
    Search.cpp
    Search.hpp
    Theme.cpp
    Theme.hpp
    Widgets.cpp
    Widgets.hpp
)

# The file of the debug UI that needs neither ImGui nor a window: the text search of the
# debug window. It is compiled into the test program directly.
target_sources(night_maze_tests PRIVATE
    Search.cpp
    Search.hpp
)

add_subdirectory(categories)
