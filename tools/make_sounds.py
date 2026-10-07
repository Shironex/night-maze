# Makes the sounds of the game: seven short WAV files in assets/audio, one per sound cue
# (the table of the cues is in src/game/SoundCues.cpp, and the file names there and here
# must stay the same).
#
# Run from the repository root, on any system:
#   python tools/make_sounds.py
# It needs nothing but Python: only the standard library is used (no numpy, no package
# for pictures). That is why the filters below are plain loops over lists of numbers.
# The whole run takes a few seconds.
#
# The sounds are not recordings and nothing is downloaded. Each one is computed here
# from sine waves and noise, so nobody else holds a right in them and they can be
# shipped with the game.
#
# How a sound is built. Every sound has a function of its own further down, and every
# function adds up a few LAYERS: one layer per thing that would make a noise in the
# real object (the contact of a switch, the ring of its metal body). Three kinds of
# layers are used again and again:
#
#   - MODES: sine waves that start together and die away, each at its own speed. A hard
#     thing that is struck (glass, metal, stone) rings at a few fixed frequencies, its
#     modes. When they are whole multiples of the lowest one the ear hears one clear
#     note, like a flute. When they are not (INHARMONIC), it hears a bell, a glass or
#     a piece of metal. Almost everything here is inharmonic on purpose.
#   - FILTERED NOISE: noise holds all frequencies at once. A filter lets a part of them
#     through: a LOW PASS the ones below its frequency, a HIGH PASS the ones above it,
#     a BAND PASS the ones around it. Its Q says how narrow the band is: a high Q rings
#     at the frequency, a low Q only colours the noise.
#   - ENVELOPES: curves between 0 and 1 that shape the loudness over time.
#
# Rules that keep the files clean (finish() applies them to every sound):
#
#   - No DC OFFSET: the samples swing around 0 and not around some other value, which
#     would waste room and click at the start and the end.
#   - Every file starts and ends at exactly 0, with a short fade.
#   - The loudest sample of a file sits at a level that is chosen per sound (the table
#     PEAK_DB): the loudest ones at -3 dBFS, never higher, and the others below that on
#     purpose, so the game plays them at the right loudness against each other without
#     a volume number per cue. dBFS counts down from the largest value a file can hold:
#     0 is that value, every -6 is half of it.
#   - The noise comes from a random generator with a FIXED seed per sound, so running
#     the script again writes the same bytes: the files do not change in git. (The same
#     bytes on the same machine. Another system may round a sine differently in its
#     last digit, which can move single samples by one step. Nobody hears that.)
#
# Most files are mono. The crystal and the gate are stereo, for a little width: the two
# channels differ in detail (other detuning, other noise), never in sign, so they do not
# cancel when a device plays them as one channel.
#
# Checking without ears: --report reads the files back and prints, for each one, the
# numbers a sound engineer would look at (length, peak, loudness, where the energy
# sits, the strongest frequencies), then compares them with what the sound is meant to
# be and prints "ok" or "MISS" per check. --pictures DIR writes one PNG per file, the
# waveform above its spectrogram. Numbers and pictures can show that a sound is built
# as intended. They cannot show that it sounds good: that takes a listener.
#
# Switches: --out-dir (where the files go and where --report reads them, default
# assets/audio), --report (measure the files instead of making them), --pictures DIR
# (draw the files instead of making them).
import argparse
import math
import os
import random
import struct
import wave
import zlib

# Samples per second of every file: the rate of a CD, which every sound device plays.
SAMPLE_RATE = 44100

# The largest value of a signed 16 bit sample.
MAX_SAMPLE = 32767

# The level of the loudest sample of every file, in dBFS. This is the mix of the game:
# the crystal and the gate are the two events of a round and stand in front, the lever
# is a step behind them, the switch of the lamp is a small thing in the hand, and the
# pulse is the quietest of all because it repeats for as long as the battery is low.
# A low tone also needs a far higher level than a high one to be heard at all, so the
# pulse is quieter to the ear than its number says (see the dBA column of --report).
PEAK_DB = {
    "flashlight_on.wav": -11.0,
    "flashlight_off.wav": -12.5,
    "flashlight_dead.wav": -15.0,
    "low_battery_pulse.wav": -16.0,
    "crystal_pickup.wav": -3.0,
    "lever_pull.wav": -6.0,
    "gate_open.wav": -3.0,
}

# ---- building blocks: time, envelopes, mixing ------------------------------------------------


def count_of(seconds):
    # How many samples a stretch of time is.
    return int(round(seconds * SAMPLE_RATE))


def silence(seconds):
    return [0.0] * count_of(seconds)


def place(target, layer, at_seconds=0.0, gain=1.0):
    # Adds a layer into a sound, starting at a moment of it. What reaches past the end
    # of the sound is cut off.
    start = count_of(at_seconds)
    end = min(len(target), start + len(layer))
    for i in range(start, end):
        target[i] += gain * layer[i - start]


def scaled(samples, curve):
    # A sound with its loudness shaped by a curve of the same length.
    return [sample * factor for sample, factor in zip(samples, curve)]


def with_level(samples, level):
    # A layer brought to a chosen loudness (the root of the mean square of its samples,
    # RMS for short), so the mix of the layers of a sound is written down as plain
    # numbers next to each other instead of hiding in the gains of their filters.
    present = math.sqrt(sum(sample * sample for sample in samples) / len(samples))
    return [sample * level / present for sample in samples]


def with_peak(samples, peak):
    # The same for a layer that is one short event: its loudest sample set to peak.
    loudest = max(abs(sample) for sample in samples)
    return [sample * peak / loudest for sample in samples]


def decay_curve(count, seconds):
    # Falls from 1 slower and slower: after the given time a good third is left
    # (e to the power of -1), after three times that time a twentieth.
    step = 1.0 / (seconds * SAMPLE_RATE)
    return [math.exp(-i * step) for i in range(count)]


