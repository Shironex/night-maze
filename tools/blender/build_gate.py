# Builds the model gate: the closed oak door of the exit, which fills one side of a maze
# cell between two pillars, in the place of a wall segment, under the stone arch
# (build_gate_arch.py). It sinks into the ground when the gate opens.
#
# The door is not built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/gate_tripo.glb: an old plank door with two iron bands, rivets and a
# ring handle, one material with a colour picture and a normal map, about 0.98 m tall and
# narrow, with orange wood. This script stretches it to the size of the doorway and turns
# the wood to weathered grey oak.
# Output: assets/models/gate.obj and gate.mtl, and the pictures assets/textures/gate.png
# and gate_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "gate"

SOURCE_FILE = "gate_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space: X is the width of the
# gate, Y is its thickness and Z is the height. After export Z becomes the game Y axis.
# The gate stands like the model wall_straight: along X, centred on the origin, with the
# origin on the floor in the middle of the base.
#
# The gate fills one side of a 2 m maze cell: its two ends disappear in the piers of the
# arch, which start 0.75 m from the middle. It is lower than the springing of the arch,
# and the game sinks it by GATE_SINK_DEPTH (3.3 m, src/game/Round.hpp), more than its
# height. The source is narrow, so it is stretched along X and Z by different factors.
WIDTH = 2.0
HEIGHT = 2.4
# From the front of one band to the ring on the other side. The collision box of the gate
# is that of a wall, 0.3 m thick, and the door stands inside it and inside the piers.
THICKNESS = 0.24

# Two corners of the source closer than this (in its own units) are one corner.
MERGE_DISTANCE = 0.000001

# The colour. Every texel of the source becomes its brightness (Rec. 709 weights of the
# sRGB values) times WOOD_GAIN in the colour WOOD_TINT: the orange wood turns to the
# mid-dark grey brown of oak that stood in the weather, and the iron, which is already a
# dark grey, stays dark. The grain keeps its contrast, because every texel is scaled
# alike. At 0.42 the door read as pale stone in the beam of the flashlight, but most of
# that was the highlight of the stone over it; the game now draws the door with a dull
# highlight of its own (GATE_DOOR_SPECULAR_STRENGTH in src/game/GameplayRenderer.cpp), and
# the wood is lifted to keep the door about as bright as before.
WOOD_GAIN = 0.62
WOOD_TINT = (1.0, 0.88, 0.72)
# The grain and the gaps between the planks of the source are faint, and from close by the
# door read as one smooth melted shape. So the brightness of every texel is pushed away
# from the average of the texels up to GRAIN_RADIUS pixels around it by GRAIN_CONTRAST:
# the grain, the gaps and the rivets get darker or brighter, and the door as a whole keeps
# its light. The normal map gets steeper by GRAIN_RELIEF for the same reason.
GRAIN_RADIUS = 24
GRAIN_CONTRAST = 2.5
GRAIN_RELIEF = 2.5
# The iron bands stand out of the planks on both faces of the door, but on the face the
# player walks up to the source painted them in the colour of the wood, and they read as
# one more plank. A face of the door that looks along Y (FACE_LOOK of its normal at least)
# and stands BAND_PROUD or more in front of the planks of its side is a band, and the
# texels only bands use get IRON_SHARE of their brightness: black iron, with its marks.
FACE_LOOK = 0.85
BAND_PROUD = 0.012
IRON_SHARE = 0.3

# The source pictures are 2048 pixels wide. The door is 2 m wide and seen from close by,
# so the game gets them at 1024.
TEXTURE_SIZE = 1024


