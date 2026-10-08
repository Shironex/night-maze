# One debug window on the right replaces the panels, and the HUD sits at the top edge
Status: accepted (2026-10-06)
Code: src/debug/DebugWindow.cpp, src/debug/DebugWindow.hpp, src/debug/Hud.cpp, src/debug/Theme.hpp, src/debug/Search.cpp

Context: Thirteen separate panels were hard to find and crowded the screen.
Decision: One window on the right with an icon rail, search and pin, amber for actions, teal for values, 14 px text. The HUD stays 16 px from the top edge, centred, and the window starts below the height the HUD reserves.
Why: Keeping thirteen panels was rejected: the layout grows with each panel and nothing can be searched or pinned. With the folded title rows gone, the HUD could be fixed instead of moving.
Cost and revisit: The reserve is computed from three text lines. I reopen it if the HUD gets more lines or the menu settings start to duplicate the window.
