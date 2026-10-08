# The flashlight shadow bias moves the point in world space, in metres
Status: accepted (2026-10-06)
Code: assets/shaders/common/shadows.glsl, src/game/Shadows.cpp, tests/ShadowTests.cpp

Context: The flashlight map is a perspective projection, so one depth step means 2 cm near the hand and about 2 m at 10 m.
Decision: The bias stays in metres up to the shader, which moves the point towards the light by that length before projecting it. Defaults are 0.01 plus 0.13 times one minus the cosine.
Why: I rejected a depth constant as for the moon because the bias would grow from centimetres to metres along the beam, so no slider could be read in metres. Moving the point keeps it on the same texel ray.
Cost and revisit: The shader pays a length and a divide. The bias suits ground up to 10 m, the current range. I reopen it with a longer range.
