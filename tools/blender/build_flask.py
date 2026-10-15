# Builds the model flask: a round glass bottle of tea in a net of cord, with a cork and a
# leather strap round its neck. The player picks it up to get stamina back. The bottle is
# not built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/flask_tripo.glb: one material with a colour picture and a normal
# map. This script cleans it up and writes the game files, the same bytes on every run.
# Output: assets/models/flask.obj and flask.mtl, and the pictures
# assets/textures/flask_bottle.png and flask_bottle_normal.png (the picture flask.png is
# the one the ropes of the lever wear, see build_crook.py, so it stays what it was).
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_flask.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

import bmesh
import bpy
from mathutils import Vector

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "flask"

SOURCE_FILE = "flask_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The source already stands upright on Blender
# Z = 0, with its cork at the top. The flask is HEIGHT tall, cork included, and its origin
# is on the floor in the middle of its base, like the origin of the crystals: the game
# floats it low over the ground (game::FLASK_FLOAT_HEIGHT) and turns it about the vertical
# axis through that origin. The first flask was 0.25 m tall and 0.18 m wide. This one is
# nearly round, so at this height it is 0.17 m wide, the size of a pickup.
HEIGHT = 0.2

# Two corners of the source closer than this (in its own units, about a metre for the
# whole bottle) are one corner. The file has a corner of its own for every face that meets
# at a seam of the picture, and without this the faces of one surface would not know that
# they are neighbours: not for the smooth normals and not for the islands.
MERGE_DISTANCE = 0.000001

# The pictures of the source are 2048 pixels wide. The game gets them at a quarter of
# that, every texel the average of 4 by 4 of the source: the bottle is a hand wide.
TEXTURE_SIZE = 512


def clean(model):
    """Returns the cleaned surface as a bmesh, in the size and place of the game."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a millimetre or two. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)

    # Knots of cord and flecks of the net hang loose around the bottle in the source. Only
    # the largest island stays: the bottle in its net.
    bottle = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in bottle],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    low = Vector((min(corner.co.x for corner in mesh.verts),
                  min(corner.co.y for corner in mesh.verts),
                  min(corner.co.z for corner in mesh.verts)))
    high = Vector((max(corner.co.x for corner in mesh.verts),
                   max(corner.co.y for corner in mesh.verts),
                   max(corner.co.z for corner in mesh.verts)))
    scale = HEIGHT / (high.z - low.z)
    middle = Vector(((low.x + high.x) / 2.0, (low.y + high.y) / 2.0, low.z))
    for corner in mesh.verts:
        corner.co = (corner.co - middle) * scale
    return mesh


def check(data):
    """Stops when the flask is not the size and in the place the game expects."""
    corners = [corner.co for corner in data.vertices]
    height = max(corner.z for corner in corners)
    lowest = min(corner.z for corner in corners)
    if abs(height - HEIGHT) > 0.0001 or abs(lowest) > 0.0001:
        raise RuntimeError(f"{NAME}: from {lowest:.4f} to {height:.4f} m, not 0 to {HEIGHT}")
    width_x = max(corner.x for corner in corners) - min(corner.x for corner in corners)
    width_y = max(corner.y for corner in corners) - min(corner.y for corner in corners)
    print(f"{NAME}: {width_x:.3f} by {width_y:.3f} m wide, {height:.3f} m high, "
          f"{len(data.polygons)} triangles")


def write_pictures(colour, normal):
    """Writes the colour picture and the normal map at TEXTURE_SIZE."""
    make_textures.save_png(generated.shrink(colour, TEXTURE_SIZE), "flask_bottle.png")
    generated.save_normal_map(normal, TEXTURE_SIZE, "flask_bottle_normal.png")


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean(model)
    write_pictures(colour, normal)

    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    check(data)
    # The bottle is smooth: the exporter writes the normal of every corner as the average
    # of the faces around it, and the net is made of thousands of small faces.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    flask = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(flask)

    common.assign_textured_material(flask, "flask", "flask_bottle.png", "flask_bottle_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the flask: from the side, and from above.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.1),
            camera_positions=[(0.0, -0.55, 0.2), (0.35, -0.3, 0.45)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
