target_sources(game_logic PRIVATE
    Maze.cpp
    Maze.hpp
    MazeGenerator.cpp
    MazeGenerator.hpp
    MazeLayout.cpp
    MazeLayout.hpp
    MazeWorld.cpp
    MazeWorld.hpp
    Stile.cpp
    Stile.hpp
    StoneSheep.cpp
    StoneSheep.hpp
    WallVariants.cpp
    WallVariants.hpp
)

target_sources(night_maze PRIVATE
    MazeRenderer.cpp
    MazeRenderer.hpp
)
