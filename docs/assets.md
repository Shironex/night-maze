# Assets

Everything in `assets/` that I made is made by a script. The script is the source and the file in `assets/` is the output. The outputs are committed, so you can build and run the game without Blender. I never edit an output by hand, because the next run would overwrite it.

## Models and textures: tools/blender

The scripts in `tools/blender/` run inside headless Blender. I use Blender 5.2.1, which the first line of every `.obj` names. Another version changes that comment and maybe number formatting.

| File | What it does |
|---|---|
| `blender_common.py` | shared helpers: scene reset, boxes, UV projection, the material, the OBJ export with every option written out, review renders |
| `make_textures.py` | the colour pictures and a normal map for each (name plus `_normal`), into `assets/textures/` |
| the eleven `build_*.py` files | one model each (`build_crystal.py` and `build_lever.py` write two), into `assets/models/`. `build_gate.py` is the door of the exit, `build_gate_arch.py` the gatehouse over it, `build_gate_lantern.py` its lantern and `build_milestone.py` the stone in front of it |
| `make_skybox.py` | the six faces of the night sky, into `assets/skybox/` |
| `make_heightmap.py` | `assets/textures/heightmap.png`, the heights of the terrain |
| `make_all.py` | runs all of the above in one process, textures first, because a model script loads its PNG |

Run them from the repository root.

```powershell
blender --background --factory-startup --python tools/blender/make_all.py
blender --background --factory-startup --python tools/blender/build_gate.py -- --shots
```

Everything after `--` goes to the script. `--shots` also writes review renders, only to the temporary folder. Running a script twice gives the same bytes (constant sizes, fixed seeds), so `git status` shows what really changed.

### Conventions

| What | Rule |
|---|---|
| Axes | the game is Y up, -Z forward. The scripts are written in Blender space (Z up) and the exporter converts, so a Blender point (x, y, z) becomes (x, z, -y) |
| Scale | 1 unit is 1 metre. A maze cell is 2 by 2 m and a wall is 3 m tall |
| Origin | the middle of the base, the underside at height 0. The exception is `crystal_b`, whose origin is the base of its main shard |
| Faces | triangles only, with one normal per face (flat shading) |
| Material | exactly one per model, with a colour picture and a normal map |
| UVs | one UV unit per 2 m on stone and the gate (`box_project_uvs`). Sloped models use `face_project_uvs` with their own density |
| Texture paths in the `.mtl` | relative, so the file works on any computer |

### Adding a model

1. Copy `build_wall_pillar.py` to `build_<name>.py` and change `NAME`, the comment at the top and the dimensions (metres, Blender space, Z is height).
2. Build the geometry with `common.add_box` or your own lists, with corners counter clockwise seen from outside.
3. Keep the order of the calls: `create_mesh_object`, a UV projection, `assign_textured_material`, `export_obj`.
4. A new texture goes into `build()` in `make_textures.py`, as a colour picture and a normal map from the same pattern.
5. Import the script and call its `build(shots)` in `make_all.py`.
6. Load the file in the code. Models are not discovered: each renderer names its files, for example `WALL_MODEL_FILE` in `src/game/MazeRenderer.cpp`.
7. Run the script with `--shots`, look at the renders, run it twice and check that the second run changes nothing.

## The OBJ and MTL subset

`src/assets/ObjLoader.cpp` is my own loader and reads only what the scripts write.

- OBJ: `v`, `vt`, `vn`, `f`, `usemtl` and `mtllib`. Faces may have three or more corners, which are split into triangles, in the forms `v`, `v/vt`, `v//vn` and `v/vt/vn`, with negative (relative) indices allowed. `o`, `g`, `s` and comments are skipped on purpose, any other keyword is counted and skipped.
- MTL: `newmtl`, `Kd`, `map_Kd` and a normal map line. Blender writes `map_Bump -bm 1.000000 file.png`. The loader also accepts `map_bump`, `bump` and `norm`. The `-bm` strength is ignored, so a normal map always has full strength. Every other line is skipped: no specular, no transparency, no other maps.
- Texture paths are relative to the `.mtl`, which is found next to the `.obj`.
- Tangents are not in the file. They are computed on load (`src/assets/Tangents.cpp`). Mirrored UVs load with a warning.

