"""Moves modules to another folder: git mv, the include lines, the mentions and the CMake lists.

Run from anywhere, with Python 3.9 or newer, on macOS or Windows:

    python3 tools/move_module.py [--dry-run] SOURCE [SOURCE ...] FOLDER

SOURCE is a module's stem (src/game/lighting/Bloom moves Bloom.hpp and Bloom.cpp, whichever
exist) or one file (src/app/main.cpp, tests/game/lighting/BloomTests.cpp). FOLDER is where
they go (src/render).
Paths are relative to the repository root, with / or \\. A module and its test go to two
folders, so they are two calls:

    python3 tools/move_module.py --dry-run src/game/lighting/Bloom src/render
    python3 tools/move_module.py src/game/lighting/Bloom src/render
    python3 tools/move_module.py tests/game/lighting/BloomTests.cpp tests/render

For every file it:
  1. moves it with git mv, so git log --follow keeps its history;
  2. rewrites every mention of its old path in src/, tests/ and tools/ to the new one: the
     #include lines (each include block it changed sorted again, as clang-format sorts it)
     and the comments that name it ("game/lighting/Bloom.hpp", "src/game/lighting/Bloom.cpp",
     "game/lighting/Bloom", "tests/game/lighting/BloomTests.cpp"), then runs clang-format on
     the C++ files it edited when it finds it, so a comment that grew past the line limit is
     wrapped again (else run make format);
  3. moves its line from the CMakeLists.txt of its old folder into the block of the same
     target in the CMakeLists.txt of the new folder, in sorted order. A file listed for two
     targets (src/debug/Search.cpp is in night_maze and night_maze_tests) moves in both. A
     folder without a list gets one, and the list of the folder above it an
     add_subdirectory line.

Only the plain blocks are patched: a line "target_sources(<target> PRIVATE" at the start of a
line, one file per line, and ")" alone. A file listed inside an if() block (the macOS video
decoder) is moved by hand.

At the end it prints the files outside src/, tests/ and tools/ that still name an old path
(docs/, .github/, the root files), for a person to update; it never edits those.

--dry-run prints the moves and the changes as a diff and touches nothing. Nothing is ever
deleted: a target_sources block left empty is taken out of its list, a list or a folder left
empty stays for a person to remove.
"""

from __future__ import annotations

import argparse
import difflib
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parent.parent
LIST = "CMakeLists.txt"
# The order the targets' blocks take in a new or growing list.
TARGETS = ["engine", "ui", "video", "audio", "game_logic", "night_maze", "night_maze_tests"]
# Where the mentions of a moved file are rewritten, and which files there are read.
SCANNED = ["src", "tests", "tools"]
SOURCE_SUFFIXES = {".cpp", ".hpp", ".h", ".mm"}
TEXT_SUFFIXES = SOURCE_SUFFIXES | {".py", ".mjs", ".js", ".md", ".txt"}
# Folders under SCANNED that are not ours to edit: generated models and Python caches.
SKIPPED_PARTS = {"__pycache__", "sources", "node_modules"}
# Where mentions are only reported. .claude/ (the worktrees of agents) is never read.
REPORTED = ["docs", ".github", "README.md", "CONTRIBUTING.md", "Makefile", "package.json",
            "CMakeLists.txt", "cmake", "assets"]
REPORT_SKIPPED = {"docs/decisions", "docs/syllabus.md", "docs/PRD.pdf"}
BLOCK_START = re.compile(r"^target_sources\((\S+) PRIVATE\s*$")
SUBDIRECTORY = re.compile(r"^add_subdirectory\((\S+)\)\s*$")
INCLUDE = re.compile(r'^\s*#\s*include\s*([<"][^>"]+[>"])')


class MoveError(Exception):
    pass


def posix(path: str) -> PurePosixPath:
    """A path from the command line as a POSIX path relative to the root."""
    text = path.replace("\\", "/").strip("/")
    pure = PurePosixPath(text)
    if pure.is_absolute() or ".." in pure.parts:
        raise MoveError(f"{path}: give a path inside the repository, relative to its root")
    return pure


def read(path: PurePosixPath) -> str:
    # newline="" keeps the line ends as they are, so nothing changes but the edited lines.
    with open(ROOT / path, encoding="utf-8", newline="") as file:
        return file.read()


def write(path: PurePosixPath, text: str) -> None:
    with open(ROOT / path, "w", encoding="utf-8", newline="") as file:
        file.write(text)


def exists(path: PurePosixPath) -> bool:
    return (ROOT / path).exists()


