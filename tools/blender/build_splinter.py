# Builds the models of the thing the player collects, a splinter of the moon, and the one
# big piece of a maze, the heartstone. None of them is built in this script: their sources
# are models that Tripo generated, each one material with a colour picture and a normal map.
#
# tools/blender/sources/splinter_tripo.glb is a crescent of pale stone, about a metre from
# horn to horn. This script makes it smaller and stands it on one horn. splinter_a is the
# crescent. splinter_b is three crescents that broke apart where they fell: the same
# crescent and two smaller copies that lean away from it, turned so they show other sides.
# tools/blender/sources/heartstone_tripo.glb is a crown of tall slivers on a spiky base,
# dark outside and pale blue inside. It becomes the heartstone, 0.9 m tall.
#
# The surface of each source is split into the rind of the moon and the fracture (see
# rind_faces), and the two go to the two halves of its picture (see BAND_WIDTH).
# Output: assets/models/splinter_a.obj, splinter_b.obj and heartstone.obj, each with its
# .mtl, and the pictures assets/textures/splinter_moon.png, splinter_moon_normal.png,
# heartstone.png and heartstone_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_splinter.py
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

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The two splinters are 0.4 m tall and the
# heartstone 0.9 m (HEARTSTONE_HEIGHT in src/game/Heartstone.hpp). Their origin is their
# lower point: the game lets them float and turns them about the upright line through it.
MOON_HEIGHT = 0.4
HEARTSTONE_HEIGHT = 0.9

# Two corners of a source closer than this (in its own units, about a metre for the
# whole model) are one corner, as in build_shade.py.
MERGE_DISTANCE = 0.000001

# The crescent has 5878 triangles. A crystal is 0.4 m and up to 40 of them float in a
# maze, drawn again for the reflections, so the crescent keeps about a third of them: the
# Decimate modifier in its collapse mode, which gives the same faces on every run. The
# heartstone is one per maze and keeps its 6359 triangles (its source has a few specks
# beside the crown, which go).
MOON_DECIMATE_RATIO = 0.34
MOON_TRIANGLES = 1998
HEARTSTONE_TRIANGLES = 6359

# The rind and the fracture. A face belongs to the rind when it faces away from the
# middle of the model: the direction from that middle to the face and the normal of the
# face more than a facing apart (the cosine of the angle between them). The crescent lies
# in a plane, its middle is the middle of its arc, and its rind is the outer, convex edge,
# the old surface of the moon: the inner edge and the two broad sides are the fracture,
# about three fifths of the surface, like three of the five sides of the old slivers. The
# middle of the heartstone is its upright axis: the outside of every sliver is rind and
# the inside fracture, and its base up to HEARTSTONE_BASE_SHARE of its height is all
# rind, so the floor of the crown does not glow.
# The surfaces are pitted, and the normal of a single face says little about the side it
# is on: the normals are first averaged with those of the neighbours a number of passes,
# so only the large curves are left. The slivers of the crown are thin, and more passes
# would mix their inside with their outside. At the end a face takes the side most of its
# neighbours are on, SMOOTHING_PASSES times, so the line between the two has no single
# faces on the wrong side.
MOON_RIND_FACING = 0.5
MOON_NORMAL_PASSES = 10
HEARTSTONE_RIND_FACING = 0.3
HEARTSTONE_NORMAL_PASSES = 3
HEARTSTONE_BASE_SHARE = 0.12
SMOOTHING_PASSES = 2

# A picture has two halves, like the old splinter.png: the left half is the rind and the
# right half the fracture. The game makes a surface glow by multiplying its colour with
# one glow value for the whole model, so the dark rind stays stone and the pale fracture
# shines. The source keeps its UVs, squeezed across: a face of the rind gets u from
# RIND_BAND to RIND_BAND + BAND_WIDTH, a face of the fracture from FRACTURE_BAND on, and
# each half of the picture is the whole picture of the source, squeezed the same way.
# Nothing lies within a twentieth of the picture of its edges or of the line between the
# halves, so the mipmaps do not mix the two.
RIND_BAND = 0.06
FRACTURE_BAND = 0.56
BAND_WIDTH = 0.38

# The pictures of the sources are 2048 pixels wide. The game gets them at 1024: each half
# is then about 400 pixels across, for a crystal that comes as close as the hand.
TEXTURE_SIZE = 1024

