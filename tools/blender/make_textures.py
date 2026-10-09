# Generates the textures of the game into assets/textures: the colour pictures
# wall_stone.png, wall_cracked.png, wall_mossy.png, wall_damaged.png, ground.png,
# gate_wood.png, splinter.png, lever_iron.png, lever_brass.png, chalk.png,
# flask.png, shade.png, lantern.png and stone_sheep.png, and one normal map for each of
# them (the same name with _normal). All twenty-eight are 512 x 512 pixels, 8 bits per
# channel, RGB.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_textures.py
#
# The textures are sampled with GL_REPEAT, so the left edge has to continue the right edge
# and the bottom edge has to continue the top edge. Every step below keeps that property:
# the stones and planks divide the image evenly, the noise is smoothed with wrap-around,
# and the distances between the cells of the crystal and to the stones of the ground are
# measured across the edges.
# The exceptions are flask.png, shade.png, lantern.png and splinter.png: the pictures of
# the flask and of the shade go once around their models, so only their left and right
# edges meet, the lantern only uses a small piece from the middle of each half of its
# picture, and the splinter a strip down the middle of each half of its picture, so there
# only the top and bottom edges meet.
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
WALL_SEED = 11
GROUND_SEED = 67
GATE_SEED = 37
CRYSTAL_SEED = 41
LEVER_SEED = 53
LEVER_BRASS_SEED = 71
# The worn walls show the same stones as the plain wall (WALL_SEED). These seeds only
# place what was added to them: the cracks, the moss and the rubble in the niches.
WALL_CRACKED_SEED = 83
WALL_MOSSY_SEED = 89
WALL_DAMAGED_SEED = 97
FLASK_SEED = 101
SHADE_SEED = 103
LANTERN_IRON_SEED = 107
LANTERN_GLASS_SEED = 109
CHALK_SEED = 113
# The fracture of a splinter is the pattern of CRYSTAL_SEED. This seed places the craters
# of its rind.
SPLINTER_RIND_SEED = 127
STONE_SHEEP_SEED = 131

# The wall model is 3 m high and one repeat of the texture is 2 m, so the lower 1 m of
# the picture (courses 0 to 3) is seen twice on a wall: at the bottom and again at the
# top. The four courses above it are seen once. Something that must not show twice, like
# a missing stone, goes there.
WALL_COURSE_HEIGHT = 64
WALL_FIRST_SINGLE_COURSE = 4

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

# The picture of the shade, used like the one of the flask: the model (build_shade.py)
# puts its height on the y axis of the picture and goes once around it along x. The
# hollow of the hood, where a face would be, lies between these two pixel rows from the
# bottom, around the column that is the front of the figure (three quarters of the way
# across: the front is at 270 degrees), this many columns to each side.
SHADE_FACE_ROWS = (400, 482)
SHADE_FACE_COLUMN = 384
SHADE_FACE_HALF_WIDTH = 62
# The hem of the cloak is frayed and darker below this row.
SHADE_HEM_ROW = 70

# The rind of a splinter, the left half of its picture. The models (build_splinter.py)
# show 1.5 pixels per millimetre, so the craters are 0.7 to 2.7 cm across. The colour is
# a dark grey on purpose: see build_splinter_textures.
SPLINTER_CRATER_COUNT = 46
SPLINTER_CRATER_RADIUS = (5.0, 20.0)
SPLINTER_RIND_COLOR = (0.27, 0.265, 0.25)

# How dark the water makes the stone of the mossy wall: the brightness of its wettest
# streak, and how much brighter the driest one is. The numbers multiply sRGB values, so
# they take away more light than they seem to: 0.6 leaves a third of it. They were 0.48
# and 0.26 once, and with the dark moss on top the wall kept 30 percent of the light of
# a plain one, which made every mossy segment a black patch in the maze.
MOSSY_DAMP_SHADE = 0.90
MOSSY_DAMP_STREAKS = 0.10
# The moss of that wall. It was (0.17, 0.26, 0.11), a fifth of the light of the stone.
MOSS_COLOR = (0.42, 0.49, 0.33)

# The stones that are missing in the damaged wall, as (course, stone in the course).
# Both lie in the courses that are seen once, at 1.0 m and at 1.5 m above the ground,
# and they do not touch.
WALL_MISSING_STONES = ((4, 3), (6, 0))

# The iron bands of the gate, in pixels: the rows of the band centres and half of the band
# height. One repeat of the texture is 2 m high and the gate is taller, so the lower band
# is seen twice: at 256 pixels per metre the bands lie at 0.375 m, 1.375 m and 2.375 m.
# build_gate.py raises the geometry at the same heights.
GATE_BAND_CENTRES = (96, 352)
GATE_BAND_HALF_HEIGHT = 20

# How far the two kinds of noise are blurred once more before they become relief, in
# pixels (see stone_height).
BUMP_BLUR_RADIUS = 8
GRAIN_BLUR_RADIUS = 1


def blur(values, radius):
    """Returns the SIZE x SIZE array with every pixel averaged with its neighbours.

    The neighbours are the pixels up to `radius` pixels away, first along one axis, then
    along the other. np.roll shifts the array and moves what falls off one edge to the
    opposite edge, so the pixels at the border are averaged with the pixels on the other
    side and the result tiles.
    """
    for axis in (0, 1):
        total = np.zeros((SIZE, SIZE))
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


