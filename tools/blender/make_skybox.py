# Generates the six pictures of the night sky into assets/skybox: px.png, nx.png, py.png,
# ny.png, pz.png and nz.png, the faces +X, -X, +Y, -Y, +Z and -Z of a cube map. All six are
# SIZE x SIZE pixels, 8 bits per channel, RGB.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_skybox.py
#
# A cube map is sampled with a DIRECTION, not with a texture coordinate: the graphics card
# picks the face the direction points at and the pixel of that face it passes through. So
# every pixel here is computed from the direction it is seen in, and nothing is painted
# "on a face". Two faces then meet without a seam, because both sides of the border
# compute the same sky for (almost) the same direction.
#
# Coordinates: the axes of the game. Right-handed, Y up, -Z forward. Blender is used only
# to write the PNG files, so its own Z-up system does not appear in this file.
#
# Row order of the files. The first row of a PNG is its TOP row, and that is the order
# a cube map face wants: OpenGL reads the first row it is given as t = 0, and on a cube
# map face t = 0 is the top of the picture (the table in FACES). The game therefore loads
# these files WITHOUT the row flip it applies to 2D textures (assets::RowOrder::TopFirst).
#
# Looking at the files. A face picture shows its side of the cube as seen from OUTSIDE the
# cube, and the game looks at it from the inside. So next to a screenshot of the game
# every file looks mirrored left to right. That is how cube maps are defined, not a bug.
#
# The colours below are sRGB values, picked by eye for the final picture and kept dark:
# the sky is a backdrop behind the lit walls. The game loads the faces as an sRGB cube
# map and encodes the frame to sRGB again at its end, so the screen shows these numbers.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import bpy
import numpy as np

import blender_common as common

# Width and height of one face in pixels. One face covers 90 degrees, so a pixel in the
# middle of a face is about 0.11 degrees wide. The game shows 60 degrees on 720 pixels
# (0.08 degrees per pixel), so the sky is drawn close to its own resolution.
SIZE = 1024

# The same seed gives the same sky on every run. Change it to get other stars.
SKY_SEED = 53

# The six faces in the order OpenGL numbers them (GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0..5),
# which is also the order gfx::Cubemap takes them in.
#
# Each face is described by three vectors. A pixel of the face is seen in the direction
#   forward + a * right + b * down
# where a runs from -1 at the left edge of the picture to 1 at the right edge and b from
# -1 at the TOP edge to 1 at the bottom edge. The vectors are the cube map table of the
# OpenGL specification ("Selection of cube map images"), written the other way round: the
# specification says which face and which (s, t) a direction hits, this table says which
# direction a pixel (s, t) of a face is.
FACES = (
    # file name, forward, right, down
    ("px.png", (1.0, 0.0, 0.0), (0.0, 0.0, -1.0), (0.0, -1.0, 0.0)),
    ("nx.png", (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, -1.0, 0.0)),
    ("py.png", (0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0)),
    ("ny.png", (0.0, -1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, -1.0)),
    ("pz.png", (0.0, 0.0, 1.0), (1.0, 0.0, 0.0), (0.0, -1.0, 0.0)),
    ("nz.png", (0.0, 0.0, -1.0), (-1.0, 0.0, 0.0), (0.0, -1.0, 0.0)),
)

# ---- the moon ---------------------------------------------------------------------------
# COUPLING WITH THE GAME: these two angles are the defaults of the moon LIGHT,
# moonYawDegrees and moonPitchDegrees of game::LightingSettings in src/game/Lighting.hpp.
# They say which way the light travels, so the disc is painted in the opposite direction:
# where the light comes from. When the defaults change there, change them here and run
# this script again. The test in tests/SkyboxTests.cpp looks for the disc where the default
# light comes from: it fails only when the two sides are more than about 2 degrees apart
# (the disc has a radius of MOON_RADIUS_DEGREES), a smaller difference goes unnoticed.
#
# Known limit: the picture is fixed. Moving the moon light in the Lights panel of the game
# changes the light on the walls, and the painted moon stays where it is.
MOON_LIGHT_YAW_DEGREES = 25.0
MOON_LIGHT_PITCH_DEGREES = -50.0

# Radius of the disc in degrees. The real moon is about 0.26 degrees: far too small to
# read as a moon in a game, so it is drawn several times larger.
MOON_RADIUS_DEGREES = 2.2
# The edge of the disc fades over this many degrees instead of ending in a hard step.
MOON_EDGE_DEGREES = 0.2
MOON_COLOR = (0.86, 0.89, 0.96)
# The grey patches on the disc: how much of the brightness they take away at most, and
# how many patches fit across the sky (the frequency of the noise).
MOON_PATCH_DEPTH = 0.22
MOON_PATCH_FREQUENCY = 28.0

