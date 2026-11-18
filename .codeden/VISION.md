# Vision

Night Maze is a first person maze game in C++20 and OpenGL 4.1: a stone maze at night, one flashlight, crystals to find, and a shadow that hunts by sound and is burned away by the light. It began as a university project and is being grown into a public portfolio game that the owner builds alone to learn graphics programming.

## Goals
- Keep growing the game past the course PRD into a portfolio piece: campaign of five nights, the shade, HUD, gate, storybook intro, music, and next a land around the maze.
- Prototype the cracked moon for the sky (source `tools/blender/sources/moon_tripo.glb`, uncommitted); leaning toward gashes that close one step per won night. Not final, ask before building it.
- Finish the GPU performance work (grass instancing, bloom, maze-cell visibility culling, wall instancing) toward a steadier frame rate without losing quality; measure each step.
- Keep the code plain and commented so the owner can explain it at the defence, and keep the src tree layered (`src/app`, `src/game/<domain>`, `src/render`, `src/hud`, `src/gfx`).
- Confirm the game still launches after the 2026-11 tree refactor (menu, round, shade).

## Target users
- Friends and portfolio visitors who download releases through the separate launcher repo (`Shironex/night-maze-launcher`).
- The owner and their course lecturer, who read the code to judge what the owner learned.
- Contributors reading the public repo, which is MIT licensed.

## Principles
- Workflow: commit small commits straight on `main`, no feature branches. Use a worktree only for genuinely parallel work and bring it back as plain commits. Never push or tag without the owner's go-ahead; a `v*` tag starts a real release.
- Commits are `type(scope): lowercase summary` with a scope. Scopes seen in history: game, render, audio, ui, hud, campaign, settings, keys, shade, walls, village, cmake, tools, debug, readme (check `git log --format=%s -50`). One logical change each.
- No AI attribution, no `Co-Authored-By`, no em or en dashes in code, comments, docs or commits.
- Build system is CMake (presets in `CMakePresets.json`, shortcuts in the `Makefile`). `pnpm` (pinned in `package.json`) only drives the README picture tooling, not the game.
- Same seed and difficulty must always produce the same maze (deterministic generation); do not break it.
- Docs stay lean and in English under `docs/` (README, `docs/building.md`, `docs/architecture.md`, short records in `docs/decisions/`). Polish study notes live only in the owner's Obsidian vault; do not add long Polish docs or "See docs" pointers in code.
- Keep both grass paths: instancing by default, `grass.geom` behind a debug switch (it demonstrates a course topic).
- Agents verify visually only from frames the game writes itself, never from a screen capture.

## Out of scope
- Editing `CHANGELOG.md` or the version in `CMakeLists.txt`; the owner writes both at release time.
- The launcher, which lives in its own repo.
- Rewriting the Polish course material or deleting repo docs before the vault covers them.
- Generic engine features that no night or screen needs.

## Quality bar
- `make check` (format-check, debug tests, release tests, clang-tidy) is the gate before a commit; it takes about 35 minutes. Run targeted checks while working and the full gate once on the final tip.
- Unit tests are doctest cases in `tests/`, listed by hand in each folder's `CMakeLists.txt`; `tests/LayeringTests.cpp` enforces include rules. Logic fixes come with a test that fails without them.
- CI runs the Release build, `ctest`, the notices check and the release feed; clang-tidy is not in CI. The Mac clang-tidy run has about 119 known findings; confirm on Windows before acting.
- When a dependency in `cmake/Dependencies.cmake` changes, regenerate `THIRD-PARTY-NOTICES.txt` with `node tools/build-notices.mjs`.

## Protected areas
- `.github/workflows/**`
- `CMakeLists.txt`
- `cmake/Dependencies.cmake`
- `CHANGELOG.md`
- `THIRD-PARTY-NOTICES.txt`
- `LICENSE`
- `external/**`
- `tools/release-notes.mjs`
- licensing
- release