def stone_pattern(seed, stone_width, stone_height, running_bond, stone_variation):
    """Returns what the colour picture and the normal map of one texture have in common.

    stone_width, stone_height: size of one stone in pixels. Both must divide SIZE.
    running_bond: True shifts every second row by half a stone, like in a brick wall.
    stone_variation: how much the brightness of whole stones differs, 0 means not at all.

    The result is a dictionary. Its arrays are SIZE x SIZE, one value per pixel:
      "row", "column":         which stone the pixel belongs to
      "inside_x", "inside_y":  where the pixel lies inside its stone, in pixels
      "edge_distance":         distance to the nearest edge of its stone, in pixels
      "stone_brightness":      the random brightness of its stone
      "patches", "grain":      large soft noise and fine noise, both from 0 to 1
    It also holds the sizes ("rows", "columns", "stone_width", "stone_height") and the
    random generator ("rng"), for more random numbers about the same stones.
    """
    rng = np.random.default_rng(seed)
    columns = SIZE // stone_width
    rows = SIZE // stone_height

    # Pixel coordinates. Row 0 is the bottom row of the image, because Blender stores
    # images bottom row first. It is also where v = 0 is.
    y, x = np.mgrid[0:SIZE, 0:SIZE]

    row = y // stone_height
    if running_bond:
        # Odd rows are shifted by half a stone. With an even number of rows the pattern
        # still continues across the top and bottom edge.
        x = x + (row % 2) * (stone_width // 2)
    # The modulo wraps a stone that crosses the right edge back to the left edge, so both
    # of its halves get the same brightness.
    column = (x // stone_width) % columns

    # Distance of every pixel to the nearest edge of its stone, in pixels.
    inside_x = x % stone_width
    inside_y = y % stone_height
    edge_distance = np.minimum(
        np.minimum(inside_x, stone_width - 1 - inside_x),
        np.minimum(inside_y, stone_height - 1 - inside_y),
    )

    # One random brightness per stone, looked up for every pixel.
    stone_brightness = rng.uniform(1.0 - stone_variation, 1.0 + stone_variation, (rows, columns))

    # Large soft patches and fine grain, so a stone is not one flat color.
    patches = smooth_noise(rng, 12)
    grain = smooth_noise(rng, 1)

    # The order of the three draws above (brightness, patches, grain) decides which random
    # numbers each of them gets. Changing it would change every texture.
    return {
        "rng": rng,
        "rows": rows,
        "columns": columns,
        "stone_width": stone_width,
        "stone_height": stone_height,
        "row": row,
        "column": column,
        "inside_x": inside_x,
        "inside_y": inside_y,
        "edge_distance": edge_distance,
        "stone_brightness": stone_brightness[row, column],
        "patches": patches,
        "grain": grain,
    }


def stone_color(pattern, joint_width, rim_width, stone_color, joint_color):
    """Returns a SIZE x SIZE x 3 array of colors from 0 to 1: stones separated by joints.

    pattern: the result of stone_pattern.
    joint_width: width of the joint between two stones in pixels.
    rim_width: how far from the joint a stone is still darkened, in pixels.
    stone_color, joint_color: (red, green, blue) from 0 to 1.
    """
    edge_distance = pattern["edge_distance"]
    grain = pattern["grain"]

    brightness = pattern["stone_brightness"]
    brightness = brightness * (0.82 + 0.36 * pattern["patches"]) * (0.90 + 0.20 * grain)

    # Darker rim: 0 at the edge of a stone, 1 from rim_width pixels inwards. It makes the
    # stones look rounded without any lighting.
    rim = np.clip(edge_distance / rim_width, 0.0, 1.0)
    brightness = brightness * (0.70 + 0.30 * rim)

    # brightness[..., None] adds a third axis of length 1, so one brightness per pixel
    # multiplies all three color channels.
    color = brightness[..., None] * np.array(stone_color)

    # The joint is the outer half joint_width of every stone: two neighbouring stones give
    # the full width together. It only gets the grain.
    joint = edge_distance < joint_width // 2
    joint_shade = 0.85 + 0.30 * grain
    color[joint] = joint_shade[joint][..., None] * np.array(joint_color)

    return np.clip(color, 0.0, 1.0)


def stone_height(pattern, joint_width, bevel_width, joint_depth, tilt, bump_depth, grain_depth):
    """Returns a SIZE x SIZE array: how far every pixel stands out of the surface.

    The unit is the size of one pixel of the texture (1 / 256 m on the models), so a
    height difference of 1 between two neighbouring pixels is a slope of 45 degrees.

    pattern: the result of stone_pattern, the same one the colour picture was made from.
    joint_width: width of the joint in pixels, the same number as for the colour picture.
    bevel_width: over how many pixels a stone rises from the joint to its face.
    joint_depth: how far the face of a stone stands in front of the joint.
    tilt: the largest height difference between two opposite edges of one stone.
    bump_depth: height of the large soft bumps on a stone.
    grain_depth: height of the fine grain, on the stones and in the joints.
    """
    edge_distance = pattern["edge_distance"]
    row = pattern["row"]
    column = pattern["column"]

    # The profile across a stone: 0 in the joint (the pixels the colour picture paints as
    # joint, where edge_distance is below joint_width // 2), then a smooth rise over
    # bevel_width pixels, then 1 on the face. 3t^2 - 2t^3 is the "smoothstep" curve: it
    # starts and ends flat, so the bevel has no sharp crease at either end.
    t = np.clip((edge_distance - (joint_width // 2 - 1)) / bevel_width, 0.0, 1.0)
    profile = t * t * (3.0 - 2.0 * t)

    # Every stone leans a little, each one differently: two random slopes per stone, one
    # along x and one along y. They are drawn after the numbers of the colour picture, so
    # they do not change it. (inside / size - 0.5) runs from -0.5 at one edge of the stone
    # to +0.5 at the other.
    rng = pattern["rng"]
    stone_count = (pattern["rows"], pattern["columns"])
    tilt_x = rng.uniform(-tilt, tilt, stone_count)[row, column]
    tilt_y = rng.uniform(-tilt, tilt, stone_count)[row, column]
    lean = tilt_x * (pattern["inside_x"] / pattern["stone_width"] - 0.5)
    lean = lean + tilt_y * (pattern["inside_y"] / pattern["stone_height"] - 0.5)

    # The two kinds of noise of the colour picture, blurred once more. A normal map shows
    # the SLOPE of the height, and the slope of noise that was blurred once is jagged:
    # from one pixel to the next the average loses one random value and gains another.
    # On the wall that looked like woven cloth. After the second blur the slope is smooth.
    # The noise is centred on 0, so it raises and lowers the surface by the same amount.
    bumps = bump_depth * (blur(pattern["patches"], BUMP_BLUR_RADIUS) - 0.5)
    grain = grain_depth * (blur(pattern["grain"], GRAIN_BLUR_RADIUS) - 0.5)

    # The face with its lean and its bumps is multiplied by the profile: all of it fades
    # out towards the joint, so two neighbouring stones meet at the same height (0) and
    # the surface has no step. The grain covers everything, the mortar included.
    return profile * (joint_depth + lean + bumps) + grain


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

    The unit is the size of one pixel of the texture, like in stone_height.

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
    # same reason as the noise in stone_height: the slope has to be smooth.
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


def crystal_pattern(seed, cell_count):
    """Returns what the colour picture and the normal map of the crystal have in common.

    The picture is divided into cells around random points: every pixel belongs to the
    point that is nearest to it. The cells look like the facets inside a crystal, and the
    borders between them become the veins.

    cell_count: number of random points, so also the number of cells.

    The result is a dictionary. Its arrays are SIZE x SIZE, one value per pixel:
      "cell":                  which cell the pixel belongs to
      "offset_x", "offset_y":  where the pixel lies relative to the point of its cell
      "border_distance":       distance to the border of its cell, in pixels
      "cell_brightness":       the random brightness of its cell
      "patches", "grain":      soft noise and fine noise, both from 0 to 1
    It also holds the random generator ("rng") and "cell_count".
    """
    rng = np.random.default_rng(seed)
    y, x = np.mgrid[0:SIZE, 0:SIZE]

    points = rng.uniform(0.0, SIZE, (cell_count, 2))

    # For every pixel: the distance to the nearest point and to the second nearest one,
    # and which points they are (`cell` and `neighbour`). Found by going through the
    # points one by one.
    nearest = np.full((SIZE, SIZE), float(SIZE))
    second = np.full((SIZE, SIZE), float(SIZE))
    cell = np.zeros((SIZE, SIZE), dtype=int)
    neighbour = np.zeros((SIZE, SIZE), dtype=int)
    offset_x = np.zeros((SIZE, SIZE))
    offset_y = np.zeros((SIZE, SIZE))
    for index in range(cell_count):
        # The offset to the point, taken the short way around: a point near the right
        # edge is close to the pixels at the left edge. This makes the cells tile.
        dx = (x - points[index, 0] + SIZE / 2) % SIZE - SIZE / 2
        dy = (y - points[index, 1] + SIZE / 2) % SIZE - SIZE / 2
        distance = np.sqrt(dx * dx + dy * dy)

        closer = distance < nearest
        between = ~closer & (distance < second)
        # Where this point is the new nearest one, the old nearest becomes the second.
        # Where it only beats the second one, it replaces that.
        second = np.where(closer, nearest, np.where(between, distance, second))
        neighbour = np.where(closer, cell, np.where(between, index, neighbour))
        nearest = np.where(closer, distance, nearest)
        cell = np.where(closer, index, cell)
        offset_x = np.where(closer, dx, offset_x)
        offset_y = np.where(closer, dy, offset_y)

    # The border between two cells is the line on which both points are equally far away.
    # The distance of a pixel to that line is (second^2 - nearest^2) / (2 * gap), where
    # gap is the distance between the two points, again taken the short way around.
    gap_x = (points[cell, 0] - points[neighbour, 0] + SIZE / 2) % SIZE - SIZE / 2
    gap_y = (points[cell, 1] - points[neighbour, 1] + SIZE / 2) % SIZE - SIZE / 2
    gap = np.sqrt(gap_x * gap_x + gap_y * gap_y)
    border_distance = (second * second - nearest * nearest) / (2.0 * gap)

    cell_brightness = rng.uniform(0.82, 1.0, cell_count)[cell]
    patches = smooth_noise(rng, 12)
    grain = smooth_noise(rng, 1)

    return {
        "rng": rng,
        "cell_count": cell_count,
        "cell": cell,
        "offset_x": offset_x,
        "offset_y": offset_y,
        "border_distance": border_distance,
        "cell_brightness": cell_brightness,
        "patches": patches,
        "grain": grain,
    }


def crystal_color(pattern, vein_width, crystal_color, vein_color):
    """Returns a SIZE x SIZE x 3 array of colors from 0 to 1: pale cells with light veins.

    pattern: the result of crystal_pattern.
    vein_width: how far from the border of a cell the vein still shows, in pixels.
    crystal_color, vein_color: (red, green, blue) from 0 to 1.
    """
    brightness = pattern["cell_brightness"]
    brightness = brightness * (0.92 + 0.08 * pattern["patches"]) * (0.97 + 0.03 * pattern["grain"])
    color = brightness[..., None] * np.array(crystal_color)

    # The vein: 1 on the border between two cells, fading to 0 over vein_width pixels.
    vein = smooth_step(np.clip(1.0 - pattern["border_distance"] / vein_width, 0.0, 1.0))
    # Mix towards the vein color: vein = 0 keeps the cell color, vein = 1 replaces it.
    color = color + vein[..., None] * (np.array(vein_color) - color)

    return np.clip(color, 0.0, 1.0)


def crystal_height(pattern, bevel_width, vein_depth, tilt, bump_depth):
    """Returns a SIZE x SIZE array: how far every pixel of the crystal stands out.

    The unit is the size of one pixel of the texture, like in stone_height.

    pattern: the result of crystal_pattern, the same one the colour picture was made from.
    bevel_width: over how many pixels a cell rises from its border to its face.
    vein_depth: how far the face of a cell stands in front of the vein.
    tilt: the largest slope of a cell, in pixels of height per pixel.
    bump_depth: height of the large soft bumps.
    """
    cell = pattern["cell"]

    # 0 on the border of a cell, a smooth rise, then 1 on the face.
    profile = smooth_step(np.clip(pattern["border_distance"] / bevel_width, 0.0, 1.0))

    # Every cell is a small tilted plane, each one tilted differently, so the cells catch
    # the light one after another. Two random slopes per cell, along x and along y.
    rng = pattern["rng"]
    tilt_x = rng.uniform(-tilt, tilt, pattern["cell_count"])[cell]
    tilt_y = rng.uniform(-tilt, tilt, pattern["cell_count"])[cell]
    lean = tilt_x * pattern["offset_x"] + tilt_y * pattern["offset_y"]

    bumps = bump_depth * (blur(pattern["patches"], BUMP_BLUR_RADIUS) - 0.5)

    # Like the stones: everything fades out towards the border, so two neighbouring cells
    # meet at the same height (0) and the surface has no step.
    return profile * (vein_depth + lean + bumps)


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

    The unit is the size of one pixel of the texture, like in stone_height.

    pattern: the result of ground_pattern, the same one the colour picture was made from.
    stone_rise: how far the middle of a stone stands above the earth.
    moss_rise: how far a cushion of moss stands above the earth.
    bump_depth: height of the large soft bumps of the earth.
    grain_depth: height of the fine grain.
    """
    # A stone is a dome: its height follows the smoothstep curve from the rim (0) to the
    # middle (1), so it meets the earth without a sharp crease.
    dome = smooth_step(np.clip(pattern["stone_depth"], 0.0, 1.0))

    # The two kinds of noise, blurred once more for a smooth slope (see stone_height).
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

    The unit is the size of one pixel of the texture, like in stone_height.

    pattern: the result of metal_pattern, the same one the colour picture was made from.
    bump_depth: height of the large soft dents, like the marks of a hammer.
    grain_depth: height of the fine grain.
    """
    # The two kinds of noise, blurred once more for a smooth slope (see stone_height), and
    # centred on 0, so they raise and lower the surface by the same amount.
    bumps = bump_depth * (blur(pattern["patches"], BUMP_BLUR_RADIUS) - 0.5)
    grain = grain_depth * (blur(pattern["grain"], GRAIN_BLUR_RADIUS) - 0.5)

    return bumps + grain


def wall_stones():
    """Returns the stone pattern of the wall: every wall texture is made from it.

    Neutral grey blocks, 0.5 m long and 0.25 m high, in a running bond. Twelve rows fit
    the 3 m wall exactly and the 0.25 m plinth of the wall is one row. Calling it again
    gives the same stones, so the courses of a worn wall line up with the plain wall next
    to it.
    """
    return stone_pattern(
        seed=WALL_SEED,
        stone_width=128,
        stone_height=WALL_COURSE_HEIGHT,
        running_bond=True,
        stone_variation=0.16,
    )


def wall_picture(wall, joint_color=(0.20, 0.20, 0.20)):
    """Returns the colour picture of the wall stones. wall: the result of wall_stones."""
    return stone_color(
        wall,
        joint_width=6,
        rim_width=7,
        stone_color=(0.62, 0.62, 0.60),
        joint_color=joint_color,
    )


def wall_relief(wall, joint_depth=2.5):
    """Returns the height field of the wall stones. wall: the result of wall_stones.

    Joints about 1 cm deep (2.5 pixels of 1 / 256 m), blocks that lean by up to 1.5
    pixels from edge to edge, soft bumps and fine grain. The two depths of the noise look
    large, but they are the range the noise had before its second blur: after it, most of
    the surface moves by a fraction of a pixel.
    """
    return stone_height(
        wall,
        joint_width=6,
        bevel_width=5,
        joint_depth=joint_depth,
        tilt=1.5,
        bump_depth=6.0,
        grain_depth=0.5,
    )


def draw_crack(mask, centre, width):
    """Returns mask with one crack added. mask is SIZE x SIZE, 1 on a crack, 0 beside it.

    centre: for every row of the picture, the x of the crack in that row, in pixels.
    width: for every row, half of the width of the crack in pixels. 0 leaves the row out.
    """
    x = np.arange(SIZE)[None, :] + 0.5

    # From one row to the next the crack may jump sideways by several pixels. Each row
    # therefore paints the whole piece between its own x and the x of the row below it,
    # so the crack is one unbroken line. np.roll takes the row below row 0 from the top
    # of the picture: a crack that runs through every row closes across that edge.
    below = np.roll(centre, 1)
    middle = (centre + below) / 2.0
    half_step = np.abs(centre - below) / 2.0

    # The distance to the middle of the piece, taken the short way around, so a crack
    # that crosses the left or right edge continues on the other side.
    offset = np.abs((x - middle[:, None] + SIZE / 2) % SIZE - SIZE / 2)
    distance = np.maximum(offset - half_step[:, None], 0.0)

    # 1 in the core of the crack, falling to 0 over its last pixel: a soft edge.
    line = np.clip(width[:, None] - distance, 0.0, 1.0)
    return np.maximum(mask, line)


def crack_mask(rng):
    """Returns a SIZE x SIZE array: 1 on a crack, 0 beside it.

    Two long cracks run through the whole height of the picture, and three short ones
    branch off them.
    """
    rows = np.arange(SIZE)
    # One full turn over the height of the picture: a sine of this angle, or of a whole
    # multiple of it, ends where it began, so the crack meets itself at the top edge.
    turn = rows / SIZE * 2.0 * np.pi
    mask = np.zeros((SIZE, SIZE))

    long_cracks = []
    for start in (rng.uniform(70.0, 190.0), rng.uniform(310.0, 440.0)):
        # A slow wander, a faster one and a small one, each with a random phase.
        phases = rng.uniform(0.0, 2.0 * np.pi, 3)
        centre = start + 30.0 * np.sin(turn + phases[0])
        centre = centre + 12.0 * np.sin(3.0 * turn + phases[1])
        centre = centre + 5.0 * np.sin(7.0 * turn + phases[2])
        # Stone breaks in straight pieces with sharp corners: every 16 rows the crack
        # steps sideways by a random amount.
        centre = centre + np.repeat(rng.uniform(-5.0, 5.0, SIZE // 16), 16)
        # The crack is wider in some places than in others, from 1.4 to 2.6 pixels.
        width = 2.0 + 0.6 * np.sin(2.0 * turn + rng.uniform(0.0, 2.0 * np.pi))
        mask = draw_crack(mask, centre, width)
        long_cracks.append(centre)

    first_row = WALL_FIRST_SINGLE_COURSE * WALL_COURSE_HEIGHT
    for branch in range(3):
        # A branch starts on a long crack, in the courses that are seen once, and runs
        # up and to one side for 70 to 120 rows. It gets thinner and ends in a point.
        parent = long_cracks[branch % 2]
        start_row = int(rng.integers(first_row, first_row + 2 * WALL_COURSE_HEIGHT))
        length = int(rng.integers(70, 120))
        slope = rng.uniform(0.5, 1.2) * rng.choice((-1.0, 1.0))
        steps = np.repeat(rng.uniform(-3.0, 3.0, SIZE // 8), 8)

        along = rows - start_row
        centre = parent[start_row] + slope * along + steps - steps[start_row]
        inside = (along >= 0) & (along < length)
        width = np.where(inside, 1.9 * (1.0 - along / length), 0.0)
        mask = draw_crack(mask, centre, width)

    return mask


def build_worn_wall_textures():
    """Writes the textures of the three worn walls, each with its normal map.

    Every one starts as the plain wall: the same stones in the same places. Only what is
    added differs, so a worn wall next to a plain one looks like the same building. What
    is added is a mask (where the cracks, the moss or the niches are), and the colour
    picture and the height field are both changed with that one mask.

    To make only these six pictures, run from the repository root (one line):
      blender --background --factory-startup --python-expr "import sys;
      sys.path.append('tools/blender'); import make_textures;
      make_textures.build_worn_wall_textures()"
    """
    build_cracked_wall_textures()
    build_mossy_wall_textures()
    build_damaged_wall_textures()


def build_cracked_wall_textures():
    """Writes wall_cracked.png and its normal map: deep joints and a few long cracks."""
    # The mortar has washed out: the joints are darker and almost twice as deep.
    wall = wall_stones()
    color = wall_picture(wall, joint_color=(0.13, 0.13, 0.13))
    height = wall_relief(wall, joint_depth=4.5)

    cracks = crack_mask(np.random.default_rng(WALL_CRACKED_SEED))

    # A crack is nearly black in the picture and a groove 6 pixels (about 2 cm) deep in
    # the relief. The groove is what catches the light of the flashlight.
    color = color * (1.0 - 0.85 * cracks[..., None])
    height = height - 6.0 * cracks

    save_png(color, "wall_cracked.png")
    save_png(normal_map(height), "wall_cracked_normal.png")


def srgb_to_linear(color):
    """Returns the amount of light of colour values from 0 to 1 (the sRGB curve).

    The pictures hold sRGB values, and the game decodes them with this curve before it
    multiplies them by a light. Half the value is about a fifth of the light.
    """
    return np.where(color <= 0.04045, color / 12.92, ((color + 0.055) / 1.055) ** 2.4)


def linear_to_srgb(light):
    """The inverse of srgb_to_linear."""
    return np.where(light <= 0.0031308, light * 12.92, 1.055 * light ** (1.0 / 2.4) - 0.055)


def with_brightness_of(color, reference):
    """Returns color scaled so that it reflects as much light as reference on average.

    Both are SIZE x SIZE x 3 arrays of colours from 0 to 1. The average is taken of the
    light (srgb_to_linear), because that is what the game shades with and what the small
    copies of a texture hold, which a far wall is drawn from. The three channels are
    weighted the way the eye does (the Rec. 709 weights). One factor scales all three, so
    no colour of the picture changes its hue.
    """
    weights = np.array((0.2126, 0.7152, 0.0722))
    light = srgb_to_linear(color)
    wanted = (srgb_to_linear(reference) @ weights).mean()
    light = light * (wanted / (light @ weights).mean())
    return linear_to_srgb(np.clip(light, 0.0, 1.0))


def build_mossy_wall_textures():
    """Writes wall_mossy.png and its normal map: a damp wall overgrown with moss."""
    wall = wall_stones()
    color = wall_picture(wall)
    height = wall_relief(wall)
    rng = np.random.default_rng(WALL_MOSSY_SEED)

    # Water has run down the wall: the stone is darker in long upright streaks (random
    # pixels blurred far along y, like the grain of the gate). There is no "wetter at
    # the bottom": the picture repeats above 2 m, and a ramp along the height would end
    # in a hard line there.
    streaks = stretch(blur_along(blur_along(rng.random((SIZE, SIZE)), 40, axis=0), 4, axis=1))
    color = color * (MOSSY_DAMP_SHADE + MOSSY_DAMP_STREAKS * streaks)[..., None]

    # Moss grows in patches, like on the ground (see ground_pattern), and it grows first
    # where water stays: in the joints. So the joints are green also outside the patches.
    moss_noise = stretch(blur(smooth_noise(rng, 24), 16))
    patches = smooth_step(np.clip((moss_noise - 0.38) / 0.20, 0.0, 1.0))
    in_joints = 1.0 - np.clip(wall["edge_distance"] / 12.0, 0.0, 1.0)
    in_joints = in_joints * smooth_step(np.clip((moss_noise - 0.15) / 0.20, 0.0, 1.0))
    # Fine noise frays the edge of every patch, so it does not look painted on.
    fray = 0.55 + 0.45 * smooth_noise(rng, 2)
    moss = np.clip(patches + 0.8 * in_joints, 0.0, 1.0) * fray

    # Mix towards the moss colour: moss = 0 keeps the stone, moss = 1 replaces it.
    moss_shade = (0.65 + 0.70 * wall["grain"])[..., None] * np.array(MOSS_COLOR)
    color = color + moss[..., None] * (moss_shade - color)

    # Moss is a soft cushion on the stone. It fills the joints and has a grain of its own.
    height = height + 3.0 * blur(moss, 2) + 1.2 * moss * (blur(wall["grain"], 1) - 0.5)

    # The damp and the moss are detail. Seen from far away a wall is the average of its
    # picture, and that average is brought back to the brightness of the plain wall: a
    # mossy segment is then as bright as its neighbours. What is left of the green in the
    # average is small, because the moss is pale.
    color = with_brightness_of(np.clip(color, 0.0, 1.0), wall_picture(wall))

    save_png(color, "wall_mossy.png")
    save_png(normal_map(height), "wall_mossy_normal.png")


def build_damaged_wall_textures():
    """Writes wall_damaged.png and its normal map: two stones have broken out.

    The wall model stays flat: a missing stone is a niche in the relief only. It is
    shallow (12 pixels, under 5 cm), so it reads as a stone that broke off its face, not
    as a hole through the wall. A stub of each stone is left at one end, with a ragged
    edge: a clean rectangle would look like a window, not like damage.
    """
    wall = wall_stones()
    color = wall_picture(wall)
    height = wall_relief(wall)
    rng = np.random.default_rng(WALL_DAMAGED_SEED)

    # 1 inside the missing stones, 0 elsewhere. A stone is named by its course and its
    # number in the course, so a stone that crosses the edge of the picture stays whole.
    missing = np.zeros((SIZE, SIZE), dtype=bool)
    for course, stone in WALL_MISSING_STONES:
        missing = missing | ((wall["row"] == course) & (wall["column"] == stone))

    # Soft noise that pushes the outline of a niche in and out by up to 4 pixels, so it
    # is not a straight line.
    ragged = smooth_noise(rng, 6)
    edge = wall["edge_distance"] + 8.0 * (ragged - 0.5)

    # The stub that is left: the first eighth to two fifths of the stone, measured along
    # its length, with the same noise moving the break line. In one of the two courses
    # the stub is at the left end, in the other at the right end.
    along = wall["inside_x"] / wall["stone_width"]
    along = np.where(wall["row"] % 4 == 0, along, 1.0 - along)
    broken_off = smooth_step(np.clip((along - (0.12 + 0.30 * ragged)) / 0.05, 0.0, 1.0))

    # How deep in the niche a pixel lies: 0 at its outline and on the stub, rising over
    # 6 pixels to 1 on the floor of the niche.
    depth = smooth_step(np.clip(edge / 6.0, 0.0, 1.0)) * broken_off * missing

    # What is left behind the stone: rough mortar and rubble, lumps of two sizes.
    rubble = 0.6 * smooth_noise(rng, 5) + 0.4 * smooth_noise(rng, 2)

    # The colour of the niche: dark rubble, and darker still towards its rim, where the
    # stones around it keep the light out. This shading is painted, like the darker rim
    # of every stone.
    shade = (0.55 + 0.45 * np.clip(edge / 20.0, 0.0, 1.0)) * (0.55 + 0.9 * rubble)
    niche_color = shade[..., None] * np.array((0.20, 0.19, 0.18))
    color = color + depth[..., None] * (niche_color - color)

    # The relief: the floor of the niche lies 12 pixels behind the mortar bed, with the
    # lumps of the rubble on it. The step at the rim is the slope the normal map shows.
    height = height * (1.0 - depth) + depth * (-12.0 + 3.5 * (blur(rubble, 1) - 0.5))

    save_png(np.clip(color, 0.0, 1.0), "wall_damaged.png")
    save_png(normal_map(height), "wall_damaged_normal.png")


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


def save_png(color, file_name):
    """Saves a SIZE x SIZE x 3 array of colors from 0 to 1 as an 8-bit RGB PNG."""
    # Round to the 256 levels of an 8-bit channel here, so the bytes in the file do not
    # depend on how Blender rounds.
    levels = np.round(color * 255.0) / 255.0

    # A Blender image without alpha is still filled with 4 values per pixel. The fourth
    # one stays 1 (opaque) and is not written to the file.
    rgba = np.ones((SIZE, SIZE, 4), dtype=np.float32)
    rgba[..., :3] = levels

    image = bpy.data.images.new(file_name, SIZE, SIZE, alpha=False)
    image.pixels.foreach_set(rgba.ravel())

    os.makedirs(common.TEXTURES_DIR, exist_ok=True)
    image.filepath_raw = os.path.join(common.TEXTURES_DIR, file_name)
    image.file_format = "PNG"
    image.save()
    print("Wrote", image.filepath_raw)


def build():
    # Wall: the plain stone, as it was built (see wall_stones, wall_picture, wall_relief).
    wall = wall_stones()
    save_png(wall_picture(wall), "wall_stone.png")
    save_png(normal_map(wall_relief(wall)), "wall_stone_normal.png")

    # The same wall worn in three ways: cracked, mossy and damaged. They are in a
    # function of their own, so they can also be made alone.
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

    # The two pictures of the splinters the player collects, in a function of their own.
    build_splinter_textures()

    # The textures of the lever, the chalk and the flask. They are in a function of their
    # own, so they can also be made without writing the pictures above again.
    build_interactable_textures()

    # The two pictures of the shade, in a function of their own for the same reason.
    build_shade_textures()

    # The two pictures of the lantern of the exit gate, in a function of their own too.
    build_lantern_textures()

    # The two pictures of the stone sheep, in a function of their own too.
    build_stone_sheep_textures()


def build_stone_sheep_textures():
    """Writes stone_sheep.png and its normal map: wool that went to stone, moss in its folds.

    The picture of the wall would draw the joints of its blocks across the animal. Here
    the picture is divided into round curls instead (the cells of crystal_pattern), in
    a stone paler than the walls, so the animal stands out in front of one in the beam of
    the lamp and in the light of the moon. Between two curls is a fold, and moss grows
    where water stays: in the folds of the damper half of the picture, and in a few
    patches over whole curls. The picture tiles, and the model (build_stone_sheep.py)
    repeats it once per metre, so a curl is about 5 cm across.
    """
    wool = crystal_pattern(seed=STONE_SHEEP_SEED, cell_count=340)
    rng = wool["rng"]
    fold_distance = wool["border_distance"]

    # A curl is lightest in its middle and darker towards the fold around it.
    rounded = smooth_step(np.clip(fold_distance / 9.0, 0.0, 1.0))
    shade = wool["cell_brightness"] * (0.80 + 0.20 * rounded)
    shade = shade * (0.90 + 0.10 * wool["patches"]) * (0.96 + 0.08 * wool["grain"])
    color = shade[..., None] * np.array((0.86, 0.85, 0.80))

    # The moss: in the folds where the noise says the wool is damp, and in soft patches
    # that cover whole curls where it is dampest. Fine noise frays every edge.
    moss_noise = stretch(blur(smooth_noise(rng, 24), 16))
    patches = smooth_step(np.clip((moss_noise - 0.62) / 0.18, 0.0, 1.0))
    in_folds = smooth_step(np.clip(1.0 - fold_distance / 4.0, 0.0, 1.0))
    in_folds = in_folds * smooth_step(np.clip((moss_noise - 0.35) / 0.25, 0.0, 1.0))
    fray = 0.55 + 0.45 * smooth_noise(rng, 2)
    moss = np.clip(0.7 * patches + 0.8 * in_folds, 0.0, 1.0) * fray
    moss_shade = (0.65 + 0.70 * wool["grain"])[..., None] * np.array((0.22, 0.33, 0.14))
    color = color + moss[..., None] * (moss_shade - color)

    # The relief: every curl is a low dome, the moss a soft cushion in the fold.
    height = 2.5 * rounded + 4.0 * (blur(wool["patches"], BUMP_BLUR_RADIUS) - 0.5)
    height = height + 1.5 * blur(moss, 2) + 0.5 * (blur(wool["grain"], GRAIN_BLUR_RADIUS) - 0.5)

    save_png(np.clip(color, 0.0, 1.0), "stone_sheep.png")
    save_png(normal_map(height), "stone_sheep_normal.png")


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
    """Writes the two metal textures that carry the name of the lever, each with its normal map."""
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

    # Warm brass, light and yellow, which the bell of the gate wears (build_gate_bell.py). The patches are weaker than on the iron:
    # the red channel is already 0.86, and stronger patches would push it past 1, where
    # it is cut off and the colour of the light patches would change.
    brass = metal_pattern(seed=LEVER_BRASS_SEED)
    brass_picture = metal_color(brass, metal_color=(0.86, 0.58, 0.20), patch_strength=0.14)
    save_png(brass_picture, "lever_brass.png")

    # The relief of the brass: gentler than the iron, like metal polished by many hands.
    brass_relief = metal_height(brass, bump_depth=3.0, grain_depth=0.4)
    save_png(normal_map(brass_relief), "lever_brass_normal.png")


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


def build_shade_textures():
    """Writes the texture of the shade with its normal map.

    To make only these two pictures, run from the repository root (one line):
      blender --background --factory-startup --python-expr "import sys;
      sys.path.append('tools/blender'); import make_textures;
      make_textures.build_shade_textures()"

    The shade is a hooded figure in a cloak that is nearly black. Nearly, not fully: a
    black picture would show a hole in the beam of the flashlight and not a shape. So the
    cloth is a very dark blue-grey, a little lighter on the ridges of its folds, and the
    folds are in the normal map too, where the light of the flashlight finds them.
    """
    rng = np.random.default_rng(SHADE_SEED)
    y, x = np.mgrid[0:SIZE, 0:SIZE]
    # One full turn around the figure, as in build_flask_textures.
    turn = x / SIZE * 2.0 * np.pi

    patches = smooth_noise(rng, 14)
    grain = smooth_noise(rng, 1)

    # The folds of the cloak: long ridges that run from the shoulders to the ground. Three
    # sine waves around the figure (whole multiples of the turn, so the left edge meets
    # the right one), bent sideways a little by the patches so no two ridges are alike.
    bend = 1.6 * (patches - 0.5)
    folds = (
        0.50
        + 0.26 * np.sin(9.0 * turn + bend + 0.4)
        + 0.14 * np.sin(17.0 * turn - 2.0 * bend + 1.9)
        + 0.10 * np.sin(4.0 * turn + 0.7)
    )

    # The cloth: darkest in the valleys of the folds, lightest on their ridges. The woven
    # threads are the fine noise drawn out along y.
    weave = stretch(blur_along(rng.random((SIZE, SIZE)), 6, axis=0))
    cloth = np.array((0.070, 0.078, 0.110))
    shade = (0.50 + 0.85 * folds) * (0.85 + 0.30 * weave) * (0.90 + 0.20 * patches)

    # The hem is frayed and wet from the grass: it gets darker towards the ground.
    hem = smooth_step(np.clip(y / SHADE_HEM_ROW, 0.0, 1.0))
    shade = shade * (0.45 + 0.55 * hem)
    color = shade[..., None] * cloth

    # The hollow of the hood: no face, only the dark. Soft at its border, so it reads as
    # a depth and not as a painted patch.
    face_low, face_high = SHADE_FACE_ROWS
    across = np.abs(x - SHADE_FACE_COLUMN) / SHADE_FACE_HALF_WIDTH
    along = np.abs(y - (face_low + face_high) / 2.0) / ((face_high - face_low) / 2.0)
    inside = 1.0 - smooth_step(np.clip((np.maximum(across, along) - 0.7) / 0.3, 0.0, 1.0))
    color = color * (1.0 - 0.9 * inside)[..., None]

    # The relief: the folds stand out, the weave is a fine grain on them, and the hollow
    # of the hood is flat.
    height = (9.0 * folds + 0.8 * (blur(grain, GRAIN_BLUR_RADIUS) - 0.5)) * (1.0 - inside)

    save_png(np.clip(color, 0.0, 1.0), "shade.png")
    save_png(normal_map(height), "shade_normal.png")


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


def build_splinter_textures():
    """Writes the texture of the moon splinters with its normal map.

    To make only these two pictures, run from the repository root (one line):
      blender --background --factory-startup --python-expr "import sys;
      sys.path.append('tools/blender'); import make_textures;
      make_textures.build_splinter_textures()"

    The picture has two halves, like the one of the lantern, and the model gives every
    side a piece from the middle of one of them (see build_splinter.py): the left half is
    the rind, the old surface of the moon, and the right half the fracture.

    The fracture is pale and the rind is dark, and that difference decides what glows.
    The game multiplies the colour of a surface with one glow value for the whole model
    (uEmissive in lit.frag), so the fracture shines turquoise and the rind stays a strip
    of dim stone, which the flashlight shows as the grey it is.
    """
    half = SIZE // 2
    y, x = np.mgrid[0:SIZE, 0:SIZE]

    # The fracture: pale turquoise cells with lighter veins, like the facets inside
    # a crystal. The game adds its own glow and a turquoise light, so the picture itself
    # stays light.
    fracture = crystal_pattern(seed=CRYSTAL_SEED, cell_count=28)
    fracture_picture = crystal_color(
        fracture,
        vein_width=3,
        crystal_color=(0.60, 0.90, 0.86),
        vein_color=(0.80, 0.97, 0.95),
    )
    # Its relief: shallow veins and gently tilted cells.
    fracture_relief = crystal_height(
        fracture,
        bevel_width=6,
        vein_depth=1.0,
        tilt=0.06,
        bump_depth=4.0,
    )

    # The rind: grey dust with soft lighter and darker patches. Its own random generator,
    # so the numbers of the other textures stay what they are.
    rng = np.random.default_rng(SPLINTER_RIND_SEED)
    patches = smooth_noise(rng, 10)
    grain = smooth_noise(rng, 1)

    # The craters. Each is a bowl with a raised lip around it: `bowl` is 1 in the middle
    # of a crater and 0 at its edge, `lip` is 1 on the edge and fades to both sides.
    # A crater that crosses the top edge continues at the bottom, and one that crosses the
    # side of the half continues at its other side, so no crater is cut off.
    centre_x = rng.uniform(0.0, half, SPLINTER_CRATER_COUNT)
    centre_y = rng.uniform(0.0, SIZE, SPLINTER_CRATER_COUNT)
    low, high = SPLINTER_CRATER_RADIUS
    # Squaring the random number makes small craters common and large ones rare.
    radius = low + (high - low) * rng.random(SPLINTER_CRATER_COUNT) ** 2
    bowl = np.zeros((SIZE, SIZE))
    lip = np.zeros((SIZE, SIZE))
    crater_relief = np.zeros((SIZE, SIZE))
    for index in range(SPLINTER_CRATER_COUNT):
        dx = (x - centre_x[index] + half / 2) % half - half / 2
        dy = (y - centre_y[index] + SIZE / 2) % SIZE - SIZE / 2
        distance = np.sqrt(dx * dx + dy * dy) / radius[index]
        this_bowl = np.clip(1.0 - distance * distance, 0.0, 1.0)
        this_lip = np.exp(-(((distance - 1.0) / 0.2) ** 2))
        bowl = np.maximum(bowl, this_bowl)
        lip = np.maximum(lip, this_lip)
        # In pixels of height: the depth of a bowl and the height of its lip grow with
        # its radius, so a large crater is not a shallow dish.
        crater_relief += radius[index] * (0.12 * this_lip - 0.30 * this_bowl)

    # The floor of a crater is darker and its lip lighter, the way the real ones look
    # under a high sun.
    shade = (0.80 + 0.40 * patches) * (0.90 + 0.20 * grain)
    shade = shade * (1.0 - 0.45 * bowl) * (1.0 + 0.30 * lip * (1.0 - bowl))
    rind_picture = np.clip(shade[..., None] * np.array(SPLINTER_RIND_COLOR), 0.0, 1.0)
    rind_relief = (
        crater_relief
        + 6.0 * (blur(patches, BUMP_BLUR_RADIUS) - 0.5)
        + 0.8 * (blur(grain, GRAIN_BLUR_RADIUS) - 0.5)
    )

    # The columns of the left half take the rind.
    is_rind = x < half
    color = np.where(is_rind[..., None], rind_picture, fracture_picture)
    height = np.where(is_rind, rind_relief, fracture_relief)

    save_png(color, "splinter.png")
    save_png(normal_map(height), "splinter_normal.png")


if __name__ == "__main__":
    build()
