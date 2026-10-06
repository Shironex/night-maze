// Tests of game::parseStartOptions: the switches of the command line.
// See docs/modules/game/menu-camera.md
#include "game/StartOptions.hpp"

#include <doctest/doctest.h>

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
    CHECK_FALSE(result.options.menuCamera.enabled);
    CHECK(result.options.menuCamera.shot == game::MenuShot::CorridorWalk);
    CHECK(result.options.menuCamera.timeOffset == 0.0F);
    // The game then opens with the main menu.
    CHECK_FALSE(result.options.play);
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
}
