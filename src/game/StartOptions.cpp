// StartOptions: what the command line asks the game to start with.
// See docs/modules/game/menu-camera.md
#include "game/StartOptions.hpp"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <string_view>

namespace game {

namespace {

// The switches, as they are typed.
constexpr std::string_view SEED_SWITCH = "--seed";
constexpr std::string_view MENU_CAMERA_SWITCH = "--menu-camera";
constexpr std::string_view MENU_SHOT_SWITCH = "--menu-shot";
constexpr std::string_view MENU_TIME_SWITCH = "--menu-time";
constexpr std::string_view PLAY_SWITCH = "--play";

// The two names --menu-shot accepts.
constexpr std::string_view WALK_SHOT_NAME = "walk";
constexpr std::string_view GLIDE_SHOT_NAME = "glide";

// Reads a number of seconds, with or without a fraction. False when the text is not
// a number.
bool parseSeconds(const char* text, float& seconds) {
    // strtof and not from_chars: the standard library of Apple clang cannot read
    // a float with from_chars yet. strtof sets end to the first character it did not
    // use, so the text was a number only if that is the end of the text and something
    // was read at all.
    char* end = nullptr;
    const float value = std::strtof(text, &end);
    if (end == text || *end != '\0' || !std::isfinite(value)) {
        return false;
    }
    seconds = value;
    return true;
}

} // namespace

// Digits only: no sign, no spaces, and not more than a std::uint32_t holds.
bool parseSeed(std::string_view text, std::uint32_t& seed) {
    if (text.empty()) {
        return false;
    }
    // Digit after digit: the number so far times ten, plus the new digit. It is kept in
    // 64 bits, where one more digit than a seed can have still fits, so a number that
    // is too large is noticed and does not wrap around.
    constexpr std::uint64_t BASE = 10;
    constexpr std::uint64_t LARGEST_SEED = std::numeric_limits<std::uint32_t>::max();
    std::uint64_t value = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return false;
        }
        value = value * BASE + static_cast<std::uint64_t>(character - '0');
        if (value > LARGEST_SEED) {
            return false;
        }
    }
    seed = static_cast<std::uint32_t>(value);
    return true;
}

const char* const START_OPTIONS_USAGE =
    "Switches: --seed <number>, --play, --menu-camera, --menu-shot <walk|glide>, "
    "--menu-time <seconds>";

StartOptionsResult parseStartOptions(std::span<const char* const> arguments) {
    StartOptionsResult result;
    StartOptions& options = result.options;

    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const std::string_view name = arguments[i];

        // The two switches without a value.
        if (name == MENU_CAMERA_SWITCH) {
            options.menuCamera.enabled = true;
            continue;
        }
        if (name == PLAY_SWITCH) {
            options.play = true;
            continue;
        }

        // Every other switch takes the next word as its value.
        const bool known =
            name == SEED_SWITCH || name == MENU_SHOT_SWITCH || name == MENU_TIME_SWITCH;
        if (!known) {
            result.error = "Unknown switch: " + std::string(name);
            return result;
        }
        if (i + 1 == arguments.size()) {
            result.error = "The switch " + std::string(name) + " needs a value";
            return result;
        }
        ++i;
        const char* const value = arguments[i];

        bool understood = false;
        if (name == SEED_SWITCH) {
            understood = parseSeed(value, options.seed);
            options.seedGiven = understood;
        } else if (name == MENU_TIME_SWITCH) {
            understood = parseSeconds(value, options.menuCamera.timeOffset);
        } else if (std::string_view(value) == WALK_SHOT_NAME) {
            options.menuCamera.shot = MenuShot::CorridorWalk;
            understood = true;
        } else if (std::string_view(value) == GLIDE_SHOT_NAME) {
            options.menuCamera.shot = MenuShot::HighGlide;
            understood = true;
        }
        if (!understood) {
            result.error = "The switch " + std::string(name) + " does not accept \"" +
                           std::string(value) + "\"";
            return result;
        }
    }
    return result;
}

} // namespace game
