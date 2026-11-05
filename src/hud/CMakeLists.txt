# The HUD of a round: drawn with ImGui over the picture of the game, so it is part of the
# executable. The debug UI calls it, the game never includes it.
target_sources(night_maze PRIVATE
    Hud.cpp
    Hud.hpp
    HudCanvas.cpp
    HudCards.cpp
    HudColors.hpp
    HudInstruments.cpp
    HudRules.cpp
    HudRules.hpp
    HudShared.hpp
)

# The arithmetic of the HUD needs neither ImGui nor a window, so it is compiled into the
# test program directly.
target_sources(night_maze_tests PRIVATE
    HudRules.cpp
    HudRules.hpp
)
