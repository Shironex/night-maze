# Generated models: prompts and settings

Prompts and settings for the models I generate in Tripo Studio for Night Maze, and how far each one got. The shade was the first ([decisions/generated-shade-model.md](decisions/generated-shade-model.md)), and every model here follows the same rule: the raw export is committed under `tools/blender/sources/` and a `build_<name>.py` script turns it into the game files with the conventions of [assets.md](assets.md). Nothing here replaces a model until its build script exists and the game has been looked at. By 2026-10-15 every model but `crook_post` is built this way, and the batch is recorded in [decisions/generated-models.md](decisions/generated-models.md).

## Status

- **Not generated**: no roll kept yet.
- **Rerolling**: rolled, not good enough, a new roll is coming.
- **Generated**: a roll is kept and waits in `C:/Users/kacpe/ai3d/tripo_inbox/nightmaze/` with its notes.
- **Built**: the source is committed and its build script writes the game files.

## What the engine needs

The game loads OBJ files with one material each: a colour picture and a normal map, no transparency, no glow map (a glow value is set per model in code). Pictures are 512 or 1024 px (2048 for the arch of the gate). Generated models get smooth normals and keep the UVs of their source, like the shade. Everything a script adds to a model (moving parts, a second piece for the glow) stays a script.

## Settings

| Group | Mode | Topology | Polycount | Texture | T-Pose | Parts |
|---|---|---|---|---|---|---|
| Held (always on screen) | HD, H3.1 | Quad | 8,000 | 2K | off | off |
| Prop | HD, H3.1 | Quad | 3,000 | 2K | off | off |
| Landmark (seen from far) | HD, H3.1 | Quad | 15,000 | 2K | off | off |

Always: Remove Lighting on, PBR on (the build keeps the normal map), Privacy private. The build script reduces the picture to 1024 or 512, or keeps the 2048 of the source for the arch.

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
| lamp_held | Built | Held | An old lamplighter's hand lamp made of dented tinplate: a short cylindrical body with a round glass lens on the front looking straight ahead, a wire bail handle on top, a small chimney cap with vent holes, a brass maker's band around the body, soot near the top. Seen from the side, lens facing forward. | Lies along the beam with the origin on the lens axis at the back of the lamp, turned so the lens looks down the beam ([decisions/held-lamp.md](decisions/held-lamp.md)); 0.17 m tall with the bail; lens and window stay script-built pieces for the glow. |
| gate_arch | Built | Landmark | A small stone gatehouse arch in a drystone wall at the end of a maze: two thick rough stone pillars and a round arch of dressed stones over a 2 m wide passage, moss in the joints, a few stones missing, an iron hook on one pillar for a lantern. No door. | Doorway 1.5 m (not 2 m: the piers would stand where the player walks), 3 m wall height at the sides, no cote; the door (`gate`) stays a separate model that sinks. |
| gate | Built | Prop | The door leaf only: a heavy old oak plank door, 2 m wide and 2.4 m tall, vertical planks held by two broad black iron bands with big rivets, a ring handle, worn grey wood. No frame, no stone, no wall, no arch around it, just the flat door standing on its own. | Origin bottom middle; sinks into the ground in the game; keep one material (oak and iron in one picture). |
| splinter | Built | Prop | A single long sliver of the moon lying on the ground, like a curved shard of pale glowing stone 0.4 m long: one side a rough dark grey rind like old stone, the broken side smooth, pale and luminous white-blue with faint crystal facets. | The glow needs its own piece: the build splits fracture and rind (two halves of one picture, [decisions/moon-splinters.md](decisions/moon-splinters.md)); 0.4 m tall, origin at the lowest point. |
| heartstone | Built | Prop | Four long slivers of the moon fused together at their bases into one jagged crystal cluster 0.9 m tall, standing upright like a broken crown: each sliver has a dark grey stone rind on its outer side and a smooth pale glowing white-blue broken face on its inner side. Sharp, angular, not round, not an egg. | Same split as the splinter; 0.9 m tall, origin in the middle of its base ([decisions/heartstone.md](decisions/heartstone.md)). |
| stone_sheep | Built | Prop | A sheep carved from weathered pale grey stone, lying curled on the ground with its legs tucked in and its head turned slightly, curly wool shown as round carved bumps, moss in the folds, lichen spots. Calm and old, like a village statue. | 0.75 m nose to tail, squeezed to 0.4 m wide (its collision box), 0.51 m high; head along +X; belly 0.1 m into the ground. |
| milestone | Built | Prop | An old weathered stone milestone, about 1 m tall, a rounded top, a carved but unreadable worn face, moss at the base, leaning very slightly. | 0.75 m tall (at 1 m the slab is 0.37 m thick and does not fit between the wall box and the player), 0.28 m thick. |
| stile_post | Built | Prop | A single oak gate post 3 m tall, weathered grey, with one black iron band and an empty iron hook at shoulder height. | Origin of the wall segment it stands in front of; the stile steps stay script-built. |
| gate_lantern | Built | Prop | An old black iron lantern for a gateway, square with a pyramid roof and a ring on top, four small panes, a candle holder inside. Panes as solid frosted glass, not open. | The window stays a script-built piece for the glow, like today. |
| gate_bell | Built | Prop | A small bronze bell hanging from a black iron bracket, a short rope pull, green patina on the bronze. | Origin at the pivot; open underneath, the build adds the inside faces. |
| flask | Built | Prop | A small round tin flask wrapped in wound cord, a cork stopper, a leather strap, dented. | Pickup size, 0.2 m tall; `flask.png` stays, because the ropes of the lever lie on its wound cord. |
| crook_post | Not generated | Prop | A flat weathered oak board, 0.3 m wide and 0.6 m tall and 5 cm thick, the kind nailed to a wall, with two black iron straps across it, one thick iron pin sticking straight out of its middle and a small iron ring hanging below the pin. Only the board, no door, no wall, no frame. | Both rolls became doors, so the script-built post stays. Must fit the lever pick box 0.3 x 0.64 x 0.26 m with the crook; the crook, the ropes and the slab ring stay script-built ([decisions/crook-lever.md](decisions/crook-lever.md)). |
| village | Built | Landmark | A small old hillside village seen from far away at night: a dozen stone cottages with steep slate roofs and chimneys along one lane climbing a ridge, a little church tower, a well, low stone walls. Simple readable silhouettes. | Drawn like the sky around the eye ([decisions/village-on-the-ridge.md](decisions/village-on-the-ridge.md)); the window lights stay script-built (five models of light). |

