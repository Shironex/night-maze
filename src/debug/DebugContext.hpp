// Debug context: the data the debug panels may read or edit during one frame.
// See docs/modules/debug-ui.md
#pragma once

#include <array>
#include <cstddef>

namespace assets {
class AssetCache;
} // namespace assets

namespace core {
class Time;
class Window;
} // namespace core

namespace game {
enum class ViewMode;
struct GameplaySettings;
struct GrassSettings;
struct LightingSettings;
struct MazeSettings;
struct MazeWorld;
struct Player;
class PostProcess;
struct PostProcessSettings;
struct Round;
class ShadowMap;
struct ShadowSettings;
struct SkyboxSettings;
struct TerrainSettings;
} // namespace game

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct Camera;
struct LightSpace;
} // namespace scene

namespace debug {

/// Everything the debug panels and the HUD may read or edit this frame.
///
/// A plain struct of references into objects owned by the application. main.cpp builds it
/// every frame and passes it to DebugUI::draw. It owns nothing and must not outlive the
/// frame it was built in.
///
/// A const member is read only for the panels, a non-const member can be edited by them.
/// A reference member keeps this meaning even when the struct itself is const: constness
/// of the struct does not pass through a reference to the object it refers to.
struct DebugContext {
    /// Frame clock, read only: FPS and frame time.
    const core::Time& time;
    /// Window, read only: sizes and OpenGL driver info.
    const core::Window& window;
    /// Clear color (red, green, blue in the range 0 to 1), editable: the background
    /// where the sky is not drawn.
    std::array<float, 3>& clearColor;
    /// Camera of the game, editable: angles and projection.
    scene::Camera& camera;
    /// Mouse look sensitivity in degrees per screen coordinate unit, editable.
    float& mouseSensitivity;
    /// Shader program of the scene without lighting (textured models), editable: the
    /// Shaders panel reloads it.
    gfx::Shader& texturedShader;
    /// Shader program of the lines of the collision boxes and spheres, editable:
    /// reloaded like texturedShader.
    gfx::Shader& colorShader;
    /// The player, editable: position, speeds and the noclip mode.
    game::Player& player;
    /// Request for the next maze, editable: size, seed and the "regenerate" flag.
    game::MazeSettings& mazeSettings;
    /// The maze in play, read only: its plan, its collision boxes and its terrain.
    const game::MazeWorld& mazeWorld;
    /// Loaded models and textures, editable: the Assets panel changes the filtering.
    assets::AssetCache& assets;
    /// What the textured shader shows (picture, normals or UVs), editable.
    game::ViewMode& viewMode;
    /// Whether the collision boxes and spheres are drawn as lines, editable.
    bool& drawColliders;
    /// Shader program of the lit scene, lighting per fragment, editable: reloaded like
    /// texturedShader.
    gfx::Shader& litShader;
    /// Shader program of the lit scene, lighting per vertex, editable: reloaded like
    /// texturedShader.
    gfx::Shader& gouraudShader;
    /// The lighting mode, the settings of every light and the normal mapping switch,
    /// editable.
    game::LightingSettings& lighting;
    /// The numbers of the rules of a round and the request for a restart, editable.
    game::GameplaySettings& gameplay;
    /// The round in play: the HUD and the panels show it. Editable for one thing, the
    /// charge of the battery (the Gameplay panel).
    game::Round& round;
    /// Shader program of the sky, editable: reloaded like texturedShader.
    gfx::Shader& skyboxShader;
    /// The switch and the brightness of the sky, editable.
    game::SkyboxSettings& skybox;
    /// Shader program of the grass, editable: reloaded like texturedShader.
    gfx::Shader& grassShader;
    /// The height scale and the wireframe switch of the terrain, editable.
    game::TerrainSettings& terrain;
    /// The switch, the density, the blade height and the wind of the grass, editable.
    game::GrassSettings& grass;
    /// How many tufts of grass are drawn. A plain number, copied when the context is
    /// built: the panels only show it.
    std::size_t grassTuftCount;
    /// Shader program of the composite pass, editable: reloaded like texturedShader.
    gfx::Shader& compositeShader;
    /// Shader program of the attachment previews, editable: reloaded like texturedShader.
    gfx::Shader& previewShader;
    /// The exposure and the tone mapping of the composite pass, the preview switch, the
    /// range of the depth preview and the settings of the bloom, the fog and the
    /// vignette, editable.
    game::PostProcessSettings& postProcessSettings;
    /// The framebuffers of the frame, read only: sizes, formats and the previews of
    /// the attachments of the scene framebuffer and of the bloom.
    const game::PostProcess& postProcess;
    /// Shader program of the bright pass of the bloom, editable: reloaded like
    /// texturedShader.
    gfx::Shader& brightPassShader;
    /// Shader program of the blur passes of the bloom, editable: reloaded like
    /// texturedShader.
    gfx::Shader& blurShader;
    /// Shader program of the depth pass of the shadow maps, editable: reloaded like
    /// texturedShader.
    gfx::Shader& shadowDepthShader;
    /// The settings of the shadows of the moon (switch, resolution, bias, PCF,
    /// strength) and the preview switch of its shadow map, editable.
    game::ShadowSettings& moonShadowSettings;
    /// The shadow map of the moon, read only: its size, its format and its preview.
    const game::ShadowMap& moonShadowMap;
    /// The view and the projection of the moon in the last frame, read only: how much
    /// ground its shadow map covers.
    const scene::LightSpace& moonLightSpace;
    /// The settings of the shadows of the flashlight and the preview switch of its
    /// shadow map, editable like the ones of the moon.
    game::ShadowSettings& flashlightShadowSettings;
    /// The shadow map of the flashlight, read only: its size, its format and its
    /// preview.
    const game::ShadowMap& flashlightShadowMap;
    /// The view and the projection of the flashlight in the last frame, read only: how
    /// much its shadow map covers.
    const scene::LightSpace& flashlightLightSpace;
    /// Whether the shadow map of the flashlight was drawn in the last frame: not with
    /// its shadows switched off, and not while the flashlight is off. A plain value,
    /// copied when the context is built: the Shadows panel only shows it.
    bool flashlightShadowDrawn;
};

} // namespace debug
