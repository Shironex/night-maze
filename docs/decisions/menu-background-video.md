# The menu background is a recorded loop from the game, played by the operating system
Status: accepted (2026-10-06)
Code: src/game/MenuBackground.cpp, src/game/MenuBackgroundRenderer.cpp, src/video/VideoPlayer.cpp, src/game/MenuCamera.cpp, assets/video/menu_loop.mp4

Context: The menu needed a clean picture behind it that costs no game frames.
Decision: I record a loop with the menu camera, one closed walk through every crystal and to the gate at 0.7 m/s. Media Foundation or AVFoundation plays it. If that fails, a still of its first frame is shown, then the live scene.
Why: The owner chose a loop for the cleaner look. System decoders cost about 10 KB. FFmpeg, about 130 MB of libraries, lost.
Cost and revisit: The loop is 17.7 MB and needs recording again when the look changes. The macOS decoder is untested. I reopen it if the walk is too long to record.