## Wall pictures

The walls are modular and must tile, so they stay script-built; only their pictures are generated. Tripo's Texture tool does not work for this: on a flat 2 by 2 m panel it painted an object (a frame, one row of stones along the top, blurred fields) instead of a surface (tried 2026-10-15, all three rolls). The pictures come from Tripo's **Image** tab instead (a picture generator, about 2 credits a roll). The picture is kept as a source under `tools/blender/sources/` and `make_textures.py` makes it seamless: it overlaps the last 128 columns, then rows, onto the first, cuts through the overlap along the joints, stretches the result back to 1024, lifts the near-black joints to a dark grey-brown and matches the brightness, then averages it down to 512. The source of `wall_stone` turned out to be two copies of one picture stacked, so the same stones came back every 1.1 m: only one copy is kept, and the other half of the picture is that copy mirrored left to right and moved by half its width, joined along the joints. Every wall picture is then moved and stretched up and down so that a level joint lies at v = 0 and another 0.3 above it: the walls put v = 0 at the top of the plinth, so the edges of the plinth and the coping lie mostly on joints (on the plain wall about 40 % of the row at the plinth and 27 % at the coping is joint, against 12 % on average). The normal maps of `wall_stone` and `wall_mossy` are baked from their slabs (`bake_wall_normals.py`, Cycles, warped onto the picture); `wall_cracked` has no slab, so its normal map comes from the picture (brighter is higher, joints deep). The average reflectance is 0.24 (`WALL_LIGHT`). A picture covers 2 m of wall.

Append this line to every wall prompt:

```
Seamless tileable texture, flat orthographic front view filling the whole square, even flat lighting, no shadows from a light direction, no border, no frame, no perspective, no objects, hand-painted, muted natural colours.
```

| Name | Status | Prompt for the picture |
|---|---|---|
| wall_stone | Built | An old drystone wall of rough grey fieldstones of mixed sizes stacked without mortar, tight joints with dark gaps, a few lighter stones, faint pale lichen spots, no moss. |
| wall_mossy | Built | An old drystone wall of rough grey fieldstones stacked without mortar, thick green moss in the joints and over the upper stones, damp dark patches. |
| wall_cracked | Built | An old drystone wall of rough grey fieldstones stacked without mortar, several stones cracked and split, one or two missing leaving dark holes, pale fresh broken faces. |

## Next: tumbled walls and night dressing