def files_of(source: PurePosixPath) -> list[PurePosixPath]:
    """The files a SOURCE names: the file itself, or the .hpp and .cpp of a stem."""
    if source.suffix in SOURCE_SUFFIXES and exists(source):
        return [source]
    pair = [source.with_name(source.name + suffix) for suffix in (".hpp", ".cpp")]
    found = [path for path in pair if exists(path)]
    if not found:
        raise MoveError(f"{source}: no such file, and no {source.name}.hpp or .cpp")
    return found


def mention_key(path: PurePosixPath) -> str | None:
    """The text other files use to name path, without its extension: relative to src/ for
    the program's sources (the include root), else relative to the root. None for a file
    straight in src/: its bare name is too common a word to rewrite."""
    stem = path.with_suffix("")
    if stem.parts[0] == "src":
        inside = PurePosixPath(*stem.parts[1:])
        return str(inside) if len(inside.parts) > 1 else None
    return str(stem)


def mention_patterns(renames: dict[str, str]) -> list[tuple[re.Pattern[str], str]]:
    """Each old key as a pattern that matches it only as a whole path ("game/Maze" in
    "game/Maze.hpp" and "src/game/Maze.cpp", not in "game/MazeWorld.hpp")."""
    return [
        (re.compile(r"(?<![A-Za-z0-9_])" + re.escape(old) + r"(?![A-Za-z0-9_])"), new)
        for old, new in renames.items()
    ]


def rewrite_mentions(
    text: str, patterns: list[tuple[re.Pattern[str], str]]
) -> tuple[str, set[int]]:
    """text with every old key replaced by its new one, and the indices of the lines that
    changed."""
    lines = text.split("\n")
    changed: set[int] = set()
    for index, line in enumerate(lines):
        new = line
        for pattern, replacement in patterns:
            new = pattern.sub(replacement, new)
        if new != line:
            lines[index] = new
            changed.add(index)
    return "\n".join(lines), changed


def include_priority(include: str, main_stem: str | None) -> int:
    """clang-format's category for an include under the LLVM style: the main header first,
    then our quoted headers, then the angle ones."""
    name = include[1:-1]
    if main_stem is not None and PurePosixPath(name).stem == main_stem:
        return 0
    if re.match(r'^"(llvm|llvm-c|clang|clang-c)/', include):
        return 2
    if re.match(r'^(<|"(gtest|gmock|isl|json)/)', include):
        return 3
    return 1


def sort_include_blocks(text: str, changed: set[int], path: PurePosixPath) -> str:
    """Sorts each run of #include lines that holds a changed line, as clang-format does with
    SortIncludes: CaseSensitive and IncludeBlocks: Preserve."""
    lines = text.split("\n")
    is_source = path.suffix in {".cpp", ".mm"}
    first_block = True
    index = 0
    while index < len(lines):
        if not INCLUDE.match(lines[index]):
            index += 1
            continue
        end = index
        while end < len(lines) and INCLUDE.match(lines[end]):
            end += 1
        if any(index <= line < end for line in changed):
            main_stem = path.stem if is_source and first_block else None

            def key(line: str, main_stem: str | None = main_stem) -> tuple[int, str]:
                include = INCLUDE.match(line).group(1)  # type: ignore[union-attr]
                return include_priority(include, main_stem), include

            lines[index:end] = sorted(lines[index:end], key=key)
        first_block = False
        index = end
    return "\n".join(lines)


