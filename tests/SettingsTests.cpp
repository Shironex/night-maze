// Tests of game/Settings: the settings of the player and the text of their file.
#include "game/Settings.hpp"

#include "game/Interactables.hpp"

#include <doctest/doctest.h>

#include <array>
#include <string>
#include <string_view>
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
          "effects_volume = 100\n"
          "ambient_volume = 100\n"
          "story_line = 0\n"
          "calm_night = off\n"
          "intro_seen = off\n"
          "campaign_night = 1\n"
          "key_forward = W\n"
          "key_back = S\n"
          "key_left = A\n"
          "key_right = D\n"
          "key_sprint = Left Shift\n"
          "key_use = E\n"
          "key_flashlight = F\n"
          "key_map = M\n"
          "key_restart = R\n");
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
    // the lines of the old file are written again as they were, with the new line after
    // them. (The two volumes that came later stand behind the master volume, at their
    // default. The campaign and the keys follow, see the test of the whole text above.)
    GameSettings seen = settings;
    seen.introSeen = true;
    CHECK(game::parseSettings(old + "intro_seen = on\n") == seen);
    const std::string written = game::formatSettings(seen);
    const std::string masterLine = "master_volume = 42\n";
    const std::size_t afterMaster = old.find(masterLine) + masterLine.size();
    CHECK(written.find(old.substr(0, afterMaster) + "effects_volume = 100\nambient_volume = 100\n" +
                       old.substr(afterMaster) + "intro_seen = on\n") != std::string::npos);
}

