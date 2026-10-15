# Builds the model milestone: a worn stone slab about a metre high that stands against the
# wall of the last cell before the exit gate, where the lamplighters turned in. The stone is
# not built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/milestone_tripo.glb: a tall slab with a rounded top and a worn,
# hollowed face with no letters on it, moss at its foot, one material with a colour picture
# and a normal map. This script cleans it up and writes the game files, the same bytes on
# every run.
# Output: assets/models/milestone.obj and milestone.mtl, and the pictures
# assets/textures/milestone.png and milestone_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_milestone.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "milestone"

SOURCE_FILE = "milestone_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The stone is upright, with its origin on the
# ground in the middle of its base. It has no collision box in the game: it stands so
# close to the wall that the body of the player cannot reach its middle. The source
# already stands on Blender Z = 0. clean() turns the slab about the vertical axis so that it
# is wide along X and thin along Y: its broad faces look along game +Z and -Z.

# The top of the stone is this high over the ground (the origin): knee to thigh high. At
# this size the slab is under 0.3 m thick, thin enough to stand between the box of its
# wall and the place the body of the player can reach.
HEIGHT = 0.75

# The stone and its foot reach this far below the ground. The game puts the origin on the
# ground at the middle of the stone, and on a slope one side of it is lower than that. The
# moss lumps at the foot of the source lie under the ground then.
BOTTOM = -0.15

# Two corners of the source closer than this (in its own units, about a metre for the
# whole stone) are one corner. The file has a corner of its own for every face that meets
# at a seam of the picture, and without this the faces of one surface would not know that
# they are neighbours: not for the smooth normals and not for the islands.
MERGE_DISTANCE = 0.000001

# The upper part of the stone, the slab itself above this height over the ground, decides
# how the stone is turned and where its middle is. Below it lie the lumps of moss.
SLAB_FROM = 0.2

# The foot below SLAB_TRIM_BELOW is under the ground. It is pulled in so that it does not
# reach past the footprint of the slab by more than FOOT_OVERHANG on any side.
SLAB_TRIM_BELOW = -0.02
FOOT_OVERHANG = 0.05

# The pictures of the source are 2048 pixels wide. The game gets them at a quarter of
# that, every texel the average of 4 by 4 of the source.
TEXTURE_SIZE = 512


def clean(model):
    """Returns the cleaned surface as a bmesh, in the size and place of the game."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a millimetre or two. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)

    # The source has three crumbs of stone lying at the foot. Only the largest island
    # stays: the stone.
    stone = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in stone],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    top = max(corner.co.z for corner in mesh.verts)
    scale = (HEIGHT - BOTTOM) / top
    for corner in mesh.verts:
        corner.co = corner.co * scale + Vector((0.0, 0.0, BOTTOM))

    # Turn the slab about the vertical axis so its broad faces look along Y: the major axis
    # of the corners of its upper part (the principal axis of their footprint) goes to X.
    # The angle is brought into -90 to 90 degrees, so the turn is the same on every run.
    upper = np.array([[c.co.x, c.co.y] for c in mesh.verts if c.co.z >= SLAB_FROM])
    centred = upper - upper.mean(axis=0)
    values, vectors = np.linalg.eigh(centred.T @ centred)
    major = vectors[:, int(np.argmax(values))]
    angle = math.atan2(major[1], major[0])
    angle = (angle + math.pi / 2.0) % math.pi - math.pi / 2.0
    bmesh.ops.transform(mesh, matrix=Matrix.Rotation(-angle, 4, "Z"), verts=mesh.verts)

    # The middle of the upper footprint goes to the origin.
    slab = [c.co for c in mesh.verts if c.co.z >= SLAB_FROM]
    low_x, high_x = min(c.x for c in slab), max(c.x for c in slab)
    low_y, high_y = min(c.y for c in slab), max(c.y for c in slab)
    middle = Vector(((low_x + high_x) / 2.0, (low_y + high_y) / 2.0, 0.0))
    for corner in mesh.verts:
        corner.co -= middle
    low_x, high_x, low_y, high_y = (low_x - middle.x, high_x - middle.x,
                                    low_y - middle.y, high_y - middle.y)

    # The flare of the buried foot is cut back to the footprint of the slab.
    for corner in mesh.verts:
        if corner.co.z < SLAB_TRIM_BELOW:
            corner.co.x = min(max(corner.co.x, low_x - FOOT_OVERHANG), high_x + FOOT_OVERHANG)
            corner.co.y = min(max(corner.co.y, low_y - FOOT_OVERHANG), high_y + FOOT_OVERHANG)
    return mesh


def check(mesh):
    """Stops when the stone has a size the game does not expect."""
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    zs = [corner.co.z for corner in mesh.verts]
    if abs(max(zs) - HEIGHT) > 0.0001 or abs(min(zs) - BOTTOM) > 0.0001:
        raise RuntimeError(f"{NAME}: from {min(zs):.4f} to {max(zs):.4f} m, "
                           f"not {BOTTOM} to {HEIGHT}")
    # Over the ground the stone must not have a foot wider than the slab by more than the
    # lumps of the moss, or it would stand in the wall it leans on.
    above = [corner.co for corner in mesh.verts if corner.co.z >= 0.0]
    print(f"{NAME}: above the ground x from {min(c.x for c in above):.3f} to "
          f"{max(c.x for c in above):.3f}, y from {min(c.y for c in above):.3f} to "
          f"{max(c.y for c in above):.3f}, {len(mesh.faces)} triangles "
          f"(x {min(xs):.3f} {max(xs):.3f}, y {min(ys):.3f} {max(ys):.3f} with the foot)")


def write_pictures(colour, normal):
    """Writes the colour picture and the normal map at TEXTURE_SIZE."""
    make_textures.save_png(generated.shrink(colour, TEXTURE_SIZE), "milestone.png")
    generated.save_normal_map(normal, TEXTURE_SIZE, "milestone_normal.png")


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean(model)
    check(mesh)
    write_pictures(colour, normal)

    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The stone is smooth: the exporter writes the normal of every corner as the average of
    # the faces around it, and the worn surface is made of thousands of small faces.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    stone = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(stone)

    common.assign_textured_material(stone, "milestone", "milestone.png", "milestone_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the stone: from the side of its worn face and
        # from the other side, and from above.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.5),
            camera_positions=[(2.2, -0.6, 0.9), (-2.2, 0.6, 0.9), (0.6, -0.6, 2.2)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
