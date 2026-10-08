# Reflection defaults are chosen to be seen, not to match physics
Status: accepted (2026-10-06)
Code: src/game/EnvironmentMapping.hpp, src/game/PuddleRenderer.cpp, assets/shaders/reflect.frag

Context: The sky is dark and real water reflects about 2 percent, so a physical puddle two metres away shows almost nothing.
Decision: puddleReflectivity starts at 0.5, not 0.02, and the slider still reaches physical values. The puddle highlight is sharp (strength 1.0, shininess 128) and the water is light blue grey. A crystal shows a fixed 0.5 share of the sky and keeps its whole glow.
Why: The point is to show environment mapping. I rejected brightening the sky only in the reflection because the moon in a puddle would no longer be the moon above it.
Cost and revisit: A puddle reads more like a mirror than water. I reopen it if puddles look too bright; colour, opacity and reflectivity are three constants.
