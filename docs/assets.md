# Assets

Everything in `assets/` that I made is made by a script. The script is the source and the file in `assets/` is the output. The outputs are committed, so you can build and run the game without Blender. I never edit an output by hand, because the next run would overwrite it.

A model that a tool generated is a source file too. It is committed under `tools/blender/sources/` as the tool wrote it, and a script turns it into the game files like any other: the shade, the lamp, the crystals, the sheep, the post of the stile, the flask, the milestone, the village, the door and the gatehouse start from a model Tripo made (see The shade and [decisions/generated-models.md](decisions/generated-models.md)). The walls, the stile, the lever and the chalk stay script-built, and so do the glow pieces of the generated models.

## Models and textures: tools/blender

The scripts in `tools/blender/` run inside headless Blender. I use Blender 5.2.1, which the first line of every `.obj` names. Another version changes that comment and maybe number formatting.

| File | What it does |
|---|---|
| `blender_common.py` | shared helpers: scene reset, boxes, roofs, UV projection, the material, the OBJ export with every option written out, review renders |
| `make_textures.py` | the colour pictures and a normal map for each (name plus `_normal`), into `assets/textures/`: twenty pictures, four walls, ground, `gate_wood`, `lever_iron`, chalk, flask and lantern. The wall pictures are made from the sources below; every generated model has its picture written by its own build script |
| `generated_common.py` | helpers for the build scripts of generated models: `import_source`, `texel_at`, `islands`, `largest_island`, `boundary_loops`, `shrink`, `save_normal_map` (split out of `build_shade.py`) |
| `bake_wall_normals.py` | bakes the normal maps of `wall_stone` and `wall_mossy` from their slabs in Cycles, warped onto the picture. It takes about 50 s, so it writes the committed `wall_*_normal_baked.png` that `make_textures.py` reads; run it only when a source changes |
| `sources/` | what Tripo generated, as committed: `<name>_tripo.glb` for the shade, the lamp, the splinters, the heartstone, the sheep, the stile post, the flask, the milestone, the village, the door, the arch, its lantern and its bell, and the two slabs of the walls, the trees, houses, landmarks and hedge of the land around the maze, plus `wall_stone.png`, `wall_mossy.png` and `wall_cracked.png`. Each `build_<name>.py` reads its own and also writes its picture |
| the sixteen `build_*.py` files | one model each (`build_stile.py` and `build_shade.py` write two, `build_splinter.py`, `build_chalk.py` and `build_lamp_held.py` three, `build_crook.py` the five of the lever, `build_village.py` the village and its five models of light, and `build_wall_straight.py` three: the plain wall, `wall_straight_crown` and `wall_straight_broken`), into `assets/models/`. `build_gate.py` is the door of the exit, `build_gate_arch.py` the gatehouse over it, `build_gate_lantern.py` its lantern, `build_gate_bell.py` the bell that swings beside the big lantern, `build_milestone.py` the stone in front of it, `build_chalk.py` the chalk marks on the walls, `build_stile.py` the stile of the start cell, `build_stone_sheep.py` the stone sheep and `build_lamp_held.py` the lamp the player holds. `build_splinter.py` is what the player collects: the game calls them crystals and draws them as splinters of the moon. Its third model, `heartstone`, is the one big splinter of a maze: a generated crown of slivers, 0.9 m tall, with a picture of its own |
| `build_land_pieces.py`, `build_land_hedge.py` | the generated pieces of the land around the maze (`land_<name>.obj`, a decimated `land_<name>_far.obj` for the middle band, and `land_<name>_windows.txt`, the windows found by rays) and the picture of the hedgerows. Some pieces are stand-ins, listed in [decisions/land-around-the-maze.md](decisions/land-around-the-maze.md) |
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
| Origin | the middle of the base, the underside at height 0. The exceptions are `splinter_a` and `splinter_b`, whose origin is the lowest point of the crescent, and `heartstone`, whose origin is the middle of its base, which float and turn about it, and the two models the game turns, `crook_handle` and `gate_bell`, whose origin is the pivot. `stile_post` has the origin of the wall segment it stands in front of, so the game draws it with the matrix of that wall. `lamp_held` and its two parts lie along the way the player looks, with the origin on the axis of the lens at the back of the lamp (see The held lamp). The origin of `village` and of its lights is the eye that looks at them (see The village) |
| Faces | triangles only. Script-built models have one normal per face (flat shading); generated models are smooth, with a normal per corner, and so is the cloth of the shade. Glow pieces added by a script stay flat. The renderer draws no back faces, so a model that is open, like the bell from below, needs faces that look inwards too |
| Material | exactly one per model, with a colour picture and a normal map. Generated models have a picture of their own. A script-built model may wear the picture of another, or a part of it: the lever has no picture of its own (see The lever), and the village lights wear `lantern.png` |
| UVs | one UV unit per 2 m on stone and the gate (`box_project_uvs`). On the walls, the pillars and the stile v = 0 is the top of the plinth (0.25 m), where every wall picture has a level joint, and the underside of the coping (2.85 m) lands on a second one. Sloped models use `face_project_uvs` with their own density. The crowned wall, the broken wall and the stile use it with the 2 m of the stone. The models of the lever and the glow pieces (the lens and window of the lamp, the panes of the gate lantern, the window lights) write their UVs by hand. Generated models keep the UVs of their source |
| Texture paths in the `.mtl` | relative, so the file works on any computer |

