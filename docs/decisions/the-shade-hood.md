# The shade has the night in its hood, and the burn shows there
Status: accepted (2026-10-09)
Code: tools/blender/build_shade.py, assets/models/shade_hollow.obj, assets/shaders/reflect.frag, src/game/NightMazeApp.cpp, src/game/Shade.hpp

Context: the story says the shadow is the hole a fallen piece leaves in the moon, and the 2.5 s of the burn could only be heard.
Decision: the back of the hollow of the hood and the two cuffs are a model of their own, drawn in the reflections pass. They show the sky along the look of the eye, turned 0.35 rad about the vertical axis (`uSkyTurn`). At rest it has 0.6 times the brightness of the sky. Right before the banish it has 3 times, and what is brighter than empty sky is added 10 times more (`uStarBoost`), both on the square of the burn progress. The sleeves hang empty to the knees and the hem is torn into tongues, with the two lowest bands built once more facing inwards.
Why: two float uniforms on the reflect program were enough: a refraction ratio of 1 already reads the sky where the eye looks. Raising the whole sky 9 times read as a lamp behind blue glass, so the stars take most of the rise. The empty sky still rises a little, because at 10 m the hood seldom holds a star. I rejected a point light in the hood: the figure must stay dark, and the lights are counted.
Cost and revisit: the starry face is a slit 10 cm wide, a few pixels at 10 m. With reflections off it is dark cloth. I reopen it if the burn is not seen from the far end of the beam.
