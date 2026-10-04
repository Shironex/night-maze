// RAII wrapper over a GLFW window with an OpenGL 4.1 Core context.
// See docs/modules/core/window-context.md
#include "core/Window.hpp"

#include "core/Log.hpp"

// GLAD first: it declares the OpenGL API. GLFW_INCLUDE_NONE (set in CMake) stops GLFW
// from including the system OpenGL header on its own.
#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace core {

namespace {

// GLFW calls this whenever one of its functions fails, with a readable description.
void onGlfwError(int code, const char* description) {
    logError("GLFW error " + std::to_string(code) + ": " + description);
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

} // namespace core
