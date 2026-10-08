# The maze stands on gentle terrain, with walls sunk to the lowest ground under them
Status: accepted (2026-10-05)
Code: src/game/Terrain.hpp, src/game/Terrain.cpp, src/game/MazeWorld.cpp, src/game/TerrainRenderer.cpp, assets/textures/ground.png

Context: The ground had to be a heightmap the player walks on, but walls are rigid, equally tall and use box colliders.
Decision: One terrain mesh replaced the floor tiles. Relief is 0.6 m under the maze and rises to 4.5 m hills outside. Each wall, pillar and the gate is lowered to the lowest grid corner under its footprint plus 0.05 m, collider included.
Why: Flat triangles never fall below their lowest corner, so no gap can show. Steep hills inside the maze were rejected: walls would need shaping to the slope, and the boxes would break.
Cost and revisit: Walls are buried on one side. The height scale is capped at 2.5. I reopen it for jumping or steep ground.
