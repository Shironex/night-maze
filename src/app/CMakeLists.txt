# The application: the entry point, the class that joins the game with the debug UI
# (main.cpp) and the class that holds the game together (NightMazeApp). It needs a window
# and an OpenGL context. It may include every folder of src/, and no other folder includes
# it.
target_sources(night_maze PRIVATE
    AppShaders.cpp
    AppShaders.hpp
    FrameState.hpp
    MenuState.hpp
    NightMazeApp.cpp
    NightMazeApp.hpp
    NightMazeAppCampaign.cpp
    NightMazeAppMenus.cpp
    NightMazeAppPasses.cpp
    NightMazeAppRender.cpp
    NightMazeAppRound.cpp
    NightMazeAppScreens.cpp
    NightMazeAppShared.hpp
    RenderSettings.hpp
    SceneRenderers.cpp
    SceneRenderers.hpp
    main.cpp
)
