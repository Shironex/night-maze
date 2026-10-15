# Builds the model gate_bell: the bronze bell that hangs under the crown of the arch of the
# exit gate (build_gate_arch.py), over the doorway. The bell, its clapper, the loop on its
# crown and the rope it hangs from. The game swings it each time the bell of the open
# gate tolls.
#
# The bell is not built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/gate_bell_tripo.glb: a bronze bell with green patina hanging by a
# short rope from a black iron wall bracket, one material with a colour picture and a
# normal map, about 0.98 m tall. The game turns the whole model about its pivot, so only
# what swings stays: the bracket would swing with the bell. The rope is cut where it met
# the bracket, and that point is the pivot.
# Output: assets/models/gate_bell.obj and gate_bell.mtl, and the pictures
# assets/textures/gate_bell.png and gate_bell_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate_bell.py
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

NAME = "gate_bell"

SOURCE_FILE = "gate_bell_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The origin is the pivot, the top of the rope.
# Everything hangs below it, so every height is negative. The game puts the pivot under
# the crown of the arch and turns the model around the axis across the gate (Blender Y),
# so the bell swings along the gate (GATE_BELL_ALONG, GATE_BELL_HEIGHT and
# GATE_BELL_SWING_DEGREES in src/game/GateLamp.hpp).
#
# From the pivot to the underside of the clapper, the lowest point.
LENGTH = 0.6

# Two corners of the source closer than this (in its own units) are one corner.
MERGE_DISTANCE = 0.000001

# What swings, in the units of the source: the faces whose middle lies below the height
# where the rope met the bracket (CUT_Z), and above BRACKET_FROM_Z only on the side of
# the bell (X above KEEP_FROM_X). The beam and the hanger are above CUT_Z, the brace at
# smaller X and above BRACKET_FROM_Z, where the bell is narrower than lower down at its
# lip. The middle of the bell is at BELL_X.
KEEP_FROM_X = 0.1
BRACKET_FROM_Z = 0.25
CUT_Z = 0.735
BELL_X = 0.247

# The bell is closed in the source: the inside of its wall has faces of its own that look
# inwards, so nothing is added for the renderer, which draws no back faces.

# The source pictures are 2048 pixels wide. The bell is about 0.3 m wide and hangs 3 m up,
# so 512 is plenty.
TEXTURE_SIZE = 512


def clean(model):
    """Returns the bell, its rope and its clapper as a bmesh, in the size and place of the
    game."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)

    bracket = []
    for face in mesh.faces:
        middle = face.calc_center_median()
        if middle.z >= CUT_Z or (middle.x <= KEEP_FROM_X and middle.z > BRACKET_FROM_Z):
            bracket.append(face)
    bmesh.ops.delete(mesh, geom=bracket, context="FACES")
    # The cut leaves no pieces of the bracket that hang on nothing, but the source has
    # specks of its own. Only the largest island stays: the bell on its rope.
    mesh.faces.index_update()
    bell = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in bell],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    top = max(corner.co.z for corner in mesh.verts)
    bottom = min(corner.co.z for corner in mesh.verts)
    scale = LENGTH / (top - bottom)
    pivot = Vector((BELL_X, 0.0, top))
    for corner in mesh.verts:
        corner.co = (corner.co - pivot) * scale
    return mesh


def check(mesh):
    """Stops when the bell has a size the game does not expect."""
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    zs = [corner.co.z for corner in mesh.verts]
    if abs(max(zs)) > 0.0001 or abs(min(zs) + LENGTH) > 0.0001:
        raise RuntimeError(f"{NAME}: from {max(zs):.4f} down to {min(zs):.4f}, not 0 to "
                           f"{-LENGTH}")
    # The lip is the widest part: it should be round and about on the axis of the pivot.
    if abs(max(xs) + min(xs)) > 0.02 or abs(max(ys) + min(ys)) > 0.02:
        raise RuntimeError(f"{NAME}: the bell is not under its pivot")
    print(f"{NAME}: {max(xs) - min(xs):.3f} by {max(ys) - min(ys):.3f} m wide, "
          f"{len(mesh.faces)} triangles")


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean(model)
    check(mesh)
    make_textures.save_png(generated.shrink(colour, TEXTURE_SIZE), "gate_bell.png")
    generated.save_normal_map(normal, TEXTURE_SIZE, "gate_bell_normal.png")

    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The bronze is smooth: the exporter writes the normal of every corner as the average
    # of the faces around it.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    bell = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(bell)
    common.assign_textured_material(bell, "gate_bell", "gate_bell.png", "gate_bell_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the bell: from the front and a little above,
        # and from below, into the bell, as the player sees it from the gate.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, -0.35),
            camera_positions=[(0.6, -1.1, -0.1), (0.3, -0.6, -1.3)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