def swell_curve(count, rise_seconds, fall_seconds):
    # Rises from 0 to 1 at the start and falls back to 0 at the end, both along half
    # a cosine wave: no corner anywhere, so the curve itself adds no click.
    rise = max(count_of(rise_seconds), 1)
    fall = max(count_of(fall_seconds), 1)
    curve = []
    for i in range(count):
        up = 0.5 - 0.5 * math.cos(math.pi * min(i / rise, 1.0))
        down = 0.5 - 0.5 * math.cos(math.pi * min((count - 1 - i) / fall, 1.0))
        curve.append(up * down)
    return curve


def wander_curve(generator, count, points_per_second, lowest=0.0):
    # A curve that wanders between lowest and 1 with no pattern: a RANDOM WALK. A new
    # target is chosen a few times per second, each one a random step away from the one
    # before (and turned back at the two ends), and the curve glides from target to
    # target. This is what makes a noise sound like something that drags and catches
    # instead of a steady hiss.
    spacing = SAMPLE_RATE / points_per_second
    points = [generator.uniform(0.3, 0.9)]
    while (len(points) - 2) * spacing < count:
        value = points[-1] + generator.uniform(-0.55, 0.55)
        if value > 1.0:
            value = 2.0 - value
        if value < 0.0:
            value = -value
        points.append(value)
    curve = []
    for i in range(count):
        place_in_points = i / spacing
        index = int(place_in_points)
        blend = 0.5 - 0.5 * math.cos(math.pi * (place_in_points - index))
        value = points[index] + (points[index + 1] - points[index]) * blend
        curve.append(lowest + (1.0 - lowest) * value)
    return curve


# ---- building blocks: sources ----------------------------------------------------------------


def noise(generator, count):
    # Raw noise: every sample a new random value. All frequencies, equally strong.
    return [generator.uniform(-1.0, 1.0) for _ in range(count)]


def modes(seconds, partials, start_seconds=0.0005):
    # A struck thing ringing. partials is a list of (frequency, strength, decay time).
    # Every sine starts at 0 and the first half millisecond rises smoothly, so the
    # strike is sharp without being a jump.
    count = count_of(seconds)
    samples = [0.0] * count
    for frequency, strength, decay_seconds in partials:
        turn = 2.0 * math.pi * frequency / SAMPLE_RATE
        fall = 1.0 / (decay_seconds * SAMPLE_RATE)
        for i in range(count):
            samples[i] += strength * math.exp(-i * fall) * math.sin(turn * i)
    start = max(count_of(start_seconds), 1)
    for i in range(min(start, count)):
        samples[i] *= 0.5 - 0.5 * math.cos(math.pi * i / start)
    return samples


def falling_thump(seconds, from_frequency, to_frequency, fall_seconds, decay_seconds):
    # A thump: a sine whose frequency drops while it fades, as the skin of a drum does
    # when it is struck. The frequency is the speed of the phase, so the phase is
    # summed up sample by sample. Because the pitch never stands still, the ear hears
    # a knock and not a note.
    count = count_of(seconds)
    samples = []
    phase = 0.0
    for i in range(count):
        t = i / SAMPLE_RATE
        frequency = to_frequency + (from_frequency - to_frequency) * math.exp(-t / fall_seconds)
        phase += 2.0 * math.pi * frequency / SAMPLE_RATE
        samples.append(math.exp(-t / decay_seconds) * math.sin(phase))
    return samples


# ---- building blocks: filters ----------------------------------------------------------------


def one_pole_low_pass(samples, frequency):
    # The gentlest low pass: every sample is a mix of the new value and the sample
    # before it. Tones above the frequency get weaker, slowly (half as strong for
    # every doubling of the frequency).
    keep = math.exp(-2.0 * math.pi * frequency / SAMPLE_RATE)
    result = []
    value = 0.0
    for sample in samples:
        value = keep * value + (1.0 - keep) * sample
        result.append(value)
    return result


def one_pole_high_pass(samples, frequency):
    # The other half: what the gentle low pass would take away.
    return [sample - low for sample, low in zip(samples, one_pole_low_pass(samples, frequency))]


def biquad(samples, kind, frequency, q=0.707):
    # A BIQUAD: the standard filter with two samples of memory on each side, twice as
    # steep as a one pole filter and with a Q. kind is "low", "high" or "band". The
    # five numbers are the well known ones of the Audio EQ Cookbook (Robert
    # Bristow-Johnson). The band pass keeps its centre frequency at full strength.
    turn = 2.0 * math.pi * frequency / SAMPLE_RATE
    cosine = math.cos(turn)
    alpha = math.sin(turn) / (2.0 * q)
    if kind == "low":
        b0, b1, b2 = (1.0 - cosine) / 2.0, 1.0 - cosine, (1.0 - cosine) / 2.0
    elif kind == "high":
        b0, b1, b2 = (1.0 + cosine) / 2.0, -(1.0 + cosine), (1.0 + cosine) / 2.0
    elif kind == "band":
        b0, b1, b2 = alpha, 0.0, -alpha
    else:
        raise ValueError(kind)
    a0, a1, a2 = 1.0 + alpha, -2.0 * cosine, 1.0 - alpha
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    result = []
    x1 = x2 = y1 = y2 = 0.0
    for x0 in samples:
        y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1, y2, y1 = x1, x0, y1, y0
        result.append(y0)
    return result


def moving_band_pass(samples, frequencies, q):
    # A band pass whose centre frequency changes from sample to sample (frequencies is
    # a curve as long as the sound): a resonance that moves, like the pitch of a creak.
    # The biquad above cannot be retuned while it runs without crackling. This form
    # (a STATE VARIABLE filter) can.
    damping = 1.0 / q
    result = []
    low = band = 0.0
    for sample, frequency in zip(samples, frequencies):
        tuning = 2.0 * math.sin(math.pi * frequency / SAMPLE_RATE)
        low += tuning * band
        high = sample - low - damping * band
        band += tuning * high
        result.append(band)
    return result


