// The objects on the graphics card the game draws with: the loaded models and textures,
// the renderers of the domains, the targets of the passes and their timers.
#pragma once

#include "assets/AssetCache.hpp"
#include "game/land/GrassRenderer.hpp"
#include "game/land/LandRenderer.hpp"
#include "game/land/PuddleRenderer.hpp"
#include "game/land/TerrainRenderer.hpp"
#include "game/land/VillageRenderer.hpp"
#include "game/maze/MazeRenderer.hpp"
#include "game/menu/MenuBackground.hpp"
#include "game/menu/MenuBackgroundRenderer.hpp"
#include "game/minimap/MinimapRenderer.hpp"
#include "game/player/InteractableRenderer.hpp"
#include "game/round/GameplayRenderer.hpp"
#include "game/screens/StorybookRenderer.hpp"
#include "render/ColliderLines.hpp"
#include "render/GpuPasses.hpp"
#include "render/LightRig.hpp"
#include "render/PostProcess.hpp"
#include "render/ShadowMap.hpp"
#include "render/Skybox.hpp"

namespace game {

/// Every OpenGL object of the game but the shader programs (AppShaders). They need a live
/// context, so the application holds this struct as a member: created after the window and
/// destroyed before it.
///
/// Order: members are constructed top to bottom. The renderers of the maze, the round and
/// the terrain ask assets for their models and textures in their constructors, so they
/// come after it.
struct SceneRenderers {
    /// background is what the command line asked to show behind the main menu
    /// (StartOptions::menuBackground). Opening the video waits for its first frame.
    explicit SceneRenderers(MenuBackground background);

    assets::AssetCache assets;
    MazeRenderer mazeRenderer;
    GameplayRenderer gameplayRenderer;
    VillageRenderer villageRenderer;
    InteractableRenderer interactableRenderer;
    TerrainRenderer terrainRenderer;
    LandRenderer landRenderer;
    GrassRenderer grassRenderer;
    PuddleRenderer puddleRenderer;
    ColliderLines colliderLines;
    LightRig lightRig;
    Skybox skybox;
    /// The HDR framebuffer of the scene and the passes after the scene.
    PostProcess postProcess;
    /// The timer queries of the passes of a frame.
    GpuPassTimers gpuPasses;
    /// The depth texture the scene is drawn into from the direction of the moon.
    ShadowMap moonShadowMap;
    /// The depth texture the scene is drawn into from the flashlight.
    ShadowMap flashlightShadowMap;
    /// The framebuffer the minimap is drawn into and the buffer of its triangles.
    MinimapRenderer minimapRenderer;
    /// What is behind the main menu: the video loop, its still picture, or nothing of its
    /// own when the live scene is shown there.
    MenuBackgroundRenderer menuBackground;
    /// The layers of the storybook intro, loaded while it plays.
    StorybookRenderer storybookRenderer;
};

} // namespace game
