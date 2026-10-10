# Security

This file says what a vulnerability in Night Maze can be, what is not one, and where to send a
report.

## What counts

The game is a local program. It has no network code. It reads its own settings file, the files under `assets/`, and a few command line
switches (`docs/building.md` lists them). That narrows the surface, but it is not empty:

- **A crafted settings or asset file that makes the game write or run something outside its
  folder.** The game writes `night-maze-settings.txt` and `imgui.ini` into its working directory,
  and nowhere else. A file that makes it write elsewhere, or start another program, counts.
- **A release asset that does not match the tagged source.** Releases are built and signed by
  `.github/workflows/release.yml`: the build job has no key, the manifest is signed in a separate
  job that waits for my approval of the `release` environment, the signature is checked before
  publishing, and every action is pinned to a commit SHA. A zip on a release that was not built
  from its tag counts.
- **A way to make that workflow publish or sign something else.** For example through the pinned
  launcher scripts it uses, a tag, or a pull request that reaches the signing job.

## What does not count

- **The Windows SmartScreen warning.** The binaries are not signed with a paid code signing
  certificate. That is a choice, not a hole. The launcher checks a signed manifest instead.
- **A crash from a hand-edited settings file with no further effect.** That is a bug, and a
  public one. Open a normal issue with the crash form.
- **Anything about installing or updating the game.** That is the launcher, in its own
  repository. Its policy is at
  <https://github.com/Shironex/night-maze-launcher/security/policy>.
- **Cheating.** The game is single player and a settings file is yours to edit.

## Reporting

Use GitHub's private vulnerability reporting for this repository:
<https://github.com/Shironex/night-maze/security/advisories/new>. It opens a draft that only I
can see. Please do not put the finding in a public issue or a discussion.

Say which version you used, what file or input triggers it, and what it does. There is no bounty.
I will reply, fix it in a new release, and credit you in the changelog unless you ask me not to.

## Supported versions

Only the latest release. I work on this alone, so fixes go forward and are not backported.
