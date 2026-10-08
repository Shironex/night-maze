# Generates the heightmap of the terrain into assets/textures: heightmap.png, a grey picture
# of SIZE x SIZE pixels, 8 bits per channel, saved as RGB with three equal channels.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_heightmap.py
#
# A heightmap is a picture read as numbers: the brightness of a pixel is the height of the
# ground at that place. Black (0) is the lowest ground, white (255) the highest. The game
# multiplies the value by a height in metres (game::Terrain), so the picture itself knows
# nothing about metres.
#
# The picture is the land seen from above: left is west (-X), right is east (+X), the top
# row is north (-Z) and the bottom row is south (+Z). The game lays it on the ground again
# and again, like a tiling texture (one repeat is game::HEIGHTMAP_SPAN metres), so the left
# edge has to continue the right edge and the top edge the bottom edge. The noise below
# repeats exactly once per picture, which gives that for free.
#
# The heights are smooth value noise in a few octaves. Value noise: random numbers on
# a coarse grid, blended smoothly between the grid points. One such layer is a few broad
# hills. An octave is the same thing with twice as many grid cells and half the height:
# smaller bumps on top of the hills. Blender is used only to write the PNG file.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import bpy
import numpy as np

import blender_common as common

# Width and height of the picture in pixels.
SIZE = 256

# The same seed gives the same picture on every run. Change it to get other hills.
HEIGHTMAP_SEED = 53

# Grid cells of the first octave across the picture: 4 cells make hills that are a quarter
# of the picture wide, 12 m when the picture covers 48 m.
BASE_CELLS = 4

# How many octaves are added up. Every octave has twice the cells of the one before it and
# OCTAVE_GAIN times its height.
OCTAVE_COUNT = 3
OCTAVE_GAIN = 0.45

FILE_NAME = "heightmap.png"


def smooth_step(t):
    """The curve 3t^2 - 2t^3 for t from 0 to 1: it starts and ends flat."""
    return t * t * (3.0 - 2.0 * t)


def value_noise(rng, cells):
    """Returns a SIZE x SIZE array of smooth random values from 0 to 1 that tiles.

    cells: the number of grid cells across the picture. One random number is drawn for
    every grid point. A pixel gets the four numbers at the corners of its grid cell,
    blended by how close it is to each corner (bilinear blending along a smooth curve).
    The grid point after the last one is the first one again, so the picture tiles.
    """
    lattice = rng.random((cells, cells))

    # Where the middle of every pixel lies on the grid: from 0 to cells.
    position = (np.arange(SIZE) + 0.5) / SIZE * cells
    cell = np.floor(position).astype(np.int64)
    inside = smooth_step(position - cell)

    low = cell % cells
    high = (cell + 1) % cells

    # np.ix_ pairs every row index with every column index: a SIZE x SIZE lookup.
    top_left = lattice[np.ix_(low, low)]
    top_right = lattice[np.ix_(low, high)]
    bottom_left = lattice[np.ix_(high, low)]
    bottom_right = lattice[np.ix_(high, high)]

    # Blend along x (the columns) in the two rows, then along y between the rows.
    along_x = inside[np.newaxis, :]
    along_y = inside[:, np.newaxis]
    top = top_left * (1.0 - along_x) + top_right * along_x
    bottom = bottom_left * (1.0 - along_x) + bottom_right * along_x
    return top * (1.0 - along_y) + bottom * along_y


def height_field():
    """Returns the heights as a SIZE x SIZE array from 0 to 1, row 0 is the top row."""
    # One generator for all octaves, used in a fixed order: the same seed then gives the
    # same picture on every run.
    rng = np.random.default_rng(HEIGHTMAP_SEED)

    heights = np.zeros((SIZE, SIZE))
    cells = BASE_CELLS
    gain = 1.0
    for _ in range(OCTAVE_COUNT):
        heights += gain * value_noise(rng, cells)
        cells *= 2
        gain *= OCTAVE_GAIN

    # Stretch to the full range: the lowest pixel becomes 0 and the highest 1, so the
    # heights in metres the game multiplies with are really reached.
    return (heights - heights.min()) / (heights.max() - heights.min())


def save_png(heights, file_name):
    """Saves a SIZE x SIZE array of values from 0 to 1 as an 8-bit grey RGB PNG.

    Row 0 of the array becomes the TOP row of the picture.
    """
    # Round to the 256 levels of an 8-bit channel here, so the bytes in the file do not
    # depend on how Blender rounds.
    levels = np.round(heights * 255.0) / 255.0

    # Blender stores an image bottom row first, so the rows are turned over. The same
    # value goes into red, green and blue. The fourth value stays 1 (opaque) and is not
    # written to the file.
    rgba = np.ones((SIZE, SIZE, 4), dtype=np.float32)
    rgba[..., :3] = levels[::-1, :, np.newaxis]

    image = bpy.data.images.new(file_name, SIZE, SIZE, alpha=False)
    image.pixels.foreach_set(rgba.ravel())

    os.makedirs(common.TEXTURES_DIR, exist_ok=True)
    image.filepath_raw = os.path.join(common.TEXTURES_DIR, file_name)
    image.file_format = "PNG"
    image.save()
    print("Wrote", image.filepath_raw)


def build():
    save_png(height_field(), FILE_NAME)


if __name__ == "__main__":
    build()
