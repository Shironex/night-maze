# Builds the model gate_arch: the stone gatehouse that stands over the exit gate. Two piers
# over the two pillars, a stone head over the doorway, a gable, and on top a small cote
# with a roof, in which the big lantern hangs above the walls of the maze.
# Output: assets/models/gate_arch.obj and gate_arch.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate_arch.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "gate_arch"

# All sizes are in metres. The geometry is written in Blender space: X runs along the gate,
# Y is its thickness and Z is the height. After export Z becomes the game Y axis.
# The gatehouse stands like the models gate and wall_straight: along X, centred on the
# origin, with the origin on the floor in the middle of the base. It is the same seen from
# both sides and from both ends, so the game can draw it with the matrix of the gate
# whichever side the player comes from.
#
# The numbers the game has to agree with are in src/game/GateLamp.hpp (where the lanterns
# hang) and src/game/MazeLayout.hpp (GATE_HOUSE_HEIGHT, PILLAR_SIZE).

# The middle of each pier: on the grid corner at the end of the gate, where a pillar of
# the maze stands (wall_pillar: 0.3 m wide with a foot of 0.4 m, 3.15 m high).
PIER_X = 1.0

# The lower part of a pier, as high as the player can reach: 0.4 m wide, the width of the
# foot of the pillar it stands around. It must not be wider: the collision box of the
# pillar is 0.3 m, and a pier that reached further into the corridor would be stone the
# player walks into. It starts below the floor, because the ground under a pier can be
# lower than under the middle of the gate.
PIER_LOW_HALF = 0.20
PIER_LOW_BOTTOM = -0.4
# From here up a pier is wider. This is above the head of the player (1.8 m), so the wide
# part needs no collision box.
PIER_STEP_HEIGHT = 2.3
PIER_HIGH_HALF = 0.27
PIER_TOP = 3.5

# The cap of a pier: a slab that stands out, and a low pyramid on it.
CAP_HALF = 0.33
CAP_TOP = 3.65
CAP_POINT = 3.85

# The stone head over the doorway, between the piers. The wooden gate is 2.75 m high, so
# the head starts just above it.
HEAD_HALF_THICKNESS = 0.25
HEAD_BOTTOM = 2.85

# A haunch under each end of the head: a wedge that makes the doorway look arched. It
# starts at the inner face of the pier and ends this far from the middle of the gate.
HAUNCH_HALF_THICKNESS = 0.20
HAUNCH_BOTTOM = 2.35
HAUNCH_END_X = 0.25

# The keystone in the middle of the head: it stands out on both faces and hangs a little
# below the head.
KEYSTONE_HALF_WIDTH = 0.11
KEYSTONE_HALF_THICKNESS = 0.29
KEYSTONE_BOTTOM = 2.78
KEYSTONE_TOP = 3.3

# A course of stone that stands out under the gable.
COURSE_HALF_LENGTH = 1.3
COURSE_HALF_THICKNESS = 0.29
COURSE_BOTTOM = 3.4

# The gable: a low triangle over the whole width, with its ridge across the gate.
GABLE_HALF_LENGTH = 1.33
GABLE_HALF_THICKNESS = 0.3
GABLE_TOP = 4.2

# The corbels: four short stone arms on the piers, two on each face of the gatehouse. The
# two on the side of the approach carry the small bracket lanterns (the game hangs them
# 0.7 m from the middle and 0.42 m in front of the gate, GATE_BRACKET_LANTERN_ALONG and
# GATE_BRACKET_LANTERN_OUT). There the cap of a lantern (0.24 m wide) stays clear of the
# wall of the corridor, whose box starts 0.85 m from the middle. The other two are there because the model is the same from
# both sides.
CORBEL_X = 0.7
CORBEL_HALF_WIDTH = 0.06
CORBEL_REACH = 0.56
CORBEL_BOTTOM = 2.6
CORBEL_TOP = 2.74

