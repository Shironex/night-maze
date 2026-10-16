# Takes the pictures of the README from the game itself and writes them, as they come
# out of the window, into showcase-out/raw/ (not committed). `pnpm showcase` then frames
# them and builds the banner, into docs/showcase/ (committed).
#
# Run from the repository root, on Windows or on macOS, after a Release build
# (make release):
#   python tools/capture_showcase.py
#   python tools/capture_showcase.py --only round,lever    (only these runs)
# It needs ffmpeg on PATH and nothing else: no Python package is used. What depends on
# the system is in tools/showcase_windows.py (the window code and the screen grabber of
# tools/record_menu_loop.py, SendInput) and tools/showcase_macos.py (CoreGraphics,
# screencapture). On macOS the terminal needs two permissions, Screen Recording for the
# pictures and Accessibility for the keys and the mouse; without the second the runs
# that send input are left out and named at the end.
#
# Every picture is one entry of RUNS below: the switches the game is started with
# (game/StartOptions.hpp), its settings file, and a few steps. A picture that only needs
# a place is reached with the switches alone (the menu camera). A picture that needs
# a moment of a round (a note being read, the tea working) is reached with a few key
# presses and mouse turns, which this script sends. The mazes are chosen so that those
# moments are a straight walk away from the start (see RUNS).
#
# The game runs in a folder of its own under the temporary folder (WORK), so the settings
# file of the player is never touched, and it is silent (master_volume = 0).
#
# While it runs, the game window (1280 x 720) is on the screen, on top of the other
# windows (on macOS it stays where the game opens it, in the middle of the main
# screen). Keys and mouse turns go to whatever window is the active one, so before each
# of them the script checks that this is the game window, and stops that picture when it
# is not. For as long as it sends input it holds the file nm-input.lock in the temporary
# folder: another script that wants the keyboard waits for that file to go away, and
# this one waits for theirs. Leave the mouse alone while a round is on the screen: it
# turns the camera.
#
# The pictures go out of date whenever the look of the game changes. Then run this script
# and `pnpm showcase` again, and look at every picture before committing it.
import argparse
import os
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from record_menu_loop import (DEFAULT_EXE, HEIGHT, LEAD_IN_SECONDS, REPOSITORY,  # noqa: E402
                              SETTINGS_FILE_NAME, WIDTH, WINDOW_LEFT, WINDOW_TOP)

# Everything that depends on the system (finding the window, bringing it to the front,
# the picture, the keys and the mouse) is in one module per system, with the same
# functions in both: "desk" is the one of this computer.
if sys.platform == "win32":
    import showcase_windows as desk  # noqa: E402
elif sys.platform == "darwin":
    import showcase_macos as desk  # noqa: E402
else:
    sys.exit("tools/capture_showcase.py runs on Windows and on macOS only")

OUT_DIR = os.path.join(REPOSITORY, "showcase-out", "raw")
WORK = os.path.join(tempfile.gettempdir(), "nm-showcase-work")
LOCK_FILE = os.path.join(tempfile.gettempdir(), "nm-input.lock")

# How long to wait for the lock of another script, and for the mouse pointer of the
# person at the computer to leave the game window, in seconds.
LOCK_WAIT_SECONDS = 120.0
POINTER_WAIT_SECONDS = 10.0

# The settings file writes mouse_sensitivity = 4, which is 0.1 degrees per unit of mouse
# movement (game/Settings.hpp), so a turn of 90 degrees is 900 units: desk.UNITS_PER_DEGREE.
MOUSE_SENSITIVITY = 4

# ---- the pictures -------------------------------------------------------------------------
#
# The steps of a run, in order:
#   ("wait", seconds)        let the game run
#   ("grab", name)           write showcase-out/raw/<name>.png
#   ("hold", keys, seconds)  hold the keys (a string, "W" or "W+SHIFT") that long
#   ("down", key), ("up", key)   press and release a key by hand, to grab in between
#   ("tap", key)             press a key once
#   ("turn", right, up)      turn the camera by that many degrees to the right and up
#   ("pointer_out",)         wait until the mouse pointer is outside the game window


