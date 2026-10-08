# The game menu is built in RmlUi, and Dear ImGui stays with the debug window
Status: accepted (2026-10-06)
Code: src/ui/UiLayer.cpp, src/ui/UiLayer.hpp, assets/ui/main_menu.rml, assets/ui/menu.rcss, cmake/Dependencies.cmake

Context: The game needed menus that move like a game. Dear ImGui already drove the debug tools and was the cheaper start.
Decision: Every menu screen is an RmlUi document with a stylesheet, drawn over the frame. Dear ImGui serves only the debug window and the HUD.
Why: RmlUi felt smoother and better animated, and the game should grow beyond the course. I accepted that I cannot explain all of it at the defence. A Dear ImGui menu was the rejected fallback.
Cost and revisit: Two UI libraries, plus FreeType, on both systems. I reopen it if RmlUi fails on the macOS 4.1 profile, which is untested.
