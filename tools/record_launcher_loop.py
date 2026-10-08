# Records the video loop behind the launcher window from the game itself and writes the
# two files the launcher ships: src/assets/menu-loop.mp4 and menu-poster.jpg in the launcher's
# own repository (Shironex/night-maze-launcher), expected in a folder next to this one.
#
# Run from the repository root, on Windows, after a Release build (make release):
#   python tools/record_launcher_loop.py
# It needs ffmpeg on PATH. It reuses the recording half of record_menu_loop.py (the game
# in menu camera mode, grabbed losslessly by window handle) and has its own cut.
#
# The loop is ONE shot: the high glide over the maze of seed 1 on the easy level. The
# glide is a circle, so after one round the camera is where it started. The file holds
# exactly one round, which is why the loop has no seam: nothing is dissolved into a
# different view. Only the last half second is crossfaded into the first frames of the
# NEXT round (the same view), which hides that the crystals bob at their own rhythm.
#
# The launcher downloads its whole installer on every update, so the file has to be
# small: the picture is cropped to the shape of the launcher window (16:10) and encoded
# far harder than the game's own loop. It sits behind a dark scrim, where that is not seen.
#
# While it records (about a minute) the game window is on the screen, on top of the other
# windows. The script takes the input lock of the agents (%TEMP%\nm-input.lock) meanwhile.
#
# Switches: --exe (the game to record), --work (where the recording is kept, default a new
# temporary folder), --cut-only (skip the recording and cut the one that is in --work).
import argparse
import os
import re
import subprocess
import sys
import tempfile
import time

import record_menu_loop as menu

# One round of the glide takes about 45.7 seconds on the easy level. The recording is longer:
# the exact length of the round is measured in the recording, and the crossfade needs the
# start of the second round.
SHOT = menu.Shot("launcher_glide", seed=1, difficulty="easy", shot="glide", time=0.0, seconds=49.0)

# Where in the recording the end of the round is looked for, in frames (38 to 48 seconds).
ROUND_SEARCH_FRAMES = (38 * menu.FRAMES_PER_SECOND, 48 * menu.FRAMES_PER_SECOND)

CROSSFADE_SECONDS = 0.5

# The launcher window is 1280 x 800 (tauri.conf.json): 16:10. The recording is 1280 x 720,
# so the sides are cut off first (CROP), which is what the window would do anyway, and
# the rest is made smaller (SIZE). The window stretches it again; behind the scrim the
# difference to the full size is not seen, and the file is a fifth smaller.
CROP = "1152:720"
SIZE = "960:600"

# The CRF of the x264 encoder. The game's loop uses 16; behind the scrim 29 is enough.
# Measured on the first recording: 1.95 MB (CRF 27 at 1152 x 720 was 3.31 MB).
QUALITY_CRF = 29

LOCK_FILE = os.path.join(tempfile.gettempdir(), "nm-input.lock")
# The launcher has its own repository since 2026-10-07. Its clone is expected beside this
# repository, under the name of the repository.
OUT_DIR = os.path.join(menu.REPOSITORY, "..", "night-maze-launcher", "src", "assets")
LOOP_FILE = os.path.join(OUT_DIR, "menu-loop.mp4")
POSTER_FILE = os.path.join(OUT_DIR, "menu-poster.jpg")


def record(exe, work):
    while os.path.exists(LOCK_FILE):
        print("waiting for", LOCK_FILE)
        time.sleep(5)
    with open(LOCK_FILE, "w", encoding="utf-8") as file:
        file.write("record_launcher_loop.py\n")
    try:
        menu.record(exe, work, SHOT)
    finally:
        os.remove(LOCK_FILE)


