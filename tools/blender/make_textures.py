# Generates the textures of the game into assets/textures: the colour pictures
# wall_stone.png, ground.png, gate_wood.png and crystal.png, and one normal map for
# each of them (the same name with _normal). All eight are 512 x 512 pixels, 8 bits per
# channel, RGB.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_textures.py
#
# The textures are sampled with GL_REPEAT, so the left edge has to continue the right edge
# and the bottom edge has to continue the top edge. Every step below keeps that property:
# the stones and planks divide the image evenly, the noise is smoothed with wrap-around,
# and the distances between the cells of the crystal and to the stones of the ground are
# measured across the edges.
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
    # Wall: neutral grey blocks, 0.5 m long and 0.25 m high, in a running bond. Twelve rows
    # fit the 3 m wall exactly and the 0.25 m plinth of the wall is one row.
    wall = stone_pattern(
        seed=WALL_SEED,
        stone_width=128,
        stone_height=64,
        running_bond=True,
        stone_variation=0.16,
    )
    wall_color = stone_color(
        wall,
        joint_width=6,
        rim_width=7,
        stone_color=(0.62, 0.62, 0.60),
        joint_color=(0.20, 0.20, 0.20),
    )
    save_png(wall_color, "wall_stone.png")

    # The relief of the wall: joints about 1 cm deep (2.5 pixels of 1 / 256 m), blocks
    # that lean by up to 1.5 pixels from edge to edge, soft bumps and fine grain. The two
    # depths of the noise look large, but they are the range the noise had before its
    # second blur: after it, most of the surface moves by a fraction of a pixel.
    wall_height = stone_height(
        wall,
        joint_width=6,
        bevel_width=5,
        joint_depth=2.5,
        tilt=1.5,
        bump_depth=6.0,
        grain_depth=0.5,
    )
    save_png(normal_map(wall_height), "wall_stone_normal.png")

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

    # Crystal: pale turquoise cells with lighter veins. The game adds its own glow and a
    # turquoise light, so the picture itself stays light.
    crystal = crystal_pattern(seed=CRYSTAL_SEED, cell_count=28)
    crystal_picture = crystal_color(
        crystal,
        vein_width=3,
        crystal_color=(0.60, 0.90, 0.86),
        vein_color=(0.80, 0.97, 0.95),
    )
    save_png(crystal_picture, "crystal.png")

    # The relief of the crystal: shallow veins and gently tilted cells.
    crystal_relief = crystal_height(
        crystal,
        bevel_width=6,
        vein_depth=1.0,
        tilt=0.06,
        bump_depth=4.0,
    )
    save_png(normal_map(crystal_relief), "crystal_normal.png")


if __name__ == "__main__":
    build()
