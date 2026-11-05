# The helpers of the passes of a frame: they talk to OpenGL and hold no rule of the game, so
# they are part of the executable. They may read the plain data of game_logic, and only the
# drawing files of src/game, src/hud and src/debug include them.
target_sources(night_maze PRIVATE
    GpuPasses.cpp
    GpuPasses.hpp
    LightRig.cpp
    LightRig.hpp
    ShaderUniforms.hpp
)
