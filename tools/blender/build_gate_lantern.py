# Builds the model gate_lantern: a square lantern of dark iron with a flared roof, a ring
# on top and four panes of pale glass. The game draws it three times at the exit gate:
# twice as it is, hanging from the hooks on the piers, and once twice as large on the
# crown of the arch.
#
# The iron is not built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/gate_lantern_tripo.glb: a square iron lantern with a flared roof,
# a candle inside and frosted panes, one material with a colour picture and a normal map,
# about 0.98 m tall. The panes of the source are thin and one side has none, so they come
# out, and four flat panes built here close the four windows. They are the part that
# glows.
# Output: assets/models/gate_lantern.obj and gate_lantern.mtl, and the pictures
# assets/textures/gate_lantern.png and gate_lantern_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate_lantern.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "gate_lantern"

SOURCE_FILE = "gate_lantern_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The lantern is upright, 0.36 m tall to the top
# of its ring, with its origin in the middle of its base. The roof is its widest part,
# about 0.2 m. The source already stands on Blender Z = 0, and its four sides look along
# the axes. The game never turns it: the windows look the same on all four sides.
HEIGHT = 0.36
# The roof may not be wider than this from the middle: the game keeps the bracket lanterns
# this far clear of every wall and pillar.
MAX_HALF_WIDTH = 0.12

# Two corners of the source closer than this (in its own units) are one corner.
MERGE_DISTANCE = 0.000001

# The panes of the source: faces with a pale grey texel (every channel above PANE_PALE
# and the channels at most PANE_GREY apart) that lie in the plane of the glass, between
# PANE_INNER and PANE_OUTER from the middle, and between PANE_LOW and PANE_HIGH up (all
# in the units of the source). The frame stands in front of them, from about 0.174.
PANE_PALE = 0.4
PANE_GREY = 0.08
PANE_INNER = 0.14
PANE_OUTER = 0.170
PANE_LOW = 0.15
PANE_HIGH = 0.6

# The four new panes. A window is found by looking at each side along a grid of rays
# from outside, WINDOW_STEP apart (metres, in the size of the game): where a ray gets
# through the frame, it went through a window. A pane covers its window
# and reaches PANE_OVERLAP further on every side, behind the frame, so no gap shows
# between them, and it lies PANE_DEPTH from the middle (in metres), the plane of the
# glass of the source.
WINDOW_STEP = 0.002
# The rays only look between these heights (metres): the windows are there, and the ring
# on the roof, which a ray also passes through, is not.
WINDOW_LOW = 0.04
WINDOW_HIGH = 0.24
PANE_OVERLAP = 0.004
PANE_DEPTH = 0.0615
# A ray went through a window when it gets this close to the middle: well past the glass,
# so a ray that only grazes the round side of a corner post does not count.
WINDOW_DEPTH = 0.045

# The iron of the source is near black with pale specks. The game makes the whole model
# glow by its colour, so every texel of the picture is darkened until its brightest
# channel is at most IRON_MAX (sRGB): the frame of the old lantern, made in
# make_textures.py, was no brighter. Otherwise the specks would sparkle.
IRON_MAX = 0.14

# The panes glow. They get a square of the picture that no face of the iron uses, painted
# in GLASS_COLOUR, the pale old glass of lantern.png (make_textures.py, also sRGB). The
# square is PATCH texels wide if the picture has room for it, else the next smaller
# size. A large square keeps the panes pale in the smaller copies of the picture the
# graphics card uses from far away.
GLASS_COLOUR = (0.86, 0.84, 0.76)
PATCH_SIZES = (64, 32, 16, 8)
# A pane (about 0.09 m) covers this many UV units per metre of the square.
PANE_UV_UNITS_PER_METRE = 0.1

# The source pictures are 2048 pixels wide. The lantern is 0.36 m tall, so 512 is plenty.
TEXTURE_SIZE = 512


def is_pane(face, colour, uv_layer):
    texel = generated.texel_at(colour, face, uv_layer)
    middle = face.calc_center_median()
    depth = max(abs(middle.x), abs(middle.y))
    return (texel.min() > PANE_PALE and texel.max() - texel.min() < PANE_GREY
            and PANE_INNER < depth < PANE_OUTER and PANE_LOW < middle.z < PANE_HIGH)


