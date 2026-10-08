// Keyboard and mouse state: keys and buttons held or just pressed, mouse movement.
#include "core/Input.hpp"

#include <GLFW/glfw3.h>

namespace core {

Input::Input(GLFWwindow* window) : m_window(window) {
    static_assert(KEY_COUNT == GLFW_KEY_LAST + 1, "KEY_COUNT must cover every GLFW key code");
    static_assert(MOUSE_BUTTON_COUNT == GLFW_MOUSE_BUTTON_LAST + 1,
                  "MOUSE_BUTTON_COUNT must cover every GLFW mouse button");
}

void Input::update() {
    m_previous = m_current;
    // GLFW key codes start at GLFW_KEY_SPACE (32), lower values are not valid keys.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        m_current[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    m_mousePrevious = m_mouseCurrent;
    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        m_mouseCurrent[button] = glfwGetMouseButton(m_window, button) == GLFW_PRESS;
    }

    // Cursor position in screen coordinates, relative to the top left corner of the window.
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(m_window, &cursorX, &cursorY);
    if (m_skipNextMouseDelta) {
        // There is no trustworthy previous position: this is the first update, or the
        // cursor mode has just changed and the reported position may have jumped.
        // Report no movement instead of one huge step (the "first mouse" problem).
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
        m_skipNextMouseDelta = false;
    } else {
        m_mouseDeltaX = cursorX - m_cursorX;
        m_mouseDeltaY = cursorY - m_cursorY;
    }
    m_cursorX = cursorX;
    m_cursorY = cursorY;
}

bool Input::isKeyDown(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key];
}

bool Input::wasKeyPressed(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key] && !m_previous[key];
}

bool Input::isMouseButtonDown(int button) const {
    return !m_mouseBlocked && isValidMouseButton(button) && m_mouseCurrent[button];
}

bool Input::wasMouseButtonPressed(int button) const {
    return !m_mouseBlocked && isValidMouseButton(button) && m_mouseCurrent[button] &&
           !m_mousePrevious[button];
}

double Input::mouseDeltaX() const {
    return m_mouseBlocked ? 0.0 : m_mouseDeltaX;
}

double Input::mouseDeltaY() const {
    return m_mouseBlocked ? 0.0 : m_mouseDeltaY;
}

CursorPosition Input::cursorPosition() const {
    if (m_mouseBlocked) {
        return {};
    }
    return {.valid = true, .x = m_cursorX, .y = m_cursorY};
}

void Input::setCursorCaptured(bool captured) {
    if (captured == m_cursorCaptured) {
        return;
    }
    m_cursorCaptured = captured;

    // GLFW_CURSOR_DISABLED hides the cursor and gives unlimited virtual movement,
    // GLFW_CURSOR_NORMAL is the ordinary visible cursor.
    glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    // Raw motion skips the system's pointer acceleration, which suits mouse look.
    // Not every platform has it, and it only has an effect while the cursor is disabled.
    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);
    }

    // Changing the cursor mode can make the reported position jump, so the next update
    // must not turn that jump into mouse movement.
    m_skipNextMouseDelta = true;
}

bool Input::isValidKey(int key) const {
    return key >= 0 && key < KEY_COUNT;
}

bool Input::isValidMouseButton(int button) const {
    return button >= 0 && button < MOUSE_BUTTON_COUNT;
}

} // namespace core
