# The night is darker from 0.11.0, and the fog test caps the density
Status: accepted (2026-10-08)
Code: src/game/Lighting.hpp, src/game/Fog.hpp, src/game/Vignette.hpp, tests/FogTests.cpp

Context: Release 0.11.0 made the night darker: less light away from the flashlight, a dimmer moon, darker fog.
Decision: Ambient went from (0.105, 0.135, 0.225) to (0.055, 0.07, 0.115), moon intensity from 0.2 to 0.12, flashlight range from 16 m to 10 m, fog colour to (0.09, 0.105, 0.13), vignette strength from 0.3 to 0.45, and fog density from 0.10 to 0.11.
Why: I rejected much thicker fog because the ground four cells ahead must still show. A test requires 20 to 60 percent fog there, and 0.11 gives 59 percent.
Cost and revisit: Density cannot rise without changing that test. I reopen it if the corridors read as unplayable.
