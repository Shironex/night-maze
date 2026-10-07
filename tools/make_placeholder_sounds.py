# Makes the placeholder sounds of the game: seven short WAV files in assets/audio, one
# per sound cue (the table of the cues is in src/game/SoundCues.cpp).
#
# Run from the repository root, on any system:
#   python tools/make_placeholder_sounds.py
# It needs nothing but Python: only the standard library is used.
#
# The sounds are not recordings. Each one is computed here from sine waves and noise, so
# nobody else holds a right in them and they can be shipped with the game. They are
# placeholders: good enough to hear WHEN a sound is played, to be replaced by real ones
# later. A replacement only has to keep the file name.
#
# Every file is mono, 16 bit, 44100 samples per second. Three rules keep them clean:
#
#   - An ENVELOPE shapes the loudness of every sound: it starts at 0, rises within
#     a few milliseconds and falls back to 0 at the end. A sound that starts or ends at
#     any other value makes the loudspeaker jump, which is heard as a click that nobody
#     asked for.
#   - The loudest sample of every file is brought to PEAK (about -6 dBFS: half of the
#     largest value a file can hold), so all files are equally far from distortion and
#     two of them played together still fit.
#   - The noise comes from a random generator with a FIXED seed per sound, so running
#     the script again writes the same bytes: the files do not change in git.
#
# Switch: --out-dir (where the files go, default assets/audio).
import argparse
import math
import os
import random
import struct
import wave

# Samples per second of every file: the rate of a CD, which every sound device plays.
SAMPLE_RATE = 44100

# The loudest sample of every file, as a share of the largest possible value:
# 10 ^ (-6 / 20) = 0.501, which is -6 dBFS.
PEAK = 0.5

# The largest value of a signed 16 bit sample.
MAX_SAMPLE = 32767

# ---- building blocks ----------------------------------------------------------------------


def envelope(t, length, attack, decay):
    # The loudness at the moment t (seconds) of a sound that is length seconds long:
    # a straight rise from 0 to 1 within attack seconds, then a fall that gets slower
    # and slower (e to the power of -t / decay: after decay seconds a good third is
    # left). The last five milliseconds bring whatever is left down to exactly 0.
    rise = min(t / attack, 1.0)
    fall = math.exp(-t / decay)
    end = min((length - t) / 0.005, 1.0)
    return rise * fall * max(end, 0.0)


def sine(frequency, t):
    # A pure tone of frequency swings per second, at the moment t.
    return math.sin(2.0 * math.pi * frequency * t)


def smooth_noise(generator, count, smoothing):
    # count samples of noise with the high tones taken out: every sample is a mix of
    # a new random value and the sample before it. smoothing from 0 (raw hiss) towards
    # 1 (a low rumble).
    samples = []
    value = 0.0
    for _ in range(count):
        value = smoothing * value + (1.0 - smoothing) * generator.uniform(-1.0, 1.0)
        samples.append(value)
    return samples


def render(length, seed, voice):
    # Computes a sound of length seconds. voice(t, noise) gives the sample at the
    # moment t, where noise is one value of raw noise for that sample (from a generator
    # with the given seed).
    generator = random.Random(seed)
    count = int(length * SAMPLE_RATE)
    return [voice(i / SAMPLE_RATE, generator.uniform(-1.0, 1.0)) for i in range(count)]


