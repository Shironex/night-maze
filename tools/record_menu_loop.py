# Records the video loop behind the main menu from the game itself and writes the two
# files the game ships: assets/video/menu_loop.mp4 and assets/video/menu_still.png.
#
# Run from the repository root, on Windows, after a Release build (make release):
#   python tools/record_menu_loop.py
# It needs ffmpeg and ffprobe on PATH and nothing else: no Python package is used.
#
# What it does, in three steps:
#
#   1. RECORD. For every shot of the list SHOTS below it starts the game in menu camera
#      mode (the switches --menu-camera, --seed, --menu-shot and --menu-time: the game
#      then shows itself, without HUD, minimap, menu or debug window), waits until the
#      picture runs and records the inside of the game window with ffmpeg (gdigrab, the
#      screen grabber for Windows). The recording is lossless: nothing is thrown away
#      before the cut.
#   2. CUT. The shots are joined with crossfades into ONE loop. The end of the last shot
#      fades into the beginning of the first one, so the last frame of the file is
#      followed by its first frame like any frame by the next: the loop has no seam.
#   3. ENCODE. One H.264 file that the decoders of Windows (Media Foundation) and of
#      macOS (AVFoundation) both play, with its colours tagged, and the first frame of
#      that file once more as a PNG: the still picture the menu shows without the video.
#
# While step 1 runs, the game window is on the screen, on top of the other windows, and
# must stay open and must not be minimised. gdigrab is given the window itself and not
# a rectangle of the screen, so only what the game drew is recorded: not the mouse
# cursor, and not another program that draws over the window (a desktop pet, a
# notification). Do not lock the screen meanwhile. Step 1 takes the sum of the shot
# lengths plus a few seconds per shot.
#
# The loop goes out of date whenever the look of the game changes (new effects, new
# models, new lighting). Then run this script again. To change WHAT the loop shows, edit
# SHOTS. To find a good moment for a shot, look at single pictures first:
#   python tools/record_menu_loop.py --scout walk --seed 1 --difficulty easy
#          --from 0 --to 120 --step 10 --out some/folder
# writes one picture per time offset into the folder (one start of the game each).
#
# Other switches: --exe (the game to record, default the Release build), --work (where
# the recordings of the shots are kept, default a new temporary folder), --cut-only
# (skip step 1 and cut the recordings that are in --work already), --out-dir (where the
# two files go, default assets/video).
import argparse
import ctypes
import ctypes.wintypes
import json
import os
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass

# ---- the shot list ------------------------------------------------------------------------


@dataclass
class Shot:
    # A short name, used for the file of the recording and in the messages.
    name: str
    # The seed of the maze (--seed): the same seed gives the same maze.
    seed: int
    # The level, which decides the size of the maze: easy (10 x 10), normal (16 x 16) or
    # hard (22 x 22). It is written into the settings file the game reads at its start.
    difficulty: str
    # The shot of the menu camera (--menu-shot): "walk" through the corridors at eye
    # height with the flashlight on, or "glide" high above the maze.
    shot: str
    # Where in the loop of that shot the camera starts, in seconds (--menu-time).
    time: float
    # How many seconds of it are used.
    seconds: float


# The loop, shot by shot: 4 shots of 8.5 seconds with crossfades of 1 second give a loop
# of 30 seconds. The first shot is what the menu opens with. All four are the maze of
# seed 1 on the easy level, so the corridors are the ones the glide shows from above.
SHOTS = [
    # High over the maze: the walls in the moonlight and the glow of the crystals.
    Shot("glide", seed=1, difficulty="easy", shot="glide", time=14.0, seconds=8.5),
    # A long corridor in the cone of the flashlight, with the shadows of the pillars,
    # then round a corner to a lever on the wall.
    Shot("lever", seed=1, difficulty="easy", shot="walk", time=72.0, seconds=8.5),
    # Towards a crystal that glows at the end of a corridor, over the grass.
    Shot("crystal", seed=1, difficulty="easy", shot="walk", time=474.0, seconds=8.5),
    # Past the wooden gate of the exit and on to three crystals close by.
    Shot("gate", seed=1, difficulty="easy", shot="walk", time=244.0, seconds=8.5),
]