TEST_CASE("the effects and the ambient volume are read and written like the master volume") {
    CHECK(GameSettings{}.effectsVolume == game::DEFAULT_MASTER_VOLUME);
    CHECK(GameSettings{}.ambientVolume == game::DEFAULT_MASTER_VOLUME);

    GameSettings settings = game::parseSettings("effects_volume = 70\nambient_volume = 25\n");
    CHECK(settings.effectsVolume == 70.0F);
    CHECK(settings.ambientVolume == 25.0F);
    // Each is a setting of its own: the master volume stays where it was.
    CHECK(settings.masterVolume == game::DEFAULT_MASTER_VOLUME);

    // The same limits and the same rounding as the master volume.
    CHECK(game::applySetting(settings, "effects_volume", "250"));
    CHECK(settings.effectsVolume == game::MAX_MASTER_VOLUME);
    CHECK(game::applySetting(settings, "ambient_volume", "0"));
    CHECK(settings.ambientVolume == game::MIN_MASTER_VOLUME);
    CHECK(game::applySetting(settings, "ambient_volume", "62.6"));
    CHECK(settings.ambientVolume == 63.0F);
    // A value that cannot be read changes nothing.
    CHECK_FALSE(game::applySetting(settings, "effects_volume", "loud"));
    CHECK_FALSE(game::applySetting(settings, "ambient_volume", "-10"));
    CHECK(settings.effectsVolume == game::MAX_MASTER_VOLUME);
    CHECK(settings.ambientVolume == 63.0F);

    // Written and read back.
    settings.effectsVolume = 70.0F;
    const std::string written = game::formatSettings(settings);
    CHECK(written.find("effects_volume = 70\n") != std::string::npos);
    CHECK(written.find("ambient_volume = 63\n") != std::string::npos);
    CHECK(game::parseSettings(written) == settings);
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

TEST_CASE("a file of game 0.12 loads with the default of everything that came later") {
    // The file exactly as game 0.12.0 wrote it: no volumes of the effects and of the
    // ambience, no campaign, no keys.
    const std::string old = "# Night Maze settings. One \"name = value\" per line, a line that "
                            "starts with # is a comment.\n"
                            "mouse_sensitivity = 7.3\n"
                            "field_of_view = 82\n"
                            "fullscreen = on\n"
                            "window_size = 1600x900\n"
                            "difficulty = hard\n"
                            "master_volume = 42\n"
                            "story_line = 5\n"
                            "calm_night = on\n"
                            "intro_seen = on\n";
    const GameSettings settings = game::parseSettings(old);

    // Everything the file says is there.
    GameSettings expected;
    expected.mouseSensitivity = 7.3F;
    expected.fieldOfViewDegrees = 82.0F;
    expected.fullscreen = true;
    expected.windowSize = {.width = 1600, .height = 900};
    expected.difficulty = game::Difficulty::Hard;
    expected.masterVolume = 42.0F;
    expected.nextStoryLine = 5;
    expected.calmNight = true;
    expected.introSeen = true;
    // And nothing else differs from a game that has no file: the comparison is of every
    // field, so a setting that is added later is covered without a new line here.
    CHECK(settings == expected);

    // The same, said field by field for what came after 0.12.0.
    CHECK(settings.effectsVolume == 100.0F);
    CHECK(settings.ambientVolume == 100.0F);
    CHECK(settings.campaignNight == 1);
    CHECK(settings.campaignSeed == game::NO_CAMPAIGN_SEED);
    for (const int best : settings.campaignBestSeconds) {
        CHECK(best == game::NO_BEST_TIME);
    }
    CHECK(game::campaignStage(settings.campaignNight) == game::CampaignStage::NotStarted);
    CHECK(settings.keys == game::defaultKeyBindings());

    // Written again, the old lines are followed by the new ones: the two volumes behind
    // the master volume, the night of the campaign (its seed and its best times have no
    // line before they exist) and the nine keys at the end.
    CHECK(game::formatSettings(settings) ==
          "# Night Maze settings. One \"name = value\" per line, a line that starts with # is "
          "a comment.\n"
          "mouse_sensitivity = 7.3\n"
          "field_of_view = 82\n"
          "fullscreen = on\n"
          "window_size = 1600x900\n"
          "difficulty = hard\n"
          "master_volume = 42\n"
          "effects_volume = 100\n"
          "ambient_volume = 100\n"
          "story_line = 5\n"
          "calm_night = on\n"
          "intro_seen = on\n"
          "campaign_night = 1\n"
          "key_forward = W\n"
          "key_back = S\n"
          "key_left = A\n"
          "key_right = D\n"
          "key_sprint = Left Shift\n"
          "key_use = E\n"
          "key_flashlight = F\n"
          "key_map = M\n"
          "key_restart = R\n");
}

TEST_CASE("reset defaults brings back the screen and the keys, and keeps the progress") {
    GameSettings settings;
    // What the settings screen shows.
    settings.mouseSensitivity = 9.0F;
    settings.fieldOfViewDegrees = 75.0F;
    settings.fullscreen = true;
    settings.windowSize = {.width = 1600, .height = 900};
    settings.masterVolume = 10.0F;
    settings.effectsVolume = 20.0F;
    settings.ambientVolume = 30.0F;
    CHECK(game::applySetting(settings, "key_sprint", "Q"));
    CHECK(settings.keys != game::defaultKeyBindings());
    // What is chosen or earned somewhere else.
    settings.difficulty = game::Difficulty::Hard;
    settings.nextStoryLine = 7;
    settings.calmNight = true;
    settings.introSeen = true;
    settings.campaignNight = 3;
    settings.campaignSeed = 482113;
    settings.campaignBestSeconds = {95, 140, 0, 0, 0};

    GameSettings expected;
    expected.difficulty = game::Difficulty::Hard;
    expected.nextStoryLine = 7;
    expected.calmNight = true;
    expected.introSeen = true;
    expected.campaignNight = 3;
    expected.campaignSeed = 482113;
    expected.campaignBestSeconds = {95, 140, 0, 0, 0};
    // Every other field is the default one, the keys too.
    CHECK(game::resetSettings(settings) == expected);
    CHECK(game::resetSettings(settings).keys == game::defaultKeyBindings());
    // A second reset changes nothing.
    CHECK(game::resetSettings(expected) == expected);
}

TEST_CASE("the campaign is written and read back: night, seed and best times") {
    GameSettings settings;
    settings.campaignNight = 3;
    settings.campaignSeed = 482113;
    settings.campaignBestSeconds = {95, 204, 0, 0, 0};
    const std::string text = game::formatSettings(settings);
    // The lines of the campaign stand together, and the keys follow them.
    CHECK(text.find("campaign_night = 3\n"
                    "campaign_seed = 482113\n"
                    "campaign_best_1 = 95\n"
                    "campaign_best_2 = 204\n"
                    "key_forward = W\n") != std::string::npos);
    CHECK(game::parseSettings(text) == settings);

    // A finished campaign, with the largest seed a seed can be.
    settings.campaignNight = game::CAMPAIGN_FINISHED;
    settings.campaignSeed = 4294967295U;
    settings.campaignBestSeconds = {95, 204, 388, 512, 5999};
    CHECK(game::parseSettings(game::formatSettings(settings)) == settings);
    CHECK(game::campaignBestSetting(5) == "campaign_best_5");
}

TEST_CASE("broken campaign lines fall back to something that can be played") {
    // A night outside the campaign is brought to its nearest end.
    CHECK(game::parseSettings("campaign_night = 0\n").campaignNight == 1);
    CHECK(game::parseSettings("campaign_night = 99\n").campaignNight == game::CAMPAIGN_FINISHED);
    CHECK(game::parseSettings("campaign_night = 999999999999\n").campaignNight ==
          game::CAMPAIGN_FINISHED);
    CHECK(game::parseSettings("campaign_night = 2.6\n").campaignNight == 3);
    // A line that is no number at all is skipped: no campaign yet.
    for (const std::string_view value : {"-2", "three", "", "2 nights"}) {
        GameSettings settings;
        CHECK_FALSE(game::applySetting(settings, "campaign_night", value));
        CHECK(settings.campaignNight == 1);
    }

    // A seed is digits only, and not more than a seed can be.
    for (const std::string_view value : {"-1", "12.5", "4294967296", "abc", ""}) {
        GameSettings settings;
        CHECK_FALSE(game::applySetting(settings, "campaign_seed", value));
        CHECK(settings.campaignSeed == game::NO_CAMPAIGN_SEED);
    }

    // A best time: a whole number of seconds, at most 99:59, and only for nights 1 to 5.
    GameSettings settings;
    CHECK(game::applySetting(settings, "campaign_best_1", "95.4"));
    CHECK(settings.campaignBestSeconds[0] == 95);
    CHECK(game::applySetting(settings, "campaign_best_5", "999999"));
    CHECK(settings.campaignBestSeconds[4] == game::MAX_BEST_SECONDS);
    CHECK_FALSE(game::applySetting(settings, "campaign_best_2", "fast"));
    CHECK_FALSE(game::applySetting(settings, "campaign_best_0", "10"));
    CHECK_FALSE(game::applySetting(settings, "campaign_best_6", "10"));
    CHECK_FALSE(game::applySetting(settings, "campaign_best_12", "10"));
    CHECK_FALSE(game::applySetting(settings, "campaign_best_", "10"));
    CHECK(settings.campaignBestSeconds[1] == game::NO_BEST_TIME);
}

TEST_CASE("a night that is not won has no best time, whatever the file says") {
    // The third night is next: a time for it or for a later night cannot be real.
    const GameSettings settings = game::parseSettings("campaign_best_1 = 95\n"
                                                      "campaign_best_3 = 10\n"
                                                      "campaign_best_5 = 10\n"
                                                      "campaign_night = 3\n"
                                                      "campaign_best_2 = 204\n");
    CHECK(settings.campaignNight == 3);
    CHECK(settings.campaignBestSeconds ==
          std::array<int, game::CAMPAIGN_NIGHT_COUNT>{95, 204, 0, 0, 0});

    // No campaign line at all: no night is won, so no time counts.
    const GameSettings none = game::parseSettings("campaign_best_1 = 95\n");
    CHECK(none.campaignBestSeconds[0] == game::NO_BEST_TIME);
}

TEST_CASE("progress without a seed still loads, and the nights that were won stay won") {
    // The seed line is lost. The game draws a seed when the next night is started.
    const GameSettings noSeed = game::parseSettings("campaign_night = 4\n"
                                                    "campaign_seed = oops\n"
                                                    "campaign_best_1 = 95\n");
    CHECK(noSeed.campaignNight == 4);
    CHECK(noSeed.campaignSeed == game::NO_CAMPAIGN_SEED);
    CHECK(noSeed.campaignBestSeconds ==
          std::array<int, game::CAMPAIGN_NIGHT_COUNT>{95, 0, 0, 0, 0});
    CHECK(game::nightStatus(noSeed.campaignNight, 2) == game::NightStatus::Finished);
    // And it is written back in a form that reads the same.
    CHECK(game::parseSettings(game::formatSettings(noSeed)) == noSeed);
}
