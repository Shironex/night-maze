# Drawing classes live in src/game and there is no renderer layer
Status: accepted (2026-10-05)
Code: src/game/PostProcess.cpp, src/game/Skybox.cpp, src/game/NightMazeApp.cpp

Context: The PRD planned a src/renderer layer. The frame grew to shadow, HDR scene, bloom and composite passes, so I had to decide whether to create it.
Decision: The sky, post process, shadow map and minimap stay classes in src/game. Only code that knows nothing of the game, like Framebuffer, lives in src/gfx. NightMazeApp::onRender holds the pass order.
Why: I rejected a layer with one class, and a move of everything, because I could not say what the passes share. Shadows fitted into two functions of NightMazeApp, so the layer never paid off.
Cost and revisit: onRender is long and the order is a comment. I reopen it if a transparent pass makes the order hard to follow.
