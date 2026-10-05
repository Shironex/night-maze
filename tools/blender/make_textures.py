# Generates the stone textures of the maze: assets/textures/wall_stone.png and
# assets/textures/floor_stone.png. Both are 512 x 512 pixels, 8 bits per channel, RGB.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_textures.py
#
# The textures are sampled with GL_REPEAT, so the left edge has to continue the right edge
# and the bottom edge has to continue the top edge. Every step below keeps that property:
# the stones divide the image evenly, and the noise is smoothed with wrap-around.
#
# There is no lighting in the game yet, so the picture itself carries all the detail: the
# joints, the darker rim of every stone and the differences between stones.
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


def smooth_noise(rng, radius):
    """Returns a SIZE x SIZE array of random values from 0 to 1 that tiles.

    It starts as one independent random value per pixel. Averaging every pixel with its
    neighbours up to `radius` pixels away turns that into soft patches. np.roll shifts the
    array and moves what falls off one edge to the opposite edge, so the pixels at the
    border are averaged with the pixels on the other side and the result tiles.
    """
    noise = rng.random((SIZE, SIZE))

    for axis in (0, 1):
        total = np.zeros((SIZE, SIZE))
        for shift in range(-radius, radius + 1):
            total += np.roll(noise, shift, axis=axis)
        noise = total / (2 * radius + 1)

    # Averaging pulls all values towards 0.5. Stretch them back to the full 0 to 1 range.
    return (noise - noise.min()) / (noise.max() - noise.min())


def stone_texture(seed, stone_width, stone_height, running_bond, joint_width, rim_width,
                  stone_color, joint_color, stone_variation):
    """Returns a SIZE x SIZE x 3 array of colors from 0 to 1: stones separated by joints.

    stone_width, stone_height: size of one stone in pixels. Both must divide SIZE.
    running_bond: True shifts every second row by half a stone, like in a brick wall.
    joint_width: width of the joint between two stones in pixels.
    rim_width: how far from the joint a stone is still darkened, in pixels.
    stone_color, joint_color: (red, green, blue) from 0 to 1.
    stone_variation: how much the brightness of whole stones differs, 0 means not at all.
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
    brightness = stone_brightness[row, column]

    # Large soft patches and fine grain, so a stone is not one flat color.
    patches = smooth_noise(rng, 12)
    grain = smooth_noise(rng, 1)
    brightness = brightness * (0.82 + 0.36 * patches) * (0.90 + 0.20 * grain)

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
    wall = stone_texture(
        seed=WALL_SEED,
        stone_width=128,
        stone_height=64,
        running_bond=True,
        joint_width=6,
        rim_width=7,
        stone_color=(0.62, 0.62, 0.60),
        joint_color=(0.20, 0.20, 0.20),
        stone_variation=0.16,
    )
    save_png(wall, "wall_stone.png")

    # Floor: square slabs of 0.5 m in a straight grid, darker and warmer than the wall, so
    # the floor and the walls differ in color and in pattern.
    floor = stone_texture(
        seed=FLOOR_SEED,
        stone_width=128,
        stone_height=128,
        running_bond=False,
        joint_width=8,
        rim_width=10,
        stone_color=(0.42, 0.34, 0.26),
        joint_color=(0.12, 0.10, 0.08),
        stone_variation=0.20,
    )
    save_png(floor, "floor_stone.png")


if __name__ == "__main__":
    build()
