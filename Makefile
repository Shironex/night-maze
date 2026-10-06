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

# Our own source files: the program in src/ and its unit tests in tests/ (third-party code
# in external/ and build/ is never checked).
# On Windows make may have no Unix shell (started from PowerShell it runs commands directly
# or through cmd.exe, where "find" is a different program), so the files are listed by make
# itself. $(wildcard) does not search subfolders: each pattern is one folder level, and
# src/ is at most two levels deep. A deeper folder needs one more pattern here.
ifeq ($(OS),Windows_NT)
    SOURCES := $(wildcard src/*.cpp src/*/*.cpp src/*/*/*.cpp tests/*.cpp)
    HEADERS := $(wildcard src/*.hpp src/*/*.hpp src/*/*/*.hpp tests/*.hpp)
else
    SOURCES := $(shell find src tests -name '*.cpp')
    HEADERS := $(shell find src tests -name '*.hpp')
endif

# The Objective-C++ files (.mm): the video decoder for macOS is the only one. They are
# formatted like everything else, on both systems: clang-format only reads the text.
# They are NOT given to clang-tidy. clang-tidy compiles what it checks, and an
# Objective-C++ file needs the headers of macOS: on Windows it cannot be checked at
# all, and on macOS the check was never run, because the file was written on Windows.
ifeq ($(OS),Windows_NT)
    OBJCXX_SOURCES := $(wildcard src/*.mm src/*/*.mm src/*/*/*.mm)
else
    OBJCXX_SOURCES := $(shell find src -name '*.mm')
endif

# clang-format and clang-tidy.
# Windows: both come with the Visual Studio C++ tools but are not added to PATH, not even
# in the developer shell. That shell sets VCINSTALLDIR (it ends with a backslash), so the
# full path is built from it. The quotes are needed because the path contains spaces.
# Without the developer shell the tools are expected on PATH.
# macOS: clang-format is on PATH. clang-tidy: use the one on PATH if there is one,
# otherwise the Homebrew LLVM package, which is not added to PATH on macOS.
ifeq ($(OS),Windows_NT)
    ifdef VCINSTALLDIR
        CLANG_FORMAT := "$(VCINSTALLDIR)Tools/Llvm/x64/bin/clang-format.exe"
        CLANG_TIDY   := "$(VCINSTALLDIR)Tools/Llvm/x64/bin/clang-tidy.exe"
    else
        CLANG_FORMAT := clang-format
        CLANG_TIDY   := clang-tidy
    endif
else
    CLANG_FORMAT := clang-format
    CLANG_TIDY   := $(shell command -v clang-tidy 2>/dev/null || echo "$$(brew --prefix llvm 2>/dev/null)/bin/clang-tidy")
endif

# clang-tidy reads the compiler flags from the file compile_commands.json in a build
# directory. On macOS that is build/debug. The Visual Studio generator used on Windows
# does not write this file, so there a second directory is configured with the Ninja
# generator only for clang-tidy.
ifeq ($(OS),Windows_NT)
    TIDY_BUILD_DIR := build/ninja-debug
else
    TIDY_BUILD_DIR := build/debug
endif

# On macOS clang-tidy from Homebrew needs to be told where the system SDK is,
# otherwise it cannot find the standard library headers. uname does not exist on Windows,
# so it is only asked on the other systems.
ifneq ($(OS),Windows_NT)
    ifeq ($(shell uname -s),Darwin)
        TIDY_EXTRA_ARGS := --extra-arg=-isysroot --extra-arg=$(shell xcrun --show-sdk-path)
    endif
endif

# These names are commands, not files. Without this line a file or folder called
# "debug" or "clean" would make the target look already up to date.
.PHONY: help debug release run run-release test test-release format format-check tidy check clean

help:
	@echo "Build and run"
	@echo "  make debug         configure and build the Debug version"
	@echo "  make release       configure and build the Release version"
	@echo "  make run           build and run the Debug version"
	@echo "  make run-release   build and run the Release version"
	@echo "Checks"
	@echo "  make test          build the Debug version and run the unit tests"
	@echo "  make test-release  build the Release version and run the unit tests"
	@echo "  make format        reformat src/ and tests/ with clang-format"
	@echo "  make format-check  fail if src/ or tests/ is not formatted"
	@echo "  make tidy          run clang-tidy on src/ and tests/ (needs the Debug build)"
	@echo "  make check         everything before a commit: format-check, both builds,"
	@echo "                     the unit tests of both builds, tidy"
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

# The unit tests: the default build also builds the night_maze_tests program, ctest runs
# it. -C names the configuration to test. The Visual Studio generator needs it (one build
# directory holds Debug and Release there), single-configuration generators ignore it.
test: debug
	ctest --test-dir build/debug -C Debug --output-on-failure

test-release: release
	ctest --test-dir build/release -C Release --output-on-failure

format:
	$(CLANG_FORMAT) -i $(SOURCES) $(HEADERS) $(OBJCXX_SOURCES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(SOURCES) $(HEADERS) $(OBJCXX_SOURCES)

# clang-tidy needs compile_commands.json (see TIDY_BUILD_DIR above), so the Debug
# configuration has to exist first. On Windows the extra step configures the Ninja
# directory: configuring is enough to write the file, nothing is compiled there.
# The star is in double quotes because cmd.exe does not understand single quotes.
tidy: debug
ifeq ($(OS),Windows_NT)
	cmake --preset debug -G Ninja -B $(TIDY_BUILD_DIR)
endif
	$(CLANG_TIDY) --quiet -p $(TIDY_BUILD_DIR) --warnings-as-errors="*" $(TIDY_EXTRA_ARGS) $(SOURCES)

# test and test-release build both versions first (their prerequisites debug and release).
check: format-check test test-release tidy
	@echo "All checks passed."

clean:
	cmake -E rm -rf build
