# Grass is lit per fragment with a fixed up normal, in one program for every mode
Status: accepted (2026-10-05)
Code: assets/shaders/grass.frag, src/game/GrassRenderer.cpp

Context: Blades are made in a geometry shader from one point per tuft, are flat, and are seen from both sides. The game has four shading modes.
Decision: One grass program computes diffuse light per fragment with computeLighting and the normal (0, 1, 0), with no specular. Unlit turns the light off, and Gouraud looks the same as Phong.
Why: I rejected the real blade normal because it is bright on one side and black on the other, so tufts flicker with wind and camera. Three programs like the walls would add nine shader files.
Cost and revisit: Gouraud does not show on grass, and low side light lights it like ground. I reopen it if grass grows tall.
