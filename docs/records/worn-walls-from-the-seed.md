# Worn wall looks are chosen per segment from the seed, and the niches are only painted
Status: accepted (2026-10-07)
Code: src/game/WallVariants.cpp, src/game/MazeRenderer.cpp, tools/blender/make_textures.py

Context: Every wall is one model. I wanted variety without new geometry, and one seed must give one maze.
Decision: chooseWallVariants draws a look per segment from the seed: Plain, Cracked, Mossy or Damaged. 35 percent of border walls and 20 percent of inner walls are worn. Walls within 2 cells of the start, and walls with a lever or note, stay plain.
Why: I rejected cutting real niches because collision stays a box, so a wall would look open and still block you. A lever on a painted niche would seem to float. Moving a note changes no other wall.
Cost and revisit: A niche is under 5 cm of relief in the textures, flat in profile. I reopen it if walls get real geometry.