# ---- the numbers of the recording ---------------------------------------------------------

# The size of the picture and its frame rate. The game window is opened in this size
# (the settings file), so the recording is pixel for pixel what the game drew.
WIDTH = 1280
HEIGHT = 720
FRAMES_PER_SECOND = 30

# How long a crossfade between two shots takes, in seconds.
CROSSFADE_SECONDS = 1.0

# How long the game runs before the recording of a shot starts, in seconds: the window
# has to open, the first frames are slow, and the window needs a moment to get on top.
# The picture of a shot therefore starts this long after its --menu-time.
LEAD_IN_SECONDS = 3.0

# The quality of the final file: the CRF of the x264 encoder. Lower is better and larger.
# At 16 the dark, smooth parts of the night sky keep their soft steps, and the file stays
# well under the size limit below.
QUALITY_CRF = 16

# The file should not be larger than this: it lives in git as a normal file.
MAX_BYTES = 20 * 1000 * 1000

# One key frame at least every this many frames (two seconds). The first frame of the
# file is always one, which is what going back to the start of the loop needs.
KEY_FRAME_DISTANCE = 60

# Where the game window is put on the screen: its top left corner, in pixels.
WINDOW_LEFT = 40
WINDOW_TOP = 40

# The title of the game window, and the name of the settings file it reads from its
# working directory (game::SETTINGS_FILE_NAME).
WINDOW_TITLE = "Night Maze"
SETTINGS_FILE_NAME = "night-maze-settings.txt"

REPOSITORY = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_EXE = os.path.join(REPOSITORY, "build", "release", "Release", "night_maze.exe")
DEFAULT_OUT_DIR = os.path.join(REPOSITORY, "assets", "video")
LOOP_FILE_NAME = "menu_loop.mp4"
STILL_FILE_NAME = "menu_still.png"

# ---- the game window (Windows only) -------------------------------------------------------

user32 = ctypes.windll.user32

# SetWindowPos: "above all other windows", and "keep the size, do not make it the active
# window, show it".
HWND_TOPMOST = -1
SWP_NOSIZE = 0x0001
SWP_NOACTIVATE = 0x0010
SWP_SHOWWINDOW = 0x0040

