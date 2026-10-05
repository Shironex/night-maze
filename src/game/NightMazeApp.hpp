// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#pragma once

#include "assets/AssetCache.hpp"
#include "core/Application.hpp"
#include "game/ColliderLines.hpp"
#include "game/LightRig.hpp"
#include "game/Lighting.hpp"
#include "game/MazeRenderer.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "gfx/Buffer.hpp"
#include "gfx/Shader.hpp"
#include "gfx/VertexArray.hpp"
#include "scene/Camera.hpp"
#include "scene/Transform.hpp"

#include <glm/glm.hpp>

#include <array>

namespace game {

/// The game itself: a generated maze of textured walls, pillars and floor tiles at
/// night, and a player who walks through it in first person without passing through the
/// walls. The mouse turns the camera, the keyboard moves the player. The maze is lit by
/// the moon, by the flashlight of the player and by a point light in every dead end.
/// A coloured cube floats above the far corner cell as a marker of the future exit.
///
/// It knows nothing about the debug UI: main.cpp derives from this class and draws the
/// debug panels on top of the frame.
class NightMazeApp : public core::Application {
public:
    /// Creates the window (through core::Application), loads the shaders and the models,
    /// uploads the vertex and index data of the cube, generates the first maze and puts
    /// the player at its start.
    NightMazeApp();

protected:
    void onUpdate(double fixedDt) override;
    void onRender(double alpha) override;

    /// Background color (red, green, blue), exposed so the debug UI can edit it live.
    std::array<float, 3>& clearColor() { return m_clearColor; }

    /// Shader program of the cube, exposed so the debug UI can reload it live.
    gfx::Shader& shader() { return m_shader; }

    /// Shader program of the maze (textured models), exposed for the same reason.
    gfx::Shader& texturedShader() { return m_texturedShader; }

    /// Shader program of the collision box lines and of the light markers, exposed for
    /// the same reason.
    gfx::Shader& colorShader() { return m_colorShader; }

    /// Shader program of the lit maze with lighting per fragment (Phong and Blinn-Phong),
    /// exposed for the same reason.
    gfx::Shader& litShader() { return m_litShader; }

    /// Shader program of the lit maze with lighting per vertex (Gouraud), exposed for
    /// the same reason.
    gfx::Shader& gouraudShader() { return m_gouraudShader; }

    /// The settings of the lighting (mode, moon, flashlight, point lights, highlight),
    /// exposed so the debug UI can edit them live.
    LightingSettings& lighting() { return m_lighting; }

    /// The camera, exposed so the debug UI can show and edit its angles and projection
    /// live. Its position follows the eyes of the player, see player().
    scene::Camera& camera() { return m_camera; }

    /// Mouse look sensitivity in degrees per screen coordinate unit of mouse movement,
    /// exposed so the debug UI can edit it live.
    float& mouseSensitivity() { return m_mouseSensitivity; }

    /// The player, exposed so the debug UI can show and edit its position, its speeds
    /// and the noclip mode live.
    Player& player() { return m_player; }

    /// The request for the next maze, exposed so the debug UI can ask for a new one.
    MazeSettings& mazeSettings() { return m_mazeSettings; }

    /// The maze in play, read only: the debug UI draws its plan and counts its boxes.
    const MazeWorld& mazeWorld() const { return m_mazeWorld; }

    /// The loaded models and textures, exposed so the debug UI can list them and change
    /// the texture filtering live.
    assets::AssetCache& assets() { return m_assets; }

    /// What the textured shader shows, exposed so the debug UI can switch it live.
    ViewMode& viewMode() { return m_viewMode; }

    /// Whether the collision boxes are drawn as lines, exposed so the debug UI can
    /// switch it live.
    bool& drawColliders() { return m_drawColliders; }

private:
    // Camera turn for one screen coordinate unit of mouse movement, in degrees. The mouse
    // is measured in the units of the window size, not in framebuffer pixels, so the same
    // hand movement turns the camera equally on a Retina display.
    static constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1F;

