# The repository keeps a short set of English docs
Status: accepted (2026-10-07)
Code: docs/decisions, README.md, docs/README.md

Context: the first docs were long Polish study notes next to the code, and they were heavier to keep current than the code itself.
Decision: the repository holds a short English set: README.md, docs/README.md, architecture, building, assets and these records. The deep study notes live in my Obsidian vault, outside the repository. Code comments do not point to docs.
Why: lean docs stay true, and a pointer in a comment goes stale when the file moves. Keeping all notes in the repository lost because they drifted from the code.
Cost and revisit: a reader of the repository cannot see the study notes. I reopen this if the vault stops being a good home for them.
