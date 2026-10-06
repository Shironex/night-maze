// UiLayer: owns RmlUi and draws the documents of the menu on top of the frame.
#include "ui/UiLayer.hpp"

#include "core/Files.hpp"
#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "ui/AssetFileInterface.hpp"

#include <GLFW/glfw3.h>
#include <RmlUi/Core.h>
// The two backends of RmlUi that are used as they are: the renderer for OpenGL 3.3 and
// later, and the helper functions that translate GLFW events for RmlUi.
#include <RmlUi_Platform_GLFW.h>
#include <RmlUi_Renderer_GL3.h>

#include <cstddef>
#include <utility>

namespace ui {

namespace {

// The name of the RmlUi context: RmlUi can hold several, this program has one.
constexpr const char* CONTEXT_NAME = "menu";

// The attribute that makes an element report its clicks, and the event it reports.
constexpr const char* ACTION_ATTRIBUTE = "data-action";
constexpr const char* CLICK_EVENT = "click";

// The elements text is typed into: while one of them has the focus, the keyboard
// belongs to the document.
constexpr const char* INPUT_TAG = "input";
constexpr const char* TEXTAREA_TAG = "textarea";

// The system interface of the GLFW backend (time, clipboard, cursor shape), with one
// change: the messages of RmlUi go to the log of the game.
class LoggingSystemInterface final : public SystemInterface_GLFW {
public:
    using SystemInterface_GLFW::SystemInterface_GLFW;

    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
        const std::string text = "RmlUi: " + message;
        if (type == Rml::Log::LT_ERROR || type == Rml::Log::LT_ASSERT) {
            core::logError(text);
        } else if (type == Rml::Log::LT_WARNING) {
            core::logWarn(text);
        } else {
            core::logInfo(text);
        }
        // True: go on. False would ask RmlUi to break into the debugger.
        return true;
    }
};

// The listener of every click in the context. RmlUi calls ProcessEvent with the element
// that was clicked. That element is often not the button itself but the text inside
// of it, so the search goes up through the parents until one of them carries the
// action attribute.
class ActionListener final : public Rml::EventListener {
public:
    explicit ActionListener(std::vector<std::string>& actions) : m_actions(actions) {}

    void ProcessEvent(Rml::Event& event) override {
        for (Rml::Element* element = event.GetTargetElement(); element != nullptr;
             element = element->GetParentNode()) {
            if (element->HasAttribute(ACTION_ATTRIBUTE)) {
                m_actions.push_back(element->GetAttribute<Rml::String>(ACTION_ATTRIBUTE, ""));
                return;
            }
        }
    }

private:
    // The list of the layer the names are written to.
    std::vector<std::string>& m_actions;
};

// The object a GLFW callback belongs to: the constructor stored it in the window.
UiLayer* layerOf(GLFWwindow* window) {
    return static_cast<UiLayer*>(glfwGetWindowUserPointer(window));
}

} // namespace

