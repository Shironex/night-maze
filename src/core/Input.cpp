// Keyboard state: which keys are held and which were pressed this frame.
// See docs/modules/core/input.md
#include "core/Input.hpp"

#include <GLFW/glfw3.h>

namespace core {

Input::Input(GLFWwindow* window) : m_window(window) {
    static_assert(KEY_COUNT == GLFW_KEY_LAST + 1, "KEY_COUNT must cover every GLFW key code");
}

void Input::update() {
    m_previous = m_current;
    // GLFW key codes start at GLFW_KEY_SPACE (32), lower values are not valid keys.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        m_current[key] = glfwGetKey(m_window, key) == GLFW_PRESS;
    }
}

bool Input::isKeyDown(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key];
}

bool Input::wasKeyPressed(int key) const {
    return !m_keyboardBlocked && isValidKey(key) && m_current[key] && !m_previous[key];
}

bool Input::isValidKey(int key) const {
    return key >= 0 && key < KEY_COUNT;
}

} // namespace core
