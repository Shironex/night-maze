# Building Night Maze

This is how I build, run and check the game: a C++20 program on OpenGL 4.1 (core profile), built with CMake 3.24 or newer. It is made and tested on Windows.

## Prerequisites

### Windows

| Tool | What for | Notes |
|---|---|---|
| Visual Studio 2022 or only the Build Tools for Visual Studio 2022, with the workload "Desktop development with C++" | the MSVC compiler, the Windows SDK, CMake, Ninja, clang-format and clang-tidy | I use the Build Tools without the IDE (17.14, MSVC 19.44). The workload brings CMake (3.31.6 on my PC), Ninja, and the LLVM tools in `VC\Tools\Llvm\x64\bin` |
| git | CMake clones every library at configure time | must be on `PATH` of the same terminal |
| GNU make | the `make` shortcuts below | optional for building, needed for `make check`. On my PC it comes from Scoop (`scoop install make`, GNU Make 4.4.1). It is not part of Visual Studio |
| Python 3 | the scripts in `tools/` | only for assets, recordings and the README pictures. The standard library is enough for every script but one: `tools/make_music.py`, which writes the menu theme, needs numpy (`python -m pip install numpy`, I use 2.3). Building, testing and `make check` need no Python. I use 3.13 |
| Node 22 and pnpm 10 | the notices file and the README pictures | only for `tools/build-notices.mjs` and `pnpm showcase`. The versions are in CI and `package.json` |

You need a driver with OpenGL 4.1. On an old driver, a remote desktop session or a virtual machine the log says `Failed to create a window with an OpenGL 4.1 Core context`.

#### The developer shell

Installing Visual Studio does not put `cmake`, `cl.exe` or the LLVM tools on the `PATH` of an ordinary terminal. I work in a PowerShell that has loaded the Visual Studio developer environment:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64
```

With the full IDE the folder is `Community` (or your edition). The Start menu shortcut "Developer PowerShell for VS 2022" does the same.

The `Makefile` depends on this. It builds the path of clang-format and clang-tidy from `VCINSTALLDIR`, a variable that only the developer shell sets, because those two programs are not on the `PATH` even there. So `make format`, `make format-check`, `make tidy` and `make check` need the shell. Without `VCINSTALLDIR` the Makefile looks for both on the `PATH`. `make debug`, `make release` and `make test` only need `cmake`.

### macOS

You need the Xcode command line tools (`xcode-select --install`), CMake 3.24 or newer (`brew install cmake`) and git. Full Xcode is not needed. Ninja is optional: CMake uses Unix Makefiles. For `make tidy` you also need `brew install llvm`, and the Makefile finds its `clang-tidy` through `brew --prefix llvm`.

I last ran the game on a Mac around version 0.9.0. Since then the video decoder for macOS (`src/video/VideoDecoderApple.mm`, Objective-C++ on AVFoundation) and the sound engine were added, and I have not started the game on a Mac again. Today the code is only compiled and unit tested there by a GitHub runner: the `macos` job of `.github/workflows/ci.yml` (macOS 15) configures the Release preset, builds with `-k` so one log shows every error, and runs `ctest`. The job is marked `continue-on-error`, so it reports and does not block. It does not prove that the window opens, the video plays or the sound works.

## Getting the source

```sh
git clone https://github.com/Shironex/night-maze.git
cd night-maze
```

The first configure needs the internet, because CMake downloads the libraries (see Dependencies). No library is installed by hand.

## Presets and make targets

`CMakePresets.json` has two configure presets and two build presets, `debug` and `release`. They set the build type and write `compile_commands.json`, and they build into `build/debug` and `build/release`. They do not name a generator, so Windows gets the Visual Studio generator and macOS gets Unix Makefiles.

```sh
cmake --preset debug
cmake --build --preset debug