# The halo is two soft glows around the disc: a small bright one and a wide faint one.
# Each is a Gaussian of the angle to the middle of the moon, the number is its sigma.
MOON_GLOW_SIGMA_DEGREES = 3.5
MOON_GLOW_COLOR = (0.16, 0.19, 0.27)
MOON_HALO_SIGMA_DEGREES = 11.0
MOON_HALO_COLOR = (0.045, 0.055, 0.085)

# ---- the background ---------------------------------------------------------------------
# A deep blue straight above and straight below, slightly lighter at the horizon. The
# zenith is close to the clear colour of the game ({0.01, 0.015, 0.04}).
ZENITH_COLOR = (0.010, 0.016, 0.045)
HORIZON_COLOR = (0.034, 0.050, 0.098)
# How quickly the horizon colour fades with height: a larger number keeps the light band
# closer to the horizon.
HORIZON_FALLOFF = 2.5

# ---- the Milky Way ----------------------------------------------------------------------
# A soft band along a great circle of the sky. The band is the set of directions at right
# angles to this vector. It is tilted against all three axes on purpose: the band then
# crosses every face at a slant, and a face that is flipped or mirrored by mistake shows
# as a break in the band.
MILKY_WAY_AXIS = (0.55, 0.45, 0.70)
# Half width of the band in degrees (the sigma of its Gaussian profile).
MILKY_WAY_WIDTH_DEGREES = 10.0
MILKY_WAY_COLOR = (0.050, 0.058, 0.090)
# The band is broken up by two layers of noise: large clouds and smaller ones.
MILKY_WAY_CLOUD_FREQUENCY = 3.0
MILKY_WAY_DETAIL_FREQUENCY = 8.0
# The part of the band that stays when the noise is at its lowest.
MILKY_WAY_FLOOR = 0.25
# The two layers of noise are multiplied. Each is 0.5 on average, so their product is
# about 0.25 and seldom above 0.5. This factor brings it back to the range from 0 to 1.
MILKY_WAY_NOISE_GAIN = 2.0

# ---- the stars --------------------------------------------------------------------------
# Stars spread evenly over the whole sky, and more of them gathered along the Milky Way.
STAR_COUNT = 2600
MILKY_WAY_STAR_COUNT = 1900
# How far the stars of the Milky Way stray from the middle of the band, in degrees.
MILKY_WAY_STAR_SPREAD_DEGREES = 9.0
# Brightness of a star is STAR_MIN_BRIGHTNESS + (1 - STAR_MIN_BRIGHTNESS) * u ^ STAR_RARITY
# with u random from 0 to 1. The power makes bright stars rare: most are dim.
STAR_MIN_BRIGHTNESS = 0.10
STAR_RARITY = 5.0
# A star is a small Gaussian spot. Its sigma in degrees grows with its brightness, from
# the first number to the second. The smallest is about half a pixel: a smaller spot
# could fall between the pixels and disappear.
STAR_MIN_SIGMA_DEGREES = 0.06
STAR_MAX_SIGMA_DEGREES = 0.17
# A spot is computed up to this many sigmas from its middle. Farther out it is below one
# level of an 8-bit channel.
STAR_REACH_SIGMAS = 3.5
# A star is drawn on a face only when the cosine of the angle between the star and the
# middle of the face is at least this. The corners of a face are at a cosine of 0.58, so
# every star on a face or just across its border passes, and the stars on the far side
# of the sky are skipped without any work.
STAR_MIN_FACING = 0.2
# In the middle of a face one pixel covers 2 / SIZE of a radian. Towards the corners the
# face is seen at a slant and a pixel covers less, down to a third of that at the very
# corner. A spot of a given angle therefore reaches over up to three times as many
# pixels there.
CORNER_PIXEL_DENSITY = 3.0
# This share of the stars is faintly coloured, half of them warm and half of them cool.
# The rest is white. STAR_TINT_STRENGTH says how far a coloured star moves from white.
STAR_TINTED_SHARE = 0.14
STAR_WARM_COLOR = (1.0, 0.78, 0.55)
STAR_COOL_COLOR = (0.62, 0.78, 1.0)
STAR_TINT_STRENGTH = 0.6

