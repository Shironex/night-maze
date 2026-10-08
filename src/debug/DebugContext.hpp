// Debug context: the data the debug window may read or edit during one frame.
// See docs/modules/debug-ui.md
#pragma once

#include <array>
#include <cstddef>

namespace assets {
class AssetCache;
} // namespace assets

namespace audio {
class AudioEngine;
} // namespace audio

namespace core {
class Time;
class Window;
} // namespace core

namespace game {
enum class GameMode;
enum class ViewMode;
struct EnvironmentSettings;
struct GameplaySettings;
struct GrassSettings;
struct LightingSettings;
struct MazeSettings;
struct MazeWorld;
struct MenuCameraSettings;
class MinimapRenderer;
struct MinimapSettings;
struct PickDebugSettings;
struct PickState;
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

/// Everything the debug window and the HUD may read or edit this frame.
///
/// A plain struct of references into objects owned by the application. main.cpp builds it
/// every frame and passes it to DebugUI::draw. It owns nothing and must not outlive the
/// frame it was built in.
///
/// A const member is read only for the debug window, a non-const member can be edited
/// by it.
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
    /// Diagnostics category reloads it.
    gfx::Shader& texturedShader;
    /// Shader program of the lines of the collision boxes and spheres, editable:
    /// reloaded like texturedShader.
    gfx::Shader& colorShader;
    /// The player, editable: position, speeds and the noclip mode.
    game::Player& player;
    /// Request for the next maze, editable: size, seed, the wanted numbers of levers and
    /// notes and the "regenerate" flag.
    game::MazeSettings& mazeSettings;
    /// The maze in play, read only: its plan, its collision boxes and its terrain.
    const game::MazeWorld& mazeWorld;
    /// Loaded models and textures, editable: the Render category changes the filtering.
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
    /// The numbers of the rules of a round and the requests for a restart and for
    /// pulling every lever, editable.
    game::GameplaySettings& gameplay;
    /// The round in play: the HUD and the debug window show it. Editable for one thing,
    /// the charge of the battery (the Gameplay category).
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
    /// built: the debug window only shows it.
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
    /// copied when the context is built: the Light category only shows it.
    bool flashlightShadowDrawn;
    /// Shader program that draws the minimap into its framebuffer, editable: reloaded
    /// like texturedShader.
    gfx::Shader& minimapShader;
    /// Shader program that puts the picture of the minimap into the window, editable:
    /// reloaded like texturedShader.
    gfx::Shader& minimapOverlayShader;
    /// The settings of the minimap (pin, reveal all, size, opacity), editable.
    game::MinimapSettings& minimapSettings;
    /// The minimap, read only: the size, the format and the picture of its framebuffer.
    const game::MinimapRenderer& minimap;
    /// Shader program of the surfaces that show the sky (crystals and puddles),
    /// editable: reloaded like texturedShader.
    gfx::Shader& reflectShader;
    /// The settings of the environment mapping (the sky on the crystals and on the
    /// puddles), editable.
    game::EnvironmentSettings& environment;
    /// How many puddles lie in the maze. A plain number, copied when the context is
    /// built: the debug window only shows it.
    std::size_t puddleCount;
    /// The picking of the last frame, read only: the ray, the lever or the note it hit
    /// and what the interaction key does. The HUD draws the crosshair and the prompt
    /// from it, the Diagnostics category shows its numbers.
    const game::PickState& pick;
    /// The switches of the debug view of the picking (draw the pick boxes and the ray,
    /// freeze the drawn ray), editable.
    game::PickDebugSettings& pickDebug;
    /// The settings of the menu camera, editable: the Player category edits them, and
    /// while the camera runs the HUD is not drawn.
    game::MenuCameraSettings& menuCamera;
    /// Whether the HUD of the round is drawn in this frame. The game decides: only
    /// while a round is played and the menu camera is off (NightMazeApp::hudVisible).
    bool hudVisible;
    /// Whether the map is on the screen in this frame (NightMazeApp::mapOnScreen). The
    /// HUD then leaves out its hint lines, so the strip does not reach into the map.
    bool mapOnScreen;
    /// How long one loop of the menu camera takes, in seconds: the Player category shows
    /// it.
    float menuCameraLoopSeconds;
    /// The screen the game is on (main menu, playing, paused, round end). A plain value,
    /// copied when the context is built: the status strip of the debug window shows it.
    game::GameMode gameMode;
    /// The sound device and the loaded sounds, read only: the Diagnostics category
    /// shows its status and the loudness in use.
    const audio::AudioEngine& audio;
    /// The name of the sound cue that was played last ("none" before the first one) and
    /// how many were played since the start. Plain values, copied when the context is
    /// built: the Diagnostics category only shows them.
    const char* lastCueName;
    int cuesPlayed;
    /// The master volume of the settings, 0 to 100. A plain value, copied when the
    /// context is built: the Diagnostics category only shows it.
    float masterVolume;
    /// The request to play the intro again, editable: the Gameplay category sets it to
    /// true and the game starts the intro at the start of its next frame
    /// (NightMazeApp::introRequest).
    bool& playIntro;
};

} // namespace debug
