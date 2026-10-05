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
struct Round;
struct SkyboxSettings;
struct TerrainSettings;
} // namespace game

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct Camera;
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
};

} // namespace debug
