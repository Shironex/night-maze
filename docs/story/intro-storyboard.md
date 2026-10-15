# The intro as a storybook: storyboard

Status: draft of 2026-10-15, for comparing two richer intros with the classic one.
Option A is an illustrated storybook played by the engine (`src/game/Storybook.hpp`, `assets/intro/`). Option B is a 3D cinematic rendered in Blender.
Both tell the same six beats below. They keep the facts and the names of the five cards of the classic intro (`src/game/Intro.cpp`) and of [the-last-lamp.md](the-last-lamp.md).

## Voice direction

One calm female narrator, the voice of an old woman of the village telling a winter tale to a child at the fire. She speaks low and slow, about 120 words a minute, warm but tired, with no drama in the scary lines. Danger is said as plainly as the weather. Leave a breath between the two sentences of a beat, and let the last words of each beat fall rather than rise. "It is yours." is the softest line of the six, almost a gift. No accent is required, but a slight rural, old-fashioned colour suits the tale. Record each beat as its own file, with half a second of room tone before and after.

## Music

One piece for the whole intro, in an Ender Lilies mood: a melancholic solo piano carries the melody, low strings hold the floor, and a soft wordless choir appears only at the moon and at the end. Slow, about 60 to 66 BPM, in a minor key that turns warmer in the last beat. It should be about as long as the intro (about 55 to 65 seconds with the voice) and end with its own fade, because a skip cuts it.

## The beats

The times are targets. With a voice file a beat stays until its line has ended, plus 1.6 seconds (the line begins 0.6 seconds after the picture comes up).

### 1. The moon breaks

- **Narration:** "Every winter, the moon drops pieces of itself."
- **Subtitle:** Every winter the moon drops pieces of itself.
- **Duration:** 7 s.
- **Illustration:** a huge pale moon over a dark winter sky. A thin crack runs across its face, and a few glowing slivers drift down from it like slow sparks. Below, the black line of hills and a stone-hedged hollow, too far to see clearly.
- **Music:** a single piano note, held, then a slow falling figure as the slivers drop. The choir breathes in under the moon.
- **Option B:** this beat is the one to render in Blender: the crack opening and the pieces falling is motion a still painting can only suggest.
- **Layers:** sky with stars (back), the moon with its crack and the falling slivers, the far hills, a foreground of frosted grass and a dry thistle in silhouette.
- **Prompt:**

  > Painted storybook illustration, moonlit winter night. A huge pale full moon fills the upper sky, a thin dark crack across its face, a few small glowing silver-blue splinters falling from it like slow sparks. Below, a low line of dark rolling hills with a faint stone-walled hollow. Frosted grass and a dry thistle in silhouette in the foreground. Muted folk colours: deep indigo, slate blue, bone white, a touch of cold teal in the splinters. Soft gouache and watercolour texture, visible brush strokes, quiet and melancholic, like an old European folk-tale book. Wide 16:9 composition, no text, no letters, no frame.

### 2. The hollow

- **Narration:** "They fall into the old maze above the village. And the village sleeps badly."
- **Subtitle:** They fall in the old maze above the village. / The village sleeps badly.
- **Duration:** 8 s.
- **Illustration:** from high above, a maze of grey stone hedges fills a hollow on a hill. Tiny lights glow in its dead ends where splinters lie. Below the hill, a small village of steep roofs, every window dark but one or two that flicker.
- **Music:** the piano takes the melody, strings enter very low. A small dissonance on "sleeps badly".
- **Layers:** night sky, the hollow with the stone maze and its small lights, the village below the hill, a foreground of a bare tree branch across one corner.
- **Prompt:**

  > Painted storybook illustration, high view on a moonlit winter night. A labyrinth of old grey stone hedges fills a round hollow on a hilltop, small cold glowing splinters lying in its dead ends. Down the slope, a small sleeping village of steep thatched and slate roofs, the windows dark. A bare twisted branch crosses the upper corner. Muted folk colours: indigo, moss grey, slate, bone white, tiny teal lights. Gouache and watercolour, soft edges, quiet, melancholic, old folk-tale book. Wide 16:9, no text, no letters, no frame.

