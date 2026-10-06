// Keyboard and mouse state: keys and buttons held or just pressed, mouse movement.
// See docs/modules/core/input.md
#pragma once

#include <array>

struct GLFWwindow;

namespace core {

/// Where the cursor is, the answer of Input::cursorPosition.
struct CursorPosition {
    /// False while the mouse is blocked: x and y mean nothing then.
    bool valid = false;

    /// The position in screen coordinates (the units of the window size, not framebuffer
    /// pixels), measured from the top left corner of the window: x grows to the right
    /// and y grows downwards.
    double x = 0.0;
    double y = 0.0;
};

/// Snapshot of the keyboard and the mouse, refreshed once per frame.
///
/// Keys are identified by the GLFW key constants (GLFW_KEY_W, GLFW_KEY_ESCAPE, ...),
/// mouse buttons by the GLFW button constants (GLFW_MOUSE_BUTTON_LEFT, ...).
class Input {
public:
    /// The window whose keyboard and mouse are read. It must outlive this object.
    explicit Input(GLFWwindow* window);

    /// Reads the current state of every key and mouse button and the cursor position.
    /// Call once per frame, after polling events.
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

    /// True for as long as the mouse button is held down.
    bool isMouseButtonDown(int button) const;

    /// True only in the frame in which the mouse button went from released to pressed.
    /// Use it only from code that runs once per frame (onRender), not from onUpdate,
    /// which runs zero or more times per frame. isMouseButtonDown is safe in both.
    bool wasMouseButtonPressed(int button) const;

    /// Horizontal cursor movement since the previous frame, in screen coordinates
    /// (the units of the window size, not framebuffer pixels). Positive is to the right.
    /// It is per frame data: use it only from code that runs once per frame (onRender),
    /// not from onUpdate. Returns 0 while the mouse is blocked.
    double mouseDeltaX() const;

    /// Vertical cursor movement since the previous frame, in screen coordinates.
    /// Positive is downwards: y grows towards the bottom of the screen.
    /// Same once-per-frame rule as mouseDeltaX. Returns 0 while the mouse is blocked.
    double mouseDeltaY() const;

    /// Where the cursor was when update read it. Pair it with the size of the WINDOW
    /// (core::Window::windowSize), never with the size of the framebuffer: on a Retina
    /// display the two differ. While the mouse is blocked the answer is not valid, the
    /// same rule as for the buttons: a cursor over a debug panel points at nothing in
    /// the scene. While the cursor is captured the position is a virtual one without
    /// limits and says nothing about a place in the window.
    CursorPosition cursorPosition() const;

    /// Blocks or unblocks the mouse. While blocked, isMouseButtonDown and
    /// wasMouseButtonPressed return false for every button, the mouse delta is 0 and
    /// cursorPosition is not valid.
    /// Used when something else owns the mouse, for example a panel of the debug UI under
    /// the cursor. core/ does not know who blocks it: the caller decides.
    void setMouseBlocked(bool blocked) { m_mouseBlocked = blocked; }

    /// Captures or releases the cursor. A captured cursor is hidden and locked to the
    /// window, and its movement is not limited by the screen edges (mouse look).
    /// Releasing shows the normal cursor again. Does nothing if the state does not change.
    void setCursorCaptured(bool captured);

    /// True while the cursor is captured, see setCursorCaptured.
    bool isCursorCaptured() const { return m_cursorCaptured; }

private:
    // One slot per GLFW key code. 348 is GLFW_KEY_LAST, checked in Input.cpp, so that
    // this header does not need to include the GLFW header.
    static constexpr int KEY_COUNT = 348 + 1;
    // One slot per GLFW mouse button. 7 is GLFW_MOUSE_BUTTON_LAST, checked in Input.cpp.
    static constexpr int MOUSE_BUTTON_COUNT = 7 + 1;

    bool isValidKey(int key) const;
    bool isValidMouseButton(int button) const;

    GLFWwindow* m_window;
    std::array<bool, KEY_COUNT> m_current{};  // key states in this frame
    std::array<bool, KEY_COUNT> m_previous{}; // key states in the previous frame
    bool m_keyboardBlocked = false;

    std::array<bool, MOUSE_BUTTON_COUNT> m_mouseCurrent{};  // button states in this frame
    std::array<bool, MOUSE_BUTTON_COUNT> m_mousePrevious{}; // button states in the previous frame
    double m_cursorX = 0.0; // cursor position read by the last update
    double m_cursorY = 0.0;
    double m_mouseDeltaX = 0.0; // cursor movement between the last two updates
    double m_mouseDeltaY = 0.0;
    // True when the next update has no valid previous cursor position to compare with:
    // before the first update and after the cursor mode has changed.
    bool m_skipNextMouseDelta = true;
    bool m_mouseBlocked = false;
    bool m_cursorCaptured = false;
};

} // namespace core
