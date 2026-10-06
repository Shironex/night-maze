// UiLayer: owns RmlUi and draws the documents of the menu on top of the frame.
#pragma once

#include "core/Window.hpp"

#include <memory>
#include <string>
#include <vector>

// Forward declarations, so that including this header pulls in neither the GLFW header
// nor the headers of RmlUi. RenderInterface_GL3 comes from the backends of RmlUi.
struct GLFWwindow;
class RenderInterface_GL3;
namespace Rml {
class Context;
class ElementDocument;
class EventListener;
class FileInterface;
class SystemInterface;
} // namespace Rml

namespace ui {

/// A document that was loaded into the layer: a number that UiLayer::loadDocument
/// returns and the other functions take.
using DocumentId = int;

/// No document: what loadDocument returns when it failed, and what show takes to hide
/// every document.
constexpr DocumentId NO_DOCUMENT = -1;

/// Owns RmlUi for the lifetime of the object (RAII): the library itself, the three
/// interfaces it talks to the program through, the context the documents live in and
/// the GLFW callbacks that feed it the keyboard and the mouse.
///
/// A document is a file in RML (elements, like HTML) with a style sheet in RCSS (like
/// CSS). This class knows nothing about the game. It loads documents by their path in
/// the assets directory, shows one of them at a time and reports clicks by name: an
/// element with the attribute data-action="play" puts "play" on a list when it is
/// clicked, and whoever owns the layer takes the list once per frame (takeActions) and
/// decides what each name means.
///
/// The object must be created before the debug UI and destroyed after it: Dear ImGui
/// installs its own GLFW callbacks later and passes every event on to the ones
/// installed here.
class UiLayer {
public:
    /// Starts RmlUi for this window and loads the font of the program. When a step
    /// fails the error is in the log and isValid() is false: every other function then
    /// does nothing, and the game runs without a menu.
    explicit UiLayer(const core::Window& window);
    ~UiLayer();

    UiLayer(const UiLayer&) = delete;
    UiLayer& operator=(const UiLayer&) = delete;

    bool isValid() const { return m_context != nullptr; }

    /// Loads a document. assetFile is its path inside the assets directory, with
    /// forward slashes, for example "ui/main_menu.rml". The document stays hidden until
    /// show. Returns NO_DOCUMENT when the file is missing or has errors (see the log).
    DocumentId loadDocument(const std::string& assetFile);

    /// Shows this document and hides the one that was shown. NO_DOCUMENT hides it and
    /// shows nothing. Showing the document that is shown already does nothing.
    void show(DocumentId document);

    /// The document that is shown, or NO_DOCUMENT.
    DocumentId shown() const { return m_shown; }

    /// Replaces the text of the element with this id attribute in a document. text is
    /// taken as RML, so it must not contain the characters < and &.
    void setText(DocumentId document, const std::string& elementId, const std::string& text);

    /// The data-action names of the elements that were clicked since the last call, in
    /// the order of the clicks. The list is empty again afterwards.
    std::vector<std::string> takeActions();

    /// True while a text field of the shown document has the keyboard focus: typing
    /// then belongs to the field, and the caller blocks the keyboard for the game.
    bool wantsKeyboard() const;

    /// Lets the documents use the mouse (true) or makes them ignore it (false).
    /// Switched off while a debug panel is under the cursor: the panels are drawn on
    /// top of the documents, so a click on a panel must not also click a button below.
    void setMouseEnabled(bool enabled);

    /// Updates the shown document (layout, animations) and draws it into the window,
    /// on top of what is there. framebuffer is the size of the window in pixels. Call
    /// it after the game has drawn its frame and before the debug UI. Without a shown
    /// document it does nothing.
    void draw(core::Size framebuffer);

private:
    // The callbacks GLFW calls for the window. Static, because GLFW is a C library and
    // takes plain function pointers: each one finds the object through the user pointer
    // of the window.
    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onChar(GLFWwindow* window, unsigned int codepoint);
    static void onCursorEnter(GLFWwindow* window, int entered);
    static void onCursorPos(GLFWwindow* window, double x, double y);
    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void onScroll(GLFWwindow* window, double xOffset, double yOffset);

    // True when an event should be handed to RmlUi at all: only while a document is
    // shown, and for the mouse only while it is not switched off.
    bool takesKeyboard() const { return m_shown != NO_DOCUMENT; }
    bool takesMouse() const { return takesKeyboard() && m_mouseEnabled; }

    // The document with this id, or nullptr for NO_DOCUMENT and for a wrong number.
    Rml::ElementDocument* documentOf(DocumentId document) const;

    GLFWwindow* m_window = nullptr;
    // True once Rml::Initialise has succeeded: the destructor then has to shut RmlUi down.
    bool m_started = false;
    bool m_mouseEnabled = true;

    // The three interfaces RmlUi talks to the outside world through: files (from the
    // assets directory), the system (time, clipboard, cursor shape, log messages) and
    // the drawing with OpenGL. RmlUi keeps pointers to them, so they live as long as
    // this object.
    std::unique_ptr<Rml::FileInterface> m_fileInterface;
    std::unique_ptr<Rml::SystemInterface> m_systemInterface;
    std::unique_ptr<RenderInterface_GL3> m_renderInterface;

    // The surface the documents live on. Owned by RmlUi, like the documents: all of it
    // is released by Rml::Shutdown in the destructor.
    Rml::Context* m_context = nullptr;
    // The loaded documents: a DocumentId is an index into this list.
    std::vector<Rml::ElementDocument*> m_documents;
    DocumentId m_shown = NO_DOCUMENT;

    // RmlUi calls this object for every click in the context. It writes the
    // data-action name of the clicked element into m_actions.
    std::unique_ptr<Rml::EventListener> m_clickListener;
    std::vector<std::string> m_actions;
};

} // namespace ui
