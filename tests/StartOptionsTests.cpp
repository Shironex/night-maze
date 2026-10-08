// Tests of game::parseStartOptions: the switches of the command line.
// See docs/modules/game/menu-camera.md
#include "game/StartOptions.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

// Reads a command line given as a list of words (without the name of the program).
game::StartOptionsResult parse(const std::vector<const char*>& words) {
    return game::parseStartOptions(words);
}

} // namespace

TEST_CASE("without switches the game starts as always") {
    const game::StartOptionsResult result = parse({});
    CHECK(result.error.empty());
    CHECK(result.options.seed == game::DEFAULT_MAZE_SEED);
    CHECK_FALSE(result.options.seedGiven);
    CHECK_FALSE(result.options.menuCamera.enabled);
    CHECK(result.options.menuCamera.shot == game::MenuShot::CorridorWalk);
    CHECK(result.options.menuCamera.timeOffset == 0.0F);
    // The game then opens with the main menu, and the menu has its video behind it.
    CHECK_FALSE(result.options.play);
    CHECK(result.options.menuBackground == game::MenuBackground::Video);
    // And the round starts where it always started, with no crystal collected.
    CHECK_FALSE(result.options.startCell.has_value());
    CHECK_FALSE(result.options.startYawDegrees.has_value());
    CHECK_FALSE(result.options.collectAll);
    CHECK_FALSE(result.options.calm);
    CHECK(result.options.night == 0);
    // Nothing says that a tool drives the game, and nothing about the intro.
    CHECK_FALSE(result.options.toolSwitch);
    CHECK_FALSE(result.options.skipIntro);
    CHECK_FALSE(result.options.intro);
}

TEST_CASE("each of the eleven switches of a tool marks the run as driven by a tool") {
    CHECK(parse({"--play"}).options.toolSwitch);
    CHECK(parse({"--seed", "1"}).options.toolSwitch);
    CHECK(parse({"--menu-camera"}).options.toolSwitch);
    CHECK(parse({"--menu-shot", "glide"}).options.toolSwitch);
    CHECK(parse({"--menu-time", "0"}).options.toolSwitch);
    // Also a value that is the default: the switch was given.
    CHECK(parse({"--menu-background", "video"}).options.toolSwitch);
    // The switches that set up a round for a picture, and the one for a calm night: a
    // run with any of them is a scripted run and must not open with the intro.
    CHECK(parse({"--start-cell", "3,4"}).options.toolSwitch);
    CHECK(parse({"--start-yaw", "90"}).options.toolSwitch);
    CHECK(parse({"--collect-all"}).options.toolSwitch);
    CHECK(parse({"--calm"}).options.toolSwitch);
    CHECK(parse({"--night", "2"}).options.toolSwitch);
}

TEST_CASE("the two switches of the intro set their fields and are no switches of a tool") {
    const game::StartOptionsResult skip = parse({"--skip-intro"});
    CHECK(skip.error.empty());
    CHECK(skip.options.skipIntro);
    CHECK_FALSE(skip.options.intro);
    CHECK_FALSE(skip.options.toolSwitch);

    const game::StartOptionsResult intro = parse({"--intro"});
    CHECK(intro.error.empty());
    CHECK(intro.options.intro);
    CHECK_FALSE(intro.options.skipIntro);
    CHECK_FALSE(intro.options.toolSwitch);

    // Next to the switches of a tool, in any place, and they take no value.
    const game::StartOptionsResult mixed = parse({"--seed", "7", "--intro", "--play"});
    CHECK(mixed.error.empty());
    CHECK(mixed.options.intro);
    CHECK(mixed.options.toolSwitch);
    CHECK(mixed.options.seed == 7U);
    CHECK_FALSE(parse({"--intro", "now"}).error.empty());
    // "--intro" is a whole word: a longer one is not known.
    CHECK_FALSE(parse({"--intros"}).error.empty());
}

TEST_CASE("the background of the main menu can be named, and only by its three names") {
    CHECK(parse({"--menu-background", "scene"}).options.menuBackground ==
          game::MenuBackground::LiveScene);
    CHECK(parse({"--menu-background", "still"}).options.menuBackground ==
          game::MenuBackground::Still);
    const game::StartOptionsResult video = parse({"--menu-background", "video"});
    CHECK(video.error.empty());
    CHECK(video.options.menuBackground == game::MenuBackground::Video);
    // It changes nothing else: the game still opens with the main menu.
    CHECK_FALSE(video.options.play);
    CHECK_FALSE(video.options.menuCamera.enabled);

    CHECK_FALSE(parse({"--menu-background", "live"}).error.empty());
    CHECK_FALSE(parse({"--menu-background"}).error.empty());
}