# ---- building blocks: the end of every sound -------------------------------------------------


def fade_ends(samples, in_seconds=0.0005, out_seconds=0.006):
    # Brings the first and the last sample to exactly 0, along half a cosine wave.
    count = len(samples)
    rise = count_of(in_seconds)
    fall = count_of(out_seconds)
    for i in range(min(rise, count)):
        samples[i] *= 0.5 - 0.5 * math.cos(math.pi * i / rise)
    for i in range(min(fall, count)):
        samples[count - 1 - i] *= 0.5 - 0.5 * math.cos(math.pi * i / fall)
    return samples


def finish(channels, out_seconds=0.006):
    # The last steps of every sound, for each of its channels (one for mono, two for
    # stereo): take out what lies below 15 Hz (nothing audible, but it would shift the
    # samples away from 0, a DC offset), then fade the two ends. The order matters: the
    # filter would pull faded ends away from 0 again. out_seconds is the length of the
    # fade at the end, for a sound that is still ringing when its file is over.
    return [fade_ends(one_pole_high_pass(channel, 15.0), out_seconds=out_seconds)
            for channel in channels]


def write_wav(path, channels, peak_db):
    # Brings the loudest sample of all channels to peak_db and writes the file. The
    # samples of the channels alternate in the file (left, right, left, right). "<h" is
    # one signed 16 bit number with its low byte first, the order a WAV file uses.
    loudest = max(abs(sample) for channel in channels for sample in channel)
    scale = 10.0 ** (peak_db / 20.0) * MAX_SAMPLE / loudest
    frames = bytearray()
    for frame in zip(*channels):
        for sample in frame:
            value = max(-MAX_SAMPLE, min(MAX_SAMPLE, int(round(sample * scale))))
            frames += struct.pack("<h", value)
    with wave.open(path, "wb") as file:
        file.setnchannels(len(channels))
        file.setsampwidth(2)
        file.setframerate(SAMPLE_RATE)
        file.writeframes(bytes(frames))
    seconds = len(channels[0]) / SAMPLE_RATE
    print(f"wrote {path} ({seconds:.2f} s, {len(channels)} ch, {len(frames) + 44} bytes)")


# ---- the switch of the lamp (three sounds share its two parts) -------------------------------

# A small switch makes two sounds a few milliseconds apart: the CONTACT (the spring
# snaps over and the metal tongues meet: a tick with nothing low in it) and the BODY
# (the snap arrives in the casing of the lamp, which answers with a short knock and,
# when it is free to ring, with a metallic ring).

# The modes of the thin metal casing: (frequency, strength, decay time). Not multiples
# of each other, as in any piece of sheet metal.
LAMP_RING = [(2640.0, 1.0, 0.016), (4130.0, 0.7, 0.011), (5870.0, 0.45, 0.007),
             (7950.0, 0.25, 0.004)]


def switch_contact(generator, darkest_frequency):
    # The tick of the contact: 1.5 ms of noise without its low part, gone at once.
    count = count_of(0.0015)
    burst = scaled(noise(generator, count), decay_curve(count, 0.0003))
    return biquad(burst, "high", darkest_frequency)


def switch_body(generator, knock_frequency):
    # The knock of the casing: a low mode that dies within a few milliseconds, and the
    # rattle of the snap around three times that frequency.
    count = count_of(0.03)
    knock = modes(0.03, [(knock_frequency, 1.0, 0.005), (knock_frequency * 1.47, 0.5, 0.0035)])
    rattle = scaled(noise(generator, count), decay_curve(count, 0.0012))
    place(knock, biquad(rattle, "band", knock_frequency * 3.1, 1.2), 0.0, 0.9)
    return knock


def flashlight_on():
    # Contact, then 6 ms later the body with its full ring: the switch of a lamp that
    # works. The ring is what tells the ear "metal, hollow, small".
    generator = random.Random(1)
    sound = silence(0.11)
    place(sound, switch_contact(generator, 3000.0), 0.001, 0.55)
    place(sound, switch_body(generator, 820.0), 0.007, 0.8)
    place(sound, modes(0.1, LAMP_RING), 0.007, 0.5)
    return finish([sound])


def flashlight_off():
    # The same switch going back. The spring is released instead of loaded, so
    # everything is a little lower and duller: the ring is three semitones lower,
    # it is shorter and it loses its two highest modes, and a low pass takes the edge
    # off the whole sound. The two parts are 5 ms apart instead of 6.
    generator = random.Random(2)
    sound = silence(0.1)
    place(sound, switch_contact(generator, 2200.0), 0.001, 0.5)
    place(sound, switch_body(generator, 690.0), 0.006, 0.85)
    ring = [(frequency * 0.84, strength, decay * 0.7)
            for frequency, strength, decay in LAMP_RING[:2]]
    place(sound, modes(0.08, ring), 0.006, 0.38)
    return finish([one_pole_low_pass(sound, 5200.0)])


def flashlight_dead():
    # The switch with no life in it. Contact and body are there, both muffled, and the
    # ring is missing altogether: nothing answers. Then, 75 ms later, one faint dry
    # tick, the tongue of the switch falling back because nothing holds it.
    generator = random.Random(3)
    sound = silence(0.16)
    place(sound, switch_contact(generator, 1500.0), 0.001, 0.4)
    place(sound, switch_body(generator, 600.0), 0.006, 0.9)
    tick_count = count_of(0.004)
    tick = scaled(noise(generator, tick_count), decay_curve(tick_count, 0.0006))
    place(sound, biquad(tick, "band", 1700.0, 1.5), 0.081, 0.8)
    return finish([one_pole_low_pass(sound, 3000.0)])