cmake --preset release
cmake --build --preset release
```

The Visual Studio generator holds both configurations in one folder, so the program is in a subfolder: `build/debug/Debug/night_maze.exe` and `build/release/Release/night_maze.exe`. On macOS it is `build/debug/night_maze`. The generator ignores `CMAKE_BUILD_TYPE`, and CMake may print a warning about an unused variable. That is expected.

`make` wraps these steps. Plain `make` prints the list.

| Command | What it does |
|---|---|
| `make debug` | configures and builds the Debug version |
| `make release` | configures and builds the Release version |
| `make run` | builds Debug and starts it |
| `make run-release` | builds Release and starts it |
| `make test` | builds Debug and runs the unit tests with `ctest` |
| `make test-release` | the same for Release |
| `make format` | reformats `src/` and `tests/` with clang-format, including the `.mm` file |
| `make format-check` | fails if any of those files is not formatted |
| `make tidy` | runs clang-tidy over `src/` and `tests/` with warnings as errors |
| `make check` | `format-check`, `test`, `test-release` and `tidy`, in that order |
| `make clean` | deletes the `build/` folder |

Run `make` from the repository root. On Windows, clang-tidy needs a `compile_commands.json`, which the Visual Studio generator does not write, so `make tidy` configures a second folder, `build/ninja-debug`, with Ninja. Nothing is compiled there.

The code rules are in `.clang-format` (LLVM style, 4 spaces, 100 columns) and `.clang-tidy` (`bugprone`, `performance`, `modernize` and naming). Warnings are strict for our own targets only: `/W4 /w14062 /permissive-` on MSVC, `-Wall -Wextra -Wpedantic` elsewhere.

## Running the game

Run it from the repository root:

```sh
build/release/Release/night_maze.exe
```

The game finds `assets/` next to the executable, so the working directory does not matter for that. It does matter for two small files that are written to the working directory: `night-maze-settings.txt` (your settings) and `imgui.ini` (the pinned debug panel). Both are in `.gitignore`.

The program is a console application on purpose: the log goes to the terminal you started it from, and a failed start prints a line that begins `Fatal:`.

#### The assets folder

On Windows the build copies `assets/` next to the executable, in a custom target called `copy_assets`. It runs with every normal build, but not when you build only the `night_maze` target. On macOS the build makes a symbolic link instead, so edits to a shader show up without a build.

The consequence on Windows: after you change a shader, a texture or a model, refresh the copy, and then press "Reload shaders" in the debug window for a shader.

```sh
cmake --build --preset debug --target copy_assets
```

This does not build the program, so it also works while the game is running.

### Command line switches

The switches are read before the window opens. A wrong one prints an `[error]` line with the list of switches and exits with a non-zero code. The code is `src/game/StartOptions.cpp`.

| Switch | Meaning |
|---|---|
| `--play` | skip the main menu and start straight in a round |
| `--seed <number>` | the seed of the first maze, a whole number from 0 to 4294967295 (digits only) |
| `--night <1..5>` | start that night of the campaign at once, with no title card. The campaign of such a run is its own, seeded from `--seed`, and your settings file is neither read nor changed |
| `--daily <YYYYMMDD>` | start the maze of that day ("Tonight's hedge") at once, with no title card. That day is also the day of the menu entry for the whole run. The saved best time of the day is read and never written |
| `--start-cell <column>,<row>` | with `--play`: put the player in the middle of that cell in the first round. A cell that does not exist in the maze is refused |
| `--start-yaw <degrees>` | the direction the player looks in at the start of the first round |
| `--collect-all` | the first round starts with every crystal and the heartstone collected, so the gate is open |
| `--calm` | a calm night for this run: no shadow in any maze. Settings are untouched |
| `--menu-camera` | start in the menu camera mode, where the game shows itself without HUD or menu (this skips the main menu too) |
| `--menu-shot <walk\|glide>` | the shot of the menu camera: a walk through the corridors or a high glide. It does not switch the mode on by itself |
| `--menu-time <seconds>` | start the shot that many seconds into its loop. A number with a decimal point, may be negative |
| `--menu-background <video\|still\|scene>` | what is behind the main menu: the recorded video (default), the still picture, or the live scene |
| `--skip-intro` | never play the intro in this run |
| `--intro` | play the intro now, even if it was seen. `--skip-intro` wins if both are given |

Any switch except the two intro ones counts as a tool switch. A run with a tool switch never opens with the intro, because the scripts start the game in fresh folders where the intro would count as unseen. It also never asks the clock for the day of "Tonight's hedge": the day is the one of `--daily`, or 8 October 2026, so a picture of the menu does not change from day to day. Example, a round of the second night in a fixed maze without the shadow:

```sh
build/release/Release/night_maze.exe --night 2 --seed 7 --calm
```

## Running the tests

The unit tests are doctest cases for the code that needs no window, over 800 of them. The default build also builds the program `night_maze_tests`, and `ctest` runs it as one test.

```sh
ctest --test-dir build/debug -C Debug --output-on-failure
```

`make test` and `make test-release` do the build and this call together. The `-C` option names the configuration, which the Visual Studio generator needs. A doctest filter runs part of it:

```sh
build/debug/Debug/night_maze_tests.exe -tc="*Bloom*"
```

The tests need no sound card and no OpenGL context. Some read real files from `assets/` through a path compiled in at configure time.

## The gate: make check

`make check` is what I run before a commit. It checks the formatting, builds Debug and Release, runs the unit tests of both, and runs clang-tidy over every file. Start it in the developer shell. On my PC it takes about 35 minutes, most of it clang-tidy, which compiles every file once more.

What CI runs is smaller on purpose (`.github/workflows/ci.yml`, on every push to `main` and every pull request):

- a Windows job that configures and builds Release with the same flags as the release, runs `ctest`, and checks that the committed notices file matches the dependencies;
- a job for the release notes and the update feed;
- a clang-format job that reports and does not block;
- the macOS compile check described above.

clang-tidy is not in CI, because it needs a second configure and a matching LLVM version.

## Dependencies

All libraries except GLAD are downloaded by CMake at configure time with FetchContent. The versions are pinned in `cmake/Dependencies.cmake`, to release tags, and for stb to a commit hash, because it has no tags.

| Library | Version | Used for |
|---|---|---|
| GLFW | 3.4 | window, context, input |
| GLM | 1.0.3 | vectors and matrices |
| Dear ImGui (docking branch) | v1.92.9b | HUD and the debug window |
| doctest | v2.5.3 | unit tests |
| stb_image | commit `2c980bb` (2.30) | decoding PNG files |
| FreeType | VER-2-14-3 | reads fonts for RmlUi |
| RmlUi | 6.3 | the menus |
| miniaudio | 0.11.25 | sound |

GLAD (OpenGL 4.1 core) is generated code and lives in `external/glad`. The two single header libraries, stb_image and miniaudio, are compiled once from small files in `external/`.

When a version in `Dependencies.cmake` changes, `THIRD-PARTY-NOTICES.txt` has to be generated again, because it is assembled from the licence files of the downloaded sources and committed. After a configure:

```sh
node tools/build-notices.mjs --deps build/release/_deps
```

CI runs the same script into a temporary file and fails on any difference. doctest is left out, because it is not shipped.

## How a release is built

The version has one source, the line `project(NightMaze VERSION ...)` in `CMakeLists.txt`. For a release I bump it, add a `## x.y.z (date)` section to `CHANGELOG.md`, wait for CI to be green, and push the tag `vx.y.z`. The workflow `.github/workflows/release.yml` then builds the Windows x64 package, writes the manifest and the news feed, and publishes them as a GitHub release after I approve the `release` environment, which holds the signing key. The launcher, which lives in its own repository, reads the signed manifest of the newest release and installs from it.

## Problems I would expect

| Symptom | Cause and fix |
|---|---|
| `cmake` is not recognized, or `make format-check` cannot find clang-format | the terminal is not a Visual Studio developer shell. Load it as described above |
| `LINK : fatal error LNK1168` while building | `night_maze.exe` is running and Windows locks it. Close the game and build again, or refresh only the assets with `--target copy_assets` |
| A changed shader or texture does not show up, or the log says a file cannot be opened | the copy of `assets/` next to the executable is old or missing. Run `cmake --build --preset debug --target copy_assets`, and press "Reload shaders" for a shader |
| A generator mismatch error, or odd values after a version change | the build folder was made by another generator or an older configuration. Delete that preset's folder (`build/debug`) and configure again. It costs a new download |
| The configure step fails while downloading a library | git is not on the `PATH`, or there is no network at the first configure. Fix that and run the configure again |

If the debug window is not visible, press the key left of `1`, because it starts hidden.
