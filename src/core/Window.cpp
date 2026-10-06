// RAII wrapper over a GLFW window with an OpenGL 4.1 Core context.
// See docs/modules/core/window-context.md
#include "core/Window.hpp"

#include "core/Log.hpp"

// GLAD first: it declares the OpenGL API. GLFW_INCLUDE_NONE (set in CMake) stops GLFW
// from including the system OpenGL header on its own.
#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>

namespace core {

namespace {

// The least distance between the top edge of a screen and the top edge of the contents
// of a window that is placed by setWindowedSize, in screen coordinates: room for the
// title bar, which is above the contents.
constexpr int TITLE_BAR_ROOM = 32;

// GLFW calls this whenever one of its functions fails, with a readable description.
void onGlfwError(int code, const char* description) {
    logError("GLFW error " + std::to_string(code) + ": " + description);
}

// The screen most of the window is on, or the main screen when the window is on none
// (it was moved off every screen). nullptr only when no screen is connected at all.
//
// GLFW calls a screen a monitor. A window that is not fullscreen does not belong to one
// (glfwGetWindowMonitor returns nullptr for it), so the screen is found by comparing
// rectangles: the one that shares the largest area with the window.
GLFWmonitor* screenOf(GLFWwindow* window) {
    int windowX = 0;
    int windowY = 0;
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowPos(window, &windowX, &windowY);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    GLFWmonitor* best = glfwGetPrimaryMonitor();
    int bestArea = 0;
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; ++i) {
        int x = 0;
        int y = 0;
        glfwGetMonitorPos(monitors[i], &x, &y);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        if (mode == nullptr) {
            continue;
        }
        // The part of the window that lies on this screen: the overlap of two
        // rectangles, along x and along y. A negative number means no overlap.
        const int overlapX =
            std::min(windowX + windowWidth, x + mode->width) - std::max(windowX, x);
        const int overlapY =
            std::min(windowY + windowHeight, y + mode->height) - std::max(windowY, y);
        const int area = std::max(overlapX, 0) * std::max(overlapY, 0);
        if (area > bestArea) {
            bestArea = area;
            best = monitors[i];
        }
    }
    return best;
}

// glGetString returns unsigned bytes, std::string wants plain chars.
std::string glString(GLenum name) {
    const GLubyte* text = glGetString(name);
    if (text == nullptr) {
        return "unknown";
    }
    return reinterpret_cast<const char*>(text);
}

} // namespace

Window::Window(int width, int height, const std::string& title) {
    glfwSetErrorCallback(onGlfwError);

    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Ask for OpenGL 4.1 Core, the highest version available on macOS.
    // macOS only offers Core contexts that are forward compatible, so ask for the same
    // on Windows. GLFW up to 3.3 refused to create the window on macOS without this hint.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_handle == nullptr) {
        // The destructor does not run when a constructor throws, so clean up here.
        glfwTerminate();
        throw std::runtime_error("Failed to create a window with an OpenGL 4.1 Core context");
    }

    // OpenGL calls always go to the context that is current on the calling thread.
    glfwMakeContextCurrent(m_handle);

    // GLAD looks up the address of every OpenGL function through GLFW.
    // It returns 0 when the functions could not be loaded.
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        glfwDestroyWindow(m_handle);
        glfwTerminate();
        throw std::runtime_error("Failed to load the OpenGL functions (GLAD)");
    }

    // Vsync: wait for one screen refresh between buffer swaps.
    glfwSwapInterval(1);

    m_glVersion = glString(GL_VERSION);
    m_glRenderer = glString(GL_RENDERER);
    logInfo("GL_VERSION:  " + m_glVersion);
    logInfo("GL_RENDERER: " + m_glRenderer);
}

Window::~Window() {
    glfwDestroyWindow(m_handle);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_handle) == GLFW_TRUE;
}

void Window::requestClose() {
    glfwSetWindowShouldClose(m_handle, GLFW_TRUE);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_handle);
}

Size Window::framebufferSize() const {
    Size size;
    glfwGetFramebufferSize(m_handle, &size.width, &size.height);
    return size;
}

Size Window::windowSize() const {
    Size size;
    glfwGetWindowSize(m_handle, &size.width, &size.height);
    return size;
}

void Window::setWindowedSize(Size size) {
    if (size.width < 1 || size.height < 1) {
        return;
    }
    // The size it has already (or will have again after fullscreen): nothing to do, and
    // a window the player has moved stays where it is.
    const Size present = m_fullscreen ? m_windowedSize : windowSize();
    if (present.width == size.width && present.height == size.height) {
        return;
    }
    GLFWmonitor* screen = screenOf(m_handle);
    const GLFWvidmode* mode = screen != nullptr ? glfwGetVideoMode(screen) : nullptr;

    // The top left corner that puts a window of this size in the middle of its screen.
    // Without a screen to ask, the window stays where it is.
    int x = 0;
    int y = 0;
    glfwGetWindowPos(m_handle, &x, &y);
    if (mode != nullptr) {
        int screenX = 0;
        int screenY = 0;
        glfwGetMonitorPos(screen, &screenX, &screenY);
        x = screenX + (mode->width - size.width) / 2;
        // Never above the top edge of the screen: the title bar has to stay reachable
        // when the window is as high as the screen.
        y = screenY + std::max((mode->height - size.height) / 2, TITLE_BAR_ROOM);
    }

    if (m_fullscreen) {
        // Nothing to see yet: this is the window setFullscreen(false) will bring back.
        m_windowedX = x;
        m_windowedY = y;
        m_windowedSize = size;
        return;
    }
    glfwSetWindowSize(m_handle, size.width, size.height);
    glfwSetWindowPos(m_handle, x, y);
}

void Window::setFullscreen(bool fullscreen) {
    if (fullscreen == m_fullscreen) {
        return;
    }

    if (fullscreen) {
        GLFWmonitor* screen = screenOf(m_handle);
        const GLFWvidmode* mode = screen != nullptr ? glfwGetVideoMode(screen) : nullptr;
        if (mode == nullptr) {
            logError("Fullscreen is not possible: no screen was found");
            return;
        }
        // Where the window is now: the way back.
        glfwGetWindowPos(m_handle, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(m_handle, &m_windowedSize.width, &m_windowedSize.height);
        // A window with a monitor is fullscreen on it. Asking for the size and the
        // refresh rate the screen already has keeps its video mode as it is.
        glfwSetWindowMonitor(m_handle, screen, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        // A window without a monitor is an ordinary window again. The last argument is
        // the refresh rate, which only a fullscreen window has.
        glfwSetWindowMonitor(m_handle, nullptr, m_windowedX, m_windowedY, m_windowedSize.width,
                             m_windowedSize.height, GLFW_DONT_CARE);
    }
    m_fullscreen = fullscreen;
    // Some drivers forget the swap interval when the window changes its kind, so the
    // wait for the screen refresh is asked for again.
    glfwSwapInterval(1);
}

Size Window::desktopSize() const {
    GLFWmonitor* screen = screenOf(m_handle);
    const GLFWvidmode* mode = screen != nullptr ? glfwGetVideoMode(screen) : nullptr;
    if (mode == nullptr) {
        return {};
    }
    return {.width = mode->width, .height = mode->height};
}

bool Window::isFocused() const {
    return glfwGetWindowAttrib(m_handle, GLFW_FOCUSED) == GLFW_TRUE;
}

} // namespace core
