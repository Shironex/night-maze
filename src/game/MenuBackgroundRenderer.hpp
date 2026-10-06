// MenuBackgroundRenderer: opens the video and the still picture of the main menu and
// draws the one that was chosen over the whole window.
#pragma once

#include "core/Window.hpp"
#include "game/MenuBackground.hpp"
#include "gfx/FrameTexture.hpp"
#include "gfx/Shader.hpp"
#include "gfx/VertexArray.hpp"
#include "video/VideoPlayer.hpp"

#include <memory>
#include <string>

namespace game {

/// The OpenGL side of the background of the main menu. Which background is shown is
/// a rule without OpenGL (game::chooseMenuBackground). This class gives the rule its
/// facts (does the video play, is the still loaded) and draws the result.
///
/// The video is a recorded loop of the game (assets/video/menu_loop.mp4, made by
/// tools/record_menu_loop.py), played by video::VideoPlayer with the decoder of the
/// operating system. The still is one frame of that loop as a picture file
/// (assets/video/menu_still.png). It is shown when the video cannot be played: the file
/// is missing, the system has no decoder (a Windows N edition without the Media Feature
/// Pack) or opening fails. The still is only loaded when it is needed.
///
/// The live scene is not drawn here: it is the scene itself, drawn by NightMazeApp as
/// before. This class then owns nothing and draw does nothing.
///
/// One line of the log says which background is used and why, at the start and again
/// if the video gives up while it plays.
///
/// It owns OpenGL objects and a thread, so it must be destroyed before the window.
class MenuBackgroundRenderer {
public:
    /// Opens what wanted asks for (the video unless the command line says otherwise),
    /// falls back as game::chooseMenuBackground says and logs the choice. Opening the
    /// video waits for its first frame (about a tenth of a second).
    explicit MenuBackgroundRenderer(MenuBackground wanted);

    /// What is behind the main menu at this moment.
    MenuBackground background() const { return m_background; }

    /// Moves the video on by deltaSeconds, the time of one frame of the game. Call it
    /// once per frame while the main menu is shown, before asking background(): a video
    /// whose decoder gave up is replaced by the still (or by the live scene) here. While
    /// it is not called the video stands still and costs nothing.
    void update(double deltaSeconds);

    /// Draws the background into the target that is bound, over all of it: the picture
    /// covers the target without being stretched (gfx::coverFit). framebuffer is the
    /// size of the target in pixels. It switches the depth test and the blending off
    /// and leaves them off. With the live scene as the background it does nothing.
    void draw(core::Size framebuffer) const;

private:
    /// Loads the still picture into m_still. False when the file cannot be loaded (the
    /// error is in the log).
    bool loadStill();

    /// Asks game::chooseMenuBackground with the facts of this moment, loading the still
    /// if the rule may need it, stores the answer and logs it. videoError is why the
    /// video does not play (empty when it does, or when it was not asked for).
    void choose(const std::string& videoError);

    // The triangle that covers the whole target (post/composite.vert) and the fragment
    // shader that shows one picture on it.
    gfx::Shader m_shader;
    gfx::VertexArray m_vertexArray;

    // What was asked for, and what is shown.
    MenuBackground m_wanted;
    MenuBackground m_background = MenuBackground::LiveScene;

    // The player of the video, while the video is the background. Empty otherwise.
    std::unique_ptr<video::VideoPlayer> m_video;
    // The still picture, once it was needed. Empty otherwise.
    std::unique_ptr<gfx::FrameTexture> m_still;
};

} // namespace game
