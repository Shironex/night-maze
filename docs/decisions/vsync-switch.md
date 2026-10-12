# Vsync is switched in the debug window and is not saved
Status: accepted (2026-10-12)
Code: src/core/Window.cpp, src/core/Window.hpp, src/main.cpp, src/debug/categories/DiagnosticsCategory.cpp

Context: With vsync on, the frame rate stops at the refresh rate of the screen, so it cannot show whether a change made a frame cheaper.
Decision: `Window::setVsync` sets the swap interval and remembers the choice, and `setFullscreen` asks for the remembered one again. The switch is a row of the Diagnostics category. Its value lives in `src/main.cpp`, and every run starts with vsync on.
Why: A line in the settings file was rejected: the switch is a tool for measuring, a player has no screen to change it on, and a forgotten "off" would tear the picture and heat the machine in every later run.
Cost and revisit: A measurement needs the switch set again after every start. I reopen this if players ask for a frame rate above the refresh rate: then it becomes a setting with a control on the settings screen.
