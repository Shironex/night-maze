// Tests of game/Settings: the settings of the player and the text of their file.
#include "game/Settings.hpp"

#include "game/Interactables.hpp"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace {

using game::GameSettings;
using game::WindowSize;

} // namespace

TEST_CASE("without a file the settings are the game as it always was") {
    const GameSettings settings;
    CHECK(settings.mouseSensitivity == 4.0F);
    // 4 on the scale of the screen is the 0.1 degrees per unit the game had before.
    CHECK(game::mouseDegreesPerUnit(settings.mouseSensitivity) == doctest::Approx(0.1F));
    CHECK(settings.fieldOfViewDegrees == 60.0F);
    CHECK_FALSE(settings.fullscreen);
    CHECK(settings.windowSize == WindowSize{.width = 1280, .height = 720});
    CHECK(settings.difficulty == game::Difficulty::Normal);
    // Full volume: the sound files as loud as they are.
    CHECK(settings.masterVolume == 100.0F);
    CHECK(game::masterVolumeGain(settings.masterVolume) == 1.0F);

    // An empty text, and one of comments only, give the same.
    CHECK(game::parseSettings("") == settings);
    CHECK(game::parseSettings("# nothing here\n\n   \n") == settings);
}

TEST_CASE("a settings file is read line by line") {
    const GameSettings settings = game::parseSettings("# Night Maze settings\n"
                                                      "mouse_sensitivity = 6.5\n"
                                                      "field_of_view = 75\n"
                                                      "fullscreen = on\n"
                                                      "window_size = 1920x1080\n"
                                                      "difficulty = hard\n"
                                                      "master_volume = 35\n");
    CHECK(settings.mouseSensitivity == doctest::Approx(6.5F));
    CHECK(settings.fieldOfViewDegrees == 75.0F);
    CHECK(settings.fullscreen);
    CHECK(settings.windowSize == WindowSize{.width = 1920, .height = 1080});
    CHECK(settings.difficulty == game::Difficulty::Hard);
    CHECK(settings.masterVolume == 35.0F);
}

TEST_CASE("a file of an older version, without the volume line, plays at the default volume") {
    // The file as the game wrote it before it had sound.
    const GameSettings settings = game::parseSettings("mouse_sensitivity = 6.5\n"
                                                      "field_of_view = 75\n"
                                                      "fullscreen = on\n"
                                                      "window_size = 1920x1080\n"
                                                      "difficulty = hard\n");
    CHECK(settings.masterVolume == game::DEFAULT_MASTER_VOLUME);
    // The lines it has still count.
    CHECK(settings.fieldOfViewDegrees == 75.0F);
}

TEST_CASE("a file of game 0.10, without the story line, starts the story at line 0") {
    const GameSettings settings = game::parseSettings("mouse_sensitivity = 6.5\n"
                                                      "field_of_view = 75\n"
                                                      "fullscreen = on\n"
                                                      "window_size = 1920x1080\n"
                                                      "difficulty = hard\n"
                                                      "master_volume = 42\n");
    CHECK(settings.nextStoryLine == 0);
    // Everything else is as the file says.
    CHECK(settings.mouseSensitivity == doctest::Approx(6.5F));
    CHECK(settings.fieldOfViewDegrees == 75.0F);
    CHECK(settings.fullscreen);
    CHECK(settings.windowSize == WindowSize{.width = 1920, .height = 1080});
    CHECK(settings.difficulty == game::Difficulty::Hard);
    CHECK(settings.masterVolume == 42.0F);
}

