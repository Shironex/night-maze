// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#include "game/NightMazeApp.hpp"

#include "core/GlCheck.hpp"
#include "core/Paths.hpp"
#include "game/ShaderUniforms.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <span>

namespace game {

namespace {

constexpr int INITIAL_WIDTH = 1280;
constexpr int INITIAL_HEIGHT = 720;

// Shader files, relative to the assets directory. The cube is drawn with the first pair,
// the maze with the second and the lines of the collision boxes with the third.
constexpr const char* VERTEX_SHADER_FILE = "shaders/basic.vert";
constexpr const char* FRAGMENT_SHADER_FILE = "shaders/basic.frag";
constexpr const char* TEXTURED_VERTEX_SHADER_FILE = "shaders/textured.vert";
constexpr const char* TEXTURED_FRAGMENT_SHADER_FILE = "shaders/textured.frag";
constexpr const char* COLOR_VERTEX_SHADER_FILE = "shaders/color.vert";
constexpr const char* COLOR_FRAGMENT_SHADER_FILE = "shaders/color.frag";

// The names of the uniforms (MODEL_UNIFORM, VIEW_UNIFORM, PROJECTION_UNIFORM and the
// others) are in game/ShaderUniforms.hpp, shared with the classes that draw the maze.

// Colours of the collision box lines (red, green, blue): the boxes of the maze in
// yellow, the box of the player in green.
constexpr glm::vec3 MAZE_COLLIDER_COLOR{1.0F, 0.85F, 0.1F};
constexpr glm::vec3 PLAYER_COLLIDER_COLOR{0.2F, 1.0F, 0.4F};

// Key that switches between walking and noclip (free flight).
constexpr int NOCLIP_KEY = GLFW_KEY_N;

// Pitch of a level look, in degrees: how the player looks at the start.
constexpr float LEVEL_PITCH_DEGREES = 0.0F;

// How high above the floor the centre of the marker cube floats, in metres: well above
// the walls (3 m) and the pillars (3.15 m), so it is seen over them from a distance.
constexpr float CUBE_HEIGHT_ABOVE_FLOOR = 4.5F;

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

// The cube is turned so that three of its faces are seen at once: tilted around the
// x axis, then turned around the y axis. It no longer stands in front of the camera: it
// floats above the far corner cell of the maze as a marker (see enterMaze).
constexpr float CUBE_ROTATION_X_DEGREES = 25.0F;
constexpr float CUBE_ROTATION_Y_DEGREES = 35.0F;

} // namespace

NightMazeApp::NightMazeApp()
    : core::Application(INITIAL_WIDTH, INITIAL_HEIGHT, "Night Maze"),
      m_shader(core::assetPath(VERTEX_SHADER_FILE), core::assetPath(FRAGMENT_SHADER_FILE)),
      m_texturedShader(core::assetPath(TEXTURED_VERTEX_SHADER_FILE),
                       core::assetPath(TEXTURED_FRAGMENT_SHADER_FILE)),
      m_colorShader(core::assetPath(COLOR_VERTEX_SHADER_FILE),
                    core::assetPath(COLOR_FRAGMENT_SHADER_FILE)),
      m_mazeRenderer(m_assets),
      // The sizes are in bytes: number of elements times the size of one element.
      m_vertexBuffer(GL_ARRAY_BUFFER, VERTICES.data(), VERTICES.size() * sizeof(float)),
      m_indexBuffer(GL_ELEMENT_ARRAY_BUFFER, INDICES.data(), INDICES.size() * sizeof(GLuint)),
      m_mazeWorld(
          buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed)) {
    // m_vertexBuffer is still bound to GL_ARRAY_BUFFER (m_indexBuffer uses another binding
    // point). Each call below records that buffer in m_vertexArray for one attribute.
    m_vertexArray.setFloatAttribute(POSITION_ATTRIBUTE, POSITION_COMPONENTS, VERTEX_STRIDE,
                                    POSITION_OFFSET);
    m_vertexArray.setFloatAttribute(COLOR_ATTRIBUTE, COLOR_COMPONENTS, VERTEX_STRIDE, COLOR_OFFSET);

    m_cubeTransform.rotationDegrees = {CUBE_ROTATION_X_DEGREES, CUBE_ROTATION_Y_DEGREES, 0.0F};

    // The first maze was built in the initializer list, because MazeWorld cannot be
    // created empty. What is left is the same as after every later regeneration.
    enterMaze();
}

