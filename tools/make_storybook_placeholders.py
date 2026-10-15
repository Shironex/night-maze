# Makes the placeholder pictures of the storybook intro: three layers per beat in
# assets/intro (a sky, a far ridge and a dark foreground), so the storybook can be played
# and judged for its timing and motion before the painted illustrations exist.
#
# Run from the repository root, on any system:
#   python tools/make_storybook_placeholders.py
# Only the standard library is used (zlib and struct write the PNG files). Nothing is
# random: running it again writes the same bytes. The painted layers replace these files
# under the same names (docs/story/intro-storyboard.md says how), and then this script
# is not run again.
#
# Each sky carries the number of its beat, faint, in the top right corner, so a
# screenshot says which beat it shows. Each beat has a sky of its own colour, so
# a crossfade is easy to see.

import math
import os
import struct
import zlib

OUT_DIR = os.path.join("assets", "intro")
WIDTH = 960
HEIGHT = 540
BEAT_COUNT = 6

# The top and bottom colour of the sky of each beat: muted night colours.
SKIES = [
    ((14, 18, 44), (58, 62, 104)),
    ((20, 16, 40), (78, 58, 96)),
    ((10, 24, 40), (46, 78, 96)),
    ((24, 20, 34), (96, 80, 92)),
    ((8, 10, 24), (34, 40, 70)),
    ((18, 20, 46), (92, 76, 70)),
]
RIDGE = (24, 26, 48)
FOREGROUND = (6, 6, 12)

# A 3 x 5 font for the digits 1 to 9, one string of 15 pixels per digit.
DIGITS = {
    1: "010110010010111", 2: "111001111100111", 3: "111001111001111",
    4: "101101111001001", 5: "111100111001111", 6: "111100111101111",
    7: "111001001001001", 8: "111101111101111", 9: "111101111001111",
}
DIGIT_PIXEL = 12


def png(path, rows):
    # rows: HEIGHT lists of bytes, RGBA, top row first.
    raw = b"".join(b"\x00" + bytes(row) for row in rows)

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    header = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 6, 0, 0, 0)
    with open(path, "wb") as file:
        file.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) +
                   chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    print("wrote", path)


def mix(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def sky(beat):
    top, bottom = SKIES[beat - 1]
    digit = DIGITS[beat]
    left = WIDTH - 10 * DIGIT_PIXEL
    moon_x, moon_y, moon_r = WIDTH * (0.3 + 0.08 * beat), HEIGHT * 0.28, HEIGHT * 0.07
    rows = []
    for y in range(HEIGHT):
        colour = mix(top, bottom, y / (HEIGHT - 1))
        row = bytearray()
        for x in range(WIDTH):
            r, g, b = colour
            # A soft moon with a glow.
            d = math.hypot(x - moon_x, y - moon_y) / moon_r
            light = 1.0 if d < 1.0 else 0.35 * math.exp(-(d - 1.0) * 1.5)
            r, g, b = (min(255, round(c + (230 - c) * light)) for c in (r, g, b))
            # The number of the beat, faint.
            cx, cy = (x - left) // DIGIT_PIXEL, (y - 5 * DIGIT_PIXEL) // DIGIT_PIXEL
            if 0 <= cx < 3 and 0 <= cy < 5 and digit[cy * 3 + cx] == "1":
                r, g, b = (min(255, c + 40) for c in (r, g, b))
            row += bytes((r, g, b, 255))
        rows.append(row)
    return rows


def silhouette(height_at, colour):
    # A shape filled from the line height_at(x) down to the bottom, with a soft edge.
    rows = []
    for y in range(HEIGHT):
        row = bytearray()
        for x in range(WIDTH):
            alpha = max(0.0, min(1.0, (y - height_at(x)) / 2.0 + 0.5))
            row += bytes((*colour, round(255 * alpha)))
        rows.append(row)
    return rows


def ridge(beat):
    phase = beat * 1.7
    return silhouette(lambda x: HEIGHT * (0.62 + 0.06 * math.sin(x / 140 + phase) +
                                          0.03 * math.sin(x / 47 + 2 * phase)), RIDGE)


def foreground(beat):
    # Two dark hedges that frame the picture, lower in the middle.
    phase = beat * 0.9

    def height_at(x):
        side = abs(x - WIDTH / 2) / (WIDTH / 2)
        return HEIGHT * (0.98 - 0.45 * side ** 2.5 + 0.015 * math.sin(x / 9 + phase))

    return silhouette(height_at, FOREGROUND)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for beat in range(1, BEAT_COUNT + 1):
        png(os.path.join(OUT_DIR, f"beat{beat}_sky.png"), sky(beat))
        png(os.path.join(OUT_DIR, f"beat{beat}_ridge.png"), ridge(beat))
        png(os.path.join(OUT_DIR, f"beat{beat}_front.png"), foreground(beat))


if __name__ == "__main__":
    main()