@dataclass
class Run:
    name: str
    # The switches of the command line.
    switches: list
    # Lines of the settings file, on top of the ones every run gets.
    settings: dict = field(default_factory=dict)
    steps: list = field(default_factory=list)


def camera(name, seed, shot, at):
    # A picture of the menu camera: the game shows itself, without HUD and without menu.
    # The picture is taken LEAD_IN_SECONDS after the start, so the shot starts that much
    # earlier than the moment that is wanted.
    return Run(name, ["--menu-camera", "--calm", "--seed", str(seed), "--menu-shot", shot,
                      "--menu-time", str(at - LEAD_IN_SECONDS)],
               steps=[("grab", name)])


# Seed 1 on the easy level is the maze of the menu video. The moments of the camera
# shots were found by looking at one picture every few seconds of its walk.
#
# Seed 76, easy: the start cell (0, 0) looks south down a corridor of four cells. A story
# note hangs on the west wall of the third cell and the flask lies in the fourth, which
# is a corner. The player walks 3 m per second and sprints 5.5 m per second, and a cell
# is 2 m wide: that is where the seconds below come from. Every sprint is a little longer
# than its corridor and ends against a wall, so a late key press cannot add up.
#
# Seed 98, easy: the start cell looks east down a corridor of four cells with a lever on
# the north wall of the last one. The lever opens the wall at the end of that corridor.
#
# Every run has the switch --calm: the maze then has no shade, so no picture has a catch
# in the middle of its walk. The two exceptions: the one picture OF the shade is the run
# "shade", and the run "intro" plays the intro (the switch --intro).
RUNS = [
    # The main menu over its video, then the settings screen: Tab five times goes from
    # the first entry (the campaign) over "Nights", "Ledger", "Tonight's hedge" and
    # "Free play" to "Settings". The day beside "Tonight's hedge" is the fixed one of a run with
    # a switch, so the picture is the same on every day. --calm
    # changes nothing in these two pictures (no round is started): it is there so that
    # no run but "shade" can ever have a shade. A picture of a night of the campaign would
    # start with --night <1..5> in place of --play (no such picture is taken yet).
    Run("menu", ["--calm", "--seed", "76"], {"difficulty": "normal", "master_volume": 100}, [
        ("wait", 2.0), ("pointer_out",), ("grab", "menu"),
        ("tap", "TAB"), ("tap", "TAB"), ("tap", "TAB"), ("tap", "TAB"), ("tap", "TAB"),
        ("tap", "ENTER"),
        ("wait", 1.0),
        ("pointer_out",), ("grab", "settings")]),
    # A corridor with a crystal glowing in front of the lit wall at its end.
    camera("corridor", 1, "walk", 75.0),
    # A crystal in a corner of worn walls: stones missing on the left, moss in the
    # middle, cracks on the right.
    camera("worn-walls", 1, "walk", 91.0),
    # High over the maze.
    camera("glide", 1, "glide", 35.0),
    # A round: the note is read from one cell away, then the flask is picked up and the
    # tea pays for a sprint through six corridors, and the map shows what was seen.
    Run("round", ["--play", "--calm", "--seed", "76"], {"story_line": 21}, [
        ("hold", "W", 0.667), ("turn", 24.2, 0), ("wait", 0.4),
        ("tap", "E"), ("wait", 0.4), ("grab", "note"), ("tap", "E"), ("turn", -24.2, 0),
        ("hold", "W", 1.5), ("turn", -90, 0), ("hold", "W+SHIFT", 0.7),
        ("turn", -90, 0), ("hold", "W+SHIFT", 1.15), ("turn", 90, 0),
        ("hold", "W+SHIFT", 1.4), ("turn", 90, 0), ("hold", "W+SHIFT", 1.15),
        ("turn", -90, 0), ("hold", "W+SHIFT", 0.7), ("turn", 90, 0),
        ("hold", "W+SHIFT", 0.7), ("wait", 0.2),
        ("down", "M"), ("wait", 0.6), ("grab", "map"), ("up", "M")]),
    # A sprint until the stamina is gone, a few steps back, a look at the lever and a pull:
    # the picture is taken while the wall it opens is sinking.
    Run("lever", ["--play", "--calm", "--seed", "98"], {}, [
        ("hold", "W+SHIFT", 6.4), ("hold", "S", 0.8), ("turn", -25.9, -13.6),
        ("wait", 0.2), ("tap", "E"), ("wait", 0.7), ("grab", "lever")]),
    # The places below are reached with the start switches (--start-cell, --start-yaw and
    # --collect-all), so nothing has to be walked. Seed 76, easy: the flask lies in the
    # dead end (6, 6), open to the south. The exit is (9, 6), and its gate stands in the
    # open side to the north, towards the cell (9, 5). The maze of a seed is printed
    # by building it (game::buildMazeWorld) and looking at its cells.
    # A flask at the end of a dead end corridor, four metres ahead, in the beam.
    Run("flask", ["--play", "--calm", "--seed", "76", "--start-cell", "6,8", "--start-yaw", "0"],
        {}, [("wait", 0.5), ("grab", "flask")]),
    # The gate across the last corridor, closed, with no crystal collected.
    Run("gate-closed", ["--play", "--calm", "--seed", "76", "--start-cell", "9,4",
                        "--start-yaw", "180"], {}, [("wait", 0.5), ("grab", "gate-closed")]),
    # The same place with every crystal collected: the gate has sunk into the ground.
    Run("gate-open", ["--play", "--calm", "--seed", "76", "--start-cell", "9,4",
                      "--start-yaw", "180", "--collect-all"], {}, [
        ("wait", 0.5), ("grab", "gate-open")]),
    # Every crystal collected, a few seconds of waiting so that the time of the round is
    # not 0:00, and a walk through the open gate (4 m at 3 m per second) into the exit:
    # the card "Through the gate" is on the screen.
    Run("round-end", ["--play", "--calm", "--seed", "76", "--start-cell", "9,4",
                      "--start-yaw", "180", "--collect-all"], {}, [
        ("wait", 5.0), ("hold", "W", 2.0), ("wait", 1.0), ("grab", "round-end")]),
    # The shade, in the one run without --calm. Seed 6, easy: the shade starts in the
    # cell (1, 9), at the west end of a corridor of seven cells along the last row. The
    # player starts three cells east of it and looks west, so the beam is on it. It
    # stands still for the first eight seconds of a round anyway (its grace time), and
    # for as long as the light is on it after that.
    Run("shade", ["--play", "--seed", "6", "--start-cell", "4,9", "--start-yaw", "270"], {}, [
        ("wait", 0.5), ("grab", "shade")]),
    # The storybook, the intro of a campaign (--intro plays it at the start of the run,
    # --intro-style storybook names it, though it is the default). The second of its six
    # pictures: the old maze on the hill with the splinters of the moon glowing in its
    # corridors, the sleeping village below, and both lines of its text standing. Of the
    # six it is the one that shows the maze of the game, and its text stands on the
    # quiet snow and not on a figure (the sixth, the child with the lamp, was the other
    # choice). The intro counts real time from its first frame, which comes after its
    # pictures and sounds are loaded. The first picture stays 7.75 seconds (as long as
    # its spoken line lasts), the second 11. A picture every two seconds of a run on a
    # Mac showed the second with both lines up from about 9 to 20 seconds after the
    # lead-in: the moment below is in the middle, with room for a slower start.
    Run("intro", ["--intro", "--intro-style", "storybook"], {}, [
        ("wait", 14.5), ("grab", "intro")]),
    # The tea at work: the flask is picked up by walking into the dead end, and the bar
    # of the tea is on the HUD. The player turns round to look back down the corridor.
    Run("tea", ["--play", "--calm", "--seed", "76", "--start-cell", "6,8", "--start-yaw", "0"],
        {}, [
        ("hold", "W", 1.5), ("turn", 180, 0), ("wait", 0.5), ("grab", "tea")]),
]

