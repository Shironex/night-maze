// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#include "game/NightMazeApp.hpp"

#include "core/GlCheck.hpp"
#include "core/Paths.hpp"

#include <cstddef>

namespace game {

namespace {

constexpr int INITIAL_WIDTH = 1280;
constexpr int INITIAL_HEIGHT = 720;

// Shader files, relative to the assets directory.
constexpr const char* VERTEX_SHADER_FILE = "shaders/basic.vert";
constexpr const char* FRAGMENT_SHADER_FILE = "shaders/basic.frag";

// Attribute numbers: the same as layout(location = N) in basic.vert.
constexpr GLuint POSITION_ATTRIBUTE = 0;
constexpr GLuint COLOR_ATTRIBUTE = 1;

// One vertex is a position (x, y, z) followed by a color (red, green, blue), all floats.
constexpr GLint POSITION_COMPONENTS = 3;
constexpr GLint COLOR_COMPONENTS = 3;
constexpr GLint FLOATS_PER_VERTEX = POSITION_COMPONENTS + COLOR_COMPONENTS;

// Stride: bytes from the start of one vertex to the start of the next one.
constexpr GLsizei VERTEX_STRIDE = static_cast<GLsizei>(FLOATS_PER_VERTEX * sizeof(float));
// Offsets: where each attribute starts inside one vertex, in bytes.
constexpr std::size_t POSITION_OFFSET = 0;
constexpr std::size_t COLOR_OFFSET = POSITION_COMPONENTS * sizeof(float);

constexpr GLsizei VERTEX_COUNT = 3;
// Number of floats in the whole vertex data.
constexpr int VERTEX_FLOAT_COUNT = VERTEX_COUNT * FLOATS_PER_VERTEX;

// Three vertices of one triangle, listed counter clockwise. There are no matrices yet, so
// the positions are normalized device coordinates: x and y from -1 to 1 cover the window.
constexpr std::array<float, VERTEX_FLOAT_COUNT> VERTICES = {
    // x, y, z,          red, green, blue
    -0.5F, -0.5F, 0.0F, 1.0F, 0.0F, 0.0F, // bottom left, red
    0.5F,  -0.5F, 0.0F, 0.0F, 1.0F, 0.0F, // bottom right, green
    0.0F,  0.5F,  0.0F, 0.0F, 0.0F, 1.0F, // top, blue
};

} // namespace

NightMazeApp::NightMazeApp()
    : core::Application(INITIAL_WIDTH, INITIAL_HEIGHT, "Night Maze"),
      m_shader(core::assetPath(VERTEX_SHADER_FILE), core::assetPath(FRAGMENT_SHADER_FILE)),
      // The size is in bytes: number of floats times the size of one float.
      m_vertexBuffer(GL_ARRAY_BUFFER, VERTICES.data(), VERTICES.size() * sizeof(float)) {
    // m_vertexBuffer has just been created, so it is still bound to GL_ARRAY_BUFFER.
    // Each call below records that buffer in m_vertexArray for one attribute.
    m_vertexArray.setFloatAttribute(POSITION_ATTRIBUTE, POSITION_COMPONENTS, VERTEX_STRIDE,
                                    POSITION_OFFSET);
    m_vertexArray.setFloatAttribute(COLOR_ATTRIBUTE, COLOR_COMPONENTS, VERTEX_STRIDE, COLOR_OFFSET);
}

void NightMazeApp::onUpdate(double /*fixedDt*/) {
    // No simulation yet.
}

void NightMazeApp::onRender(double /*alpha*/) {
    // The viewport is set in pixels, so it must come from the framebuffer size, which
    // differs from the window size on Retina displays. Querying it every frame also
    // handles window resizing.
    const core::Size framebuffer = window().framebufferSize();
    GL_CHECK(glViewport(0, 0, framebuffer.width, framebuffer.height));

    GL_CHECK(glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], 1.0F));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));

    // Without a shader program there is nothing to draw with. The load error was logged
    // once, when the shader was created, so the frame stays at the clear color.
    if (m_shader.isValid()) {
        m_shader.use();
        m_vertexArray.bind();
        // Every three vertices, starting at vertex 0, form one triangle.
        GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT));
    }
}

} // namespace game