UiLayer::UiLayer(const core::Window& window) : m_window(window.nativeHandle()) {
    // The interfaces first: RmlUi asks for them while it starts. The renderer compiles
    // its shaders in its constructor and converts to false when that failed.
    m_fileInterface = std::make_unique<AssetFileInterface>();
    m_systemInterface = std::make_unique<LoggingSystemInterface>(m_window);
    m_renderInterface = std::make_unique<RenderInterface_GL3>();
    if (!static_cast<bool>(*m_renderInterface)) {
        core::logError("Menu: the OpenGL renderer of RmlUi could not be created");
        return;
    }
    Rml::SetFileInterface(m_fileInterface.get());
    Rml::SetSystemInterface(m_systemInterface.get());
    Rml::SetRenderInterface(m_renderInterface.get());
    if (!Rml::Initialise()) {
        core::logError("Menu: RmlUi could not be started");
        return;
    }
    m_started = true;

    // The font. RmlUi has no built-in one: without a font no text is drawn at all. The
    // path goes through the file interface, so it is a path in the assets directory.
    if (!Rml::LoadFontFace(core::TEXT_FONT_FILE)) {
        core::logError(std::string("Menu: the font cannot be loaded: ") + core::TEXT_FONT_FILE);
        return;
    }

    // The context is the surface the documents live on. Its size is the framebuffer of
    // the window, in pixels. draw sets it again in every frame.
    const core::Size framebuffer = window.framebufferSize();
    m_context =
        Rml::CreateContext(CONTEXT_NAME, Rml::Vector2i(framebuffer.width, framebuffer.height));
    if (m_context == nullptr) {
        core::logError("Menu: the RmlUi context could not be created");
        return;
    }

    // One listener for the clicks of every document: an event that no element has
    // stopped travels up to the context.
    m_clickListener = std::make_unique<ActionListener>(m_actions);
    m_context->AddEventListener(CLICK_EVENT, m_clickListener.get());

    // The callbacks. GLFW has one callback of each kind per window. Nothing else in the
    // program has set one at this point. Dear ImGui sets its own later and calls these
    // from them, so both libraries see every event.
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, onKey);
    glfwSetCharCallback(m_window, onChar);
    glfwSetCursorEnterCallback(m_window, onCursorEnter);
    glfwSetCursorPosCallback(m_window, onCursorPos);
    glfwSetMouseButtonCallback(m_window, onMouseButton);
    glfwSetScrollCallback(m_window, onScroll);
}

UiLayer::~UiLayer() {
    // Dear ImGui is gone by now and has put these callbacks back, so removing them
    // leaves the window without any.
    if (glfwGetWindowUserPointer(m_window) == this) {
        glfwSetKeyCallback(m_window, nullptr);
        glfwSetCharCallback(m_window, nullptr);
        glfwSetCursorEnterCallback(m_window, nullptr);
        glfwSetCursorPosCallback(m_window, nullptr);
        glfwSetMouseButtonCallback(m_window, nullptr);
        glfwSetScrollCallback(m_window, nullptr);
        glfwSetWindowUserPointer(m_window, nullptr);
    }

    // Shutdown destroys the context with its documents and releases the textures and
    // the geometry through the render interface. The interfaces and the listener are
    // members, so they are destroyed after this body, as RmlUi requires.
    if (m_started) {
        Rml::Shutdown();
    }
}

DocumentId UiLayer::loadDocument(const std::string& assetFile) {
    if (!isValid()) {
        return NO_DOCUMENT;
    }
    // RmlUi reads the file, and the style sheets it links to, through the file
    // interface. A new document is hidden until Show is called.
    Rml::ElementDocument* document = m_context->LoadDocument(assetFile);
    if (document == nullptr) {
        core::logError("Menu: the document cannot be loaded: " + assetFile);
        return NO_DOCUMENT;
    }
    m_documents.push_back(document);
    return static_cast<DocumentId>(m_documents.size()) - 1;
}

Rml::ElementDocument* UiLayer::documentOf(DocumentId document) const {
    if (document < 0 || static_cast<std::size_t>(document) >= m_documents.size()) {
        return nullptr;
    }
    return m_documents[static_cast<std::size_t>(document)];
}

void UiLayer::show(DocumentId document) {
    // A number that names no document shows nothing.
    if (documentOf(document) == nullptr) {
        document = NO_DOCUMENT;
    }
    if (document == m_shown) {
        return;
    }

    if (Rml::ElementDocument* previous = documentOf(m_shown)) {
        previous->Hide();
        // The cursor is no longer over anything of that document: without this
        // a button would still be drawn hovered when the document comes back.
        m_context->ProcessMouseLeave();
    }
    m_shown = document;
    if (Rml::ElementDocument* next = documentOf(m_shown)) {
        next->Show();
    }
}

void UiLayer::setText(DocumentId document, const std::string& elementId, const std::string& text) {
    Rml::ElementDocument* target = documentOf(document);
    if (target == nullptr) {
        return;
    }
    if (Rml::Element* element = target->GetElementById(elementId)) {
        element->SetInnerRML(text);
    }
}