# ---- keys and mouse ----------------------------------------------------------------------


class Keyboard:
    # Sends keys and mouse turns, each only while the game window is the active one, and
    # remembers which keys are down so that all of them can be released at the end.

    def __init__(self, window):
        self.window = window
        self.down = set()

    def check(self):
        if not desk.is_front(self.window):
            raise RuntimeError("the game window is not the active window: nothing was sent")

    def key(self, name, up):
        if up:
            # A release is sent whatever window is in front: a key that stays down would
            # repeat there. A release without a press before it does nothing.
            self.down.discard(name)
        else:
            self.check()
            self.down.add(name)
        desk.send_key(name, up)

    def turn(self, right, up):
        # In pieces of at most 3 degrees, a moment apart: one large jump would be a single
        # mouse event. Moving the mouse down looks down, so up is sent with a minus sign.
        x = round(right * desk.UNITS_PER_DEGREE)
        y = -round(up * desk.UNITS_PER_DEGREE)
        pieces = max(1, (max(abs(x), abs(y)) + 29) // 30)
        for piece in range(1, pieces + 1):
            dx = x * piece // pieces - x * (piece - 1) // pieces
            dy = y * piece // pieces - y * (piece - 1) // pieces
            self.check()
            desk.send_move(self.window, dx, dy)
            time.sleep(0.01)

    def release_all(self):
        for name in list(self.down):
            self.key(name, up=True)


class InputLock:
    # The file that says "somebody is sending keys": created here, deleted at the end.

    def __enter__(self):
        waited_from = time.monotonic()
        while True:
            try:
                # "x": fails when the file is there, so two scripts cannot both get it.
                with open(LOCK_FILE, "x", encoding="utf-8") as file:
                    file.write(f"tools/capture_showcase.py, process {os.getpid()}\n")
                return self
            except FileExistsError:
                if time.monotonic() - waited_from > LOCK_WAIT_SECONDS:
                    raise RuntimeError(f"{LOCK_FILE} is held by another script") from None
                time.sleep(0.5)

    def __exit__(self, *_):
        os.remove(LOCK_FILE)
        return False


# ---- the game -----------------------------------------------------------------------------


class Game:
    # Starts the game for one run and stops it again: "with Game(...) as game:", and
    # game.window is the game window.

    def __init__(self, exe, a_run):
        self.exe = exe
        self.run = a_run
        self.process = None
        self.window = None
        self.pointer = (0, 0)
        # Whether the script moved the mouse pointer itself (only on macOS, before a
        # picture of a menu: desk.move_pointer_away).
        self.pointer_moved = False

    def __enter__(self):
        os.makedirs(WORK, exist_ok=True)
        settings = {"fullscreen": "off", "window_size": f"{WIDTH}x{HEIGHT}",
                    "difficulty": "easy", "mouse_sensitivity": MOUSE_SENSITIVITY,
                    "master_volume": 0, "story_line": 0} | self.run.settings
        with open(os.path.join(WORK, SETTINGS_FILE_NAME), "w", encoding="utf-8") as file:
            for name, value in settings.items():
                file.write(f"{name} = {value}\n")

        # During a round the game hides the mouse pointer and keeps it in the middle of
        # its window. Where the pointer was is remembered here and put back at the end.
        self.pointer = desk.pointer()
        self.process = subprocess.Popen([self.exe, *self.run.switches], cwd=WORK,
                                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        started = time.monotonic()
        window = None
        while window is None and time.monotonic() - started < 20.0:
            if self.process.poll() is not None:
                raise RuntimeError("the game ended right after its start")
            window = desk.find_window(self.process.pid, self.pointer)
            time.sleep(0.05)
        if window is None:
            raise RuntimeError("the game window did not appear")

        left, top = self.place()
        desk.show(window, left, top)
        # A run that sends keys needs the game window in front, in a round and in a menu.
        if starts_round(self.run) or sends_input(self.run):
            desk.bring_to_front(window)
        time.sleep(max(0.0, LEAD_IN_SECONDS - (time.monotonic() - started)))
        width, height = desk.client_size(window)
        if width != WIDTH or height != HEIGHT:
            raise RuntimeError(f"the game window is {width} x {height}, not {WIDTH} x {HEIGHT}")
        self.window = window
        return self

    def place(self):
        # Where the window goes: the usual corner of the main screen. A menu lights up
        # the button under the mouse pointer, so a run that waits for the pointer to be
        # outside ("pointer_out") starts on another screen when the pointer is in the
        # way and there is one. With one screen it waits for the pointer to move. (On
        # macOS the window is not moved at all: desk.show and desk.move_pointer_away.)
        usual = (WINDOW_LEFT, WINDOW_TOP)
        if ("pointer_out",) not in self.run.steps:
            return usual
        x, y = self.pointer
        for left, top in [usual] + [place for place in desk.screen_corners() if place != usual]:
            over_x = left <= x < left + WIDTH
            over_y = top <= y < top + HEIGHT
            if not (over_x and over_y):
                return left, top
        return usual

    def __exit__(self, *_):
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            self.process.wait(timeout=10)
        if starts_round(self.run) or self.pointer_moved:
            desk.put_pointer(*self.pointer)
        return False


def wait_for_pointer(game):
    if desk.move_pointer_away(game.window):
        game.pointer_moved = True
    waited_from = time.monotonic()
    while desk.pointer_inside(game.window):
        if time.monotonic() - waited_from > POINTER_WAIT_SECONDS:
            raise RuntimeError("the mouse pointer stayed over the game window")
        time.sleep(0.2)


def play(exe, a_run, out_dir):
    with Game(exe, a_run) as game:
        window = game.window
        keyboard = Keyboard(window)
        try:
            for step in a_run.steps:
                kind = step[0]
                if kind == "wait":
                    time.sleep(step[1])
                elif kind == "grab":
                    # A round stops with the pause menu when its window is not the
                    # active one any more: that must not be the picture.
                    if starts_round(a_run):
                        keyboard.check()
                    desk.grab(window, os.path.join(out_dir, step[1] + ".png"))
                elif kind == "hold":
                    names = step[1].split("+")
                    for name in names:
                        keyboard.key(name, up=False)
                    time.sleep(step[2])
                    for name in reversed(names):
                        keyboard.key(name, up=True)
                elif kind in ("down", "up"):
                    keyboard.key(step[1], up=kind == "up")
                elif kind == "tap":
                    keyboard.key(step[1], up=False)
                    time.sleep(0.05)
                    keyboard.key(step[1], up=True)
                    time.sleep(0.25)
                elif kind == "turn":
                    keyboard.turn(step[1], step[2])
                    time.sleep(0.15)
                elif kind == "pointer_out":
                    wait_for_pointer(game)
                else:
                    raise ValueError(f"unknown step: {step}")
        finally:
            keyboard.release_all()


def starts_round(a_run):
    # A run that begins in a round: free play (--play), a night of the campaign (--night)
    # or the maze of a day (--daily).
    return any(switch in a_run.switches for switch in ("--play", "--night", "--daily"))


def sends_input(a_run):
    return any(step[0] in ("hold", "down", "up", "tap", "turn") for step in a_run.steps)


def main():
    parser = argparse.ArgumentParser(description="Takes the pictures of the README.")
    parser.add_argument("--exe", default=DEFAULT_EXE, help="the game to take them from")
    parser.add_argument("--only", help="names of the runs to do, with commas between them")
    parser.add_argument("--out-dir", default=OUT_DIR, help="where the pictures go")
    arguments = parser.parse_args()

    if not os.path.isfile(arguments.exe):
        sys.exit(f"The game is not built: {arguments.exe} (run make release first)")
    desk.prepare()
    os.makedirs(arguments.out_dir, exist_ok=True)
    wanted = arguments.only.split(",") if arguments.only else [a_run.name for a_run in RUNS]
    # Whether the system lets this script send keys and mouse movements at all. When it
    # does not, the runs that need them are left out and the others are still taken.
    input_allowed, refusal = desk.input_allowed()

    failed = []
    for a_run in RUNS:
        if a_run.name not in wanted:
            continue
        print("run", a_run.name)
        try:
            if sends_input(a_run) and not input_allowed:
                raise RuntimeError(refusal)
            if sends_input(a_run):
                with InputLock():
                    play(arguments.exe, a_run, arguments.out_dir)
            else:
                play(arguments.exe, a_run, arguments.out_dir)
        except (RuntimeError, subprocess.CalledProcessError) as error:
            print(f"FAILED {a_run.name}: {error}")
            failed.append(a_run.name)
    if failed:
        sys.exit("Not taken: " + ", ".join(failed) + ". Run them again with --only.")


if __name__ == "__main__":
    main()
