// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#include "game/NightMazeApp.hpp"

#include "core/GlCheck.hpp"
#include "core/Paths.hpp"

#include <GLFW/glfw3.h>

#include <cstddef>

namespace game {

namespace {

constexpr int INITIAL_WIDTH = 1280;
constexpr int INITIAL_HEIGHT = 720;

// Shader files, relative to the assets directory.
constexpr const char* VERTEX_SHADER_FILE = "shaders/basic.vert";
constexpr const char* FRAGMENT_SHADER_FILE = "shaders/basic.frag";

// Names of the matrix uniforms: the same as the "uniform mat4" lines in basic.vert.
constexpr const char* MODEL_UNIFORM = "uModel";
constexpr const char* VIEW_UNIFORM = "uView";
constexpr const char* PROJECTION_UNIFORM = "uProjection";

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

// A cube has 6 faces. Each face is a square with its own 4 vertices, drawn as 2 triangles.
constexpr int FACE_COUNT = 6;
constexpr int VERTICES_PER_FACE = 4;
constexpr int INDICES_PER_FACE = 6;

constexpr int VERTEX_COUNT = FACE_COUNT * VERTICES_PER_FACE;
// Number of floats in the whole vertex data.
constexpr int VERTEX_FLOAT_COUNT = VERTEX_COUNT * FLOATS_PER_VERTEX;
constexpr GLsizei INDEX_COUNT = FACE_COUNT * INDICES_PER_FACE;

// A cube with a side of 1 (one metre), centred on the origin of its local space: every
// coordinate is -0.5 or 0.5. There are no matrices in the data, the positions are local.
//
// A cube has only 8 corners, but here it has 24 vertices: 4 for each face. A vertex is
// a position together with its color, and each face has its own flat color, so a corner
// shared by three faces is three different vertices. With 8 shared vertices the colors
// would be shared too and would blend across the faces. Normals and texture coordinates
// (M2) need vertices per face for the same reason.
//
// The 4 vertices of a face are listed counter clockwise as seen from outside the cube,
// starting at the bottom left corner: bottom left, bottom right, top right, top left.
constexpr std::array<float, VERTEX_FLOAT_COUNT> VERTICES = {
    // x, y, z,          red, green, blue
    -0.5F, -0.5F, 0.5F,  0.9F, 0.2F, 0.2F, // 0: front, z = +0.5, red, bottom left
    0.5F,  -0.5F, 0.5F,  0.9F, 0.2F, 0.2F, // 1: bottom right
    0.5F,  0.5F,  0.5F,  0.9F, 0.2F, 0.2F, // 2: top right
    -0.5F, 0.5F,  0.5F,  0.9F, 0.2F, 0.2F, // 3: top left
    0.5F,  -0.5F, -0.5F, 0.2F, 0.8F, 0.3F, // 4: back, z = -0.5, green, bottom left
    -0.5F, -0.5F, -0.5F, 0.2F, 0.8F, 0.3F, // 5: bottom right
    -0.5F, 0.5F,  -0.5F, 0.2F, 0.8F, 0.3F, // 6: top right
    0.5F,  0.5F,  -0.5F, 0.2F, 0.8F, 0.3F, // 7: top left
    -0.5F, -0.5F, -0.5F, 0.2F, 0.4F, 0.9F, // 8: left, x = -0.5, blue, bottom left
    -0.5F, -0.5F, 0.5F,  0.2F, 0.4F, 0.9F, // 9: bottom right
    -0.5F, 0.5F,  0.5F,  0.2F, 0.4F, 0.9F, // 10: top right
    -0.5F, 0.5F,  -0.5F, 0.2F, 0.4F, 0.9F, // 11: top left
    0.5F,  -0.5F, 0.5F,  0.9F, 0.8F, 0.2F, // 12: right, x = +0.5, yellow, bottom left
    0.5F,  -0.5F, -0.5F, 0.9F, 0.8F, 0.2F, // 13: bottom right
    0.5F,  0.5F,  -0.5F, 0.9F, 0.8F, 0.2F, // 14: top right
    0.5F,  0.5F,  0.5F,  0.9F, 0.8F, 0.2F, // 15: top left
    -0.5F, 0.5F,  0.5F,  0.2F, 0.8F, 0.8F, // 16: top, y = +0.5, cyan, bottom left
    0.5F,  0.5F,  0.5F,  0.2F, 0.8F, 0.8F, // 17: bottom right
    0.5F,  0.5F,  -0.5F, 0.2F, 0.8F, 0.8F, // 18: top right
    -0.5F, 0.5F,  -0.5F, 0.2F, 0.8F, 0.8F, // 19: top left
    -0.5F, -0.5F, -0.5F, 0.8F, 0.3F, 0.8F, // 20: bottom, y = -0.5, magenta, bottom left
    0.5F,  -0.5F, -0.5F, 0.8F, 0.3F, 0.8F, // 21: bottom right
    0.5F,  -0.5F, 0.5F,  0.8F, 0.3F, 0.8F, // 22: top right
    -0.5F, -0.5F, 0.5F,  0.8F, 0.3F, 0.8F, // 23: top left
};

// Indices: which vertices form each triangle. A face with the vertices a, b, c, d (in the
// order above) is split along the diagonal a-c into the triangles a, b, c and c, d, a.
// Both keep the counter clockwise order of the face.
constexpr std::array<GLuint, INDEX_COUNT> INDICES = {
    0,  1,  2,  2,  3,  0,  // front
    4,  5,  6,  6,  7,  4,  // back
    8,  9,  10, 10, 11, 8,  // left
    12, 13, 14, 14, 15, 12, // right
    16, 17, 18, 18, 19, 16, // top
    20, 21, 22, 22, 23, 20, // bottom
};

// The cube is turned so that the default camera sees three of its faces: tilted towards
// the camera around the x axis (the top comes into view), then turned around the y axis
// (the left side comes into view).
constexpr float CUBE_ROTATION_X_DEGREES = 25.0F;
constexpr float CUBE_ROTATION_Y_DEGREES = 35.0F;

} // namespace

NightMazeApp::NightMazeApp()
    : core::Application(INITIAL_WIDTH, INITIAL_HEIGHT, "Night Maze"),
      m_shader(core::assetPath(VERTEX_SHADER_FILE), core::assetPath(FRAGMENT_SHADER_FILE)),
      // The sizes are in bytes: number of elements times the size of one element.
      m_vertexBuffer(GL_ARRAY_BUFFER, VERTICES.data(), VERTICES.size() * sizeof(float)),
      m_indexBuffer(GL_ELEMENT_ARRAY_BUFFER, INDICES.data(), INDICES.size() * sizeof(GLuint)) {
    // m_vertexBuffer is still bound to GL_ARRAY_BUFFER (m_indexBuffer uses another binding
    // point). Each call below records that buffer in m_vertexArray for one attribute.
    m_vertexArray.setFloatAttribute(POSITION_ATTRIBUTE, POSITION_COMPONENTS, VERTEX_STRIDE,
                                    POSITION_OFFSET);
    m_vertexArray.setFloatAttribute(COLOR_ATTRIBUTE, COLOR_COMPONENTS, VERTEX_STRIDE, COLOR_OFFSET);

    m_cubeTransform.rotationDegrees = {CUBE_ROTATION_X_DEGREES, CUBE_ROTATION_Y_DEGREES, 0.0F};
}

void NightMazeApp::onUpdate(double fixedDt) {
    // Remember where the camera was before this step. It is done in every step, also
    // when the camera does not move, so that onRender never blends with an old position.
    m_previousCameraPosition = m_camera.position;

    // The camera is controlled only while the cursor is captured: one click in the scene
    // switches on both mouse look and movement, Escape switches both off.
    if (!input().isCursorCaptured()) {
        return;
    }

    // Free flight (there are no collisions yet): W and S move along the view direction,
    // so looking up while holding W also climbs. A and D move sideways, Space and Left
    // Shift move straight up and down. Opposite keys cancel each other.
    const glm::vec3 forward = m_camera.forward();
    const glm::vec3 right = m_camera.right();
    glm::vec3 direction{0.0F};
    if (input().isKeyDown(GLFW_KEY_W)) {
        direction += forward;
    }
    if (input().isKeyDown(GLFW_KEY_S)) {
        direction -= forward;
    }
    if (input().isKeyDown(GLFW_KEY_D)) {
        direction += right;
    }
    if (input().isKeyDown(GLFW_KEY_A)) {
        direction -= right;
    }
    if (input().isKeyDown(GLFW_KEY_SPACE)) {
        direction += scene::Camera::WORLD_UP;
    }
    if (input().isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        direction -= scene::Camera::WORLD_UP;
    }

    // Two keys at once give a vector longer than 1 (about 1.41 for W and D), which would
    // make diagonal movement faster. Normalizing brings the length back to 1. With no key
    // held the vector is zero and must be left alone: normalizing it divides by zero.
    if (glm::length(direction) > 0.0F) {
        direction = glm::normalize(direction);
    }

    // Distance of one step: metres per second times seconds.
    m_camera.position += direction * (m_moveSpeed * static_cast<float>(fixedDt));
}

void NightMazeApp::onRender(double alpha) {
    // Mouse look. It runs here, once per frame, and not in onUpdate: a click and a mouse
    // delta describe one frame, and onUpdate runs zero or more times per frame.
    if (!input().isCursorCaptured()) {
        // A click on a debug panel does not arrive here: main.cpp blocks the mouse for
        // the game while the debug UI is using it.
        if (input().wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            input().setCursorCaptured(true);
        }
    } else {
        // Mouse movement to the right is positive and positive yaw turns right, so x is
        // used as it is. Screen y grows downwards while pitch grows upwards, hence the
        // minus sign: moving the mouse up (negative y) looks up.
        const float yawDelta = static_cast<float>(input().mouseDeltaX()) * m_mouseSensitivity;
        const float pitchDelta = -static_cast<float>(input().mouseDeltaY()) * m_mouseSensitivity;
        m_camera.rotate(yawDelta, pitchDelta);
    }

    // The viewport is set in pixels, so it must come from the framebuffer size, which
    // differs from the window size on Retina displays. Querying it every frame also
    // handles window resizing.
    const core::Size framebuffer = window().framebufferSize();
    GL_CHECK(glViewport(0, 0, framebuffer.width, framebuffer.height));

    // Depth test: a fragment is kept only if it is nearer to the camera than what is
    // already drawn at that pixel, so the near faces of the cube hide the far ones in
    // whatever order the triangles are drawn. It is switched on every frame, next to the
    // other state this frame relies on, instead of once at start-up: the frame then does
    // not depend on other code (the debug UI changes this state) leaving it switched on.
    GL_CHECK(glEnable(GL_DEPTH_TEST));

    // The depth buffer has to be cleared together with the color, otherwise the depths of
    // the previous frame would hide the new one.
    GL_CHECK(glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], 1.0F));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // A minimized window can have a framebuffer of size 0 x 0. The aspect ratio would
    // then be 0 / 0, which is NaN (not a number): glm::perspective stops the program with
    // an assert in a Debug build and returns a matrix with NaN in it in a Release build.
    // A size of 0 in one direction only gives an aspect ratio of 0 or infinity, and
    // a matrix that is just as useless. There is nothing to draw in such a frame anyway.
    if (framebuffer.width == 0 || framebuffer.height == 0) {
        return;
    }

    // Without a shader program there is nothing to draw with. The load error was logged
    // once, when the shader was created, so the frame stays at the clear color.
    if (!m_shader.isValid()) {
        return;
    }

    // Width divided by height of the same pixels the viewport covers. The casts make it
    // a division of floats: 1280 / 720 as integers would be 1.
    const float aspectRatio =
        static_cast<float>(framebuffer.width) / static_cast<float>(framebuffer.height);

    // The simulation moves the camera in fixed steps, and this frame is drawn at some
    // moment between two of them: alpha (0 to 1) tells how far. Drawing from a point
    // between the position before the last step and the position after it keeps the
    // movement smooth at any frame rate. m_camera.position itself is not changed.
    const glm::vec3 eye =
        glm::mix(m_previousCameraPosition, m_camera.position, static_cast<float>(alpha));

    // The uniforms belong to the program in use, so use() comes before setMat4.
    m_shader.use();
    m_shader.setMat4(MODEL_UNIFORM, m_cubeTransform.matrix());
    m_shader.setMat4(VIEW_UNIFORM, m_camera.viewMatrix(eye));
    m_shader.setMat4(PROJECTION_UNIFORM, m_camera.projectionMatrix(aspectRatio));

    m_vertexArray.bind();
    // Draws INDEX_COUNT indices from the element buffer recorded in the vertex array,
    // every three of them form one triangle. GL_UNSIGNED_INT is the type of one index
    // (GLuint). The last parameter has the type "pointer" for historical reasons, like in
    // glVertexAttribPointer: with an element buffer bound it is the byte offset of the
    // first index inside that buffer, and nullptr means offset 0, the start of the buffer.
    GL_CHECK(glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, nullptr));
}

} // namespace game