# The cote on the gable: a sill, two posts, a beam the big lantern hangs from and a small
# roof. The big lantern is 0.4 m wide and its glass is centred 5.1 m up (the game draws
# the lantern model twice as large there).
SILL_HALF_LENGTH = 0.5
SILL_HALF_THICKNESS = 0.2
SILL_BOTTOM = 3.98
SILL_TOP = 4.12
POST_X = 0.36
POST_HALF = 0.07
POST_TOP = 5.58
BEAM_HALF_THICKNESS = 0.05
BEAM_BOTTOM = 5.5
ROOF_HALF_LENGTH = 0.58
ROOF_HALF_THICKNESS = 0.35
# The ridge of the roof: the highest point of the model, just under GATE_HOUSE_HEIGHT (6 m).
ROOF_TOP = 5.98


def add_roof(vertices, faces, half_length, half_thickness, bottom, top, with_base):
    """Appends a roof: a triangle seen from the front, with its ridge across the gate.

    The ridge runs along Y in the middle (x = 0) at the height `top`, and the two slopes
    fall to x = -half_length and x = +half_length at the height `bottom`. `with_base`
    adds the level underside, for a roof that can be seen from below.
    """
    first = len(vertices)
    vertices.extend(
        [
            (-half_length, -half_thickness, bottom),  # 0
            (half_length, -half_thickness, bottom),  # 1
            (half_length, half_thickness, bottom),  # 2
            (-half_length, half_thickness, bottom),  # 3
            (0.0, -half_thickness, top),  # 4
            (0.0, half_thickness, top),  # 5
        ]
    )
    # Counter clockwise as seen from outside.
    roof = [
        (0, 4, 5, 3),  # the slope facing -X
        (1, 2, 5, 4),  # the slope facing +X
        (0, 1, 4),  # the triangle facing -Y
        (2, 3, 5),  # the triangle facing +Y
    ]
    if with_base:
        roof.append((0, 3, 2, 1))
    for corners in roof:
        faces.append(tuple(first + corner for corner in corners))


def add_pyramid(vertices, faces, centre_x, half, bottom, top):
    """Appends the four slopes of a pyramid over a square at the height `bottom`."""
    first = len(vertices)
    vertices.extend(
        [
            (centre_x - half, -half, bottom),  # 0
            (centre_x + half, -half, bottom),  # 1
            (centre_x + half, half, bottom),  # 2
            (centre_x - half, half, bottom),  # 3
            (centre_x, 0.0, top),  # 4
        ]
    )
    for corners in ((0, 1, 4), (1, 2, 4), (2, 3, 4), (3, 0, 4)):
        faces.append(tuple(first + corner for corner in corners))