def clean(model):
    """Returns the door as a bmesh, in the size and place of the game."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    # The source has specks of its own around the ring. Only the largest island stays.
    mesh.faces.index_update()
    door = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in door],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    low = Vector([min(corner.co[axis] for corner in mesh.verts) for axis in range(3)])
    high = Vector([max(corner.co[axis] for corner in mesh.verts) for axis in range(3)])
    middle = Vector(((low.x + high.x) / 2.0, (low.y + high.y) / 2.0, low.z))
    size = high - low
    scale = Vector((WIDTH / size.x, THICKNESS / size.y, HEIGHT / size.z))
    for corner in mesh.verts:
        corner.co = (corner.co - middle) * scale
    return mesh


def check(mesh):
    """Stops when the door has a size the game does not expect."""
    for axis, size in ((0, WIDTH), (1, THICKNESS)):
        values = [corner.co[axis] for corner in mesh.verts]
        if abs(max(values) - size / 2.0) > 0.0001 or abs(min(values) + size / 2.0) > 0.0001:
            raise RuntimeError(f"{NAME}: from {min(values):.4f} to {max(values):.4f} along "
                               f"axis {axis}, not {size} centred")
    zs = [corner.co.z for corner in mesh.verts]
    if abs(min(zs)) > 0.0001 or abs(max(zs) - HEIGHT) > 0.0001:
        raise RuntimeError(f"{NAME}: from {min(zs):.4f} to {max(zs):.4f} m, not 0 to {HEIGHT}")
    print(f"{NAME}: {len(mesh.faces)} triangles")


def band_faces(mesh):
    """The faces of the iron bands: on each face of the door, the faces that look along Y
    and stand BAND_PROUD or more in front of its planks. The planks are where most of the
    area of that side lies."""
    mesh.normal_update()
    bands = []
    for side in (-1.0, 1.0):
        facing = [face for face in mesh.faces if face.normal.y * side > FACE_LOOK]
        depths = np.array([face.calc_center_median().y * side for face in facing])
        areas = np.array([face.calc_area() for face in facing])
        order = np.argsort(depths)
        planks = depths[order][np.searchsorted(np.cumsum(areas[order]), areas.sum() / 2.0)]
        bands += [face for face, depth in zip(facing, depths) if depth >= planks + BAND_PROUD]
    return bands


def uv_mask(faces, uv_layer):
    """The texels of the picture at TEXTURE_SIZE whose middle lies in a face of the list,
    rows from the bottom."""
    mask = np.zeros((TEXTURE_SIZE, TEXTURE_SIZE), dtype=bool)
    for face in faces:
        corners = np.array([loop[uv_layer].uv[:] for loop in face.loops]) * TEXTURE_SIZE
        low = np.clip(np.floor(corners.min(axis=0)).astype(int), 0, TEXTURE_SIZE)
        high = np.clip(np.ceil(corners.max(axis=0)).astype(int), 0, TEXTURE_SIZE)
        x, y = np.meshgrid(np.arange(low[0], high[0]) + 0.5, np.arange(low[1], high[1]) + 0.5)
        # A texel is inside when it lies on the same side of every edge.
        sides = []
        for first, second in zip(corners, np.roll(corners, -1, axis=0)):
            sides.append((second[0] - first[0]) * (y - first[1])
                         - (second[1] - first[1]) * (x - first[0]))
        sides = np.array(sides)
        inside = (sides >= 0.0).all(axis=0) | (sides <= 0.0).all(axis=0)
        mask[low[1]:high[1], low[0]:high[0]] |= inside
    return mask


def iron_mask(mesh):
    """The texels that the bands use and no other face does."""
    uv_layer = mesh.loops.layers.uv.active
    bands = band_faces(mesh)
    chosen = set(bands)
    iron = uv_mask(bands, uv_layer) & ~uv_mask([face for face in mesh.faces
                                                 if face not in chosen], uv_layer)
    print(f"{NAME}: {len(bands)} faces of the bands, {iron.sum()} texels made iron")
    return iron


def write_pictures(colour, normal, iron):
    """Writes the colour picture, in grey oak with black iron, and the normal map at
    TEXTURE_SIZE."""
    picture = generated.shrink(colour, TEXTURE_SIZE)
    brightness = picture @ np.array((0.2126, 0.7152, 0.0722))
    around = make_textures.blur(brightness, GRAIN_RADIUS)
    brightness = np.clip(around + GRAIN_CONTRAST * (brightness - around), 0.0, 1.0)
    brightness[iron] *= IRON_SHARE
    make_textures.save_png(brightness[..., None] * WOOD_GAIN * np.array(WOOD_TINT), "gate.png")
    generated.save_normal_map(normal, TEXTURE_SIZE, "gate_normal.png", GRAIN_RELIEF)


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean(model)
    check(mesh)
    write_pictures(colour, normal, iron_mask(mesh))

    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The wood is smooth: the exporter writes the normal of every corner as the average of
    # the faces around it.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    gate = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(gate)
    common.assign_textured_material(gate, "gate", "gate.png", "gate_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the gate: from the front and a little to the
        # side, and from close to the floor near one end.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 1.2),
            camera_positions=[(3.0, -6.5, 2.2), (-2.2, -2.6, 0.6)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