class CMakeList:
    """One CMakeLists.txt: target_sources blocks of file names and add_subdirectory lines."""

    def __init__(self, path: PurePosixPath, text: str):
        self.path = path
        self.lines = text.split("\n")

    def text(self) -> str:
        return "\n".join(self.lines)

    def blocks(self) -> list[tuple[str, int, int]]:
        """(target, line of target_sources, line of its closing parenthesis) of each block."""
        found = []
        index = 0
        while index < len(self.lines):
            match = BLOCK_START.match(self.lines[index])
            if match:
                end = index + 1
                while end < len(self.lines) and self.lines[end].strip() != ")":
                    end += 1
                if end == len(self.lines):
                    raise MoveError(f"{self.path}: target_sources without its closing )")
                found.append((match.group(1), index, end))
                index = end
            index += 1
        return found

    def remove(self, name: str) -> list[str]:
        """Takes out every line of file name and returns its targets, in list order."""
        targets = []
        while True:
            for target, start, end in self.blocks():
                found = next(
                    (i for i in range(start + 1, end) if self.lines[i].strip() == name), None
                )
                if found is not None:
                    del self.lines[found]
                    if end - start - 1 == 1:
                        self._remove_block(start)
                    targets.append(target)
                    break
            else:
                break
        if not targets:
            raise MoveError(f"{self.path}: {name} is not listed in any plain target_sources block")
        return targets

    def _remove_block(self, start: int) -> None:
        end = start
        while self.lines[end].strip() != ")":
            end += 1
        del self.lines[start : end + 1]
        # One blank line goes with it, so no double gap is left.
        if start < len(self.lines) and self.lines[start] == "" and (
            start == 0 or self.lines[start - 1] == ""
        ):
            del self.lines[start]
        elif start > 0 and self.lines[start - 1] == "" and (
            start == len(self.lines) or self.lines[start] == ""
        ):
            del self.lines[start - 1]

    def add(self, target: str, name: str) -> None:
        """Puts file name in target's block, sorted, making the block when there is none."""
        blocks = self.blocks()
        for block_target, start, end in blocks:
            if block_target == target:
                names = [line.strip() for line in self.lines[start + 1 : end]]
                if name in names:
                    raise MoveError(f"{self.path}: {name} is already listed for {target}")
                at = start + 1
                while at < end and (
                    self.lines[at].strip().startswith("#") or self.lines[at].strip() < name
                ):
                    at += 1
                self.lines.insert(at, "    " + name)
                return
        block = [f"target_sources({target} PRIVATE", "    " + name, ")"]
        rank = TARGETS.index(target) if target in TARGETS else len(TARGETS)
        later = [
            start
            for block_target, start, _ in blocks
            if (TARGETS.index(block_target) if block_target in TARGETS else len(TARGETS)) > rank
        ]
        if later:
            at = later[0]
            self.lines[at:at] = block + [""]
            return
        if blocks:
            at = blocks[-1][2] + 1
            self.lines[at:at] = [""] + block
            return
        # A list of only add_subdirectory lines (or an empty one): the block goes first.
        while self.lines and self.lines[0] == "":
            del self.lines[0]
        self.lines[0:0] = block + [""]

    def add_subdirectory(self, name: str) -> None:
        entries = [
            (index, match.group(1))
            for index, line in enumerate(self.lines)
            if (match := SUBDIRECTORY.match(line))
        ]
        if any(entry == name for _, entry in entries):
            return
        line = f"add_subdirectory({name})"
        for index, entry in entries:
            if entry > name:
                self.lines.insert(index, line)
                return
        if entries:
            self.lines.insert(entries[-1][0] + 1, line)
            return
        while self.lines and self.lines[-1] == "":
            del self.lines[-1]
        self.lines.extend(([""] if self.lines else []) + [line, ""])


def clang_format() -> str | None:
    """clang-format on PATH, or the one Visual Studio brings (the Makefile looks there too)."""
    found = shutil.which("clang-format")
    if found is None and os.environ.get("VCINSTALLDIR"):
        bundled = Path(os.environ["VCINSTALLDIR"]) / "Tools/Llvm/x64/bin/clang-format.exe"
        found = str(bundled) if bundled.exists() else None
    return found


def scanned_files() -> list[PurePosixPath]:
    """The text files whose mentions are rewritten."""
    found = []
    for top in SCANNED:
        for file in sorted((ROOT / top).rglob("*")):
            relative = PurePosixPath(file.relative_to(ROOT).as_posix())
            if (
                file.is_file()
                and file.suffix in TEXT_SUFFIXES
                and not SKIPPED_PARTS.intersection(relative.parts)
                and file != Path(__file__).resolve()
            ):
                found.append(relative)
    return found


def reported_files() -> list[PurePosixPath]:
    """The files outside SCANNED that are only searched, for the closing report."""
    found = []
    for entry in REPORTED:
        path = ROOT / entry
        files = [path] if path.is_file() else sorted(path.rglob("*")) if path.is_dir() else []
        for file in files:
            relative = PurePosixPath(file.relative_to(ROOT).as_posix())
            if not file.is_file() or any(
                str(relative) == skip or str(relative).startswith(skip + "/")
                for skip in REPORT_SKIPPED
            ):
                continue
            if file.suffix in TEXT_SUFFIXES | {".yml", ".yaml", ".json", ".cmake", ""}:
                found.append(relative)
    return found


