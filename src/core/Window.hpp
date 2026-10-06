// RAII wrapper over a GLFW window with an OpenGL 4.1 Core context.
// See docs/modules/core/window-context.md
#pragma once

#include <string>

// Forward declaration, so that including this header does not pull in the GLFW header.
struct GLFWwindow;

namespace core {

/// Width and height in pixels or in screen coordinates, depending on the function.
struct Size {
    int width = 0;
    int height = 0;
};

/// Owns the GLFW library state, one window and its OpenGL context.
///
/// The constructor initializes GLFW, creates the window, makes its context current and
/// loads the OpenGL functions through GLAD. The destructor releases all of it.
/// The object cannot be copied or moved: there is exactly one window for the whole run.
class Window {
public:
    /// Creates the window. Throws std::runtime_error if GLFW, the OpenGL 4.1 Core context
    /// or the OpenGL function loader cannot be set up.
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// True once the user (or requestClose) asked the window to close.
    bool shouldClose() const;

    /// Marks the window as closing, the main loop ends after the current frame.
    void requestClose();

    /// Processes pending operating system events (keyboard, resize, close button).
    void pollEvents();

    /// Shows the frame that was just rendered (swaps the back and front buffers).
    void swapBuffers();

    /// Size of the drawable area in pixels. Use this for glViewport.
    /// On a Retina display it is twice the window size.
    Size framebufferSize() const;

    /// Size of the window in screen coordinates. Mouse positions use these units.
    Size windowSize() const;

    /// Gives the window this size, in screen coordinates, and puts it in the middle of
    /// its screen. While the window is fullscreen nothing changes on the screen: the
    /// size is remembered and used when fullscreen is switched off. Does nothing when
    /// the window has that size already.
    void setWindowedSize(Size size);

    /// Switches between a window and fullscreen. Fullscreen covers the whole screen the
    /// window is on, at the resolution that screen has at that moment: the video mode
    /// of the desktop is not changed, so nothing else on the desktop moves. Switching
    /// back gives the window the place and the size it had before. Does nothing when
    /// the state does not change.
    void setFullscreen(bool fullscreen);

    /// True while the window is fullscreen (setFullscreen).
    bool isFullscreen() const { return m_fullscreen; }

    /// The resolution of the screen the window is on, in screen coordinates: the largest
    /// window that fits on it. A size of 0 x 0 when it cannot be asked.
    Size desktopSize() const;

    /// True while the window is the active one of the desktop: the one the keyboard
    /// types into. False after the player has switched to another program.
    bool isFocused() const;

    /// The underlying GLFW handle, for code that must talk to GLFW directly.
    GLFWwindow* nativeHandle() const { return m_handle; }

    /// OpenGL version string reported by the driver (GL_VERSION).
    const std::string& glVersion() const { return m_glVersion; }

    /// Name of the graphics card as reported by the driver (GL_RENDERER).
    const std::string& glRenderer() const { return m_glRenderer; }

private:
    GLFWwindow* m_handle = nullptr;
    std::string m_glVersion;
    std::string m_glRenderer;

    // Fullscreen: whether it is on, and where the window was and how large when it was
    // switched on, in screen coordinates. That is what switching it off goes back to.
    bool m_fullscreen = false;
    int m_windowedX = 0;
    int m_windowedY = 0;
    Size m_windowedSize;
};

} // namespace core
