# Generates the textures of the game into assets/textures: the colour pictures
# wall_stone.png, wall_cracked.png, wall_mossy.png, wall_damaged.png, ground.png,
# gate_wood.png, lever_iron.png, chalk.png, flask.png and lantern.png, and one normal map
# for each of them (the same name with _normal). All twenty are 512 x 512 pixels, 8 bits
# per channel, RGB. The pictures of the generated models are not made here: each build
# script takes them from the model it starts from (see generated_common.py). The four
# wall pictures are not drawn here either: they are made from generated pictures and
# baked normal maps in tools/blender/sources (see plain_wall).
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_textures.py
#
# The textures are sampled with GL_REPEAT, so the left edge has to continue the right edge
# and the bottom edge has to continue the top edge. Every step below keeps that property:
# the planks divide the image evenly, the noise is smoothed with wrap-around, and the
# distances to the stones of the ground are measured across the edges. The wall pictures
# are made seamless (see make_seamless). The exceptions are flask.png and lantern.png: the
# picture of the flask goes once around its model, so only its left and right edges meet,
# and the lantern only uses a small piece from the middle of each half of its picture.
# Their noise still wraps around, because it comes from the same helpers.
#
# A colour picture and its normal map are made from the same pattern (the same stones, the
# same joints, the same noise), so the relief lies exactly where the picture shows it.
# The colour picture still carries its own shading (the darker rim of every stone): it was
# made before the game had lighting, and it is kept as it is.
#
# Convention of the normal maps. A normal map stores, for every texel, the direction the
# surface faces there, in tangent space: +X is the direction in which u grows (to the
# right in the picture), +Y the direction in which v grows (UP in the picture, the OpenGL
# convention), +Z points out of the surface. Each component goes from -1..1 to 0..1
# (colour = normal * 0.5 + 0.5), so a flat surface (0, 0, 1) is the colour (128, 128, 255).
# Programs that follow the DirectX convention store -Y in the green channel instead. Such
# a map would make every horizontal joint look like a ridge here.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import bpy
import numpy as np

import blender_common as common

# Width and height of every texture in pixels. A power of two, and 256 pixels per metre
# with the texel density of the stone and gate models (one repeat = 2 m). The ground is
# the exception: the terrain repeats its texture every 4 m, so there it is 128 pixels per
# metre.
SIZE = 512

# The same seeds give the same pictures on every run. Change a seed to get another
# arrangement of light and dark stones.
GROUND_SEED = 67
GATE_SEED = 37
LEVER_SEED = 53
# The damaged wall shows the stones of the plain wall. This seed only shapes the
# outline and the rubble of its niches.
WALL_DAMAGED_SEED = 97
FLASK_SEED = 101
LANTERN_IRON_SEED = 107
LANTERN_GLASS_SEED = 109
CHALK_SEED = 113

# The wall pictures are not drawn here. They start from pictures that Tripo's Image tab
# generated and from the normal maps bake_wall_normals.py baked from the wall slabs Tripo
# generated from two of them, all in this folder (see docs/generation.md, "Wall
# pictures"). A source is 1024 x 1024 for 2 m of wall, twice the texels of the game's
# pictures, and is halved at the end.
SOURCES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sources")

# The size of a source picture: 1024 x 1024 for 2 m of wall.
WALL_SOURCE_SIZE = 1024
# make_seamless lays the last WALL_OVERLAP columns of a picture over its first ones, and
# then the last rows over the first ones, and cuts through the overlap along the cheapest
# path. The path keeps WALL_CUT_MARGIN pixels away from the edges of the overlap. It moves
# by up to WALL_CUT_STEP pixels from one row to the next, so it can follow a joint that
# runs at a slant, and every WALL_CUT_START_SPACING-th column is tried as its start.
WALL_OVERLAP = 128
WALL_CUT_MARGIN = 16
WALL_CUT_STEP = 3
WALL_CUT_START_SPACING = 4

# The joints of the source pictures, by brightness (sRGB, Rec. 709 weights): darker than
# WALL_JOINT_ABOVE - WALL_JOINT_SOFTNESS is joint, brighter than WALL_JOINT_ABOVE is stone.
WALL_JOINT_ABOVE = 0.16
WALL_JOINT_SOFTNESS = 0.06
# The joints of the sources are close to black. Pixels darker than WALL_LIFT_BELOW are
# drawn towards WALL_JOINT_COLOR, the darkest ones all of the way.
WALL_LIFT_BELOW = 0.22
WALL_JOINT_COLOR = (0.17, 0.15, 0.13)

# How much light every wall picture reflects on average (sRGB decoded, Rec. 709 weights).
# The generated plain picture as it comes reflects 0.18, and the stones the script drew
# before it reflected 0.29. At 0.29 the pale stones of the generated pictures run into
# white (one pixel in a hundred); 0.24 is about as bright as they go without that.
WALL_LIGHT = 0.24
# A worn wall may differ from the plain wall by this share of its light, and the share of
# a channel in the sum of the three by this much (the limits of tests/ImageLoaderTests.cpp).
# The pictures keep their own colour: the moss stays green.
WALL_LIGHT_TOLERANCE = 0.05
WALL_COLOUR_SHARE_TOLERANCE = 0.03

# The generated picture of the plain wall is two copies of one picture stacked: it repeats
# after WALL_STONE_PERIOD rows. Stretched over 2 m of wall that showed the same stones
# every 1.1 m. So only one copy is kept, and the other half of the picture is that copy
# mirrored left to right and moved by half its width (unrepeated_plain).
WALL_STONE_PERIOD = 512
# The two halves meet along a cut through this many rows around the seam, the cheapest
# path as in seamless_across: where the halves differ least and are darkest.
WALL_HALF_SEAM_BAND = 128

# Every wall picture is moved and stretched up and down so that a level joint lies at
# v = 0 and another WALL_SECOND_COURSE of the picture above it (align_courses). The wall
# models put v = 0 at the top of the plinth (0.25 m) and so the second joint at the
# underside of the coping (2.85 m, 1.3 pictures up): both edges follow a joint instead of
# cutting through stones. A joint row is one where the most pixels are joint, averaged
# over WALL_COURSE_SMOOTHING rows to each side; the second is looked for up to
# WALL_COURSE_SEARCH of the picture from where it should lie.
WALL_SECOND_COURSE = 0.3
WALL_COURSE_SMOOTHING = 2
WALL_COURSE_SEARCH = 0.06

# The baked relief is used as it was baked.
WALL_BAKED_RELIEF = 1.0
# The relief made from a picture, in pixels of the source (1/512 m): how much higher the
# brightest pixel is than the darkest, and how deep a joint lies on top of that.
WALL_PICTURE_RISE = 10.0
WALL_PICTURE_JOINT_DEPTH = 8.0

# The stones that are missing in the damaged wall, as one pixel (column, row from the
# bottom) on each, in the seamless plain picture at the size of the source. The wall model
# is 3 m high, one repeat of the picture is 2 m and v = 0 is the top of the plinth: the
# plinth shows the rows from 896 up, the coping the rows from 307 to 384, and the body
# every row once and the rows up to 307 a second time. Both stones lie between the rows
# 384 and 896, so a niche is not seen twice, and they do not touch.
WALL_MISSING_STONES = ((470, 839), (720, 659))
# A stone grows from its pixel by one pixel per step: this many steps reach across the
# largest stone. If it still grows after them, it has run into a joint that is too light.
WALL_STONE_GROWTH = 300
WALL_STONE_SHRINK = 5
# A stone smaller than this many pixels is a chip, not a stone that broke out.
WALL_STONE_SMALLEST = 3000
# The colour of the rubble in a niche, and how deep its floor lies, in pixels of the
# source behind the face of the stones (24 pixels are under 5 cm).
WALL_NICHE_COLOR = (0.20, 0.19, 0.18)
WALL_NICHE_DEPTH = 24.0

