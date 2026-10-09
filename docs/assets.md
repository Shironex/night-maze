# Assets

Everything in `assets/` that I made is made by a script. The script is the source and the file in `assets/` is the output. The outputs are committed, so you can build and run the game without Blender. I never edit an output by hand, because the next run would overwrite it.

## Models and textures: tools/blender

The scripts in `tools/blender/` run inside headless Blender. I use Blender 5.2.1, which the first line of every `.obj` names. Another version changes that comment and maybe number formatting.

| File | What it does |
|---|---|
| `blender_common.py` | shared helpers: scene reset, boxes, UV projection, the material, the OBJ export with every option written out, review renders |
| `make_textures.py` | the colour pictures and a normal map for each (name plus `_normal`), into `assets/textures/` |
| the thirteen `build_*.py` files | one model each (`build_splinter.py` and `build_stile.py` write two, `build_chalk.py` three, `build_crook.py` the five of the lever, and `build_wall_straight.py` three: the plain wall, `wall_straight_crown` and `wall_straight_broken`), into `assets/models/`. `build_gate.py` is the door of the exit, `build_gate_arch.py` the gatehouse over it, `build_gate_lantern.py` its lantern, `build_gate_bell.py` the bell that swings beside the big lantern, `build_milestone.py` the stone in front of it, `build_chalk.py` the chalk marks on the walls and `build_stile.py` the stile of the start cell. `build_splinter.py` is what the player collects: the game calls them crystals and draws them as splinters of the moon |
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
| Origin | the middle of the base, the underside at height 0. The exceptions are `splinter_a` and `splinter_b`, whose origin is the lower point of their main sliver: they float and turn about it, and the two models the game turns, `crook_handle` and `gate_bell`, whose origin is the pivot. `stile_post` has the origin of the wall segment it stands in front of, so the game draws it with the matrix of that wall |
| Faces | triangles only, with one normal per face (flat shading). The renderer draws no back faces, so a model that is open, like the bell from below, needs faces that look inwards too |
| Material | exactly one per model, with a colour picture and a normal map. A model may wear the picture of another, or a part of it: the bell has `lever_brass.png`, the post of the stile is cut out of the picture of the gate, and the lever has no picture of its own (see The lever) |
| UVs | one UV unit per 2 m on stone and the gate (`box_project_uvs`). Sloped models use `face_project_uvs` with their own density. The crowned wall, the broken wall and the stile use it with the 2 m of the stone. The flask, the lantern, the splinters, the post of the stile and the models of the lever write their UVs by hand |
| Texture paths in the `.mtl` | relative, so the file works on any computer |

### Chalk marks

A note is chalk drawn on the stone, not an object: `chalk_lamp` for a line of the story, `chalk_arrow` for a hint, and `chalk_crook` beside every lever. `build_chalk.py` holds each mark as a list of lines on the wall. Every piece of a line becomes one flat strip 18 mm wide with square ends, 3 mm in front of the wall face, facing out of the wall only, because the renderer does not draw the back of a face. The origin is the point of the wall face behind the mark, like for the lever. The lamp and the arrow, in each of the four directions the game turns it to, must fit the pick box of a note (0.4 by 0.5 m), and the script stops if they do not. `chalk.png` is a plain pale picture: the loader reads no transparency, so the shape is all geometry. The glow is not in the files. It is `CHALK_GLOW` in `src/game/Interaction.hpp`.

### The stile

`stile` is one wall segment with a notch 0.7 m wide, where the wall is 2.4 m high, and four through-stones that climb to it. The game draws it in place of `wall_straight` on the north border wall of the start cell (`src/game/Stile.hpp`). The stones are boxes, each turned a few degrees by a fixed seed. Every piece of the wall is a closed box where it can be seen, so the notch shows stone and never the inside of the wall. `stile_post` is the oak post beside the steps, with an empty iron hook. It reuses `gate_wood.png` and needs no picture of its own: the post is the middle of one plank, the band at the height of the hook is one of the two iron bands of the gate, with its rivet, and the hook takes its iron from that band. Below the band 3 m of post share the 0.8 m of wood between the two bands, stretched along the grain only. The post stands 1 cm clear of where a wall across the end of the segment would be, because the start cell is a corner, and it reaches 0.15 m into the ground like the milestone. Neither model has transparency or a second material.

### The lever

A lever is a shepherd's crook on a wall, and `build_crook.py` writes its five models. `crook_post` is the oak board with two iron straps, the pin and a ring under it. `crook_handle` is the crook: a tube of six sides swept ring by ring along a shaft and a hook of 250 degrees, with its origin in the pin, 0.05 m in front of the wall. `crook_rope_slack` and `crook_rope_taut` are the rope from the pin through the ring into the turf, and the game draws one of them. `slab_ring` is the iron ring the game hangs at the foot of the wall a lever opens, on both faces.

None of them has a picture of its own. A model has one material, so the oak and the iron of the post both come from `gate_wood.png`: the wood faces lie on one plank between its iron bands, the iron faces on a piece of a band between two rivets. The ropes lie on the wound cord of `flask.png`, and the slab ring wears `lever_iron.png`. `UV_RULES` in the script holds those places in pixels, and the script stops if a face leaves its part or would show its picture mirrored.

The script also stops if the board or the crook, turned up, straight out or down, leaves the pick box of a lever (0.3 by 0.64 by 0.26 m, `LEVER_BOX_*` in `src/game/Interactables.hpp`). The ring under the board hangs 2 cm below the box and the rope runs to the ground: both are left out on purpose. The rope ends 4 cm under the ground 1.2 m below the board, so `MOUNT_HEIGHT` in the script must follow `LEVER_MOUNT_HEIGHT`.

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

The pictures from `make_textures.py` are 512 by 512 PNG, 8 bits per channel, RGB. They tile, except the flask and the shade, which wrap once around their models, the lantern, whose left half is its pale glass and whose right half its dark iron, and the splinter, whose left half is the dark rind of the moon and whose right half the pale fracture that glows. The six sky faces are 1024 by 1024 and the heightmap is 256 by 256. Decoding is stb_image, behind `src/assets/ImageLoader.cpp`.

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
