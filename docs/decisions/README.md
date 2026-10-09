# Decision records

A record explains why a non-obvious part of the code looks the way it does. Each one is about ten lines: a title that is a sentence, `Status:`, `Code:` (paths only), then Context, Decision, Why (with the one rejected alternative) and Cost and revisit. A record is short on purpose. Theory and code walk-throughs live elsewhere.

Status words: `accepted (date)` means the code does this today. `superseded by <record>` means the decision is gone and the named record replaces it. An `Earlier:` clause names an older form of a decision that still stands.

## Rendering
- [rendering-in-the-game-layer](rendering-in-the-game-layer.md): drawing classes live in src/game
- [linear-colour-pipeline](linear-colour-pipeline.md): decode on read, encode once at the end
- [aces-tone-mapping](aces-tone-mapping.md): ACES default, two switches
- [bloom-passes](bloom-passes.md): half resolution, three targets
- [bloom-input](bloom-input.md): bloom from the unfogged scene
- [fog](fog.md): fog from depth, height at the pixel
- [vignette](vignette.md): measured in texture coordinates
- [moon-shadow-map](moon-shadow-map.md): whole terrain, bias in metres
- [what-the-moon-shadow-covers](what-the-moon-shadow-covers.md): moon light only, per fragment
- [flashlight-in-hand](flashlight-in-hand.md): held lamp aimed ahead of the eye
- [flashlight-shadow-bias](flashlight-shadow-bias.md): bias in world space
- [grass-lighting](grass-lighting.md): fixed up normal
- [tangents-on-load](tangents-on-load.md): computed once per OBJ
- [reflections-and-puddles](reflections-and-puddles.md): own pass, puddles follow ground
- [visible-effect-over-physical-values](visible-effect-over-physical-values.md): defaults chosen to be seen
- [terrain-sampling](terrain-sampling.md): heightmap tiled in world metres
- [painted-moon](painted-moon.md): moon painted into the sky
- [darker-night-defaults](darker-night-defaults.md): darker values and the fog cap
- [worn-walls-from-the-seed](worn-walls-from-the-seed.md): wall variants, painted niches
- [crowned-and-broken-walls](crowned-and-broken-walls.md): two wall models, new top edge

## World and gameplay
- [collision-aabb-sliding](collision-aabb-sliding.md): own boxes, one axis at a time
- [deterministic-random](deterministic-random.md): mt19937 and own range helper
- [maze-on-terrain](maze-on-terrain.md): walls sunk onto gentle ground
- [the-stile](the-stile.md): step stile in the north wall of the start cell, one box
- [round-state](round-state.md): the round owns a maze copy
- [picking](picking.md): ray from the eye, pulsing glow
- [crook-lever](crook-lever.md): crook on a board, rope, ring on the wall it opens
- [exit-cell](exit-cell.md): farthest cell by passages
- [crystals-and-gate](crystals-and-gate.md): crystal count, gate share, darkness
- [crystal-lights](crystal-lights.md): nearest crystals carry the lights
- [moon-splinters](moon-splinters.md): crystals drawn as splinters, dark rind in one picture
- [sprint-stamina-and-flasks](sprint-stamina-and-flasks.md): stamina and tea flasks

## Enemy and story
- [the-shade](the-shade.md): wanders, hunts by sound and sight, burned away by the lamp
- [the-shade-hood](the-shade-hood.md): night sky in the hood, brighter as it burns, empty sleeves, torn hem
- [calm-night](calm-night.md): a switch without the shade
- [campaign-of-five-nights](campaign-of-five-nights.md): own table of nights
- [village-on-the-ridge](village-on-the-ridge.md): drawn with the sky, real depth, lit night by night
- [tonights-hedge](tonights-hedge.md): a maze a day, the date is the seed
- [story-notes-in-order](story-notes-in-order.md): lines read nearest first
- [lamplighters-ledger](lamplighters-ledger.md): the lines whose note was opened, as codes in one settings line
- [chalk-marks](chalk-marks.md): notes drawn as chalk, the arrow leans along the wall
- [intro-played-live](intro-played-live.md): five cards in the engine

## Interface
- [debug-window](debug-window.md): one window, HUD at the top
- [menu-in-rmlui](menu-in-rmlui.md): menu in RmlUi
- [menu-flow](menu-flow.md): Escape pauses and goes back
- [menu-background-video](menu-background-video.md): recorded loop behind the menu
- [minimap-fog-of-war](minimap-fog-of-war.md): only seen corridors
- [minimap-rendering](minimap-rendering.md): plain sRGB, rebuilt each frame
- [map-held-with-m](map-held-with-m.md): held map, player stands still
- [key-bindings](key-bindings.md): action table, own key names

## Audio
- [audio](audio.md): miniaudio and generated sounds
- [audio-loops-and-groups](audio-loops-and-groups.md): loops, footsteps, volume groups

## Build and release
- [releases-by-github-actions](releases-by-github-actions.md): tag build, approved signing
- [licence](licence.md): MIT code, assets excluded
- [docs-lean-english](docs-lean-english.md): short English docs only
