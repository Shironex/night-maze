# miniaudio (sound playback)

Third-party code, but only one small file of it lives here. Contents:

- `miniaudio.c`: hand-written, two lines of code. It defines `MINIAUDIO_IMPLEMENTATION` and
  includes `miniaudio.h`, which makes the header emit the implementation of the library.

The header itself is not in the repository. CMake downloads it at configure time
(`cmake/Dependencies.cmake`, FetchContent) from <https://github.com/mackron/miniaudio>, pinned to
the release tag `0.11.25`. Only the download happens: the `CMakeLists.txt` of miniaudio is
not run, because it defines a target with the name of ours and adds install rules.

The static `miniaudio` target is defined in `cmake/Dependencies.cmake`, next to the download,
because it needs the directory the header was downloaded to. It is built without the strict
warnings of the project, and its header is a system header for the code that includes it.
The same place lists the parts of miniaudio that are left out of the build (the `MA_NO_*`
macros) and what each system links.

Only `src/audio/AudioEngine.cpp` includes the header.

Licence: public domain (Unlicense) or MIT No Attribution, whichever the user prefers. The
text is the file `LICENSE` of the downloaded source and is part of `THIRD-PARTY-NOTICES.txt`.

## Updating

Change the tag in `cmake/Dependencies.cmake`, configure again and generate
`THIRD-PARTY-NOTICES.txt` again (`tools/build-notices.mjs`). Nothing in this
directory has to change.
