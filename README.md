<div align="center">

<img src="docs/showcase/hero.webp" alt="Night Maze: the main menu, a corridor at night and a crystal in front of a cracked wall" width="100%" />

<h1>Night Maze</h1>

**A first person maze game in C++20 and OpenGL 4.1: a stone maze at night, one flashlight, crystals to find.**

[![Latest release](https://img.shields.io/github/v/release/Shironex/night-maze?style=flat&color=ffb854)](https://github.com/Shironex/night-maze/releases/latest)
[![CI](https://img.shields.io/github/actions/workflow/status/Shironex/night-maze/ci.yml?branch=main&style=flat&label=ci)](https://github.com/Shironex/night-maze/actions/workflows/ci.yml)
[![Licence: MIT](https://img.shields.io/badge/licence-MIT-56d6ca?style=flat)](LICENSE)

[Play it](#play-it) · [The launcher](https://github.com/Shironex/night-maze-launcher) · [Changelog](CHANGELOG.md) · [Build it](#build-from-source)

> The moon sees every corridor. You see one.

</div>

> [!WARNING]
> Night Maze is a university project that I keep working on to learn OpenGL. It changes a lot, and things will break here and there. Expect rough edges and breaking changes between versions.

---

## What is Night Maze?

You stand in a stone maze at night with a flashlight. Crystals are scattered through the corridors. Collect enough of them and the gate at the far end sinks into the ground. Walk through it and the round is over.

The battery of the flashlight runs down while the light is on, and every crystal charges it a little. Nothing hunts you and no clock runs against you: an empty battery only leaves you in the moonlight until you find the next crystal.

I write the game to learn how real time graphics work, so most of what is on the screen is written by hand on top of OpenGL: the lighting, the shadows, the fog, the glow. There is no game engine underneath.

## Play it

1. Download the launcher from its [latest release](https://github.com/Shironex/night-maze-launcher/releases/latest). Windows 10 or 11, 64 bit, only for now.
2. Run the installer. It is not signed with a paid certificate, so Windows shows "Windows protected your PC". Click "More info", then "Run anyway".
3. The [launcher](https://github.com/Shironex/night-maze-launcher) downloads the game, keeps it up to date and starts it.

The game needs a graphics card and driver with OpenGL 4.1.

Windows is what ships. I last ran the game on a Mac around version 0.9.0. Since then the code is only compiled and unit tested on macOS by a GitHub runner, so I do not know whether the newest version works there.

## Screenshots

<table>
  <tr>
    <td width="50%"><img src="docs/showcase/menu.webp" alt="Night Maze: Main menu" /></td>
    <td width="50%"><img src="docs/showcase/corridor.webp" alt="Night Maze: A corridor" /></td>
  </tr>
  <tr>
    <td align="center"><sub>The main menu, over a video loop recorded from the game.</sub></td>
    <td align="center"><sub>A crystal glows at the end of a corridor, and a flask of tea lies on the way.</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/showcase/cracked-wall.webp" alt="Night Maze: A cracked wall" /></td>
    <td width="50%"><img src="docs/showcase/note.webp" alt="Night Maze: A note" /></td>
  </tr>
  <tr>
    <td align="center"><sub>A cracked wall behind a crystal. The seed decides which walls are worn.</sub></td>
    <td align="center"><sub>A note on the wall, read with E. This one is a line of the story.</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/showcase/lever.webp" alt="Night Maze: A lever" /></td>
    <td width="50%"><img src="docs/showcase/map.webp" alt="Night Maze: The map" /></td>
  </tr>
  <tr>
    <td align="center"><sub>A lever, just pulled: the mossy wall beside it sinks and opens a shortcut.</sub></td>
    <td align="center"><sub>The map while M is held: the corridors seen so far, two levers and a note.</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/showcase/glide.webp" alt="Night Maze: From above" /></td>
    <td width="50%"><img src="docs/showcase/settings.webp" alt="Night Maze: Settings" /></td>
  </tr>
  <tr>
    <td align="center"><sub>The maze from above, the way the camera behind the menu sees it.</sub></td>
    <td align="center"><sub>Settings: mouse sensitivity, field of view, volume, fullscreen and window size.</sub></td>
  </tr>
</table>

Every picture is the game window as it is, at 1280 x 720. The corridor, the cracked wall and the view from above are taken by the menu camera, which shows the game without the HUD; the others are from a round and from the menus.

## What is in the game

| Feature | What it does |
| --- | --- |
| Mazes from a seed | Every maze is generated from a number. The same seed and difficulty always give the same maze, with the crystals, flasks, levers, notes and worn walls in the same places. The main menu offers a random seed, and you can type your own. |
| Three difficulty levels | Easy, Normal and Hard change the size of the maze, the number of crystals and flasks, how many crystals the gate asks for and how long the battery lasts (table below). |
| Flashlight and battery | F switches the flashlight on and off. The battery only drains while the light is on, and each crystal gives back a quarter of it. Under 20 % the light flickers, the bar turns red and a slow pulse starts. At zero the light goes out until you find a crystal. |
| Crystals and the gate | The gate opens by itself once you carry enough crystals: you never need all of them. The exit is the cell farthest from the start, counted in steps. Walking through the open gate ends the round with "Through the gate": your time, your crystals, the difficulty and the seed. |
| Levers | A maze has up to two levers. Pull one with E and a wall somewhere else sinks into the ground, which opens a shortcut. |
| Notes and a story | Six notes are chalked on the walls of every maze. Two point towards the gate, two towards the nearest crystal, and two carry a line of story. The story has sixteen lines, read in order: when you finish a maze, the next one carries on where you stopped, also after you close the game. |
| Stamina and tea | Sprinting empties a thin bar in about six seconds. Run it empty and you are winded: no sprint until the bar is half full again. A flask of tea (one, two or three per maze) fills the bar and makes sprinting free for 20 seconds. |
| The map | Hold M and the map opens in the middle of the screen. It shows only the corridors you have already seen. While you hold it you stand still and cannot look around, and the battery keeps draining. |
| Worn walls | Some walls are cracked, mossy or have stones missing. Which ones is decided by the seed. The walls near the start are whole. |
| Sound | Nine short sounds: the flashlight switch (on, off, empty), the low battery pulse, a crystal, a flask, a lever, the gate and the breathing of a winded player. One volume slider. There is no music yet. |
| Menus and settings | A main menu over a video loop recorded from the game, a pause menu, the round end screen and a settings screen: mouse sensitivity, field of view, volume, fullscreen and window size. The settings are kept in a text file. |

| Level | Maze | Crystals | Gate opens at | Flasks of tea | Battery |
| --- | --- | --- | --- | --- | --- |
| Easy | 10 x 10 cells | 13 | 10 | 1 | 3:00 |
| Normal | 16 x 16 cells | 26 | 19 | 2 | 2:30 |
| Hard | 22 x 22 cells | 40 | 32 | 3 | 2:00 |

[CHANGELOG.md](CHANGELOG.md) has the details for every version.

## Controls

| Key | Action |
| --- | --- |
| Mouse | Look around |
| W, A, S, D | Walk |
| Left Shift | Sprint, while there is stamina |
| F | Flashlight on and off |
| E or left click | Use what you look at: pull a lever, read a note, close a note |
| Hold M | Show the map; you stand still while you read it |
| R | Start the round again on the same maze |
| Esc | Pause and resume; on the settings screen, go back |

In the menus: arrow keys or Tab to move, Enter to choose.

Debug keys, not meant for play: the key left of 1 (`) shows the debug window, N switches to free flight (Space up, Left Shift down), F2 switches the menu camera.

## How it is drawn

Everything here is in `src/gfx`, `src/game`, `src/assets` and `assets/shaders`.

| Technique | In this game |
| --- | --- |
| Lighting | Blinn-Phong per fragment. The moon is a directional light, the flashlight a spot light, and the sixteen crystals nearest to the eye each carry a point light. |
| Shadow maps | Two: one for the moon and one for the flashlight, softened with percentage closer filtering. |
| Normal mapping | In tangent space, on the walls, the ground and the models. The tangents are computed when a model is loaded. |
| HDR and tone mapping | The scene is drawn into a 16 bit float buffer and brought to the screen with ACES tone mapping. The colours are kept linear until the last step. |
| Bloom | A bright pass and a blur at half resolution, added back to the picture: the glow of the crystals. |
| Fog | Distance fog that is thicker near the ground, computed after the scene from its depth buffer. |
| Vignette | The corners of the picture are darkened a little. |
| Skybox | A cube map of the night sky with stars and a painted moon. |
| Reflections | Puddles and crystals mirror the sky: they sample the cube map of the skybox (environment mapping), not the scene around them. |
| Grass | A geometry shader turns every point into a tuft of three blades that move in the wind. |
| Terrain | The ground is built from a height map, so the floor of the corridors is gently uneven. |
| Models | OBJ models, made in Blender by the scripts in `tools/blender` and read by a loader of my own. |

## Built with

C++20 and OpenGL 4.1 (core profile), built with CMake 3.24 or newer. CMake downloads the libraries at configure time, except GLAD.

| Part | What |
| --- | --- |
| Window and input | [GLFW](https://www.glfw.org/) 3.4 |
| Math | [GLM](https://github.com/g-truc/glm) 1.0.3 |
| OpenGL loader | [GLAD](https://github.com/Dav1dde/glad) 2.0.8, generated and kept in `external/glad` |
| Menus | [RmlUi](https://github.com/mikke89/RmlUi) 6.3 with [FreeType](https://freetype.org/) 2.14.3: the menus are HTML and CSS style documents in `assets/ui` |
| HUD and debug window | [Dear ImGui](https://github.com/ocornut/imgui) 1.92.9b (docking branch) |
| Images | [stb_image](https://github.com/nothings/stb) 2.30 |
| Sound | [miniaudio](https://miniaud.io/) 0.11.25 |
| Menu video | The decoders of the operating system: Media Foundation on Windows |
| Tests | [doctest](https://github.com/doctest/doctest) 2.5.3, more than 600 test cases for the code that needs no window |
| Checks | clang-format and clang-tidy, and a CI run on every push to `main` |

## Build from source

You need CMake 3.24 or newer, a C++20 compiler and git (CMake downloads the libraries). On Windows I use the Visual Studio Build Tools 2022 and run these from its developer shell:

```sh
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release -C Release --output-on-failure
```

The game is then `build/release/Release/night_maze.exe`. Run it from the repository root.

`make` wraps the same steps (plain `make` lists the targets):

| Command | What it does |
| --- | --- |
| `make run` | Builds the Debug version and starts it |
| `make release` | Builds the Release version |
| `make test` | Builds the Debug version and runs the unit tests |
| `make check` | Everything I run before a commit: the format check, both builds, the unit tests of both, clang-tidy |

`make check` needs GNU make and the developer shell of Visual Studio, which brings clang-format, clang-tidy and Ninja.

Step by step guides: [docs/guides/build-windows.md](docs/guides/build-windows.md) and [docs/guides/build-macos.md](docs/guides/build-macos.md), both in Polish.

## Repository layout

| Folder | What is in it |
| --- | --- |
| `src/` | The game and its engine code: `core`, `gfx`, `scene`, `assets`, `game`, `audio`, `video`, `ui`, `debug` |
| `assets/` | Shaders, textures, models, skybox, menu documents, fonts, sounds and the menu video. All of it ships with the game |
| `tests/` | Unit tests |
| `tools/` | Scripts that make assets (sounds, the menu loop, Blender scripts), the release notes and the README pictures |
| `docs/` | Guides, module notes and decision notes |

The launcher that installs and updates the game has its own repository: [night-maze-launcher](https://github.com/Shironex/night-maze-launcher).

## Docs and the story

Most of `docs/` is in Polish, because these are my study notes: [guides](docs/guides), a note per module in [docs/modules](docs/modules), a note per library in [docs/libraries](docs/libraries), and about seventy short [decision notes](docs/decisions) on why something is done the way it is. [docs/README.md](docs/README.md) is the index.

The story behind the notes is in [docs/story/the-last-lamp.md](docs/story/the-last-lamp.md), in English under a short Polish note. It is a design draft: the sixteen lines on the notes and the title of the round end screen come from it, and the rest of it, an enemy among other things, is not in the game.

Plans, not promises: an enemy that moves only when it is unlit and unseen, and more sound.

## Showcase images

The pictures in this README come out of the game itself and are framed with [@noctcore/showcase-kit](https://github.com/noctcore/showcase-kit), the same way as the pictures of the launcher. These commands rebuild them:

```sh
cmake --preset release
cmake --build --preset release
python tools/capture_showcase.py   # the game window, into showcase-out/raw/
pnpm install
pnpm showcase                      # frames and banner, into docs/showcase/
```

The kit cannot start a native game, so the capture is a script of mine. It starts the Release build in a 1280 x 720 window, with fixed seeds, in a folder of its own (your settings are not touched), and saves what the window shows. The views without a HUD come from the menu camera, set with command line switches. The views from a round need a few key presses and mouse turns. The script sends them only while the game window is the active one, so leave the mouse and the keyboard alone while it runs. It needs Python 3, ffmpeg on `PATH` and Windows.

`pnpm showcase` only reads those pictures: from the same raw pictures it wrote the same files, byte for byte, every time I ran it on my PC. It needs Node 22, pnpm 10 and the Chromium of Playwright, downloaded once with `pnpm exec playwright install chromium`. The game itself is not a still picture (the crystals turn, a clock runs in the HUD), so a new capture is never identical to the last one.

The pictures live in `docs/showcase/` and not under `assets/`, because everything under `assets/` is packed into the game.

## Licence

The code is MIT, see [LICENSE](LICENSE). The assets are not covered by it: the models, textures, sounds, video, user interface art and story text under `assets/` and `docs/story/`, and the pictures of this README under `docs/showcase/`, remain all rights reserved, and reusing them needs my permission. Third-party libraries and fonts keep their own licences, listed in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