### 3. The lamplighter

- **Narration:** "Someone has always walked in and carried them out. This winter, the lamplighter's hands shake."
- **Subtitle:** Someone has always walked in and carried them out. / This winter the lamplighter's hands shake.
- **Duration:** 9 s.
- **Illustration:** an old lamplighter in a long coat and a knitted hat stands at the stile of the maze, a lantern in one trembling hand. Behind, faint like a memory, a line of earlier lamplighters walks into the hedge, each with a lantern. The old hands are the brightest thing in the picture.
- **Music:** strings carry a slow warm phrase for the line of walkers, then thin out to a lone piano for the shaking hands.
- **Layers:** sky, the ghostly line of past lamplighters along the far hedge, the stone stile and the hedge wall, the old lamplighter in the foreground.
- **Prompt:**

  > Painted storybook illustration, moonlit winter night at the edge of a stone-hedge maze. In the foreground an old lamplighter in a long worn coat and a knitted hat stands by a stone step stile, holding an old tin lantern in a trembling hand, warm light on the wrinkled fingers. Behind, faint and translucent like a memory, a line of earlier lamplighters with lanterns walks into the maze. Muted folk colours: indigo, slate, worn brown wool, bone white, one warm amber lantern glow. Gouache and watercolour, quiet, tender, melancholic, old folk-tale book. Wide 16:9, no text, no letters, no frame.

### 4. Splinters

- **Narration:** "Outsiders call them crystals. We call them splinters. They remember being light, and the lamp believes them."
- **Subtitle:** Outsiders call them crystals. We call them splinters. / They remember being light. The lamp believes them.
- **Duration:** 9 s.
- **Illustration:** close up, a child's cupped hands hold a curved sliver of moon that glows from inside. Next to it a battered lamp with three small panes lights up in answer. Frost on the wool of the mittens.
- **Music:** a high, glassy piano motif (the splinter), answered by the choir for "the lamp believes them".
- **Layers:** the dark maze wall behind, soft out of focus, a faint glow haze, the hands with the splinter and the lamp, a few drifting snowflakes in front.
- **Prompt:**

  > Painted storybook illustration, close-up at night. A child's cupped hands in grey wool mittens hold a curved crescent-shaped splinter of the moon that glows softly from inside, pale silver and teal. Beside it a small battered tin lamp with three little glass panes begins to glow in answer. Behind, an out-of-focus stone wall. A few snowflakes drift in the air. Muted folk colours: indigo, slate, grey wool, bone white, cold teal and a hint of warm amber. Gouache and watercolour, gentle, quiet wonder, old folk-tale book. Wide 16:9, no text, no letters, no frame.

### 5. Something walks

- **Narration:** "Keep the lamp lit. Something walks where it is not."
- **Subtitle:** Keep the lamp lit. / Something walks where it is not.
- **Duration:** 8 s.
- **Illustration:** a narrow corridor of the stone maze. A cone of lamplight from below the frame lights the near walls. Far down the corridor, just past the end of the light, a tall hooded shape of darkness stands still. It has no face, only a deeper black.
- **Music:** the melody stops. A low string drone and a slow heartbeat on the piano's lowest key. No choir.
- **Layers:** the moonlit sky strip above the walls, the far end of the corridor with the hooded shape, the lit near walls, the edge of the lamp beam in the foreground.
- **Prompt:**

  > Painted storybook illustration, a narrow corridor between tall old stone hedge walls at night, seen at eye height. A cone of warm lamplight from the bottom of the frame lights the near stones. Far down the corridor, just beyond the edge of the light, a tall hooded figure made of darkness stands still, faceless, only deeper black. A strip of moonlit sky above the walls. Muted folk colours: indigo, black, moss grey, bone white, one warm amber pool of light. Gouache and watercolour, quiet dread, not gory, old folk-tale book. Wide 16:9, no text, no letters, no frame.

