# Crystal lights follow the 16 crystals nearest the eye, fading at the edge of the set
Status: accepted (2026-10-06)
Code: src/game/Lighting.hpp, src/game/Lighting.cpp, src/game/Crystals.hpp, src/game/NightMazeApp.cpp, src/scene/Light.hpp

Context: The shaders take 16 point lights, but levels ask for up to 40 crystals and the limit is 64. Earlier: in M4 lights sat in dead ends until crystals replaced them.
Decision: Each frame the 16 crystals nearest the eye carry the lights. A light at the edge of the set has strength 0 and full strength 4 m inside it.
Why: Two lights that swap places are equally far, so both are dark at that moment and nothing pops. A 64-light block was rejected, because every fragment would loop over more lights. Unmeasured.
Cost and revisit: Far crystals give only glow, and nobody has watched the fade with over 16 crystals. I reopen it if it shows.
