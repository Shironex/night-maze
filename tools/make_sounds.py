# Makes the sounds of the game: twenty-three WAV files in assets/audio, one per sound cue
# (the table of the cues is in src/game/SoundCues.cpp, and the file names there and here
# must stay the same).
#
# Run from the repository root, on any system:
#   python tools/make_sounds.py
# It needs nothing but Python: only the standard library is used (no numpy, no package
# for pictures). That is why the filters below are plain loops over lists of numbers.
# The whole run takes a few seconds. --report takes most of a minute: the wind of the
# intro is half a minute long, and measuring it is far more work than making it.
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
#   - Every file starts and ends at exactly 0, with a short fade. (The one loop, the
#     wind of the maze, has no fade: it ends where it begins. See maze_wind.)
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

# Samples per second of every file but the loop (RATES): the rate of a CD, which every
# sound device plays.
SAMPLE_RATE = 44100

# The largest value of a signed 16 bit sample.
MAX_SAMPLE = 32767

# The level of the loudest sample of every file, in dBFS. This is the mix of the game:
# the crystal and the gate are the two events of a round and stand in front, the lever
# is a step behind them, the switch of the lamp is a small thing in the hand, and the
# pulse is the quietest of all because it repeats for as long as the battery is low.
# A low tone also needs a far higher level than a high one to be heard at all, so the
# pulse is quieter to the ear than its number says (see the dBA column of --report).
# The breath repeats too, while the player is winded, and it lies where the ear hears
# best: it gets the lowest number of all and is still easier to hear than the pulse.
# The flask is a pickup like the crystal, but a small comfort and not the goal of the
# round: it stands a step behind the crystal.
# The hum of the shade repeats like the pulse and the breath, but it is the one warning
# that must not be missed, so it stands above both. The sound of being caught is heard
# once and is meant to be soft: well below the crystal and the gate.
# The alert of the shade is heard once per hunt and has to cut through the wind and the
# steps, so it stands a little above the hum. The banish is a breath going out: soft,
# about as loud as the hum, because it is good news and nothing to be startled by.
PEAK_DB = {
    "flashlight_on.wav": -11.0,
    "flashlight_off.wav": -12.5,
    "flashlight_dead.wav": -15.0,
    "low_battery_pulse.wav": -16.0,
    "crystal_pickup.wav": -3.0,
    "lever_pull.wav": -6.0,
    "gate_open.wav": -3.0,
    "winded_breath.wav": -24.0,
    "flask_pickup.wav": -9.0,
    "shade_near.wav": -13.0,
    "caught.wav": -10.0,
    # The two sounds of the intro. The wind lies under spoken text (the cards) for half
    # a minute and must never be in front. Noise is far quieter to the ear than its
    # loudest sample says, so its number is not the lowest. The bell is far away.
    "intro_wind.wav": -11.0,
    "intro_bell.wav": -10.0,
    # The steps of the player are heard twice a second for a whole round, so they are
    # among the quietest sounds: about as present as the breath, and below the hum of
    # the shade, which is a warning. (A walked step is played at 0.6 of this, a sprinted
    # one in full: FOOTSTEP_WALK_VOLUME in src/game/SoundCues.hpp.)
    "footstep_1.wav": -14.0,
    "footstep_2.wav": -14.0,
    "footstep_3.wav": -14.0,
    # The steps of the shade are quieter than the ones of the player also right next to
    # it, and the game turns them down further with the distance.
    "shade_step_1.wav": -21.0,
    "shade_step_2.wav": -21.0,
    # The wind of the maze lies under everything for the whole round: quieter to the
    # ear than every sound that tells the player something.
    "maze_wind.wav": -27.5,
    "shade_alert.wav": -11.0,
    "shade_banish.wav": -14.0,
    # The bell of the open gate tolls every few seconds until the round ends, so it is
    # softer than the one bell of the intro, and the game turns it down further with
    # the distance (gateBellVolume in src/game/SoundCues.hpp).
    "gate_bell.wav": -12.0,
    # The heartstone is heard once in a maze. It stands just behind the crystal: it is
    # the same glass, and its low ring carries more than its peak says.
    "heartstone_pickup.wav": -5.0,
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


def write_wav(path, channels, peak_db, rate=SAMPLE_RATE):
    # Brings the loudest sample of all channels to peak_db and writes the file. The
    # samples of the channels alternate in the file (left, right, left, right). "<h" is
    # one signed 16 bit number with its low byte first, the order a WAV file uses.
    # rate is how many samples per second the file says it has.
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
        file.setframerate(rate)
        file.writeframes(bytes(frames))
    seconds = len(channels[0]) / rate
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


def crystal_strike(seconds, lowest_frequency, detune, lean, ring=1.0):
    # One splinter ringing, for one channel. Every mode of the bar is played as a PAIR
    # of sines a few Hz apart. Two tones that close drift in and out of step, so their
    # sum slowly swells and thins: a shimmer, as in a real glass, which is never
    # perfectly round. lean (0 to 1) says how much of the upper tone of each pair this
    # channel gets: the two channels lean opposite ways, which spreads the shimmer
    # between left and right.
    #
    # The higher a mode, the weaker it is and the faster it dies: the sound starts
    # bright and ends as one soft tone. ring makes every mode last that many times as
    # long: a larger piece rings longer.
    partials = []
    for number, ratio in enumerate(BAR_RATIOS):
        frequency = lowest_frequency * ratio
        strength = 0.5 ** number
        decay_seconds = ring * 0.36 / (1.0 + 1.1 * number)
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


def heartstone_pickup():
    # The heartstone: the big splinter of a maze. The sound of a crystal, made as much
    # larger as the stone is, so the player knows the glass and hears that this piece
    # is another size.
    #
    #   - The main strike: the same bar with the same inharmonic modes, at 524 Hz in
    #     place of 1397 Hz (a little more than an octave lower), ringing two and a half
    #     times as long.
    #   - The answer: a quieter strike 80 ms later, 1.6 times as high, as in the crystal
    #     and for the same reason (no simple interval, so it is no tune).
    #   - The weight: a soft thump that falls from 210 to 135 Hz, the stone landing in
    #     the hand. It stays above the heartbeat of the low battery (below 120 Hz).
    #
    # Stereo like the crystal: the pairs lean left in one channel and right in the
    # other. It uses no noise, so it needs no random generator.
    length = 2.2
    weight = falling_thump(0.4, 210.0, 135.0, 0.05, 0.11)
    channels = []
    for detune, lean in ((1.3, 0.0), (1.8, 1.0)):
        sound = silence(length)
        place(sound, crystal_strike(length, 524.0, detune, lean, ring=2.5), 0.0, 1.0)
        place(sound, crystal_strike(length - 0.08, 838.0, detune * 1.3, 1.0 - lean, ring=2.5),
              0.08, 0.3)
        place(sound, weight, 0.0, 0.45)
        channels.append(biquad(sound, "high", 110.0))
    # The lowest mode still rings faintly at the end, so the end is a slow fade.
    return finish(channels, out_seconds=0.25)


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


# ---- the breath of a winded player -----------------------------------------------------------


def winded_breath():
    # One heavy breath through the open mouth: in, a short stop, out. A breath has no
    # tone, it is air rushing through a narrow place, so both halves are noise behind
    # band passes with a low Q (a high Q would whistle).
    #
    #   - In, 0.32 s: a band around 1500 Hz. Air that is drawn in is the thinner and
    #     brighter of the two, and the quieter one.
    #   - Out, 0.5 s, starting at 0.4 s: two bands, around 650 and 1250 Hz. Lower and
    #     fuller, with a fast start and a long tail: the air is pushed out and then
    #     runs out.
    #   - Both swell and fade along smooth curves, and a little wander of the loudness
    #     keeps them from being a clean hiss.
    #
    # A low pass at 2600 Hz keeps the breath soft (no "s" in it) and a high pass at
    # 250 Hz keeps it from rumbling. The whole sound is 0.95 s long: the breaths come
    # every 1.1 s (WINDED_BREATH_SECONDS in src/game/SoundCues.hpp), and a breath that
    # started again before the last one ended would be cut off.
    generator = random.Random(8)
    sound = silence(0.95)

    in_count = count_of(0.32)
    air_in = biquad(noise(generator, in_count), "band", 1500.0, 0.9)
    air_in = scaled(scaled(air_in, swell_curve(in_count, 0.15, 0.13)),
                    wander_curve(generator, in_count, 22.0, lowest=0.7))

    out_count = count_of(0.5)
    raw = noise(generator, out_count)
    air_out = [low + 0.5 * high for low, high in zip(biquad(raw, "band", 650.0, 1.0),
                                                     biquad(raw, "band", 1250.0, 1.4))]
    air_out = scaled(scaled(air_out, swell_curve(out_count, 0.08, 0.36)),
                     wander_curve(generator, out_count, 22.0, lowest=0.7))

    # The mix: breathing out at a loudness of 1, breathing in at half of it.
    place(sound, with_level(air_in, 0.5), 0.0)
    place(sound, with_level(air_out, 1.0), 0.4)
    return finish([biquad(biquad(sound, "low", 2600.0), "high", 250.0)])


# ---- the flask of tea -------------------------------------------------------------------------


def flask_pickup():
    # A flask is opened and the tea warms: a cork pop, then a soft warm note. It must
    # not be mistaken for the crystal, which is a struck glass far above 1 kHz and
    # inharmonic, so everything here is low, round and in tune.
    #
    #   - The pop, at the start: the air in the neck of the flask. A thump that falls
    #     from 950 to 330 Hz within a few milliseconds and is gone after a twentieth of
    #     a second, with 6 ms of noise around 1700 Hz on top, the cork leaving the neck.
    #   - The warm note, from 0.11 s: three sine waves that ARE whole multiples of each
    #     other on purpose (392, 588 and 784 Hz: a note, its fifth and its octave), so
    #     the ear hears one soft chord. It rises over 30 ms instead of being struck and
    #     dies away within half a second.
    generator = random.Random(9)
    sound = silence(0.62)

    pop = falling_thump(0.09, 950.0, 330.0, 0.012, 0.022)
    cork_count = count_of(0.006)
    cork = biquad(scaled(noise(generator, cork_count), decay_curve(cork_count, 0.0015)),
                  "band", 1700.0, 1.2)

    warm = modes(0.5, [(392.0, 1.0, 0.15), (588.0, 0.45, 0.12), (784.0, 0.2, 0.08)],
                 start_seconds=0.03)

    # The mix: the pop is the loudest moment, the cork a small part of it, and the
    # note a little below the pop.
    place(sound, with_peak(pop, 1.0), 0.0)
    place(sound, with_peak(cork, 0.3), 0.0)
    place(sound, with_peak(warm, 0.7), 0.11)
    return finish([biquad(sound, "low", 3000.0)])


# ---- the shade --------------------------------------------------------------------------------


def shade_near():
    # The shade is near: a cold hum that swells and fades, with no beat and no breath in
    # it. It must not be taken for the low battery pulse (two thumps below 120 Hz) or for
    # the breath of a winded player (noise around 1 kHz), so it is neither: steady tones
    # between them, with no knock at the start.
    #
    #   - The drone: 147 Hz and its fifth at 220 Hz, with no third between them. A chord
    #     without a third is neither glad nor sad, only empty.
    #   - Each of the two has a second sine a little beside it (147 against 148.3, 220
    #     against 221.7). Two tones that close do not sound like two: they swell and
    #     thin out against each other about once a second, so the hum is never still.
    #   - A thin band of noise around 440 Hz, far below the tones: a little air, like
    #     wind in a pipe.
    #   - The loudness rises for a quarter of a second and falls for longer. Nothing
    #     starts suddenly.
    #
    # The whole sound is 0.62 s long: the hums come as fast as every 0.7 s
    # (SHADE_HUM_FAST_SECONDS in src/game/SoundCues.hpp), and a hum that started again
    # before the last one ended would be cut off.
    generator = random.Random(10)
    seconds = 0.62
    count = count_of(seconds)
    # Decay times far longer than the sound: the tones do not die away by themselves,
    # the swell curve shapes them.
    drone = modes(seconds, [(147.0, 1.0, 30.0), (148.3, 0.8, 30.0),
                            (220.0, 0.55, 30.0), (221.7, 0.45, 30.0)])
    air = biquad(noise(generator, count), "band", 440.0, 3.0)

    sound = silence(seconds)
    place(sound, with_level(drone, 1.0))
    place(sound, with_level(air, 0.08))
    sound = scaled(sound, swell_curve(count, 0.25, 0.33))
    return finish([biquad(sound, "low", 900.0)])


def caught():
    # The shade has reached you and sets you down at the stile: soft, not a fright. Two
    # low round notes, the second a fifth below the first (330 Hz, then 220 Hz), like
    # a breath let go. Each rises slowly instead of being struck, and both have their
    # octave above them at a quarter of the strength, so they sound like a note and not
    # like a test tone. The first note is still fading when the second one comes.
    sound = silence(1.5)
    first = scaled(modes(0.9, [(330.0, 1.0, 0.45), (660.0, 0.25, 0.3)], start_seconds=0.12),
                   swell_curve(count_of(0.9), 0.12, 0.5))
    second = scaled(modes(1.15, [(220.0, 1.0, 0.6), (440.0, 0.25, 0.4)], start_seconds=0.15),
                    swell_curve(count_of(1.15), 0.15, 0.7))
    place(sound, with_peak(first, 0.8), 0.0)
    place(sound, with_peak(second, 1.0), 0.35)
    return finish([biquad(sound, "low", 1500.0)])
def shade_alert():
    # The shade has noticed you and turns towards you: one short, cold sound. It must
    # not be taken for the hum (a low drone that swells), for the crystal (a struck glass
    # far above 1 kHz) or for the click of the lamp, so it is none of them: thin tones in
    # the middle, close together, that start at once and are gone in a third of a second.
    #
    #   - Two sine waves a small step apart (587 and 622 Hz). So close, they do not
    #     sound like a chord: they grind against each other 35 times a second, which the
    #     ear hears as something rough and cold.
    #   - A third one above them (911 Hz), quieter and shorter: a little edge. It is no
    #     whole multiple and no fifth of the two below, so the three are no chord.
    #   - A thin band of noise around 2500 Hz for the first twentieth of a second, far
    #     below the tones: a hiss of frost at the start.
    #   - It starts within 8 ms. Not a click, but sudden: the hum swells, this does not.
    generator = random.Random(17)
    sound = silence(0.42)
    tones = modes(0.42, [(587.0, 1.0, 0.11), (622.0, 0.9, 0.09), (911.0, 0.35, 0.05)],
                  start_seconds=0.008)
    frost_count = count_of(0.05)
    frost = biquad(scaled(noise(generator, frost_count), decay_curve(frost_count, 0.012)),
                   "band", 2500.0, 2.0)
    place(sound, with_peak(tones, 1.0), 0.0)
    place(sound, with_peak(frost, 0.12), 0.0)
    return finish([biquad(sound, "low", 4000.0)])


def shade_banish():
    # The beam has burned the shade away: soft, like breath going out. No tone that
    # could be a note, no knock: air that rushes out and runs dry.
    #
    #   - Noise through a band that FALLS, from 900 Hz down to 260 Hz over the whole
    #     sound: a sigh gets darker as it ends. The band is wide (a low Q), so nothing
    #     whistles.
    #   - The loudness comes up within a tenth of a second and takes the rest of the
    #     second to go, with a little wander so it is not a clean hiss.
    #   - Far below it a low tone at 110 Hz that dies with it: the weight of the thing
    #     that is gone.
    generator = random.Random(18)
    seconds = 1.0
    count = count_of(seconds)
    # The centre of the band, sample by sample: from 900 Hz down to 260 Hz, fast at
    # first and slower later, like the pitch of air running out.
    centres = [260.0 + 640.0 * math.exp(-3.0 * i / count) for i in range(count)]
    air = moving_band_pass(noise(generator, count), centres, 1.1)
    air = scaled(scaled(air, swell_curve(count, 0.1, 0.8)),
                 wander_curve(generator, count, 14.0, lowest=0.75))
    weight = scaled(modes(seconds, [(110.0, 1.0, 0.3)], start_seconds=0.08),
                    swell_curve(count, 0.08, 0.8))
    sound = silence(seconds)
    place(sound, with_peak(air, 1.0), 0.0)
    place(sound, with_peak(weight, 0.12), 0.0)
    return finish([biquad(biquad(sound, "low", 2200.0), "high", 80.0)])


# ---- the intro: the wind and the bell --------------------------------------------------------

# How long the intro is, in seconds: the five cards of src/game/Intro.cpp (INTRO_CARDS)
# add up to this. The wind is exactly as long, because the game cannot fade a sound: it
# starts the file with the first card and the file ends with the last one.
INTRO_SECONDS = 30.0


def intro_wind():
    # Night wind over open ground, for the whole intro. Wind has no tone: it is air
    # that rushes past things, noise whose colour and loudness move slowly.
    #
    #   - The body: noise through a band pass whose centre wanders between 180 and
    #     520 Hz (a random walk with a new target every three seconds). The moving
    #     centre is the "whoo" of wind: a gust is higher AND louder.
    #   - The air: the same noise through a wide band around 1100 Hz, quiet. It keeps
    #     the wind from sounding like something behind a door.
    #   - The gusts: the loudness of both follows the same random walk as the centre,
    #     and never falls below a third, so the bed does not drop out.
    #   - The ground: noise below 90 Hz, steady and quiet. The size of the place.
    #
    # The fades are in the file: it rises over three seconds and falls over the last
    # four, so the wind is already going when the first card is read and gone when the
    # main menu comes in.
    generator = random.Random(10)
    count = count_of(INTRO_SECONDS)
    raw = noise(generator, count)
    gust = wander_curve(generator, count, 0.33, lowest=0.0)

    body = moving_band_pass(raw, [180.0 + 340.0 * g for g in gust], 1.6)
    air = biquad(raw, "band", 1100.0, 0.6)
    ground = biquad(biquad(noise(generator, count), "low", 90.0), "high", 30.0)

    loudness = [0.34 + 0.66 * g for g in gust]
    sound = scaled(with_level(body, 1.0), loudness)
    place(sound, scaled(with_level(air, 0.16), loudness))
    place(sound, with_level(ground, 0.22))
    sound = scaled(biquad(sound, "low", 2400.0), swell_curve(count, 3.0, 4.0))
    return finish([sound])


# How the partials of a church bell lie above its "prime", the note the ear names. The
# names are the ones bell founders use: the hum an octave below, then the prime, the
# tierce (a minor third, which is what makes a bell sound sad), the quint, the nominal
# an octave above and two more above that. They are tuned by the founder and still are
# no harmonic series: that is the sound of a bell.
BELL_PARTIALS = [(0.5, 0.55, 3.2), (1.0, 1.0, 2.4), (1.2, 0.7, 1.7), (1.5, 0.3, 1.2),
                 (2.0, 0.55, 0.9), (2.51, 0.18, 0.55), (3.01, 0.12, 0.4)]


def intro_bell():
    # One bell of the village, far away. The prime is at 311 Hz (a small tower bell).
    #
    #   - The ring: the seven partials above, each with its own strength and decay
    #     time. The low ones ring for seconds, the high ones are gone within a second.
    #   - Far: the strike has a slow start of 12 ms and no clapper noise at all, and
    #     a low pass at 1500 Hz takes the brightness away. Air does that over
    #     a distance: the high part of a sound arrives last and weakest.
    #
    # The file is 5.5 s long, and its end is a slow fade: the hum still rings then.
    prime = 311.0
    ring = modes(5.5, [(prime * ratio, strength, decay) for ratio, strength, decay in BELL_PARTIALS],
                 start_seconds=0.012)
    return finish([one_pole_low_pass(one_pole_low_pass(ring, 1500.0), 1500.0)], out_seconds=0.8)


def gate_bell():
    # The bell that tolls while the gate is open: the same bell of the village as in the
    # intro (the same partials on the same prime of 311 Hz), heard from the maze, so it
    # is the one sound of the night the player already knows.
    #
    #   - Shorter: every partial dies away in half the time, and the file is 2.6 s long.
    #     The bell tolls every 6 s and is never cut off by its next toll.
    #   - Farther: the strike takes 20 ms to arrive, and the low pass sits at 1100 Hz
    #     instead of 1500 Hz.
    #
    # Its end is a slow fade, like the end of the bell of the intro. It uses no noise,
    # so it needs no random generator.
    prime = 311.0
    ring = modes(2.6, [(prime * ratio, strength, 0.5 * decay)
                       for ratio, strength, decay in BELL_PARTIALS], start_seconds=0.02)
    return finish([one_pole_low_pass(one_pole_low_pass(ring, 1100.0), 1100.0)], out_seconds=0.5)


# ---- steps: the player and the shade ---------------------------------------------------------


def footstep(seed, pitch, brush_at):
    # One step of the player on the ground of the maze: earth with grass on it. Three
    # files are made from this function, each with its own noise, a slightly other
    # pitch and its brush a little earlier or later, and the game plays them in turn:
    # the same step again and again would sound like a machine.
    #
    #   - The heel: a thump that falls from about 240 to 110 Hz within a few hundredths
    #     of a second and is gone at once. The weight of the body arriving. It rises
    #     over 3 ms: sudden enough for a step, and no click. It lies above the heartbeat
    #     of the low battery (below 120 Hz), so the two are not taken for each other.
    #   - The earth: a short burst of dull noise around 300 Hz with the heel, the soil
    #     giving way a little.
    #   - The grass: noise in a wide band around 1500 Hz that swells and fades within
    #     a tenth of a second, a moment after the heel. The sole rolling over the
    #     blades. Quiet, and its loudness wanders, so it rustles and does not hiss.
    #
    # The file is 0.26 s long: shorter than the wait between two steps of a sprint
    # (0.36 s, FOOTSTEP_SPRINT_METRES in src/game/SoundCues.hpp at 5.5 m/s).
    generator = random.Random(seed)
    sound = silence(0.26)

    heel_count = count_of(0.14)
    heel = scaled(falling_thump(0.14, 240.0 * pitch, 110.0 * pitch, 0.02, 0.03),
                  swell_curve(heel_count, 0.003, 0.06))
    earth_count = count_of(0.06)
    earth = biquad(scaled(noise(generator, earth_count), decay_curve(earth_count, 0.012)),
                   "band", 300.0 * pitch, 0.9)

    grass_count = count_of(0.16)
    grass = biquad(noise(generator, grass_count), "band", 1500.0 * pitch, 0.7)
    grass = scaled(scaled(grass, swell_curve(grass_count, 0.03, 0.11)),
                   wander_curve(generator, grass_count, 45.0, lowest=0.4))

    # The mix: the heel is the step, the earth a good half of it, the grass below both.
    place(sound, with_peak(heel, 1.0), 0.0)
    place(sound, with_peak(earth, 0.6), 0.002)
    place(sound, with_peak(grass, 0.25), brush_at)
    return finish([biquad(sound, "low", 3500.0)])


def shade_step(seed, pitch):
    # One step of the shade: something heavy and soft set down without a sound of its
    # own. No heel, no grass and no edge anywhere, so it cannot be taken for a step of
    # the player, and it is not the low battery pulse either, which is two thumps below
    # 120 Hz: this is one, and most of it lies above that.
    #
    #   - The weight: a thump that falls from about 250 to 125 Hz and rises over 30 ms,
    #     like a hand laid on a table.
    #   - The cloth: a little dull noise around 420 Hz that swells and fades with it.
    #
    # Two files, a little apart in pitch, played in turn. Each is 0.32 s long, and the
    # steps of the shade come every 0.65 s (SHADE_STEP_METRES in src/game/SoundCues.hpp
    # at 4 m/s).
    generator = random.Random(seed)
    sound = silence(0.32)

    weight_count = count_of(0.26)
    weight = scaled(falling_thump(0.26, 250.0 * pitch, 125.0 * pitch, 0.035, 0.07),
                    swell_curve(weight_count, 0.03, 0.16))
    cloth_count = count_of(0.2)
    cloth = biquad(noise(generator, cloth_count), "band", 420.0 * pitch, 1.0)
    cloth = scaled(cloth, swell_curve(cloth_count, 0.04, 0.14))

    place(sound, with_peak(weight, 1.0), 0.0)
    place(sound, with_peak(cloth, 0.12), 0.01)
    return finish([biquad(sound, "low", 1000.0)])


# ---- the wind of the maze: a loop ------------------------------------------------------------

# How long one round of the loop is, in seconds, and how much of its end is laid over
# its beginning to close it.
MAZE_WIND_SECONDS = 10.0
MAZE_WIND_OVERLAP_SECONDS = 2.0


def maze_wind():
    # The wind of the maze at night, as a LOOP: the game plays the file again and again
    # for as long as a round lasts, so its end has to run into its beginning without
    # anything to hear. It is the wind of the intro (intro_wind above) made smaller:
    # quieter, with less movement, and without the low ground, which belongs to the
    # heartbeat of the low battery.
    #
    #   - The body: noise through a band pass whose centre moves between 240 and 480 Hz.
    #   - The air: the same noise through a wide band around 1200 Hz, quiet.
    #   - The movement: the centre and the loudness follow one slow curve. In the intro
    #     that curve is a random walk. Here it is two sine waves, one that takes the
    #     whole loop and one that takes a third of it: both are back where they began
    #     when the loop ends, so the wind rises and falls without a pattern the ear
    #     could count, and the movement has no seam either.
    #
    # Closing the loop. Twelve seconds are made, two more than the loop is long. The
    # last two are laid over the first two: where the loop begins, the extra end is at
    # full strength and the beginning at none, and over two seconds they change places
    # (a CROSSFADE). So the first sample of the file is the very sample that would have
    # followed its last one, and the file goes round as if it had never been cut. The
    # two curves of the crossfade are a quarter of a sine and of a cosine: their
    # squares add up to 1, which keeps the loudness of two different noises steady
    # (with straight lines the middle of the crossfade would be quieter).
    #
    # The file has none of the fades of finish(): a loop must not start and end at 0.
    # The game fades it in and out itself. It is written at half the sample rate of
    # the other files. Nothing in it lies above 3 kHz, and a file at 22 050 samples per
    # second holds everything up to 11 kHz, so the only difference is half the size.
    generator = random.Random(14)
    count = count_of(MAZE_WIND_SECONDS)
    overlap = count_of(MAZE_WIND_OVERLAP_SECONDS)
    total = count + overlap
    raw = noise(generator, total)

    # From 0 to 1, and the same again after count samples.
    turn = 2.0 * math.pi / count
    gust = [0.5 + 0.3 * math.sin(turn * i + 1.0) + 0.2 * math.sin(3.0 * turn * i + 2.3)
            for i in range(total)]

    body = moving_band_pass(raw, [240.0 + 240.0 * g for g in gust], 1.5)
    air = biquad(raw, "band", 1200.0, 0.6)
    loudness = [0.6 + 0.4 * g for g in gust]
    sound = scaled(with_level(body, 1.0), loudness)
    place(sound, scaled(with_level(air, 0.2), loudness))
    # Two low passes keep it soft and make the halving of the sample rate safe. The
    # high pass takes away what lies under 120 Hz (and with it any DC offset).
    sound = biquad(biquad(biquad(sound, "low", 2400.0), "low", 4000.0), "high", 120.0)

    loop = sound[:count]
    for i in range(overlap):
        angle = 0.5 * math.pi * i / overlap
        loop[i] = sound[i] * math.sin(angle) + sound[count + i] * math.cos(angle)
    # Every second sample: half the sample rate.
    loop = loop[::2]
    # A loop has no beginning, so the file may start at any of its samples. It starts
    # where the wave crosses 0 most gently: of all pairs of neighbours, at the one that
    # lies nearest to 0. That is not for looks. The game converts the file to the
    # sample rate of the sound device when it loads it, and that conversion starts from
    # silence: its first few samples are pulled towards 0. With the seam at a loud
    # sample the loop ticked on every round (measured: a jump of 0.040 in a sound whose
    # loudest sample is 0.042). With the seam at 0 there is nothing to pull.
    start = min(range(len(loop)), key=lambda i: abs(loop[i]) + abs(loop[i - 1]))
    return [loop[start:] + loop[:start]]


# The files that are not written at SAMPLE_RATE, with their rate. A sound without
# anything high in it needs fewer samples per second (see maze_wind).
RATES = {"maze_wind.wav": SAMPLE_RATE // 2}

# The files the game plays as loops: they do not start and end at 0, they end where
# they begin.
LOOPS = ("maze_wind.wav",)


# The file of every sound. The names are the ones in src/game/SoundCues.cpp.
SOUNDS = [
    ("flashlight_on.wav", flashlight_on),
    ("flashlight_off.wav", flashlight_off),
    ("flashlight_dead.wav", flashlight_dead),
    ("low_battery_pulse.wav", low_battery_pulse),
    ("crystal_pickup.wav", crystal_pickup),
    ("lever_pull.wav", lever_pull),
    ("gate_open.wav", gate_open),
    ("winded_breath.wav", winded_breath),
    ("flask_pickup.wav", flask_pickup),
    ("shade_near.wav", shade_near),
    ("caught.wav", caught),
    ("intro_wind.wav", intro_wind),
    ("intro_bell.wav", intro_bell),
    ("footstep_1.wav", lambda: footstep(11, 1.0, 0.035)),
    ("footstep_2.wav", lambda: footstep(12, 0.95, 0.05)),
    ("footstep_3.wav", lambda: footstep(13, 1.08, 0.028)),
    ("shade_step_1.wav", lambda: shade_step(15, 1.0)),
    ("shade_step_2.wav", lambda: shade_step(16, 0.95)),
    ("maze_wind.wav", maze_wind),
    # Added after the nineteen above, like their cues in the game (SoundCue). Each
    # sound has a random generator of its own, so the files above stay byte for byte
    # what they were.
    ("shade_alert.wav", shade_alert),
    ("shade_banish.wav", shade_banish),
    ("gate_bell.wav", gate_bell),
    ("heartstone_pickup.wav", heartstone_pickup),
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


def seam_numbers(samples):
    # How a file goes round as a loop, in steps of a 16 bit sample. A STEP is the
    # difference between two samples that follow each other, a BEND the change of that
    # difference from one pair to the next (how sharply the line through three samples
    # turns). Both are measured over the seam, from the last samples of the file to its
    # first ones, and compared with what is ordinary inside the file: the root of the
    # mean square of all steps and of all bends, and the largest of each. A loop that
    # is closed well has a seam like any other place. A cut that was not closed shows
    # as a step several times the ordinary one, and is heard as a tick.
    def rms(values):
        return math.sqrt(sum(value * value for value in values) / len(values))

    steps = [b - a for a, b in zip(samples, samples[1:])]
    bends = [b - a for a, b in zip(steps, steps[1:])]
    over = samples[0] - samples[-1]
    return {
        "step": abs(over) * 32768,
        "step_rms": rms(steps) * 32768,
        "step_max": max(abs(step) for step in steps) * 32768,
        "bend": max(abs(over - steps[-1]), abs(steps[0] - over)) * 32768,
        "bend_rms": rms(bends) * 32768,
        "bend_max": max(abs(bend) for bend in bends) * 32768,
    }


def measure(path):
    # Everything --report prints about one file, as a dictionary.
    channels, rate = read_wav(path)
    if rate not in (SAMPLE_RATE, SAMPLE_RATE // 2):
        raise ValueError(f"{path}: {rate} samples per second, expected {SAMPLE_RATE} or half")
    # The seam is measured on the samples the file holds, before anything else.
    seam = seam_numbers(channels[0])
    stored_seconds = len(channels[0]) / rate
    if rate != SAMPLE_RATE:
        # A file at half the rate is brought to the full rate for everything below, so
        # one set of measuring code serves both: between every two samples goes the
        # value half way between them. That is how the simplest player would do it. It
        # costs a third of a decibel at 2 kHz and nothing below.
        channels = [[value for a, b in zip(channel, channel[1:] + channel[-1:])
                     for value in (a, 0.5 * (a + b))] for channel in channels]
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
        # For a loop: the seam, the length as the file states it, the loudness of every
        # half second, and of its first and of its last twentieth of a second.
        "seam": seam,
        "stored_seconds": stored_seconds,
        "rate": rate,
        "halves": [decibels(level) for level in frame_levels(mono, 0.5)],
        "ends": [decibels(level) for level in (frame_levels(mono[:count_of(0.05)], 0.05)
                                               + frame_levels(mono[-count_of(0.05):], 0.05))],
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


def checks(m, together, heart_together):
    # What every sound is meant to be, as numbers: (what is checked, true or false).
    on, off, dead = m["flashlight_on.wav"], m["flashlight_off.wav"], m["flashlight_dead.wav"]
    pulse, crystal = m["low_battery_pulse.wav"], m["crystal_pickup.wav"]
    lever, gate = m["lever_pull.wav"], m["gate_open.wav"]
    breath = m["winded_breath.wav"]
    flask = m["flask_pickup.wav"]
    hum, caught_sound = m["shade_near.wav"], m["caught.wav"]
    wind, bell = m["intro_wind.wav"], m["intro_bell.wav"]
    # The two sounds of the intro are not sounds of a round: they are left out where
    # the sounds of a round are compared with each other.
    intro = ("intro_wind.wav", "intro_bell.wav")
    # The steps and the wind of the maze came later, and they are not events either:
    # they sound all the time. The two old rules that compare the pulse and the breath
    # with "every other sound" leave them out, and they have rules of their own below.
    steps = [m[f"footstep_{number}.wav"] for number in (1, 2, 3)]
    shade_steps = [m[f"shade_step_{number}.wav"] for number in (1, 2)]
    maze = m["maze_wind.wav"]
    alert, banish = m["shade_alert.wav"], m["shade_banish.wav"]
    toll = m["gate_bell.wav"]
    heart = m["heartstone_pickup.wav"]
    constant = ("footstep_1.wav", "footstep_2.wav", "footstep_3.wav", "shade_step_1.wav",
                "shade_step_2.wav", "maze_wind.wav")
    intro = intro + constant
    result = []
    for name, one in m.items():
        result.append((f"{name}: peak at or below -3 dBFS", one["peak"] <= -2.99))
        result.append((f"{name}: no DC offset (below 0.001)", abs(one["dc"]) < 0.001))
        if name not in LOOPS:
            result.append((f"{name}: starts and ends at 0",
                           not any(one["first"]) and not any(one["last"])))
    others = [one["dba"] for name, one in m.items()
              if name != "low_battery_pulse.wav" and name not in intro]
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
        ("breath: shorter than the wait between two breaths (1.1 s)", breath["seconds"] < 1.1),
        ("breath: air and no rumble, under 5 % of its energy below 250 Hz",
         breath["below250"] < 0.05),
        ("breath: soft, under 15 % of its energy above 2 kHz", breath["above2k"] < 0.15),
        ("breath: quieter to the ear than every sound but the pulse",
         breath["dba"] < min(one["dba"] for name, one in m.items()
                             if name not in ("low_battery_pulse.wav", "winded_breath.wav")
                             and name not in intro)),
        ("flask: warm, at least 90 % of its energy below 1 kHz (the crystal lies above)",
         flask["above1k"] < 0.1),
        ("flask: a note and not a bell (its strongest frequencies are in tune)",
         not inharmonic(flask["peaks"])),
        ("flask: quieter to the ear than the crystal", flask["dba"] < crystal["dba"]),
        (f"crystal and gate started together: peak {together:.2f} dBFS, at or below -1",
         together <= -1.0),
        ("hum: shorter than the fastest wait between two hums (0.7 s)", hum["seconds"] < 0.7),
        ("hum: not a heartbeat, under 10 % of its energy below 120 Hz", hum["below120"] < 0.1),
        ("hum: not a breath, under 5 % of its energy above 1 kHz", hum["above1k"] < 0.05),
        ("hum: a swell, loudest later than 0.15 s after its start", hum["loud_at"] > 0.15),
        ("hum: easier to hear than the pulse and the breath",
         hum["dba"] > max(pulse["dba"], breath["dba"])),
        ("caught: soft, at least 90 % of its energy below 1 kHz", caught_sound["above1k"] < 0.1),
        ("caught: no sudden start, loudest later than 0.1 s after its start",
         caught_sound["loud_at"] > 0.1),
        ("caught: quieter to the ear than the crystal and the gate",
         caught_sound["dba"] < min(crystal["dba"], gate["dba"])),
        ("alert: short, under half a second", alert["seconds"] < 0.5),
        ("alert: sudden, loudest within the first 50 ms (the hum swells, this does not)",
         alert["loud_at"] < 0.05),
        ("alert: not the hum, under 10 % of its energy below 500 Hz", alert["below500"] < 0.1),
        ("alert: not the crystal, under 10 % of its energy above 1 kHz", alert["above1k"] < 0.1),
        ("alert: cold, its strongest frequencies are not in tune", inharmonic(alert["peaks"])),
        ("alert: easier to hear than the hum, and quieter than the crystal",
         hum["dba"] < alert["dba"] < crystal["dba"]),
        ("banish: a breath going out, longer than the alert and at most a second",
         alert["seconds"] < banish["seconds"] <= 1.0),
        ("banish: no sudden start, loudest later than 0.05 s after its start",
         banish["loud_at"] > 0.05),
        ("banish: soft, under 10 % of its energy above 2 kHz", banish["above2k"] < 0.1),
        ("banish: air and no rumble, under 20 % of its energy below 120 Hz",
         banish["below120"] < 0.2),
        ("banish: quieter to the ear than the alert", banish["dba"] < alert["dba"]),
        ("wind: exactly as long as the intro (30 s)", abs(wind["seconds"] - INTRO_SECONDS) < 0.001),
        ("wind: no hiss, under 10 % of its energy above 2 kHz", wind["above2k"] < 0.1),
        ("wind: a bed, its loudest tenth of a second quieter to the ear than the crystal",
         wind["dba"] < crystal["dba"]),
        ("bell: its three strongest partials are the hum, the prime and the tierce "
         "(0.5, 1 and 1.2 times the strongest)",
         all(abs(ratio - wanted) < 0.02 for ratio, wanted in zip(
             sorted(frequency / bell["peaks"][0][0] for frequency, _ in bell["peaks"]),
             (0.5, 1.0, 1.2)))),
        ("bell: far away, under 5 % of its energy above 2 kHz", bell["above2k"] < 0.05),
        ("bell: loudest at its strike (within the first quarter of a second)",
         bell["loud_at"] < 0.25),
        ("gate bell: the bell of the intro, its three strongest partials are the hum, the "
         "prime and the tierce",
         all(abs(ratio - wanted) < 0.02 for ratio, wanted in zip(
             sorted(frequency / toll["peaks"][0][0] for frequency, _ in toll["peaks"]),
             (0.5, 1.0, 1.2)))),
        ("gate bell: on the same prime as the bell of the intro (within 2 Hz)",
         abs(toll["peaks"][0][0] - bell["peaks"][0][0]) < 2.0),
        ("gate bell: shorter than the shortest wait between two tolls (3 s)",
         toll["seconds"] < 3.0),
        ("gate bell: farther away than the bell of the intro: duller (centroid) and quieter "
         "to the ear", toll["centroid"] < bell["centroid"] and toll["dba"] < bell["dba"]),
        ("gate bell: far away, under 5 % of its energy above 2 kHz", toll["above2k"] < 0.05),
        ("gate bell: a soft strike, loudest between 10 ms and a quarter of a second",
         0.01 < toll["loud_at"] < 0.25),
        ("gate bell: quieter to the ear than the crystal and the gate",
         toll["dba"] < min(crystal["dba"], gate["dba"])),
        ("heartstone: the crystal made larger, its centroid under half of the crystal's",
         heart["centroid"] < 0.5 * crystal["centroid"]),
        ("heartstone: glass and no note, its strongest frequencies are inharmonic",
         inharmonic(heart["peaks"])),
        ("heartstone: rings longer than the crystal", heart["seconds"] > crystal["seconds"]),
        ("heartstone: loudest within the first 40 ms, like the crystal", heart["loud_at"] < 0.04),
        ("heartstone: not the heartbeat of the battery, under 5 % of its energy below 120 Hz",
         heart["below120"] < 0.05),
        ("heartstone: no louder to the ear than the crystal", heart["dba"] <= crystal["dba"]),
        (f"heartstone and gate started together: peak {heart_together:.2f} dBFS, at or below -1",
         heart_together <= -1.0),
    ]

    # The steps of the player, the steps of the shade and the wind of the maze. What
    # carries information: every sound of a round but these, and the steps themselves.
    # The pulse is left out wherever loudness to the ear is compared: it lies so low
    # that the measure of the ear (dBA) hardly counts it, and it has its own rule above.
    seam = maze["seam"]
    telling = [one["dba"] for name, one in m.items()
               if name not in intro and name != "low_battery_pulse.wav"]
    telling += [one["dba"] for one in steps + shade_steps]
    step_levels = [one["dba"] for one in steps]
    shade_levels = [one["dba"] for one in shade_steps]
    result += [
        ("steps: quieter to the ear than the chime of a crystal, by at least 15 dBA",
         max(step_levels) < crystal["dba"] - 15.0),
        ("steps: quieter to the ear than the hum of the shade, which is a warning",
         max(step_levels) < hum["dba"]),
        ("steps: the three are one kind of step, within 3 dBA of each other",
         max(step_levels) - min(step_levels) < 3.0),
        ("steps: the three are not the same file (their strongest frequency differs)",
         len({round(one["peaks"][0][0]) for one in steps}) == 3),
        ("steps: a thump, at least 80 % of their energy below 500 Hz",
         all(one["below500"] > 0.8 for one in steps)),
        ("steps: not the heartbeat of the battery, under half of their energy below 120 Hz",
         all(one["below120"] < 0.5 for one in steps)),
        ("steps: shorter than the wait between two steps of a sprint (0.36 s)",
         all(one["seconds"] < 0.36 for one in steps)),
        ("shade steps: quieter to the ear than the steps of the player, by at least 3 dBA",
         max(shade_levels) < min(step_levels) - 3.0),
        ("shade steps: soft, under 2 % of their energy above 1 kHz",
         all(one["above1k"] < 0.02 for one in shade_steps)),
        ("shade steps: no sudden start, loudest later than 0.01 s after their start",
         all(one["loud_at"] > 0.01 for one in shade_steps)),
        ("shade steps: not the heartbeat of the battery, under half of their energy below 120 Hz",
         all(one["below120"] < 0.5 for one in shade_steps)),
        ("shade steps: shorter than the wait between two of them (0.65 s)",
         all(one["seconds"] < 0.65 for one in shade_steps)),
        ("maze wind: a loop of 8 to 12 seconds", 8.0 <= maze["stored_seconds"] <= 12.0),
        ("maze wind: mono", maze["channels"] == 1),
        ("maze wind: quieter to the ear than every sound that carries information",
         maze["dba"] < min(telling)),
        ("maze wind: leaves the heartbeat of the battery its place, under 5 % of its energy "
         "below 120 Hz", maze["below120"] < 0.05),
        ("maze wind: no hiss, under 10 % of its energy above 2 kHz", maze["above2k"] < 0.1),
        (f"maze wind: no step at the seam ({seam['step']:.0f} against an ordinary "
         f"{seam['step_rms']:.0f}, at most 3 times that)", seam["step"] <= 3.0 * seam["step_rms"]),
        (f"maze wind: no bend at the seam ({seam['bend']:.0f} against an ordinary "
         f"{seam['bend_rms']:.0f}, at most 3 times that)", seam["bend"] <= 3.0 * seam["bend_rms"]),
        (f"maze wind: as loud before the seam as after it ({maze['ends'][1]:.1f} and "
         f"{maze['ends'][0]:.1f} dBFS, within 4 dB)", abs(maze["ends"][0] - maze["ends"][1]) < 4.0),
        ("maze wind: it moves, its half seconds differ by more than 3 dB",
         max(maze["halves"]) - min(maze["halves"]) > 3.0),
        ("maze wind: it never drops out, its half seconds differ by less than 12 dB",
         max(maze["halves"]) - min(maze["halves"]) < 12.0),
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
    for name in LOOPS:
        m = measured[name]
        seam = m["seam"]
        print(f"{name}: a loop of {m['stored_seconds']:.3f} s at {m['rate']} samples per second."
              f" Over the seam: step {seam['step']:.0f} (inside the file: ordinary"
              f" {seam['step_rms']:.0f}, largest {seam['step_max']:.0f}), bend {seam['bend']:.0f}"
              f" (ordinary {seam['bend_rms']:.0f}, largest {seam['bend_max']:.0f}), in steps of"
              f" a 16 bit sample. Last sample {m['last']}, first {m['first']}.")
    print()
    missed = 0
    together = together_peak(directory, ["crystal_pickup.wav", "gate_open.wav"])
    heart_together = together_peak(directory, ["heartstone_pickup.wav", "gate_open.wav"])
    for text, passed in checks(measured, together, heart_together):
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
        write_wav(os.path.join(arguments.out_dir, name), make(), PEAK_DB[name],
                  RATES.get(name, SAMPLE_RATE))


if __name__ == "__main__":
    main()