def write_wav(path, samples):
    # Brings the loudest sample to PEAK and writes the file. "<h" is one signed 16 bit
    # number with its low byte first, the order a WAV file uses.
    loudest = max(abs(sample) for sample in samples)
    scale = PEAK * MAX_SAMPLE / loudest
    data = b"".join(struct.pack("<h", round(sample * scale)) for sample in samples)
    with wave.open(path, "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(SAMPLE_RATE)
        file.writeframes(data)
    print(f"wrote {path} ({len(samples) / SAMPLE_RATE:.2f} s, {len(data) + 44} bytes)")


# ---- the seven sounds -----------------------------------------------------------------------


def flashlight_on():
    # A short bright click: a burst of noise with a high tone in it, gone in 40 ms.
    length = 0.06
    return render(length, 1, lambda t, noise: envelope(t, length, 0.001, 0.008)
                  * (0.6 * noise + 0.4 * sine(2400.0, t)))


def flashlight_off():
    # The same switch going back: a little lower and a little softer at the start.
    length = 0.06
    return render(length, 2, lambda t, noise: envelope(t, length, 0.002, 0.010)
                  * (0.5 * noise + 0.5 * sine(1500.0, t)))


def flashlight_dead():
    # A dull click with nothing bright in it: the switch moves and no light comes. Low
    # tone, almost no noise, a little longer.
    length = 0.12
    return render(length, 3, lambda t, noise: envelope(t, length, 0.003, 0.025)
                  * (0.85 * sine(180.0, t) + 0.15 * noise))


def low_battery_pulse():
    # A soft thump like a heartbeat: a low tone whose pitch drops from 90 to 50 swings
    # per second while it fades. The pitch is the speed of the phase, so the phase is
    # the sum of the pitch over time (worked out by hand for a straight drop).
    length = 0.28

    def voice(t, noise):
        phase = 2.0 * math.pi * (90.0 * t - 0.5 * (40.0 / length) * t * t)
        return envelope(t, length, 0.012, 0.07) * math.sin(phase)

    return render(length, 4, voice)


def crystal_pickup():
    # A bright chime: three pure tones that ring out together (a major chord, two
    # octaves above the middle of a piano), the highest one fading fastest.
    length = 0.5
    return render(length, 5, lambda t, noise: envelope(t, length, 0.004, 0.12)
                  * (sine(1318.5, t) + 0.6 * sine(1661.2, t) + 0.4 * math.exp(-t / 0.05)
                     * sine(2637.0, t)))


def lever_pull():
    # A clunk of wood and metal: a thud (low tone and noise, gone at once) and a short
    # metallic ring (two tones that do not fit together) on top of it.
    length = 0.3

    def voice(t, noise):
        thud = math.exp(-t / 0.03) * (0.7 * sine(110.0, t) + 0.5 * noise)
        ring = math.exp(-t / 0.08) * (0.25 * sine(620.0, t) + 0.2 * sine(987.0, t))
        return envelope(t, length, 0.002, 0.2) * (thud + ring)

    return render(length, 6, voice)


def gate_open():
    # Stone grinding on stone for as long as the gate sinks (GATE_OPEN_SECONDS in
    # src/game/Round.hpp: 1.5 s): a low rumble of smoothed noise that shakes about
    # eleven times per second, with a deep tone under it. It swells in a fifth of
    # a second and dies away in the last half second.
    length = 1.5
    count = int(length * SAMPLE_RATE)
    rumble = smooth_noise(random.Random(7), count, 0.97)
    samples = []
    for i in range(count):
        t = i / SAMPLE_RATE
        swell = min(t / 0.2, 1.0) * min((length - t) / 0.5, 1.0)
        shake = 0.65 + 0.35 * sine(11.0, t)
        samples.append(swell * (shake * rumble[i] * 6.0 + 0.3 * sine(55.0, t)))
    return samples


# The file of every sound. The names are the ones in src/game/SoundCues.cpp.
SOUNDS = [
    ("flashlight_on.wav", flashlight_on),
    ("flashlight_off.wav", flashlight_off),
    ("flashlight_dead.wav", flashlight_dead),
    ("low_battery_pulse.wav", low_battery_pulse),
    ("crystal_pickup.wav", crystal_pickup),
    ("lever_pull.wav", lever_pull),
    ("gate_open.wav", gate_open),
]


def main():
    parser = argparse.ArgumentParser(description="Makes the placeholder sounds of the game.")
    parser.add_argument("--out-dir", default=os.path.join("assets", "audio"))
    arguments = parser.parse_args()

    os.makedirs(arguments.out_dir, exist_ok=True)
    for name, make in SOUNDS:
        write_wav(os.path.join(arguments.out_dir, name), make())


if __name__ == "__main__":
    main()