# The colours. The brightest channel of a texel of the source (sRGB) becomes a brightness,
# so the pits and facets the source painted stay. The crescent is pale and the crown dark,
# so the brightness of a source is first scaled until its average is BRIGHTNESS_MIDDLE
# (about that of the crescent): the rind of both is then an old grey around the 0.27 of
# the old rind, and the fracture a pale white-blue.
BRIGHTNESS_MIDDLE = 0.66
RIND_FLOOR = 0.10
RIND_RANGE = 0.26
RIND_TINT = (1.0, 0.98, 0.93)
FRACTURE_FLOOR = 0.70
FRACTURE_RANGE = 0.30
FRACTURE_TINT = (0.84, 0.93, 1.0)

# splinter_b: the crescent and two smaller copies from the same spot. Each copy is
# (scale, its own turn about the upright, its lean in degrees, the direction of the lean
# in degrees around the upright, where its horn stands). The tallest one is splinter_a.
MOON_COPIES = (
    (1.0, 0.0, 0.0, 0.0, (0.0, 0.0, 0.0)),
    (0.72, 140.0, 30.0, 20.0, (0.015, -0.01, 0.0)),
    (0.56, 250.0, 34.0, 200.0, (-0.015, 0.01, 0.0)),
)


def import_surface(file_name, decimate_ratio, triangles):
    """Imports a source and returns its largest island as a bmesh and its two pictures.

    decimate_ratio is the share of the triangles the Decimate modifier keeps, or None.
    Stops when the surface has not exactly `triangles` triangles.
    """
    model, colour, normal = generated.import_source(file_name)
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a millimetre or less. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    keep = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in keep],
                     context="FACES")
    mesh.to_mesh(model.data)
    mesh.free()
    model.matrix_world = Matrix.Identity(4)

    if decimate_ratio is not None:
        decimate = model.modifiers.new("decimate", "DECIMATE")
        decimate.decimate_type = "COLLAPSE"
        decimate.ratio = decimate_ratio
        decimate.use_collapse_triangulate = True
    mesh = bmesh.new()
    mesh.from_object(model, bpy.context.evaluated_depsgraph_get())
    mesh.faces.index_update()
    mesh.normal_update()
    if len(mesh.faces) != triangles:
        raise RuntimeError(f"{file_name} has {len(mesh.faces)} triangles, not {triangles}")
    return mesh, colour, normal


def fit_circle(points):
    """The middle (x, z) of the circle closest to the points (x, z), least squares."""
    x = points[:, 0]
    z = points[:, 1]
    # x^2 + z^2 + d x + e z + f = 0 is linear in d, e and f.
    rows = np.column_stack((x, z, np.ones_like(x)))
    (d, e, _), *_ = np.linalg.lstsq(rows, -(x * x + z * z), rcond=None)
    return -d / 2.0, -e / 2.0


def rind_faces(mesh, outwards, facing, normal_passes, forced=()):
    """The set of the indices of the faces of the rind.

    outwards(point) is the direction away from the middle of the model at a point.
    `forced` are indices of faces that are rind whatever they face.
    """
    normals = [face.normal.copy() for face in mesh.faces]
    for _ in range(normal_passes):
        normals = [sum((normals[other.index] for other in generated.neighbours(face)),
                       normals[face.index].copy())
                   for face in mesh.faces]
        for normal in normals:
            normal.normalize()
    labels = [face.index in forced or
              normals[face.index].dot(outwards(face.calc_center_median())) > facing
              for face in mesh.faces]
    for _ in range(SMOOTHING_PASSES):
        following = []
        for face in mesh.faces:
            around = [labels[face.index]] + [labels[other.index]
                                             for other in generated.neighbours(face)]
            rind = sum(around)
            # A tie keeps the side the face is on.
            following.append(labels[face.index] if rind * 2 == len(around)
                             else rind * 2 > len(around))
        labels = following
    return {number for number, rind in enumerate(labels) if rind}


def split_uvs(mesh, rind):
    """Squeezes the UVs of every face into the half of its side (see BAND_WIDTH)."""
    uv_layer = mesh.loops.layers.uv.active
    for face in mesh.faces:
        start = RIND_BAND if face.index in rind else FRACTURE_BAND
        for loop in face.loops:
            loop[uv_layer].uv.x = start + BAND_WIDTH * loop[uv_layer].uv.x


