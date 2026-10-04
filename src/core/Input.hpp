// Keyboard state: which keys are held and which were pressed this frame.
// See docs/modules/core/input.md
#pragma once

#include <array>

struct GLFWwindow;

namespace core {

/// Snapshot of the keyboard, refreshed once per frame.
///
/// Keys are identified by the GLFW key constants (GLFW_KEY_W, GLFW_KEY_ESCAPE, ...).
class Input {
public:
    /// The window whose keyboard is read. It must outlive this object.
    explicit Input(GLFWwindow* window);

    /// Reads the current state of every key. Call once per frame, after polling events.
    void update();

    /// True for as long as the key is held down.
    bool isKeyDown(int key) const;

    /// True only in the frame in which the key went from released to pressed.
    /// Use it only from code that runs once per frame (onRender), not from onUpdate,
    /// which runs zero or more times per frame. isKeyDown is safe in both.
    bool wasKeyPressed(int key) const;

    /// Blocks or unblocks the keyboard. While blocked, isKeyDown and wasKeyPressed return
    /// false for every key. Used when something else owns the keyboard, for example a
    /// text field of the debug UI, so that typing does not also control the game.
    /// core/ does not know who blocks it: the caller decides.
    void setKeyboardBlocked(bool blocked) { m_keyboardBlocked = blocked; }

private:
    // One slot per GLFW key code. 348 is GLFW_KEY_LAST, checked in Input.cpp, so that
    // this header does not need to include the GLFW header.
    static constexpr int KEY_COUNT = 348 + 1;

    bool isValidKey(int key) const;

    GLFWwindow* m_window;
    std::array<bool, KEY_COUNT> m_current{};  // key states in this frame
    std::array<bool, KEY_COUNT> m_previous{}; // key states in the previous frame
    bool m_keyboardBlocked = false;
};

} // namespace core
