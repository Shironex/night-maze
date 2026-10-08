# The map is drawn after the composite pass in plain sRGB, with vertices rebuilt every frame
Status: accepted (2026-10-06)
Code: src/game/MinimapRenderer.cpp, src/game/Minimap.cpp, src/game/NightMazeApp.cpp, assets/shaders/post/minimap.frag, assets/shaders/post/minimap_overlay.frag

Context: The map is a schematic from maze data, and its arrow moves all the time.
Decision: It is drawn into its own RGBA8 target after the composite pass, in constant sRGB colours. The vertices are rebuilt on the CPU and uploaded as a dynamic buffer every frame the map is shown.
Why: Drawing before the composite pass was rejected, since fog, vignette and the tone curve would change a schematic. Rebuilding means nothing is forgotten when a cell is discovered or a lever opens a wall.
Cost and revisit: Blending happens in sRGB, and the upload (about 1400 vertices in the default maze) is unmeasured. Earlier: a default corner at the bottom left. That is gone, since the map is held centred.