TEST_CASE("the story line counter is a number that goes round the table") {
    const int lines = game::flavourLineCount();
    REQUIRE(lines == 24);
    CHECK(game::parseSettings("story_line = 7\n").nextStoryLine == 7);
    CHECK(game::parseSettings("story_line = 12\n").nextStoryLine == 12);
    CHECK(game::parseSettings("story_line = " + std::to_string(lines - 1)).nextStoryLine ==
          lines - 1);
    // Past the end: wrapped, not clamped.
    CHECK(game::parseSettings("story_line = " + std::to_string(lines)).nextStoryLine == 0);
    CHECK(game::parseSettings("story_line = " + std::to_string(lines + 3)).nextStoryLine == 3);
    CHECK(game::parseSettings("story_line = 99999999999\n").nextStoryLine < lines);
    // Not a number (a minus sign included): the counter stays at 0 and the rest still loads.
    const GameSettings broken = game::parseSettings("story_line = soon\n"
                                                    "story_line = -4\n"
                                                    "fullscreen = on\n");
    CHECK(broken.nextStoryLine == 0);
    CHECK(broken.fullscreen);

    GameSettings settings;
    CHECK_FALSE(game::applySetting(settings, "story_line", "x"));
    CHECK(settings.nextStoryLine == 0);
}

TEST_CASE("a file of game 0.11 keeps its place in the story: the old counter is carried over") {
    // The old table had 16 lines: the 24 of today without the eight about the shadow,
    // which stand at the places 9 to 16. So the old places 0 to 8 stay, and 9 to 15
    // become 17 to 23.
    for (int old = 0; old <= 8; ++old) {
        CAPTURE(old);
        CHECK(game::parseSettings("next_story_line = " + std::to_string(old)).nextStoryLine == old);
    }
    for (int old = 9; old <= 15; ++old) {
        CAPTURE(old);
        CHECK(game::parseSettings("next_story_line = " + std::to_string(old)).nextStoryLine ==
              old + 8);
    }
    // The line itself is the same one before and after.
    CHECK(game::flavourLine(game::parseSettings("next_story_line = 9\n").nextStoryLine) ==
          "Puddles hold stars. Stars are no use. Walk on.");
    CHECK(game::flavourLine(game::parseSettings("next_story_line = 15\n").nextStoryLine) ==
          "One lamp is enough, if it is the one still lit.");

    // A hand written number past the old table goes round the OLD table first.
    CHECK(game::parseSettings("next_story_line = 16\n").nextStoryLine == 0);
    CHECK(game::parseSettings("next_story_line = 25\n").nextStoryLine == 17);
    // Broken: nothing changes.
    CHECK(game::parseSettings("next_story_line = soon\n").nextStoryLine == 0);
    GameSettings settings;
    CHECK_FALSE(game::applySetting(settings, "next_story_line", "x"));
    CHECK(settings.nextStoryLine == 0);
}

TEST_CASE("the new story counter wins over the old one, wherever the two lines stand") {
    CHECK(game::parseSettings("next_story_line = 12\nstory_line = 5\n").nextStoryLine == 5);
    CHECK(game::parseSettings("story_line = 5\nnext_story_line = 12\n").nextStoryLine == 5);
    // A new counter of 0 is a counter too.
    CHECK(game::parseSettings("story_line = 0\nnext_story_line = 12\n").nextStoryLine == 0);
    // A new counter that cannot be read is no counter: the old one is carried over.
    CHECK(game::parseSettings("story_line = soon\nnext_story_line = 12\n").nextStoryLine == 20);
}

TEST_CASE("only the new story counter is written, so the old one is carried over once") {
    const GameSettings old = game::parseSettings("next_story_line = 12\n");
    REQUIRE(old.nextStoryLine == 20);
    const std::string written = game::formatSettings(old);
    CHECK(written.find("story_line = 20\n") != std::string::npos);
    CHECK(written.find("next_story_line") == std::string::npos);
    // Read again, the number is taken as it is and not moved a second time.
    CHECK(game::parseSettings(written).nextStoryLine == 20);
}

TEST_CASE("calm night is a switch that is off unless the file says on") {
    CHECK_FALSE(GameSettings{}.calmNight);
    CHECK(game::parseSettings("calm_night = on\n").calmNight);
    CHECK_FALSE(game::parseSettings("calm_night = off\n").calmNight);
    CHECK_FALSE(game::parseSettings("calm_night = yes\n").calmNight);
    // It is its own setting: the difficulty stays whatever it is.
    const GameSettings settings = game::parseSettings("difficulty = hard\ncalm_night = on\n");
    CHECK(settings.calmNight);
    CHECK(settings.difficulty == game::Difficulty::Hard);

    GameSettings written;
    written.calmNight = true;
    CHECK(game::formatSettings(written).find("calm_night = on\n") != std::string::npos);
    CHECK(game::parseSettings(game::formatSettings(written)).calmNight);
}

