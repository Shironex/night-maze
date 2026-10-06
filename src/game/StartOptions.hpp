// StartOptions: what the command line asks the game to start with.
// See docs/modules/game/menu-camera.md
#pragma once

#include "game/MazeWorld.hpp"
#include "game/MenuCamera.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace game {

// Plain data and one function without OpenGL, like the rest of the game_logic library,
// so tests can use it. The switches exist for one purpose: to start the game straight in
// the menu camera, in a chosen maze and at a chosen place, so the picture can be
// recorded without touching the keyboard.

/// How the game starts. Without switches on the command line it is the game as it
/// always started: the default maze and the player at the start.
struct StartOptions {
    /// The seed of the first maze (MazeSettings::seed).
    std::uint32_t seed = DEFAULT_MAZE_SEED;

    /// The settings of the menu camera the game starts with.
    MenuCameraSettings menuCamera;
};

/// What parseStartOptions found.
struct StartOptionsResult {
    /// The options. Only meaningful when error is empty.
    StartOptions options;

    /// Empty when every switch was understood. Otherwise one line that names the switch
    /// that was not, for the log.
    std::string error;
};

/// The switches the game understands, one per line: main.cpp logs it after an error.
extern const char* const START_OPTIONS_USAGE;

/// Reads the switches of the command line. arguments are the words after the name of
/// the program (argv without its first entry).
///
///     --seed <number>        the seed of the first maze, a whole number from 0 up
///     --menu-camera          start with the menu camera switched on
///     --menu-shot <name>     the shot of the menu camera: walk or glide
///     --menu-time <seconds>  start the shot that many seconds into its loop
///
/// An unknown switch, a missing value or a value that is not a number is an error: the
/// result then carries a message and the game should not start.
StartOptionsResult parseStartOptions(std::span<const char* const> arguments);

} // namespace game
