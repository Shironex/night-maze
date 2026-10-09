# Makes sketches of a theme for the main menu: three short, slow pieces for piano, each
# with a second voice (soft strings, and a music box that doubles the melody in the last
# third). They are SKETCHES for listening and choosing. Nothing here is shipped with the
# game yet: the engine has no music group and reads only WAV files.
#
# The notes are written down in this file (SKETCHES below), by hand, as an own piece:
# no melody of any existing piece is quoted. The script turns the notes into sound in
# one of two ways, and the same notes go through both, so the two can be compared:
#
#   --route synth     Everything is computed, like the sounds of tools/make_sounds.py:
#                     a soft piano from sine waves (see piano_note), strings and a music
#                     box likewise. Needs nothing but numpy. Nobody else holds a right in
#                     the result.
#   --route sampled   The piano is played by FluidSynth from recordings of a real upright
#                     piano, the strings and the bells from recordings of real ones. The
#                     recordings are not in this repository. They are public domain (CC0):
#                       - FluidSynth 2.6.1 (the player, LGPL, only run, not shipped):
#                         https://github.com/FluidSynth/fluidsynth/releases/tag/v2.6.1
#                       - FreePats "Upright Piano KW", SF2, version 2022-02-21, CC0:
#                         https://freepats.zenvoid.org/Piano/acoustic-grand-piano.html
#                       - Versilian Studios VSCO 2 Community Edition, CC0 (viola section
#                         "susvib" and the glockenspiel, which stands in for a celesta:
#                         the set has none): https://github.com/sgossner/VSCO-2-CE
#                     --tools DIR is the folder that holds them (see find_tools for the
#                     names it looks for).
#
# Both routes end in the same room: the reverb of tools/make_sounds.py (room_wash), made
# far larger, because this music lives on its long echo.
#
# Run it with a Python that has numpy (the sounds of the game need none, this does):
#   python tools/make_music.py --out-dir some/folder
#   python tools/make_music.py --out-dir some/folder --route sampled --tools some/tools
# The same arguments write the same bytes every time: every random choice (the small
# unevenness of a hand, the noise of a hammer) comes from a generator with a fixed seed.
#
# Every piece is a LOOP: what still rings when the piece is over (the last notes and the
# room) is added to its beginning, so the file can be played round and round.
import argparse
import glob
import hashlib
import math
import os
import random
import re
import struct
import subprocess
import sys
import tempfile
import wave

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_sounds  # noqa: E402  (the room is shared with the sounds of the game)

RATE = make_sounds.SAMPLE_RATE

# How long the room of the music takes to die away, and how much of each voice is sent
# into it. The sounds of the game use a room of 1.2 s.
MUSIC_ROOM_SECONDS = 3.6
ROOM_SEND = {"piano": 0.5, "strings": 0.75, "bells": 0.8}
# The loudest sample of each voice before the mix: the piano leads, the strings lie far
# under it, the bells between.
VOICE_PEAK = {"piano": 1.0, "strings": 0.13, "bells": 0.26}
# The loudest sample of a finished file, in dBFS.
PEAK_DB = -3.0

# ---- the notes -------------------------------------------------------------------------------