def plan(sources: list[str], folder: str):
    """Everything the move changes, computed before anything is touched."""
    destination = posix(folder)
    if destination.suffix:
        raise MoveError(f"{folder}: the last argument is the folder the files go to")
    if destination.parts[0] not in ("src", "tests"):
        raise MoveError(f"{folder}: modules live under src/ or tests/")
    moves: list[tuple[PurePosixPath, PurePosixPath]] = []
    for source in sources:
        for old in files_of(posix(source)):
            new = destination / old.name
            if exists(new) or any(new == planned for _, planned in moves):
                raise MoveError(f"{new} exists already")
            if old.parent == destination:
                raise MoveError(f"{old} is in {destination} already")
            moves.append((old, new))

    # The CMake lists, loaded once each, keyed by folder.
    lists: dict[PurePosixPath, CMakeList] = {}
    created: list[PurePosixPath] = []

    def list_of(directory: PurePosixPath, create: bool) -> CMakeList:
        if directory in lists:
            return lists[directory]
        path = directory / LIST
        if exists(path):
            lists[directory] = CMakeList(path, read(path))
        elif create:
            if len(directory.parts) <= 1:
                raise MoveError(f"{directory} has no {LIST} and is too high to get one")
            parent = list_of(directory.parent, create=True)
            parent.add_subdirectory(directory.name)
            lists[directory] = CMakeList(path, "")
            created.append(directory)
        else:
            raise MoveError(f"{directory} has no {LIST}")
        return lists[directory]

    for old, new in moves:
        for target in list_of(old.parent, create=False).remove(old.name):
            list_of(new.parent, create=True).add(target, new.name)

    renames = {}
    for old, new in moves:
        old_key, new_key = mention_key(old), mention_key(new)
        if old_key is not None and new_key is not None and old_key != new_key:
            renames[old_key] = new_key

    # Every scanned file, with the text it ends up with, at its new path.
    patterns = mention_patterns(renames)
    edits: dict[PurePosixPath, tuple[str, str]] = {}
    moved = dict(moves)
    for path in scanned_files():
        text = read(path)
        new_text, changed = rewrite_mentions(text, patterns)
        final = moved.get(path, path)
        if changed:
            if final.suffix in SOURCE_SUFFIXES:
                new_text = sort_include_blocks(new_text, changed, final)
            edits[final] = (text, new_text)

    for directory, cmake in lists.items():
        path = directory / LIST
        before = read(path) if exists(path) else ""
        after = cmake.text()
        if not after.endswith("\n"):
            after += "\n"
        edits[path] = (before, after)

    # Mentions left outside SCANNED, for a person to update.
    leftovers = []
    for path in reported_files():
        try:
            text = read(path)
        except UnicodeDecodeError:
            continue
        for number, line in enumerate(text.split("\n"), start=1):
            if any(pattern.search(line) for pattern, _ in patterns):
                leftovers.append(f"{path}:{number}: {line.strip()[:100]}")
    return moves, created, edits, leftovers


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Move modules to another folder: git mv, includes and CMake lists."
    )
    parser.add_argument("--dry-run", action="store_true", help="print the changes, make none")
    parser.add_argument("paths", nargs="+", metavar="SOURCE ... FOLDER")
    args = parser.parse_args()
    if len(args.paths) < 2:
        parser.error("give at least one SOURCE and the FOLDER")
    try:
        moves, created, edits, leftovers = plan(args.paths[:-1], args.paths[-1])
    except MoveError as error:
        print(f"move_module: {error}", file=sys.stderr)
        return 1

    for old, new in moves:
        print(f"git mv {old} {new}")
    for directory in created:
        print(f"new list {directory / LIST}")
    for path, (before, after) in sorted(edits.items()):
        if before == after:
            continue
        if args.dry_run:
            sys.stdout.writelines(
                difflib.unified_diff(
                    before.splitlines(keepends=True),
                    after.splitlines(keepends=True),
                    fromfile=f"a/{path}",
                    tofile=f"b/{path}",
                )
            )
        else:
            print(f"edit {path}")
    if leftovers:
        print("still named outside src/, tests/ and tools/ (update by hand):")
        for line in leftovers:
            print(f"  {line}")
    if args.dry_run:
        return 0

    for old, new in moves:
        (ROOT / new.parent).mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "mv", str(old), str(new)], cwd=ROOT, check=True)
    for path, (before, after) in edits.items():
        if before != after:
            write(path, after)
    for directory in created:
        subprocess.run(["git", "add", str(directory / LIST)], cwd=ROOT, check=True)
    sources = [
        str(path)
        for path, (before, after) in edits.items()
        if before != after and path.suffix in SOURCE_SUFFIXES
    ]
    formatter = clang_format()
    if formatter is None:
        print("clang-format not found: run make format to wrap the edited comments")
    elif sources:
        subprocess.run([formatter, "-i", *sources], cwd=ROOT, check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