A model that fails to load is not drawn. A texture that fails to load becomes white, so the part shows its `Kd` colour.

## Textures

The pictures from `make_textures.py` are 512 by 512 PNG, 8 bits per channel, RGB. They tile, except the note, the flask and the shade, which wrap once around their models, and the lantern, whose left half is its pale glass and whose right half its dark iron. The six sky faces are 1024 by 1024 and the heightmap is 256 by 256. Decoding is stb_image, behind `src/assets/ImageLoader.cpp`.

The colour space is decided in code, not in the file. `assets::AssetCache` loads the `map_Kd` picture as sRGB, so the GPU decodes it to linear values when sampling, and the normal map as linear data, because its bytes are directions. The sky and the ground are sRGB too (`src/game/Skybox.cpp`). Normal maps use the OpenGL convention (green is +Y).

## Sounds

`tools/make_sounds.py` computes all twenty-two sounds from sine waves and noise, `gate_bell.wav` being the newest. It uses only the standard library, writes the same bytes every run, and writes into `assets/audio/`. `assets/audio/README.md` is the table of every file and the rules for replacing one with a recording.

```sh
python tools/make_sounds.py
python tools/make_sounds.py --report
```

`--report` reads the files back, prints length, peak, loudness and the strongest frequencies, and checks each sound against its intent (the pulse lies below 120 Hz, for example). `--pictures <folder>` writes a waveform and spectrogram per file.

To add a sound and a cue:

1. Write a function in `make_sounds.py` that builds the samples, add a line to `SOUNDS` and its checks to `checks()`.
2. Run the script and then `--report`.
3. Add a value to the `SoundCue` enum in `src/game/SoundCues.hpp`, raise `SOUND_CUE_COUNT`, and add the matching line to the table in `src/game/SoundCues.cpp`, in the order of the enum. A test (`tests/SoundCueTests.cpp`) fails if a cue has no table line.
4. Add the code that plays the cue and a row to `assets/audio/README.md`. Set `.ambient = true` in the table for a wind.

## Menu video and launcher loop

`tools/record_menu_loop.py` records the video behind the main menu from the game itself and writes `assets/video/menu_loop.mp4` and the still picture `assets/video/menu_still.png`. It starts the Release build in menu camera mode, grabs the window with ffmpeg, crossfades the shots into a loop without a seam and encodes H.264. It needs Windows, ffmpeg, ffprobe and a Release build, and the window must stay open and on top.

`tools/record_launcher_loop.py` does the same for the launcher: one glide over the maze of seed 1, cropped to its 16:10 window and encoded much smaller. It writes into the launcher's repository, checked out next to this one.

Record again whenever the look of the game changes.

## README pictures

`tools/capture_showcase.py` starts the Release build and saves what its window shows into `showcase-out/raw/` (not committed). Then:

```sh
pnpm install
pnpm showcase
```

`pnpm showcase` runs `showcase frame && showcase hero` and writes the framed pictures and the banner into `docs/showcase/`. The kit cannot start a native game, so the capture is my script. With this config only `showcase frame` and `showcase hero` may be run: `showcase capture` and `showcase all` would try to open a web page that does not exist. The pictures live under `docs/`, because everything under `assets/` is packed into the game.

## Fonts

`assets/fonts/` holds Atkinson Hyperlegible Regular (Braille Institute of America) and `OFL.txt`, its SIL Open Font License 1.1, which must stay next to it. The debug window and the menus load it through `TEXT_FONT_FILE` in `src/core/Files.hpp`. `assets/fonts/README.md` has the source and checksum.

## Licence

`LICENSE` is MIT for the source code. It states that the assets are not covered and remain all rights reserved: the models, textures, sounds, video, user interface art and story text under `assets/` and `docs/story/`, and the pictures under `docs/showcase/`. Reusing them needs my permission. A file that states another licence, such as the font, is an exception. Libraries keep their own licences, listed in `THIRD-PARTY-NOTICES.txt`.