# The bands of the picture of the flask, in pixel rows from the bottom. The model
# (build_flask.py) puts the height of the flask on the y axis of the picture: 512 rows
# are its 0.25 m, so 1 cm is about 20 rows.
# Below this row, on average, the body is bare clay, above it glazed.
FLASK_GLAZE_ROW = 150
# The cord around the neck, from 16.5 cm to 19 cm, and the height of one turn of it.
FLASK_CORD_ROWS = (338, 389)
FLASK_CORD_TURN_HEIGHT = 8.5
# From this row up the picture shows the cork (the lip ends at 21.2 cm).
FLASK_CORK_ROW = 433

# The iron bands of the gate, in pixels: the rows of the band centres and half of the band
# height. One repeat of the texture is 2 m high and the gate is taller, so the lower band
# is seen twice: at 256 pixels per metre the bands lie at 0.375 m, 1.375 m and 2.375 m.
# build_gate.py raises the geometry at the same heights.
GATE_BAND_CENTRES = (96, 352)
GATE_BAND_HALF_HEIGHT = 20

# How far the two kinds of noise are blurred once more before they become relief, in
# pixels. A normal map shows the SLOPE of the height, and the slope of noise that was
# blurred once is jagged: from one pixel to the next the average loses one random value
# and gains another. On the first walls that looked like woven cloth. After the second blur
# the slope is smooth.
BUMP_BLUR_RADIUS = 8
GRAIN_BLUR_RADIUS = 1


def blur(values, radius):
    """Returns the array with every pixel averaged with its neighbours. It is SIZE x SIZE
    for the pictures made here; the wall pictures are blurred at the size of their source.

    The neighbours are the pixels up to `radius` pixels away, first along one axis, then
    along the other. np.roll shifts the array and moves what falls off one edge to the
    opposite edge, so the pixels at the border are averaged with the pixels on the other
    side and the result tiles.
    """
    for axis in (0, 1):
        total = np.zeros(values.shape)
        for shift in range(-radius, radius + 1):
            total += np.roll(values, shift, axis=axis)
        values = total / (2 * radius + 1)
    return values


def blur_along(values, radius, axis):
    """Like blur, but along one axis only: axis 0 is y (up the picture), axis 1 is x.

    A long blur along y and a short one along x turns random pixels into the streaks of
    wood grain.
    """
    total = np.zeros((SIZE, SIZE))
    for shift in range(-radius, radius + 1):
        total += np.roll(values, shift, axis=axis)
    return total / (2 * radius + 1)


def stretch(values):
    """Returns the array moved and scaled so that its values run from 0 to 1."""
    return (values - values.min()) / (values.max() - values.min())


def smooth_step(t):
    """The curve 3t^2 - 2t^3 for t from 0 to 1: it starts and ends flat."""
    return t * t * (3.0 - 2.0 * t)


def smooth_noise(rng, radius):
    """Returns a SIZE x SIZE array of random values from 0 to 1 that tiles.

    It starts as one independent random value per pixel. Blurring turns that into soft
    patches, the larger the radius, the larger the patches.
    """
    noise = blur(rng.random((SIZE, SIZE)), radius)

    # Averaging pulls all values towards 0.5. Stretch them back to the full 0 to 1 range.
    return (noise - noise.min()) / (noise.max() - noise.min())








def normal_map(height):
    """Turns a height field into a SIZE x SIZE x 3 array of colors from 0 to 1.

    The normal of the surface z = height(x, y) is (-dheight/dx, -dheight/dy, 1), brought
    to length 1: where the height grows towards +x, the surface leans back towards -x.
    """
    # Slope along x and along y: the difference between the two neighbours of a pixel,
    # divided by their distance (2 pixels). np.roll takes the neighbour of a border pixel
    # from the opposite border, which keeps the map tileable. Axis 1 is x. Axis 0 is y,
    # and row 0 is the bottom row, so +y is up in the picture: the green channel follows
    # the OpenGL convention (+Y up) without any sign flip.
    slope_x = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / 2.0
    slope_y = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) / 2.0

    normal = np.stack([-slope_x, -slope_y, np.ones((SIZE, SIZE))], axis=-1)
    normal = normal / np.linalg.norm(normal, axis=-1, keepdims=True)

    # From -1..1 to 0..1. A flat surface (0, 0, 1) becomes (0.5, 0.5, 1.0), which save_png
    # rounds to the bytes (128, 128, 255).
    return normal * 0.5 + 0.5


def wood_pattern(seed, plank_width, band_centres, band_half_height):
    """Returns what the colour picture and the normal map of the gate have in common.

    The gate is made of upright planks, held together by horizontal iron bands with one
    rivet per plank.

    plank_width: width of one plank in pixels. It must divide SIZE.
    band_centres: the rows of the band centres in pixels.
    band_half_height: half of the height of a band in pixels.

    The result is a dictionary. Its arrays are SIZE x SIZE, one value per pixel:
      "edge_distance":     distance to the nearest edge of its plank, in pixels
      "plank_brightness":  the random brightness of its plank
      "band_depth":        how deep the pixel lies inside a band, in pixels. Zero and
                           below means outside of every band
      "rivet_distance":    distance to the nearest rivet, in pixels
      "streaks":           the grain of the wood, long along y, from 0 to 1
      "patches", "grain":  soft noise and fine noise, both from 0 to 1
    """
    rng = np.random.default_rng(seed)
    columns = SIZE // plank_width

    # Pixel coordinates. Row 0 is the bottom row of the image.
    y, x = np.mgrid[0:SIZE, 0:SIZE]

    column = x // plank_width
    inside_x = x % plank_width
    edge_distance = np.minimum(inside_x, plank_width - 1 - inside_x)

    # One random brightness per plank, looked up for every pixel.
    plank_brightness = rng.uniform(0.82, 1.12, columns)[column]

    # Distance of the centre of every pixel to the centre line of the nearest band. The
    # bands are far from the top and bottom edge, so no wrap-around is needed.
    band_distance = np.full((SIZE, SIZE), float(SIZE))
    for centre in band_centres:
        band_distance = np.minimum(band_distance, np.abs(y + 0.5 - centre))
    band_depth = band_half_height - band_distance

    # One rivet in the middle of every plank, on the centre line of every band.
    rivet_x = inside_x + 0.5 - plank_width / 2
    rivet_distance = np.sqrt(rivet_x * rivet_x + band_distance * band_distance)

    # Grain: random pixels blurred far along y and only a little along x.
    streaks = stretch(blur_along(blur_along(rng.random((SIZE, SIZE)), 48, axis=0), 1, axis=1))
    patches = smooth_noise(rng, 6)
    grain = smooth_noise(rng, 1)

    return {
        "edge_distance": edge_distance,
        "plank_brightness": plank_brightness,
        "band_depth": band_depth,
        "rivet_distance": rivet_distance,
        "streaks": streaks,
        "patches": patches,
        "grain": grain,
    }