# ---- the low battery pulse -------------------------------------------------------------------


def low_battery_pulse():
    # Your own heartbeat noticing that the lamp is going: two soft thumps, the second
    # one weaker and a little lower, 170 ms apart (the "lub-dub" of a heart). Each is
    # a sine that falls from about 85 to 52 Hz while it fades, so there is no steady
    # pitch that could read as an alarm tone, and a slow rise of 12 ms, so there is no
    # click at its start. Two low passes at 160 Hz remove whatever is left above.
    #
    # The whole sound is 0.45 s long: the beats come as fast as every 0.8 s, and
    # a beat that started again before the last one ended would be cut off.
    sound = silence(0.45)
    first = scaled(falling_thump(0.25, 85.0, 52.0, 0.04, 0.05),
                   swell_curve(count_of(0.25), 0.012, 0.05))
    second = scaled(falling_thump(0.27, 74.0, 46.0, 0.04, 0.055),
                    swell_curve(count_of(0.27), 0.014, 0.05))
    place(sound, first, 0.0, 1.0)
    place(sound, second, 0.17, 0.6)
    return finish([one_pole_low_pass(one_pole_low_pass(sound, 160.0), 160.0)])


# ---- the crystal -----------------------------------------------------------------------------

# How the modes of a free bar of glass or metal lie above its lowest one (the numbers
# come from the physics of a bending bar). None of them is a whole multiple: this is
# why a glass rod or a chime bar sounds glassy and not like a flute.
BAR_RATIOS = [1.0, 2.756, 5.404, 8.933]


def crystal_strike(seconds, lowest_frequency, detune, lean):
    # One splinter ringing, for one channel. Every mode of the bar is played as a PAIR
    # of sines a few Hz apart. Two tones that close drift in and out of step, so their
    # sum slowly swells and thins: a shimmer, as in a real glass, which is never
    # perfectly round. lean (0 to 1) says how much of the upper tone of each pair this
    # channel gets: the two channels lean opposite ways, which spreads the shimmer
    # between left and right.
    #
    # The higher a mode, the weaker it is and the faster it dies: the sound starts
    # bright and ends as one soft tone.
    partials = []
    for number, ratio in enumerate(BAR_RATIOS):
        frequency = lowest_frequency * ratio
        strength = 0.5 ** number
        decay_seconds = 0.36 / (1.0 + 1.1 * number)
        apart = detune * (1.0 + 0.6 * number)
        partials.append((frequency, strength * (1.0 - 0.45 * lean), decay_seconds))
        partials.append((frequency + apart, strength * (0.55 + 0.45 * lean), decay_seconds * 0.9))
    # A rise of 3 ms: soft to the ear, and still fast enough that a second pickup that
    # cuts this one off and starts it again is heard at once.
    return modes(seconds, partials, start_seconds=0.003)


def crystal_pickup():
    # The one beautiful sound: a splinter of the moon, struck like a small glass.
    #
    #   - The main strike: a bar at 1397 Hz (about two and a half octaves above the middle of
    #     a piano) with its inharmonic modes, as detuned pairs.
    #   - A second, much quieter splinter 55 ms later, 1.6 times as high: the piece
    #     answering as it is picked up. 1.6 is on purpose no simple interval (a fifth
    #     would be 1.5): two strikes a fifth apart are a little tune, a jingle, and
    #     this is meant to be one piece of glass. Its modes fall between those of the
    #     first strike and thicken it.
    #   - The touch: 3 ms of very high noise at the start, the fingernail on the glass.
    #
    # Nothing lies below 1 kHz and the loudest moment is within the first hundredths of
    # a second, so the sound survives being cut and started again by the next pickup.
    # Stereo: the pairs lean left in one channel and right in the other, and the two
    # channels are detuned by slightly different amounts.
    generator = random.Random(5)
    length = 1.25
    touch_count = count_of(0.003)
    touch = biquad(scaled(noise(generator, touch_count), decay_curve(touch_count, 0.0008)),
                   "high", 5000.0)
    channels = []
    for detune, lean in ((2.3, 0.0), (3.1, 1.0)):
        sound = silence(length)
        place(sound, crystal_strike(length, 1397.0, detune, lean), 0.0, 1.0)
        place(sound, crystal_strike(length - 0.055, 2235.0, detune * 1.3, 1.0 - lean), 0.055, 0.22)
        place(sound, touch, 0.0, 0.12)
        channels.append(biquad(sound, "high", 700.0))
    # The lowest mode still rings faintly after 1.25 s, so the end is a slow fade.
    return finish(channels, out_seconds=0.12)


# ---- the lever -------------------------------------------------------------------------------