# The types of the call. Without them ctypes passes every argument as a 32 bit number, so
# the handle -1 of HWND_TOPMOST becomes 0xFFFFFFFF on a 64 bit Windows, which names no
# window, and the call fails without a word.
user32.SetWindowPos.argtypes = [ctypes.wintypes.HWND, ctypes.wintypes.HWND, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
user32.SetWindowPos.restype = ctypes.wintypes.BOOL


def use_real_pixels():
    # Without this Windows would give this script scaled sizes on a display with scaling
    # (125 %, 150 %), and the size check of the game window would be wrong.
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(2)
    except OSError:
        pass


def put_on_top(window, left, top):
    # Moves the window to this place and above all other windows, without taking the
    # keyboard focus. Stops the run when Windows does not do it: a window that stayed
    # where it was could have another program over it, and that would be in the picture.
    if not user32.SetWindowPos(window, ctypes.wintypes.HWND(HWND_TOPMOST), left, top, 0, 0,
                               SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW):
        raise RuntimeError("the game window could not be moved")


def find_window(process_id):
    # The visible window with the title of the game that belongs to OUR game process.
    # Another copy of the game may be open on the same desktop: it is never touched.
    found = []

    @ctypes.WINFUNCTYPE(ctypes.wintypes.BOOL, ctypes.wintypes.HWND, ctypes.wintypes.LPARAM)
    def visit(window, _):
        owner = ctypes.wintypes.DWORD()
        user32.GetWindowThreadProcessId(window, ctypes.byref(owner))
        if owner.value == process_id and user32.IsWindowVisible(window):
            title = ctypes.create_unicode_buffer(256)
            user32.GetWindowTextW(window, title, 256)
            if title.value == WINDOW_TITLE:
                found.append(window)
        return True

    user32.EnumWindows(visit, 0)
    return found[0] if found else None


def client_size(window):
    # The size of the inside of the window (without title bar and frame) in pixels.
    rectangle = ctypes.wintypes.RECT()
    user32.GetClientRect(window, ctypes.byref(rectangle))
    return rectangle.right, rectangle.bottom


class Game:
    # Starts the game for one shot and stops it again: "with Game(...) as window:".

    def __init__(self, exe, folder, shot):
        self.exe = exe
        self.folder = folder
        self.shot = shot
        self.process = None

    def __enter__(self):
        os.makedirs(self.folder, exist_ok=True)
        # The settings file of this run: a window of the recording size, and the level
        # that decides the size of the maze. The game reads it from its working
        # directory, which is this folder, so the settings of the player are not touched.
        with open(os.path.join(self.folder, SETTINGS_FILE_NAME), "w", encoding="utf-8") as file:
            file.write("fullscreen = off\n")
            file.write(f"window_size = {WIDTH}x{HEIGHT}\n")
            file.write(f"difficulty = {self.shot.difficulty}\n")

        self.process = subprocess.Popen(
            [self.exe, "--menu-camera", "--seed", str(self.shot.seed),
             "--menu-shot", self.shot.shot, "--menu-time", str(self.shot.time)],
            cwd=self.folder, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        started = time.monotonic()

        window = None
        while window is None and time.monotonic() - started < 20.0:
            if self.process.poll() is not None:
                raise RuntimeError("the game ended right after its start")
            window = find_window(self.process.pid)
            time.sleep(0.05)
        if window is None:
            raise RuntimeError("the game window did not appear")

        put_on_top(window, WINDOW_LEFT, WINDOW_TOP)
        # The rest of the lead-in: the picture runs, and the window is on top.
        time.sleep(max(0.0, LEAD_IN_SECONDS - (time.monotonic() - started)))

        width, height = client_size(window)
        if width != WIDTH or height != HEIGHT:
            raise RuntimeError(f"the game window is {width} x {height}, not {WIDTH} x {HEIGHT}")
        return window

    def __exit__(self, *_):
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            self.process.wait(timeout=10)
        return False


# ---- ffmpeg -------------------------------------------------------------------------------


def run(arguments):
    subprocess.run(arguments, check=True)


def grab_arguments(window):
    # The input of ffmpeg that copies the inside of the game window: the window is named
    # by its handle (a number), so a second window with the same title cannot be meant.
    return ["-f", "gdigrab", "-framerate", str(FRAMES_PER_SECOND), "-draw_mouse", "0",
            "-i", f"hwnd={window}"]


def recording_path(work, shot):
    return os.path.join(work, shot.name + ".mkv")


def record(exe, work, shot):
    # Lossless and in RGB (libx264rgb with quantizer 0): the file holds exactly the
    # pixels of the screen. The conversion to the colours of a video happens once, in
    # the final encode.
    with Game(exe, os.path.join(work, "run_" + shot.name), shot) as window:
        run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *grab_arguments(window),
             "-t", str(shot.seconds), "-c:v", "libx264rgb", "-preset", "ultrafast", "-qp", "0",
             recording_path(work, shot)])


def scout(exe, folder, shot, offsets):
    # One picture per time offset: the first frame a recording of the shot would have.
    os.makedirs(folder, exist_ok=True)
    for offset in offsets:
        at = Shot(shot.name, shot.seed, shot.difficulty, shot.shot, offset, 0.0)
        picture = os.path.join(folder, f"{shot.shot}_seed{shot.seed}_{shot.difficulty}_{offset:07.1f}.png")
        with Game(exe, os.path.join(folder, "run"), at) as window:
            run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *grab_arguments(window),
                 "-frames:v", "1", picture])
        print("wrote", picture)