def crescent_surface():
    """Returns the crescent as a bmesh standing on its horn, and its two pictures."""
    mesh, colour, normal = import_surface("splinter_tripo.glb", MOON_DECIMATE_RATIO,
                                          MOON_TRIANGLES)

    # The source lies with the crescent in its XZ plane, the horns up.
    centre_x, centre_z = fit_circle(np.array([(corner.co.x, corner.co.z)
                                              for corner in mesh.verts]))

    def outwards(point):
        return Vector((point.x - centre_x, 0.0, point.z - centre_z)).normalized()

    split_uvs(mesh, rind_faces(mesh, outwards, MOON_RIND_FACING, MOON_NORMAL_PASSES))

    # The horn at the left end of the source goes down. The crescent is turned so that its
    # middle (the average of the middles of its faces, each counted by its area) is
    # straight above that horn: the game turns it about the upright line through its
    # origin, and it turns around itself and not around a point beside it.
    horn = min(mesh.verts, key=lambda corner: (corner.co.x, corner.index)).co.copy()
    area = sum(face.calc_area() for face in mesh.faces)
    middle = sum((face.calc_center_median() * face.calc_area() for face in mesh.faces),
                 Vector()) / area
    turn = (middle - horn).rotation_difference(Vector((0.0, 0.0, 1.0))).to_matrix().to_4x4()
    bmesh.ops.transform(mesh, matrix=turn, verts=mesh.verts)

    # MOON_HEIGHT tall, the lowest corner on the origin.
    low = min(mesh.verts, key=lambda corner: (corner.co.z, corner.index)).co.copy()
    top = max(corner.co.z for corner in mesh.verts)
    scale = MOON_HEIGHT / (top - low.z)
    for corner in mesh.verts:
        corner.co = (corner.co - low) * scale
    mesh.normal_update()
    return mesh, colour, normal


def heartstone_surface():
    """Returns the heartstone as a bmesh and its two pictures."""
    mesh, colour, normal = import_surface("heartstone_tripo.glb", None, HEARTSTONE_TRIANGLES)

    # The source stands upright on Blender Z = 0. Its origin goes to the middle of its
    # base, and it is scaled to HEARTSTONE_HEIGHT.
    low = Vector((min(corner.co.x for corner in mesh.verts),
                  min(corner.co.y for corner in mesh.verts),
                  min(corner.co.z for corner in mesh.verts)))
    high = Vector((max(corner.co.x for corner in mesh.verts),
                   max(corner.co.y for corner in mesh.verts),
                   max(corner.co.z for corner in mesh.verts)))
    base = Vector(((low.x + high.x) / 2.0, (low.y + high.y) / 2.0, low.z))
    scale = HEARTSTONE_HEIGHT / (high.z - low.z)
    for corner in mesh.verts:
        corner.co = (corner.co - base) * scale
    mesh.normal_update()

    def outwards(point):
        return Vector((point.x, point.y, 0.0)).normalized()

    floor = {face.index for face in mesh.faces
             if face.calc_center_median().z < HEARTSTONE_BASE_SHARE * HEARTSTONE_HEIGHT}
    split_uvs(mesh, rind_faces(mesh, outwards, HEARTSTONE_RIND_FACING,
                               HEARTSTONE_NORMAL_PASSES, floor))
    return mesh, colour, normal


def band(picture, start):
    """The whole picture squeezed across into the columns from start to start + BAND_WIDTH
    (in parts of the width), at the size of the picture. The columns beyond the band
    repeat its edge."""
    size = picture.shape[1]
    u = ((np.arange(size) + 0.5) / size - start) / BAND_WIDTH
    column = np.clip(np.clip(u, 0.0, 1.0) * size - 0.5, 0.0, size - 1.0)
    left = np.floor(column).astype(int)
    right = np.minimum(left + 1, size - 1)
    share = (column - left)[None, :, None]
    return picture[:, left] * (1.0 - share) + picture[:, right] * share


def write_pictures(colour, normal, picture_file, normal_file):
    """Writes the colour picture of the two halves and its normal map."""
    half = colour.shape[1] // 2
    brightness = np.clip(colour.max(-1) * (BRIGHTNESS_MIDDLE / colour.max(-1).mean()),
                         0.0, 1.0)[..., None]
    rind = band(brightness, RIND_BAND)[..., 0]
    fracture = band(brightness, FRACTURE_BAND)[..., 0]
    picture = np.empty_like(colour)
    picture[:, :half] = ((RIND_FLOOR + RIND_RANGE * rind[:, :half])[..., None]
                         * np.array(RIND_TINT))
    picture[:, half:] = ((FRACTURE_FLOOR + FRACTURE_RANGE * fracture[:, half:])[..., None]
                         * np.array(FRACTURE_TINT))
    make_textures.save_png(generated.shrink(picture, TEXTURE_SIZE), picture_file)

    directions = np.empty_like(normal)
    directions[:, :half] = band(normal, RIND_BAND)[:, :half]
    directions[:, half:] = band(normal, FRACTURE_BAND)[:, half:]
    generated.save_normal_map(directions, TEXTURE_SIZE, normal_file)