# ---- the noise --------------------------------------------------------------------------
# The noise is a cube of NOISE_GRID ^ 3 random numbers that fills space and repeats.
NOISE_GRID = 32
# Directions have coordinates from -1 to 1. They are moved by this much before the noise
# is looked up, so that the grid coordinates are never negative.
NOISE_OFFSET = 64.0


def normalized(vectors):
    """Returns the vectors (an array whose last axis is x, y, z) scaled to length 1."""
    return vectors / np.linalg.norm(vectors, axis=-1, keepdims=True)


def direction_from_angles(yaw_degrees, pitch_degrees):
    """The same formula as scene::directionFromAngles in src/scene/Light.cpp.

    Yaw like a compass (0 towards -Z, 90 towards +X), pitch above 0 means upwards.
    """
    yaw = math.radians(yaw_degrees)
    pitch = math.radians(pitch_degrees)
    horizontal = math.cos(pitch)
    return np.array([horizontal * math.sin(yaw), math.sin(pitch), -horizontal * math.cos(yaw)])


def face_directions(forward, right, down):
    """Returns the direction every pixel of one face is seen in: SIZE x SIZE x 3, length 1.

    Row 0 of the result is the TOP row of the picture, the order of the PNG file.
    """
    # The middle of pixel number i is at (i + 0.5) / SIZE of the way across the face.
    # Going from 0..1 to -1..1 gives the a and b of the table in FACES.
    centres = (np.arange(SIZE) + 0.5) / SIZE * 2.0 - 1.0
    # indexing="ij": the first index of both arrays is the row (b), the second the
    # column (a).
    b, a = np.meshgrid(centres, centres, indexing="ij")
    directions = (
        np.array(forward)
        + a[..., np.newaxis] * np.array(right)
        + b[..., np.newaxis] * np.array(down)
    )
    return normalized(directions)


def smooth_step(t):
    """The curve 3t^2 - 2t^3 for t from 0 to 1: it starts and ends flat."""
    return t * t * (3.0 - 2.0 * t)


def value_noise(lattice, directions, frequency):
    """Returns soft random patches as a function of the direction: values from 0 to 1.

    lattice is the cube of random numbers. A direction is a point in space (on the sphere
    of radius 1), scaled by `frequency`: the eight numbers at the corners of the grid cell
    the point lies in are blended by how close the point is to each corner. Because the
    result depends only on the point in space, it does not know about the faces of the
    cube map and has no seams.
    """
    points = directions * frequency + NOISE_OFFSET
    cell = np.floor(points).astype(np.int64)
    inside = smooth_step(points - cell)

    # The grid repeats: the neighbour of the last number is the first one.
    x0, y0, z0 = (cell[..., axis] % NOISE_GRID for axis in range(3))
    x1, y1, z1 = ((index + 1) % NOISE_GRID for index in (x0, y0, z0))
    fx, fy, fz = (inside[..., axis] for axis in range(3))

    # Blend along x on the four edges of the cell, then along y, then along z.
    bottom_near = lattice[x0, y0, z0] * (1.0 - fx) + lattice[x1, y0, z0] * fx
    top_near = lattice[x0, y1, z0] * (1.0 - fx) + lattice[x1, y1, z0] * fx
    bottom_far = lattice[x0, y0, z1] * (1.0 - fx) + lattice[x1, y0, z1] * fx
    top_far = lattice[x0, y1, z1] * (1.0 - fx) + lattice[x1, y1, z1] * fx
    near = bottom_near * (1.0 - fy) + top_near * fy
    far = bottom_far * (1.0 - fy) + top_far * fy
    return near * (1.0 - fz) + far * fz


def angle_between(directions, target):
    """Returns the angle in degrees between every direction and the direction `target`."""
    cosine = np.clip(np.sum(directions * target, axis=-1), -1.0, 1.0)
    return np.degrees(np.arccos(cosine))


def background(directions):
    """Returns the colour of the empty sky: dark at the top, lighter at the horizon."""
    # The y of a direction is the sine of its height above the horizon: 0 at the horizon,
    # 1 straight up, -1 straight down. The sky below the horizon mirrors the sky above.
    height = np.abs(directions[..., 1])
    near_horizon = (1.0 - height) ** HORIZON_FALLOFF
    zenith = np.array(ZENITH_COLOR)
    horizon = np.array(HORIZON_COLOR)
    return zenith + near_horizon[..., np.newaxis] * (horizon - zenith)


