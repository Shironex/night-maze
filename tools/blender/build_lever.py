# Builds the two models of the lever that hangs on a wall: lever is the dark iron plate
# that is fixed to the wall, lever_handle is the brass handle that the game turns up and
# down.
# Output: assets/models/lever.obj, lever.mtl, lever_handle.obj and lever_handle.mtl.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_lever.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

# The lever is two models and not one, because the game draws the handle with a model
# matrix of its own: the plate never moves, and the handle turns around its pivot.
# The two models also have two materials: the plate is dark iron and the handle is light
# brass, so the handle can be told apart from the plate, and both from the stone wall.

# All sizes are in metres. The geometry is written in Blender space: X is the width, Z is
# the height and Y is the depth. The exporter maps a Blender point (x, y, z) to the game
# point (x, z, -y), so the game +Z axis (out of the wall, towards the player) is the
# Blender -Y axis. That is why every depth below is written with a minus sign in front of
# the Blender Y coordinate.

# One repeat of a lever texture covers 0.5 m. With the 2 m of the walls the 0.30 m plate
# would only show a small blurred corner of the picture.
METRES_PER_UV_UNIT = 0.5

# The plate (model lever). Its origin is the middle of its back, the point that is fixed
# to the face of the wall: the back lies in the game plane z = 0, and the plate reaches
# equally far to the left and the right, and up and down.
# The back plate is 0.16 m wide, 0.30 m high and 0.02 m thick.
PLATE_HALF_WIDTH = 0.08
PLATE_HALF_HEIGHT = 0.15
PLATE_THICKNESS = 0.02
# A raised frame on the front of the plate: four bars along its four edges. Each bar is
# 0.02 m wide, and its front is 0.04 m away from the wall, so it stands 0.02 m out of the
# plate. Without it the plate is one flat rectangle when seen from the front.
FRAME_BAR_WIDTH = 0.02
FRAME_FRONT = 0.04
# The housing is a small block in the middle of the plate that holds the pivot. It is
# 0.06 m wide and 0.08 m high, and its front is 0.07 m away from the wall.
HOUSING_HALF_WIDTH = 0.03
HOUSING_HALF_HEIGHT = 0.04
HOUSING_FRONT = 0.07

# The handle (model lever_handle). Its origin is the pivot, the point it turns around. In
# the model it points straight out of the wall, along the game +Z axis.
# The rod is 0.03 m by 0.03 m thick and 0.14 m long, measured from the pivot.
ROD_HALF_THICKNESS = 0.015
ROD_LENGTH = 0.14
# The knob at the end of the rod is a box 0.07 m wide and 0.07 m high. It starts 0.13 m
# from the pivot (so the last 0.01 m of the rod is inside it) and ends 0.20 m from it.
KNOB_HALF_SIZE = 0.035
KNOB_START = 0.13
KNOB_END = 0.20

# How the game puts the two models together. These numbers are not used for the geometry,
# only for the check in check_pick_box below. They must stay in step with the constants of
# the game: when one side changes, the other has to change too.
# The game places the pivot this far in front of the wall face, in the middle of the plate
# (LEVER_PIVOT_DEPTH in src/game/Interaction.hpp). That is inside the housing, which
# reaches from 0.02 m to 0.07 m.
PIVOT_DEPTH = 0.05
# The game turns the handle by this angle up or down around the game X axis
# (LEVER_HANDLE_UP_DEGREES and LEVER_HANDLE_DOWN_DEGREES in src/game/Interaction.hpp).
# The angle is this steep so that, seen from the front, the rod clearly runs up or down
# from the middle of the plate and the knob stands beyond the edge of the plate: turned
# by 55 degrees the knob reaches from about 0.09 m to about 0.18 m above the pivot, and
# the plate ends at 0.15 m.
HANDLE_SWING_DEGREES = 55.0
# The box the game tests the mouse ray against when the player clicks on the lever: its
# width, its height (both centred on the origin of the plate) and how far it reaches out
# from the wall (LEVER_BOX_WIDTH, LEVER_BOX_HEIGHT and LEVER_BOX_DEPTH in
# src/game/Interactables.hpp).
PICK_BOX_WIDTH = 0.3
PICK_BOX_HEIGHT = 0.4
PICK_BOX_DEPTH = 0.25
# The knob may end exactly on the front of the pick box (0.05 m + 0.20 m = 0.25 m). A
# computer stores such numbers with a tiny error, so the comparison allows a thousandth
# of a millimetre more.
PICK_BOX_TOLERANCE = 0.000001


def check_pick_box():
    """Stops the script if a part of the lever would stick out of its pick box.

    A part outside of the pick box could be seen but not clicked. The plate is tested as
    it is. The handle is tested where it reaches furthest: turned by the full swing.
    """
    # The corner of the knob that is furthest from the pivot lies KNOB_END out and
    # KNOB_HALF_SIZE up. Turning a point (out, up) by an angle gives
    # (out * cos - up * sin, out * sin + up * cos). The largest reach to the front comes
    # from the lower corner (up is negative, so the sine part adds), the largest reach
    # upwards from the upper corner.
    swing = math.radians(HANDLE_SWING_DEGREES)
    reach_out = PIVOT_DEPTH + KNOB_END * math.cos(swing) + KNOB_HALF_SIZE * math.sin(swing)
    reach_up = KNOB_END * math.sin(swing) + KNOB_HALF_SIZE * math.cos(swing)
    # Without any turn the knob points straight out, which is its largest depth of all.
    reach_out = max(reach_out, PIVOT_DEPTH + KNOB_END)

    # With the sizes above: 0.25 m out of 0.25 m to the front (the knob pointing straight
    # out), and 0.184 m out of 0.2 m up (the knob turned by 55 degrees).
    fits = (
        PLATE_HALF_WIDTH <= PICK_BOX_WIDTH / 2
        and PLATE_HALF_HEIGHT <= PICK_BOX_HEIGHT / 2
        and KNOB_HALF_SIZE <= PICK_BOX_WIDTH / 2
        and reach_up <= PICK_BOX_HEIGHT / 2 + PICK_BOX_TOLERANCE
        and reach_out <= PICK_BOX_DEPTH + PICK_BOX_TOLERANCE
    )
    if not fits:
        raise ValueError("The lever does not fit into its pick box (src/game/Interactables.hpp)")