TEST_CASE("what is written is read back the same") {
    GameSettings settings;
    settings.mouseSensitivity = 7.3F;
    settings.fieldOfViewDegrees = 82.0F;
    settings.fullscreen = true;
    settings.windowSize = {.width = 2560, .height = 1440};
    settings.difficulty = game::Difficulty::Easy;
    settings.masterVolume = 42.0F;
    settings.nextStoryLine = 21;
    settings.calmNight = true;
    settings.introSeen = true;

    const GameSettings readBack = game::parseSettings(game::formatSettings(settings));
    CHECK(readBack.mouseSensitivity == doctest::Approx(7.3F));
    CHECK(readBack.fieldOfViewDegrees == 82.0F);
    CHECK(readBack.fullscreen);
    CHECK(readBack.windowSize == settings.windowSize);
    CHECK(readBack.difficulty == game::Difficulty::Easy);
    CHECK(readBack.masterVolume == 42.0F);
    CHECK(readBack.nextStoryLine == 21);
    CHECK(readBack.calmNight);
    CHECK(readBack.introSeen);

    // The defaults too, and exactly: writing them twice gives the same text.
    const std::string defaults = game::formatSettings(GameSettings{});
    CHECK(game::parseSettings(defaults) == GameSettings{});
    CHECK(game::formatSettings(game::parseSettings(defaults)) == defaults);
}

TEST_CASE("the file is plain text a person can read and edit") {
    CHECK(game::formatSettings(GameSettings{}) ==
          "# Night Maze settings. One \"name = value\" per line, a line that starts with # is "
          "a comment.\n"
          "mouse_sensitivity = 4.0\n"
          "field_of_view = 60\n"
          "fullscreen = off\n"
          "window_size = 1280x720\n"
          "difficulty = normal\n"
          "master_volume = 100\n"
          "story_line = 0\n"
          "calm_night = off\n"
          "intro_seen = off\n");
}

TEST_CASE("a file from before the intro means not seen, and nothing else changes") {
    // The file as it was written before the intro: every line but intro_seen.
    const std::string old = "mouse_sensitivity = 7.3\n"
                            "field_of_view = 82\n"
                            "fullscreen = on\n"
                            "window_size = 1600x900\n"
                            "difficulty = hard\n"
                            "master_volume = 42\n"
                            "story_line = 5\n"
                            "calm_night = on\n";
    const GameSettings settings = game::parseSettings(old);
    CHECK_FALSE(settings.introSeen);
    CHECK(settings.mouseSensitivity == doctest::Approx(7.3F));
    CHECK(settings.fieldOfViewDegrees == 82.0F);
    CHECK(settings.fullscreen);
    CHECK(settings.windowSize == WindowSize{.width = 1600, .height = 900});
    CHECK(settings.difficulty == game::Difficulty::Hard);
    CHECK(settings.masterVolume == 42.0F);
    CHECK(settings.nextStoryLine == 5);
    CHECK(settings.calmNight);

    // Marking the intro as seen changes that one field, and in the file that one line:
    // what is written is the old file with the new line at its end.
    GameSettings seen = settings;
    seen.introSeen = true;
    CHECK(game::parseSettings(old + "intro_seen = on\n") == seen);
    const std::string written = game::formatSettings(seen);
    CHECK(written.ends_with(old + "intro_seen = on\n"));
}

TEST_CASE("the intro line reads on and off and nothing else") {
    GameSettings settings;
    CHECK(game::applySetting(settings, "intro_seen", "on"));
    CHECK(settings.introSeen);
    CHECK_FALSE(game::applySetting(settings, "intro_seen", "yes"));
    CHECK(settings.introSeen);
    CHECK(game::applySetting(settings, "intro_seen", "off"));
    CHECK_FALSE(settings.introSeen);
}

