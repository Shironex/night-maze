// Debug context: the data the debug panels may read or edit during one frame.
// See docs/modules/debug-ui.md
#pragma once

#include <array>

namespace assets {
class AssetCache;
} // namespace assets

namespace core {
class Time;
class Window;
} // namespace core

namespace game {
enum class ViewMode;
struct LightingSettings;
struct MazeSettings;
struct MazeWorld;
struct Player;
} // namespace game

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

/// Everything the debug panels may read or edit this frame.
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
    /// Background color (red, green, blue in the range 0 to 1), editable.
    std::array<float, 3>& clearColor;
    /// Shader program of the marker cube, editable: the Shaders panel reloads it.
    gfx::Shader& shader;
    /// Camera of the game, editable: angles and projection.
    scene::Camera& camera;
    /// Mouse look sensitivity in degrees per screen coordinate unit, editable.
    float& mouseSensitivity;
    /// Shader program of the maze (textured models), editable: reloaded like shader.
    gfx::Shader& texturedShader;
    /// Shader program of the collision box lines, editable: reloaded like shader.
    gfx::Shader& colorShader;
    /// The player, editable: position, speeds and the noclip mode.
    game::Player& player;
    /// Request for the next maze, editable: size, seed and the "regenerate" flag.
    game::MazeSettings& mazeSettings;
    /// The maze in play, read only: its plan and its collision boxes.
    const game::MazeWorld& mazeWorld;
    /// Loaded models and textures, editable: the Assets panel changes the filtering.
    assets::AssetCache& assets;
    /// What the textured shader shows (picture, normals or UVs), editable.
    game::ViewMode& viewMode;
    /// Whether the collision boxes are drawn as lines, editable.
    bool& drawColliders;
    /// Shader program of the lit maze, lighting per fragment, editable: reloaded like shader.
    gfx::Shader& litShader;
    /// Shader program of the lit maze, lighting per vertex, editable: reloaded like shader.
    gfx::Shader& gouraudShader;
    /// The lighting mode and the settings of every light, editable.
    game::LightingSettings& lighting;
};

} // namespace debug
