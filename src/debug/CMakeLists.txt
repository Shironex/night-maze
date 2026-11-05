target_sources(night_maze PRIVATE
    Categories.hpp
    DebugContext.hpp
    DebugUI.cpp
    DebugUI.hpp
    DebugWindow.cpp
    DebugWindow.hpp
    Hud.cpp
    Hud.hpp
    HudCanvas.cpp
    HudCards.cpp
    HudInstruments.cpp
    HudRules.cpp
    HudRules.hpp
    HudShared.hpp
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

# The files of the debug UI that need neither ImGui nor a window: the text search of the
# debug window and the arithmetic of the HUD. They are compiled into the test program
# directly.
target_sources(night_maze_tests PRIVATE
    HudRules.cpp
    HudRules.hpp
    Search.cpp
    Search.hpp
)

add_subdirectory(categories)