# How a voice is written. Notes are separated by spaces: NAME:BEATS, for example A4:1.5.
# NAME+NAME is a chord, "-" is a rest. "|" ends a bar, and the two hands of the piano
# must fill every bar exactly (the script checks). The strings and the bells are written
# as one long row without bars.
#
# The piano is played with the pedal down, changed at every bar line: a note rings until
# its bar ends, whatever its written length. That is how such music is played, and it is
# why the left hand can spread a chord over a bar (the root, its fifth, its tenth) and
# the three sound together at the end.
#
# "slow" stretches single bars (bar number: factor), the breath at the end of a phrase.
SKETCHES = {
    # A minor, 3/4. The motif: up a fourth, down a step, down a third (E A G E), then
    # one long note. It is stated, answered higher, and comes back in the last third
    # with the music box an octave above it and the strings under it.
    "sketch1_a-minor_60bpm": {
        "seed": 31, "bpm": 60.0, "beats": 3, "slow": {8: 1.06, 16: 1.06, 23: 1.04, 24: 1.08},
        "right": """-:1 E5:1 A5:1 | G5:2 E5:1 | F5:3 | -:3 | -:1 E5:1 G5:1 | E5:2 D5:1 |
                    B4:3 | -:3 |
                    -:1 F5:1 A5:1 | D6:2 C6:1 | A5:2 E5:1 | -:3 | -:1 A5:1 C6:1 | A5:2 G5:1 |
                    A5:3 | G5:2 -:1 |
                    -:1 E5:1 A5:1 | G5:2 E5:1 | F5:2 A5:1 | C6:3 | -:1 G5:1 E5:1 | D5:2 B4:1 |
                    A4:3 | -:3""",
        "left": """A2:1 E3:1 C4:1 | A2+E3:3 | F2:1 C3:1 A3:1 | F2+C3:3 | C3:1 G3:1 E4:1 | C3+G3:3 |
                   E2:1 B2:1 G3:1 | E2+B2:3 |
                   D3:1 A3:1 F4:1 | D3+A3:3 | A2:1 E3:1 C4:1 | A2+E3:3 | F2:1 C3:1 A3:1 | F2+C3:3 |
                   E2:1 B2:1 A3:1 | E2:1 B2:1 G3:1 |
                   A2:1 E3:1 C4:1 | A2+E3:3 | F2:1 C3:1 A3:1 | F2+C3:3 | C3:1 G3:1 E4:1 |
                   G2:1 D3:1 B3:1 | A2:1 E3:1 C4:1 | A2+E3:3""",
        "strings": "-:48 A3+E4:6 A3+F4:6 G3+E4:3 G3+D4:3 A3+C4:5 -:1",
        "bells": "-:48 -:1 E6:1 A6:1 G6:2 E6:1 F6:2 A6:1 C7:3 -:1 G6:1 E6:1 D6:2 B5:1 A5:3 -:3",
    },
    # D minor, free time: no bar to count, only cells of six slow beats, each a chord
    # that the left hand rolls upward and leaves to ring. The motif falls: one short
    # note, then two long ones (A, F, D). The strings come in after the first third and
    # stay, the music box joins for the last third.
    "sketch2_d-minor_54bpm_free": {
        "seed": 32, "bpm": 54.0, "beats": 6, "slow": {4: 1.08, 8: 1.08, 11: 1.04, 12: 1.1},
        "right": """-:1.5 A5:0.5 F5:2 D5:2 | -:1.5 G5:0.5 D5:2 Bb4:2 | -:1.5 A5:0.5 F5:2 C5:2 |
                    E5:4 -:2 |
                    -:1.5 Bb5:0.5 G5:2 D5:2 | -:1.5 A5:0.5 F5:2 D5:2 | -:1.5 F5:0.5 D5:2 Bb4:2 |
                    C5:3 -:3 |
                    -:1.5 A5:0.5 F5:2 D5:2 | -:1.5 G5:0.5 D5:2 Bb4:2 | -:1.5 Bb5:0.5 A5:2 E5:2 |
                    D5:3 -:3""",
        "left": """D2:0.5 A2:0.5 F3:5 | Bb2:0.5 F3:0.5 D4:5 | F2:0.5 C3:0.5 A3:5 |
                   A2:0.5 E3:0.5 C4:5 |
                   G2:0.5 D3:0.5 Bb3:5 | F2:0.5 D3:0.5 A3:5 | Bb2:0.5 F3:0.5 D4:5 |
                   C3:0.5 G3:0.5 E4:5 |
                   D2:0.5 A2:0.5 F3:5 | Bb2:0.5 F3:0.5 D4:5 | A2:0.5 E3:0.5 C#4:5 |
                   D2+A2:6""",
        "strings": "-:24 G3+D4:6 F3+D4:12 G3+E4:6 A3+F4:6 Bb3+F4:6 A3+E4:6 A3+D4:4 -:2",
        "bells": """-:48 -:1.5 A6:0.5 F6:2 D6:2 -:1.5 G6:0.5 D6:2 Bb5:2 -:1.5 Bb6:0.5 A6:2 E6:2
                    D6:3 -:3""",
    },
    # B minor, 3/4, a little faster: a lullaby. The motif rocks on one note (F sharp,
    # the G above it, F sharp again) and then falls a third and a third. The left hand
    # plays tenths. In the last third the music box plays the lullaby along, as if it
    # were where the tune came from.
    "sketch3_b-minor_66bpm": {
        "seed": 33, "bpm": 66.0, "beats": 3,
        "slow": {8: 1.05, 16: 1.06, 24: 1.05, 27: 1.05, 28: 1.1},
        "right": """F#5:1.5 G5:0.5 F#5:1 | D5:2 B4:1 | E5:1.5 F#5:0.5 E5:1 | B4:3 | -:1 A4:1 D5:1 |
                    E5:2 C#5:1 | D5:3 | -:3 |
                    G5:1.5 A5:0.5 G5:1 | E5:2 B4:1 | F#5:1.5 G5:0.5 F#5:1 | D5:3 | -:1 B4:1 D5:1 |
                    F#5:2 A5:1 | B5:3 | A#5:2 -:1 |
                    F#5:1.5 G5:0.5 F#5:1 | D5:2 B4:1 | E5:1.5 F#5:0.5 E5:1 | B4:3 | -:1 A4:1 D5:1 |
                    E5:2 A5:1 | G5:3 | F#5:2 -:1 |
                    -:1 D5:1 G5:1 | F#5:2 D5:1 | B4:3 | -:3""",
        "left": """B2+D4:1 F#3:2 | B2+F#3:3 | G2+B3:1 D3:2 | G2+D3:3 | D3+F#4:1 A3:2 |
                   A2+C#4:1 E3:2 | B2+D4:1 F#3:2 | B2+F#3:3 |
                   E2+G3:1 B2:2 | E2+B2:3 | B2+D4:1 F#3:2 | B2+F#3:3 | G2+B3:1 D3:2 |
                   F#2+A3:1 D3:2 | F#2+B3:1 C#3:2 | F#2+A#3:1 C#3:2 |
                   B2+D4:1 F#3:2 | B2+F#3:3 | G2+B3:1 D3:2 | G2+D3:3 | D3+F#4:1 A3:2 |
                   A2+C#4:1 E3:2 | E2+G3:1 B2:2 | F#2+A3:1 C#3:2 |
                   G2+B3:1 D3:2 | G2+D3:3 | B2+D4:1 F#3:2 | B2+F#3:3""",
        "strings": "-:48 B3+F#4:6 B3+D4:6 A3+F#4:3 A3+E4:3 G3+E4:3 F#3+C#4:3 G3+D4:6 F#3+D4:5 -:1",
        "bells": """-:48 F#6:1.5 G6:0.5 F#6:1 D6:2 B5:1 E6:1.5 F#6:0.5 E6:1 B5:3 -:1 A5:1 D6:1
                    E6:2 A6:1 G6:3 F#6:2 -:1 -:1 D6:1 G6:1 F#6:2 D6:1 B5:3 -:3""",
    },
}

