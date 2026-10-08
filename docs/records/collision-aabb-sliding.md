# Collisions are own boxes moved one axis at a time, with no physics engine
Status: accepted (2026-10-05)
Code: src/scene/Collider.hpp, src/scene/Collider.cpp, src/game/Player.cpp, tests/ColliderTests.cpp

Context: The player must slide along walls. The world is only axis-aligned walls and pillars, and the ground is read, not collided with.
Decision: A small box test and a function that moves the player's box along x, then z, then y, measuring the free distance on each axis. Spheres give yes or no answers only.
Why: A physics library means thousands of lines I cannot explain for problems this world lacks. Move, test and push back is simpler but tunnels on long steps. I never wrote it, so that is reasoning, not measurement.
Cost and revisit: The path is stair-stepped, so steps must be short, and a 1 mm overlap is allowed. I reopen it for rotated obstacles, fast movers, jumping or steep ground.
