// The settings of the picture that the debug UI edits live: plain data, read by the passes.
#pragma once

#include "game/land/Grass.hpp"
#include "game/land/Terrain.hpp"
#include "game/land/Village.hpp"
#include "game/lighting/EnvironmentMapping.hpp"
#include "game/lighting/Shadows.hpp"
#include "game/maze/MazeRenderer.hpp"
#include "game/minimap/Minimap.hpp"
#include "game/player/Interaction.hpp"
#include "render/PostProcess.hpp"
#include "render/Skybox.hpp"

#include <array>

namespace game {

/// How the frame is drawn, apart from the lighting: every member has a control in the debug
/// UI (main.cpp hands them over through debug::DebugContext). Nothing here is a rule of the
/// game or is saved.
struct RenderSettings {
    /// The colour every frame starts with: a very dark blue, darker than the ambient light
    /// on the stone. The sky is drawn over it wherever no wall is, so it shows only when
    /// the skybox is switched off or its pictures could not be loaded. It is close to the
    /// colour of the sky straight above, so switching the skybox off does not change the
    /// mood of the scene. An sRGB value, as the colour picker of the debug UI shows it:
    /// onRender converts it to linear for the HDR buffer.
    std::array<float, 3> clearColor{0.022F, 0.033F, 0.088F};

    /// The settings of the terrain: its height scale and its wireframe switch. The first
    /// maze is built with the height scale in the initializer list of the application.
    TerrainSettings terrainSettings;

    /// The settings of the grass. The tufts themselves are not kept: they are placed,
    /// copied to the graphics card and forgotten.
    GrassSettings grassSettings;

    /// The switches of the debug view of the picking: draw the pick boxes and the ray,
    /// freeze the drawn ray.
    PickDebugSettings pickDebug;

    /// The shadows of the moon: switch, resolution, bias, PCF, strength.
    ShadowSettings moonShadow;

    /// The same for the shadows of the flashlight. They start with a smaller map and a bias
    /// of their own (game::flashlightShadowDefaults).
    ShadowSettings flashlightShadow = flashlightShadowDefaults();

    /// What the textured shader shows: the picture, or one of the two debug views. A debug
    /// view replaces the lighting: it is drawn with the textured program in every lighting
    /// mode.
    ViewMode viewMode = ViewMode::Textured;
    /// Whether the collision boxes and spheres are drawn as lines on top of the scene.
    bool drawColliders = false;

    /// View frustum culling: whether a pass leaves out the objects that lie outside of what
    /// it sees. On by default. Off, every pass draws everything, which is the picture to
    /// compare with.
    bool frustumCulling = true;

    /// Whether the sky is drawn and how bright it is.
    SkyboxSettings skyboxSettings;

    /// Whether the village on the ridge is drawn and how high it stands.
    VillageSettings villageSettings;

    /// How the crystals and the puddles show the sky. The puddles themselves are not kept:
    /// they are placed, handed to SceneRenderers::puddleRenderer and forgotten, like the
    /// tufts of the grass.
    EnvironmentSettings environment;

    /// The exposure and the tone mapping of the composite pass and the settings of the
    /// bloom, the fog and the vignette.
    PostProcessSettings postProcessSettings;

    /// The map: whether it is pinned open, whether it shows the whole maze, and its size
    /// and its opacity.
    MinimapSettings minimapSettings;
};

} // namespace game
