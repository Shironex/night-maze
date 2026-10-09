Draft of 2026-10-07. It began as a proposal to choose from; what I decided since is recorded in
[`decisions/the-shade.md`](../decisions/the-shade.md), [`decisions/story-notes-in-order.md`](../decisions/story-notes-in-order.md),
[`decisions/intro-played-live.md`](../decisions/intro-played-live.md) and [`decisions/campaign-of-five-nights.md`](../decisions/campaign-of-five-nights.md).
The sections below are kept as written, so some of their "not built" remarks and file references are out of date.

# The Last Lamp: story design for Night Maze

Status: the design the game follows. Built from it so far (2026-10-08): all 24 story lines with a saved place, the shadow and the calm night, the caught lines, the intro of five cards, the campaign of five nights with its title cards, night end lines and ending card, the hint wording and the round end title. The premise ("The Last Lamp", quiet folk tale, moon splinters, hedge-stone maze, lamplighter's grandchild) and the enemy rules (moves only when unlit and unseen, caught means back to the start of the same maze, no loss screen, Calm has no enemy) are the owner's decisions and are taken as fixed.

Limits read from the code, which every line below respects:

- A note line is at most 60 characters and printable ASCII only. `tests/InteractablesTests.cpp` (line 1001) checks both, because the HUD font has only those letters and the card is one line. The brief allowed 90; every line here fits in 60, so no card change is needed.
- Note number `i` gets kind `i % 3` (exit hint, crystal hint, flavour). A maze shows at most 5 flavour lines (16 notes is the cap) and by default 1 (3 notes). Which line comes first is chosen by the seed.
- A hint is built as `<subject> <verb> to the <direction>.` or `<subject> <verb> right here.`. The direction is a straight line on the grid and ignores walls.
- The menu camera has two shots: High glide (circle above the maze, flashlight off) and Corridor walk (eye height, flashlight on, start chosen with `--menu-time`). Nothing else: no hands, no crane move, no look down at a puddle.
- Seven sound cues exist: FlashlightOn, FlashlightOff, FlashlightDead, LowBatteryPulse, CrystalPickup, LeverPull, GateOpen. Anything else is a new sound file.
- Maze sizes that are tested: 10, 16 and 22 cells a side. The author of the difficulty table estimates that about 24 is the ceiling (moon shadow texel, far plane of the glide).

Words used only in text, with no model behind them: "chalk". Showing it would need new code and new art. This design never shows it. "The stile" has a model since: a step stile in the north border wall of the start cell, with an empty hook where the lamp hung (`docs/decisions/the-stile.md`). "The village" and "the lamps" have one since too: the village stands on the far rim of the hollow, beyond the gate, where the lane leaves the hollow and goes down, and every night that is won lights a part of its windows and lamps (`docs/decisions/village-on-the-ridge.md`).

---

## 1. The story

### 1.1 One paragraph

Every winter the moon drops pieces of itself into the hollow above the village. Long ago the village grew a hedge around that hollow and the hedge went to stone, and since then one person has walked in each night of the fall, gathered the pieces, and carried them out through the far gate to feed the village lamps. That person was your grandparent, the lamplighter. This winter the lamplighter's hands shake, the village lamps have gone out one by one, and the only lamp still burning is a battered flashlight. It is yours now. The pieces, which the village calls splinters, remember being light, so they keep the flashlight going. Something else comes down after them: the dark gap each piece left in the moon, looking for what it lost. It will not move while your light is on it or while you are watching it. If it reaches you it does not hurt you. It carries you back to where you came in and the night starts over. Five nights, five bigger mazes, and on the last one you light the lamp in your own window.

### 1.2 Ten sentences

1. You are the lamplighter's grandchild, with no name and no voice, and tonight you carry the only lamp the village has left: a flashlight.
2. The maze is a hedge the village planted around the hollow where the moon's pieces fall; it turned to stone generations ago, and it still grows a new way each day, which is why every night is a different maze and why a night you repeat is the same maze.
3. The crystals are those pieces, called splinters: each one remembers being moonlight, so picking one up gives the flashlight back a quarter of its charge, and the flashlight fades because it is only borrowing what they remember.
4. The gate at the far end is the way to the village, and it opens when you carry enough splinters to be worth the walk, never all of them, because the rule of the lamplighters is to leave a few so the moon comes back to look for them next winter.
5. The levers are the gates of the shepherds who grazed the hollow before the hedge: each is roped under the turf to a slab somewhere else, so pulling one makes a distant wall sink and a long way round becomes a short one.
6. The notes are chalk on the stone, left by lamplighters before you and mostly by the one you know; the chalk is ground from a splinter, so it still leans toward the other splinters and toward the gate, in a straight line that takes no notice of walls.
7. The shadow is the hole a fallen piece leaves in the moon, come down to look for it; it is not hunting you, it wants what is in your pockets.
8. Light held in a hand shows it for what it is, a dark place on the ground, and being watched does the same, so it only walks behind your back and outside your beam; the moon overhead does not stop it, because the moon is where it comes from.
9. When it reaches you it carries you back to the stile where you came in, and because the hedge only regrows by day, you wake in the same maze on the same night with the work still to do.
10. The ending is not a victory over the dark: the lamps are lit, a few splinters stay in the hollow on purpose, the shadow keeps those, and you are the lamplighter now.

### 1.3 Every mechanic and its reason

| Mechanic in the game | Reason in the fiction |
|---|---|
| Seeded maze, new seed for a new game | The hedge regrows by day. The seed is that night's growth. |
| "Play again" and R replay the same maze | It is still the same night. The hedge has not regrown. |
| Three sizes (and five in the campaign) | The fall gets heavier as winter deepens, and the hedge grows to hold it. |
| Battery drains only while the light is on | The lamp burns borrowed memory. Off, it keeps it. |
| Flicker and the low battery pulse | The lamp is forgetting. The pulse is your own heartbeat noticing. |
| A crystal restores a quarter, capped at full | One splinter remembers only so much, and the lamp holds only so much. |
| Shorter battery on harder levels | Deeper winter: the lamp forgets faster in the cold. |
| Gate opens at 70 or 80 percent of the crystals | Carry enough for the lamps, leave a few for the moon. Later nights the village needs more. |
| Exit is the cell farthest from the start, always a dead end | The hedge grows its gate as far from the stile as it can. |
| Crystals sit in dead ends first | Splinters roll until something stops them. |
| Levers sink one wall somewhere else | Shepherds' gates, roped under the turf. |
| Notes with a direction to the exit or to the nearest crystal left | Splinter chalk leans toward its own kind. It updates because it is still leaning. |
| Flavour notes | The lamplighters' working advice, written for whoever came next. |
| Minimap fills in as you walk | What you have walked, you remember. |
| Fog | The hollow breathing out the cold. |
| Puddles that mirror the stars | Stars that look like splinters and are not. A small trap for the eye. |
| Painted moon | The thing that lost the pieces, watching all of it. |
| Shadow moves only unlit and unseen (not built) | Sentence 8. |
| Caught returns you to the start of the same maze, no loss screen (not built) | Sentence 9. |
| Calm has no shadow (not built) | A night the moon has not yet noticed what it dropped. Nothing comes looking. |
| Flashlight off (key F) | Saves the lamp and frees the shadow. The one real choice the player makes all night. |

Nothing above needs a mechanic the game lacks, apart from the shadow itself, which is already decided and not yet built.

---

## 2. Two other takes on the same premise

The take in section 1 is called **Inheritance** below. Its emotional centre is work handed down: plain advice from a hand you know.

### 2.1 Warmer: "Lamp's Back"

1. The village children play a game called Lamp's Back: one child faces the wall with a lamp, the others creep up, and anyone caught moving when the lamp turns goes back to the start.
2. Your grandparent taught it to you, and never said where the village learned it.
3. The shadow in the maze plays by exactly those rules, because it is the thing the game was copied from, and it has been waiting all year for someone to play.
4. Being caught is being tagged: no harm, back to the stile, and the notes read like a grown-up coaching a child through a game ("Turn sooner than you think").
5. The ending is the shadow stopping at the gate, at the edge of the lamplight, the way a friend stops at your door.

### 2.2 Eerier: "The One Before"

1. The notes are not from your grandparent but from every lamplighter who went in before, and the handwriting changes from note to note.
2. One winter a lamplighter let the lamp go out in the hollow and did not come back through the gate.
3. The shadow is what stayed of that one: it keeps to the dark because the dark is where it was left, and it stops when seen because it does not want to be recognised.
4. When it catches you it carries you back to the stile because it wants the round finished properly, the round it never finished.
5. The ending is you leaving one splinter at the gate on purpose, for it, and the last note in the game is in a hand you have not seen before.

### 2.3 Recommendation

Build **Inheritance**, and borrow one line from Lamp's Back (note N13 below).

- Inheritance is the only one of the three where every existing mechanic has a reason, including the awkward ones: the gate that opens before you have everything, the hints that point through walls, the moon that lights the whole maze without stopping the shadow.
- Its notes are advice, and advice works when read out of order. The One Before depends on a reveal, and a reveal breaks when note 20 is the first one a player finds. Notes are shuffled by the seed today.
- Lamp's Back makes the enemy a playmate. That is charming for one line and then it removes the only threat the game is about to gain.
- The One Before is the strongest single image, and the most common one: "the monster was a person" is in a great many games. The moon's missing piece looking in your pockets is not.
- Inheritance needs no on-screen character except the shadow and no voice. It is the cheapest of the three to ship.

---

## 3. Five night campaign

Sizes 13 and 19 and their numbers are proposals that would need new rows (new code, see section 5). Sizes 10, 16 and 22 are the existing Easy, Normal and Hard rows. Crystal counts for the new rows follow the slope of the existing table and need the same play tuning the existing rows still need.

"Story notes" is how many lines from the night's pool the player can find. "Hint notes" are exit and crystal hints. With today's `i % 3` rotation a night with N notes shows N / 3 story lines (rounded down), so the mixes below need the "note mix as data" item from section 5. Without it, 15 notes a night shows all 5 story lines at the price of 10 hints, which makes the maze too easy.

| Night | Title | Plot in one sentence | Maze and numbers | What the notes teach | Notes |
|---|---|---|---|---|---|
| 1 | First Frost | The first pieces fall, nothing follows them yet, and you learn the walk. | 10 x 10, 13 crystals, gate at 70 percent, battery 180 s (the Easy row). No shadow. | What a splinter is, why the lamp eats them, that the gate never asks for all, and why you leave a few. | 5 story + 2 hint = 7 |
| 2 | The Shepherds' Gates | The hedge is bigger than you can walk on one lamp, and you find the old gates under the turf. | 13 x 13, 19 crystals, 70 percent, 165 s (new row). Shadow present from here on. | Levers, the chalk that leans, the gate being far, and the first warning about switching the lamp off. | 5 story + 3 hint = 8 |
| 3 | Lamp's Back | Something has been walking behind you, and you learn its rules by being carried home. | 16 x 16, 26 crystals, 70 percent, 150 s (the Normal row). | It walks when you turn or when the lamp is off, light and eyes stop it, the moon does not, and being caught only costs the night. | 5 story + 4 hint = 9 |
| 4 | What the Moon Misses | You learn what it is, and that it was never after you. | 19 x 19, 33 crystals, 80 percent, 135 s (new row). | The hole that follows the piece, the pockets, and the small lies of the hollow: puddle stars, a stuttering lamp. | 5 story + 5 hint = 10 |
| 5 | The Last Lamp | The heaviest fall of the winter, one lamp still dark in the village, and it is the one at home. | 22 x 22, 40 crystals, 80 percent, 120 s (the Hard row). | Why the lamplighter is not here, who the notes were for, and that the gate is the only way home. | 4 story + 6 hint = 10 |

Night 1 has no shadow whatever the Calm decision turns out to be: it is the tutorial, and the first story lines must not describe something the player cannot meet.

---

## 4. The text

All lines are ASCII with straight apostrophes. Lengths were counted by script; every note and caught line is 60 characters or fewer.

### 4.1 Intro: five cards

About 6 seconds a card, 30 seconds in all, any key skips. The four camera shots are the four already found and checked for the menu loop in `tools/record_menu_loop.py` (seed 1, Easy), so no scouting is needed. The glide runs with the flashlight off and the walk with it on, so the cut from card 2 to card 3 is the moment the lamp comes on.

| # | Text (line 1 / line 2) | Camera shot | Sound |
|---|---|---|---|
| 1 | see I1 | None. Text on black. A UI card, not a camera shot. | Wind bed fades in (new file). One far bell (new file). |
| 2 | see I2 | High glide, seed 1, Easy, `--menu-time 14`. | Wind bed continues. |
| 3 | see I3 | Corridor walk, seed 1, Easy, `--menu-time 474` (toward a crystal at the end of a corridor). | FlashlightOn on the cut. CrystalPickup, quiet, as the crystal comes into view. |
| 4 | see I4 | Corridor walk, seed 1, Easy, `--menu-time 72` (long corridor, pillar shadows, then a lever). | Wind drops out. LowBatteryPulse, two beats. |
| 5 | see I5 | Corridor walk, seed 1, Easy, `--menu-time 244` (past the exit gate). | The bell again, once, then silence. |

- I1a: `Every winter the moon drops pieces of itself.`
- I1b: `They fall in the old maze. The village sleeps badly.`
- I2a: `Someone has always walked in and carried them out.`
- I2b: `This winter the lamplighter's hands shake.`
- I3a: `Outsiders call them crystals. We call them splinters.`
- I3b: `They remember being light. The lamp believes them.`
- I4a: `Keep the lamp lit.`
- I4b: `Something walks where it is not.`
- I5a: `One lamp is left in the village.`
- I5b: `It is yours.`

Card 5 holds, then the subtitle `The Last Lamp` under the existing NIGHT MAZE wordmark.

For a game without the shadow (Calm, or before the enemy is built), card 4 becomes:

- I4b-calm: `It forgets quickly.`

### 4.2 Story notes: 24 lines

Numbered in the order a player should find them: the night first, then nearest to the start first within the night. [S] marks a line about the shadow, to be skipped in a game without it. "Kept" means the existing line unchanged. "Revised" names the existing line it replaces.

Night 1, First Frost

- N01: `The moon sees every corridor. You see one.` Kept. It is also the menu tagline in `assets/ui/main_menu.rml`, which stays.
- N02: `A splinter remembers being moon. The lamp believes it.` New.
- N03: `Dead ends are where the light hides.` Kept. It is true: crystals are placed in dead ends first.
- N04: `The gate counts what you carry. It never asks for all.` Revised from "The gate listens for crystals."
- N05: `Leave a few. The moon comes back to look for them.` New.

Night 2, The Shepherds' Gates

- N06: `A lever moves a wall. Somewhere.` Kept.
- N07: `Shepherds roped their gates under the turf. Pull. Listen.` New.
- N08: `Chalk ground from a splinter. It leans. It ignores walls.` New. Explains every hint note.
- N09: `The gate grows as far from the stile as it can.` New. It is true: the exit is the farthest cell.
- N10: `Lamp off saves the lamp. Something else is glad of it.` New. [S]

Night 3, Lamp's Back

- N11: `It walks when you turn. It walks when the lamp sleeps.` New. [S]
- N12: `Shine on it and it is only ground. Look, and it waits.` New. [S]
- N13: `We played this as children. It learned the rules from us.` New. [S]
- N14: `If it reaches you, it only carries you back. Begin again.` New. [S]
- N15: `It does not mind the moon. The moon is where it lives.` New. [S]

Night 4, What the Moon Misses

- N16: `Each piece that falls leaves a hole. The hole comes after.` New. [S]
- N17: `It is not hunting you. It is looking in your pockets.` New. [S]
- N18: `Puddles hold stars. Stars are no use. Walk on.` New.
- N19: `When the lamp stutters, it is forgetting. Feed it.` New.
- N20: `Count your steps. The maze does not.` Kept.

Night 5, The Last Lamp

- N21: `My hands shake now. The lamp does not mind whose hand.` New.
- N22: `I wrote these for whoever came next. I hoped for you.` New.
- N23: `There is no way back. The gate is the way home.` Revised from "Save your battery for the way back." The old line was wrong for this game: the exit is the far end and nobody walks back.
- N24: `One lamp is enough, if it is the one still lit.` New. The title line.

Out of order check: each line names its own subject or uses "it" for the shadow, which reads as a warning on its own. None needs another line to make sense. Without the campaign the whole table works as one pool on any difficulty, as long as the game keeps moving through it (section 5, item 4).

### 4.3 Caught lines

Shown for a few seconds over the play view when the shadow takes you, after the fade back to the start. Not a screen. Each works whether or not crystals and battery reset (that is still open, see section 6). Rotate in order so the same line never shows twice in a row.

- C1: `It carried you back to the stile. Nothing more than that.`
- C2: `The same night. The same hedge. Walk it again.`
- C3: `You turned too late. It was gentle about it.`
- C4: `It set you down where you came in, and went on looking.`
- C5: `Still dark. Still yours to do.`

### 4.4 Hint templates

These fit the existing shape (`subject`, `verb`, then "to the north-east." or "right here."), so only the two string pairs and the one fixed line change. An exit hint never says "right here" (no note hangs in the exit cell). A crystal hint can (a note may share a cell with a crystal).

Exit hint, subject "The gate", verb "waits":

- H1: `The gate waits to the north-east.`

Crystal hint, subject "A splinter", verb "glows":

- H2: `A splinter glows to the south.`
- H3: `A splinter glows right here.`

No crystal left (replaces "No crystal is left to find."). This line shows only when the player took every crystal, which the gate never asked for and the notes advise against, so it says so:

- H4: `You took every one. The moon will look harder.`

Card header (replaces "A note on the wall" in `src/debug/Hud.cpp`):

- H5: `Chalk on the stone`

Would need new code (a second sentence after the direction), only if the owner wants the hints to carry more voice:

- H6: `The gate waits to the north-east. Chalk ignores walls.`

### 4.5 Night end lines and the ending card

The round end card today has the title "You escaped". Proposed title for every night:

- E0: `Through the gate`

One line under the title, by night:

- E1: `The near lamps are lit. The village slept a little.`
- E2: `The lane is lit as far as the well.`
- E3: `It followed you to the gate and stopped at the lamplight.`
- E4: `Lamps to the edge of the village. One window still dark.`

After night 5, the ending card, four lines on black, then back to the menu:

- E5a: `Every lamp in the village has its splinter now.`
- E5b: `The last one hangs in a window you know.`
- E5c: `You left a few behind. The moon will come back for them.`
- E5c-all: `You left none behind. The moon will look harder.`
- E5d: `So will you.`

E5c shows when the player left at least one crystal in the last maze, E5c-all when they took every one (the game allows it). The card already knows both numbers ("32 of 40"). E0 avoids the word "carried" on purpose: the caught lines use it for the shadow.

Sound for the ending: GateOpen (existing) as the card comes up, the wind bed out, the bell once (new file, the same one as the intro).

Without the campaign, E0 and one line chosen by difficulty (E1 for Easy, E3 for Normal, E4 for Hard) work on the existing card, and E5 is held back.

---

## 5. What this needs from the code

Cheapest first. No implementation here, only where it would land.

| # | Change | Size | Files |
|---|---|---|---|
| 1 | Swap the text: the flavour table (6 lines become 24), the hint subject and verb, the "no crystal left" line, the card header, the round end title. All lines already pass the 60 character ASCII test. | small | `src/game/Interactables.cpp` (`FLAVOUR_LINES`, `noteText`), `src/debug/Hud.cpp` (`drawNoteCard`), `assets/ui/round_end.rml`, `tests/InteractablesTests.cpp` (exact strings), `docs/modules/game/interactables.md` |
| 2 | One line under the round end title, chosen by difficulty (later by night, and E5c or E5c-all by whether any crystal was left). | small | `assets/ui/round_end.rml`, `src/game/NightMazeApp.cpp` (`fillRoundEndDocument`) |
| 3 | New sound files and cues: wind bed, bell, a "caught" cue. | small code, plus the audio work | `src/game/SoundCues.hpp` and `.cpp`, `assets/audio/` |
| 4 | Two parts. (a) Inside one maze, the story notes are handed their lines nearest to the start first; `passageDistances` already exists and the levers already use it. (b) Which lines a maze gets stays a choice, not always "from line 1": in the campaign it is the night's pool, and outside the campaign a counter saved in the settings file ("next unread line") moves the window on after each finished maze, wrapping at the end. Without (b) every default maze (3 notes, so 1 story note) would show N01 and nothing else, ever. Until (b) exists, keep today's rule, where the seed picks the first line. The tests pin only that a line index is in range and that lines do not repeat inside a maze (checked in `tests/InteractablesTests.cpp`), not which line a seed shows. | small for (a), small to medium for (b) | `src/game/Interactables.cpp` (`placeInteractables`), `src/game/Settings.cpp` for (b), `tests/InteractablesTests.cpp` |
| 5 | Note mix as data: a count of story notes and a count of hint notes instead of `i % 3`. | small to medium | `src/game/Interactables.hpp` and `.cpp` (`InteractableSettings`, `placeInteractables`), the Maze tab of the debug window, `tests/InteractablesTests.cpp` |
| 6 | Tags on lines: which night a line belongs to, and whether it needs the shadow. Lets Calm skip the [S] lines and lets a night draw from its own pool. | small to medium | `src/game/Interactables.hpp` and `.cpp` (the table becomes a table of structs, `flavourLine` grows), tests |
| 7 | The caught line over the play view. Depends on the shadow existing; the line itself is a timed HUD message. | small, but blocked by the enemy (the largest item, not in this list) | `src/game/Round.hpp` and `.cpp` (which line, how long), `src/debug/Hud.cpp` or a small RmlUi document |
| 8 | A text card screen in RmlUi: two to four lines, fade in and out. Used by the intro cards and by the ending card. | medium | new `assets/ui/card.rml`, `assets/ui/menu.rcss`, `src/ui/UiLayer.*`, `src/game/NightMazeApp.cpp` |
| 9 | An intro state: a script of entries (shot, seed, difficulty, time offset, seconds, two lines, cue) played before the main menu on first launch, any key skips. The camera is already a pure function of maze, settings and seconds. The cost is building the seed 1 Easy maze for the intro and then the real one, and a "seen the intro" flag in the settings file. | medium | `src/game/GameState.hpp` and `.cpp` (`GameMode::Intro`, events), `src/game/NightMazeApp.cpp`, `src/game/Settings.cpp`, `tests/GameStateTests.cpp` |
| 9b | Alternative to 9, cheaper, listed here only because it replaces it: record the intro as a video with the existing script and play it with the existing menu video player, cards drawn by item 8 on top. No intro maze to build. Costs a second binary file that goes stale when the graphics change. | small to medium | `tools/record_menu_loop.py` (a second shot list), `assets/video/`, `src/game/NightMazeApp.cpp` |
| 10 | The campaign: five rows (two new sizes that must pass the bounds tests in `tests/DifficultyTests.cpp`), progress saved, a "Continue: Night 3" entry in the menu, "Next night" on the round end card, the ending card after night 5. | large | `src/game/Difficulty.hpp` and `.cpp` or a new `Campaign.hpp`, `src/game/Settings.cpp`, `src/game/GameState.*`, `src/game/NightMazeApp.cpp`, `assets/ui/main_menu.rml`, `assets/ui/round_end.rml`, tests |

Not needed: a longer note card. Nothing here is over 60 characters.

Not in this list: the shadow itself. It is decided and unbuilt, and it is the largest item of all. Items 1 to 5 and 8 to 9 do not wait for it; until it exists, ship the table without the [S] lines (16 lines remain: N10 to N17 are the eight shadow lines), the Calm variant of intro card 4, and E1 or E4 instead of E3 on the round end card.

Smallest slice that already reads as a story: item 1 alone, which is string edits and their tests.

---

## 6. Open questions for the owner

1. **Do the HUD and the menu keep the word "Crystals"?** Default: yes. The interface says crystals, the people in the story say splinters, and intro card 3 says so out loud ("Outsiders call them crystals"). Renaming the HUD is one string if the owner prefers one word.
2. **When the shadow catches you, do crystals, battery, levers and the minimap reset?** Default: yes, everything, the same as pressing R. It is the code path that already exists (`beginRound`), and the fiction wants it: the thing was after what was in your pockets. The caught lines work either way.
3. **Is Calm a fourth level or a switch?** Default: a switch ("Calm night") that works on any size, so a player who wants no enemy is not locked to one maze size. Night 1 of the campaign is calm regardless.
4. **Does crystal glow stop the shadow, or only the flashlight?** Default: only the flashlight. Moonlight and splinter light are both the moon's and it does not mind them (note N15). This also keeps the rule one sentence long for the player. If the owner wants crystals to be safe spots, N15 needs a second clause and nothing else changes.
5. **Campaign now, or notes and intro on the three existing levels first?** Default: existing levels first (items 1 to 9), with all 24 lines as one pool that the game moves through maze by maze (item 4b), nearest note first inside each maze. Raise the default note count from 3 to 6 so a maze shows two story lines. The campaign is the one large item and the writing does not depend on it.
6. **Is the lamplighter a grandmother, a grandfather, or unstated?** Default: unstated. Every line is first person or says "the lamplighter", and the player fills in the face. Naming one costs nothing later; taking a name back does.
