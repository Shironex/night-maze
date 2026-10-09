# Builds the model gate_bell: the bell that hangs in the double cote of the gatehouse
# (build_gate_arch.py), beside the big lantern. A yoke, the bell itself, and a clapper in
# it. The game swings it each time the bell of the open gate tolls.
# Output: assets/models/gate_bell.obj and gate_bell.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate_bell.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "gate_bell"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The origin is the pivot, the point the bell
# turns around: the top of the yoke. Everything hangs below it, so every height here is
# negative. The game puts the pivot 5.47 m up, in the middle of the opening at -X, and
# turns the model around the axis across the gate (Blender Y), so the bell swings along
# the gate (GATE_BELL_ALONG, GATE_BELL_HEIGHT and GATE_BELL_SWING_DEGREES in
# src/game/GateLamp.hpp). The model is the same seen from the front and from the back.

# The yoke: the block the bell hangs from, along the gate.
YOKE_HALF_LENGTH = 0.13
YOKE_HALF_THICKNESS = 0.035
YOKE_BOTTOM = -0.06

# The bell: an outline that is turned around the Z axis in BELL_SIDES steps. Each point is
# (distance from the axis, height). The outline starts on the axis under the yoke, runs
# down the outside over the shoulder and the waist to the lip, turns around the lip and
# climbs back up the inside to the axis. So the bell has a wall with two sides: the faces
# of the inside point inwards, and somebody who looks up into the bell sees them. The
# renderer draws no back faces, so without them the bell would be a hole from below.
BELL_SIDES = 12
BELL_OUTLINE = [
    (0.0, -0.06),
    (0.055, -0.06),
    (0.085, -0.08),
    (0.1, -0.13),
    (0.108, -0.2),
    (0.135, -0.29),
    (0.172, -0.35),
    (0.168, -0.365),
    (0.15, -0.36),
    (0.122, -0.31),
    (0.094, -0.22),
    (0.075, -0.12),
    (0.0, -0.1),
]

# The clapper: a thin rod from the top of the inside, and a block at its end that hangs
# a little below the lip. Its underside, 0.37 m under the pivot, is the lowest point.
ROD_HALF = 0.008
ROD_TOP = -0.1
ROD_BOTTOM = -0.32
CLAPPER_HALF = 0.028
CLAPPER_TOP = -0.31
CLAPPER_BOTTOM = -0.37

# One repeat of the picture covers 0.5 m, as on the lever, whose brass the bell wears.
METRES_PER_UV_UNIT = 0.5


def face_normal(points):
    """The direction a face looks in, for corners listed counter clockwise (Newell)."""
    normal = [0.0, 0.0, 0.0]
    for index, (x0, y0, z0) in enumerate(points):
        x1, y1, z1 = points[(index + 1) % len(points)]
        normal[0] += (y0 - y1) * (z0 + z1)
        normal[1] += (z0 - z1) * (x0 + x1)
        normal[2] += (x0 - x1) * (y0 + y1)
    return normal


def add_lathe(vertices, faces, outline, sides):
    """Appends the surface that `outline` sweeps when it is turned around the Z axis.

    `outline` is a list of (radius, height). A point on the axis (radius 0) is one
    vertex, and the faces that touch it are triangles. Every face looks to the left of
    the outline as it is walked: for an outline that runs down the outside and back up
    the inside, that is away from the wall of the bell on both of its sides.
    """
    rings = []
    for radius, height in outline:
        if radius < 0.000001:
            vertices.append((0.0, 0.0, height))
            rings.append([len(vertices) - 1] * sides)
            continue
        ring = []
        for step in range(sides):
            angle = step / sides * 2.0 * math.pi
            vertices.append((radius * math.cos(angle), radius * math.sin(angle), height))
            ring.append(len(vertices) - 1)
        rings.append(ring)

    for index in range(len(outline) - 1):
        radius_step = outline[index + 1][0] - outline[index][0]
        height_step = outline[index + 1][1] - outline[index][1]
        for step in range(sides):
            following = (step + 1) % sides
            corners = []
            for corner in (
                rings[index][step],
                rings[index][following],
                rings[index + 1][following],
                rings[index + 1][step],
            ):
                # On the axis two corners are the same vertex: it is listed once.
                if corner not in corners:
                    corners.append(corner)
            # The direction the face has to look in: the outline turned a quarter turn
            # to the left, in the middle of this step around the axis.
            angle = (step + 0.5) / sides * 2.0 * math.pi
            out = (-height_step * math.cos(angle), -height_step * math.sin(angle), radius_step)
            normal = face_normal([vertices[corner] for corner in corners])
            if sum(a * b for a, b in zip(normal, out)) < 0.0:
                corners.reverse()
            faces.append(tuple(corners))


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # The yoke, all six sides: the bell under it is narrower than it is long.
    common.add_box(
        vertices,
        faces,
        (-YOKE_HALF_LENGTH, -YOKE_HALF_THICKNESS, YOKE_BOTTOM),
        (YOKE_HALF_LENGTH, YOKE_HALF_THICKNESS, 0.0),
    )

    add_lathe(vertices, faces, BELL_OUTLINE, BELL_SIDES)

    # The clapper. The top of the rod is against the inside of the bell and its lower
    # end is inside the block.
    common.add_box(
        vertices,
        faces,
        (-ROD_HALF, -ROD_HALF, ROD_BOTTOM),
        (ROD_HALF, ROD_HALF, ROD_TOP),
        skip=("-z", "+z"),
    )
    common.add_box(
        vertices,
        faces,
        (-CLAPPER_HALF, -CLAPPER_HALF, CLAPPER_BOTTOM),
        (CLAPPER_HALF, CLAPPER_HALF, CLAPPER_TOP),
    )

    model = common.create_mesh_object(NAME, vertices, faces)
    # Every face is projected onto its own plane: the wall of the bell is slanted
    # everywhere.
    common.face_project_uvs(model.data, METRES_PER_UV_UNIT)
    # No picture of its own: the brass of the lever handle.
    common.assign_textured_material(
        model, "lever_brass", "lever_brass.png", "lever_brass_normal.png"
    )
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the bell: from the front and a little above,
        # and from below, into the bell, as the player sees it from the gate.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, -0.2),
            camera_positions=[(0.5, -0.9, 0.0), (0.25, -0.5, -0.75)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
