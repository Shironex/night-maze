# Notes are chalk strokes on the wall, and the arrow of a hint leans along its wall
Status: accepted (2026-10-09)
Code: tools/blender/build_chalk.py, src/game/Interaction.hpp, src/game/Interaction.cpp, src/game/InteractableRenderer.cpp, src/game/Interactables.cpp

Context: The card of a note is headed "Chalk on the stone" and the story says the chalk leans towards the gate and the splinters, but the model was a sheet of paper. The loader reads no transparency.
Decision: A mark is geometry: strips 18 mm wide, 3 mm off the wall. A story line is a lamp, a hint is an arrow, and a crook is drawn 0.32 m to the left of every lever as a sign only. The arrow asks noteLean, the same function the text uses, in every frame. A direction with a part along the wall, diagonals too, leans right or left. Straight through the wall is up, straight behind the reader is down, and so is a splinter in the reader's own cell. A crystal hint with no crystal left shows the lamp. Every mark glows with CHALK_GLOW, a faint splinter colour, and adds no light.
Why: One mesh turned by 0, 90, 180 or 270 degrees covers every hint. A picture of the mark on a quad was rejected: without transparency it would be a pale rectangle. Eight arrow directions were rejected too, since a diagonal on a wall reads as up or down a slope.
Cost and revisit: From 10 m in the beam an arrow is one or two pixels thick and reads best with the lamp off. 3 mm showed no flicker at 10.9 m or at a 13 degree grazing angle. Walls with a note or a lever stay plain, so a mark never lies on a worn wall. I reopen it when moon-chalk arrives.