TEST_CASE("the play switch skips the main menu and changes nothing else") {
    const game::StartOptionsResult result = parse({"--play"});
    CHECK(result.error.empty());
    CHECK(result.options.play);
    CHECK_FALSE(result.options.menuCamera.enabled);
    CHECK(result.options.seed == game::DEFAULT_MAZE_SEED);

    // Together with other switches, in any place.
    const game::StartOptionsResult mixed = parse({"--seed", "7", "--play", "--menu-camera"});
    CHECK(mixed.error.empty());
    CHECK(mixed.options.play);
    CHECK(mixed.options.seed == 7U);
    CHECK(mixed.options.menuCamera.enabled);

    // It takes no value: the next word is read as a switch of its own.
    CHECK_FALSE(parse({"--play", "now"}).error.empty());
}

TEST_CASE("every switch sets its field, in any order") {
    const game::StartOptionsResult result =
        parse({"--menu-time", "12.5", "--menu-camera", "--seed", "42", "--menu-shot", "glide"});
    CHECK(result.error.empty());
    CHECK(result.options.seed == 42U);
    CHECK(result.options.menuCamera.enabled);
    CHECK(result.options.menuCamera.shot == game::MenuShot::HighGlide);
    CHECK(result.options.menuCamera.timeOffset == doctest::Approx(12.5F));
}

TEST_CASE("the shot can be named walk, and the largest seed is accepted") {
    const game::StartOptionsResult result = parse({"--menu-shot", "walk", "--seed", "4294967295"});
    CHECK(result.error.empty());
    CHECK(result.options.menuCamera.shot == game::MenuShot::CorridorWalk);
    CHECK(result.options.seed == 4294967295U);
    // Naming a shot does not switch the camera on.
    CHECK_FALSE(result.options.menuCamera.enabled);
}

TEST_CASE("an unknown switch is an error that names it") {
    const game::StartOptionsResult result = parse({"--menu-camera", "--fullscreen"});
    CHECK(result.error.find("--fullscreen") != std::string::npos);
}

TEST_CASE("a switch without its value is an error") {
    CHECK_FALSE(parse({"--seed"}).error.empty());
    CHECK_FALSE(parse({"--menu-camera", "--menu-time"}).error.empty());
}

TEST_CASE("a value that is not understood is an error") {
    // Not a number, a negative seed, a seed too large for 32 bits, a number with a rest.
    CHECK_FALSE(parse({"--seed", "abc"}).error.empty());
    CHECK_FALSE(parse({"--seed", "-3"}).error.empty());
    CHECK_FALSE(parse({"--seed", "4294967296"}).error.empty());
    CHECK_FALSE(parse({"--seed", "12x"}).error.empty());
    CHECK_FALSE(parse({"--seed", ""}).error.empty());
    CHECK_FALSE(parse({"--menu-time", "soon"}).error.empty());
    CHECK_FALSE(parse({"--menu-time", "3s"}).error.empty());
    CHECK_FALSE(parse({"--menu-time", "inf"}).error.empty());
    CHECK_FALSE(parse({"--menu-shot", "orbit"}).error.empty());
}

TEST_CASE("the list of switches names every switch") {
    const std::string usage = game::START_OPTIONS_USAGE;
    CHECK(usage.find("--seed") != std::string::npos);
    CHECK(usage.find("--menu-camera") != std::string::npos);
    CHECK(usage.find("--menu-shot") != std::string::npos);
    CHECK(usage.find("--menu-time") != std::string::npos);
    CHECK(usage.find("--play") != std::string::npos);
    CHECK(usage.find("--menu-background") != std::string::npos);
    CHECK(usage.find("--start-cell") != std::string::npos);
    CHECK(usage.find("--start-yaw") != std::string::npos);
    CHECK(usage.find("--collect-all") != std::string::npos);
    CHECK(usage.find("--calm") != std::string::npos);
    CHECK(usage.find("--night") != std::string::npos);
    CHECK(usage.find("--skip-intro") != std::string::npos);
    // With the space in front it is not the end of --skip-intro.
    CHECK(usage.find(" --intro") != std::string::npos);
}

TEST_CASE("the calm switch asks for a run without a shade and changes nothing else") {
    CHECK_FALSE(parse({}).options.calm);
    const game::StartOptionsResult result = parse({"--calm"});
    CHECK(result.error.empty());
    CHECK(result.options.calm);
    CHECK_FALSE(result.options.play);
    CHECK_FALSE(result.options.menuCamera.enabled);
    CHECK_FALSE(result.options.seedGiven);
    // Together with the others, in any place.
    const game::StartOptionsResult both = parse({"--play", "--calm", "--seed", "7"});
    CHECK(both.error.empty());
    CHECK(both.options.calm);
    CHECK(both.options.play);
    CHECK(both.options.seed == 7U);
    // It takes no value.
    CHECK_FALSE(parse({"--calm", "on"}).error.empty());
}

TEST_CASE("a seed on the command line is remembered as given, also the default one") {
    CHECK(parse({"--seed", "42"}).options.seedGiven);
    // Seed 1 is the default, but asking for it by name is still asking.
    const game::StartOptionsResult one = parse({"--seed", "1"});
    CHECK(one.options.seed == game::DEFAULT_MAZE_SEED);
    CHECK(one.options.seedGiven);
    CHECK_FALSE(parse({"--play"}).options.seedGiven);
}

