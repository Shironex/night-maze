# The moon shadow map covers the whole terrain and takes its bias in metres
Status: accepted (2026-10-05)
Code: src/scene/LightSpace.cpp, src/game/Shadows.cpp, assets/shaders/common/shadows.glsl

Context: The moon is a directional light, so its map is an orthographic box. Shadows must not shimmer when the player walks.
Decision: Each frame the box is fitted to the terrain plus 0.5 m, from the terrain and the moon angles only. Its matrix and settings are plain uniforms beside the sampler. Bias is in metres (0.02 plus 0.12 times one minus the cosine), applied in the scene shader.
Why: I rejected a box around the camera view because its shadows shimmer unless snapped to texels. A depth constant was rejected because a metre is a different fraction in every box.
Cost and revisit: Half the map covers hills, so texels are 3.2 cm. I reopen it if mazes grow far past the start size.