### Chalk marks

A note is chalk drawn on the stone, not an object: `chalk_lamp` for a line of the story, `chalk_arrow` for a hint, and `chalk_crook` beside every lever. `build_chalk.py` holds each mark as a list of lines on the wall. Every piece of a line becomes one flat strip 18 mm wide with square ends, 3 mm in front of the wall face, facing out of the wall only, because the renderer does not draw the back of a face. The origin is the point of the wall face behind the mark, like for the lever. The lamp and the arrow, in each of the four directions the game turns it to, must fit the pick box of a note (0.4 by 0.5 m), and the script stops if they do not. `chalk.png` is a plain pale picture: the loader reads no transparency, so the shape is all geometry. The glow is not in the files. It is `CHALK_GLOW` in `src/game/Interaction.hpp`.

### The stile

`stile` is one wall segment with a notch 0.7 m wide, where the wall is 2.4 m high, and four through-stones that climb to it. The game draws it in place of `wall_straight` on the north border wall of the start cell (`src/game/Stile.hpp`). The stones are boxes, each turned a few degrees by a fixed seed. Every piece of the wall is a closed box where it can be seen, so the notch shows stone and never the inside of the wall. `stile_post` is the oak post beside the steps, a generated model with an iron band and a nub of a hook. It has its own picture, `stile_post.png`, and is squeezed across into the footprint of the old post (x 0.75 to 0.85 m, y -0.37 to -0.27 m, Blender), the wood stretched along the grain and the iron bands keeping their height. It reaches from 0.15 m into the ground to 3.29 m, above the 3.15 m pillars. The post stands 1 cm clear of where a wall across the end of the segment would be, because the start cell is a corner. Neither model has transparency or a second material.

### The stone sheep

`stone_sheep` is one model for the whole flock: a generated lying sheep, curled up, with its head along +X, which is east. `build_stone_sheep.py` scales it to 0.75 m from the nose to the end of the tail and squeezes it across to 0.4 m (the source is 0.54 m wide at that length), so it is 0.51 m high, and puts the origin on the ground under the middle of its length. The game turns the model to one of eight directions (`sheepYawDegrees` in `src/game/StoneSheep.hpp`). The belly reaches 0.1 m into the ground. I squeezed it and did not widen its collision box, because the passage rules of the tests would not hold (see [decisions/generated-models.md](decisions/generated-models.md)). `stone_sheep.png` is its own 1024 picture of pale curled stone.

### The lever

