# stb_image (image file decoder)

Third-party code, but only one small file of it lives here. Contents:

- `stb_image.c`: hand-written, two lines of code. It defines `STB_IMAGE_IMPLEMENTATION` and
  includes `stb_image.h`, which makes the header emit the implementation of the library.

The header itself is not in the repository. CMake downloads it at configure time
(`cmake/Dependencies.cmake`, FetchContent) from <https://github.com/nothings/stb>, pinned to
commit `2c980bb59875b0d32144a71867fbdebb2f77cd20` (stb_image 2.30). The stb repository has no
release tags, so a commit hash is the only way to pin a version.

The static `stb_image` target is defined in `cmake/Dependencies.cmake`, next to the download,
because it needs the directory the header was downloaded to. It is built without the strict
warnings of the project, and its header is a system header for the code that includes it.

Documentation: `docs/libraries/stb_image.md`.

## Updating

Change the commit hash in `cmake/Dependencies.cmake` and configure again. Nothing in this
directory has to change.
