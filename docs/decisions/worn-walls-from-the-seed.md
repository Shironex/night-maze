# Worn wall looks are chosen per segment from the seed, and the niches are only painted
Status: accepted (2026-10-07)
Code: src/game/WallVariants.cpp, src/game/MazeRenderer.cpp, tools/blender/make_textures.py

Context: Every wall is one model. I wanted variety without new geometry, and one seed must give one maze.
Decision: chooseWallVariants draws a look per segment from the seed: Plain, Cracked, Mossy or Damaged. 35 percent of border walls and 20 percent of inner walls are worn. Walls within 2 cells of the start, and walls with a lever or note, stay plain. A worn picture must reflect as much light on average as the plain stone, within 5 percent, and tests/ImageLoaderTests.cpp checks it. The first mossy picture was damp and dark, 30 percent of the plain one, so every mossy segment was a black patch seen from above, and make_textures.py now scales it to the plain wall and paints the moss pale.
Why: I rejected cutting real niches because collision stays a box, so a wall would look open and still block you. A lever on a painted niche would seem to float. Moving a note changes no other wall.
Cost and revisit: A niche is under 5 cm of relief in the textures, flat in profile. I reopen it if walls get real geometry inside the collision box. The two looks with real geometry on the top edge are in crowned-and-broken-walls.
