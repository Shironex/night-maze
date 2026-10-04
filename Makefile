# Shortcuts for the commands used every day. This file does not build anything itself:
# every target calls CMake (through the presets in CMakePresets.json) or a tool.
# See docs/guides/project-structure.md
#
# Usage: make <target>, for example "make run". Plain "make" prints the list of targets.

# Where the built program is. The Visual Studio generator on Windows adds a
# per-configuration subfolder (Debug/ or Release/) and the .exe extension.
ifeq ($(OS),Windows_NT)
    DEBUG_BIN   := build/debug/Debug/night_maze.exe
    RELEASE_BIN := build/release/Release/night_maze.exe
else
    DEBUG_BIN   := build/debug/night_maze
    RELEASE_BIN := build/release/night_maze
endif

# Our own source files (third-party code in external/ and build/ is never checked).
SOURCES := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.hpp')

# clang-tidy: use the one on PATH if there is one, otherwise the Homebrew LLVM package,
# which is not added to PATH on macOS.
CLANG_TIDY := $(shell command -v clang-tidy 2>/dev/null || echo "$$(brew --prefix llvm 2>/dev/null)/bin/clang-tidy")

# On macOS clang-tidy from Homebrew needs to be told where the system SDK is,
# otherwise it cannot find the standard library headers.
ifeq ($(shell uname -s),Darwin)
    TIDY_EXTRA_ARGS := --extra-arg=-isysroot --extra-arg=$(shell xcrun --show-sdk-path)
endif

# These names are commands, not files. Without this line a file or folder called
# "debug" or "clean" would make the target look already up to date.
.PHONY: help debug release run run-release format format-check tidy check clean

help:
	@echo "Build and run"
	@echo "  make debug         configure and build the Debug version"
	@echo "  make release       configure and build the Release version"
	@echo "  make run           build and run the Debug version"
	@echo "  make run-release   build and run the Release version"
	@echo "Checks"
	@echo "  make format        reformat src/ with clang-format"
	@echo "  make format-check  fail if src/ is not formatted"
	@echo "  make tidy          run clang-tidy on src/ (needs the Debug build)"
	@echo "  make check         everything before a commit: format-check, both builds, tidy"
	@echo "Other"
	@echo "  make clean         delete the build/ directory"

# Configuring again is cheap when nothing changed, so both steps always run.
debug:
	cmake --preset debug
	cmake --build --preset debug

release:
	cmake --preset release
	cmake --build --preset release

# Run from the repository root, so imgui.ini is always written to the same place.
run: debug
	./$(DEBUG_BIN)

run-release: release
	./$(RELEASE_BIN)

format:
	clang-format -i $(SOURCES) $(HEADERS)

format-check:
	clang-format --dry-run --Werror $(SOURCES) $(HEADERS)

# clang-tidy reads the compiler flags from build/debug/compile_commands.json,
# so the Debug configuration has to exist first.
tidy: debug
	$(CLANG_TIDY) --quiet -p build/debug --warnings-as-errors='*' $(TIDY_EXTRA_ARGS) $(SOURCES)

check: format-check debug release tidy
	@echo "All checks passed."

clean:
	cmake -E rm -rf build
