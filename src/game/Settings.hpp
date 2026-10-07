// Settings: what the player changes on the settings screen, and the text of the file
// they are kept in.
#pragma once

#include "game/Difficulty.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace game {

// Plain data and pure functions without a window and without files, like the rest of
// the game_logic library, so tests can check every rule. The application reads and
// writes the file and hands its text to parseSettings and takes it from formatSettings.

/// The name of the settings file. It is looked for in the working directory of the
/// program: the launcher starts the game in its data directory, so the file stays
/// there when the game itself is replaced by a new version.
constexpr const char* SETTINGS_FILE_NAME = "night-maze-settings.txt";

/// The mouse sensitivity as the settings screen shows it: a number from 1 to 10. The
/// game turns the camera by MOUSE_DEGREES_PER_SENSITIVITY degrees for every unit of
/// mouse movement and every point of it, so the default 4 is 0.1 degrees per unit.
constexpr float MIN_MOUSE_SENSITIVITY = 1.0F;
constexpr float MAX_MOUSE_SENSITIVITY = 10.0F;
constexpr float DEFAULT_MOUSE_SENSITIVITY = 4.0F;
constexpr float MOUSE_DEGREES_PER_SENSITIVITY = 0.025F;

/// The vertical field of view in degrees (scene::Camera::fovDegrees): the angle between
/// the top and the bottom edge of the picture. 60 is the camera as it always was.
constexpr float MIN_FIELD_OF_VIEW_DEGREES = 45.0F;
constexpr float MAX_FIELD_OF_VIEW_DEGREES = 90.0F;
constexpr float DEFAULT_FIELD_OF_VIEW_DEGREES = 60.0F;

/// The master volume as the settings screen shows it: a whole number from 0 (silent) to
/// 100 (the sound files as loud as they are). 100 is the game as it sounded before it
/// had this setting. What the audio engine gets is masterVolumeGain of it.
constexpr float MIN_MASTER_VOLUME = 0.0F;
constexpr float MAX_MASTER_VOLUME = 100.0F;
constexpr float DEFAULT_MASTER_VOLUME = 100.0F;

/// The size of the window, in screen coordinates (the units of core::Window::windowSize).
struct WindowSize {
    int width = 0;
    int height = 0;

    bool operator==(const WindowSize& other) const = default;
};

/// The sizes the settings screen offers, from the smallest to the largest, all 16 : 9
/// except the second. The first one is the size the game always started with.
constexpr std::array<WindowSize, 6> WINDOW_SIZES = {{
    {.width = 1280, .height = 720},
    {.width = 1366, .height = 768},
    {.width = 1600, .height = 900},
    {.width = 1920, .height = 1080},
    {.width = 2560, .height = 1440},
    {.width = 3840, .height = 2160},
}};
constexpr WindowSize DEFAULT_WINDOW_SIZE = WINDOW_SIZES[0];

/// The smallest and the largest window size the settings file may ask for. A file can
/// name a size that is not in the list above, as long as it is inside these limits.
constexpr WindowSize MIN_WINDOW_SIZE = {.width = 640, .height = 360};
constexpr WindowSize MAX_WINDOW_SIZE = {.width = 7680, .height = 4320};

/// Everything the settings screen edits and the file stores. The values here are the
/// defaults: the game without a settings file.
struct GameSettings {
    /// From MIN_MOUSE_SENSITIVITY to MAX_MOUSE_SENSITIVITY.
    float mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;

    /// From MIN_FIELD_OF_VIEW_DEGREES to MAX_FIELD_OF_VIEW_DEGREES.
    float fieldOfViewDegrees = DEFAULT_FIELD_OF_VIEW_DEGREES;

    /// True: the game covers the whole screen, at the resolution the desktop has.
    bool fullscreen = false;

    /// The size of the window while the game is not fullscreen.
    WindowSize windowSize = DEFAULT_WINDOW_SIZE;

    /// The difficulty the main menu starts with: the one that was chosen last.
    Difficulty difficulty = Difficulty::Normal;

    /// From MIN_MASTER_VOLUME to MAX_MASTER_VOLUME, a whole number.
    float masterVolume = DEFAULT_MASTER_VOLUME;