Planned, not built (second of three releases). Some walls get a real break: the top third has fallen out in a V and stones lie at the foot, still too high to climb. Each night also gets props that match its title (frost, shepherds, a children's game, craters, the lamplighter's things). Nothing here is in the game yet; the build scripts come with the release. Append the style line to every prompt.

Wall shapes repeat, so they are Prop and not Landmark. The build cuts the ends flat to the 2 m segment, gives the shape the wall's own UVs and `wall_stone.png`, and decimates it to about 600 triangles (the crowned wall is 414). Dressing props have their origin at the middle of the base, and the box is the footprint, at most 0.45 m deep against a wall.

| Name | Status | Settings | Prompt | Limits for the build |
|---|---|---|---|---|
| wall_tumbled | Not generated | Prop | A straight section of an old drystone wall 2 m long, 3 m tall and 0.4 m thick, built of rough grey fieldstones without mortar, whose upper part has collapsed: a deep V-shaped gap in the top third where stones have fallen out, the stones on both sides of the gap loose and tilted, a few fallen stones lying close along its foot on both faces alike, both ends cut straight and flat. | 2.0 x 3.0 x 0.4 m, origin of a wall segment; the bottom of the gap stays above 2.2 m; fallen stones stay within 0.2 m of the wall face and below 0.25 m, so no box. Both faces alike, because walls along X are never turned. |
| wall_bulged | Not generated | Prop | A straight section of an old drystone wall 2 m long, 3 m tall and 0.4 m thick, rough grey fieldstones without mortar, bulging outwards in its middle as if about to fall, two thick grey oak props leaning against the bulge with their feet in the ground, both ends cut straight and flat. | Same limits. Border walls only, props on the outer face in the land; needs a 180 degree variant of `wallModelMatrix`, or the west and east borders. |
| wall_roots | Not generated | Prop | A straight section of an old drystone wall 2 m long, 3 m tall and 0.4 m thick, rough grey fieldstones without mortar, gripped by thick old stony roots of a hedge that turned to stone: grey roots as thick as an arm growing out of the joints, climbing over the top and down both faces alike, lying flat against the stone, no leaves, both ends cut straight and flat. | Same limits. The hedge that went to stone, as a shape and not as leaves. |
| trough_iced | Not generated | Prop | An old stone water trough cut from one block, 1 m long, thick rim worn smooth, the water inside frozen into a flat white opaque sheet of ice with cracks, frost on the rim, moss at the foot. | Night 1. Box 1.0 x 0.4 x 0.5 m. |
| coping_fall | Not generated | Prop | A low heap of six fallen coping stones from a drystone wall, flat rough grey slabs lying tumbled and tilted against each other, frost and lichen on their upper faces. | Night 1. Box 1.2 x 0.4 x 0.35 m. |
| field_gate_leaning | Not generated | Prop | An old wooden field gate 1.8 m long and 1.1 m tall made of five thick weathered grey oak planks with one diagonal brace, two black iron hinge straps, standing on its own and leaning backwards as if propped against a wall. | Night 2. 1.8 m long, 0.3 m deep, leans on a wall. Five thick planks, so it is not a thin repeating part. |
| shepherd_satchel | Not generated | Prop | A worn shepherd's satchel of thick brown leather and grey wool felt lying slumped on the ground, the flap open, a wooden cup and a heel of bread showing inside, a long strap coiled beside it, iron buckle. | Night 2. 0.4 m, on the ground against a wall, no box. |
| feed_trough | Not generated | Prop | An old wooden sheep feed trough 1.2 m long on four short splayed legs, a V-shaped box of thick grey oak boards with black iron corner straps, a little old hay in the bottom. | Night 2. Box 1.2 x 0.4 x 0.45 m. |
| toy_lamp | Not generated | Prop | A child's toy lamp carved from pale wood, 0.25 m tall, a chunky square body with a painted yellow window on each side, a little peaked roof and a wooden ring on top, the paint faded and chipped. | Night 3. 0.25 m, on the ground, no box. Echoes the held lamp; the windows are painted, not glowing. |
| counting_stool | Not generated | Prop | A small old three-legged milking stool of grey oak with a round seat worn smooth, 0.45 m tall, one leg mended with an iron band. | Night 3. 0.45 m, faces a wall, where the "lamp" child stood. Box 0.35 m. |
| splinter_crater | Not generated | Prop | A shallow round crater 1.4 m across in dark earth and broken stone where a piece of the moon struck the ground: a ring of cracked dark grey stone around the edge, and in the middle a smooth pale glassy white-blue scorch like melted crystal, sharply separate from the dark stone. | Night 4. 1.4 m across, 0.15 m high, flat on the ground, no box. The glassy middle glows, so the build splits it like the splinter ([decisions/moon-splinters.md](decisions/moon-splinters.md)). |
| oil_can_dropped | Not generated | Prop | An old lamplighter's oil can of dented tinplate lying on its side, a round body with a long thin spout and a wire handle on top, a brass maker's band like a railway lamp, a dark stain of spilled oil under the spout. | Night 5. 0.3 m, on its side, no box. |
| lamp_cart | Not generated | Prop | A small old lamplighter's handcart of grey oak boards, two solid wooden disc wheels with iron rims and no spokes, two long handles resting on the ground, an open crate in the cart holding three old iron lanterns with dark panes, a coiled rope. | Night 5. Box 1.3 x 0.7 x 0.9 m; needs a dead end, since it takes 0.7 m. |

