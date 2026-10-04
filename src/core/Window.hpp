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
};

} // namespace core
