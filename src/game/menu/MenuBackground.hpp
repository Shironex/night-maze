// MenuBackground: what is shown behind the main menu, and the rule that chooses it.
#pragma once

#include <string>
#include <string_view>

namespace game {

// Plain data and one pure function without a window, without OpenGL and without
// a video decoder, like the rest of the game_logic library, so tests can check the
// rule. game::MenuBackgroundRenderer opens the files and draws what the rule chose.

/// What can be behind the main menu (and behind the settings opened from it).
enum class MenuBackground {
    /// The recorded loop, a video file played by the decoder of the operating system.
    Video = 0,
    /// One picture of that loop, a normal image file: what is shown when the video
    /// cannot be played.
    Still,
    /// The game itself: the menu camera gliding high over the maze in play. It was the
    /// background before the video existed, and it is the picture the loop is recorded
    /// from.
    LiveScene,
};

/// The answer of chooseMenuBackground.
struct MenuBackgroundChoice {
    /// What is shown.
    MenuBackground background = MenuBackground::LiveScene;

    /// Why, as the end of one line of the log: "the video plays", or what went wrong
    /// on the way to it.
    std::string reason;
};

/// Chooses the background of the main menu.
///
/// wanted is what was asked for: the video unless the command line names something
/// else (--menu-background). videoPlays tells whether the video file was opened and
/// its first frame was decoded, videoError why not (the text of the player, used only
/// in the reason). stillLoaded tells whether the picture file was loaded.
///
/// The rule, from the best background down to the one that always works:
///
///     wanted      video plays  still loaded  shown
///     LiveScene   any          any           LiveScene
///     Video       yes          any           Video
///     Video       no           yes           Still
///     Still       any          yes           Still
///     Video       no           no            LiveScene
///     Still       any          no            LiveScene
///
/// The live scene needs no file, so the menu always has a background.
MenuBackgroundChoice chooseMenuBackground(MenuBackground wanted, bool videoPlays, bool stillLoaded,
                                          std::string_view videoError);

/// True when the background is a picture that covers the whole window by itself: the
/// video and the still do, the live scene is the scene. It is the second argument of
/// game::drawsScene: behind such a background the scene is not drawn at all.
bool coversWindow(MenuBackground background);

/// The name of a background in the log and on the command line: "video", "still" or
/// "scene".
const char* menuBackgroundName(MenuBackground background);

/// Reads the name of a background, as --menu-background takes it. False for any other
/// text: background is then left as it was.
bool parseMenuBackground(std::string_view name, MenuBackground& background);

} // namespace game
