# Builds the model gate_lantern: a small lantern of dark iron with panes of pale glass. The
# game draws it three times at the exit gate: twice as it is, hanging on the piers, and
# once twice as large in the cote on top of the gatehouse.
# Output: assets/models/gate_lantern.obj and gate_lantern.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate_lantern.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

from mathutils import Vector

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "gate_lantern"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The lantern is upright, 0.2 m wide and 0.36 m
# tall, with its origin in the middle of its base. It is the same seen from all four
# sides, so the game never has to turn it.

# The base plate the glass stands on.
BASE_HALF = 0.10
BASE_TOP = 0.03

# The glass: a box of four panes. The middle of the glass is 0.13 m above the base, which
# is the number the game uses to hang the big lantern at the right height
# (GATE_COTE_LANTERN_HEIGHT in src/game/GateLamp.hpp).
GLASS_HALF = 0.08
GLASS_TOP = 0.23

# The four iron posts at the corners of the glass, and the band around its middle.
POST_HALF = 0.012
BAND_BOTTOM = 0.12
BAND_TOP = 0.14
BAND_DEPTH = 0.008

# The cap: a plate that is wider than the base, a low pyramid on it and the block of the
# ring the lantern hangs from.
CAP_HALF = 0.12
CAP_PLATE_TOP = 0.25
CAP_POINT_HALF = 0.11
CAP_POINT = 0.33
RING_HALF = 0.015
RING_BOTTOM = 0.31
RING_TOP = 0.36

# The picture lantern.png has two halves (make_textures.py): the left half is the pale
# glass and the right half the dark iron. Every face gets a small piece from the middle of
# its half. That is all the game needs to light the lantern with one number: the glow of
# a surface is its colour times the glow value, so the pale glass shines and the dark iron
# stays dark.
GLASS_UV_CENTRE = (0.25, 0.5)
IRON_UV_CENTRE = (0.75, 0.5)
# One metre of the model is this many units of u and v. The largest face is 0.24 m wide,
# so it covers a quarter of the picture and stays inside its half.
UV_UNITS_PER_METRE = 1.0


def add_part(vertices, faces, centres, low, high, centre, skip=()):
    """Appends a box (common.add_box) and, for each of its faces, the middle of its piece
    of the picture: GLASS_UV_CENTRE or IRON_UV_CENTRE."""
    before = len(faces)
    common.add_box(vertices, faces, low, high, skip=skip)
    centres.extend([centre] * (len(faces) - before))


def project_uvs(mesh, centres):
    """Gives every face its piece of the picture, around the middle listed for it.

    The face is projected onto its own plane, like common.face_project_uvs does: `up` is
    the height as far as the face allows and `right` points to the right for someone who
    looks at the face from outside. The corners are measured from the middle of the face,
    so the piece lies around the middle of the half of the picture the face belongs to.
    """
    uv_layer = mesh.uv_layers.new(name="uv")

    # from_pydata keeps the order of the faces, so face number i has centres[i].
    for polygon, centre in zip(mesh.polygons, centres):
        normal = polygon.normal
        up = Vector((0.0, 0.0, 1.0)) - normal * normal.z
        if up.length < 0.000001:
            # A level face has no height direction. Blender Y is used instead.
            up = Vector((0.0, 1.0, 0.0))
        up.normalize()
        right = up.cross(normal)

        for loop_index in polygon.loop_indices:
            position = mesh.vertices[mesh.loops[loop_index].vertex_index].co - polygon.center
            u = centre[0] + position.dot(right) * UV_UNITS_PER_METRE
            v = centre[1] + position.dot(up) * UV_UNITS_PER_METRE
            uv_layer.data[loop_index].uv = (u, v)


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []
    # For every face, in the order of `faces`: the middle of its piece of the picture.
    centres = []

    # The base plate. All six sides: the underside is what the player sees from below.
    add_part(
        vertices, faces, centres, (-BASE_HALF, -BASE_HALF, 0.0), (BASE_HALF, BASE_HALF, BASE_TOP),
        IRON_UV_CENTRE,
    )

    # The glass. Its top and its underside are covered by the cap and the base.
    add_part(
        vertices, faces, centres, (-GLASS_HALF, -GLASS_HALF, BASE_TOP),
        (GLASS_HALF, GLASS_HALF, GLASS_TOP), GLASS_UV_CENTRE, skip=("-z", "+z"),
    )

    # The four corner posts.
    for x in (-GLASS_HALF, GLASS_HALF):
        for y in (-GLASS_HALF, GLASS_HALF):
            add_part(
                vertices, faces, centres, (x - POST_HALF, y - POST_HALF, BASE_TOP),
                (x + POST_HALF, y + POST_HALF, GLASS_TOP), IRON_UV_CENTRE, skip=("-z", "+z"),
            )

    # The band around the middle of the glass: one thin bar on each pane, between the
    # posts. The side of a bar that lies on the glass is hidden, and so are its two ends.
    reach = GLASS_HALF - POST_HALF
    outside = GLASS_HALF + BAND_DEPTH
    add_part(
        vertices, faces, centres, (-reach, -outside, BAND_BOTTOM), (reach, -GLASS_HALF, BAND_TOP),
        IRON_UV_CENTRE, skip=("-x", "+x", "+y"),
    )
    add_part(
        vertices, faces, centres, (-reach, GLASS_HALF, BAND_BOTTOM), (reach, outside, BAND_TOP),
        IRON_UV_CENTRE, skip=("-x", "+x", "-y"),
    )
    add_part(
        vertices, faces, centres, (-outside, -reach, BAND_BOTTOM), (-GLASS_HALF, reach, BAND_TOP),
        IRON_UV_CENTRE, skip=("-y", "+y", "+x"),
    )
    add_part(
        vertices, faces, centres, (GLASS_HALF, -reach, BAND_BOTTOM), (outside, reach, BAND_TOP),
        IRON_UV_CENTRE, skip=("-y", "+y", "-x"),
    )

    # The plate of the cap, all six sides: it is wider than everything under and over it.
    add_part(
        vertices, faces, centres, (-CAP_HALF, -CAP_HALF, GLASS_TOP),
        (CAP_HALF, CAP_HALF, CAP_PLATE_TOP), IRON_UV_CENTRE,
    )

    # The pyramid of the cap: four slopes that meet in a point.
    first = len(vertices)
    vertices.extend(
        [
            (-CAP_POINT_HALF, -CAP_POINT_HALF, CAP_PLATE_TOP),  # 0
            (CAP_POINT_HALF, -CAP_POINT_HALF, CAP_PLATE_TOP),  # 1
            (CAP_POINT_HALF, CAP_POINT_HALF, CAP_PLATE_TOP),  # 2
            (-CAP_POINT_HALF, CAP_POINT_HALF, CAP_PLATE_TOP),  # 3
            (0.0, 0.0, CAP_POINT),  # 4
        ]
    )
    # Counter clockwise as seen from outside.
    for corners in ((0, 1, 4), (1, 2, 4), (2, 3, 4), (3, 0, 4)):
        faces.append(tuple(first + corner for corner in corners))
        centres.append(IRON_UV_CENTRE)

    # The block of the ring on the point. Its underside is inside the pyramid.
    add_part(
        vertices, faces, centres, (-RING_HALF, -RING_HALF, RING_BOTTOM),
        (RING_HALF, RING_HALF, RING_TOP), IRON_UV_CENTRE, skip=("-z",),
    )

    model = common.create_mesh_object(NAME, vertices, faces)
    project_uvs(model.data, centres)
    common.assign_textured_material(model, "lantern", "lantern.png", "lantern_normal.png")
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
