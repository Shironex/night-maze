# Generated models: prompts and settings

Prompts and settings for the models I generate in Tripo Studio for Night Maze, and how far each one got. The shade was the first ([decisions/generated-shade-model.md](decisions/generated-shade-model.md)), and every model here follows the same rule: the raw export is committed under `tools/blender/sources/` and a `build_<name>.py` script turns it into the game files with the conventions of [assets.md](assets.md). Nothing here replaces a model until its build script exists and the game has been looked at.

## Status

- **Not generated**: no roll kept yet.
- **Rerolling**: rolled, not good enough, a new roll is coming.
- **Generated**: a roll is kept and waits in `C:/Users/kacpe/ai3d/tripo_inbox/nightmaze/` with its notes.
- **Built**: the source is committed and its build script writes the game files.

## What the engine needs

The game loads OBJ files with one material each: a colour picture and a normal map, no transparency, no glow map (a glow value is set per model in code). Pictures are 512 or 1024 px. Generated models get smooth normals, like the shade. Everything a script adds to a model (moving parts, a second piece for the glow) stays a script.

## Settings

| Group | Mode | Topology | Polycount | Texture | T-Pose | Parts |
|---|---|---|---|---|---|---|
| Held (always on screen) | HD, H3.1 | Quad | 8,000 | 2K | off | off |
| Prop | HD, H3.1 | Quad | 3,000 | 2K | off | off |
| Landmark (seen from far) | HD, H3.1 | Quad | 15,000 | 2K | off | off |

Always: Remove Lighting on, PBR on (the build keeps the normal map), Privacy private. The build script reduces the picture to 1024 or 512.

Append this line to every prompt so the set matches. It is not the anime line of Shatter: Night Maze is an old folk tale under the moon.

```
Moonlit folk tale game asset, hand-painted texture, old and weathered, muted natural colours, stone, oak, wool and black iron, no baked lighting, no text or logos, single isolated object.
```

Lessons from the first two batches of Shatter apply here too: say how an object is built ("rope through an iron ring", "lens looking straight ahead"), and leave thin repeating parts (wire, rope coils, spokes, chain) to a script.

## The models

Ordered by how much of the screen they fill. Sizes and limits come from [assets.md](assets.md) and the decision records.

