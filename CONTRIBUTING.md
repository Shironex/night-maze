# Contributing

Thanks for looking. Night Maze is a learning and portfolio project that I make alone, so I want to
be able to explain every line of it. That shapes what I can accept, and I would rather say it now
than after you spent an evening on a pull request.

## What is welcome

- **Bug reports and crash reports.** Use the [issue forms](https://github.com/Shironex/night-maze/issues/new/choose).
  They ask for what I need to see the same thing: the version, and the maze (night, difficulty and
  seed from the pause menu). For a crash, the log.
- **Ideas and feedback on balance.** Use [Discussions](https://github.com/Shironex/night-maze/discussions),
  in Ideas or Q&A. A new feature starts there, not as a pull request.
- **Code fixes.** Open an issue first so we agree on the fix, then send a pull request. Keep it
  small and plain. I may rewrite it to fit how the rest of the code reads, or decline it. That is
  not a verdict on you or your code.

## What is not open to pull requests

The story, the models, the textures, the sounds, the music, the video and the interface art are
mine. They are not under the MIT licence (see "Assets are not covered" in [LICENSE](LICENSE)). I do
not take pull requests that add, change or replace them, or that edit `docs/story/`.

## Building and checking

[docs/building.md](docs/building.md) has everything: the tools, the Visual Studio developer shell,
the command line switches and the problems I expect. Three things are easy to miss:

- `make check` is the gate I run before a commit. It takes about 35 minutes, most of it clang-tidy,
  and it needs the Visual Studio developer shell. CI runs less (the Release build, `ctest`, the
  notices check and the release feed), and clang-tidy is not in CI.
- The tests are doctest cases in `tests/` for code that needs no window. A fix for such code should
  come with a test that fails without it. The test files are listed by hand in `CMakeLists.txt`, so
  a new file has to be added there.
- The switches reproduce a maze fast. Example, the second night in a fixed maze without the shadow:
  `night_maze.exe --night 2 --seed 7 --calm`.

When a library in `cmake/Dependencies.cmake` changes, run
`node tools/build-notices.mjs --deps build/release/_deps` and commit `THIRD-PARTY-NOTICES.txt`. CI
fails on a difference.

## Why the code looks the way it does

[docs/decisions/](docs/decisions/README.md) holds short records of non-obvious choices, and
[docs/architecture.md](docs/architecture.md) walks through the modules. Read the record before you
change something it explains. If your fix goes against one, say so in the issue.

## Commits and pull requests

Conventional commits, always with a scope and a lowercase summary: `type(scope): summary`. The types
are `feat`, `fix`, `chore`, `docs`, `refactor`, `test`, `ci`, `build` and `perf`. One logical change
per commit. The title of the pull request has the same shape.

Scopes come from the area you touch. The ones in the history are `game`, `render`, `audio`, `ui`,
`hud`, `campaign`, `settings`, `keys`, `shade`, `walls`, `village`, `cmake`, `tools`, `debug` and
`readme`. Run `git log --format=%s -50` and pick the nearest one.

Do not edit `CHANGELOG.md` or the version in `CMakeLists.txt`. I write the changelog and set the
version when I make a release. Branch from `main`, and fill in the pull request template.

## Labels

| Label | What it covers |
|---|---|
| `area:gameplay` | the rules of the round: the shadow, the campaign, pickups, levers, the gate |
| `area:render` | lighting, shadows, fog, bloom, shaders |
| `area:audio` | sounds, music, volumes |
| `area:ui` | menus, HUD, settings, key bindings |
| `area:world` | the maze, models, textures, story text |
| `area:build` | CMake, CI, the release workflow, tools |
| `area:site` | the website under `site/` and its Pages workflow |

`good first issue` and `help wanted` mark the small ones where a pull request is most welcome.

## Security

Do not report a vulnerability in a public issue. See [SECURITY.md](SECURITY.md).
