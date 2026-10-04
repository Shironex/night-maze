// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#pragma once

#include "core/Application.hpp"
#include "gfx/Buffer.hpp"
#include "gfx/Shader.hpp"
#include "gfx/VertexArray.hpp"

#include <array>

namespace game {

/// The game itself. For now it clears the screen and draws one colored triangle.
///
/// It knows nothing about the debug UI: main.cpp derives from this class and draws the
/// debug panels on top of the frame.
class NightMazeApp : public core::Application {
public:
    /// Creates the window (through core::Application), loads the shader and uploads
    /// the vertex data.
    NightMazeApp();

protected:
    void onUpdate(double fixedDt) override;
    void onRender(double alpha) override;

    /// Background color (red, green, blue), exposed so the debug UI can edit it live.
    std::array<float, 3>& clearColor() { return m_clearColor; }

private:
    // A dark night blue.
    std::array<float, 3> m_clearColor{0.02F, 0.03F, 0.08F};

    // OpenGL objects. They are members of a class derived from core::Application, so they
    // are created after the window and its OpenGL context, and destroyed before them.
    //
    // Order: the vertex array first, then the vertex buffer. Members are constructed top
    // to bottom and the constructor body runs after all of them. At that point the buffer,
    // created last, is still bound to GL_ARRAY_BUFFER, and that is how the attribute setup
    // in the body tells the vertex array which buffer to read from.
    gfx::Shader m_shader;
    gfx::VertexArray m_vertexArray;
    gfx::Buffer m_vertexBuffer;
};

} // namespace game