| Name | Status | Settings | Prompt | Limits for the build |
|---|---|---|---|---|
| shade | Built | Prop | (see the decision record) | |
| lamp_held | Generated | Held | An old lamplighter's hand lamp made of dented tinplate: a short cylindrical body with a round glass lens on the front looking straight ahead, a wire bail handle on top, a small chimney cap with vent holes, a brass maker's band around the body, soot near the top. Seen from the side, lens facing forward. | Lies along the beam with the origin in the middle of the butt and the lens down the axis of the cone ([decisions/held-lamp.md](decisions/held-lamp.md)); about 0.25 m long; lens and window stay script-built pieces for the glow. |
| gate_arch | Generated | Landmark | A small stone gatehouse arch in a drystone wall at the end of a maze: two thick rough stone pillars and a round arch of dressed stones over a 2 m wide passage, moss in the joints, a few stones missing, an iron hook on one pillar for a lantern. No door. | Fits the 2 m cell, 3 m wall height at the sides; the door (`gate`) stays a separate model that sinks. |
| gate | Rerolling | Prop | The door leaf only: a heavy old oak plank door, 2 m wide and 2.4 m tall, vertical planks held by two broad black iron bands with big rivets, a ring handle, worn grey wood. No frame, no stone, no wall, no arch around it, just the flat door standing on its own. | Origin bottom middle; sinks into the ground in the game; keep one material (oak and iron in one picture). |
| splinter | Generated | Prop | A single long sliver of the moon lying on the ground, like a curved shard of pale glowing stone 0.4 m long: one side a rough dark grey rind like old stone, the broken side smooth, pale and luminous white-blue with faint crystal facets. | The glow needs its own piece: the build splits fracture and rind (two halves of one picture, [decisions/moon-splinters.md](decisions/moon-splinters.md)); origin at the lower point of the main sliver. |
| heartstone | Rerolling | Prop | Four long slivers of the moon fused together at their bases into one jagged crystal cluster 0.9 m tall, standing upright like a broken crown: each sliver has a dark grey stone rind on its outer side and a smooth pale glowing white-blue broken face on its inner side. Sharp, angular, not round, not an egg. | Same split as the splinter; 0.9 m tall ([decisions/heartstone.md](decisions/heartstone.md)). |
| stone_sheep | Generated | Prop | A sheep carved from weathered pale grey stone, lying curled on the ground with its legs tucked in and its head turned slightly, curly wool shown as round carved bumps, moss in the folds, lichen spots. Calm and old, like a village statue. | 0.9 m nose to tail and at most 0.4 m wide (its collision box); head along +X; legs 0.1 m into the ground. |
| milestone | Generated | Prop | An old weathered stone milestone, about 1 m tall, a rounded top, a carved but unreadable worn face, moss at the base, leaning very slightly. | No readable text. |
| stile_post | Generated | Prop | A single oak gate post 3 m tall, weathered grey, with one black iron band and an empty iron hook at shoulder height. | Origin of the wall segment it stands in front of; the stile steps stay script-built. |
| gate_lantern | Generated | Prop | An old black iron lantern for a gateway, square with a pyramid roof and a ring on top, four small panes, a candle holder inside. Panes as solid frosted glass, not open. | The window stays a script-built piece for the glow, like today. |
| gate_bell | Generated | Prop | A small bronze bell hanging from a black iron bracket, a short rope pull, green patina on the bronze. | Origin at the pivot; open underneath, the build adds the inside faces. |
| flask | Generated | Prop | A small round tin flask wrapped in wound cord, a cork stopper, a leather strap, dented. | Pickup size, about 0.15 m. |
| crook_post | Rerolling | Prop | A flat weathered oak board, 0.3 m wide and 0.6 m tall and 5 cm thick, the kind nailed to a wall, with two black iron straps across it, one thick iron pin sticking straight out of its middle and a small iron ring hanging below the pin. Only the board, no door, no wall, no frame. | Must fit the lever pick box 0.3 x 0.64 x 0.26 m with the crook; the crook, the ropes and the slab ring stay script-built ([decisions/crook-lever.md](decisions/crook-lever.md)). |
| village | Generated | Landmark | A small old hillside village seen from far away at night: a dozen stone cottages with steep slate roofs and chimneys along one lane climbing a ridge, a little church tower, a well, low stone walls. Simple readable silhouettes. | Drawn like the sky around the eye ([decisions/village-on-the-ridge.md](decisions/village-on-the-ridge.md)); the window lights stay script-built (five models of light). |

## Wall pictures

The walls are modular and must tile, so they stay script-built; only their pictures are generated. Tripo's Texture tool does not work for this: on a flat 2 by 2 m panel it painted an object (a frame, one row of stones along the top, blurred fields) instead of a surface (tried 2026-10-15, all three rolls). The pictures come from Tripo's **Image** tab instead (a picture generator, about 2 credits a roll). The build makes the picture seamless by blending its edges, derives the normal map, and keeps it as a source picture for `make_textures.py`. A picture covers 2 m of wall.

Append this line to every wall prompt:

```
Seamless tileable texture, flat orthographic front view filling the whole square, even flat lighting, no shadows from a light direction, no border, no frame, no perspective, no objects, hand-painted, muted natural colours.
```

| Name | Status | Prompt for the picture |
|---|---|---|
| wall_stone | Not generated | An old drystone wall of rough grey fieldstones of mixed sizes stacked without mortar, tight joints with dark gaps, a few lighter stones, faint pale lichen spots, no moss. |
| wall_mossy | Not generated | An old drystone wall of rough grey fieldstones stacked without mortar, thick green moss in the joints and over the upper stones, damp dark patches. |
| wall_cracked | Not generated | An old drystone wall of rough grey fieldstones stacked without mortar, several stones cracked and split, one or two missing leaving dark holes, pale fresh broken faces. |

## Budget

About 16 models and 3 textures, 50 to 60 credits a roll: roughly 1,000 to 1,500 credits with rerolls.