A lever is a shepherd's crook on a wall, and `build_crook.py` writes its five models. `crook_post` is the oak board with two iron straps, the pin and a ring under it. `crook_handle` is the crook: a tube of six sides swept ring by ring along a shaft and a hook of 250 degrees, with its origin in the pin, 0.05 m in front of the wall. `crook_rope_slack` and `crook_rope_taut` are the rope from the pin through the ring into the turf, and the game draws one of them. `slab_ring` is the iron ring the game hangs at the foot of the wall a lever opens, on both faces.

None of them has a picture of its own. A model has one material, so the oak and the iron of the post both come from `gate_wood.png`: the wood faces lie on one plank between its iron bands, the iron faces on a piece of a band between two rivets. The ropes lie on the wound cord of `flask.png`, and the slab ring wears `lever_iron.png`. `UV_RULES` in the script holds those places in pixels, and the script stops if a face leaves its part or would show its picture mirrored.

The script also stops if the board or the crook, turned up, straight out or down, leaves the pick box of a lever (0.3 by 0.64 by 0.26 m, `LEVER_BOX_*` in `src/game/Interactables.hpp`). The ring under the board hangs 2 cm below the box and the rope runs to the ground: both are left out on purpose. The rope ends 4 cm under the ground 1.2 m below the board, so `MOUNT_HEIGHT` in the script must follow `LEVER_MOUNT_HEIGHT`.

### The shade

The shade is the one model not built in its script. `tools/blender/sources/shade_tripo.glb` is a hooded figure in a long robe that Tripo generated: 0.98 m tall, about 16,000 triangles, one material with a royal blue colour picture and a normal map of 2048 by 2048, and a dark mannequin under the robe whose face shows in the hood and whose hands show in the sleeves. `build_shade.py` imports it into headless Blender and writes the same bytes on every run:

1. It joins the corners the file splits at the seams of its picture, so the surface knows its neighbours.
2. It takes out the hands: every face whose texel, at the middle of its UVs, is near black and grey (all channels below 0.08, at most 0.05 apart) and lies below 0.65 of the height. Then it keeps only the largest island, which drops the bits of the hands and about thirty specks of the source.
3. It scales the figure to 2.2 m and lowers it by 0.1 m: the hood ends at 2.1 m and the pool of folds at the hem lies under the ground, like the hem of every model that stands on the uneven ground. The middle of the base is the origin. The figure is 1.08 m wide at that pool and about 0.8 m above it.
4. It makes the colour picture dark blue: the brightest channel of a texel becomes a brightness of 0.05 to 0.39, times (0.18, 0.24, 0.70). Both pictures are averaged down to 1024 by 1024.
5. `shade_hollow` gets two kinds of faces. The night in the hood is a sheet in front of the face of the mannequin, which stays in the cloth, black, behind it. Rays from the front find the outline of the face row by row, 1 cm apart, and the sheet reaches 1 cm beyond it on every side, behind the brim of the hood. It lies 4 mm in front of the tip of the nose and bends back to its sides by 6 times the square of the distance from the middle, so it never stands in front of the brim. The holes the hands left in the sleeves are closed by lids that follow the edge of each hole, once looking each way.

`shade` is 14,557 triangles with smooth normals: the folds are made of many small faces, and one normal per face would show each of them. `shade_hollow` is 1,184 flat triangles, and its faces wear a tiny piece of the black of the mannequin's face, which is what shows wherever the game draws no sky. The game draws them with the reflect program as a piece of the night sky, and the stars get brighter while the beam burns the shade (`shadeHoodStarBoost` and `shadeHoodBrightness` in `src/game/Shade.hpp`). The middle of the night that the stars spread from, `SHADE_HOOD_CENTRE` in `src/game/NightMazeAppPasses.cpp`, is the one the script prints. On 68 small triangles of the cloth the normal map is upside down: 14 of them are turned over in the generated picture, and the others lie at sharp folds where the smooth normal leans over the face. `tests/ObjLoaderTests.cpp` checks that number, the sizes and where the night lies.

### The held lamp

