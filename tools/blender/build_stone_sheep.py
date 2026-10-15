# Builds the model stone_sheep: a sheep of the flock that went to stone with the hedge. It is
# the compass of the maze: every one of them looks towards the gate, so the end with the head
# has to read from far away. The sheep is not built in this script. Its source is a model that
# Tripo generated, tools/blender/sources/stone_sheep_tripo.glb: a sheep of pale grey stone
# that lies curled up with its legs tucked in and its head turned towards the side, with moss
# and lichen on the wool, one material with a colour picture and a normal map. This script
# cleans it up and writes the game files, the same bytes on every run.
# Output: assets/models/stone_sheep.obj and stone_sheep.mtl, and the pictures
# assets/textures/stone_sheep.png and stone_sheep_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_stone_sheep.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "stone_sheep"

SOURCE_FILE = "stone_sheep_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space: X is the length of
# the sheep with the head end at +X, Y is its width and Z is the height. After export the
# head end still looks along the game +X axis, which is east: the game turns the model about
# the vertical axis to the direction of the gate (game::sheepYawDegrees).
#
# The source lies along X with its head at -X (and the nose turned towards -Y), so the
# script turns it half a circle about the vertical axis. It is then scaled so that it is
# LENGTH long from the nose to the end of the tail, and the origin is in the middle of
# that length and of the width, on the ground. The box of the game (STONE_SHEEP_LENGTH in
# src/game/StoneSheep.hpp) is exactly as long. A lying sheep is round, and at 0.9 m it
# reached into the walls when it lies across a corner, so it is shorter than the standing
# one was.
LENGTH = 0.75

# The sheep lies with its legs tucked in, and the source is broad: 0.54 m across at 0.75 m
# long. A box that wide would block the way past the sheep that the player and the shade
# need, so the model is squeezed across to the width of the box of the game
# (STONE_SHEEP_WIDTH there). Its length and its height keep the scale of the source.
WIDTH = 0.4

# The sheep lies, so its belly and its legs sink this far under the origin. The ground of
# the maze is uneven, and a body that ended exactly at the origin would hang in the air on
# the low side. The box of the game reaches as far down (STONE_SHEEP_BOX_DEPTH).
SINK = 0.1

# Two corners of the source closer than this (in its own units, about a metre for the
# whole sheep) are one corner. The file has a corner of its own for every face that meets
# at a seam of the picture, and without this the faces of one surface would not know that
# they are neighbours: not for the smooth normals and not for the islands.
MERGE_DISTANCE = 0.000001

# A black scrap of something, a thorn or a bit of chain, sticks to the chest of the source.
# A face belongs to it when the texel at the middle of its UVs is darker than SCRAP_DARK in
# every channel (sRGB) and it lies higher than SCRAP_FROM over the ground of the source:
# the dark places lower down are the hooves and the shadows under the belly, and stay.
SCRAP_DARK = 0.3
SCRAP_FROM = 0.2

# The pictures of the source are 2048 pixels wide. The game gets them at half of that,
# every texel the average of 2 by 2 of the source: the sheep is seen from a metre away at
# the closest, and the lumps of the wool are only a few centimetres across.
TEXTURE_SIZE = 1024


def clean(model, colour):
    """Returns the cleaned surface as a bmesh, in the size and place of the game."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a millimetre or two. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)

    # The source has specks and a few black scraps at the neck that hang on nothing, and
    # one that sticks to the chest. Taking out that one cuts it off, and only the largest
    # island stays: the sheep.
    uv_layer = mesh.loops.layers.uv.active
    scrap = [face for face in mesh.faces
             if generated.texel_at(colour, face, uv_layer).max() < SCRAP_DARK
             and face.calc_center_median().z > SCRAP_FROM]
    bmesh.ops.delete(mesh, geom=scrap, context="FACES")
    mesh.faces.index_update()
    sheep = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in sheep],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    # The head goes to +X: half a circle about the vertical axis.
    bmesh.ops.transform(mesh, matrix=Matrix.Rotation(math.pi, 4, "Z"), verts=mesh.verts)

    low = Vector((min(corner.co.x for corner in mesh.verts),
                  min(corner.co.y for corner in mesh.verts),
                  min(corner.co.z for corner in mesh.verts)))
    high = Vector((max(corner.co.x for corner in mesh.verts),
                   max(corner.co.y for corner in mesh.verts), 0.0))
    scale = LENGTH / (high.x - low.x)
    across = WIDTH / (high.y - low.y)
    middle = Vector(((low.x + high.x) / 2.0, (low.y + high.y) / 2.0, low.z))
    for corner in mesh.verts:
        offset = corner.co - middle
        corner.co = Vector((offset.x * scale, offset.y * across, offset.z * scale - SINK))
    return mesh, scale


def check(mesh):
    """Stops when the sheep has a size the game does not expect."""
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    zs = [corner.co.z for corner in mesh.verts]
    length = max(xs) - min(xs)
    width = max(ys) - min(ys)
    if abs(length - LENGTH) > 0.0001 or abs(max(xs) + min(xs)) > 0.0001:
        raise RuntimeError(f"{NAME}: {length:.4f} m long, the box is {LENGTH} m")
    if abs(width - WIDTH) > 0.0001 or abs(max(ys) + min(ys)) > 0.0001:
        raise RuntimeError(f"{NAME}: {width:.4f} m wide, the box is {WIDTH} m")
    if abs(min(zs) + SINK) > 0.0001:
        raise RuntimeError(f"{NAME}: the underside is at {min(zs):.4f}, not {-SINK}")
    print(f"{NAME}: {width:.4f} m wide, {max(zs):.3f} m high, "
          f"{len(mesh.faces)} triangles")


def write_pictures(colour, normal):
    """Writes the colour picture and the normal map at TEXTURE_SIZE."""
    make_textures.save_png(generated.shrink(colour, TEXTURE_SIZE), "stone_sheep.png")
    generated.save_normal_map(normal, TEXTURE_SIZE, "stone_sheep_normal.png")


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh, _ = clean(model, colour)
    check(mesh)
    write_pictures(colour, normal)

    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The wool is smooth: the exporter writes the normal of every corner as the average of
    # the faces around it. It is made of thousands of small faces, and one normal per face
    # would show every one of them.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    sheep = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(sheep)

    common.assign_textured_material(sheep, "stone_sheep", "stone_sheep.png",
                                    "stone_sheep_normal.png")
    common.export_obj(NAME)

    if shots:
        # From the side with the head on the right, from the front left, and from above.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.3),
            camera_positions=[(0.2, -1.9, 0.6), (1.5, 1.2, 0.9), (-0.3, -0.5, 2.2)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
