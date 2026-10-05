# Generates the stone textures of the maze into assets/textures: the colour pictures
# wall_stone.png and floor_stone.png, and their normal maps wall_stone_normal.png and
# floor_stone_normal.png. All four are 512 x 512 pixels, 8 bits per channel, RGB.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_textures.py
#
# The textures are sampled with GL_REPEAT, so the left edge has to continue the right edge
# and the bottom edge has to continue the top edge. Every step below keeps that property:
# the stones divide the image evenly, and the noise is smoothed with wrap-around.
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

# Width and height of both textures in pixels. A power of two, and 256 pixels per metre
# with the texel density of the models (one repeat = 2 m).
SIZE = 512

# The same seeds give the same pictures on every run. Change a seed to get another
# arrangement of light and dark stones.
WALL_SEED = 11
FLOOR_SEED = 23

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

    # Floor: square slabs of 0.5 m in a straight grid, darker and warmer than the wall, so
    # the floor and the walls differ in color and in pattern.
    floor = stone_pattern(
        seed=FLOOR_SEED,
        stone_width=128,
        stone_height=128,
        running_bond=False,
        stone_variation=0.20,
    )
    floor_color = stone_color(
        floor,
        joint_width=8,
        rim_width=10,
        stone_color=(0.42, 0.34, 0.26),
        joint_color=(0.12, 0.10, 0.08),
    )
    save_png(floor_color, "floor_stone.png")

    # The relief of the floor: wider and shallower joints than in the wall.
    floor_height = stone_height(
        floor,
        joint_width=8,
        bevel_width=6,
        joint_depth=2.0,
        tilt=1.5,
        bump_depth=6.0,
        grain_depth=0.5,
    )
    save_png(normal_map(floor_height), "floor_stone_normal.png")


if __name__ == "__main__":
    build()