def round_frames(recording):
    # The frame of the recording that looks most like its first frame: there the camera
    # is back where it started. psnr prints one line per compared frame.
    first, last = ROUND_SEARCH_FRAMES
    graph = (f"[0:v]split[a][b];[a]trim=end_frame=1,loop=loop=-1:size=1,setpts=N/FRAME_RATE/TB[still];"
             f"[b]trim=start_frame={first}:end_frame={last},setpts=PTS-STARTPTS[late];"
             "[late][still]psnr=stats_file=-:shortest=1")
    done = subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", recording,
                           "-filter_complex", graph, "-f", "null", "-"],
                          check=True, capture_output=True, text=True)
    scores = [float(score) for score in re.findall(r"psnr_avg:([\d.]+)", done.stdout)]
    best = max(range(len(scores)), key=scores.__getitem__)
    print(f"the round ends at frame {first + best} (psnr {scores[best]:.1f} dB, "
          f"one frame earlier {scores[max(best - 1, 0)]:.1f} dB)")
    return first + best


def cut_and_encode(recording):
    os.makedirs(OUT_DIR, exist_ok=True)
    seconds = round_frames(recording) / menu.FRAMES_PER_SECOND
    fade = CROSSFADE_SECONDS
    # One round, starting half a second in, and its end fades into the first half second:
    # the same view one round later. The colour conversion and its tags are the ones of
    # record_menu_loop.py (BT.709, TV range), so every decoder shows the same colours.
    graph = (f"[0:v]fps={menu.FRAMES_PER_SECOND},format=gbrp,settb=AVTB,split[a][b];"
             f"[a]trim=duration={fade},setpts=PTS-STARTPTS[head];"
             f"[b]trim=start={fade}:end={seconds + fade},setpts=PTS-STARTPTS[body];"
             f"[body][head]xfade=transition=fade:duration={fade}:offset={seconds - fade},"
             f"crop={CROP},scale={SIZE}:flags=lanczos:"
             "out_color_matrix=bt709:out_range=tv,format=yuv420p,"
             "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:range=tv[out]")
    menu.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", recording,
              "-filter_complex", graph, "-map", "[out]", "-an",
              # H.264 is the one codec both webviews play: WebView2 and WKWebView.
              "-c:v", "libx264", "-profile:v", "high", "-pix_fmt", "yuv420p",
              "-preset", "veryslow", "-crf", str(QUALITY_CRF),
              # More of the bits go to the dark, flat sky, which bands first.
              "-x264-params", "aq-mode=3",
              "-movflags", "+faststart", LOOP_FILE])
    # The poster: the first frame of the finished loop, shown before and without the video.
    menu.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", LOOP_FILE,
              # A JPEG is read with the BT.601 matrix and the full range, whatever its tags say.
              "-frames:v", "1", "-vf", "scale=in_color_matrix=bt709:in_range=tv:"
              "out_color_matrix=bt601:out_range=pc,format=yuvj420p",
              "-q:v", "4", "-update", "1", POSTER_FILE])
    for path in (LOOP_FILE, POSTER_FILE):
        print(f"{path}: {os.path.getsize(path) / 1e6:.2f} MB")
    print(f"the loop is {seconds:.2f} s long")


def main():
    parser = argparse.ArgumentParser(description="Records the video loop of the launcher.")
    parser.add_argument("--exe", default=menu.DEFAULT_EXE, help="the game to record")
    parser.add_argument("--work", help="folder for the recording")
    parser.add_argument("--cut-only", action="store_true",
                        help="do not record: cut the recording that is in --work")
    arguments = parser.parse_args()

    work = arguments.work or tempfile.mkdtemp(prefix="night-maze-launcher-loop-")
    os.makedirs(work, exist_ok=True)
    if not arguments.cut_only:
        if not os.path.isfile(arguments.exe):
            sys.exit(f"The game is not built: {arguments.exe} (run make release first)")
        menu.use_real_pixels()
        record(arguments.exe, work)
    cut_and_encode(menu.recording_path(work, SHOT))
    print("the recording is in", work)


if __name__ == "__main__":
    main()