def finish(name, vertices, faces, material_name, shots):
    """Turns the lists into a textured model, exports it and renders the review shots.

    `material_name` is also the name of the two pictures: <material_name>.png and
    <material_name>_normal.png in assets/textures.
    """
    model = common.create_mesh_object(name, vertices, faces)
    # Every face is projected onto its own plane, with the small texel density of this
    # file. box_project_uvs would do the same for these boxes, but only with the 2 m of
    # the walls.
    common.face_project_uvs(model.data, METRES_PER_UV_UNIT)
    common.assign_textured_material(
        model, material_name, material_name + ".png", material_name + "_normal.png"
    )
    common.export_obj(name)

    if shots:
        # Two views from in front of the wall (the front is Blender -Y): from the right
        # and a little above, and from the left and a little below.
        common.render_review_shots(
            name,
            target=(0.0, -0.06, 0.0),
            camera_positions=[(0.45, -0.75, 0.3), (-0.5, -0.5, -0.25)],
        )


def build_plate(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # The back plate: from the wall (Blender y = 0) out to PLATE_THICKNESS. Its back, the
    # side facing Blender +Y, lies on the wall and is never seen.
    common.add_box(
        vertices,
        faces,
        (-PLATE_HALF_WIDTH, -PLATE_THICKNESS, -PLATE_HALF_HEIGHT),
        (PLATE_HALF_WIDTH, 0.0, PLATE_HALF_HEIGHT),
        skip=("+y",),
    )

    # The frame. The top bar and the bottom bar run over the full width of the plate.
    # Each stands on the front of the plate, so its own back is hidden. -1 is the bottom
    # bar, +1 the top bar: the bar reaches from the edge of the plate FRAME_BAR_WIDTH
    # towards the middle.
    frame_inner_height = PLATE_HALF_HEIGHT - FRAME_BAR_WIDTH
    for side in (-1.0, 1.0):
        # min and max put the lower height first, whichever sign `side` has.
        low_z = min(side * frame_inner_height, side * PLATE_HALF_HEIGHT)
        high_z = max(side * frame_inner_height, side * PLATE_HALF_HEIGHT)
        common.add_box(
            vertices,
            faces,
            (-PLATE_HALF_WIDTH, -FRAME_FRONT, low_z),
            (PLATE_HALF_WIDTH, -PLATE_THICKNESS, high_z),
            skip=("+y",),
        )

    # The two side bars fill the space between the top bar and the bottom bar. Their back
    # is hidden on the plate, and their two ends are hidden against the other two bars.
    # -1 is the left bar, +1 the right bar.
    frame_inner_width = PLATE_HALF_WIDTH - FRAME_BAR_WIDTH
    for side in (-1.0, 1.0):
        low_x = min(side * frame_inner_width, side * PLATE_HALF_WIDTH)
        high_x = max(side * frame_inner_width, side * PLATE_HALF_WIDTH)
        common.add_box(
            vertices,
            faces,
            (low_x, -FRAME_FRONT, -frame_inner_height),
            (high_x, -PLATE_THICKNESS, frame_inner_height),
            skip=("+y", "-z", "+z"),
        )

    # The housing: it stands on the front of the plate, so its own back is hidden. The
    # parts of the plate front that the housing and the bars cover stay in the model:
    # add_box can only leave out a whole side, not cut a hole into one.
    common.add_box(
        vertices,
        faces,
        (-HOUSING_HALF_WIDTH, -HOUSING_FRONT, -HOUSING_HALF_HEIGHT),
        (HOUSING_HALF_WIDTH, -PLATE_THICKNESS, HOUSING_HALF_HEIGHT),
        skip=("+y",),
    )

    finish("lever", vertices, faces, "lever_iron", shots)


def build_handle(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # The rod: from the pivot (the origin) straight out. Both of its ends are hidden. The
    # outer end is inside the knob. The end at the pivot is inside the housing of the
    # plate, also when the handle is turned: the pivot is 0.02 m behind the front of the
    # housing, and at 55 degrees the corners of that end move only about 0.012 m towards
    # the front.
    common.add_box(
        vertices,
        faces,
        (-ROD_HALF_THICKNESS, -ROD_LENGTH, -ROD_HALF_THICKNESS),
        (ROD_HALF_THICKNESS, 0.0, ROD_HALF_THICKNESS),
        skip=("-y", "+y"),
    )

    # The knob. All six sides stay: the rod is thinner than the knob, so most of the side
    # facing the wall can be seen around it.
    common.add_box(
        vertices,
        faces,
        (-KNOB_HALF_SIZE, -KNOB_END, -KNOB_HALF_SIZE),
        (KNOB_HALF_SIZE, -KNOB_START, KNOB_HALF_SIZE),
    )

    finish("lever_handle", vertices, faces, "lever_brass", shots)


def build(shots):
    check_pick_box()
    build_plate(shots)
    build_handle(shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