`build_lamp_held.py` writes the flashlight the player holds as three models that fit into each other: `lamp_held` is a generated railway hand lamp, 8,718 triangles (the script decimates the source by half), `lamp_held_lens` the lens, a flat fan of 16 triangles with a radius of 0.027 m, 0.5 mm in front of the glass dome, and `lamp_held_window` a small grid of 24 triangles, 3 by 1.5 cm, on the back of the shoulder of the body, facing back and up at the eye. They are three files because the game gives each a glow of its own and a glow is one value per draw (`heldLampGlow` in `src/game/Lighting.hpp`): the lens follows the beam and the window the battery. The lens and the window are script-built and flat.

The lamp stands upright with the lens in the side of its body, and the script turns it so that the lens looks down Blender +Y, which is -Z in the game. It is 0.17 m tall with the bail. The origin is on the axis of the lens at the back of the lamp, and the game puts that axis on the axis of its beam (`heldLampMatrix`). The lens stands 0.113 m out of the origin (`HELD_LAMP_LENS_OUT` in `src/game/Lighting.hpp`, `LENS_OUT` in the script), and the spill light is at (-0.025, 0.05, -0.115). The model has 12 mirrored triangles, so the loader warns about it.

`lamp_held.png` is one 1024 picture for all three. The lens samples a plain pale patch painted into unused texels of it. The window is laid over a patch of its own, a frame of dark tin around three teal panes that are brightest in the middle.

### The village

`build_village.py` writes the village on the ridge from a generated diorama of a hillside village: the underside, two sides and the back of its block are cut away and about a thousand faces of specks dropped, and the rest is decimated to a quarter, 8,924 triangles. With the 104 triangles of the ridge the model is 9,028 triangles. The model is built around the eye and not around its base, because the game draws it like the sky, centred on the camera (`src/game/Village.hpp`): one source unit is 36 m, the front lies just behind the edge of the ridge at 255 m, and the depth is pulled towards the eye so that nothing is further out than 288 m (`VILLAGE_REACH` is 290 m, and the script stops if a vertex is beyond it). It is 7.7 degrees wide and stands 6.0 to 9.0 degrees up. It is only ever seen from the origin.

The ridge has no ends: it is the rim of the hollow and runs the whole way round the eye, 52 pieces of one slope, still script-built, in the darkest patch of `village.png`. Beside the hill of the village its upper edge lies about 1 degree above the horizon, and its foot lies 200 m out and 200 m below the eye everywhere (`VILLAGE_FOOT_DISTANCE`, `VILLAGE_FOOT_DEPTH`). From the corridors the fog hides the rim. A camera high above the maze sees it as dark land under the sky, and its foot stays in the fog up to an eye 60 m high (`VILLAGE_CLEAR_EYE_HEIGHT`). `tests/VillageTests.cpp` reads the file and checks both.

`village_lights_1` to `village_lights_5` hold what each night of the campaign lights: one quad for every light, 5, 7, 8, 11 and 1 of them, 64 triangles in all. The script finds the places by rays from the eye: 7 on dark windows of the picture and 25 on walls that face the eye, each 0.3 m in front of its surface along the ray, because the depth buffer cannot tell faces apart that lie closer together at that distance. Night 5 is a middle window of the upper right cottage. There are no lamp posts or well any more.

`village` wears its own 1024 picture, `village.png`. The lights wear `lantern.png`: every face gets a small piece from the middle of its pale half or of its dark half (`piece_project_uvs`). The tint and the glow are not in the files. They are `VILLAGE_TINT` in `src/game/VillageRenderer.cpp`, (0.17, 0.22, 0.37), and `VILLAGE_LIGHT_GLOW` in `src/game/Village.hpp`.

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

