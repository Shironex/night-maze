# Bloom is built from the unfogged scene, added after the fog, and crystals glow at strength 4
Status: accepted (2026-10-05)
Code: assets/shaders/post/composite.frag, src/game/PostProcess.cpp, src/game/Crystals.hpp

Context: The crystal is the goal, and the player must see its glow through the fog. Bloom also vanished at the darkest moment of the pulse.
Decision: The bloom is made from the scene before fog, and the composite shader mixes the fog first and adds the bloom after. CRYSTAL_GLOW_STRENGTH is 4.0, up from 2.5; the threshold stays 0.8 and the pulse depth 0.3.
Why: I rejected bloom from the fogged scene because distant crystals would fall under the threshold and lose the glow they exist for. I rejected a lower threshold because the whole scene shares it and the walls would light up.
Cost and revisit: I reopen it if I change the threshold, the pulse or the crystal texture.