Rerolls of a weak model. The first two rolls of `crook_post` both became doors, so this prompt says it lies flat and shows it alone. The door, the heartstone and the milestone are also weak, but I have no new prompt for them yet.

| Name | Status | Settings | Prompt | Limits for the build |
|---|---|---|---|---|
| crook_post | Not generated | Prop | A small flat rectangular board of weathered grey oak lying flat on the ground, seen from above, only 0.3 m wide and 0.6 m long and 5 cm thick, like a plaque, two black iron straps across it, one short thick iron pin standing straight up from its middle, a small iron ring lying beside the pin, nothing behind it and nothing around it. | Same limits as the row above. The build stands it up on the wall. |

New wall picture, from the Image tab (about 2 credits). Append the seamless line from Wall pictures above. A darker picture also means loosening the test that keeps a worn picture within 5 % of the plain wall's brightness (`tests/ImageLoaderTests.cpp`). The frost and damp variants and the ground pictures of the five nights have no prompt yet.

| Name | Status | Prompt for the picture |
|---|---|---|
| wall_dark | Not generated | An old drystone wall of rough dark grey fieldstones stacked without mortar, soot-blackened and damp, deep shadowed joints, a few stones scorched almost black, no moss. |

## Next: gate seals

Planned, not built (third of three releases). The gate asks for one more thing on two nights. On night 2 an iron lock sits on the door and the shepherd's key hangs on a post in a far dead end. On night 4 a stone dial beside the door has three rings of moon phases, and three chalk marks in the maze each show one phase to match. Append the style line to every prompt.

| Name | Status | Settings | Prompt | Limits for the build |
|---|---|---|---|---|
| shepherd_key | Not generated | Prop | A large old black iron gate key 0.3 m long, a round ring bow, a thick round shank, a big square bit with two teeth, a loop of worn leather thong tied through the bow. | Night 2. 0.3 m, floats and turns like a flask. |
| key_post | Not generated | Prop | A short thick weathered oak post 1.5 m tall leaning slightly, a black iron hook near the top, a shepherd's crook-shaped notch carved into its face, the bare foot driven straight into the ground. | Night 2. 1.5 m, the post the key hangs on. Box 0.3 m. |
| gate_lock | Not generated | Prop | A heavy old black iron box lock for an oak door, a flat rectangular iron case 0.35 m wide with big round rivets at the corners, a keyhole plate with a swinging iron cover, a thick iron bolt sticking out of one side, rust in the seams. | Night 2. 0.35 x 0.3 m, mounted on the door leaf and sinks with it; one picture for iron. |
| moon_dial | Not generated | Prop | A square block of dressed grey stone 0.9 m tall with a round dial carved in its front face: three plain flat stone rings one inside the other around a central black iron boss, each ring divided by small grooves into eight smooth empty fields, a raised worn rim, moss in the cracks. | Night 4. 0.9 m block set against a pier of the arch. The body has plain fields; the phase pictures are script strips like the chalk, because Tripo does not draw exact symbols. The build cuts the three rings out as separate models, or they are script rings wearing the picture, so they can turn. |

## Next: the land around the maze