def add_haunch(vertices, faces, side):
    """Appends one haunch. side is -1 for the end at -X and +1 for the end at +X."""
    pier_x = side * (PIER_X - PIER_HIGH_HALF)
    end_x = side * HAUNCH_END_X
    first = len(vertices)
    for y in (-HAUNCH_HALF_THICKNESS, HAUNCH_HALF_THICKNESS):
        vertices.extend(
            [
                (pier_x, y, HAUNCH_BOTTOM),  # 0 and 3: at the pier, the low corner
                (pier_x, y, HEAD_BOTTOM),  # 1 and 4: at the pier, under the head
                (end_x, y, HEAD_BOTTOM),  # 2 and 5: under the head, towards the middle
            ]
        )
    # Written for the end at +X, counter clockwise as seen from outside. The faces
    # against the pier and against the head are hidden and left out.
    wedge = [
        (0, 2, 5, 3),  # the slanted underside
        (0, 1, 2),  # the triangle facing -Y
        (3, 5, 4),  # the triangle facing +Y
    ]
    for corners in wedge:
        if side < 0.0:
            # The other end is a mirror image, so the order turns around.
            corners = tuple(reversed(corners))
        faces.append(tuple(first + corner for corner in corners))


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    for side in (-1.0, 1.0):
        x = side * PIER_X
        # The lower part of the pier. Its underside is in the ground and its top is
        # covered by the upper part.
        common.add_box(
            vertices,
            faces,
            (x - PIER_LOW_HALF, -PIER_LOW_HALF, PIER_LOW_BOTTOM),
            (x + PIER_LOW_HALF, PIER_LOW_HALF, PIER_STEP_HEIGHT),
            skip=("-z", "+z"),
        )
        # The upper part. Its underside is the step that can be seen from below.
        common.add_box(
            vertices,
            faces,
            (x - PIER_HIGH_HALF, -PIER_HIGH_HALF, PIER_STEP_HEIGHT),
            (x + PIER_HIGH_HALF, PIER_HIGH_HALF, PIER_TOP),
            skip=("+z",),
        )
        # The cap and its point.
        common.add_box(
            vertices,
            faces,
            (x - CAP_HALF, -CAP_HALF, PIER_TOP),
            (x + CAP_HALF, CAP_HALF, CAP_TOP),
            skip=("+z",),
        )
        add_pyramid(vertices, faces, x, CAP_HALF, CAP_TOP, CAP_POINT)

        add_haunch(vertices, faces, side)

        # The two corbels of this pier, one on each face of the gatehouse.
        for face in (-1.0, 1.0):
            near = face * PIER_HIGH_HALF
            far = face * CORBEL_REACH
            common.add_box(
                vertices,
                faces,
                (side * CORBEL_X - CORBEL_HALF_WIDTH, min(near, far), CORBEL_BOTTOM),
                (side * CORBEL_X + CORBEL_HALF_WIDTH, max(near, far), CORBEL_TOP),
                # The end that touches the pier is hidden.
                skip=("-y",) if face > 0.0 else ("+y",),
            )

        # A post of the cote.
        common.add_box(
            vertices,
            faces,
            (side * POST_X - POST_HALF, -POST_HALF, SILL_TOP),
            (side * POST_X + POST_HALF, POST_HALF, POST_TOP),
            skip=("-z", "+z"),
        )

    # The head between the piers. Its ends are inside the piers and its top is under the
    # gable.
    head_half_length = PIER_X - PIER_HIGH_HALF
    common.add_box(
        vertices,
        faces,
        (-head_half_length, -HEAD_HALF_THICKNESS, HEAD_BOTTOM),
        (head_half_length, HEAD_HALF_THICKNESS, PIER_TOP),
        skip=("-x", "+x", "+z"),
    )
    common.add_box(
        vertices,
        faces,
        (-KEYSTONE_HALF_WIDTH, -KEYSTONE_HALF_THICKNESS, KEYSTONE_BOTTOM),
        (KEYSTONE_HALF_WIDTH, KEYSTONE_HALF_THICKNESS, KEYSTONE_TOP),
    )
    common.add_box(
        vertices,
        faces,
        (-COURSE_HALF_LENGTH, -COURSE_HALF_THICKNESS, COURSE_BOTTOM),
        (COURSE_HALF_LENGTH, COURSE_HALF_THICKNESS, PIER_TOP),
        skip=("-x", "+x", "+z"),
    )

    # The gable. Its underside lies on the head and is never seen.
    add_roof(vertices, faces, GABLE_HALF_LENGTH, GABLE_HALF_THICKNESS, PIER_TOP, GABLE_TOP, False)

    # The cote: the sill, the beam between the posts and the roof.
    common.add_box(
        vertices,
        faces,
        (-SILL_HALF_LENGTH, -SILL_HALF_THICKNESS, SILL_BOTTOM),
        (SILL_HALF_LENGTH, SILL_HALF_THICKNESS, SILL_TOP),
    )
    common.add_box(
        vertices,
        faces,
        (-POST_X, -BEAM_HALF_THICKNESS, BEAM_BOTTOM),
        (POST_X, BEAM_HALF_THICKNESS, POST_TOP),
        skip=("-x", "+x", "+z"),
    )
    add_roof(vertices, faces, ROOF_HALF_LENGTH, ROOF_HALF_THICKNESS, POST_TOP, ROOF_TOP, True)

    model = common.create_mesh_object(NAME, vertices, faces)
    # face_project_uvs and not box_project_uvs: the gable, the roof, the haunches and the
    # points of the caps are slanted, and a projection along an axis would stretch them.
    # The density is the one of the walls, so the stones have the same size.
    common.face_project_uvs(model.data, common.METRES_PER_UV_UNIT)
    common.assign_textured_material(model, "wall_stone", "wall_stone.png", "wall_stone_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views: the whole gatehouse from the front and a little to the side, and the
        # doorway from below, as the player sees it.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 3.0),
            camera_positions=[(4.0, -11.0, 3.2), (0.6, -3.2, 1.7)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
