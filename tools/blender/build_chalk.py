# Builds the three chalk marks that are drawn on the walls: chalk_lamp is a lamp with its
# rays (a note that tells a line of the story), chalk_arrow is an arrow (a note that is
# a hint, the game turns it towards what the hint names) and chalk_crook is a shepherd's
# crook (a mark beside every lever, which nobody can read or pick).
# Output: assets/models/chalk_lamp.obj, chalk_arrow.obj and chalk_crook.obj, each with
# its .mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_chalk.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

# All sizes are in metres. The geometry is written in Blender space: X is the width, Z is
# the height and Y is the depth. The exporter maps a Blender point (x, y, z) to the game
# point (x, z, -y), so the game +Z axis (out of the wall, towards the player) is the
# Blender -Y axis.
# The origin of a mark is a point of the wall face: the wall face is the game plane z = 0.
# Left and right are meant for someone who stands in front of the wall and looks at it:
# the right hand side is Blender +X, which is also game +X.

# A mark is a list of lines, and a line is a list of points (x, z) on the wall. Every
# piece between two points becomes one flat strip this wide. The strip is this much longer
# than the piece at both ends too (half of the width), so its ends are square and two
# pieces that meet in a corner close the corner.
STROKE_WIDTH = 0.018
# The strips lie this far in front of the wall. In the same plane as the stone the depth
# buffer could not decide which of the two is in front, and the mark would flicker
# (z-fighting). 3 mm is far more than the depth buffer needs at the far end of a corridor
# and too little to be seen from the side.
WALL_OFFSET = 0.003

# One repeat of chalk.png covers 0.25 m, so a strip shows about 36 pixels of grain across.
METRES_PER_UV_UNIT = 0.25

# The box the game tests the mouse ray against for a note: its width and its height,
# centred on the origin of the mark (NOTE_BOX_WIDTH and NOTE_BOX_HEIGHT in
# src/game/Interactables.hpp). They are only used by check_pick_box below and must stay in
# step with the game.
PICK_BOX_WIDTH = 0.4
PICK_BOX_HEIGHT = 0.5

# The game turns the arrow in the plane of the wall by one of these angles
# (game::chalkArrowDegrees in src/game/Interaction.hpp): to the right, up, to the left
# and down.
ARROW_TURNS = (0.0, 90.0, 180.0, 270.0)


def arrow_lines():
    """An arrow that points to the right: a shaft and a head of two strokes."""
    return [
        [(-0.17, 0.0), (0.17, 0.0)],
        [(0.07, 0.09), (0.17, 0.0), (0.07, -0.09)],
    ]


def lamp_lines():
    """A lamp: a square body, a roof, a ring to hang it by, a wick and four rays."""
    # The ring: a circle of eight pieces, 2.5 cm in radius, above the tip of the roof.
    ring = [
        (0.025 * math.cos(math.tau * k / 8), 0.165 + 0.025 * math.sin(math.tau * k / 8))
        for k in range(9)
    ]
    return [
        [(-0.06, -0.1), (0.06, -0.1), (0.06, 0.06), (-0.06, 0.06), (-0.06, -0.1)],
        [(-0.085, 0.06), (0.0, 0.13), (0.085, 0.06)],
        ring,
        [(-0.1, -0.02), (-0.16, -0.03)],
        [(0.1, -0.02), (0.16, -0.03)],
        [(-0.09, 0.03), (-0.14, 0.08)],
        [(0.09, 0.03), (0.14, 0.08)],
        [(0.0, -0.06), (0.0, 0.02)],
    ]


def crook_lines():
    """A shepherd's crook: a staff and a hook that bends over to the right."""
    points = [(0.0, -0.16), (0.0, 0.08)]
    # The hook: ten pieces around a centre 4.5 cm to the right of the top of the staff.
    # It starts at the staff (180 degrees) and runs over the top for 225 degrees.
    for k in range(1, 11):
        angle = math.pi - k * (math.pi * 1.25 / 10)
        points.append((0.045 + 0.045 * math.cos(angle), 0.08 + 0.045 * math.sin(angle)))
    return [points]


def stroke_corners(lines):
    """Returns the strips of a mark: a list of four corners (x, z) for each piece.

    The corners are in counter clockwise order for someone in front of the wall. That
    order makes the face point out of the wall. The game does not draw the back of
    a face, so a strip the other way round would not be seen at all.
    """
    half = STROKE_WIDTH / 2.0
    strips = []
    for line in lines:
        for (ax, az), (bx, bz) in zip(line, line[1:]):
            length = math.hypot(bx - ax, bz - az)
            # The direction of the piece (along) and the direction to its left (side),
            # both half a width long.
            along_x = (bx - ax) / length * half
            along_z = (bz - az) / length * half
            side_x = -along_z
            side_z = along_x
            strips.append(
                [
                    (ax - along_x - side_x, az - along_z - side_z),
                    (bx + along_x - side_x, bz + along_z - side_z),
                    (bx + along_x + side_x, bz + along_z + side_z),
                    (ax - along_x + side_x, az - along_z + side_z),
                ]
            )
    return strips


def check_pick_box():
    """Stops the script if a mark that can be read would stick out of its pick box.

    A part outside of the pick box could be seen but not clicked. The lamp is tested as
    it is, the arrow in each of the four directions the game turns it to. The crook is
    not tested: it cannot be picked.
    """
    marks = [stroke_corners(lamp_lines())]
    for degrees in ARROW_TURNS:
        turn = math.radians(degrees)
        marks.append(
            [
                [
                    (
                        x * math.cos(turn) - z * math.sin(turn),
                        x * math.sin(turn) + z * math.cos(turn),
                    )
                    for x, z in strip
                ]
                for strip in stroke_corners(arrow_lines())
            ]
        )
    for strips in marks:
        for strip in strips:
            for x, z in strip:
                if abs(x) > PICK_BOX_WIDTH / 2 or abs(z) > PICK_BOX_HEIGHT / 2:
                    raise ValueError(
                        "A chalk mark does not fit into the pick box of a note "
                        "(src/game/Interactables.hpp)"
                    )


def build_mark(name, lines, shots):
    common.reset_scene()

    vertices = []
    faces = []
    for strip in stroke_corners(lines):
        first = len(vertices)
        vertices.extend((x, -WALL_OFFSET, z) for x, z in strip)
        faces.append((first, first + 1, first + 2, first + 3))

    model = common.create_mesh_object(name, vertices, faces)
    # Every strip must face out of the wall (Blender -Y), see stroke_corners.
    for polygon in model.data.polygons:
        if polygon.normal.y > -0.99:
            raise ValueError("A strip of " + name + " does not face out of the wall")

    # The projection uses the place on the wall, so where two strips overlap in a corner
    # both show the same pixels and it does not matter which of them is drawn.
    common.face_project_uvs(model.data, METRES_PER_UV_UNIT)
    common.assign_textured_material(model, "chalk", "chalk.png", "chalk_normal.png")
    common.export_obj(name)

    if shots:
        # Two views from in front of the wall (the front is Blender -Y): straight at the
        # mark, and from the right side, nearly along the wall.
        common.render_review_shots(
            name,
            target=(0.0, 0.0, 0.0),
            camera_positions=[(0.0, -0.9, 0.0), (0.8, -0.25, 0.1)],
        )


def build(shots):
    check_pick_box()
    build_mark("chalk_lamp", lamp_lines(), shots)
    build_mark("chalk_arrow", arrow_lines(), shots)
    build_mark("chalk_crook", crook_lines(), shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
