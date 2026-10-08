# Docs

This folder holds the English documentation of Night Maze.

- [architecture.md](architecture.md): an overview of the code, module by module, and how a frame is made.
- [building.md](building.md): how I build, run and check the game on Windows, and what `make check` does.
- [assets.md](assets.md): where the models, textures, sounds, video and fonts come from and how the tools in `tools/` make them.
- [decisions/](decisions/README.md): short records of why a non-obvious part of the code looks the way it does. The index groups them by area, and every record names the code it is about.

The other things in this folder are not documentation of the code:

- [story/the-last-lamp.md](story/the-last-lamp.md) is the story design the game follows: the premise, every line of text, and the plan of the five nights. Its sections are kept as first written, so some remarks about what is not built yet are out of date; the decision records say what I chose.
- `PRD.pdf` and `syllabus.md` are course material for the course this game was written for. Both are in Polish and I keep them as they are. The syllabus links to notes that no longer exist in the tree, so those links are dead.
- `showcase/` holds the pictures of the root README. `pnpm showcase` frames them again from the raw captures made by `tools/capture_showcase.py`, and the README has the full steps.

My deep study notes are in Polish, and I keep them outside this repository, in my own notebook. They were written to learn from, so they are long and explain theory next to the code. The repository keeps only the short English set above.

The old Polish notes (a note per module, per library and per decision, and the build guides) are still in the git history. The last commit that held the Polish docs is 78b302f8660710dfa4f114ccdd9a423462e308a1.
