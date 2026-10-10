<!-- Keep this short. CONTRIBUTING.md has the long version. -->

## What

<!-- One or two sentences, and the issue this fixes ("Fixes #12"). I ask for an issue first. -->

## Checklist

- [ ] The issue was agreed before I wrote the fix, and this pull request is small: one fix, one logical change per commit.
- [ ] Commits and the title look like `type(scope): lowercase summary`, for example `fix(game): ...`.
- [ ] `make check` is green in the Visual Studio developer shell (about 35 minutes), or I say below why I could not run it. CI runs a smaller set: the Release build, `ctest`, the notices check and the release feed.
- [ ] A fix for code that needs no window comes with a doctest case in `tests/` (a new file is also listed in `CMakeLists.txt`).
- [ ] If I changed a dependency in `cmake/Dependencies.cmake`: I ran `node tools/build-notices.mjs --deps build/release/_deps` and committed `THIRD-PARTY-NOTICES.txt`.
- [ ] I did not edit `CHANGELOG.md` or the version in `CMakeLists.txt`. I write those at release time.
- [ ] I did not change anything under `assets/` or `docs/story/`. Those are not open to pull requests.