def cut_filter(shots):
    # The filter graph of ffmpeg that joins the recordings into one loop.
    #
    # With the shots A, B, C and a crossfade of one second the loop is:
    #
    #     A without its first second  ->  B  ->  C  ->  the first second of A
    #
    # where every arrow is a crossfade. The loop ends in the middle of the fade from
    # C into the first second of A, exactly where that second is over, and it begins
    # with the frame of A that comes next. Its length is the sum of the shot lengths
    # minus one crossfade per shot.
    fade = CROSSFADE_SECONDS
    parts = []
    # Every recording gets the same frame rate, pixel format and time base: the
    # crossfade filter needs its two inputs to agree in all three.
    same = f"fps={FRAMES_PER_SECOND},format=gbrp,settb=AVTB"
    for index, shot in enumerate(shots):
        parts.append(f"[{index}:v]trim=duration={shot.seconds},setpts=PTS-STARTPTS,{same}[shot{index}]")
    # The first shot twice: its first second (the end of the loop) and the rest (the
    # beginning of the loop).
    parts.append("[shot0]split[first0][first1]")
    parts.append(f"[first0]trim=duration={fade},setpts=PTS-STARTPTS[head]")
    parts.append(f"[first1]trim=start={fade},setpts=PTS-STARTPTS[body]")

    # The chain of crossfades. offset is the moment the fade starts, counted in the
    # picture so far: its length minus the length of the fade.
    chain = ["body"] + [f"shot{index}" for index in range(1, len(shots))] + ["head"]
    lengths = [shots[0].seconds - fade] + [shot.seconds for shot in shots[1:]] + [fade]
    so_far = "body"
    length = lengths[0]
    for index in range(1, len(chain)):
        joined = f"joined{index}"
        parts.append(f"[{so_far}][{chain[index]}]xfade=transition=fade:duration={fade}:"
                     f"offset={length - fade}[{joined}]")
        so_far = joined
        length += lengths[index] - fade

    # From the RGB of the screen to the YUV of a video, with the BT.709 matrix and the
    # "TV" range (16 to 235) that the tags of the file will name. Written out here: left
    # to its defaults ffmpeg may convert with another matrix than the tags say, and
    # every decoder would then show slightly wrong colours.
    # setparams then writes on every frame what its numbers mean: BT.709 for the colours
    # (primaries), for the curve (transfer) and for the matrix, and the TV range. The
    # encoder copies that into the file. The switches of the same names on the command
    # line are not enough: ffmpeg lets what the frames say win over them.
    parts.append(f"[{so_far}]scale=out_color_matrix=bt709:out_range=tv,format=yuv420p,"
                 "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:range=tv[out]")
    return ";".join(parts), length


def cut_and_encode(work, shots, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    loop_file = os.path.join(out_dir, LOOP_FILE_NAME)
    still_file = os.path.join(out_dir, STILL_FILE_NAME)

    inputs = []
    for shot in shots:
        inputs += ["-i", recording_path(work, shot)]
    graph, seconds = cut_filter(shots)

    run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *inputs,
         "-filter_complex", graph, "-map", "[out]", "-an",
         # H.264, High profile, 8 bits, 4:2:0: what every hardware and software decoder
         # of the last fifteen years plays.
         "-c:v", "libx264", "-profile:v", "high", "-pix_fmt", "yuv420p",
         "-preset", "slow", "-crf", str(QUALITY_CRF),
         # No B-frames: every frame is stored in the order it is shown, so no decoder
         # has to reorder frames, and the first time stamp of the file is zero.
         "-bf", "0",
         "-g", str(KEY_FRAME_DISTANCE),
         # The index of the file at its beginning: a player can start without reading
         # the whole file first.
         "-movflags", "+faststart",
         loop_file])

    # The still: the first frame of the finished loop, decoded with the same matrix and
    # range, so the menu looks the same with the video and without it.
    run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", loop_file,
         "-frames:v", "1", "-vf", "scale=in_color_matrix=bt709:in_range=tv,format=rgb24",
         "-update", "1", still_file])
    return loop_file, still_file, seconds