std::vector<std::string> UiLayer::takeActions() {
    // std::exchange hands out the list and leaves an empty one in its place.
    return std::exchange(m_actions, {});
}

bool UiLayer::wantsKeyboard() const {
    if (!isValid() || m_shown == NO_DOCUMENT) {
        return false;
    }
    const Rml::Element* focused = m_context->GetFocusElement();
    if (focused == nullptr) {
        return false;
    }
    const Rml::String& tag = focused->GetTagName();
    return tag == INPUT_TAG || tag == TEXTAREA_TAG;
}

void UiLayer::setMouseEnabled(bool enabled) {
    if (enabled == m_mouseEnabled) {
        return;
    }
    m_mouseEnabled = enabled;
    if (!enabled && isValid()) {
        m_context->ProcessMouseLeave();
    }
}

void UiLayer::draw(core::Size framebuffer) {
    // A minimized window has a framebuffer of size 0 x 0: nothing to draw into.
    if (m_shown == NO_DOCUMENT || framebuffer.width == 0 || framebuffer.height == 0) {
        return;
    }

    // The size and the scale are asked in every frame, like the game does for its own
    // viewport: that handles a resized window and a window moved to another display.
    // The scale is what one "dp" of the style sheet is in pixels (1.5 at 150 % display
    // scaling on Windows, 2 on a Retina display). Both calls do nothing when the
    // number is the one RmlUi already has.
    m_context->SetDimensions(Rml::Vector2i(framebuffer.width, framebuffer.height));
    float scale = 1.0F;
    glfwGetWindowContentScale(m_window, &scale, nullptr);
    m_context->SetDensityIndependentPixelRatio(scale);

    // Update lays the document out and advances its animations and transitions.
    m_context->Update();

    // BeginFrame saves the OpenGL state it is about to change and binds a framebuffer
    // of its own. Render draws the document into it. EndFrame blends the result over
    // the window and puts the saved state back. Clear of the renderer is NOT called: it
    // would wipe the frame of the game.
    m_renderInterface->SetViewport(framebuffer.width, framebuffer.height);
    m_renderInterface->BeginFrame();
    m_context->Render();
    // GL_CHECK: in a Debug build an OpenGL error left behind by the frame of RmlUi is
    // logged here, with this line as its place.
    GL_CHECK(m_renderInterface->EndFrame());
}

void UiLayer::onKey(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
    UiLayer* layer = layerOf(window);
    if (layer->takesKeyboard()) {
        RmlGLFW::ProcessKeyCallback(layer->m_context, key, action, mods);
    }
}

void UiLayer::onChar(GLFWwindow* window, unsigned int codepoint) {
    UiLayer* layer = layerOf(window);
    if (layer->takesKeyboard()) {
        RmlGLFW::ProcessCharCallback(layer->m_context, codepoint);
    }
}

void UiLayer::onCursorEnter(GLFWwindow* window, int entered) {
    UiLayer* layer = layerOf(window);
    if (layer->takesMouse()) {
        RmlGLFW::ProcessCursorEnterCallback(layer->m_context, entered);
    }
}

void UiLayer::onCursorPos(GLFWwindow* window, double x, double y) {
    UiLayer* layer = layerOf(window);
    if (layer->takesMouse()) {
        // The helper converts the position from screen coordinates to framebuffer
        // pixels, which differ on a Retina display. 0: no modifier keys.
        RmlGLFW::ProcessCursorPosCallback(layer->m_context, window, x, y, 0);
    }
}

void UiLayer::onMouseButton(GLFWwindow* window, int button, int action, int mods) {
    UiLayer* layer = layerOf(window);
    if (layer->takesMouse()) {
        RmlGLFW::ProcessMouseButtonCallback(layer->m_context, button, action, mods);
    }
}

void UiLayer::onScroll(GLFWwindow* window, double /*xOffset*/, double yOffset) {
    UiLayer* layer = layerOf(window);
    if (layer->takesMouse()) {
        RmlGLFW::ProcessScrollCallback(layer->m_context, yOffset, 0);
    }
}

} // namespace ui