The pictures from `make_textures.py` are 512 by 512 PNG, 8 bits per channel, RGB, a colour picture and a normal map for each of ten names: four walls, the ground, `gate_wood`, `lever_iron`, chalk, flask and lantern. The wall pictures come from the generated sources in `tools/blender/sources/` (see [generation.md](generation.md)): made seamless (the plain stone, whose source is two copies of one picture, keeps one copy and fills the other half with it mirrored), its joints moved to v = 0 and 0.3, matched in brightness and averaged down to 512, with normal maps baked from the slabs for stone and mossy and derived from the picture for cracked. They tile, except the flask, which wraps once around the ropes of the lever, and the lantern, whose left half is its pale glass and whose right half its dark iron. The pictures of the generated models are written by their own build scripts, from the pictures of the source averaged down from 2048: 1024 by 1024 for the shade, the lamp, the splinters, the heartstone, the sheep, the stile post, the village and the door, 2048 by 2048 (not averaged down) for the arch, 512 by 512 for the flask bottle, the milestone, the gate lantern and the gate bell. The splinter picture has the dark rind in its left half and the pale fracture that glows in its right half. The six sky faces are 1024 by 1024 and the heightmap is 256 by 256. Decoding is stb_image, behind `src/assets/ImageLoader.cpp`.

The colour space is decided in code, not in the file. `assets::AssetCache` loads the `map_Kd` picture as sRGB, so the GPU decodes it to linear values when sampling, and the normal map as linear data, because its bytes are directions. The sky and the ground are sRGB too (`src/game/Skybox.cpp`). Normal maps use the OpenGL convention (green is +Y).

## Sounds

`tools/make_sounds.py` computes all twenty-three sounds from sine waves and noise, `heartstone_pickup.wav` being the newest. The ones that happen in the maze share one computed reverb, a stone room. It uses only the standard library, writes the same bytes every run, and writes into `assets/audio/`. `assets/audio/README.md` is the table of every file and the rules for replacing one with a recording.

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

A sound that another piece of code times itself by has its length written there: the clocks of the hum, the breath and the bell in `src/game/SoundCues.hpp`, and the last card of the intro in `src/game/Intro.cpp`. Check them when a sound gets longer.

## Music

`tools/make_music.py` computes the theme of the main menu, `assets/audio/menu_theme.flac`: a piano, soft strings and a music box, 72.7 seconds that loop. The notes are written in the script. It is the one tool here that needs a package, numpy (I use 2.3), because the piece is 3.2 million samples per channel. It writes the same bytes every run with the same numpy, and it writes the FLAC format itself, so it needs no encoder.

```sh
python -m pip install numpy
python tools/make_music.py
python tools/make_music.py --report
```

`--report` computes the piece again and checks it: the peak, the seam of the loop on both channels, its level against the click of a volume slider, and that the file in `assets/audio/` is that very piece byte for byte. The decoder of the game is checked by `tests/AudioEngineTests.cpp`, which loads the file and listens across the seam.

The script also holds the three sketches the theme was chosen from. `--sketches <folder>` writes them as WAV files, and `--route sampled --tools <folder>` plays them from recorded instruments that are not in the repository (the script lists them). Neither is part of a plain run, and the game uses none of it.

To change the piece, edit its notes in `SKETCHES` and run the script and `--report` again. The level is `THEME_PEAK_DB`. Where the game plays it is `game::menuThemePlays` in `src/game/SoundCues.cpp`.

## Menu video and launcher loop

`tools/record_menu_loop.py` records the video behind the main menu from the game itself and writes `assets/video/menu_loop.mp4` and the still picture `assets/video/menu_still.png`. It starts the Release build in menu camera mode, grabs the window with ffmpeg, crossfades the shots into a loop without a seam and encodes H.264. It needs Windows, ffmpeg, ffprobe and a Release build, and the window must stay open and on top.

`tools/record_launcher_loop.py` does the same for the launcher: one glide over the maze of seed 1, cropped to its 16:10 window and encoded much smaller. It writes into the launcher's repository, checked out next to this one.

Record again whenever the look of the game changes.

## Storybook intro

`assets/intro/` holds the storybook intro: `storybook.txt` and the pictures it names, one painted picture a beat (`beatN.png`, 1376 x 768, generated in Tripo's Image tab and cropped by 4 % on each side to drop the paper border). Parallax layers, the voice lines and the music are added by hand. The file names, formats and sizes are in [story/intro-storyboard.md](story/intro-storyboard.md).

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