def lever_pull():
    # Wood and rope under tension, then a slab of stone letting go somewhere else.
    #
    #   - The creak, 0.4 s. A creak is STICK-SLIP: the wood holds, slips a hair, holds
    #     again, many times per second, and every slip knocks on the wood. So the
    #     source is a train of single knocks whose rate climbs from about 40 to 105 per
    #     second as the pull tightens, each one a little early or late and of its own
    #     strength (a regular train would be a buzz). Two moving band passes play the
    #     part of the wood: they ring at frequencies that climb with the rate.
    #   - The rope: a band of noise around 1 kHz that follows the creak, the fibres
    #     straining. Quiet: it only roughens the creak.
    #   - The knock, at 0.46 s: three low stone modes and a short burst of dull noise,
    #     all behind a low pass because it happens at a distance, and once more 95 ms
    #     later, weaker and duller, as it comes back from the walls of the maze.
    generator = random.Random(6)
    sound = silence(0.9)

    creak_count = count_of(0.42)
    slips = [0.0] * creak_count
    position = 0.0
    while position < creak_count:
        progress = position / creak_count
        slips[int(position)] = generator.uniform(0.5, 1.0)
        rate = 40.0 + 65.0 * progress ** 1.4
        position += SAMPLE_RATE / rate * generator.uniform(0.8, 1.2)
    climb = [i / creak_count for i in range(creak_count)]
    wood = moving_band_pass(slips, [330.0 + 190.0 * p for p in climb], 9.0)
    grain = moving_band_pass(slips, [860.0 + 420.0 * p for p in climb], 6.0)
    # The knocks are single samples, as sharp as a sound can be, and the band passes
    # let a good part of that edge through. Wood has no such edge: the low pass takes
    # it away, and what is left is a groan and not a buzz of clicks.
    creak = biquad([a + 0.45 * b for a, b in zip(wood, grain)], "low", 2200.0)
    rope = biquad(noise(generator, creak_count), "band", 1050.0, 1.4)
    strain = wander_curve(generator, creak_count, 18.0, lowest=0.45)
    creak = [(c + 0.05 * r) * s for c, r, s in zip(creak, rope, strain)]
    creak = with_level(scaled(creak, swell_curve(creak_count, 0.07, 0.11)), 0.34)
    place(sound, creak)

    knock_count = count_of(0.3)
    knock = modes(0.3, [(96.0, 1.0, 0.07), (163.0, 0.8, 0.045), (251.0, 0.55, 0.028)])
    dull = scaled(noise(generator, knock_count), decay_curve(knock_count, 0.012))
    place(knock, with_peak(biquad(dull, "low", 500.0), 1.0))
    # The creak has a loudness of 0.34 (see above) and the knock peaks at 3: it is the
    # event, the creak leads up to it.
    knock = with_peak(one_pole_low_pass(knock, 900.0), 3.0)
    place(sound, knock, 0.46)
    place(sound, one_pole_low_pass(knock, 350.0), 0.555, 0.28)
    return finish([sound])


# ---- the gate --------------------------------------------------------------------------------


def gate_open():
    # A slab of stone sinking into the ground. It sinks for 1.5 s (GATE_OPEN_SECONDS in
    # src/game/Round.hpp), then the sound has a third of a second to die away.
    #
    #   - The drag: noise through wide band passes at 240 and 470 Hz and a weaker one
    #     at 900 Hz, stone on stone, behind a low pass. Its loudness wanders (two
    #     random walks, a slow one for the slab catching and coming free, a fast one
    #     for the chatter of the surfaces), so it is a grind with grain and not a
    #     steady hiss.
    #   - The rumble: noise between 38 and 85 Hz, the weight of the slab in the
    #     ground. Noise and not a sine: a steady low tone would read as a machine.
    #   - The grit: sparse tiny ticks around 2 kHz, small stones crushed in the track.
    #     Very quiet. They come more often where the drag is loud.
    #   - The settling thud at 1.36 s: the slab reaches the bottom. A falling thump and
    #     two stone modes, with a burst of dull noise.
    #
    # Stereo: each channel has its own drag noise and its own grit, while the wander of
    # the loudness, the rumble and the thud are the same in both, so the slab stays in
    # the middle and only its texture is wide.
    generator = random.Random(7)
    length = 1.85
    count = count_of(length)
    moving = count_of(1.5)

    # 1 while the slab moves, then down to 0 within 0.12 s as it stops.
    motion = swell_curve(moving, 0.09, 0.12) + [0.0] * (count - moving)
    catch = wander_curve(generator, count, 7.0, lowest=0.12)
    chatter = wander_curve(generator, count, 38.0, lowest=0.5)
    grain = [m * c ** 1.5 * h for m, c, h in zip(motion, catch, chatter)]

    rumble = biquad(biquad(noise(generator, count), "low", 85.0), "high", 38.0)
    weight = [m * (0.6 + 0.4 * c) for m, c in zip(motion, catch)]
    rumble = with_level(scaled(rumble, weight), 0.6)

    thud_count = count_of(0.45)
    thud = falling_thump(0.45, 120.0, 58.0, 0.03, 0.11)
    place(thud, modes(0.45, [(149.0, 0.5, 0.06), (228.0, 0.3, 0.04)]))
    burst = scaled(noise(generator, thud_count), decay_curve(thud_count, 0.02))
    place(thud, with_peak(biquad(burst, "low", 350.0), 0.8))
    thud = with_peak(thud, 4.0)

    channels = []
    for _ in range(2):
        raw = noise(generator, count)
        drag = [a + 0.7 * b + 0.2 * c for a, b, c in zip(biquad(raw, "band", 240.0, 0.9),
                                                         biquad(raw, "band", 470.0, 1.3),
                                                         biquad(raw, "band", 900.0, 1.0))]
        # A band pass lets a good deal through far above its band. The low pass keeps
        # the slab heavy: without it the drag is a hiss with a third of its energy
        # above 1 kHz.
        drag = with_level(scaled(biquad(drag, "low", 650.0), grain), 1.0)
        grit = [0.0] * count
        for i in range(count):
            if generator.random() < 0.0016 * grain[i]:
                grit[i] = generator.uniform(-1.0, 1.0)
        grit = biquad(grit, "band", 2100.0, 2.5)
        # The mix, with the drag at a loudness of 1: the rumble at a little over half
        # of it, the grit at a thirtieth, and the thud peaking well above all of them.
        sound = drag
        place(sound, rumble)
        place(sound, with_level(grit, 0.033))
        place(sound, thud, 1.36)
        channels.append(sound)
    return finish(channels)


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


# ---- measuring the files (--report) ----------------------------------------------------------

# The size of the pieces a file is cut into for its spectrum. 4096 samples tell
# frequencies about 11 Hz apart.
FRAME = 4096


def read_wav(path):
    # A file as written above: its channels as lists of numbers from -1 to 1.
    with wave.open(path, "rb") as file:
        channel_count = file.getnchannels()
        if file.getsampwidth() != 2:
            raise ValueError(f"{path}: not 16 bit")
        rate = file.getframerate()
        data = file.readframes(file.getnframes())
    values = struct.unpack(f"<{len(data) // 2}h", data)
    channels = [[value / 32768.0 for value in values[c::channel_count]]
                for c in range(channel_count)]
    return channels, rate