def export(name, mesh, height, material, shots, shot_cameras):
    """Writes the bmesh as one textured model with smooth normals.

    material is (its name, the colour picture, the normal map). Stops when the model is
    not `height` tall over its origin or a UV lies outside the halves of the picture.
    """
    common.reset_scene()
    data = bpy.data.meshes.new(name)
    mesh.to_mesh(data)
    # A generated surface of small faces: the exporter writes the normal of every corner
    # as the average of the faces around it, as for the shade.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    model = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(model)

    lowest = min(corner.co.z for corner in data.vertices)
    top = max(corner.co.z for corner in data.vertices)
    if lowest < -0.001 or abs(top - height) > 0.000001:
        raise RuntimeError(f"{name} reaches from {lowest:.4f} m to {top:.4f} m, "
                           f"not from 0 to {height} m")
    for loop in data.uv_layers[0].data:
        u = loop.uv.x
        if not (RIND_BAND <= u <= RIND_BAND + BAND_WIDTH or
                FRACTURE_BAND <= u <= FRACTURE_BAND + BAND_WIDTH):
            raise RuntimeError(f"{name} has u = {u:.4f} outside the halves")

    common.assign_textured_material(model, *material)
    common.export_obj(name)

    if shots:
        common.render_review_shots(name, target=(0.0, 0.0, height / 2.0),
                                   camera_positions=shot_cameras)


def build_splinters(shots):
    mesh, colour, normal = crescent_surface()
    write_pictures(colour, normal, "splinter_moon.png", "splinter_moon_normal.png")
    material = ("splinter_moon", "splinter_moon.png", "splinter_moon_normal.png")
    # Three views: from the side of the rind, from the side of the fracture, from above.
    cameras = [(1.0, -0.6, 0.35), (-0.9, 0.7, 0.3), (0.4, -0.5, 1.1)]

    export("splinter_a", mesh, MOON_HEIGHT, material, shots, cameras)

    # The copies are added one after another to one bmesh: from_mesh adds to what the
    # bmesh already has.
    copy_data = bpy.data.meshes.new("splinter_copy")
    mesh.to_mesh(copy_data)
    cluster = bmesh.new()
    for scale, own_turn, lean, lean_turn, horn in MOON_COPIES:
        first = len(cluster.verts)
        cluster.from_mesh(copy_data)
        cluster.verts.ensure_lookup_table()
        place = (Matrix.Translation(horn)
                 @ Matrix.Rotation(math.radians(lean_turn), 4, "Z")
                 @ Matrix.Rotation(math.radians(lean), 4, "X")
                 @ Matrix.Rotation(math.radians(own_turn - lean_turn), 4, "Z")
                 @ Matrix.Scale(scale, 4))
        added = cluster.verts[first:]
        bmesh.ops.transform(cluster, matrix=place, verts=added)
        # A leaning copy turns the corners beside its horn below it. It is lifted until
        # none lies below the origin, so the origin stays the lowest point.
        dip = min(corner.co.z for corner in added)
        if dip < 0.0:
            bmesh.ops.translate(cluster, vec=(0.0, 0.0, -dip), verts=added)
    export("splinter_b", cluster, MOON_HEIGHT, material, shots, cameras)
    cluster.free()
    mesh.free()


def build_heartstone(shots):
    mesh, colour, normal = heartstone_surface()
    write_pictures(colour, normal, "heartstone.png", "heartstone_normal.png")
    # Three views: from the side, from a player who looks down into the crown, from below.
    export("heartstone", mesh, HEARTSTONE_HEIGHT,
           ("heartstone", "heartstone.png", "heartstone_normal.png"), shots,
           [(1.8, -1.0, 0.7), (-1.0, 1.2, 1.9), (1.2, 1.4, -0.4)])
    mesh.free()


def build(shots):
    build_splinters(shots)
    build_heartstone(shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