### 6. The last lamp

- **Narration:** "One lamp is left in the village. It is yours."
- **Subtitle:** One lamp is left in the village. / It is yours.
- **Duration:** 9 s.
- **Illustration:** the village at night from the lane, every street lamp dark. In the foreground, seen from behind, the child holds up the battered lamp, the only warm light in the picture. Far up the hill, the maze waits under the cracked moon.
- **Music:** the piano melody returns, now in a warmer key, the choir enters softly and the piece fades with the picture.
- **Layers:** sky and the cracked moon, the hill with the maze, the dark village street, the child with the lamp in the foreground.
- **Prompt:**

  > Painted storybook illustration, a small village street on a winter night, all the street lamps dark. In the foreground, seen from behind, a small child in a wool coat and hood holds up an old battered tin lamp, the only warm light in the scene. Up the hill behind the roofs, a stone-hedge maze waits under a pale moon with a thin crack. Muted folk colours: indigo, slate, bone white, grey wool, one warm amber lamp glow. Gouache and watercolour, hopeful but melancholic, old folk-tale book. Wide 16:9, no text, no letters, no frame.

## Making the layers

Generate the whole scene first and keep it as the reference. Then make the layers in one of two ways.

- **Cut them:** load the full picture in an editor, select each depth band (sky, far, middle, near) and put each on its own layer. Paint the sky behind the cut-out parts so the back layer has no holes, because the layers drift apart.
- **Generate them:** ask for the same scene with only one band, on a plain flat green background ("same scene, only the foreground silhouette, on a flat green background"), and key the green out. Keep the sky layer as the full scene without the near bands.

Two to four layers per beat are enough. The back layer must be fully opaque. The front layers should leave a little extra picture past their visible edges, because they are shown enlarged and moved.

## Dropping in the files (option A)

Everything goes into `assets/intro/`, next to `storybook.txt`. The manifest names every file, so any names work. Today each beat is one painted picture, `beatN.png`, picked on 2026-10-15 from four rolls of the prompt above (Nano Banana 2, 16:9) and cropped by 4 % on each side to drop the paper border. Splitting a beat into layers means adding `layer` lines for the new files and keeping `beatN.png` as the opaque back layer or replacing it.

| What | Format | Notes |
|---|---|---|
| Layer | PNG, RGB or RGBA, 1920 x 1080 (any 16:9 size works) | back layer opaque, the others with alpha. Each layer takes about 8 MB of video memory while the intro plays |
| Voice line | WAV or FLAC, mono, 44.1 or 48 kHz, peaks near -3 dBFS | one file per beat, `voice beatN_voice.wav` under the beat. It plays in the effects group, so the effects slider sets its volume |
| Music | FLAC (or WAV), stereo, 44.1 or 48 kHz | one line `music intro_theme.flac` at the top of the manifest. It plays in the music group and is cut when the intro is skipped |

A layer line is `layer <file> <drift x> <drift y> <scale from> <scale to>`. Drift is how far the layer travels over the beat, as a share of the window (0.03 is three percent, positive is right and down). Scale is the zoom at the start and at the end, from 1 (all of the picture) up. Near layers should drift and zoom more than far ones. A drift larger than the zoom leaves room for stops at the picture's edge. `seconds` is the shortest time of a beat, `crossfade` the fade between beats (1.5 s). The words are listed in `src/game/Storybook.hpp`.

To compare the two intros, start the game with `--intro` (the storybook) or with `--intro --intro-style classic`. A new campaign plays the storybook too, unless `--intro-style classic` is given. A manifest that cannot be read, or a layer that cannot be loaded, falls back to the classic intro and says why in the log.
