# Two wall looks are models of their own, and only their top edge differs
Status: accepted (2026-10-09)
Code: src/game/WallVariants.cpp, src/game/MazeRenderer.cpp, tools/blender/build_wall_straight.py

Context: The top of a wall is what the player sees most, against the stars, and it was the same straight line on every wall. The maze is a hedge that went to stone. The record on worn walls keeps niches painted, because collision stays a box.
Decision: Crowned walls carry twigs of stone on the coping, and Broken walls have lost a coping stone with the next one askew. Both are separate models with the plain stone. Of the walls that got no painted look, 24 in 100 are Crowned and 12 in 100 are Broken, which is about one wall in six and one in twelve of a maze. A second generator draws them, so the painted walls of a seed stay where they were. Walls near the start, walls with a lever or note, and walls a lever opens keep their top whole.
Why: Everything new is above or inside the 3 m collision box, so nothing looks open and still blocks. I rejected crowns on painted walls too, because one look per wall keeps one enum and one draw call. A wall a lever opens sinks 3.3 m and the twigs reach 3.64 m, so they would stand in the opening.
Cost and revisit: A crowned wall is 414 triangles and a plain one 30, so a 22 by 22 maze draws about 54 000 wall triangles in place of 16 000. Every crown is the same row of twigs, turned by the direction of its wall. I reopen it if the repeat shows, with a second crown model.