Built on 2026-10-28 with stand-ins from the models already generated (first of three releases, [decisions/land-around-the-maze.md](decisions/land-around-the-maze.md), which lists them). The rows below are still the models to generate; each drops in by changing its line in `build_land_pieces.py`. The land past the border wall becomes a moonlit countryside in two bands of real models: a near band on the terrain margin (1.5 to 14 m past the wall) and a middle band on a ring of ground that rises to a crest 30 to 36 m out, except on the village side, which falls into a valley. The ridge and the village stay drawn with the sky, with a mill and a church tower added on other bearings. Every finished night lights more windows across the land. It takes in the old landmarks plan: the five outer landmarks stand on the crests (three a maze), and the hawthorn moved out of the maze into the near band. Append the style line to every prompt.

The field walls, the sheepfold, the hedgerows, the stream, the ground of the ring and every window light stay script-built. Trees have no leaves: bare limbs are geometry, and the engine has no transparency for leaf cards. Every building is built with dark, deep window openings, because the build finds them by rays and puts the glow quads in them, like the village. A middle band piece is the same roll decimated by the build to a far version (about 1,500 triangles a tree, 3,000 a landmark), so it costs no extra roll. Origin at the middle of the base unless the row says otherwise; nothing here has a collision box, because the player never leaves the maze.

| Name | Status | Settings | Prompt | Limits for the build |
|---|---|---|---|---|
| land_farmhouse | Not generated | Landmark | A small old stone farmhouse seen from far away: one storey and a half of rough grey fieldstone walls without render, a steep roof of heavy stone slabs with a big stone chimney at each gable end, four small deep-set dark window openings on the long front and one on each gable, a low plank door, a lean-to shed of stone against one end, moss on the roof slabs. | Near band, one or two a maze; on the village side it stands low, at the head of the valley. 7 m to the roof ridge, about 10 m long. Window quads by rays from the front and the gables, 0.3 m off the wall like the village. Must stay under the reveal line (5 degrees up from the reveal camera). |
| land_oak | Not generated | Landmark | A broad old oak tree in winter with no leaves, 10 m tall: a short massive trunk with deeply furrowed grey bark that splits at 2.5 m into four thick crooked limbs spreading wide, a rounded crown made only of heavy branches, no thin twigs, thick roots spreading into the ground at the foot. | Near band, and its far version in the middle band. 10 m, crown 11 m wide. Turned and scaled 0.8 to 1.2 by the seed. |
| land_hawthorn | Not generated | Landmark | A lone ancient hawthorn tree with no leaves, 6 m tall: a thick twisted grey trunk that splits at 2 m into three heavy crooked limbs, all bent the same way as if by years of wind, only a few thick branches and no thin twigs, deeply furrowed bark, thick roots going straight down into the ground close around the trunk. | Was `lm_hawthorn`; now outside the maze, so no trunk box. Near band, often on a field line or beside the fold. 6 m. The game turns every hawthorn of a maze to lean the same way. |
| land_copse | Not generated | Landmark | A small copse of six bare winter trees standing close together on a low mound, seen from far away: thick dark trunks and heavy crooked branches merging into one rounded silhouette, no leaves, no thin twigs, a few low bushes of solid dark mass at the foot. | Middle band, 8 m tall and 14 m across, one draw for six trees. Only the far version is used, about 2,500 triangles. |
| lm_chapel_gable | Not generated | Landmark | The single standing gable end of a ruined stone chapel, 8 m tall: one thick wall of rough stone with a pointed top, a tall empty arched window in the middle, a small empty bellcote on the peak with no bell, the side walls broken off low, ivy-free, moss on the ledges. | Crest of the middle band. 8 m tall. The empty bellcote is the silhouette that matters, and the window must show sky. Shown on the map once seen. |
| lm_dovecote | Not generated | Landmark | A ruined round dovecote tower 7 m tall built of drystone fieldstones without mortar, a conical roof of stone slabs with one side fallen in, rows of small square nesting holes high on the wall, a low doorway blocked with stones, moss at the foot. | Crest. 7 m tall. Shown on the map once seen. |
| lm_standing_stone | Not generated | Landmark | A tall rough standing stone 5 m high, one slab of dark grey granite leaning slightly to one side, narrowing towards the top, a large round hole worn right through its upper third, pale lichen patches, set in a low ring of small stones. | Crest. 5 m tall. The hole must show sky from the corridors, so check that it reads. Shown on the map once seen. |
| lm_watch_hut | Not generated | Landmark | A small old shepherd's watch hut on a stone base 2 m high, a timber upper room with one deep dark window opening facing forward, a steep roof of stone slabs with a short chimney, the whole hut leaning slightly, weathered grey oak and black iron straps. | Crest. 5 m tall. Its window is a slot of the land lights, dealt like the farmhouse windows. Shown on the map once seen. |
| lm_cairn | Not generated | Landmark | A tall shepherd's cairn 4 m high, a cone of stacked flat fieldstones without mortar, one long upright stone on its top like a finger pointing at the sky, moss between the lower stones. | Crest. 4 m tall, the lowest landmark, so it stands on the highest point of its crest. Shown on the map once seen. |
| land_cottage | Not generated | Prop | A tiny old one-room stone cottage seen from far away: low rough grey fieldstone walls, a steep stone slab roof, one stone chimney at one end, two small deep dark window openings and a plank door on the front, a low stone wall around a small yard. | Middle band, two to four a maze, mostly in the valley. 5 m to the ridge. Window slots by rays like the farmhouse. |
| land_bridge | Not generated | Prop | A small old packhorse bridge of rough grey stone over a narrow stream, one round arch 3 m wide, low parapets of stacked stones on both sides, the deck humped in the middle, moss on the arch, the ends running down into the banks. | Valley of the village side, where the stream crosses a field line. 6 m long, 2.5 m tall; the stream strip runs under the arch, so the arch stays open. Seen from the reveal and the glide only. |
| land_hay_rick | Not generated | Prop | An old round hay rick in a field, 4 m tall: a fat cylinder of packed pale grey-gold hay with a pointed thatched cap, the thatch held down by a few thick ropes painted on its surface and weighted with stones hanging at the eaves, a ladder of grey oak leaning on one side. | Near band, beside a field wall or the fold. 4 m. The ropes must lie on the thatch as picture, not as loose geometry. |
| ridge_mill | Not generated | Landmark | An old stone tower windmill seen from far away at night: a tapering round tower of rough grey stone with a small wooden cap, four broad sails made of solid flat boards with no lattice, set in an X, one small dark window opening high on the tower, standing alone on a bare hill. | Far band, drawn with the sky like the village, 35 to 50 degrees from the village along the ridge. Built around the eye, nothing past `VILLAGE_REACH` (290 m). If the sails come out as lattice, the build cuts them and adds four flat script boards. Its window lights with night 2. |
| ridge_church | Not generated | Landmark | A small old stone church seen from far away at night: a square west tower with a short pointed spire and battlements, a long low nave with a steep slate roof, two tall dark arched windows in the nave, a few gravestones as simple dark shapes at its foot, on a bare hilltop. | Far band, drawn with the sky, on the other side of the village from the mill. Built around the eye like the village. Its two windows light with night 4. |

