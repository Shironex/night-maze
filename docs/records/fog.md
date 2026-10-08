# Fog is computed in the composite pass from a position rebuilt from depth, with height taken at the pixel
Status: accepted (2026-10-05)
Code: assets/shaders/post/composite.frag, src/game/Fog.hpp, tests/FogTests.cpp

Context: The fog has to lie low in the corridors, keep the moon clear and not move when the camera turns.
Decision: The shader rebuilds each pixel's world position from depth with the inverse view projection. Distance is eye to that point, and the height factor is taken once at the point. The sky has no special case: depth 1 gives a high point, so it stays clear.
Why: I rejected linear depth because the fog floated when the camera turned. An integral along the ray was rejected as harder to explain.
Cost and revisit: From far above, the floor gets too much fog. I reopen it if play moves to high places. Earlier: density 0.10, now 0.11 (see darker-night-defaults).