def milky_way(directions, lattice, axis):
    """Returns the light of the Milky Way band, to be added to the background."""
    # The sine of the angle between a direction and the plane of the band is its dot
    # product with the axis of the band. Converted to degrees it is the distance from
    # the middle line of the band.
    sine = np.clip(np.sum(directions * axis, axis=-1), -1.0, 1.0)
    distance = np.degrees(np.arcsin(sine))
    profile = np.exp(-((distance / MILKY_WAY_WIDTH_DEGREES) ** 2))

    clouds = value_noise(lattice, directions, MILKY_WAY_CLOUD_FREQUENCY)
    detail = value_noise(lattice, directions, MILKY_WAY_DETAIL_FREQUENCY)
    # Multiplying the two layers gives dark lanes where either of them is low.
    density = MILKY_WAY_FLOOR + (1.0 - MILKY_WAY_FLOOR) * clouds * detail * MILKY_WAY_NOISE_GAIN
    strength = profile * np.clip(density, 0.0, 1.0)
    return strength[..., np.newaxis] * np.array(MILKY_WAY_COLOR)


def moon_layers(directions, lattice, moon_direction):
    """Returns (coverage, disc colour, halo light) of the moon for every pixel.

    coverage is 1 inside the disc, 0 outside and in between on its soft edge.
    """
    angle = angle_between(directions, moon_direction)

    edge = np.clip((MOON_RADIUS_DEGREES - angle) / MOON_EDGE_DEGREES + 0.5, 0.0, 1.0)
    coverage = smooth_step(edge)

    patches = value_noise(lattice, directions, MOON_PATCH_FREQUENCY)
    disc = np.array(MOON_COLOR) * (1.0 - MOON_PATCH_DEPTH * patches[..., np.newaxis])

    glow = np.exp(-((angle / MOON_GLOW_SIGMA_DEGREES) ** 2))
    halo = np.exp(-((angle / MOON_HALO_SIGMA_DEGREES) ** 2))
    light = glow[..., np.newaxis] * np.array(MOON_GLOW_COLOR) + halo[..., np.newaxis] * np.array(
        MOON_HALO_COLOR
    )
    return coverage, disc, light


def make_stars(rng, milky_way_axis):
    """Returns the stars as three arrays: directions (N x 3), brightness (N), colour (N x 3)."""
    # Three independent normal random numbers, scaled to length 1, are a direction that
    # is equally likely to point anywhere on the sphere.
    everywhere = normalized(rng.normal(size=(STAR_COUNT, 3)))

    # The stars of the band start as random directions too. Each one is then moved onto
    # the plane of the band (its part along the axis is taken away) and pushed back out
    # of that plane by a small random amount, so the stars thin out towards the edges.
    in_band = normalized(rng.normal(size=(MILKY_WAY_STAR_COUNT, 3)))
    along_axis = np.sum(in_band * milky_way_axis, axis=-1, keepdims=True)
    in_band = normalized(in_band - along_axis * milky_way_axis)
    stray_degrees = rng.normal(
        scale=MILKY_WAY_STAR_SPREAD_DEGREES, size=(MILKY_WAY_STAR_COUNT, 1)
    )
    in_band = normalized(in_band + np.sin(np.radians(stray_degrees)) * milky_way_axis)

    directions = np.concatenate([everywhere, in_band])
    count = len(directions)

    rarity = rng.random(count) ** STAR_RARITY
    brightness = STAR_MIN_BRIGHTNESS + (1.0 - STAR_MIN_BRIGHTNESS) * rarity

    # Colour: white, or white moved towards the warm or the cool colour.
    color = np.ones((count, 3))
    kind = rng.random(count)
    warm = kind < STAR_TINTED_SHARE / 2.0
    cool = (kind >= STAR_TINTED_SHARE / 2.0) & (kind < STAR_TINTED_SHARE)
    color[warm] += STAR_TINT_STRENGTH * (np.array(STAR_WARM_COLOR) - 1.0)
    color[cool] += STAR_TINT_STRENGTH * (np.array(STAR_COOL_COLOR) - 1.0)
    return directions, brightness, color