void NightMazeApp::regenerateMaze() {
    // generateMaze throws for a size outside 1 to Maze::MAX_SIZE. The request comes from
    // a panel, where any number can be typed, so it is brought into the range here and
    // written back for the panel to show.
    m_mazeSettings.width = std::clamp(m_mazeSettings.width, 1, Maze::MAX_SIZE);
    m_mazeSettings.height = std::clamp(m_mazeSettings.height, 1, Maze::MAX_SIZE);

    // Replaces the maze, the model matrices and the collision boxes in one assignment.
    m_mazeWorld = buildMazeWorld(m_mazeSettings.width, m_mazeSettings.height, m_mazeSettings.seed);
    enterMaze();
}

void NightMazeApp::enterMaze() {
    // The marker cube floats above the far corner cell, the place of the future exit.
    m_cubeTransform.position =
        m_mazeWorld.exitPosition + glm::vec3{0.0F, CUBE_HEIGHT_ABOVE_FLOOR, 0.0F};

    // The player goes to the start. After a regeneration the old position may be inside
    // a wall of the new maze, or outside of it.
    m_player.position = m_mazeWorld.startPosition;
    // Both positions at once: otherwise the next frame would be drawn from a point
    // between the old place and the new one, a visible swoop through the walls.
    m_previousPlayerPosition = m_player.position;

    m_camera.position = m_player.eyePosition();
    m_camera.yawDegrees = m_mazeWorld.startYawDegrees;
    m_camera.pitchDegrees = LEVEL_PITCH_DEGREES;
}

void NightMazeApp::onUpdate(double fixedDt) {
    // Remember where the player was before this step. It is done in every step, also
    // when the player does not move, so that onRender never blends with an old position.
    m_previousPlayerPosition = m_player.position;

    // The keys reach the player only while the cursor is captured: one click in the scene
    // switches on both mouse look and movement, Escape switches both off. Without the
    // capture the struct stays as it is created: nothing is held.
    PlayerInput wanted;
    if (input().isCursorCaptured()) {
        wanted.forward = input().isKeyDown(GLFW_KEY_W);
        wanted.backward = input().isKeyDown(GLFW_KEY_S);
        wanted.left = input().isKeyDown(GLFW_KEY_A);
        wanted.right = input().isKeyDown(GLFW_KEY_D);
        wanted.up = input().isKeyDown(GLFW_KEY_SPACE);
        // Left Shift has one meaning per mode: sprint when walking, down when flying.
        // The player uses the field that belongs to its mode and ignores the other.
        wanted.down = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
        wanted.sprint = input().isKeyDown(GLFW_KEY_LEFT_SHIFT);
    }

    // The step runs also with nothing held: it is what brings the feet back to the floor
    // after noclip was switched off in a panel.
    m_player.update(wanted, m_camera.yawDegrees, m_camera.pitchDegrees, static_cast<float>(fixedDt),
                    m_mazeWorld.colliders);

    // Walking never changes the height, with one exception: the step right after noclip
    // was switched off in mid-air, which puts the feet back on the floor. That is a jump
    // and not a movement, so it must not be blended: without this line one frame would
    // be drawn from a point part of the way down.
    if (!m_player.noclip) {
        m_previousPlayerPosition.y = m_player.position.y;
    }

    // The camera stands where the eyes of the player are. onRender does not draw from
    // this position directly (it blends two steps), but the debug UI shows it.
    m_camera.position = m_player.eyePosition();
}

