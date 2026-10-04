# GLAD (OpenGL 4.1 Core loader)

Generated code, do not edit by hand. Contents:

- `include/glad/gl.h`, `include/KHR/khrplatform.h`: the OpenGL API declarations
- `src/gl.c`: the loader that fills in the function pointers at runtime
- `CMakeLists.txt`: hand-written, defines the static `glad` target

Generated with GLAD 2.0.8 for the OpenGL 4.1 Core profile, C language, no extensions.
OpenGL 4.1 is the highest version macOS supports, so nothing from 4.2+ is declared here
and using it by mistake is a compile error.

## Regenerating

Run from the repository root:

```sh
python3 -m venv .glad-venv
.glad-venv/bin/pip install glad2==2.0.8
.glad-venv/bin/glad --api gl:core=4.1 --extensions "" --out-path external/glad c
rm -rf .glad-venv
```

On Windows the executables are in `.glad-venv\Scripts\` instead of `.glad-venv/bin/`.

Without `--extensions ""` GLAD adds every known extension (over 600), which makes the
header about five times larger for no benefit.