Pictures for the script-built land, from the Image tab (about 2 credits a roll). Append the seamless line from Wall pictures above. The ring of ground wears `land_meadow` and the hedgerows wear `hedge`; the field walls and the fold wear `wall_stone`.

| Name | Status | Prompt for the picture |
|---|---|---|
| land_meadow | Not generated | Rough winter pasture seen from straight above, short dark green and grey-brown grass in tufts, a few bare patches of dark earth, small flat stones half buried, fallen dead leaves, a little frost on the grass tips. |
| hedge | Not generated | A dense old hedgerow seen straight from the side, filling the square: tangled dark bare hawthorn and blackthorn branches woven thickly together, a few dark red haws and a few dry brown leaves caught in them, no sky and no gaps showing through. |

## Budget

About 16 models and 3 textures, 50 to 60 credits a roll: roughly 1,000 to 1,500 credits with rerolls.

The three planned releases add 31 models, 3 pictures and the crook_post reroll, at about 55 credits a roll and 3 to 4 rolls a model (Landmark rolls fail more, so 4):

| Line | Models | Rolls each | Rolls | Credits |
|---|---|---|---|---|
| Land: Landmark (farmhouse, oak, hawthorn, copse, five landmarks, mill, church) | 11 | 4 | 44 | 2,420 |
| Land: Prop (cottage, bridge, hay rick) | 3 | 3 | 9 | 495 |
| Tumbled walls | 3 | 4 | 12 | 660 |
| Night dressing | 10 | 3 | 30 | 1,650 |
| Gate seals | 4 | 3 | 12 | 660 |
| Rerolls (door, heartstone, crook_post, milestone) | 4 | 4 | 16 | 880 |
| Pictures (the dark wall, the meadow, the hedge and the night pictures) | about 17 | | | about 35 |
| Total | | | 123 | about 6,800 |

The land alone is 53 rolls, about 2,900 credits. Of 19,800 credits the three releases leave about 13,000. Credits are not the limit, building is, so a model can be generated well before its release and kept in the inbox.
