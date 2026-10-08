# Reflections have their own program and pass, and puddles follow the ground
Status: accepted (2026-10-06)
Code: assets/shaders/reflect.frag, src/game/NightMazeApp.cpp, src/game/Puddles.cpp

Context: Crystals and puddles show the night sky from the cube map, and puddles lie on uneven terrain.
Decision: A reflect program and a drawReflections pass, after the grass and before the sky, draw crystals and puddles. With the effect off, crystals use the wall program. Each puddle vertex lies 8 mm over the ground, in 6 rings of 32, with normals kept at (0, 1, 0).
Why: I rejected branches in the three wall shaders because every wall would pay for an effect on two object types. Earlier: a flat disc at the lowest ground hid up to 46 percent of some puddles.
Cost and revisit: Reflect repeats the lighting block of lit.frag. I reopen it if more objects reflect.
