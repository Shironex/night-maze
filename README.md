# Night Maze

A first-person night maze in C++ and OpenGL. The moon sees every corridor, you see one: you walk with a flashlight, collect crystals to open the gate, and find the exit.

> [!WARNING]
> Night Maze is a university project that I keep working on to learn OpenGL. It changes a lot, and things will break here and there. Expect rough edges and breaking changes between versions.

## Play it

1. Download the launcher from the [latest release](https://github.com/Shironex/night-maze-launcher/releases/latest). Windows 10 or 11, 64 bit, only for now.
2. Run the installer. It is not signed with a paid certificate, so Windows shows "Windows protected your PC". Click "More info", then "Run anyway".
3. The launcher installs the game and keeps it up to date.

The game needs a graphics card and driver with OpenGL 4.1.

## Controls

| Key | Action |
| --- | --- |
| Mouse | Look around |
| W, A, S, D | Walk |
| Left Shift | Sprint, while there is stamina |
| F | Flashlight on and off |
| E or left click | Use what you look at (pull a lever, read a note, close a note) |
| Hold M | Show the map; you stand still while you read it |
| R | Start the round again |
| Esc | Pause; in the menus, go back |

In the menus: arrow keys or Tab to move, Enter to choose.

Debug keys, not meant for play: the key left of 1 (`) shows the debug window, N switches to free flight (Space up, Left Shift down), F2 switches the menu camera.

## What is in the game today

- A maze generated from a seed: the same seed and difficulty always give the same maze
- Three difficulty levels (Easy, Normal, Hard) that change the size of the maze, the number of crystals and the battery
- Crystals to collect, and a flashlight battery that drains and that crystals charge
- A gate that opens when you have enough crystals, and an exit in the far corner
- Levers that open shortcuts and notes that point you somewhere
- Flasks of tea in dead ends: walk into one and sprinting costs no stamina for 20 seconds
- Moon and flashlight shadows, fog, glow, a night sky and uneven ground
- A map, shown while you hold M, with only the corridors you have already seen
- A main menu with a looping video background, pause, a round summary and a settings screen (mouse sensitivity, field of view, fullscreen, window size, volume)
- The first sounds: flashlight, crystals, levers, the gate and a low battery pulse

[CHANGELOG.md](CHANGELOG.md) has the details for every version.

## Built with

C++20 and OpenGL 4.1 (core profile). CMake fetches the libraries at configure time, except GLAD:

- [GLFW](https://www.glfw.org/): window, OpenGL context and input
- [GLM](https://github.com/g-truc/glm): vector and matrix math
- [GLAD](https://github.com/Dav1dde/glad) (generated loader, kept in `external/glad`): loads the OpenGL functions
- [Dear ImGui](https://github.com/ocornut/imgui): the debug window
- [RmlUi](https://github.com/mikke89/RmlUi) with [FreeType](https://freetype.org/): the menus, written as HTML and CSS style documents
- [stb_image](https://github.com/nothings/stb): decodes PNG and other image files
- [miniaudio](https://miniaud.io/): plays the sounds
- [doctest](https://github.com/doctest/doctest): unit tests

The menu video is decoded through the operating system's own decoders.

## Build it yourself

You need CMake 3.24 or newer, a C++20 compiler and git (CMake downloads the libraries). On Windows I use the Visual Studio Build Tools.

```sh
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release -C Release --output-on-failure
```

`make` wraps the same steps: `make run` builds and starts the Debug version, `make release`, `make test` and `make check` do what they say, and plain `make` lists the targets. Run the game from the repository root.

Step by step guides: [docs/guides/build-windows.md](docs/guides/build-windows.md) and [docs/guides/build-macos.md](docs/guides/build-macos.md). Those guides and most of `docs/` are in Polish, because they are my study notes.

I built and ran the earlier milestones on a Mac (Apple Silicon). The newer code, including the menus, the video and the audio, has not been built there yet.

## Repository layout

- `src/`: the game and its engine code (`core`, `gfx`, `scene`, `game`, `audio`, `video`, `ui`, `debug`)
- `assets/`: shaders, textures, models, skybox, UI documents, fonts, sounds and the menu video
- `tests/`: unit tests
- `tools/`: scripts that make assets (sounds, the menu loop, Blender scripts)
- `docs/`: guides, module notes and decision notes (mostly Polish)

The launcher that installs and updates the game has its own repository: [night-maze-launcher](https://github.com/Shironex/night-maze-launcher). Its source lived in a `launcher/` folder here until 2026-10-07.

## What is next

Plans, not promises: a story told through notes scattered in the maze, an enemy that moves only when it is unlit and unseen, and more sound. The decision notes are in [docs/decisions](docs/decisions).

## Licence

The code is MIT, see [LICENSE](LICENSE). The assets are not covered by it: the models, textures, sounds, video, user interface art and story text under `assets/` and `docs/story/` remain all rights reserved, and reusing them needs my permission. Third-party libraries and fonts keep their own licences, listed in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
