// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#pragma once

#include "core/Application.hpp"
#include "gfx/Buffer.hpp"
#include "gfx/Shader.hpp"
#include "gfx/VertexArray.hpp"
#include "scene/Camera.hpp"
#include "scene/Transform.hpp"

#include <array>

namespace game {

/// The game itself. For now it clears the screen and draws one colored cube, seen through
/// a camera that does not move yet.
///
/// It knows nothing about the debug UI: main.cpp derives from this class and draws the
/// debug panels on top of the frame.
class NightMazeApp : public core::Application {
public:
    /// Creates the window (through core::Application), loads the shader and uploads
    /// the vertex and index data of the cube.
    NightMazeApp();

protected:
    void onUpdate(double fixedDt) override;
    void onRender(double alpha) override;

    /// Background color (red, green, blue), exposed so the debug UI can edit it live.
    std::array<float, 3>& clearColor() { return m_clearColor; }

    /// Shader program of the cube, exposed so the debug UI can reload it live.
    gfx::Shader& shader() { return m_shader; }

private:
    // A dark night blue.
    std::array<float, 3> m_clearColor{0.02F, 0.03F, 0.08F};

    // OpenGL objects. They are members of a class derived from core::Application, so they
    // are created after the window and its OpenGL context, and destroyed before them.
    //
    // Order: members are constructed top to bottom, and the constructor body runs after
    // all of them.
    //   1. m_vertexArray: its constructor binds it, so the two buffers below are created
    //      while it is the bound vertex array.
    //   2. m_vertexBuffer: stays bound to GL_ARRAY_BUFFER, and that is how the attribute
    //      setup in the constructor body tells the vertex array which buffer to read from.
    //   3. m_indexBuffer: binding it to GL_ELEMENT_ARRAY_BUFFER records it in the bound
    //      vertex array. It uses a different binding point, so m_vertexBuffer stays bound.
    gfx::Shader m_shader;
    gfx::VertexArray m_vertexArray;
    gfx::Buffer m_vertexBuffer;
    gfx::Buffer m_indexBuffer;

    // Where the cube stands and how it is turned (the model matrix).
    scene::Transform m_cubeTransform;
    // Where the scene is seen from (the view and projection matrices).
    scene::Camera m_camera;
};

} // namespace game
