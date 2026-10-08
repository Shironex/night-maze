# The flashlight is held in the hand and aimed at a point in front of the eye
Status: accepted (2026-10-05)
Code: src/game/Lighting.cpp, src/game/Lighting.hpp, tests/LightingTests.cpp

Context: A light at the eye casts every shadow behind the thing that makes it, so its shadow map shows nothing.
Decision: The light sits 0.20 m right of the eye and 0.25 m straight down in the world, and the beam aims at a point 4 m ahead on the line of view. The right offset is capped at 0.25 m so the hand stays inside the 0.6 m body.
Why: I rejected a light at the eye because its shadows would never show. A beam parallel to the view was rejected because the spot would slide to the bottom right near walls.
Cost and revisit: The spot is centred at one distance only. I reopen it if the hand looks wrong in play.
