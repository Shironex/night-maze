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

/// No key: what UiLayer::takeCapturedKey returns while no key was pressed. It is the
/// number GLFW gives a key it does not know (GLFW_KEY_UNKNOWN).
constexpr int NO_CAPTURED_KEY = -1;

/// A control of a document that was moved by the player: a slider, for example. name is
/// what the attribute data-setting of the control says, value its new value as text.
struct ControlChange {
    std::string name;
    std::string value;
};

/// Owns RmlUi for the lifetime of the object (RAII): the library itself, the three
/// interfaces it talks to the program through, the context the documents live in and
/// the GLFW callbacks that feed it the keyboard and the mouse.
///
/// A document is a file in RML (elements, like HTML) with a style sheet in RCSS (like
/// CSS). This class knows nothing about the game. It loads documents by their path in
/// the assets directory, shows one of them at a time and reports clicks by name: an
/// element with the attribute data-action="play" puts "play" on a list when it is
/// clicked, and whoever owns the layer takes the list once per frame (takeActions) and
/// decides what each name means. A control with the attribute data-setting="name"
/// reports its new value in the same way whenever it changes (takeChanges).
///
/// The keyboard works in every document without code of the game: Tab and the arrow
/// keys move the focus between the elements the style sheet allows (tab-index and nav),
/// and Enter or Space clicks the focused one, which reports its action like a click of
/// the mouse.
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
    ///
    /// The keyboard focus goes to the element of the new document that carries the
    /// attribute autofocus. The body of the document gets the class "open" one moment
    /// after it became visible, so a style sheet can let the screen fade or slide in
    /// with a transition between "body" and "body.open".
    void show(DocumentId document);

    /// The document that is shown, or NO_DOCUMENT.
    DocumentId shown() const { return m_shown; }

    /// Replaces the text of the element with this id attribute in a document. text is
    /// taken as RML, so it must not contain the characters < and &.
    void setText(DocumentId document, const std::string& elementId, const std::string& text);

    /// Sets the value of a control (a text field, a slider) of a document, as text. The
    /// control reports that like a change made by the player (takeChanges).
    void setValue(DocumentId document, const std::string& elementId, const std::string& value);

    /// The value of a control of a document, as text: what a text field holds, where
    /// a slider stands. Empty when there is no such control.
    std::string value(DocumentId document, const std::string& elementId) const;

    /// Adds a class to the element with this id attribute (on true) or takes it away
    /// (on false). The style sheet decides what the class looks like: a chosen
    /// difficulty, a switch that is on. The id "#document" names the document itself,
    /// its body: RmlUi knows that name.
    void setClass(DocumentId document, const std::string& elementId, const std::string& className,
                  bool on);

    /// Sets how much of the element with this id attribute is there: 0 is nothing, 1 is
    /// all of it, with everything inside the element. Values outside of that range are
    /// brought into it. For what the code moves by itself in every frame, where
    /// a transition of the style sheet cannot do the work: the cards of the intro.
    void setOpacity(DocumentId document, const std::string& elementId, float opacity);

    /// Gives the keyboard focus to the element with this id attribute of a document, with
    /// the frame that shows where the keyboard is. For a screen whose first choice is not
    /// always the same element: the attribute autofocus is fixed in the document. Call
    /// it after show, which puts the focus on the autofocus element.
    void focus(DocumentId document, const std::string& elementId);

    /// Scrolls the element with this id attribute of a document up or down, when its
    /// style lets it scroll (overflow). pages is how far, in heights of the element:
    /// 1 is one full page down, -0.5 half a page up. The ends of the content stop it, so
    /// a large number is "to the end". RmlUi scrolls by the wheel of the mouse by itself,
    /// and by no key.
    void scrollBy(DocumentId document, const std::string& elementId, float pages);

    /// The data-action names of the elements that were clicked since the last call, in
    /// the order of the clicks. The list is empty again afterwards. Enter in a text
    /// field with the attribute data-submit="name" puts that name on the list too.
    std::vector<std::string> takeActions();

    /// The controls with a data-setting attribute whose value changed since the last
    /// call, in the order of the changes. A slider that is dragged reports many values.
    /// The list is empty again afterwards.
    std::vector<ControlChange> takeChanges();

    /// True while a text field of the shown document has the keyboard focus: typing
    /// then belongs to the field, and the caller blocks the keyboard for the game. A
    /// slider or a button with the focus does not count: it takes single keys only.
    /// Also true while a key is being captured (setKeyCapture): that key press belongs
    /// to whoever asked for it and to nothing else.
    bool wantsKeyboard() const;

    /// Starts (true) or ends (false) the capture of a key: the screen that lets the
    /// player choose a key waits for the next key press. While it is on, no key reaches
    /// the documents (Enter clicks nothing, the arrows move no focus), and the first
    /// key that is pressed is kept for takeCapturedKey. The mouse works as always.
    /// Showing another document ends the capture.
    void setKeyCapture(bool on);

    /// The key that was pressed since the capture started or since the last call, as
    /// its GLFW key code, or NO_CAPTURED_KEY. It is handed out once. The capture stays
    /// on: the caller ends it when the key is one it can use.
    int takeCapturedKey();

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

    // The capture of a key (setKeyCapture): whether it is on and the key that was
    // pressed and not taken yet. m_swallowedKey is the last captured key: what follows
    // its press (repeats while it is held, its release) is kept from the documents also
    // after the capture ended. Without that the release of a captured Space would click
    // the focused button, and the capture would start again.
    bool m_keyCapture = false;
    int m_capturedKey = NO_CAPTURED_KEY;
    int m_swallowedKey = NO_CAPTURED_KEY;

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

    // RmlUi calls this object for every changed control in the context. It writes the
    // data-setting name of the control and its new value into m_changes.
    std::unique_ptr<Rml::EventListener> m_changeListener;
    std::vector<ControlChange> m_changes;
};

} // namespace ui
