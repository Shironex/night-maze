# The shade has the night in its hood, and the burn shows there
Status: accepted (2026-10-09)
Code: tools/blender/build_shade.py, assets/models/shade_hollow.obj, assets/shaders/reflect.frag, src/game/NightMazeApp.cpp, src/game/Shade.hpp

Context: the story says the shadow is the hole a fallen piece leaves in the moon, and the 2.5 s of the burn could only be heard.
Decision: the back of the hollow of the hood and the two cuffs are a model of their own, drawn in the reflections pass. They show the sky along the look of the eye, turned 0.65 rad about the vertical axis (`uSkyTurn`), at 0.6 times the brightness of the sky at rest and 9 times right before the banish, on the square of the burn progress. The sleeves hang empty to the knees and the hem is torn into tongues, with the two lowest bands built once more facing inwards.
Why: one float uniform on the reflect program was enough: a refraction ratio of 1 already reads the sky where the eye looks. I rejected a point light in the hood: the figure must stay dark, and the lights are counted.
Cost and revisit: the starry face is a slit 10 cm wide, a few pixels at 10 m, and at the end of the burn it reads as a blue light more than as stars. With reflections off it is dark cloth. I reopen it if the burn is not seen from the far end of the beam.