def fourier(samples):
    # The FAST FOURIER TRANSFORM: turns a piece of sound (a power of two samples long)
    # into the strength of every frequency in it. The standard method, in place:
    # reorder the samples, then combine them in pairs, pairs of pairs and so on.
    count = len(samples)
    values = [complex(sample) for sample in samples]
    j = 0
    for i in range(1, count):
        bit = count >> 1
        while j & bit:
            j ^= bit
            bit >>= 1
        j |= bit
        if i < j:
            values[i], values[j] = values[j], values[i]
    size = 2
    while size <= count:
        half = size // 2
        turns = [complex(math.cos(2.0 * math.pi * k / size), -math.sin(2.0 * math.pi * k / size))
                 for k in range(half)]
        for start in range(0, count, size):
            for k in range(half):
                a = values[start + k]
                b = values[start + k + half] * turns[k]
                values[start + k] = a + b
                values[start + k + half] = a - b
        size *= 2
    return values


def power_spectrum(samples, frame=FRAME):
    # The energy of a whole sound per frequency: the sound is cut into overlapping
    # pieces, each piece is faded at its ends (a Hann window, which keeps one frequency
    # from smearing over its neighbours) and transformed, and the energies are added.
    # Entry k belongs to the frequency k * SAMPLE_RATE / frame.
    padded = list(samples) + [0.0] * (-len(samples) % (frame // 2)) + [0.0] * (frame // 2)
    window = [0.5 - 0.5 * math.cos(2.0 * math.pi * i / frame) for i in range(frame)]
    power = [0.0] * (frame // 2 + 1)
    for start in range(0, len(padded) - frame + 1, frame // 2):
        values = fourier([padded[start + i] * window[i] for i in range(frame)])
        for k in range(frame // 2 + 1):
            power[k] += values[k].real ** 2 + values[k].imag ** 2
    return power


def a_weight(frequency):
    # How much the ear makes of a frequency compared with 1 kHz, as a factor on the
    # energy: the A WEIGHTING of sound level meters. Tones around 3 kHz count in full,
    # 100 Hz counts a hundredth, 50 Hz a thousandth. A rough stand-in for loudness.
    if frequency <= 0.0:
        return 0.0
    f2 = frequency * frequency
    ratio = (12194.0 ** 2 * f2 * f2) / ((f2 + 20.6 ** 2) * math.sqrt((f2 + 107.7 ** 2)
                                        * (f2 + 737.9 ** 2)) * (f2 + 12194.0 ** 2))
    return (ratio * 1.2589) ** 2


def decibels(value):
    return 20.0 * math.log10(max(value, 1e-9))


def frame_levels(samples, seconds):
    # The loudness of a sound piece by piece (root of the mean square of each piece).
    size = count_of(seconds)
    return [math.sqrt(sum(s * s for s in samples[i:i + size]) / size)
            for i in range(0, max(len(samples) - size, 0) + 1, size)]


def measure(path):
    # Everything --report prints about one file, as a dictionary.
    channels, rate = read_wav(path)
    if rate != SAMPLE_RATE:
        raise ValueError(f"{path}: {rate} samples per second, expected {SAMPLE_RATE}")
    count = len(channels[0])
    # The spectrum is taken from the sum of the channels: what a mono device plays.
    mono = [sum(frame) / len(channels) for frame in zip(*channels)]
    everything = [sample for channel in channels for sample in channel]
    power = power_spectrum(mono)
    step = SAMPLE_RATE / FRAME
    total = sum(power[1:]) or 1e-30

    def share(low, high):
        return sum(p for k, p in enumerate(power) if k and low <= k * step < high) / total

    # The strongest frequencies: the highest points of the spectrum that are at least
    # 40 Hz away from a stronger one (without that rule the three "peaks" would be
    # three neighbouring entries of one and the same tone). Each is placed between the
    # entries by fitting a curve through the point and its two neighbours.
    tops = sorted((k for k in range(2, len(power) - 1)
                   if power[k] > power[k - 1] and power[k] >= power[k + 1]),
                  key=lambda k: -power[k])
    peaks = []
    for k in tops:
        if all(abs(k * step - other) >= 40.0 for other, _ in peaks):
            left, mid, right = (math.log(max(power[n], 1e-30)) for n in (k - 1, k, k + 1))
            bend = left - 2.0 * mid + right
            shift = 0.5 * (left - right) / bend if bend else 0.0
            peaks.append(((k + shift) * step, 10.0 * math.log10(power[k] / max(power))))
        if len(peaks) == 3:
            break

    # The loudest tenth of a second, weighed the way the ear weighs frequencies: every
    # tenth of a second of the sound is weighed, and the loudest one counts. (Choosing
    # the piece by its plain energy first would pick a low knock over a creak that the
    # ear hears as louder.)
    # Silence is added in front and behind, so a click at the very start of a file
    # can sit in the middle of a piece, where the fade of the window leaves it whole.
    window = FRAME
    weights = [a_weight(k * SAMPLE_RATE / window) for k in range(window // 2)]
    hann = [0.5 - 0.5 * math.cos(2.0 * math.pi * i / window) for i in range(window)]
    padded = [0.0] * (window // 2) + mono + [0.0] * window
    weighed = 0.0
    for start in range(0, count, 512):
        values = fourier([padded[start + i] * hann[i] for i in range(window)])
        weighed = max(weighed, sum(2.0 * (values[k].real ** 2 + values[k].imag ** 2) * weights[k]
                                   for k in range(1, window // 2)))
    # The fade of the window takes away 0.375 of the energy of a steady sound.
    weighed /= 0.375
    levels = frame_levels(mono, 0.02)
    middle = [level for i, level in enumerate(levels) if 0.1 <= i * 0.02 < 1.3] or levels
    loud_at = max(range(len(levels)), key=lambda i: levels[i]) * 0.02

    return {
        "channels": len(channels),
        "seconds": count / SAMPLE_RATE,
        "bytes": os.path.getsize(path),
        "peak": decibels(max(abs(s) for s in everything)),
        "rms": decibels(math.sqrt(sum(s * s for s in everything) / len(everything))),
        "loud50": decibels(max(frame_levels(mono, 0.05) or [0.0])),
        "dba": 10.0 * math.log10(max(weighed / window ** 2, 1e-18)),
        "dc": sum(everything) / len(everything),
        "first": [int(round(channel[0] * 32768)) for channel in channels],
        "last": [int(round(channel[-1] * 32768)) for channel in channels],
        "centroid": sum(k * step * p for k, p in enumerate(power)) / total,
        "peaks": peaks,
        "below120": share(0.0, 120.0),
        "below250": share(0.0, 250.0),
        "below500": share(0.0, 500.0),
        "above1k": share(1000.0, 1e9),
        "above2k": share(2000.0, 1e9),
        # How far the loudness moves between 0.1 and 1.3 s, in pieces of 20 ms. Only
        # asked of a sound that lasts that long.
        "swing": decibels(max(middle) / max(min(middle), 1e-9)) if count > 1.3 * rate else None,
        "loud_at": loud_at,
    }


def inharmonic(peaks):
    # True when the strongest frequencies do not form a simple chord with the lowest
    # of them: no ratio within 0.03 of a whole number, of a half or of a third (2, 1.5,
    # 1.333 and so on are the octave, the fifth, the fourth).
    lowest = min(frequency for frequency, _ in peaks)
    ratios = [frequency / lowest for frequency, _ in peaks if frequency != lowest]
    return bool(ratios) and all(abs(ratio * parts - round(ratio * parts)) >= 0.03 * parts
                                for ratio in ratios for parts in (1, 2, 3))


def together_peak(directory, names):
    # The loudest sample when the given files start at the same moment, in dBFS: the
    # game does that with the crystal and the gate when the last crystal that was
    # needed opens the gate. A mono file counts for both channels.
    sounds = [read_wav(os.path.join(directory, name))[0] for name in names]
    loudest = 0.0
    for side in range(2):
        tracks = [channels[min(side, len(channels) - 1)] for channels in sounds]
        for i in range(max(len(track) for track in tracks)):
            loudest = max(loudest, abs(sum(track[i] for track in tracks if i < len(track))))
    return decibels(loudest)


def checks(m, together):
    # What every sound is meant to be, as numbers: (what is checked, true or false).
    on, off, dead = m["flashlight_on.wav"], m["flashlight_off.wav"], m["flashlight_dead.wav"]
    pulse, crystal = m["low_battery_pulse.wav"], m["crystal_pickup.wav"]
    lever, gate = m["lever_pull.wav"], m["gate_open.wav"]
    result = []
    for name, one in m.items():
        result.append((f"{name}: peak at or below -3 dBFS", one["peak"] <= -2.99))
        result.append((f"{name}: no DC offset (below 0.001)", abs(one["dc"]) < 0.001))
        result.append((f"{name}: starts and ends at 0",
                       not any(one["first"]) and not any(one["last"])))
    others = [one["dba"] for name, one in m.items() if name != "low_battery_pulse.wav"]
    result += [
        ("on: a metallic ring, most energy above 2 kHz", on["above2k"] > 0.5),
        ("off: lower than on (centroid)", off["centroid"] < 0.85 * on["centroid"]),
        ("off: duller than on (share above 2 kHz)", off["above2k"] < on["above2k"]),
        ("off: quieter than on", off["dba"] < on["dba"]),
        ("dead: far less ring than on (share above 2 kHz under a third of it)",
         dead["above2k"] < on["above2k"] / 3.0),
        ("dead: quieter than off", dead["dba"] < off["dba"]),
        ("pulse: at least 90 % of its energy below 120 Hz", pulse["below120"] > 0.9),
        ("pulse: shorter than the fastest beat (0.8 s)", pulse["seconds"] < 0.8),
        ("pulse: the quietest to the ear, by at least 10 dBA", pulse["dba"] < min(others) - 10.0),
        ("crystal: at least 95 % of its energy above 1 kHz", crystal["above1k"] > 0.95),
        ("crystal: strongest frequencies are inharmonic", inharmonic(crystal["peaks"])),
        ("crystal: loudest within the first 40 ms (clean restart)", crystal["loud_at"] < 0.04),
        ("crystal and gate: the two most present (dBA)",
         min(crystal["dba"], gate["dba"]) > max(on["dba"], off["dba"], dead["dba"], lever["dba"])),
        ("lever: creak and knock, energy on both sides of 250 Hz",
         0.2 < lever["below250"] < 0.8),
        ("gate: at least 70 % of its energy below 500 Hz", gate["below500"] > 0.7),
        ("gate: loudness is not flat (swings by more than 10 dB while it sinks)",
         gate["swing"] is not None and gate["swing"] > 10.0),
        ("gate: loudest at the settling thud (1.3 to 1.5 s)", 1.3 <= gate["loud_at"] <= 1.5),
        (f"crystal and gate started together: peak {together:.2f} dBFS, at or below -1",
         together <= -1.0),
    ]
    return result


def report(directory):
    measured = {name: measure(os.path.join(directory, name)) for name, _ in SOUNDS}
    print("file                    ch  seconds  peak dBFS  RMS dBFS  loudest 50 ms  loudest dBA"
          "       DC  first/last  centroid Hz")
    for name, m in measured.items():
        print(f"{name:<22} {m['channels']:>3} {m['seconds']:>8.3f} {m['peak']:>10.2f}"
              f" {m['rms']:>9.2f} {m['loud50']:>14.2f} {m['dba']:>12.2f} {m['dc']:>+8.5f}"
              f"  {m['first']}/{m['last']}".ljust(96) + f"{m['centroid']:>10.0f}")
    print()
    print("file                    <120 Hz  <500 Hz   >1 kHz   >2 kHz  swing dB  loudest at"
          "  strongest frequencies (Hz, dB below the strongest)")
    for name, m in measured.items():
        peaks = ", ".join(f"{frequency:.0f} ({level:.0f})" for frequency, level in m["peaks"])
        swing = "-" if m["swing"] is None else f"{m['swing']:.1f}"
        print(f"{name:<22} {m['below120']:>8.1%} {m['below500']:>8.1%} {m['above1k']:>8.1%}"
              f" {m['above2k']:>8.1%} {swing:>9} {m['loud_at']:>9.2f} s  {peaks}")
    print()
    missed = 0
    together = together_peak(directory, ["crystal_pickup.wav", "gate_open.wav"])
    for text, passed in checks(measured, together):
        missed += not passed
        print(f"  {'ok  ' if passed else 'MISS'}  {text}")
    total = sum(m["bytes"] for m in measured.values())
    print(f"\n{total} bytes in {len(measured)} files, {missed} checks missed")
    return missed


# ---- drawing the files (--pictures) ----------------------------------------------------------


def write_png(path, rows):
    # A grey picture as a PNG file: rows of numbers from 0 (black) to 255 (white).
    # A PNG is a signature and three chunks (size of the picture, the squeezed rows,
    # the end), each with its length in front and a checksum behind.
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    raw = b"".join(b"\x00" + bytes(row) for row in rows)
    header = struct.pack(">IIBBBBB", len(rows[0]), len(rows), 8, 0, 0, 0, 0)
    with open(path, "wb") as file:
        file.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header)
                   + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def picture(path, wav_path):
    # The waveform (top, 120 rows) above the spectrogram (360 rows) of one file, 720
    # columns for its whole length. The spectrogram shows time from left to right and
    # frequency from 30 Hz (bottom) to 16 kHz (top), each octave equally tall. White is
    # loud, black is 70 dB below the loudest point. The three grey lines are 100 Hz,
    # 1 kHz and 10 kHz, the ticks along the bottom are tenths of a second.
    channels, _ = read_wav(wav_path)
    mono = [sum(frame) / len(channels) for frame in zip(*channels)]
    count = len(mono)
    width, wave_rows, spec_rows = 720, 120, 360
    frame = 1024 if count < SAMPLE_RATE // 2 else 2048
    window = [0.5 - 0.5 * math.cos(2.0 * math.pi * i / frame) for i in range(frame)]
    padded = [0.0] * (frame // 2) + mono + [0.0] * frame

    low, high = 30.0, 16000.0

    def row_of(frequency):
        return int((1.0 - math.log(frequency / low) / math.log(high / low)) * (spec_rows - 1))

    # Which entries of the spectrum fall into every row of the picture.
    bins = []
    for row in range(spec_rows):
        top = low * (high / low) ** (1.0 - row / spec_rows)
        bottom = low * (high / low) ** (1.0 - (row + 1) / spec_rows)
        first = int(bottom * frame / SAMPLE_RATE)
        last = max(int(top * frame / SAMPLE_RATE), first + 1)
        bins.append((max(first, 1), last))

    columns = []
    for x in range(width):
        start = x * count // width
        values = fourier([padded[start + i] * window[i] for i in range(frame)])
        strengths = [abs(value) for value in values[:frame // 2 + 1]]
        columns.append([max(strengths[first:last + 1]) for first, last in bins])
    loudest = max(max(column) for column in columns) or 1.0

    rows = [[18] * width for _ in range(wave_rows + spec_rows)]
    scale = (wave_rows / 2 - 2) / (max(abs(sample) for sample in mono) or 1.0)
    for x in range(width):
        part = mono[x * count // width:max((x + 1) * count // width, x * count // width + 1)]
        top = int(wave_rows / 2 - max(part) * scale)
        bottom = int(wave_rows / 2 - min(part) * scale)
        for y in range(top, bottom + 1):
            rows[y][x] = 235
        for row in range(spec_rows):
            level = decibels(columns[x][row] / loudest)
            rows[wave_rows + row][x] = int(255 * max(0.0, 1.0 + level / 70.0))
    for frequency in (100.0, 1000.0, 10000.0):
        line = rows[wave_rows + row_of(frequency)]
        for x in range(0, width, 4):
            line[x] = max(line[x], 110)
    for tenth in range(int(count / SAMPLE_RATE * 10) + 1):
        x = min(int(tenth * 0.1 * SAMPLE_RATE * width / count), width - 1)
        for y in range(wave_rows + spec_rows - 6, wave_rows + spec_rows):
            rows[y][x] = 255
    write_png(path, rows)
    print(f"drew {path}")


# ---- the program -----------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description="Makes the sounds of the game.")
    parser.add_argument("--out-dir", default=os.path.join("assets", "audio"))
    parser.add_argument("--report", action="store_true")
    parser.add_argument("--pictures", metavar="DIR")
    arguments = parser.parse_args()

    if arguments.report:
        raise SystemExit(1 if report(arguments.out_dir) else 0)
    if arguments.pictures:
        os.makedirs(arguments.pictures, exist_ok=True)
        for name, _ in SOUNDS:
            picture(os.path.join(arguments.pictures, name[:-4] + ".png"),
                    os.path.join(arguments.out_dir, name))
        return

    os.makedirs(arguments.out_dir, exist_ok=True)
    for name, make in SOUNDS:
        write_wav(os.path.join(arguments.out_dir, name), make(), PEAK_DB[name])


if __name__ == "__main__":
    main()