TEST_CASE("spaces, Windows line ends and a byte order mark do not matter") {
    const GameSettings settings = game::parseSettings("\xEF\xBB\xBF"
                                                      "  mouse_sensitivity=2.5  \r\n"
                                                      "\tfield_of_view   =   50\r\n"
                                                      "fullscreen = on\r\n"
                                                      "window_size = 1600 x 900\r\n"
                                                      "difficulty = easy");
    CHECK(settings.mouseSensitivity == doctest::Approx(2.5F));
    CHECK(settings.fieldOfViewDegrees == 50.0F);
    CHECK(settings.fullscreen);
    CHECK(settings.windowSize == WindowSize{.width = 1600, .height = 900});
    // The last line has no line end at all.
    CHECK(settings.difficulty == game::Difficulty::Easy);
}

TEST_CASE("lines that cannot be read are skipped and the rest still counts") {
    const GameSettings settings = game::parseSettings("volume = 11\n"
                                                      "this line has no sign\n"
                                                      "= 5\n"
                                                      "mouse_sensitivity = fast\n"
                                                      "field_of_view = 70\n"
                                                      "fullscreen = maybe\n"
                                                      "window_size = big\n"
                                                      "difficulty = nightmare\n");
    const GameSettings defaults;
    // The one good line.
    CHECK(settings.fieldOfViewDegrees == 70.0F);
    // An unknown name (a setting of a newer version) and every broken value: defaults.
    CHECK(settings.mouseSensitivity == defaults.mouseSensitivity);
    CHECK(settings.fullscreen == defaults.fullscreen);
    CHECK(settings.windowSize == defaults.windowSize);
    CHECK(settings.difficulty == defaults.difficulty);
}

TEST_CASE("a broken file never breaks the game") {
    const GameSettings defaults;
    // Bytes that are not text, a line of signs, and a very long line.
    CHECK(game::parseSettings(std::string("\x00\x01\xFF\xFE garbage \x7F", 14)) == defaults);
    CHECK(game::parseSettings("=====\n=\n==\n#=\n") == defaults);
    CHECK(game::parseSettings(std::string(100000, 'x')) == defaults);
    CHECK(game::parseSettings("field_of_view = " + std::string(5000, '9')) == defaults);
    CHECK(game::parseSettings("\n\n\n\r\n\r\r") == defaults);
}

TEST_CASE("when a name appears twice the last line wins") {
    const GameSettings settings =
        game::parseSettings("field_of_view = 50\nfield_of_view = 80\nfield_of_view = oops\n");
    CHECK(settings.fieldOfViewDegrees == 80.0F);
}

TEST_CASE("numbers outside their limits are brought to the nearest limit") {
    GameSettings settings;
    CHECK(game::applySetting(settings, "mouse_sensitivity", "0.2"));
    CHECK(settings.mouseSensitivity == game::MIN_MOUSE_SENSITIVITY);
    CHECK(game::applySetting(settings, "mouse_sensitivity", "500"));
    CHECK(settings.mouseSensitivity == game::MAX_MOUSE_SENSITIVITY);
    CHECK(game::applySetting(settings, "field_of_view", "5"));
    CHECK(settings.fieldOfViewDegrees == game::MIN_FIELD_OF_VIEW_DEGREES);
    CHECK(game::applySetting(settings, "field_of_view", "179.9"));
    CHECK(settings.fieldOfViewDegrees == game::MAX_FIELD_OF_VIEW_DEGREES);
    CHECK(game::applySetting(settings, "window_size", "10x10"));
    CHECK(settings.windowSize == game::MIN_WINDOW_SIZE);
    CHECK(game::applySetting(settings, "window_size", "99999x99999"));
    CHECK(settings.windowSize == game::MAX_WINDOW_SIZE);
    CHECK(game::applySetting(settings, "master_volume", "250"));
    CHECK(settings.masterVolume == game::MAX_MASTER_VOLUME);
    CHECK(game::applySetting(settings, "master_volume", "0"));
    CHECK(settings.masterVolume == game::MIN_MASTER_VOLUME);
}