    /// Builds the maze described by m_mazeSettings and enters it (enterMaze).
    void regenerateMaze();

    /// What has to happen whenever m_mazeWorld holds a new maze: moves the marker cube
    /// above its far corner cell and puts the player at its start, looking down an open
    /// passage.
    void enterMaze();

    /// The parts of a frame. Each one selects its own shader program and sets its
    /// uniforms. The maze has two: without lighting (the textured program, also used for
    /// the debug views of the normals and the texture coordinates) and with lighting.
    void drawMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawLitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawLightMarkers(const glm::mat4& view, const glm::mat4& projection) const;
    void drawCube(const glm::mat4& view, const glm::mat4& projection) const;
    void drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const;

    // The night sky: a very dark blue, darker than the ambient light on the stone, so
    // the walls stand out against it.
    std::array<float, 3> m_clearColor{0.01F, 0.015F, 0.04F};

    // OpenGL objects. They are members of a class derived from core::Application, so they
    // are created after the window and its OpenGL context, and destroyed before them.
    //
    // Order: members are constructed top to bottom, and the constructor body runs after
    // all of them.
    //   1. The five shader programs. They bind no buffer, so their place does not matter.
    //   2. m_assets, m_mazeRenderer (it loads the models through m_assets, so it comes
    //      after it), m_colliderLines and m_lightRig. The last three create meshes, and
    //      creating a mesh binds its own vertex array and buffers. They stand BEFORE the
    //      cube on purpose: the cube relies on its buffer still being bound when the
    //      constructor body runs, and a mesh created after it would take that binding
    //      away.
    //   3. m_vertexArray: its constructor binds it, so the two buffers below are created
    //      while it is the bound vertex array.
    //   4. m_vertexBuffer: stays bound to GL_ARRAY_BUFFER, and that is how the attribute
    //      setup in the constructor body tells the vertex array which buffer to read from.
    //   5. m_indexBuffer: binding it to GL_ELEMENT_ARRAY_BUFFER records it in the bound
    //      vertex array. It uses a different binding point, so m_vertexBuffer stays bound.
    gfx::Shader m_shader;
    gfx::Shader m_texturedShader;
    gfx::Shader m_colorShader;
    gfx::Shader m_litShader;
    gfx::Shader m_gouraudShader;
    assets::AssetCache m_assets;
    MazeRenderer m_mazeRenderer;
    ColliderLines m_colliderLines;
    LightRig m_lightRig;
    gfx::VertexArray m_vertexArray;
    gfx::Buffer m_vertexBuffer;
    gfx::Buffer m_indexBuffer;

    // Where the cube stands and how it is turned (the model matrix).
    scene::Transform m_cubeTransform;

    // The request for the next maze (edited by the debug UI) and the maze in play.
    MazeSettings m_mazeSettings;
    MazeWorld m_mazeWorld;

    // The player is simulation state: onUpdate moves it in fixed steps.
    Player m_player;
    // Position of the player before the last fixed step. onRender draws from a point
    // between this one and m_player.position. It starts equal to the position of the
    // player, so the frames before the first step are drawn from where the player stands.
    // Declared after m_player, because members are initialized top to bottom.
    glm::vec3 m_previousPlayerPosition = m_player.position;

    // Where the scene is seen from (the view and projection matrices). The angles are
    // turned by the mouse. The position is not controlled directly: after every fixed
    // step it is set to the eyes of the player.
    scene::Camera m_camera;

    // The lighting: how the maze is shaded and the settings of every light. The lights
    // of a frame are built from it in onRender.
    LightingSettings m_lighting;

    // What the textured shader shows: the picture, or one of the two debug views. A debug
    // view replaces the lighting: it is drawn with the textured program in every
    // lighting mode.
    ViewMode m_viewMode = ViewMode::Textured;
    // Whether the collision boxes are drawn as lines on top of the scene.
    bool m_drawColliders = false;

    // How the camera is turned. It belongs to the controls, not to the camera.
    float m_mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;
};

} // namespace game