void NightMazeApp::onRender(double alpha) {
    // A new maze asked for by the debug UI is built here, at the start of a frame and
    // outside of the fixed steps, so no step ever sees a half replaced maze.
    if (m_mazeSettings.regenerate) {
        m_mazeSettings.regenerate = false;
        regenerateMaze();
    }

    // The noclip key. wasKeyPressed is true for one frame, so it is read here, once per
    // frame, and not in onUpdate, which runs zero or more times per frame.
    if (input().wasKeyPressed(NOCLIP_KEY)) {
        m_player.noclip = !m_player.noclip;
    }

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
    // already drawn at that pixel, so the near walls hide the far ones in whatever order
    // the triangles are drawn. It is switched on every frame, next to the other state
    // this frame relies on, instead of once at start-up: the frame then does not depend
    // on other code (the debug UI changes this state) leaving it switched on.
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

    // Width divided by height of the same pixels the viewport covers. The casts make it
    // a division of floats: 1280 / 720 as integers would be 1.
    const float aspectRatio =
        static_cast<float>(framebuffer.width) / static_cast<float>(framebuffer.height);

    // The simulation moves the player in fixed steps, and this frame is drawn at some
    // moment between two of them: alpha (0 to 1) tells how far. Drawing from a point
    // between the position before the last step and the position after it keeps the
    // movement smooth at any frame rate. m_player.position itself is not changed. The
    // eyes are a fixed height above the feet, so blending the feet and then going up
    // gives the same point as blending the eyes.
    const glm::vec3 feet =
        glm::mix(m_previousPlayerPosition, m_player.position, static_cast<float>(alpha));
    const glm::vec3 eye = feet + glm::vec3{0.0F, Player::EYE_HEIGHT, 0.0F};

    // The two matrices that are the same for everything drawn in this frame.
    const glm::mat4 view = m_camera.viewMatrix(eye);
    const glm::mat4 projection = m_camera.projectionMatrix(aspectRatio);

    drawMaze(view, projection);
    drawCube(view, projection);
    if (m_drawColliders) {
        drawColliderLines(view, projection);
    }
}

void NightMazeApp::drawMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // Without a shader program there is nothing to draw with. The load error was logged
    // once, when the shader was created, and the rest of the frame is still drawn.
    if (!m_texturedShader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    m_texturedShader.use();
    m_texturedShader.setMat4(VIEW_UNIFORM, view);
    m_texturedShader.setMat4(PROJECTION_UNIFORM, projection);
    // The enum values are the numbers textured.frag compares uViewMode with.
    m_texturedShader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));

    m_mazeRenderer.draw(m_texturedShader, m_mazeWorld);
}

void NightMazeApp::drawCube(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_shader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before setMat4.
    m_shader.use();
    m_shader.setMat4(MODEL_UNIFORM, m_cubeTransform.matrix());
    m_shader.setMat4(VIEW_UNIFORM, view);
    m_shader.setMat4(PROJECTION_UNIFORM, projection);

    m_vertexArray.bind();
    // Draws INDEX_COUNT indices from the element buffer recorded in the vertex array,
    // every three of them form one triangle. GL_UNSIGNED_INT is the type of one index
    // (GLuint). The last parameter has the type "pointer" for historical reasons, like in
    // glVertexAttribPointer: with an element buffer bound it is the byte offset of the
    // first index inside that buffer, and nullptr means offset 0, the start of the buffer.
    GL_CHECK(glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, nullptr));
}

void NightMazeApp::drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_colorShader.isValid()) {
        return;
    }

    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);

    // The depth test stays on: a line behind a wall is hidden by it, which shows where
    // each box really is. The box of the player is drawn at the simulation position (the
    // last fixed step), the camera at a blend of two steps, so while moving the box runs
    // ahead of the camera by a fraction of one step.
    m_colliderLines.draw(m_colorShader, m_mazeWorld.colliders, MAZE_COLLIDER_COLOR);
    // draw takes a list of boxes. A span made of a pointer and a count of 1 is a list
    // with this one box in it.
    const scene::Aabb playerBox = m_player.box();
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&playerBox, 1),
                         PLAYER_COLLIDER_COLOR);
}

} // namespace game