def add_stars(image, directions, forward, right, down, stars):
    """Adds to one face (image, SIZE x SIZE x 3) every star that shines onto it.

    A star is a spot around a direction, not around a pixel. The spot of a star close to
    a border is drawn on both faces, each face computing its own part from the directions
    of its own pixels, so the two halves fit together.
    """
    star_directions, star_brightness, star_colors = stars
    forward = np.array(forward)
    right = np.array(right)
    down = np.array(down)

    for star, brightness, color in zip(star_directions, star_brightness, star_colors):
        # Where the star lies on the plane of this face. A star behind the plane, or
        # nearly parallel to it, cannot reach the face.
        depth = float(np.dot(star, forward))
        if depth < STAR_MIN_FACING:
            continue
        a = float(np.dot(star, right)) / depth
        b = float(np.dot(star, down)) / depth
        column = (a + 1.0) / 2.0 * SIZE - 0.5
        row = (b + 1.0) / 2.0 * SIZE - 0.5

        sigma = (
            STAR_MIN_SIGMA_DEGREES
            + (STAR_MAX_SIGMA_DEGREES - STAR_MIN_SIGMA_DEGREES) * brightness
        )
        # How many pixels the spot may reach from its middle, counted for the corner of
        # a face, where it is the most (see CORNER_PIXEL_DENSITY). One more for rounding.
        reach_radians = math.radians(STAR_REACH_SIGMAS * sigma)
        reach = int(math.ceil(reach_radians * SIZE / 2.0 * CORNER_PIXEL_DENSITY)) + 1

        first_row = max(int(math.floor(row)) - reach, 0)
        last_row = min(int(math.floor(row)) + reach + 1, SIZE - 1)
        first_column = max(int(math.floor(column)) - reach, 0)
        last_column = min(int(math.floor(column)) + reach + 1, SIZE - 1)
        if first_row > last_row or first_column > last_column:
            continue  # the spot lies entirely outside of this face

        window = directions[first_row : last_row + 1, first_column : last_column + 1]
        angle = angle_between(window, star)
        spot = brightness * np.exp(-0.5 * (angle / sigma) ** 2)
        image[first_row : last_row + 1, first_column : last_column + 1] += (
            spot[..., np.newaxis] * color
        )


def save_face(rng, color, file_name):
    """Saves a SIZE x SIZE x 3 array of colours from 0 to 1 (row 0 = top) as an RGB PNG."""
    # An 8-bit channel has 256 levels, and the dark sky uses only a few of them: without
    # help the gradient would show as wide bands with visible steps between them. Adding
    # a random amount of up to half a level before rounding (dithering) breaks the steps
    # up into fine grain that the eye averages back into a smooth gradient.
    dither = rng.random(color.shape) - 0.5
    levels = np.clip(np.round(color * 255.0 + dither), 0.0, 255.0) / 255.0

    # A Blender image without alpha is still filled with 4 values per pixel. The fourth
    # one stays 1 (opaque) and is not written to the file.
    rgba = np.ones((SIZE, SIZE, 4), dtype=np.float32)
    rgba[..., :3] = levels

    # Blender stores images BOTTOM row first, the arrays here are top row first. Turning
    # the rows around here makes the top row of the array the top row of the PNG file.
    rgba = rgba[::-1]

    image = bpy.data.images.new(file_name, SIZE, SIZE, alpha=False)
    image.pixels.foreach_set(rgba.ravel())

    os.makedirs(common.SKYBOX_DIR, exist_ok=True)
    image.filepath_raw = os.path.join(common.SKYBOX_DIR, file_name)
    image.file_format = "PNG"
    image.save()
    print("Wrote", image.filepath_raw)


def build():
    # One generator for everything, used in a fixed order: the same seed then gives the
    # same six files on every run.
    rng = np.random.default_rng(SKY_SEED)
    lattice = rng.random((NOISE_GRID, NOISE_GRID, NOISE_GRID))

    milky_way_axis = normalized(np.array(MILKY_WAY_AXIS))
    stars = make_stars(rng, milky_way_axis)

    # The light travels along direction_from_angles(...), so it comes FROM the opposite
    # direction: that is where the disc is.
    moon_direction = -direction_from_angles(MOON_LIGHT_YAW_DEGREES, MOON_LIGHT_PITCH_DEGREES)

    for file_name, forward, right, down in FACES:
        directions = face_directions(forward, right, down)

        color = background(directions)
        color += milky_way(directions, lattice, milky_way_axis)
        add_stars(color, directions, forward, right, down, stars)

        coverage, disc, light = moon_layers(directions, lattice, moon_direction)
        color += light
        # The disc hides what is behind it (stars, the Milky Way) instead of adding to it.
        coverage = coverage[..., np.newaxis]
        color = color * (1.0 - coverage) + disc * coverage

        save_face(rng, np.clip(color, 0.0, 1.0), file_name)


if __name__ == "__main__":
    build()