def clean(model, colour):
    """Returns the iron of the lantern as a bmesh, in the size and place of the game."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    uv_layer = mesh.loops.layers.uv.active

    panes = [face for face in mesh.faces if is_pane(face, colour, uv_layer)]
    bmesh.ops.delete(mesh, geom=panes, context="FACES")
    # Without the panes the candle can stand on its own. Only the largest island stays.
    mesh.faces.index_update()
    iron = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in iron],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    top = max(corner.co.z for corner in mesh.verts)
    scale = HEIGHT / top
    low = Vector((min(corner.co.x for corner in mesh.verts),
                  min(corner.co.y for corner in mesh.verts), 0.0))
    high = Vector((max(corner.co.x for corner in mesh.verts),
                   max(corner.co.y for corner in mesh.verts), 0.0))
    middle = (low + high) / 2.0
    for corner in mesh.verts:
        corner.co = (corner.co - middle) * scale
    return mesh


def find_windows(mesh):
    """Returns one (axis, side, low, high) per side of the lantern: the rectangle of its
    window in the plane of that side. axis is 0 for the sides that look along X and 1
    for those along Y, side is -1 or +1, and low and high are the corners of the window
    as (along the side, height)."""
    tree = BVHTree.FromBMesh(mesh)
    windows = []
    steps = round(MAX_HALF_WIDTH / WINDOW_STEP)
    rows = range(round(WINDOW_LOW / WINDOW_STEP), round(WINDOW_HIGH / WINDOW_STEP) + 1)
    for axis in (0, 1):
        for side in (-1.0, 1.0):
            inside = []
            for row in rows:
                height = row * WINDOW_STEP
                # For every ray of the row: did it stop in front, at the frame?
                frame = []
                for column in range(-steps, steps + 1):
                    start = [0.0, 0.0, height]
                    start[axis] = side * 1.0
                    start[1 - axis] = column * WINDOW_STEP
                    direction = [0.0, 0.0, 0.0]
                    direction[axis] = -side
                    hit = tree.ray_cast(Vector(start), Vector(direction), 2.0)[0]
                    frame.append(hit is not None and side * hit[axis] >= WINDOW_DEPTH)
                # The rays between the frame on the left and the frame on the right that
                # do not stop at it went through the window: they hit nothing (the window
                # on the far side is open too) or something deep inside. The rays outside
                # the frame went past the lantern.
                columns = [index for index, stopped in enumerate(frame) if stopped]
                if not columns:
                    continue
                for index in range(columns[0], columns[-1] + 1):
                    if not frame[index]:
                        inside.append(((index - steps) * WINDOW_STEP, height))
            low = (min(point[0] for point in inside), min(point[1] for point in inside))
            high = (max(point[0] for point in inside), max(point[1] for point in inside))
            windows.append((axis, side, low, high))
    return windows


def free_square(mesh, uv_layer):
    """Finds a square of the picture (at TEXTURE_SIZE) that no face uses.

    Returns its lower left texel and its size. A face uses every texel of the rectangle
    around its UVs, which is more than it covers, so the square is free for sure.
    """
    used = np.zeros((TEXTURE_SIZE, TEXTURE_SIZE), dtype=bool)
    for face in mesh.faces:
        us = [loop[uv_layer].uv.x * TEXTURE_SIZE for loop in face.loops]
        vs = [loop[uv_layer].uv.y * TEXTURE_SIZE for loop in face.loops]
        used[max(0, int(min(vs)) - 1):int(max(vs)) + 2,
             max(0, int(min(us)) - 1):int(max(us)) + 2] = True
    for size in PATCH_SIZES:
        # The squares of this size on a grid of the size, from the bottom row up.
        for row in range(0, TEXTURE_SIZE, size):
            for column in range(0, TEXTURE_SIZE, size):
                if not used[row:row + size, column:column + size].any():
                    return (column, row), size
    raise RuntimeError(f"{NAME}: the picture has no free square for the glass")


def add_panes(mesh, windows, uv_layer, patch, size):
    """Adds one flat pane per window, looking outwards, with the patch of glass as its
    picture. Returns the new faces."""
    # A texel of margin inside the square, so the picture never blends in the iron
    # around it.
    u0 = (patch[0] + 1) / TEXTURE_SIZE
    v0 = (patch[1] + 1) / TEXTURE_SIZE
    u1 = (patch[0] + size - 1) / TEXTURE_SIZE
    v1 = (patch[1] + size - 1) / TEXTURE_SIZE
    panes = []
    for axis, side, low, high in windows:
        corners = []
        for across, height in ((low[0], low[1]), (high[0], low[1]), (high[0], high[1]),
                               (low[0], high[1])):
            across += PANE_OVERLAP if across > (low[0] + high[0]) / 2.0 else -PANE_OVERLAP
            height += PANE_OVERLAP if height > (low[1] + high[1]) / 2.0 else -PANE_OVERLAP
            point = [0.0, 0.0, height]
            point[axis] = side * PANE_DEPTH
            point[1 - axis] = across
            corners.append(mesh.verts.new(point))
        face = mesh.faces.new(corners)
        face.normal_update()
        outwards = Vector((0.0, 0.0, 0.0))
        outwards[axis] = side
        if face.normal.dot(outwards) < 0.0:
            face.normal_flip()
        face.smooth = False
        # The pane is projected onto its own plane, as common.face_project_uvs does, and
        # shrunk into the square around its middle: never a mirror image, and always
        # inside the plain colour.
        up = Vector((0.0, 0.0, 1.0))
        right = up.cross(face.normal)
        middle = face.calc_center_median()
        for loop in face.loops:
            offset = loop.vert.co - middle
            loop[uv_layer].uv = ((u0 + u1) / 2.0 + offset.dot(right) * PANE_UV_UNITS_PER_METRE,
                                 (v0 + v1) / 2.0 + offset.dot(up) * PANE_UV_UNITS_PER_METRE)
        reach = max(abs(loop[uv_layer].uv.x - (u0 + u1) / 2.0) for loop in face.loops)
        if reach > (u1 - u0) / 2.0:
            raise RuntimeError(f"{NAME}: a pane is larger than its square of the picture")
        panes.append(face)
    return panes


def check(mesh, windows):
    """Stops when the lantern has a size the game does not expect."""
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    zs = [corner.co.z for corner in mesh.verts]
    if abs(max(zs) - HEIGHT) > 0.0001 or abs(min(zs)) > 0.0001:
        raise RuntimeError(f"{NAME}: from {min(zs):.4f} to {max(zs):.4f} m, not 0 to {HEIGHT}")
    half = max(max(xs), -min(xs), max(ys), -min(ys))
    if half > MAX_HALF_WIDTH:
        raise RuntimeError(f"{NAME}: {half:.4f} m from the middle, at most {MAX_HALF_WIDTH}")
    for axis, side, low, high in windows:
        width = high[0] - low[0]
        tall = high[1] - low[1]
        # A window of the source is about 0.08 m wide and 0.09 m tall in this size.
        if not (0.05 < width < 0.12 and 0.06 < tall < 0.14):
            raise RuntimeError(f"{NAME}: a window of {width:.3f} by {tall:.3f} m at {low}")
    glass = sum(high[1] + low[1] for _, _, low, high in windows) / 2.0 / len(windows)
    print(f"{NAME}: {half:.4f} m from the middle, the middle of the glass {glass:.3f} m up, "
          f"{len(mesh.faces)} triangles")


def write_pictures(colour, normal, patch, size):
    """Writes the colour picture, darkened and with the square of glass, and the normal
    map, flat on the square, at TEXTURE_SIZE."""
    picture = generated.shrink(colour, TEXTURE_SIZE)
    brightest = picture.max(axis=-1, keepdims=True)
    picture = picture * np.minimum(1.0, IRON_MAX / np.maximum(brightest, 0.000001))
    column, row = patch
    # The arrays have their rows from the bottom, like the UVs.
    picture[row:row + size, column:column + size] = GLASS_COLOUR
    make_textures.save_png(picture, "gate_lantern.png")

    directions = generated.shrink(normal, TEXTURE_SIZE) * 2.0 - 1.0
    directions /= np.linalg.norm(directions, axis=-1, keepdims=True)
    directions[row:row + size, column:column + size] = (0.0, 0.0, 1.0)
    make_textures.save_png(directions * 0.5 + 0.5, "gate_lantern_normal.png")


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean(model, colour)
    uv_layer = mesh.loops.layers.uv.active
    # The iron is smooth: the exporter writes the normal of every corner as the average of
    # the faces around it. The panes are flat.
    for face in mesh.faces:
        face.smooth = True
    windows = find_windows(mesh)
    patch, size = free_square(mesh, uv_layer)
    add_panes(mesh, windows, uv_layer, patch, size)
    bmesh.ops.triangulate(mesh, faces=mesh.faces[:])
    check(mesh, windows)
    print(f"{NAME}: the glass is a square of {size} texels at {patch}")
    write_pictures(colour, normal, patch, size)

    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    lantern = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(lantern)
    common.assign_textured_material(lantern, "gate_lantern", "gate_lantern.png",
                                    "gate_lantern_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the lantern: from the side and a little above,
        # and from below, as the player sees it hanging.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.17),
            camera_positions=[(0.45, -0.7, 0.35), (0.3, -0.45, -0.15)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
