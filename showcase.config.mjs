// Night Maze: the pictures and the banner of the README, framed with @noctcore/showcase-kit.
//
// The kit was made for web pages, Electron windows and terminals, which it can start and
// photograph itself. A native OpenGL game is none of those, so the capture step is not the
// kit's here:
//
//   1. python tools/capture_showcase.py   starts the Release build of the game and writes
//                                         what its window shows to showcase-out/raw/<id>.png
//   2. pnpm showcase                      `showcase frame` and `showcase hero`: they only
//                                         read those PNG files and never start anything
//
// So `showcase capture` and `showcase all` must not be run with this config. `target` is
// only there because the kit asks for one: nothing listens on that address, and neither
// `frame` nor `hero` ever opens it.
//
// Step 2 gives the same files, byte for byte, for the same raw pictures. Step 1 does not:
// the game is not a still picture (crystals turn, the grass moves, a clock runs in the HUD).
//
// Outputs: showcase-out/raw/<id>.png (not committed), docs/showcase/<id>.webp and
// docs/showcase/hero.webp (committed, linked from README.md). They live under docs/ and not
// under assets/, because everything under assets/ is copied next to the game and packed
// into its release.
import { defineConfig } from '@noctcore/showcase-kit';

// Crystal teal into deep night navy: the same background as the pictures of the launcher.
const BACKGROUND = { type: 'gradient', from: '#12444c', to: '#090d1a', angle: 135 };

export default defineConfig({
  name: 'Night Maze',
  slug: 'night-maze',
  target: { mode: 'url', url: 'http://127.0.0.1:9' },
  // The raw pictures are the game window as it is, 1280 x 720, one pixel per pixel.
  deviceScaleFactor: 1,
  langs: ['en'],
  // One entry per picture of the README, in its order, as tools/capture_showcase.py writes it.
  // The script takes a few more (the flask, the open gate, the tea, the view from above,
  // the settings): they stay in showcase-out/raw/ and are not framed.
  shots: [
    {
      id: 'menu',
      title: 'Main menu',
      caption: 'The main menu, over a video loop recorded from the game.',
    },
    {
      id: 'shade',
      title: 'The shadow',
      caption: 'The shadow at the end of a corridor. It stands still while the light is on it.',
    },
    {
      id: 'intro',
      title: 'The intro',
      caption: 'The third of the five cards of the intro, over a walk through the maze.',
    },
    {
      id: 'corridor',
      title: 'A corridor',
      caption: 'A crystal glows in front of the wall at the end of a corridor.',
    },
    {
      id: 'gate-closed',
      title: 'The gate',
      caption: 'The gate, still closed, with a crystal in front of it.',
    },
    {
      id: 'worn-walls',
      title: 'Worn walls',
      caption: 'Stones missing, moss and cracks. The seed decides which walls are worn.',
    },
    {
      id: 'note',
      title: 'A note',
      caption: 'A note on the wall, read with E. This one is a line of the story.',
    },
    {
      id: 'lever',
      title: 'A lever',
      caption: 'A lever, just pulled: the mossy wall beside it sinks and opens a shortcut.',
    },
    {
      id: 'map',
      title: 'The map',
      caption: 'The map while M is held: the corridors seen so far, a lever and a note.',
    },
    {
      id: 'round-end',
      title: 'Through the gate',
      caption: 'The end of a round: the time, the crystals, the difficulty and the seed.',
    },
  ],
  frame: {
    // The game draws no window buttons of its own, and a picture of a game needs no
    // title bar: only the rounded picture on the background.
    style: 'none',
    theme: 'dark',
    background: BACKGROUND,
    padding: 72,
    radius: 14,
    shadow: true,
  },
  hero: {
    layout: 'stack',
    // The line under the title of the main menu.
    tagline: 'The moon sees every corridor. You see one.',
    shots: ['menu', 'shade', 'corridor'],
    background: BACKGROUND,
    theme: 'dark',
    output: 'docs/showcase/hero.webp',
  },
  outputs: {
    raw: 'showcase-out/raw/{id}.png',
    readme: 'docs/showcase/{id}.webp',
  },
});
