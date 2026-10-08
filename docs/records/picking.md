# The pick ray starts in the eye, and the aimed object glows with a pulsing emissive
Status: accepted (2026-10-06)
Code: src/game/Interaction.hpp, src/game/Interaction.cpp, src/game/InteractableRenderer.cpp, src/scene/Raycast.cpp

Context: The screen ray begins on the near plane, up to 0.155 m ahead of the eye, but the reach of 2.5 m is meant from the eye. The aimed lever or note also needs a highlight.
Decision: rayFromEye keeps the direction and moves the origin to the eye. The aimed object is drawn with the existing emissive uniform, a warm yellow pulsing between 0.8 and 2.4 times its colour.
Why: The eye and the near plane point share one line, so the same pixel is picked. Adding 0.1 m to the reach was rejected, since the error differs per pixel.
Cost and revisit: The whole object lights, with no outline. I reopen it for larger, shaped objects.
