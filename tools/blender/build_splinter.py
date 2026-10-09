# Builds the two models of the thing the player collects, a splinter of the moon:
# splinter_a is one sliver, splinter_b is three slivers that broke apart where they fell.
# A sliver is long and thin. Two of its five sides are the rind of the moon, grey and
# pitted, the other three are clean fracture, and those glow.
# Output: assets/models/splinter_a.obj, splinter_a.mtl, splinter_b.obj and splinter_b.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_splinter.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

from mathutils import Vector

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. Both models are 0.5 m tall (0.496 m, because the
# slivers lean) and their origin is the lower point of the main sliver: the game lets them
# float and turns them about the upright line through that point.

# A sliver is built ring by ring, like the flask, and every ring has these five corners:
# the angle of the corner around the axis in degrees, and how far out it lies (1 = the
# width of the sliver). The first three corners lie on the outer arc, so the two faces
# between them are the rind, curved like the surface the sliver was broken from. The
# other three faces are the fracture.
SECTION = ((-40.0, 1.0), (0.0, 1.1), (40.0, 1.0), (132.0, 0.74), (228.0, 0.8))
RIND_SIDES = 2

# The rings from the lower point to the upper one: where the ring lies along the length
# (0 to 1) and how wide it is (0 = the ring is a single point).
RINGS = ((0.0, 0.0), (0.1, 0.55), (0.32, 0.95), (0.58, 1.0), (0.8, 0.7), (1.0, 0.0))

# A sliver is flatter than it is wide: the direction across the rind is this much narrower.
FLATTEN = 0.72

# Nothing broken is regular. Every corner lies between 88 % and 112 % of its distance from
# the axis, and each of the two points up to 6 mm beside it. The numbers come from the
# seed of the sliver, so every run writes the same file.
CORNER_JITTER_LOW = 0.88
CORNER_JITTER_RANGE = 0.24
POINT_JITTER = 0.012

# The picture splinter.png has two halves (make_textures.py): the left half is the rind
# and the right half the fracture. Every side of a sliver gets a piece from the middle of
# its half, like the faces of the gate lantern. The rind is dark and the fracture pale,
# and the game makes a surface glow by multiplying its colour with one glow value, so the
# fracture shines and the rind stays stone.
RIND_U_CENTRE = 0.25
FRACTURE_U_CENTRE = 0.75
# One metre of the model is this many units of u and v. The widest side then covers
# a third of the picture and stays inside its half (tests/ObjLoaderTests.cpp checks that).
# The piece of a side lies as far up the picture as the side lies along the sliver (the
# picture continues from its top edge to its bottom edge), so the sides of two rings do
# not show the same craters.
UV_UNITS_PER_METRE = 3.0


def random_numbers(seed):
    """Returns a function that gives the next number from 0 to 1 of a fixed sequence.

    The generator is written out here (it is the one known as mulberry32) and uses whole
    numbers of 32 bits only, so the sequence of a seed is the same in every version of
    Python and of Blender.
    """
    state = seed & 0xFFFFFFFF

    def following():
        nonlocal state
        state = (state + 0x6D2B79F5) & 0xFFFFFFFF
        mixed = ((state ^ (state >> 15)) * (1 | state)) & 0xFFFFFFFF
        mixed = ((mixed + ((mixed ^ (mixed >> 7)) * (61 | mixed))) & 0xFFFFFFFF) ^ mixed
        return (mixed ^ (mixed >> 14)) / 4294967296.0

    return following


def add_sliver(vertices, faces, uvs, base, axis, length, width, twist, seed):
    """Appends one sliver to the lists `vertices`, `faces` and `uvs`.

    base: the lower point (x, y, z). axis: the direction from it to the upper point, of
    any length. length, width: the size of the sliver, the width measured from the axis
    to the rind. twist: by how many degrees the last ring is turned against the first.
    seed: the seed of the jitter of this sliver.

    faces gets triangles only. uvs gets one entry per triangle: the (u, v) of its three
    corners.
    """
    following = random_numbers(seed)
    base = Vector(base)
    along = Vector(axis).normalized()
    # Two directions across the sliver. across points at the middle of the rind.
    reference = Vector((0.0, 0.0, 1.0)) if abs(along.z) < 0.9 else Vector((1.0, 0.0, 0.0))
    across = reference.cross(along).normalized()
    flat = along.cross(across)

    # The index of every corner, ring by ring. A ring that is a point has one vertex,
    # listed once for every corner, so the code below needs no special case to find it.
    rings = []
    for position, ring_width in RINGS:
        centre = base + along * (position * length)
        if ring_width == 0.0:
            vertices.append(tuple(centre + across * ((following() - 0.5) * POINT_JITTER)))
            rings.append([len(vertices) - 1] * len(SECTION))
            continue
        turn = math.radians(twist * position)
        ring = []
        for angle, reach in SECTION:
            angle = math.radians(angle) + turn
            reach = width * ring_width * reach
            reach = reach * (CORNER_JITTER_LOW + following() * CORNER_JITTER_RANGE)
            corner = centre + across * (math.cos(angle) * reach)
            corner = corner + flat * (math.sin(angle) * reach * FLATTEN)
            vertices.append(tuple(corner))
            ring.append(len(vertices) - 1)
        rings.append(ring)

    for number in range(len(RINGS) - 1):
        lower = rings[number]
        upper = rings[number + 1]
        middle = (RINGS[number][0] + RINGS[number + 1][0]) / 2.0
        inside = base + along * (length * middle)
        for side in range(len(SECTION)):
            following_side = (side + 1) % len(SECTION)
            # The four corners of this side between the two rings. At a point two of
            # them are the same vertex, and the side is a triangle.
            corners = [lower[side], lower[following_side], upper[following_side], upper[side]]
            corners = [corner for place, corner in enumerate(corners)
                       if corner != corners[place - 1]]
            u_centre = RIND_U_CENTRE if side < RIND_SIDES else FRACTURE_U_CENTRE
            add_side(vertices, faces, uvs, corners, inside, (u_centre, middle))


def add_side(vertices, faces, uvs, corners, inside, uv_centre):
    """Appends one side of a sliver, three or four corners, as one or two triangles.

    The side faces away from the point `inside`. Its piece of the picture lies around
    `uv_centre`.
    """
    points = [Vector(vertices[corner]) for corner in corners]
    centre = sum(points, Vector()) / len(points)

    # The direction the side faces. The rings are twisted and jittered, so four corners
    # do not lie in one plane: this is the average over its edges (Newell's method),
    # which for a flat side is its normal.
    normal = Vector()
    for place, point in enumerate(points):
        after = points[(place + 1) % len(points)]
        normal += Vector(((point.y - after.y) * (point.z + after.z),
                          (point.z - after.z) * (point.x + after.x),
                          (point.x - after.x) * (point.y + after.y)))
    normal.normalize()
    if normal.dot(centre - inside) < 0.0:
        # The corners went clockwise as seen from outside. Turn them around.
        corners.reverse()
        points.reverse()
        normal = -normal

    # The side is projected onto its own plane, like common.face_project_uvs does: `up`
    # is the height as far as the side allows, `right` points to the right for someone
    # who looks at it from outside. Both triangles of a side use the same two directions,
    # so the picture continues across the line between them.
    up = Vector((0.0, 0.0, 1.0)) - normal * normal.z
    if up.length < 0.000001:
        up = Vector((0.0, 1.0, 0.0))
    up.normalize()
    right = up.cross(normal)
    side_uvs = [
        (uv_centre[0] + (point - centre).dot(right) * UV_UNITS_PER_METRE,
         uv_centre[1] + (point - centre).dot(up) * UV_UNITS_PER_METRE)
        for point in points
    ]

    # A fan around the first corner: one triangle for three corners, two for four. Each
    # triangle is flat, so each has one normal.
    for place in range(1, len(corners) - 1):
        faces.append((corners[0], corners[place], corners[place + 1]))
        uvs.append((side_uvs[0], side_uvs[place], side_uvs[place + 1]))


def finish(name, vertices, faces, uvs, shots):
    """Turns the lists into a textured model, exports it and renders the review shots."""
    model = common.create_mesh_object(name, vertices, faces)

    # The texture coordinates are written by hand, as in build_flask.py. A loop is one
    # corner of one face, and from_pydata keeps the order of both lists.
    uv_layer = model.data.uv_layers.new(name="uv")
    for polygon, face_uvs in zip(model.data.polygons, uvs):
        for loop_index, uv in zip(polygon.loop_indices, face_uvs):
            uv_layer.data[loop_index].uv = uv

    common.assign_textured_material(model, "splinter", "splinter.png", "splinter_normal.png")
    common.export_obj(name)

    if shots:
        # Two views aimed at the middle of the splinter: the rind, and the fracture.
        common.render_review_shots(
            name,
            target=(0.03, 0.0, 0.25),
            camera_positions=[(0.9, -0.5, 0.45), (-0.8, 0.6, 0.5)],
        )


def build_splinter_a(shots):
    common.reset_scene()

    vertices = []
    faces = []
    uvs = []

    # One sliver that stands on its lower point and leans a little.
    add_sliver(vertices, faces, uvs, base=(0.0, 0.0, 0.0), axis=(0.12, -0.05, 1.0),
               length=0.5, width=0.085, twist=28.0, seed=3)

    finish("splinter_a", vertices, faces, uvs, shots)


def build_splinter_b(shots):
    common.reset_scene()

    vertices = []
    faces = []
    uvs = []

    # Three slivers from one spot that lean away from each other. They overlap at their
    # lower points, which hides where one ends and the next one starts. The tallest one
    # stands almost upright and is the size of splinter_a.
    add_sliver(vertices, faces, uvs, base=(0.0, 0.0, 0.0), axis=(0.05, 0.02, 1.0),
               length=0.5, width=0.08, twist=24.0, seed=4)
    add_sliver(vertices, faces, uvs, base=(0.02, -0.01, 0.0), axis=(0.8, -0.35, 1.0),
               length=0.36, width=0.062, twist=-30.0, seed=5)
    add_sliver(vertices, faces, uvs, base=(-0.02, 0.02, 0.0), axis=(-0.62, 0.58, 1.0),
               length=0.28, width=0.05, twist=20.0, seed=6)

    finish("splinter_b", vertices, faces, uvs, shots)


def build(shots):
    build_splinter_a(shots)
    build_splinter_b(shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
