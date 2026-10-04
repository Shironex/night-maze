// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#pragma once

#include "core/Application.hpp"

#include <array>

namespace game {

/// The game itself. For now it only clears the screen.
///
/// It knows nothing about the debug UI: main.cpp derives from this class and draws the
/// debug panels on top of the frame.
class NightMazeApp : public core::Application {
public:
    NightMazeApp();

protected:
    void onUpdate(double fixedDt) override;
    void onRender(double alpha) override;

    /// Background color (red, green, blue), exposed so the debug UI can edit it live.
    std::array<float, 3>& clearColor() { return m_clearColor; }

private:
    // A dark night blue.
    std::array<float, 3> m_clearColor{0.02F, 0.03F, 0.08F};
};

} // namespace game
