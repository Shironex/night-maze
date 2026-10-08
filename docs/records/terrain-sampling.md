# The heightmap tiles every 48 m of world and a height scale change rebuilds the terrain
Status: accepted (2026-10-05)
Code: src/game/Terrain.hpp, src/game/Terrain.cpp, tools/blender/make_heightmap.py

Context: One 256 by 256 image gives the heights, mazes range from 2 to 40 cells, and walls, crystals, grass and the player all stand on the ground.
Decision: The image coordinate is metres divided by HEIGHTMAP_SPAN (48) and repeats. A height scale change sets a flag, and the next frame rebuilds heights, mesh, walls, crystals, player and grass on the CPU.
Why: I rejected stretching the image over the terrain because hill size would change with the maze size. A vertex shader scale was rejected because heightAt would stop matching the drawn triangles and walls would float.
Cost and revisit: The image must tile, and big mazes repeat. A slider drag rebuilds a whole mesh, unmeasured. I reopen it if the repeat shows from above.