NOTE_STEPS = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def midi_of(name):
    # The number of a note as MIDI counts them: A4 (440 Hz) is 69, every half step is 1.
    match = re.fullmatch(r"([A-G])([#b]?)(-?\d)", name)
    if not match:
        raise ValueError(f"not a note: {name}")
    letter, sign, octave = match.groups()
    return 12 * (int(octave) + 1) + NOTE_STEPS[letter] + {"#": 1, "b": -1, "": 0}[sign]


def frequency_of(midi):
    return 440.0 * 2.0 ** ((midi - 69) / 12.0)


def read_row(text):
    # A row of notes as (beat it starts on, [MIDI numbers], beats long), and its length.
    notes = []
    beat = 0.0
    for token in text.replace("|", " ").split():
        what, _, long = token.partition(":")
        if what != "-":
            notes.append((beat, [midi_of(name) for name in what.split("+")], float(long)))
        beat += float(long)
    return notes, beat


def events_of(sketch):
    # The notes of a sketch with their times in seconds: per voice a list of
    # (start, MIDI number, seconds it sounds, strength from 0 to 1), and the length of
    # the whole piece.
    beats = sketch["beats"]
    bars = [bar for bar in sketch["left"].split("|")]
    for hand in ("left", "right"):
        rows = sketch[hand].split("|")
        if len(rows) != len(bars):
            raise ValueError(f"{hand}: {len(rows)} bars, expected {len(bars)}")
        for number, bar in enumerate(rows, 1):
            if abs(read_row(bar)[1] - beats) > 1e-9:
                raise ValueError(f"{hand}, bar {number}: not {beats} beats: {bar.strip()}")
    bar_seconds = [beats * 60.0 / sketch["bpm"] * sketch["slow"].get(number, 1.0)
                   for number in range(1, len(bars) + 1)]
    bar_starts = [sum(bar_seconds[:number]) for number in range(len(bars) + 1)]
    total = bar_starts[-1]

    def seconds_at(beat):
        bar = min(int(beat // beats), len(bars) - 1)
        return bar_starts[bar] + (beat - bar * beats) / beats * bar_seconds[bar]

    # A hand is never even: every note comes a few milliseconds early or late and a
    # little softer or louder. The upper notes of a chord follow the lowest one, as
    # a hand rolls them. Always the same unevenness: the generator has a fixed seed.
    hand_of = random.Random(sketch["seed"])
    events = {"piano": [], "strings": [], "bells": []}
    for hand, strength in (("left", 0.4), ("right", 0.56)):
        for beat, chord, _ in read_row(sketch[hand])[0]:
            bar = min(int(beat // beats), len(bars) - 1)
            start = seconds_at(beat) + hand_of.uniform(0.0, 0.014)
            # The first beat of a bar leans a little, an upbeat is lighter.
            lean = 0.04 if beat % beats == 0 else -0.02 if beat % 1 else 0.0
            for place_in, midi in enumerate(sorted(chord)):
                at = start + 0.028 * place_in
                # The pedal: the note rings to the end of its bar and a moment beyond.
                events["piano"].append((at, midi, bar_starts[bar + 1] + 0.2 - at,
                                        strength + lean + hand_of.uniform(-0.05, 0.05)))
    for voice in ("strings", "bells"):
        notes, length = read_row(sketch[voice])
        if abs(length - beats * len(bars)) > 1e-9:
            raise ValueError(f"{voice}: {length} beats, expected {beats * len(bars)}")
        for beat, chord, long in notes:
            start = seconds_at(beat) + hand_of.uniform(0.0, 0.01)
            for midi in chord:
                events[voice].append((start, midi, seconds_at(beat + long) - seconds_at(beat),
                                      0.5 + hand_of.uniform(-0.05, 0.05)))
    return events, total


# ---- laying notes into a loop ----------------------------------------------------------------


def lay(track, start_seconds, note):
    # Adds a stereo note into a track, starting at a moment of it. What reaches past
    # the end of the track goes on at its beginning: the track is a loop.
    length = track.shape[1]
    start = int(round(start_seconds * RATE)) % length
    for offset in range(0, note.shape[1], length):
        piece = note[:, offset:offset + length]
        first = min(piece.shape[1], length - start)
        track[:, start:start + first] += piece[:, :first]
        track[:, :piece.shape[1] - first] += piece[:, first:]


def fade(count, rise_seconds, hold_seconds, fall_seconds):
    # 0 to 1 over rise_seconds (half a cosine), 1 until hold_seconds, then down to 0 over
    # fall_seconds, as a curve of count samples.
    t = np.arange(count) / RATE
    up = 0.5 - 0.5 * np.cos(np.pi * np.clip(t / max(rise_seconds, 1e-6), 0.0, 1.0))
    down = 0.5 + 0.5 * np.cos(np.pi * np.clip((t - hold_seconds) / fall_seconds, 0.0, 1.0))
    return up * down


# ---- route "synth": every voice is computed --------------------------------------------------


def piano_note(midi, seconds, strength, generator):
    # One note of a soft piano with a felt-covered hammer, stereo.
    #
    #   - A string does not ring at whole multiples of its lowest frequency. It is
    #     stiff, so every partial lies a little higher than its multiple, the more the
    #     higher it is (INHARMONICITY: partial k at k * f * sqrt(1 + B * k * k)). That
    #     slight stretch is what makes a piano sound like a piano and not like an organ.
    #   - The hammer strikes at an eighth of the string, which weakens every eighth
    #     partial, and its felt is soft: the harder the strike, the more high partials.
    #     A soft note is nearly a sine wave with a little colour.
    #   - Every partial dies away in two stages: fast at first (the strike), then slowly
    #     (the long after-sound). High partials die first, and low notes ring longest.
    #   - A note has two or three strings, never tuned exactly alike. Here every partial
    #     is two sine waves a hair apart, other ones on the left than on the right: the
    #     note beats slowly and has width.
    #   - The wooden body of a piano is too small to carry its lowest tones: below
    #     200 Hz a partial is the weaker the lower it lies. The ear still hears the low
    #     note, from the spacing of the partials above it. (Measured on the recorded
    #     upright of the other route, which the first version of this note was far
    #     heavier than.)
    #   - The hammer itself: a thump of dull noise for a few hundredths of a second.
    #   - When the pedal lets go (after seconds), the damper stops the string within
    #     a fifth of a second.
    frequency = frequency_of(midi)
    count = int((seconds + 0.5) * RATE)
    t = np.arange(count) / RATE
    stiffness = min(0.00032 * (frequency / 261.6) ** 1.25, 0.012)
    ring = min(max(5.5 * (261.6 / frequency) ** 0.6, 0.9), 9.0)
    felt = 900.0 + 2200.0 * strength
    note = np.zeros((2, count))
    for k in range(1, 40):
        partial = k * frequency * math.sqrt(1.0 + stiffness * k * k)
        if partial > 9000.0:
            break
        level = (k ** -(2.0 - 0.6 * strength) * (0.15 + abs(math.sin(math.pi * k * 0.122)))
                 * math.exp(-(partial / felt) ** 2) * min(1.0, (partial / 200.0) ** 1.5))
        if level < 2e-4:
            continue
        slow = ring / (1.0 + 0.11 * k ** 1.25)
        dying = 0.72 * np.exp(-t / (0.3 * slow)) + 0.28 * np.exp(-t / slow)
        for side in range(2):
            apart = generator.uniform(0.0003, 0.0007) * (1.0 if side else -1.0)
            pair = (np.sin(2.0 * np.pi * partial * (1.0 + apart) * t + generator.uniform(0.0, 0.6))
                    + 0.7 * np.sin(2.0 * np.pi * partial * (1.0 - 0.6 * apart) * t
                                   + generator.uniform(0.0, 0.6)))
            note[side] += level * dying * pair
    thump_count = min(count, int(0.08 * RATE))
    thump = np.array([generator.uniform(-1.0, 1.0) for _ in range(thump_count)])
    spectrum = np.fft.rfft(thump)
    band = np.fft.rfftfreq(thump_count, 1.0 / RATE)
    spectrum *= np.exp(-((band - 380.0) / 300.0) ** 2)
    thump = np.fft.irfft(spectrum, thump_count) * np.exp(-np.arange(thump_count) / (0.012 * RATE))
    note[:, :thump_count] += 0.5 * (0.3 + strength) * thump / (np.abs(thump).max() + 1e-12) * 0.12
    note *= fade(count, 0.004 + 0.006 * (1.0 - strength), seconds, 0.45)
    return note * strength ** 1.4


def open_strings(track, key_notes):
    # With the pedal down every string of a piano is free, and the ones that belong to
    # the notes being played ring along a little (SYMPATHETIC RESONANCE): a faint halo
    # around the music. Here: the track is passed through one very narrow resonance per
    # string of the key (every note of the scale from C2 to C6), and a little of what
    # comes out is added. Done on the spectrum of the whole track, which treats the track
    # as a loop, as it is.
    count = track.shape[1]
    band = np.fft.rfftfreq(count, 1.0 / RATE)[1:]
    answer = np.zeros(len(band), dtype=complex)
    for midi in range(36, 85):
        if midi % 12 in key_notes:
            centre = frequency_of(midi)
            sharpness = math.pi * centre * 1.4
            answer += 1.0 / (1.0 + 1j * sharpness * (band / centre - centre / band))
    result = np.empty_like(track)
    for side in range(2):
        spectrum = np.fft.rfft(track[side])
        spectrum[1:] *= 1.0 + 0.05 * answer
        result[side] = np.fft.irfft(spectrum, count)
    return result


def string_note(midi, seconds, strength, generator):
    # A soft section of strings holding one note: six players, each a little off the
    # pitch (up to 7 hundredths of a half step), each with a slow vibrato of his own
    # that starts late, each at his own place between left and right. Dark: the partials
    # fall off fast above 2 kHz. It swells in over most of a second and dies away over
    # a second and a half.
    frequency = frequency_of(midi)
    count = int((seconds + 1.6) * RATE)
    t = np.arange(count) / RATE
    note = np.zeros((2, count))
    for player in range(6):
        off = 2.0 ** (generator.uniform(-7.0, 7.0) / 1200.0)
        speed = generator.uniform(4.6, 5.8)
        depth = 0.0028 * np.clip((t - 0.5) / 1.2, 0.0, 1.0)
        bend = 1.0 + depth * np.sin(2.0 * np.pi * speed * t + generator.uniform(0.0, 6.28))
        phase = 2.0 * np.pi * np.cumsum(frequency * off * bend) / RATE
        voice = np.zeros(count)
        for k in range(1, 14):
            if k * frequency > 5000.0:
                break
            level = 1.0 / k / (1.0 + (k * frequency / 2000.0) ** 2)
            voice += level * np.sin(k * phase + generator.uniform(0.0, 6.28))
        voice *= 1.0 + 0.12 * np.sin(2.0 * np.pi * generator.uniform(0.15, 0.4) * t
                                     + generator.uniform(0.0, 6.28))
        side = player / 5.0
        note[0] += voice * math.cos(0.5 * math.pi * (0.2 + 0.6 * side))
        note[1] += voice * math.sin(0.5 * math.pi * (0.2 + 0.6 * side))
    return note * fade(count, 0.9, seconds, 1.5) * strength


def bell_note(midi, seconds, strength, generator):
    # One note of a music box (or a celesta: both are a struck piece of metal): the note
    # itself, a weak octave, and the high inharmonic partial of a metal tongue at 6.27
    # times the note, which is gone at once and is heard as the "ting". It rings on after
    # its written length: nothing damps a music box.
    frequency = frequency_of(midi)
    ring = min(max(1.5 * (880.0 / frequency) ** 0.5, 0.5), 2.4)
    count = int((seconds + 2.5 * ring) * RATE)
    t = np.arange(count) / RATE
    note = np.zeros((2, count))
    for ratio, level, lasting in ((1.0, 1.0, 1.0), (2.0, 0.14, 0.45), (6.27, 0.08, 0.07)):
        if frequency * ratio > 12000.0:
            continue
        for side in range(2):
            apart = 1.0 + (0.0004 if side else -0.0004)
            note[side] += level * np.exp(-t / (ring * lasting)) * np.sin(
                2.0 * np.pi * frequency * ratio * apart * t)
    lean = generator.uniform(0.35, 0.65)
    note[0] *= math.cos(0.5 * math.pi * lean)
    note[1] *= math.sin(0.5 * math.pi * lean)
    return note * fade(count, 0.002, count / RATE - 0.3, 0.3) * strength


def synth_voices(events, total, key_notes, seed):
    generator = random.Random(seed)
    count = int(round(total * RATE))
    voices = {}
    for voice, make in (("piano", piano_note), ("strings", string_note), ("bells", bell_note)):
        track = np.zeros((2, count))
        for start, midi, seconds, strength in events[voice]:
            lay(track, start, make(midi, seconds, strength, generator))
        voices[voice] = track
    voices["piano"] = open_strings(voices["piano"], key_notes)
    return voices


# ---- route "sampled": recordings of real instruments -----------------------------------------


def find_tools(tools):
    # The three things the sampled route needs, somewhere under the folder tools.
    def one(pattern, what):
        found = sorted(glob.glob(os.path.join(tools, "**", pattern), recursive=True))
        if not found:
            raise SystemExit(f"--route sampled: no {what} ({pattern}) under {tools}")
        return found[0]

    return {"fluidsynth": one("fluidsynth.exe" if os.name == "nt" else "fluidsynth", "FluidSynth"),
            "piano": one("UprightPianoKW-*.sf2", "FreePats Upright Piano KW sound bank"),
            "strings": os.path.dirname(one("ViolaEns_susvib_*_v1_1.wav",
                                           "VSCO 2 CE viola section")),
            "bells": os.path.dirname(one("glock_medium_*.wav", "VSCO 2 CE glockenspiel"))}


def read_sound(path):
    # A WAV file (16 or 24 bit) as numbers between -1 and 1, one row per channel.
    with wave.open(path, "rb") as file:
        channels, width, rate = file.getnchannels(), file.getsampwidth(), file.getframerate()
        raw = file.readframes(file.getnframes())
    if width == 2:
        data = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    elif width == 3:
        bytes_of = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        whole = bytes_of[:, 0] | (bytes_of[:, 1] << 8) | (bytes_of[:, 2] << 16)
        data = (whole - ((whole & 0x800000) << 1)).astype(np.float64) / 8388608.0
    else:
        raise ValueError(f"{path}: {8 * width} bit")
    data = data.reshape(-1, channels).T
    return (np.vstack([data[0], data[-1]]), rate)


def write_midi(path, notes, total):
    # A MIDI file of one track: the notes as "key down" and "key up" messages, each with
    # the time since the message before it. 1920 ticks are one second (960 ticks per
    # beat at 120 beats per minute, the tempo a MIDI file has when it names none).
    #
    # A key that is struck again while it still rings (the pedal holds a note a moment
    # into the next bar) gets its "key up" at the new strike: otherwise the late "key up"
    # of the old note would silence the new one.
    messages = []
    notes = sorted(notes, key=lambda note: (note[1], note[0]))
    for index, (start, midi, seconds, strength) in enumerate(notes):
        end = start + seconds
        if index + 1 < len(notes) and notes[index + 1][1] == midi:
            end = min(end, notes[index + 1][0])
        messages.append((int(round(start * 1920)), 1, 0x90, midi, int(round(22 + 78 * strength))))
        messages.append((int(round(end * 1920)), 0, 0x80, midi, 0))
    # One message that changes nothing, ten seconds after the piece: the player goes on
    # until then, so the last notes have time to die away.
    messages.append((int(round((total + 10.0) * 1920)), 2, 0xB0, 7, 100))
    track = bytearray(b"\x00\xC0\x00\x00\xB0\x07\x64")
    now = 0
    for tick, _, status, first, second in sorted(messages):
        delta, now = tick - now, tick
        stack = [delta & 0x7F]
        while delta > 0x7F:
            delta >>= 7
            stack.append(0x80 | (delta & 0x7F))
        track += bytes(reversed(stack)) + bytes((status, first, second))
    track += b"\x00\xFF\x2F\x00"
    with open(path, "wb") as file:
        file.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, 960)
                   + b"MTrk" + struct.pack(">I", len(track)) + bytes(track))


def played_piano(notes, total, tools):
    # The piano of the sampled route: FluidSynth plays the notes from the sound bank
    # into a file, with its own reverb and chorus switched off (the room is added later,
    # the same one for both routes).
    with tempfile.TemporaryDirectory() as folder:
        midi_path = os.path.join(folder, "piano.mid")
        wav_path = os.path.join(folder, "piano.wav")
        write_midi(midi_path, notes, total)
        subprocess.run([tools["fluidsynth"], "-ni", "-q", "-g", "0.7", "-R", "0", "-C", "0",
                        "-r", str(RATE), "-O", "s16", "-T", "wav", "-F", wav_path,
                        tools["piano"], midi_path], check=True, stdout=subprocess.DEVNULL)
        sound, rate = read_sound(wav_path)
    if rate != RATE:
        raise ValueError(f"FluidSynth wrote {rate} samples per second")
    track = np.zeros((2, int(round(total * RATE))))
    lay(track, 0.0, sound)
    return track


def pitch_of(sound, rate, at_seconds):
    # The lowest frequency a recording repeats at, measured on a third of a second of
    # it: the shortest shift at which the sound matches itself (AUTOCORRELATION).
    size = 16384
    start = min(int(at_seconds * rate), max(sound.shape[1] - size, 0))
    piece = sound.mean(axis=0)[start:start + size]
    piece = (piece - piece.mean()) * np.hanning(len(piece))
    match = np.fft.irfft(np.abs(np.fft.rfft(piece, 2 * size)) ** 2)[:size]
    first, last = int(rate / 4500.0), int(rate / 45.0)
    best = match[first:last].max()
    for shift in range(first + 1, last - 1):
        if match[shift] >= 0.9 * best and match[shift] >= match[shift - 1] \
                and match[shift] >= match[shift + 1]:
            return rate / shift
    return rate / (first + int(np.argmax(match[first:last])))


def read_bank(folder, pattern, at_seconds, steady):
    # The recordings of one instrument: (MIDI number, sound, sample rate) per file. The
    # file names say which note it is but count the octaves in their own way, so the
    # octave is taken from the measured pitch and only the name of the note from the
    # file name. steady: the recording is a held note, and its loudness is taken from
    # its middle (otherwise from its loudest sample).
    bank = []
    for path in sorted(glob.glob(os.path.join(folder, pattern))):
        named = re.search(r"_([A-G]#?)-?\d(_|\.)", os.path.basename(path))
        sound, rate = read_sound(path)
        measured = 69.0 + 12.0 * math.log2(pitch_of(sound, rate, at_seconds) / 440.0)
        step = NOTE_STEPS[named.group(1)[0]] + (1 if "#" in named.group(1) else 0)
        midi = step + 12 * round((measured - step) / 12.0)
        if abs(midi - measured) > 1.0:
            print(f"  {os.path.basename(path)}: measured {measured:.1f}, taken as {midi}")
        if steady:
            middle = sound[:, int(0.5 * rate):int(2.5 * rate)]
            sound = sound / math.sqrt(float((middle ** 2).mean()))
        else:
            sound = sound / np.abs(sound).max()
        bank.append((midi, sound, rate))
    if not bank:
        raise SystemExit(f"no recordings match {pattern} in {folder}")
    return bank


def recorded(bank, midi, skip_seconds=0.0):
    # The recording nearest to a note, played faster or slower so that it IS the note
    # (a tape that runs faster sounds higher), and brought to the sample rate here.
    # skip_seconds leaves out its beginning.
    nearest, sound, rate = min(bank, key=lambda entry: (abs(entry[0] - midi), entry[0]))
    speed = 2.0 ** ((midi - nearest) / 12.0) * rate / RATE
    places = np.arange(int(skip_seconds * rate), sound.shape[1] - 1, speed)
    return np.vstack([np.interp(places, np.arange(sound.shape[1]), channel) for channel in sound])


def bowed_note(bank, midi, seconds, strength):
    # A held note of the recorded string section. A recording lasts six to nine
    # seconds. A longer note is made of several bows: before one recording runs out
    # the next one fades in under it (without its first 0.8 s, the start of the bow),
    # over a second and a half.
    wanted = int((seconds + 1.6) * RATE)
    cross = int(1.5 * RATE)
    note = np.zeros((2, wanted))
    at = 0
    first = True
    while at < wanted:
        bow = recorded(bank, midi, 0.0 if first else 0.8)
        bow = bow[:, :max(bow.shape[1] - int(0.4 * RATE), cross * 2)]
        long = min(bow.shape[1], wanted - at)
        shape = np.ones(long)
        if not first:
            shape[:cross] = np.sin(0.5 * np.pi * np.arange(cross) / cross)[:long]
        last = at + long >= wanted
        if not last:
            shape[-cross:] *= np.cos(0.5 * np.pi * np.arange(cross) / cross)
        note[:, at:at + long] += bow[:, :long] * shape
        if last:
            break
        at += long - cross
        first = False
    return note * fade(wanted, 0.7, seconds, 1.5) * strength


def sampled_voices(events, total, tools):
    count = int(round(total * RATE))
    strings = read_bank(tools["strings"], "ViolaEns_susvib_*_v1_1.wav", 1.0, True)
    bells = read_bank(tools["bells"], "glock_medium_*.wav", 0.05, False)
    voices = {"piano": played_piano(events["piano"], total, tools),
              "strings": np.zeros((2, count)), "bells": np.zeros((2, count))}
    for start, midi, seconds, strength in events["strings"]:
        lay(voices["strings"], start, bowed_note(strings, midi, seconds, strength))
    for start, midi, seconds, strength in events["bells"]:
        # A glockenspiel is far brighter than a music box or a celesta: most of its
        # sound lies above 4 kHz. Each note is made duller here (half as strong at
        # 2.5 kHz, a fifth at 5 kHz), which is most of the way from the one to the other.
        note = recorded(bells, midi)
        spectrum = np.fft.rfft(note, axis=1)
        spectrum /= 1.0 + (np.fft.rfftfreq(note.shape[1], 1.0 / RATE) / 2500.0) ** 2
        note = np.fft.irfft(spectrum, note.shape[1], axis=1)
        lay(voices["bells"], start, note * fade(note.shape[1], 0.0, note.shape[1] / RATE - 0.2,
                                                0.2) * strength)
    return voices


# ---- the room and the file -------------------------------------------------------------------

_rooms = {}


def room_answer(count):
    # The spectrum of what the room of make_sounds.py gives back for one click, per
    # side, for a track of count samples. The room is computed once, on a click, and
    # then laid over the whole track by multiplying spectra (a CONVOLUTION): the same
    # result as sending the track through the room sample by sample, in a fraction of
    # the time, and round the loop.
    if count not in _rooms:
        long = make_sounds.count_of(1.6 * MUSIC_ROOM_SECONDS)
        answers = []
        for side in range(2):
            click = np.zeros(count)
            wash = make_sounds.room_wash([1.0], long, side, MUSIC_ROOM_SECONDS)
            click[:long] = wash
            answers.append(np.fft.rfft(click))
        _rooms[count] = answers
    return _rooms[count]


def mixed(voices):
    # The voices at their levels, with the room, as one stereo track at PEAK_DB.
    count = voices["piano"].shape[1]
    dry = np.zeros((2, count))
    send = np.zeros((2, count))
    for voice, track in voices.items():
        track = track - track.mean(axis=1, keepdims=True)
        track = track * VOICE_PEAK[voice] / (np.abs(track).max() + 1e-12)
        dry += track
        send += ROOM_SEND[voice] * track
    answers = room_answer(count)
    sound = np.vstack([dry[side] + np.fft.irfft(np.fft.rfft(send[side]) * answers[side], count)
                       for side in range(2)])
    sound -= sound.mean(axis=1, keepdims=True)
    return sound * 10.0 ** (PEAK_DB / 20.0) / np.abs(sound).max()


def write_file(path, sound):
    data = np.round(sound.T * 32767.0).astype("<i2")
    with wave.open(path, "wb") as file:
        file.setnchannels(2)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(data.tobytes())
    # What can be said about the file without ears: its length and levels, and the seam
    # of the loop (the step from its last sample to its first one, next to the ordinary
    # step between two samples inside it, in steps of a 16 bit sample).
    values = data.astype(np.float64)
    steps = np.diff(values[:, 0])
    seam = abs(values[0, 0] - values[-1, 0])
    print(f"wrote {path}: {len(data) / RATE:.1f} s, peak "
          f"{20.0 * math.log10(np.abs(values).max() / 32768.0):.2f} dBFS, RMS "
          f"{10.0 * math.log10((values ** 2).mean() / 32768.0 ** 2):.2f} dBFS, DC "
          f"{values.mean() / 32768.0:+.5f}, seam step {seam:.0f} (ordinary "
          f"{math.sqrt((steps ** 2).mean()):.0f}), sha1 "
          f"{hashlib.sha1(data.tobytes()).hexdigest()[:12]}")


def self_check():
    # The smallest things that must hold: note names, a bar that does not add up is
    # refused, and a note laid over the end of a loop comes back at its beginning.
    assert midi_of("A4") == 69 and midi_of("C#4") == 61 and midi_of("Bb2") == 46
    assert read_row("-:1 A4+E5:2 | C4:3")[1] == 6.0
    track = np.zeros((2, 10))
    lay(track, 8.0 / RATE, np.ones((2, 4)))
    assert track[0].tolist() == [1.0, 1.0, 0, 0, 0, 0, 0, 0, 1.0, 1.0]
    for name, sketch in SKETCHES.items():
        events, total = events_of(sketch)
        assert 60.0 <= total <= 90.0, (name, total)
        assert all(seconds > 0 for voice in events.values() for _, _, seconds, _ in voice)


def main():
    parser = argparse.ArgumentParser(description="Makes sketches of a menu theme.")
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--route", choices=("synth", "sampled"), default="synth")
    parser.add_argument("--tools", help="folder with FluidSynth and the recordings")
    parser.add_argument("--only", help="make only the sketches whose name contains this")
    arguments = parser.parse_args()

    self_check()
    tools = None
    if arguments.route == "sampled":
        if not arguments.tools:
            raise SystemExit("--route sampled needs --tools DIR")
        tools = find_tools(arguments.tools)
    os.makedirs(arguments.out_dir, exist_ok=True)
    for name, sketch in SKETCHES.items():
        if arguments.only and arguments.only not in name:
            continue
        events, total = events_of(sketch)
        if tools:
            voices = sampled_voices(events, total, tools)
        else:
            key_notes = {midi % 12 for _, midi, _, _ in events["piano"]}
            voices = synth_voices(events, total, key_notes, sketch["seed"])
        write_file(os.path.join(arguments.out_dir, f"{name}_{arguments.route}.wav"), mixed(voices))


if __name__ == "__main__":
    main()