TEST_CASE("one setting is set from the text a control of the settings screen sends") {
    GameSettings settings;
    // A slider sends its number with as many digits as it likes.
    CHECK(game::applySetting(settings, "mouse_sensitivity", "5.30000019"));
    CHECK(settings.mouseSensitivity == doctest::Approx(5.3F));
    CHECK(game::applySetting(settings, "field_of_view", "72"));
    CHECK(settings.fieldOfViewDegrees == 72.0F);
    CHECK(game::applySetting(settings, "fullscreen", "on"));
    CHECK(settings.fullscreen);
    CHECK(game::applySetting(settings, "fullscreen", "off"));
    CHECK_FALSE(settings.fullscreen);
    CHECK(game::applySetting(settings, "window_size", "1366x768"));
    CHECK(settings.windowSize == WindowSize{.width = 1366, .height = 768});
    CHECK(game::applySetting(settings, "difficulty", "hard"));
    CHECK(settings.difficulty == game::Difficulty::Hard);
    // The volume is kept as a whole number, whatever a hand written file says.
    CHECK(game::applySetting(settings, "master_volume", "37.00000153"));
    CHECK(settings.masterVolume == 37.0F);
    CHECK(game::applySetting(settings, "master_volume", "62.6"));
    CHECK(settings.masterVolume == 63.0F);
}

TEST_CASE("a value that cannot be read changes nothing and says so") {
    GameSettings settings;
    settings.mouseSensitivity = 3.0F;
    const GameSettings before = settings;

    CHECK_FALSE(game::applySetting(settings, "mouse_sensitivity", ""));
    CHECK_FALSE(game::applySetting(settings, "mouse_sensitivity", "-4"));
    CHECK_FALSE(game::applySetting(settings, "mouse_sensitivity", "4,5"));
    CHECK_FALSE(game::applySetting(settings, "mouse_sensitivity", "4.5.1"));
    CHECK_FALSE(game::applySetting(settings, "mouse_sensitivity", "."));
    CHECK_FALSE(game::applySetting(settings, "mouse_sensitivity", "1e3"));
    CHECK_FALSE(game::applySetting(settings, "field_of_view", "sixty"));
    CHECK_FALSE(game::applySetting(settings, "fullscreen", "ON"));
    CHECK_FALSE(game::applySetting(settings, "fullscreen", "1"));
    CHECK_FALSE(game::applySetting(settings, "window_size", "1280"));
    CHECK_FALSE(game::applySetting(settings, "window_size", "1280x"));
    CHECK_FALSE(game::applySetting(settings, "window_size", "x720"));
    CHECK_FALSE(game::applySetting(settings, "difficulty", "HARD"));
    CHECK_FALSE(game::applySetting(settings, "master_volume", "loud"));
    CHECK_FALSE(game::applySetting(settings, "master_volume", "-10"));
    CHECK_FALSE(game::applySetting(settings, "master_volume", "80%"));
    CHECK_FALSE(game::applySetting(settings, "sound_volume", "5"));
    CHECK_FALSE(game::applySetting(settings, "", "5"));
    CHECK(settings == before);
}

TEST_CASE("the mouse turns in proportion to the sensitivity") {
    CHECK(game::mouseDegreesPerUnit(1.0F) == doctest::Approx(0.025F));
    CHECK(game::mouseDegreesPerUnit(8.0F) ==
          doctest::Approx(2.0F * game::mouseDegreesPerUnit(4.0F)));
}

TEST_CASE("a window size is written for the file and for the menu") {
    const WindowSize size{.width = 1920, .height = 1080};
    CHECK(game::windowSizeValue(size) == "1920x1080");
    CHECK(game::windowSizeLabel(size) == "1920 x 1080");
    // The value is what applySetting reads.
    GameSettings settings;
    CHECK(game::applySetting(settings, "window_size", game::windowSizeValue(size)));
    CHECK(settings.windowSize == size);
}

