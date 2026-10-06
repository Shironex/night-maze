// Debug user interface: owns the Dear ImGui context and draws the debug panels and the HUD.
// See docs/modules/debug-ui.md
#pragma once

#include "debug/DebugWindow.hpp"

#include <vector>

namespace core {
class Window;
} // namespace core

namespace debug {

struct DebugContext;

/// Owns Dear ImGui for the lifetime of the object (RAII) and draws all debug panels and
/// the HUD of the game.
///
/// The constructor sets ImGui up for the given window, the destructor shuts it down.
/// It must be destroyed before the window, because shutdown needs the OpenGL context.
class DebugUI {
public:
    /// Initializes ImGui with the GLFW and OpenGL 3 backends for this window, then applies
    /// the theme and loads the panel font (Theme.hpp).
    explicit DebugUI(const core::Window& window);
    ~DebugUI();

    DebugUI(const DebugUI&) = delete;
    DebugUI& operator=(const DebugUI&) = delete;

    /// Shows or hides all panels. The HUD of the game is not a panel: it stays.
    void toggleVisible() { m_visible = !m_visible; }

    /// Shows (true) or hides (false) all panels, and tells which of the two it is. The
    /// menu camera hides them while it runs and puts them back afterwards.
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    /// True while ImGui uses the keyboard itself: a text field is being edited or another
    /// widget is active (for example a slider being dragged).
    /// ImGui computes this at the start of each frame it builds, so it lags by a frame.
    bool wantsKeyboard() const;

    /// True while ImGui uses the mouse itself: the cursor is over a panel or a widget is
    /// being dragged. The transparent middle of the dock area does not count.
    /// ImGui computes this at the start of each frame it builds, so it lags by a frame.
    /// Always false while the mouse is switched off with setMouseEnabled(false).
    bool wantsMouse() const;

    /// Lets ImGui use the mouse (true) or makes it ignore the mouse (false). Switch it
    /// off while the game owns the mouse (the cursor is captured for mouse look): the
    /// hidden cursor still has a position, and without this it would hover and click the
    /// panels it passes over. Call it before draw, it takes effect in that draw.
    void setMouseEnabled(bool enabled);

    /// Builds and renders the HUD and, while they are visible, the debug panels on top of
    /// the current frame. While the menu camera runs the HUD is left out: the game
    /// then shows itself, not a round.
    /// Call it last in the frame, after the scene has been drawn.
    /// The context holds the data the panels show and edit, see DebugContext.hpp.
    void draw(const DebugContext& context);

private:
    // The debug UI starts hidden: the game opens with its main menu, and the window
    // would cover a part of it. The panel key (main.cpp) shows it.
    bool m_visible = false;
    // The window with the seven categories. It keeps what the user chose in it and owns
    // one OpenGL object (the sampler of its texture previews). It is created with this
    // class, so after the OpenGL context exists, and it is destroyed with the members,
    // while the window of the game is still there (see main.cpp).
    DebugWindow m_window;
    // The bytes of the panel font file. ImGui only keeps a pointer to them, so they live
    // here. The body of the destructor destroys the ImGui context first and the members
    // are destroyed after it, so the bytes outlive every use of the pointer.
    std::vector<unsigned char> m_fontBytes;
};

} // namespace debug
