// StartOptions: what the command line asks the game to start with.
// See docs/modules/game/menu-camera.md
#pragma once

#include "game/MazeWorld.hpp"
#include "game/MenuBackground.hpp"
#include "game/MenuCamera.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

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

    /// True when the seed above was named on the command line (--seed). The main menu
    /// then offers that seed for the first game, and not a random one.
    bool seedGiven = false;

    /// The settings of the menu camera the game starts with.
    MenuCameraSettings menuCamera;

    /// True: skip the main menu and start straight in a round. For tests, for scripts
    /// and for recording. The menu camera (--menu-camera) skips the main menu too.
    bool play = false;

    /// What the main menu shows behind itself (game::chooseMenuBackground): the
    /// recorded video unless --menu-background asks for the still picture or for the
    /// live scene. The live scene is what the video is recorded from, and the way to
    /// compare the two.
    MenuBackground menuBackground = MenuBackground::Video;

    /// The cell of the first round the player starts in (--start-cell, with --play): in
    /// the middle of it. Empty: the start cell of the maze. Only the first round of the
    /// program starts there. The switch only checks that the text is two whole numbers:
    /// whether they are a cell of the maze is known once the maze is built, and the
    /// application then refuses a cell that is not one.
    std::optional<MazeCell> startCell;

    /// The direction the player looks in at the start of the first round (--start-yaw),
    /// in degrees like Camera::yawDegrees. Empty: towards the first open side of the
    /// start cell.
    std::optional<float> startYawDegrees;

    /// True: the first round starts with every crystal collected, so the gate is open
    /// (--collect-all). The crystals are picked up by the rules of the round, not set.
    bool collectAll = false;

    /// True: a calm night for this run, whatever the settings file says: no maze of the
    /// run has a shade. The settings are neither read for it nor changed by it.
    bool calm = false;

    /// The night of the campaign the game starts in at once (--night), 1 to
    /// CAMPAIGN_NIGHT_COUNT, or 0 for none. Like --play it skips the main menu, and it
    /// shows no title card. The campaign of such a run is one of its own: its seed is
    /// the seed of the command line (--seed) or TOOL_CAMPAIGN_SEED, so the maze is always
    /// the same, and the campaign in the settings file is neither read nor changed.
    int night = 0;

    /// True when one of the switches above was on the command line (--seed, --play,
    /// --menu-camera, --menu-shot, --menu-time, --menu-background, --start-cell,
    /// --start-yaw, --collect-all, --calm or --night): a tool or a test is driving the game. Such a
    /// run never opens with the intro (game::startMode). Scripts start the game in fresh folders
    /// without a settings file, where the intro would count as not seen.
    bool toolSwitch = false;

    /// True: never play the intro in this run (--skip-intro), also when it was not seen.
    bool skipIntro = false;

    /// True: play the intro now, also when it was seen and next to the switches of
    /// a tool (--intro). --skip-intro wins over it when both are given.
    bool intro = false;
};

/// What parseStartOptions found.
struct StartOptionsResult {
    /// The options. Only meaningful when error is empty.
    StartOptions options;

    /// Empty when every switch was understood. Otherwise one line that names the switch
    /// that was not, for the log.
    std::string error;
};

/// Reads a seed: a whole number from 0 to the largest std::uint32_t (4294967295),
/// written with digits only. False, and seed left as it was, when the text is anything
/// else: empty, negative, too large or with other characters in it. The command line
/// (--seed) and the seed field of the main menu both read their text with it.
bool parseSeed(std::string_view text, std::uint32_t& seed);

/// The switches the game understands, one per line: main.cpp logs it after an error.
extern const char* const START_OPTIONS_USAGE;

/// Reads the switches of the command line. arguments are the words after the name of
/// the program (argv without its first entry).
///
///     --seed <number>        the seed of the first maze, a whole number from 0 up
///     --menu-camera          start with the menu camera switched on
///     --menu-shot <name>     the shot of the menu camera: walk or glide
///     --menu-time <seconds>  start the shot that many seconds into its loop
///     --play                 skip the main menu and start in a round
///     --menu-background <name>  behind the main menu: video, still or scene
///     --start-cell <column>,<row>  the cell the first round starts in (with --play)
///     --start-yaw <degrees>  the direction the player looks in at the start
///     --collect-all          the first round starts with every crystal collected
///     --calm                 no shade in this run (a calm night), settings untouched
///     --night <1..5>         start that night of the campaign at once, settings untouched
///     --skip-intro           never play the intro in this run
///     --intro                play the intro now, also when it was seen
///
/// An unknown switch, a missing value or a value that is not a number is an error: the
/// result then carries a message and the game should not start.
StartOptionsResult parseStartOptions(std::span<const char* const> arguments);

} // namespace game
