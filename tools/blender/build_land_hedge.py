# Builds the picture of the hedgerows of the land around the maze. A hedgerow is built in
# code (game::buildHedgerowMesh): a lumpy strip along a line of the field grid. What it
# wears is the side of a hedge that Tripo generated, tools/blender/sources/
# hedge_block_tripo.glb, photographed straight from the side with the flat light of the
# Workbench engine, so the picture holds the colours of the source and no light of its own.
# The middle of the side is kept, and its two ends are blended into each other so the
# picture repeats along the strip without a seam.
# Output: assets/textures/land_hedge.png and land_hedge_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_land_hedge.py
import os
import sys
import tempfile

import bpy
import numpy as np

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import generated_common as generated
import make_textures

SOURCE_FILE = "hedge_block_tripo.glb"

# The picture, in pixels: twice as long as high. One repeat covers HEDGE_TEXTURE_SPAN
# (src/game/Land.hpp) along the strip and the whole height of the hedge.
WIDTH = 512
HEIGHT = 256

# The part of the side that is photographed, in the units of the source (the block is
# 1 long and 0.42 high): the middle, without the rounded ends and the earth at the foot.
VIEW_LEFT = -0.36
VIEW_RIGHT = 0.36
VIEW_BOTTOM = 0.04
VIEW_TOP = 0.40

# How many pixels at each end are blended into the other end.
SEAM = 96

# How steep the normal map makes the lumps of the leaves: the slope of one step of the
# brightness of the picture.
RELIEF = 3.0


def photograph():
    """The side of the hedge as a HEIGHT x WIDTH x 3 array of sRGB colours, the bottom row
    first, as Blender stores pictures."""
    model, _, _ = generated.import_source(SOURCE_FILE)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "FLAT"
    scene.display.shading.color_type = "TEXTURE"
    scene.view_settings.view_transform = "Standard"
    scene.render.resolution_x = WIDTH
    scene.render.resolution_y = HEIGHT
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    # No smoothing of the edges: the picture is filled with the hedge from side to side.
    scene.display.render_aa = "OFF"

    camera_data = bpy.data.cameras.new("side")
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = VIEW_RIGHT - VIEW_LEFT
    camera = bpy.data.objects.new("side", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    # The block lies along X with its side towards -Y. The camera looks along +Y at the
    # middle of the part that is kept.
    camera.location = ((VIEW_LEFT + VIEW_RIGHT) / 2.0, -5.0, (VIEW_BOTTOM + VIEW_TOP) / 2.0)
    camera.rotation_euler = (1.5707963, 0.0, 0.0)
    del model

    path = os.path.join(tempfile.gettempdir(), "night_maze_hedge_side.png")
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    picture = bpy.data.images.load(path)
    return generated.picture_array(picture)


def seamless(picture):
    """The picture with its ends blended: the last SEAM columns fade into the first ones,
    and are then cut off, so the right edge continues into the left edge."""
    width = picture.shape[1] - SEAM
    result = picture[:, :width].copy()
    fade = np.linspace(0.0, 1.0, SEAM, dtype=np.float32)[None, :, None]
    result[:, :SEAM] = picture[:, width:] * (1.0 - fade) + picture[:, :SEAM] * fade
    return result


def stretched(picture):
    """The picture brought back to WIDTH columns by repeating neighbours (nearest)."""
    columns = np.minimum((np.arange(WIDTH) * picture.shape[1]) // WIDTH, picture.shape[1] - 1)
    return picture[:, columns]


def normal_map(colour):
    """A normal map from the brightness of the picture: bright leaves stand out, the dark
    gaps between them lie deeper. The slopes wrap around the ends like the picture."""
    brightness = colour.mean(axis=-1)
    slope_x = (np.roll(brightness, -1, axis=1) - np.roll(brightness, 1, axis=1)) * RELIEF
    slope_y = (np.roll(brightness, -1, axis=0) - np.roll(brightness, 1, axis=0)) * RELIEF
    directions = np.stack([-slope_x, -slope_y, np.ones_like(brightness)], axis=-1)
    directions /= np.linalg.norm(directions, axis=-1, keepdims=True)
    return directions * 0.5 + 0.5


def build():
    colour = stretched(seamless(photograph()))
    make_textures.save_png(colour, "land_hedge.png")
    make_textures.save_png(normal_map(colour), "land_hedge_normal.png")


if __name__ == "__main__":
    build()