def wood_color(pattern, gap_width, rim_width, rivet_radius, wood_color, gap_color, iron_color):
    """Returns a SIZE x SIZE x 3 array of colors from 0 to 1: planks with iron bands.

    pattern: the result of wood_pattern.
    gap_width: width of the gap between two planks in pixels.
    rim_width: how far from the gap a plank is still darkened, in pixels.
    rivet_radius: radius of a rivet in pixels.
    wood_color, gap_color, iron_color: (red, green, blue) from 0 to 1.
    """
    edge_distance = pattern["edge_distance"]
    band_depth = pattern["band_depth"]
    grain = pattern["grain"]

    # Wood: the brightness of the plank, the streaks of the grain and the fine noise.
    brightness = pattern["plank_brightness"]
    brightness = brightness * (0.74 + 0.52 * pattern["streaks"]) * (0.92 + 0.16 * grain)
    # Darker towards the gap, like the rim of the stones.
    rim = np.clip(edge_distance / rim_width, 0.0, 1.0)
    brightness = brightness * (0.72 + 0.28 * rim)
    color = brightness[..., None] * np.array(wood_color)

    # The gap is the outer half gap_width of every plank.
    gap = edge_distance < gap_width // 2
    gap_shade = 0.85 + 0.30 * grain
    color[gap] = gap_shade[gap][..., None] * np.array(gap_color)

    # Iron band: it covers the planks and the gaps. Soft patches make it look hammered,
    # and the 3 pixels next to its edge are darker, which separates it from the wood.
    band = band_depth > 0.0
    iron_shade = (0.80 + 0.40 * pattern["patches"]) * (0.92 + 0.16 * grain)
    iron_shade = iron_shade * (0.65 + 0.35 * np.clip(band_depth / 3.0, 0.0, 1.0))
    # Rivets are lighter than the band: 1.5 times in the middle, falling to 1 at the rim.
    rivet = np.clip(1.0 - pattern["rivet_distance"] / rivet_radius, 0.0, 1.0)
    iron_shade = iron_shade * (1.0 + 0.5 * rivet)
    color[band] = iron_shade[band][..., None] * np.array(iron_color)

    return np.clip(color, 0.0, 1.0)