    bool operator==(const GameSettings& other) const = default;
};

// The names of the settings: in the file, and as the attribute data-setting of the
// controls of the settings screen (assets/ui/settings.rml).
constexpr std::string_view MOUSE_SENSITIVITY_SETTING = "mouse_sensitivity";
constexpr std::string_view FIELD_OF_VIEW_SETTING = "field_of_view";
constexpr std::string_view FULLSCREEN_SETTING = "fullscreen";
constexpr std::string_view WINDOW_SIZE_SETTING = "window_size";
constexpr std::string_view DIFFICULTY_SETTING = "difficulty";
constexpr std::string_view MASTER_VOLUME_SETTING = "master_volume";

/// Sets one setting from text, the way the file and the controls of the settings screen
/// write it:
///
///     mouse_sensitivity   a number, "4" or "4.5"
///     field_of_view       a number of degrees, "60"
///     fullscreen          "on" or "off"
///     window_size         width and height with an x between them, "1280x720"
///     difficulty          "easy", "normal" or "hard"
///     master_volume       a number from 0 to 100, "80" (rounded to a whole number)
///
/// A number outside its limits is brought to the nearest limit. Returns false, and
/// changes nothing, when the name is not a setting or the value cannot be read.
bool applySetting(GameSettings& settings, std::string_view name, std::string_view value);

/// Reads the text of a settings file: one "name = value" per line, with any spaces
/// around the two. Empty lines and lines that start with # are skipped. So is every
/// line that cannot be read (an unknown name, a broken value, no = sign): the setting
/// keeps its default, and a file written by a newer version of the game still loads.
/// When a name appears twice the last line wins. An empty text gives the defaults.
///
/// The text may come from any editor: a byte order mark at its start and the \r of
/// Windows line ends are ignored.
GameSettings parseSettings(std::string_view text);

/// The text of a settings file with these settings: a comment line and one line per
/// setting. parseSettings of the result gives the same settings back (the mouse
/// sensitivity rounded to one decimal place, the field of view to whole degrees).
/// Numbers are always written with a point, whatever the language of the system is.
std::string formatSettings(const GameSettings& settings);

/// How far the camera turns for one unit of mouse movement at this sensitivity, in
/// degrees: sensitivity times MOUSE_DEGREES_PER_SENSITIVITY.
float mouseDegreesPerUnit(float sensitivity);

/// The two numbers as the settings screen shows them next to their sliders: the mouse
/// sensitivity with one decimal place, "4.0", and the field of view in whole degrees,
/// "60 deg". The first one is also the text the file holds.
std::string mouseSensitivityLabel(float sensitivity);
std::string fieldOfViewLabel(float degrees);

/// The master volume as the settings screen shows it next to its slider and as the
/// file holds it: a whole number, "80".
std::string masterVolumeLabel(float volume);

/// The loudness the audio engine is given for a master volume of the settings: a factor
/// from 0 (silent) to 1 (the sound files as they are). Not a straight line but the
/// square of volume / 100, so 50 gives 0.25. The ear hears loudness in ratios: with
/// a straight line everything below the middle of the slider would sound almost equally
/// loud and the last few points would do all the work. With the square the lower half
/// of the slider is useful too, and both ends stay where they are.
float masterVolumeGain(float volume);

/// A window size as the settings write it, "1280x720", and as a menu shows it,
/// "1280 x 720".
std::string windowSizeValue(WindowSize size);
std::string windowSizeLabel(WindowSize size);

/// The sizes of WINDOW_SIZES that fit on a desktop of the given size, from the smallest
/// to the largest. current, the size in use, is added in its place when it is not in
/// the list (a size from a hand written file), so the screen can always show it. The
/// smallest size is always offered, also on a desktop it does not fit on.
std::vector<WindowSize> windowSizeChoices(WindowSize desktop, WindowSize current);

/// The choice step places after current in choices: step 1 is the next larger size, -1
/// the next smaller one. At the ends of the list it stays at the end. A current size
/// that is not in the list, and an empty list, give current back.
WindowSize steppedWindowSize(const std::vector<WindowSize>& choices, WindowSize current, int step);

} // namespace game