TEST_CASE("a seed is read from digits only, up to the largest 32 bit number") {
    std::uint32_t seed = 7;
    CHECK(game::parseSeed("0", seed));
    CHECK(seed == 0U);
    CHECK(game::parseSeed("482913", seed));
    CHECK(seed == 482913U);
    CHECK(game::parseSeed("4294967295", seed));
    CHECK(seed == 4294967295U);
    // Leading zeros are digits like any other.
    CHECK(game::parseSeed("007", seed));
    CHECK(seed == 7U);

    // Everything else is refused and leaves the seed as it was.
    seed = 7;
    CHECK_FALSE(game::parseSeed("", seed));
    CHECK_FALSE(game::parseSeed("4294967296", seed));
    CHECK_FALSE(game::parseSeed("99999999999999999999999", seed));
    CHECK_FALSE(game::parseSeed("-1", seed));
    CHECK_FALSE(game::parseSeed("12a", seed));
    CHECK_FALSE(game::parseSeed(" 12", seed));
    CHECK_FALSE(game::parseSeed("1.5", seed));
    CHECK(seed == 7U);
}

TEST_CASE("the start cell is two whole numbers with a comma, and only the form is checked here") {
    const game::StartOptionsResult result = parse({"--play", "--start-cell", "3,12"});
    CHECK(result.error.empty());
    REQUIRE(result.options.startCell.has_value());
    const game::MazeCell cell = result.options.startCell.value_or(game::MazeCell{});
    CHECK(cell.x == 3);
    CHECK(cell.z == 12);
    // It changes nothing else.
    CHECK(result.options.play);
    CHECK_FALSE(result.options.startYawDegrees.has_value());
    CHECK_FALSE(result.options.collectAll);

    // Whether a cell is in the maze depends on its size, which is not known here.
    CHECK(parse({"--start-cell", "0,0"}).error.empty());
    CHECK(parse({"--start-cell", "999,999"}).error.empty());

    CHECK_FALSE(parse({"--start-cell"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "3"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "3,"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", ",3"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "3 4"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "3,4,5"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "-1,2"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "1.5,2"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "a,b"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "99999999999,1"}).error.empty());
    CHECK_FALSE(parse({"--start-cell", "4294967295,1"}).error.empty());
}

TEST_CASE("the start direction is any number of degrees") {
    const game::StartOptionsResult result = parse({"--start-yaw", "90"});
    CHECK(result.error.empty());
    CHECK(result.options.startYawDegrees.value_or(0.0F) == 90.0F);
    CHECK_FALSE(result.options.startCell.has_value());

    CHECK(parse({"--start-yaw", "-45.5"}).options.startYawDegrees.value_or(0.0F) == -45.5F);
    // A direction of 0 is a direction: North, not "not given".
    CHECK(parse({"--start-yaw", "0"}).options.startYawDegrees.has_value());

    CHECK_FALSE(parse({"--start-yaw"}).error.empty());
    CHECK_FALSE(parse({"--start-yaw", "north"}).error.empty());
    CHECK_FALSE(parse({"--start-yaw", "90deg"}).error.empty());
    CHECK_FALSE(parse({"--start-yaw", "inf"}).error.empty());
}

TEST_CASE("collect-all is a switch without a value") {
    const game::StartOptionsResult result = parse({"--play", "--collect-all", "--seed", "5"});
    CHECK(result.error.empty());
    CHECK(result.options.collectAll);
    CHECK(result.options.play);
    CHECK(result.options.seed == 5U);
    CHECK_FALSE(parse({"--play"}).options.collectAll);
}

TEST_CASE("the night switch names a night of the campaign, 1 to 5") {
    for (const char* text : {"1", "2", "3", "4", "5"}) {
        const game::StartOptionsResult result = parse({"--night", text});
        CHECK(result.error.empty());
        CHECK(result.options.night == text[0] - '0');
        // It changes nothing else: the seed is not given, and no round is asked for
        // by --play.
        CHECK_FALSE(result.options.seedGiven);
        CHECK_FALSE(result.options.play);
        CHECK_FALSE(result.options.calm);
    }
    // A night the campaign does not have, and anything that is not a whole number.
    for (const char* text : {"0", "6", "12", "-1", "two", "2.0", ""}) {
        CAPTURE(text);
        const game::StartOptionsResult result = parse({"--night", text});
        CHECK_FALSE(result.error.empty());
        CHECK(result.error.find("--night") != std::string::npos);
    }
    CHECK_FALSE(parse({"--night"}).error.empty());

    // Next to the switches that set up a picture, in any order.
    const game::StartOptionsResult all =
        parse({"--collect-all", "--night", "5", "--seed", "42", "--start-cell", "3,4"});
    CHECK(all.error.empty());
    CHECK(all.options.night == 5);
    CHECK(all.options.seed == 42U);
    CHECK(all.options.seedGiven);
    CHECK(all.options.collectAll);
}