def wood_height(
    pattern, gap_width, bevel_width, gap_depth, grain_depth, band_rise, rivet_radius, rivet_rise
):
    """Returns a SIZE x SIZE array: how far every pixel of the gate stands out.

    The unit is the size of one pixel of the texture (1 / 256 m on the models), so a
    height difference of 1 between two neighbouring pixels is a slope of 45 degrees.

    pattern: the result of wood_pattern, the same one the colour picture was made from.
    gap_width: width of the gap in pixels, the same number as for the colour picture.
    bevel_width: over how many pixels a plank rises from the gap to its face.
    gap_depth: how far the face of a plank stands in front of the gap.
    grain_depth: depth of the grooves of the grain.
    band_rise: how far an iron band stands in front of the planks.
    rivet_radius: radius of a rivet in pixels, the same number as for the colour picture.
    rivet_rise: how far the middle of a rivet stands in front of its band.
    """
    edge_distance = pattern["edge_distance"]
    band_depth = pattern["band_depth"]

    # The profile across a plank: 0 in the gap, a smooth rise, then 1 on the face.
    profile = smooth_step(np.clip((edge_distance - (gap_width // 2 - 1)) / bevel_width, 0.0, 1.0))

    # The streaks become grooves. They are blurred once more across the grain, for the
    # same reason as the noise (BUMP_BLUR_RADIUS): the slope has to be smooth.
    grooves = grain_depth * (blur_along(pattern["streaks"], 1, axis=1) - 0.5)
    fine = 0.5 * (blur(pattern["grain"], GRAIN_BLUR_RADIUS) - 0.5)
    wood = profile * (gap_depth + grooves) + fine

    # The band lies on the faces of the planks (height gap_depth) and rises over 3 pixels
    # from its edge. Soft dents make it look hammered.
    band_profile = smooth_step(np.clip(band_depth / 3.0, 0.0, 1.0))
    dents = 2.0 * (blur(pattern["patches"], 4) - 0.5)
    iron = gap_depth + band_profile * (band_rise + dents) + fine

    # A rivet is a dome: the upper half of a ball, flattened to the height rivet_rise.
    inside = np.clip(1.0 - (pattern["rivet_distance"] / rivet_radius) ** 2, 0.0, 1.0)
    iron = iron + rivet_rise * np.sqrt(inside)

    return np.where(band_depth > 0.0, iron, wood)








def ground_pattern(seed, stone_count, min_radius, max_radius):
    """Returns what the colour picture and the normal map of the ground have in common.

    The ground is packed earth with soft patches of moss and small stones pressed into it.

    stone_count: how many stones lie on one repeat of the texture.
    min_radius, max_radius: the size range of a stone, in pixels.

    The result is a dictionary. Its arrays are SIZE x SIZE, one value per pixel:
      "stone_depth":       how deep the pixel lies inside a stone, from 0 at its rim to
                           1 in its middle. Zero and below means outside of every stone
      "stone_brightness":  the random brightness of the nearest stone
      "moss":              how much moss covers the pixel, from 0 to 1
      "patches", "grain":  large soft noise and fine noise, both from 0 to 1
    """
    rng = np.random.default_rng(seed)

    # Pixel coordinates. Row 0 is the bottom row of the image.
    y, x = np.mgrid[0:SIZE, 0:SIZE]

    # Random places and sizes of the stones, and a brightness for each.
    centres_x = rng.uniform(0.0, SIZE, stone_count)
    centres_y = rng.uniform(0.0, SIZE, stone_count)
    radii = rng.uniform(min_radius, max_radius, stone_count)
    brightnesses = rng.uniform(0.75, 1.05, stone_count)

    # Soft noise that pushes the rim of every stone in and out, so that no stone is
    # a perfect circle.
    outline = smooth_noise(rng, 5)

    stone_depth = np.full((SIZE, SIZE), -1.0)
    stone_brightness = np.ones((SIZE, SIZE))
    for stone in range(stone_count):
        # Distance to the centre along each axis, the short way round: a stone near the
        # right edge continues at the left edge, so the texture tiles.
        offset_x = np.abs(x + 0.5 - centres_x[stone])
        offset_x = np.minimum(offset_x, SIZE - offset_x)
        offset_y = np.abs(y + 0.5 - centres_y[stone])
        offset_y = np.minimum(offset_y, SIZE - offset_y)
        distance = np.sqrt(offset_x * offset_x + offset_y * offset_y)

        # 1 in the middle of the stone, 0 on its rim, negative outside. The radius
        # changes with the noise by up to a quarter in each direction.
        depth = 1.0 - distance / (radii[stone] * (0.75 + 0.5 * outline))
        # Where two stones overlap, the one the pixel lies deeper in wins.
        nearer = depth > stone_depth
        stone_depth[nearer] = depth[nearer]
        stone_brightness[nearer] = brightnesses[stone]

    # Moss grows in patches: wide soft noise, and only its upper part counts as moss. The
    # edge of a patch is a smooth ramp, not a line.
    # The noise is blurred a second time: one blur over a square of pixels leaves patches
    # with straight edges, the second one rounds them.
    moss_noise = stretch(blur(smooth_noise(rng, 24), 16))
    moss = smooth_step(np.clip((moss_noise - 0.50) / 0.20, 0.0, 1.0))

    patches = smooth_noise(rng, 14)
    grain = smooth_noise(rng, 1)

    return {
        "stone_depth": stone_depth,
        "stone_brightness": stone_brightness,
        "moss": moss,
        "patches": patches,
        "grain": grain,
    }


def ground_color(pattern, earth_color, moss_color, stone_color):
    """Returns a SIZE x SIZE x 3 array of colors from 0 to 1: the ground.

    pattern: the result of ground_pattern.
    earth_color, moss_color, stone_color: (red, green, blue) from 0 to 1.
    """
    grain = pattern["grain"]
    moss = pattern["moss"]

    # The earth, lighter and darker in soft patches, with fine grain on top.
    brightness = (0.80 + 0.40 * pattern["patches"]) * (0.88 + 0.24 * grain)
    color = brightness[..., None] * np.array(earth_color)

    # Mix towards the moss colour: moss = 0 keeps the earth, moss = 1 replaces it. The
    # moss carries the grain too, a little stronger, so it does not look painted on.
    moss_shade = (0.80 + 0.40 * grain)[..., None] * np.array(moss_color)
    color = color + moss[..., None] * (moss_shade - color)

    # The stones lie on top of both. A darker rim makes them look rounded without any
    # lighting, like the rim of the wall stones.
    stone_depth = pattern["stone_depth"]
    inside = stone_depth > 0.0
    rim = np.clip(stone_depth / 0.5, 0.0, 1.0)
    stone_shade = pattern["stone_brightness"] * (0.72 + 0.28 * rim) * (0.85 + 0.30 * grain)
    color[inside] = stone_shade[inside][..., None] * np.array(stone_color)

    return np.clip(color, 0.0, 1.0)


def ground_height(pattern, stone_rise, moss_rise, bump_depth, grain_depth):
    """Returns a SIZE x SIZE array: how far every pixel of the ground stands out.

    The unit is the size of one pixel of the texture (1 / 256 m on the models), so a
    height difference of 1 between two neighbouring pixels is a slope of 45 degrees.

    pattern: the result of ground_pattern, the same one the colour picture was made from.
    stone_rise: how far the middle of a stone stands above the earth.
    moss_rise: how far a cushion of moss stands above the earth.
    bump_depth: height of the large soft bumps of the earth.
    grain_depth: height of the fine grain.
    """
    # A stone is a dome: its height follows the smoothstep curve from the rim (0) to the
    # middle (1), so it meets the earth without a sharp crease.
    dome = smooth_step(np.clip(pattern["stone_depth"], 0.0, 1.0))

    # The two kinds of noise, blurred once more for a smooth slope (BUMP_BLUR_RADIUS).
    bumps = bump_depth * (blur(pattern["patches"], BUMP_BLUR_RADIUS) - 0.5)
    grain = grain_depth * (blur(pattern["grain"], GRAIN_BLUR_RADIUS) - 0.5)

    return stone_rise * dome + moss_rise * pattern["moss"] + bumps + grain


def metal_pattern(seed):
    """Returns what the colour picture and the normal map of one metal have in common.

    There are two metals: dark iron for the slab ring and brass for the bell of the gate.
    Neither has stones or planks: each is one surface with soft lighter and darker patches
    and a fine grain. Two different seeds give the two metals different patches.

    The result is a dictionary. Its arrays are SIZE x SIZE, one value per pixel:
      "patches", "grain":  large soft noise and fine noise, both from 0 to 1
    """
    # Its own random generator, so the numbers of the other textures stay what they are.
    rng = np.random.default_rng(seed)

    patches = smooth_noise(rng, 12)
    grain = smooth_noise(rng, 1)

    return {
        "patches": patches,
        "grain": grain,
    }


def metal_color(pattern, metal_color, patch_strength):
    """Returns a SIZE x SIZE x 3 array of colors from 0 to 1: one metal with soft patches.

    pattern: the result of metal_pattern.
    metal_color: (red, green, blue) from 0 to 1, the average color of the picture.
    patch_strength: how much the patches change the brightness in total. 0.30 means from
        15 % darker to 15 % lighter.
    """
    # Both factors are close to 1 on average (the noise is about 0.5 on average), so the
    # average of the picture stays close to metal_color. The first factor runs from
    # 1 - patch_strength / 2 to 1 + patch_strength / 2. The grain changes the brightness
    # by up to 8 % in each direction.
    patches = 1.0 - patch_strength / 2.0 + patch_strength * pattern["patches"]
    brightness = patches * (0.92 + 0.16 * pattern["grain"])

    # brightness[..., None] adds a third axis of length 1, so one brightness per pixel
    # multiplies all three color channels.
    color = brightness[..., None] * np.array(metal_color)

    return np.clip(color, 0.0, 1.0)


def metal_height(pattern, bump_depth, grain_depth):
    """Returns a SIZE x SIZE array: how far every pixel of the metal stands out.

    The unit is the size of one pixel of the texture (1 / 256 m on the models), so a
    height difference of 1 between two neighbouring pixels is a slope of 45 degrees.

    pattern: the result of metal_pattern, the same one the colour picture was made from.
    bump_depth: height of the large soft dents, like the marks of a hammer.
    grain_depth: height of the fine grain.
    """
    # The two kinds of noise, blurred once more for a smooth slope (BUMP_BLUR_RADIUS), and
    # centred on 0, so they raise and lower the surface by the same amount.
    bumps = bump_depth * (blur(pattern["patches"], BUMP_BLUR_RADIUS) - 0.5)
    grain = grain_depth * (blur(pattern["grain"], GRAIN_BLUR_RADIUS) - 0.5)

    return bumps + grain


def srgb_to_linear(color):
    """Returns the amount of light of colour values from 0 to 1 (the sRGB curve).

    The pictures hold sRGB values, and the game decodes them with this curve before it
    multiplies them by a light. Half the value is about a fifth of the light.
    """
    return np.where(color <= 0.04045, color / 12.92, ((color + 0.055) / 1.055) ** 2.4)


def linear_to_srgb(light):
    """The inverse of srgb_to_linear."""
    return np.where(light <= 0.0031308, light * 12.92, 1.055 * light ** (1.0 / 2.4) - 0.055)


def load_source_picture(file_name):
    """Returns a picture of tools/blender/sources as a height x width x 3 array, rows from
    the bottom like the arrays of this file, with the values the file holds (sRGB for a
    colour picture, directions for a normal map)."""
    image = bpy.data.images.load(os.path.join(SOURCES_DIR, file_name))
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    bpy.data.images.remove(image)
    return pixels.reshape(height, width, 4)[..., :3].astype(np.float64)


def cheapest_cut(cost):
    """Returns, for every row of cost, the column of the cheapest path from the top row to
    the bottom row: the path whose costs add up to the least. From one row to the next it
    moves by up to WALL_CUT_STEP columns, and it ends in the column it started in, so it
    also closes where the picture repeats below itself.
    """
    rows, columns = cost.shape
    best = None
    for start in range(0, columns, WALL_CUT_START_SPACING):
        total = np.full(columns, np.inf)
        total[start] = cost[0, start]
        came_from = np.zeros((rows, columns), dtype=np.int64)
        for row in range(1, rows):
            # The cheapest way into every column from the columns up to WALL_CUT_STEP away
            # in the row before. np.argmin takes the first of equal options, so the same
            # cost always gives the same path.
            walls = np.full(WALL_CUT_STEP, np.inf)
            padded = np.concatenate((walls, total, walls))
            options = np.stack([padded[k:k + columns] for k in range(2 * WALL_CUT_STEP + 1)])
            pick = np.argmin(options, axis=0)
            total = options[pick, np.arange(columns)] + cost[row]
            came_from[row] = np.arange(columns) + pick - WALL_CUT_STEP
        if best is None or total[start] < best[0]:
            best = (total[start], start, came_from)

    _, start, came_from = best
    path = np.zeros(rows, dtype=np.int64)
    path[-1] = start
    for row in range(rows - 1, 0, -1):
        path[row - 1] = came_from[row, path[row]]
    return path


def seamless_across(pictures):
    """Returns the pictures made narrower so that their left edge continues their right
    edge.

    The last WALL_OVERLAP columns of a picture are laid over its first ones, and the
    picture ends where they begin. Through the overlap runs a cut from the top to the
    bottom, along the path where the two layers differ least and are darkest: mostly the
    joints, where nobody sees that a stone of one meets a stone of the other. Left of
    the cut the result shows the end of the picture, right of it the start, so its first
    column continues its last one. The cut is softened over 3 pixels.

    pictures: a list of arrays of the same size. The first one (the colour picture)
    decides where the cut runs, and all are cut in the same place, so a normal map stays
    on its stones.
    """
    first = pictures[0]
    width = first.shape[1] - WALL_OVERLAP
    start = first[:, :WALL_OVERLAP]
    end = first[:, width:]
    cost = np.abs(start - end).sum(axis=-1) + start.mean(axis=-1) + end.mean(axis=-1)
    # The cut keeps away from the edges of the overlap, where the slabs are rounded.
    cut = WALL_CUT_MARGIN + cheapest_cut(cost[:, WALL_CUT_MARGIN:WALL_OVERLAP - WALL_CUT_MARGIN])
    x = np.arange(WALL_OVERLAP)[None, :]
    show_end = np.clip((cut[:, None] - x + 1.5) / 3.0, 0.0, 1.0)[..., None]

    result = []
    for picture in pictures:
        joined = picture[:, :width].copy()
        joined[:, :WALL_OVERLAP] = (show_end * picture[:, width:]
                                    + (1.0 - show_end) * picture[:, :WALL_OVERLAP])
        result.append(joined)
    return result


def make_seamless(pictures):
    """seamless_across, then the same along the height (on the pictures turned on their
    side), then stretched back to the size of the source.

    The second step takes whole rows, so the left and right edges still meet. The rows
    of the first cut run on without a break across the second one, so the corner meets
    too. The stretch makes the stones larger than in the source (by 1024 / 896, a
    seventh), as the picture still covers 2 m of wall.
    """
    pictures = seamless_across(pictures)
    turned = seamless_across([np.transpose(picture, (1, 0, 2)) for picture in pictures])
    return [stretch_tiling(np.transpose(picture, (1, 0, 2))) for picture in turned]


def stretch_tiling(picture):
    """The picture stretched to WALL_SOURCE_SIZE x WALL_SOURCE_SIZE, every pixel
    interpolated between its four nearest. Beyond the last row and column come the first
    ones, as the picture tiles."""
    height, width = picture.shape[:2]
    rows = np.arange(WALL_SOURCE_SIZE) * height / WALL_SOURCE_SIZE
    columns = np.arange(WALL_SOURCE_SIZE) * width / WALL_SOURCE_SIZE
    row = np.floor(rows).astype(int)
    column = np.floor(columns).astype(int)
    down = (rows - row)[:, None, None]
    across = (columns - column)[None, :, None]
    above = (row + 1) % height
    right = (column + 1) % width
    return ((1 - down) * ((1 - across) * picture[row][:, column] + across * picture[row][:, right])
            + down * ((1 - across) * picture[above][:, column] + across * picture[above][:, right]))


def unrepeated_plain(pictures):
    """The pictures of the plain wall (colour and normal map, as loaded) made seamless
    without their repeat: one copy of WALL_STONE_PERIOD rows, made seamless across, under
    the same copy mirrored left to right and moved by half its width, at the size of the
    source.

    Both halves repeat up and down by themselves, so they are defined for every row, and
    the result shows the lower half from a cut near its middle row to the same cut one
    copy higher, and the upper half elsewhere. The cut runs along the joints where the two
    halves differ least, as in seamless_across, so the stones of one half do not end in
    the middle of a stone of the other.
    """
    lower = seamless_across([picture[:WALL_STONE_PERIOD] for picture in pictures])
    width = lower[0].shape[1]
    upper = []
    for index, picture in enumerate(lower):
        mirrored = np.roll(picture[:, ::-1], width // 2, axis=1)
        if index > 0:
            # A normal map turned left to right: its x direction turns round too.
            mirrored = mirrored.copy()
            mirrored[..., 0] = 1.0 - mirrored[..., 0]
        upper.append(mirrored)

    # The rows from -WALL_HALF_SEAM_BAND / 2 to WALL_HALF_SEAM_BAND / 2 around the seam,
    # one path from the left edge to the right one that closes on itself.
    half_band = WALL_HALF_SEAM_BAND // 2
    rows = np.arange(-half_band, half_band) % WALL_STONE_PERIOD
    first, second = lower[0][rows], upper[0][rows]
    cost = np.abs(first - second).sum(axis=-1) + first.mean(axis=-1) + second.mean(axis=-1)
    cut = (WALL_CUT_MARGIN + cheapest_cut(
        cost[WALL_CUT_MARGIN:WALL_HALF_SEAM_BAND - WALL_CUT_MARGIN].T)) - half_band

    # Where every pixel lies from the cut of its column, from minus one copy up to one
    # copy: from 0 up it shows the upper half, below 0 the lower one. Softened over 3
    # pixels at both of its edges, like the cut of seamless_across.
    height = 2 * WALL_STONE_PERIOD
    above = (np.arange(height)[:, None] - cut[None, :] + WALL_STONE_PERIOD) % height
    above = above - WALL_STONE_PERIOD
    show_upper = (np.clip((above + 1.5) / 3.0, 0.0, 1.0)
                  * np.clip((WALL_STONE_PERIOD - above - 0.5) / 3.0, 0.0, 1.0))[..., None]
    every_row = np.arange(height) % WALL_STONE_PERIOD
    return [stretch_tiling(show_upper * top[every_row] + (1.0 - show_upper) * bottom[every_row])
            for bottom, top in zip(lower, upper)]


def joint_rows(color):
    """How much of every row of a wall picture is joint, averaged with the
    WALL_COURSE_SMOOTHING rows to each side (the picture repeats, so the first rows are
    averaged with the last)."""
    share = joint_mask(color).mean(axis=1)
    return sum(np.roll(share, shift) for shift in
               range(-WALL_COURSE_SMOOTHING, WALL_COURSE_SMOOTHING + 1))


def align_courses(color, slopes=None):
    """Returns the wall picture (and the slopes of its relief, when given) moved and
    stretched up and down so that its most level joint lies at row 0 and the most level
    joint near WALL_SECOND_COURSE of the picture above it at that row exactly.

    The rows between the two joints are stretched evenly, and so are the rows from the
    second joint round to the first. The slopes along the height change with the stretch.
    """
    size = color.shape[0]
    share = joint_rows(color)
    first = int(np.argmax(share))
    want = WALL_SECOND_COURSE * size
    search = int(WALL_COURSE_SEARCH * size)
    near = (first + int(round(want)) + np.arange(-search, search + 1)) % size
    second = int(near[np.argmax(share[near])])
    gap = (second - first) % size

    # The row of the source that every row of the result shows, and how many rows of the
    # source one row of the result covers there.
    out = np.arange(size, dtype=np.float64)
    low = out < want
    source = np.where(low, first + out * gap / want,
                      second + (out - want) * (size - gap) / (size - want))
    step = np.where(low, gap / want, (size - gap) / (size - want))
    below = np.floor(source).astype(int)
    share_above = (source - below)[:, None, None]

    def remap(picture):
        return ((1.0 - share_above) * picture[below % size]
                + share_above * picture[(below + 1) % size])

    if slopes is None:
        return remap(color)
    slopes = remap(slopes)
    slopes[..., 1] *= step[:, None]
    return remap(color), slopes


def luminance(color):
    """How bright every pixel of a colour picture looks, from its sRGB values."""
    return color @ np.array((0.2126, 0.7152, 0.0722))


def joint_mask(color):
    """1 in the dark gaps between the stones of a wall picture, 0 on the stones, soft in
    between."""
    return np.clip((WALL_JOINT_ABOVE - luminance(color)) / WALL_JOINT_SOFTNESS, 0.0, 1.0)


def lift_joints(color):
    """The joints of the generated pictures are almost black. Under the flashlight they
    read as holes through the wall, so the darkest pixels are drawn towards a very dark
    grey brown, the colour of shadowed earth and stone."""
    lift = np.clip((WALL_LIFT_BELOW - luminance(color)) / WALL_LIFT_BELOW, 0.0, 1.0)
    return color + lift[..., None] * (np.array(WALL_JOINT_COLOR) - color)


def average_light(color):
    """The average light of a colour picture in the three channels: sRGB decoded, as the
    game shades it and as the small copies of the texture a far wall is drawn from hold
    it."""
    return srgb_to_linear(color).mean(axis=(0, 1))


def with_light(color, light):
    """Returns color scaled so that it reflects the light `light` on average (Rec. 709
    weights). One factor scales all three channels, so no colour changes its hue."""
    weights = np.array((0.2126, 0.7152, 0.0722))
    linear = srgb_to_linear(color)
    linear = linear * (light / (linear.mean(axis=(0, 1)) @ weights))
    return linear_to_srgb(np.clip(linear, 0.0, 1.0))


def halve(picture):
    """The picture at half its width and height: every pixel the average of four."""
    height, width, channels = picture.shape
    return picture.reshape(height // 2, 2, width // 2, 2, channels).mean(axis=(1, 3))


def slopes_of(normal_colors):
    """The slopes of the relief (dheight/dx, dheight/dy) of a normal map, as colours from
    0 to 1. The inverse of normal_map."""
    normal = normal_colors * 2.0 - 1.0
    return -normal[..., :2] / normal[..., 2:]


def normal_colors(slopes):
    """A normal map, as colours from 0 to 1, of a relief with the given slopes."""
    normal = np.concatenate([-slopes, np.ones(slopes.shape[:2] + (1,))], axis=-1)
    return normal / np.linalg.norm(normal, axis=-1, keepdims=True) * 0.5 + 0.5


def save_wall(color, slopes, name):
    """Writes name.png and name_normal.png at SIZE from a colour picture and the slopes of
    its relief at the size of the sources. The normal map is halved as slopes, so the
    lean of four texels is averaged and not their colours."""
    save_png(np.clip(halve(color), 0.0, 1.0), name + ".png")
    save_png(normal_colors(halve(slopes)), name + "_normal.png")


def plain_wall():
    """Returns the colour picture of the plain wall and the slopes of its relief, both
    seamless, at the size of the sources, with the joints and the light as they came.

    The colour is the generated picture, and the relief was baked from the slab Tripo
    generated from it (see bake_wall_normals.py).
    """
    color, normal = unrepeated_plain([
        load_source_picture("wall_stone.png"),
        load_source_picture("wall_stone_normal_baked.png"),
    ])
    return align_courses(color, WALL_BAKED_RELIEF * slopes_of(normal))


def build_plain_wall_textures():
    """Writes wall_stone.png and its normal map."""
    color, slopes = plain_wall()
    save_wall(with_light(lift_joints(color), WALL_LIGHT), slopes, "wall_stone")


def build_worn_wall_textures():
    """Writes the textures of the three worn walls, each with its normal map.

    Each is scaled to reflect as much light on average as the plain wall, so a worn
    segment between plain ones does not turn into a patch from across the maze. Each
    returns its colour picture, and the script stops if one is too far from the plain wall
    in light or in colour.

    To make only these six pictures, run from the repository root (one line):
      blender --background --factory-startup --python-expr "import sys;
      sys.path.append('tools/blender'); import make_textures;
      make_textures.build_worn_wall_textures()"
    """
    plain_color, plain_slopes = plain_wall()
    plain = average_light(halve(with_light(lift_joints(plain_color), WALL_LIGHT)))
    worn = {
        "wall_cracked": build_cracked_wall_textures(),
        "wall_mossy": build_mossy_wall_textures(),
        "wall_damaged": build_damaged_wall_textures(plain_color, plain_slopes),
    }
    weights = np.array((0.2126, 0.7152, 0.0722))
    for name, color in worn.items():
        light = average_light(halve(color))
        if abs(light @ weights / (plain @ weights) - 1.0) > WALL_LIGHT_TOLERANCE:
            raise RuntimeError(f"{name} is not as bright as the plain wall")
        if np.abs(light / light.sum() - plain / plain.sum()).max() > WALL_COLOUR_SHARE_TOLERANCE:
            raise RuntimeError(f"{name} is not of the colour of the plain wall")


def height_slopes(height):
    """The slopes (dheight/dx, dheight/dy) of a height field that tiles, like in
    normal_map."""
    return np.stack([
        (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / 2.0,
        (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) / 2.0,
    ], axis=-1)


def picture_slopes(color):
    """The slopes of a relief made from a wall picture alone: brighter is higher, which
    rounds every stone and raises its pale faces a little, and the dark joints lie
    deepest. bake_wall_normals.py also fills the holes of a bake with it."""
    height = WALL_PICTURE_RISE * blur(luminance(color), 2)
    height = height - WALL_PICTURE_JOINT_DEPTH * blur(joint_mask(color), 2)
    return height_slopes(height)


def build_cracked_wall_textures():
    """Writes wall_cracked.png and its normal map.

    The picture is generated, and Tripo has no slab of it, so the relief is made from the
    picture itself (see picture_slopes).
    """
    (color,) = make_seamless([load_source_picture("wall_cracked.png")])
    color = align_courses(color)
    slopes = picture_slopes(color)
    color = with_light(lift_joints(color), WALL_LIGHT)
    save_wall(color, slopes, "wall_cracked")
    return color


def build_mossy_wall_textures():
    """Writes wall_mossy.png and its normal map: the generated picture and the relief
    baked from the slab Tripo generated from it, like the plain wall."""
    color, normal = make_seamless([
        load_source_picture("wall_mossy.png"),
        load_source_picture("wall_mossy_normal_baked.png"),
    ])
    color, slopes = align_courses(color, WALL_BAKED_RELIEF * slopes_of(normal))
    color = with_light(lift_joints(color), WALL_LIGHT)
    save_wall(color, slopes, "wall_mossy")
    return color


def spread(mask):
    """The mask with every pixel next to it (left, right, above, below) added."""
    grown = mask.copy()
    grown[1:] |= mask[:-1]
    grown[:-1] |= mask[1:]
    grown[:, 1:] |= mask[:, :-1]
    grown[:, :-1] |= mask[:, 1:]
    return grown


def grow_stone(stone, row, column):
    """Returns a mask of the stone that covers the pixel (row, column).

    stone: True on the stones, False in the joints. Two stones may touch where a joint
    is thin, so the stones are first made WALL_STONE_SHRINK pixels smaller all around.
    The stone grows from its pixel inside that, and then by the same pixels outward
    again, up to the joints.
    """
    core = stone.copy()
    for _ in range(WALL_STONE_SHRINK):
        core &= ~spread(~core)
    if not core[row, column]:
        raise RuntimeError(f"the pixel at row {row}, column {column} is not on a stone")
    found = np.zeros(stone.shape, dtype=bool)
    found[row, column] = True
    for _ in range(WALL_STONE_GROWTH):
        grown = spread(found) & core
        if (grown == found).all():
            for _ in range(WALL_STONE_SHRINK):
                found = spread(found) & stone
            if found.sum() < WALL_STONE_SMALLEST:
                raise RuntimeError(f"the stone at row {row}, column {column} is too small")
            return found
        found = grown
    raise RuntimeError(f"the stone at row {row}, column {column} runs into its neighbours")


def build_damaged_wall_textures(color, slopes):
    """Writes wall_damaged.png and its normal map: the plain wall with two stones broken
    out.

    The wall model stays flat: a missing stone is a niche in the relief only. It is
    shallow (about 5 cm), so it reads as a stone that broke off its face, not as a hole
    through the wall. Its outline is the outline of the stone, pushed in and out a little
    by noise, so it is ragged like a break.
    """
    rng = np.random.default_rng(WALL_DAMAGED_SEED)
    size = color.shape[0]
    stone = joint_mask(color) < 0.5
    missing = np.zeros((size, size))
    for column, row in WALL_MISSING_STONES:
        missing = np.maximum(missing, grow_stone(stone, row, column))

    # The share of the pixels around every pixel that are missing, 0.5 on the outline,
    # with noise that moves the outline by a few pixels.
    ragged = blur(rng.random((size, size)), 12)
    ragged = (ragged - ragged.min()) / (ragged.max() - ragged.min())
    around = blur(missing, 6) + 0.3 * (ragged - 0.5)
    # How deep in the niche a pixel lies: 0 at its outline, 1 on its floor.
    depth = smooth_step(np.clip((around - 0.5) / 0.4, 0.0, 1.0))

    # What is left behind the stone: rough mortar and rubble, lumps of two sizes.
    lumps = 0.6 * blur(rng.random((size, size)), 10) + 0.4 * blur(rng.random((size, size)), 4)
    rubble = (lumps - lumps.min()) / (lumps.max() - lumps.min())

    # The colour of the niche: dark rubble, and darker still towards its rim, where the
    # stones around it keep the light out. This shading is painted, like the shading of
    # the stones in the picture.
    shade = (0.55 + 0.45 * depth) * (0.55 + 0.9 * rubble)
    niche_color = shade[..., None] * np.array(WALL_NICHE_COLOR)
    color = lift_joints(color)
    color = color + depth[..., None] * (niche_color - color)

    # The relief: the floor of the niche lies WALL_NICHE_DEPTH pixels behind the face of
    # the stones, with the lumps of the rubble on it. In the niche the slopes of the
    # stone that broke out are gone.
    height = depth * (-WALL_NICHE_DEPTH + 7.0 * (rubble - 0.5))
    slopes = slopes * (1.0 - depth[..., None]) + height_slopes(height)

    color = with_light(color, WALL_LIGHT)
    save_wall(color, slopes, "wall_damaged")
    return color


def build_flask_textures():
    """Writes the texture of the flask with its normal map.

    The picture is used once on the model (see build_flask.py): x goes once around the
    flask and y is the height on it. So the picture is painted in bands, from the bottom
    up: the stoneware body, the cord wound around the neck, the lip and the cork.
    """
    rng = np.random.default_rng(FLASK_SEED)
    y, x = np.mgrid[0:SIZE, 0:SIZE]
    # One full turn around the flask: a sine of this angle, or of a whole multiple of
    # it, meets itself where the left and the right edge of the picture meet.
    turn = x / SIZE * 2.0 * np.pi

    patches = smooth_noise(rng, 12)
    grain = smooth_noise(rng, 1)

    # The body. The potter held the flask by its foot and dipped it into the glaze, so
    # the lower third is bare clay and the rest is glazed. The line between them is
    # uneven: two waves around the flask and a little noise.
    dip_row = FLASK_GLAZE_ROW + 12.0 * np.sin(2.0 * turn + 1.0) + 16.0 * (patches - 0.5)
    glazed = smooth_step(np.clip((y - dip_row) / 8.0, 0.0, 1.0))
    clay = np.array((0.80, 0.72, 0.56))
    glaze = np.array((0.66, 0.43, 0.17))
    # The glaze ran before it set: it is darker in long drips (noise blurred along y).
    drips = stretch(blur_along(blur_along(rng.random((SIZE, SIZE)), 30, axis=0), 3, axis=1))
    glaze_shade = (0.80 + 0.35 * drips)[..., None] * glaze
    clay_shade = (0.90 + 0.20 * grain)[..., None] * clay
    color = clay_shade + glazed[..., None] * (glaze_shade - clay_shade)
    color = color * (0.88 + 0.24 * patches)[..., None]
    height = 1.5 * glazed + 5.0 * (blur(patches, BUMP_BLUR_RADIUS) - 0.5)
    height = height + 0.6 * (blur(grain, GRAIN_BLUR_RADIUS) - 0.5)

    # The cord around the neck: a few turns lying on top of each other. Across the band
    # every turn is a round ridge, and along it the strands of the cord slant.
    cord_low, cord_high = FLASK_CORD_ROWS
    in_cord = (y >= cord_low) & (y < cord_high)
    across = np.abs(np.sin((y - cord_low) / FLASK_CORD_TURN_HEIGHT * np.pi))
    strands = 0.5 + 0.5 * np.sin(24.0 * turn + y * 0.9)
    cord_shade = (0.45 + 0.45 * across) * (0.80 + 0.20 * strands) * (0.90 + 0.20 * grain)
    color[in_cord] = cord_shade[in_cord][..., None] * np.array((0.62, 0.50, 0.31))
    height[in_cord] = (4.0 * across + 1.0 * strands)[in_cord]

    # The cork: light brown, with the small dark pits of real cork (the upper part of
    # a fine noise, and a pit is a dent in the relief too).
    in_cork = y >= FLASK_CORK_ROW
    pits = smooth_step(np.clip((smooth_noise(rng, 2) - 0.62) / 0.12, 0.0, 1.0))
    cork_shade = (0.85 + 0.30 * patches) * (1.0 - 0.55 * pits)
    color[in_cork] = cork_shade[in_cork][..., None] * np.array((0.70, 0.55, 0.37))
    height[in_cork] = (-2.5 * pits + 1.0 * (blur(grain, GRAIN_BLUR_RADIUS) - 0.5))[in_cork]

    save_png(np.clip(color, 0.0, 1.0), "flask.png")
    save_png(normal_map(height), "flask_normal.png")


def save_png(color, file_name, folder=None):
    """Saves a height x width x 3 array of colors from 0 to 1 as an 8-bit RGB PNG.

    The pictures of this file are SIZE x SIZE. build_shade.py writes larger ones, and
    bake_wall_normals.py writes into the sources folder.
    """
    height, width, _ = color.shape
    # Round to the 256 levels of an 8-bit channel here, so the bytes in the file do not
    # depend on how Blender rounds.
    levels = np.round(color * 255.0) / 255.0

    # A Blender image without alpha is still filled with 4 values per pixel. The fourth
    # one stays 1 (opaque) and is not written to the file.
    rgba = np.ones((height, width, 4), dtype=np.float32)
    rgba[..., :3] = levels

    image = bpy.data.images.new(file_name, width, height, alpha=False)
    image.pixels.foreach_set(rgba.ravel())

    folder = folder or common.TEXTURES_DIR
    os.makedirs(folder, exist_ok=True)
    image.filepath_raw = os.path.join(folder, file_name)
    image.file_format = "PNG"
    image.save()
    print("Wrote", image.filepath_raw)


def build():
    # Wall: the plain stone, made from the generated picture (see plain_wall).
    build_plain_wall_textures()

    # The wall worn in three ways: cracked, mossy and damaged. They are in a function of
    # their own, so they can also be made alone.
    build_worn_wall_textures()

    # Ground: packed earth with patches of moss and small stones, for the terrain. The
    # colours are sRGB values (the game loads the picture as an sRGB texture) and are
    # kept fairly light: the night comes from the lighting, not from the picture.
    ground = ground_pattern(seed=GROUND_SEED, stone_count=48, min_radius=4.0, max_radius=11.0)
    ground_picture = ground_color(
        ground,
        earth_color=(0.44, 0.36, 0.27),
        moss_color=(0.30, 0.42, 0.22),
        stone_color=(0.50, 0.48, 0.44),
    )
    save_png(ground_picture, "ground.png")

    # The relief of the ground: stones as low domes, moss as soft cushions, and the earth
    # itself gently uneven.
    ground_relief = ground_height(
        ground,
        stone_rise=4.0,
        moss_rise=1.5,
        bump_depth=8.0,
        grain_depth=0.6,
    )
    save_png(normal_map(ground_relief), "ground_normal.png")

    # Gate: eight upright planks of 0.25 m in a warm brown, with dark iron bands.
    gate = wood_pattern(
        seed=GATE_SEED,
        plank_width=64,
        band_centres=GATE_BAND_CENTRES,
        band_half_height=GATE_BAND_HALF_HEIGHT,
    )
    gate_color = wood_color(
        gate,
        gap_width=4,
        rim_width=5,
        rivet_radius=6,
        wood_color=(0.50, 0.33, 0.19),
        gap_color=(0.10, 0.07, 0.05),
        iron_color=(0.24, 0.25, 0.28),
    )
    save_png(gate_color, "gate_wood.png")

    # The relief of the gate: gaps about 1 cm deep, shallow grooves along the grain, and
    # the rivets as small domes. The bands themselves are real geometry in the model, so
    # here they only rise by one pixel at their edge.
    gate_height = wood_height(
        gate,
        gap_width=4,
        bevel_width=4,
        gap_depth=2.5,
        grain_depth=3.0,
        band_rise=1.0,
        rivet_radius=6,
        rivet_rise=2.5,
    )
    save_png(normal_map(gate_height), "gate_wood_normal.png")

    # The textures of the lever, the chalk and the flask. They are in a function of their
    # own, so they can also be made without writing the pictures above again.
    build_interactable_textures()

    # The two pictures of the lantern of the exit gate, in a function of their own too.
    build_lantern_textures()




def build_interactable_textures():
    """Writes the textures of the things the player uses: the lever, the chalk, the flask.

    To make only these eight pictures, run from the repository root (one line):
      blender --background --factory-startup --python-expr "import sys;
      sys.path.append('tools/blender'); import make_textures;
      make_textures.build_interactable_textures()"
    The three functions it calls can be run alone in the same way, to make only the four
    pictures of the lever, only the two of the chalk or only the two of the flask.
    """
    build_lever_textures()
    build_chalk_textures()
    build_flask_textures()


def build_lever_textures():
    """Writes the metal texture that carries the name of the lever, with its normal map."""
    # Iron with a little blue in it, for the ring at the foot of a wall that a lever
    # opens (build_crook.py). Unlike the other pictures this one is dark on purpose: the
    # ring hangs on a stone wall that the flashlight lights up, and the picture of that
    # wall is about 0.62. Light iron had the same colour as the wall and could not be
    # told apart from it.
    iron = metal_pattern(seed=LEVER_SEED)
    iron_picture = metal_color(iron, metal_color=(0.17, 0.18, 0.21), patch_strength=0.30)
    save_png(iron_picture, "lever_iron.png")

    # The relief of the iron: soft dents and fine grain, like the bands of the gate.
    iron_relief = metal_height(iron, bump_depth=5.0, grain_depth=0.6)
    save_png(normal_map(iron_relief), "lever_iron_normal.png")



def build_chalk_textures():
    """Writes the texture of the chalk marks with its normal map."""
    # Its own random generator, so the numbers of the other textures stay what they are.
    rng = np.random.default_rng(CHALK_SEED)

    # Chalk: a pale grey-white with a little green in it and nothing drawn on it. The
    # shape of a mark is its model (build_chalk.py). The picture only keeps a stroke from
    # being one even colour: fine grain and soft patches where the chalk lies thinner,
    # from 14 % darker to full strength.
    grain = smooth_noise(rng, 1)
    dust = smooth_noise(rng, 6)
    brightness = 0.86 + 0.08 * grain + 0.06 * dust
    save_png(brightness[..., None] * np.array((0.93, 0.95, 0.91)), "chalk.png")

    # The relief: only the grain, and very little of it. Chalk is a film of dust.
    save_png(normal_map(1.2 * (blur(grain, GRAIN_BLUR_RADIUS) - 0.5)), "chalk_normal.png")


def build_lantern_textures():
    """Writes the texture of the lantern of the exit gate with its normal map.

    The picture has two halves, and the model gives every face a small piece from the
    middle of one of them (see build_gate_lantern.py): the left half is the glass and the
    right half the iron of the frame.

    The glass is pale and the iron is dark, and that difference is what lights the
    lantern. The game makes a surface glow by multiplying its colour with a glow value
    (uEmissive in lit.frag). One value for the whole model then gives a bright pane, and
    a frame that stays a dark outline around it.
    """
    # The iron of the frame: darker than the iron of the slab ring (0.17), because here the
    # dark is the point.
    iron = metal_pattern(seed=LANTERN_IRON_SEED)
    iron_picture = metal_color(iron, metal_color=(0.09, 0.09, 0.10), patch_strength=0.30)
    iron_relief = metal_height(iron, bump_depth=5.0, grain_depth=0.6)

    # The glass: old, a little yellow and not quite even. It has no stones or planks
    # either, so the pattern of a metal with weak patches does for it. The colour stays
    # below 1 with the lightest patch and the lightest grain (0.86 * 1.06 * 1.08).
    glass = metal_pattern(seed=LANTERN_GLASS_SEED)
    glass_picture = metal_color(glass, metal_color=(0.86, 0.84, 0.76), patch_strength=0.12)
    glass_relief = metal_height(glass, bump_depth=1.5, grain_depth=0.2)

    # x is the column of every pixel. The columns of the left half take the glass.
    _, x = np.mgrid[0:SIZE, 0:SIZE]
    is_glass = x < SIZE // 2
    color = np.where(is_glass[..., None], glass_picture, iron_picture)
    height = np.where(is_glass, glass_relief, iron_relief)

    save_png(color, "lantern.png")
    save_png(normal_map(height), "lantern_normal.png")



