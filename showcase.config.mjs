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
  // One entry per picture that tools/capture_showcase.py writes, in the order of the README.
  shots: [
    {
      id: 'menu',
      title: 'Main menu',
      caption: 'The main menu, over a video loop recorded from the game.',
    },
    {
      id: 'corridor',
      title: 'A corridor',
      caption: 'A crystal glows at the end of a corridor, and a flask of tea lies on the way.',
    },
    {
      id: 'cracked-wall',
      title: 'A cracked wall',
      caption: 'A cracked wall behind a crystal. The seed decides which walls are worn.',
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
      caption: 'The map while M is held: the corridors seen so far, two levers and a note.',
    },
    {
      id: 'glide',
      title: 'From above',
      caption: 'The maze from above, the way the camera behind the menu sees it.',
    },
    {
      id: 'settings',
      title: 'Settings',
      caption: 'Settings: mouse sensitivity, field of view, volume, fullscreen and window size.',
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
    shots: ['menu', 'corridor', 'cracked-wall'],
    background: BACKGROUND,
    theme: 'dark',
    output: 'docs/showcase/hero.webp',
  },
  outputs: {
    raw: 'showcase-out/raw/{id}.png',
    readme: 'docs/showcase/{id}.webp',
  },
});