def report(loop_file, still_file, seconds):
    probe = subprocess.run(
        ["ffprobe", "-v", "error", "-select_streams", "v:0", "-count_frames",
         "-show_entries", "stream=codec_name,profile,pix_fmt,width,height,r_frame_rate,"
         "nb_read_frames,has_b_frames,color_range,color_space,color_transfer,color_primaries",
         "-of", "json", loop_file], check=True, capture_output=True, text=True)
    stream = json.loads(probe.stdout)["streams"][0]
    size = os.path.getsize(loop_file)
    print(f"{loop_file}: {size / 1e6:.2f} MB, {seconds:.1f} s, "
          f"{size * 8 / seconds / 1e6:.2f} Mbit/s")
    for name in sorted(stream):
        print(f"    {name} = {stream[name]}")
    print(f"{still_file}: {os.path.getsize(still_file) / 1e6:.2f} MB")
    if size > MAX_BYTES:
        print(f"WARNING: the loop is larger than {MAX_BYTES / 1e6:.0f} MB. Raise QUALITY_CRF "
              "or shorten the shots.")


# ---- the command line ---------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description="Records the video loop of the main menu.")
    parser.add_argument("--exe", default=DEFAULT_EXE, help="the game to record")
    parser.add_argument("--work", help="folder for the recordings of the shots")
    parser.add_argument("--out-dir", default=DEFAULT_OUT_DIR, help="where the two files go")
    parser.add_argument("--cut-only", action="store_true",
                        help="do not record: cut the recordings that are in --work")
    parser.add_argument("--scout", choices=["walk", "glide"],
                        help="write single pictures of this shot instead of recording")
    parser.add_argument("--seed", type=int, default=1, help="scout: the seed of the maze")
    parser.add_argument("--difficulty", default="easy", choices=["easy", "normal", "hard"],
                        help="scout: the level")
    parser.add_argument("--from", dest="first", type=float, default=0.0,
                        help="scout: the first time offset in seconds")
    parser.add_argument("--to", dest="last", type=float, default=60.0,
                        help="scout: the last time offset in seconds")
    parser.add_argument("--step", type=float, default=10.0, help="scout: seconds between pictures")
    parser.add_argument("--out", help="scout: the folder for the pictures")
    arguments = parser.parse_args()

    if not os.path.isfile(arguments.exe):
        sys.exit(f"The game is not built: {arguments.exe} (run make release first)")
    use_real_pixels()

    if arguments.scout:
        if not arguments.out:
            sys.exit("--scout needs --out, the folder for the pictures")
        offsets = []
        offset = arguments.first
        while offset <= arguments.last:
            offsets.append(offset)
            offset += arguments.step
        shot = Shot("scout", arguments.seed, arguments.difficulty, arguments.scout, 0.0, 0.0)
        scout(arguments.exe, arguments.out, shot, offsets)
        return

    work = arguments.work or tempfile.mkdtemp(prefix="night-maze-menu-loop-")
    os.makedirs(work, exist_ok=True)
    if not arguments.cut_only:
        for shot in SHOTS:
            print(f"recording {shot.name}: seed {shot.seed}, {shot.difficulty}, {shot.shot}, "
                  f"from {shot.time} s, {shot.seconds} s")
            record(arguments.exe, work, shot)
    loop_file, still_file, seconds = cut_and_encode(work, SHOTS, arguments.out_dir)
    report(loop_file, still_file, seconds)
    print("the recordings of the shots are in", work)


if __name__ == "__main__":
    main()
