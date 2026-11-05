# The game layer is split into one folder per domain, and the passes of a frame live in src/render
Status: accepted (2026-11-05). Supersedes [rendering-in-the-game-layer](rendering-in-the-game-layer.md).
Code: src/game/CMakeLists.txt, src/game/round/CMakeLists.txt, src/render/CMakeLists.txt, tests/LayeringTests.cpp, tools/move_module.py

Context: src/game had grown to about 120 files in one flat folder: rules, renderers, the application class and the helpers of the passes side by side, so a reader could not see which files belong together.
Decision: The helpers of the passes that hold no rule of the game (PostProcess, ShadowMap, Skybox, LightRig and the others) moved to src/render. The rest of src/game sits in one folder per domain (maze, land, round, player, shade, screens, menu, minimap, sound, lighting), each with its own CMakeLists.txt that keeps the split between game_logic and night_maze. Only NightMazeApp and its shared header stay in src/game. Namespaces stayed game.
Why: A folder per domain puts a rule, its renderer and its test next to each other, so a domain can be read on its own. I rejected one folder for all renderers, because it keeps a renderer away from the data it draws and does not shrink the rules folder.
Cost and revisit: An include path is one folder longer, and a file that serves two domains sits in the one it is closest to. tests/LayeringTests.cpp guards the rules across the folders. I reopen it if a domain folder grows past about 30 files.
