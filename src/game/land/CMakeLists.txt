target_sources(game_logic PRIVATE
    Grass.cpp
    Grass.hpp
    Land.cpp
    Land.hpp
    LandMeshes.cpp
    LandMeshes.hpp
    Puddles.cpp
    Puddles.hpp
    Terrain.cpp
    Terrain.hpp
    Village.cpp
    Village.hpp
)

target_sources(night_maze PRIVATE
    GrassRenderer.cpp
    GrassRenderer.hpp
    LandRenderer.cpp
    LandRenderer.hpp
    PuddleRenderer.cpp
    PuddleRenderer.hpp
    TerrainRenderer.cpp
    TerrainRenderer.hpp
    VillageRenderer.cpp
    VillageRenderer.hpp
)
