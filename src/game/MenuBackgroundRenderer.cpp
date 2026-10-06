// MenuBackgroundRenderer: opens the video and the still picture of the main menu and
// draws the one that was chosen over the whole window.
#include "game/MenuBackgroundRenderer.hpp"

#include "assets/ImageLoader.hpp"
#include "core/GlCheck.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "gfx/CoverFit.hpp"

#include <filesystem>

namespace game {

namespace {

// The two files, relative to the assets directory. Both are made by
// tools/record_menu_loop.py: the still is the first frame of the loop.
constexpr const char* VIDEO_FILE = "video/menu_loop.mp4";
constexpr const char* STILL_FILE = "video/menu_still.png";

// The shader files. The vertex shader is the whole screen triangle of the
// post-processing passes.
constexpr const char* VERTEX_SHADER_FILE = "shaders/post/composite.vert";
constexpr const char* FRAGMENT_SHADER_FILE = "shaders/post/menu_background.frag";

// The uniforms of post/menu_background.frag.
constexpr const char* PICTURE_UNIFORM = "uPicture";
constexpr const char* SHOWN_LEFT_UNIFORM = "uShownLeft";
constexpr const char* SHOWN_RIGHT_UNIFORM = "uShownRight";
constexpr const char* SHOWN_TOP_UNIFORM = "uShownTop";
constexpr const char* SHOWN_BOTTOM_UNIFORM = "uShownBottom";
constexpr const char* BRIGHTNESS_UNIFORM = "uBrightness";

// The texture unit the picture is bound to while it is drawn.
constexpr GLuint PICTURE_TEXTURE_UNIT = 0;

// The triangle of composite.vert: three vertices, starting with number 0.
constexpr GLint FIRST_VERTEX = 0;
constexpr GLsizei TRIANGLE_VERTEX_COUNT = 3;

// The scrim: the colours of the background are multiplied by this number. The loop is
// a night scene and the menu documents darken the picture under their text themselves
// (assets/ui/menu.rcss), so the picture is shown as it was recorded.
constexpr float BACKGROUND_BRIGHTNESS = 1.0F;

// The channel counts of a picture file the still can have.
constexpr int RGB_CHANNELS = 3;
constexpr int RGBA_CHANNELS = 4;

} // namespace

MenuBackgroundRenderer::MenuBackgroundRenderer(MenuBackground wanted)
    : m_shader(core::assetPath(VERTEX_SHADER_FILE), core::assetPath(FRAGMENT_SHADER_FILE)),
      m_wanted(wanted) {
    // Without the shader neither picture can be drawn: the live scene it is.
    if (!m_shader.isValid()) {
        m_wanted = MenuBackground::LiveScene;
        m_background = MenuBackground::LiveScene;
        core::logError("Menu background: scene (the shader of the menu background cannot be "
                       "loaded)");
        return;
    }

    std::string videoError;
    if (m_wanted == MenuBackground::Video) {
        const std::filesystem::path file = core::assetPath(VIDEO_FILE);
        m_video = std::make_unique<video::VideoPlayer>(file);
        if (!m_video->isPlaying()) {
            videoError = m_video->error() + " [" + core::pathText(file) + "]";
            // Ends the decoding thread of a player that has nothing to play.
            m_video.reset();
        }
    }
    choose(videoError);
}

void MenuBackgroundRenderer::update(double deltaSeconds) {
    if (m_video == nullptr) {
        return;
    }
    m_video->update(deltaSeconds);
    if (!m_video->isPlaying()) {
        // The decoder gave up in the middle of playing. The menu goes on with the next
        // background of the rule.
        const std::string videoError = m_video->error();
        m_video.reset();
        choose(videoError);
    }
}

void MenuBackgroundRenderer::choose(const std::string& videoError) {
    const bool videoPlays = m_video != nullptr;
    // The still is loaded only when the rule can ask for it: not next to a playing
    // video and not for the live scene.
    const bool stillNeeded = m_wanted != MenuBackground::LiveScene && !videoPlays;
    const bool stillLoaded = stillNeeded && (m_still != nullptr || loadStill());

    const MenuBackgroundChoice choice =
        chooseMenuBackground(m_wanted, videoPlays, stillLoaded, videoError);
    m_background = choice.background;

    // The one line that says what is behind the menu and why. A warning when it is not
    // what was asked for.
    const std::string line = std::string("Menu background: ") + menuBackgroundName(m_background) +
                             " (" + choice.reason + ")";
    if (m_background == m_wanted) {
        core::logInfo(line);
    } else {
        core::logWarn(line);
    }
}

bool MenuBackgroundRenderer::loadStill() {
    const std::filesystem::path path = core::assetPath(STILL_FILE);
    assets::Image image;
    std::string error;
    // RowOrder::TopFirst: the rows as they are in the file, like the frames of the
    // video, so one shader shows both the same way up.
    if (!assets::loadImage(path, image, error, assets::RowOrder::TopFirst)) {
        // loadImage has logged which file failed and why.
        return false;
    }
    if (image.channels != RGB_CHANNELS && image.channels != RGBA_CHANNELS) {
        core::logError("The still picture of the menu must be a colour picture: " +
                       core::pathText(path));
        return false;
    }
    m_still = std::make_unique<gfx::FrameTexture>(image.width, image.height);
    m_still->upload(image.pixels.data(),
                    image.channels == RGBA_CHANNELS ? gfx::PixelOrder::Rgba : gfx::PixelOrder::Rgb);
    return true;
}

void MenuBackgroundRenderer::draw(core::Size framebuffer) const {
    // The picture of the moment and its size in pixels.
    int pictureWidth = 0;
    int pictureHeight = 0;
    if (m_background == MenuBackground::Video && m_video != nullptr) {
        m_video->bind(PICTURE_TEXTURE_UNIT);
        pictureWidth = m_video->width();
        pictureHeight = m_video->height();
    } else if (m_background == MenuBackground::Still && m_still != nullptr) {
        m_still->bind(PICTURE_TEXTURE_UNIT);
        pictureWidth = m_still->width();
        pictureHeight = m_still->height();
    } else {
        return;
    }

    // The triangle covers every pixel and replaces what is there: no depth test (the
    // depth buffer of the window is never cleared) and no blending. GL_FRAMEBUFFER_SRGB
    // is off, as for the composite pass: the bytes of the picture are written as they
    // are.
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glDisable(GL_BLEND));
    GL_CHECK(glDisable(GL_FRAMEBUFFER_SRGB));

    // Which part of the picture the window shows: all of it when the shapes are the
    // same, otherwise the middle, cut at two sides.
    const gfx::UvRect shown =
        gfx::coverFit(pictureWidth, pictureHeight, framebuffer.width, framebuffer.height);

    m_shader.use();
    m_shader.setInt(PICTURE_UNIFORM, static_cast<int>(PICTURE_TEXTURE_UNIT));
    m_shader.setFloat(SHOWN_LEFT_UNIFORM, shown.left);
    m_shader.setFloat(SHOWN_RIGHT_UNIFORM, shown.right);
    m_shader.setFloat(SHOWN_TOP_UNIFORM, shown.top);
    m_shader.setFloat(SHOWN_BOTTOM_UNIFORM, shown.bottom);
    m_shader.setFloat(BRIGHTNESS_UNIFORM, BACKGROUND_BRIGHTNESS);

    // A Core profile needs a vertex array object bound for every draw call, also for
    // one without attributes.
    m_vertexArray.bind();
    GL_CHECK(glDrawArrays(GL_TRIANGLES, FIRST_VERTEX, TRIANGLE_VERTEX_COUNT));
}

} // namespace game
