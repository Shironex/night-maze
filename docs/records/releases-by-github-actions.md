# A tag starts the release build, and signing needs my approval
Status: accepted (2026-10-07)
Code: .github/workflows/release.yml, .github/launcher-pin.json, tools/release-notes.mjs

Context: friends get the game through a launcher, and a release should come from a clean machine, with the signing key reachable only after my say.
Decision: pushing a v tag builds and packs on runners. A separate job signs with keys held as secrets of an environment that waits for my approval. Verify and publish run last, and the release stays a draft until the end. The launcher lives in its own repository, Shironex/night-maze-launcher, pinned by commit.
Why: local releases kept the key off GitHub but depended on my machine. One gated signing job gives a clean build with a small exposure.
Cost and revisit: the workflow costs Actions minutes while the repository is private. Earlier: releases were built locally, and the launcher lived in this repository. I reopen this if the gated job proves fragile.
