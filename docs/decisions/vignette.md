# The vignette is measured in texture coordinates and not corrected for the window shape
Status: accepted (2026-10-05)
Code: assets/shaders/post/composite.frag, src/game/Vignette.hpp, tests/VignetteTests.cpp

Context: The vignette darkens the picture towards the corners, and the window can be stretched to any shape.
Decision: The distance from the centre is length(uv - 0.5) with no aspect factor, and smoothstep runs from the radius setting to the corner distance 0.7071. The bright middle is an ellipse in the shape of the window.
Why: I rejected an aspect correction because in a wide window the sides reach full darkness while the top stays bright, and the corner distance would depend on the ratio. Without it the four corners are always equally dark.
Cost and revisit: It is not a circle like a lens. Defaults are now strength 0.45 and radius 0.4. I reopen it for ultrawide screens.