TEST_CASE("the settings screen offers the window sizes that fit on the desktop") {
    const WindowSize smallest = game::WINDOW_SIZES[0];

    // A full HD desktop: the four sizes up to 1920 x 1080.
    const std::vector<WindowSize> fullHd =
        game::windowSizeChoices({.width = 1920, .height = 1080}, smallest);
    REQUIRE(fullHd.size() == 4U);
    CHECK(fullHd.front() == smallest);
    CHECK(fullHd.back() == WindowSize{.width = 1920, .height = 1080});

    // A large desktop: all of them.
    CHECK(game::windowSizeChoices({.width = 3840, .height = 2160}, smallest).size() ==
          game::WINDOW_SIZES.size());

    // A desktop smaller than every size: the smallest size is still there.
    const std::vector<WindowSize> tiny =
        game::windowSizeChoices({.width = 1024, .height = 600}, smallest);
    REQUIRE(tiny.size() == 1U);
    CHECK(tiny[0] == smallest);
}

TEST_CASE("the window size in use is always among the choices") {
    // A size of the list that does not fit on the desktop.
    const WindowSize large{.width = 2560, .height = 1440};
    const std::vector<WindowSize> withLarge =
        game::windowSizeChoices({.width = 1920, .height = 1080}, large);
    REQUIRE(withLarge.size() == 5U);
    CHECK(withLarge.back() == large);

    // A size of its own, from a hand written file: it stands in its place by width.
    const WindowSize custom{.width = 1500, .height = 800};
    const std::vector<WindowSize> withCustom =
        game::windowSizeChoices({.width = 1920, .height = 1080}, custom);
    REQUIRE(withCustom.size() == 5U);
    CHECK(withCustom[2] == custom);
    CHECK(withCustom[1].width < custom.width);
    CHECK(withCustom[3].width > custom.width);
}

TEST_CASE("the numbers of the settings screen are written like in the file") {
    CHECK(game::mouseSensitivityLabel(4.0F) == "4.0");
    CHECK(game::mouseSensitivityLabel(7.25F) == "7.3");
    CHECK(game::mouseSensitivityLabel(10.0F) == "10.0");
    CHECK(game::fieldOfViewLabel(60.0F) == "60 deg");
    CHECK(game::fieldOfViewLabel(74.6F) == "75 deg");
    CHECK(game::masterVolumeLabel(80.0F) == "80");
    CHECK(game::masterVolumeLabel(0.0F) == "0");
    CHECK(game::masterVolumeLabel(100.0F) == "100");
}

TEST_CASE("the volume of the screen becomes a gain that grows with its square") {
    // Both ends stay where they are.
    CHECK(game::masterVolumeGain(0.0F) == 0.0F);
    CHECK(game::masterVolumeGain(100.0F) == 1.0F);
    // The middle of the slider is a quarter of the loudness, not a half.
    CHECK(game::masterVolumeGain(50.0F) == doctest::Approx(0.25F));
    CHECK(game::masterVolumeGain(80.0F) == doctest::Approx(0.64F));
    // More volume is never less gain.
    CHECK(game::masterVolumeGain(30.0F) < game::masterVolumeGain(31.0F));
    // A number outside of the limits cannot make the sound louder than the files.
    CHECK(game::masterVolumeGain(250.0F) == 1.0F);
    CHECK(game::masterVolumeGain(-5.0F) == 0.0F);
}

TEST_CASE("the window size steps through the choices and stops at both ends") {
    const std::vector<WindowSize> choices =
        game::windowSizeChoices({.width = 1920, .height = 1080}, game::DEFAULT_WINDOW_SIZE);
    REQUIRE(choices.size() == 4U);

    CHECK(game::steppedWindowSize(choices, choices[0], 1) == choices[1]);
    CHECK(game::steppedWindowSize(choices, choices[1], -1) == choices[0]);
    CHECK(game::steppedWindowSize(choices, choices[2], 0) == choices[2]);
    // No way past the smallest and the largest.
    CHECK(game::steppedWindowSize(choices, choices[0], -1) == choices[0]);
    CHECK(game::steppedWindowSize(choices, choices[3], 1) == choices[3]);
    CHECK(game::steppedWindowSize(choices, choices[1], 100) == choices[3]);

    // A size the list does not have, and a list without sizes, change nothing.
    const WindowSize other{.width = 800, .height = 600};
    CHECK(game::steppedWindowSize(choices, other, 1) == other);
    CHECK(game::steppedWindowSize({}, other, 1) == other);
}
