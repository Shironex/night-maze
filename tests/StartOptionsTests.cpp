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
