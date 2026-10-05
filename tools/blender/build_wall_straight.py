# Builds the model wall_straight: one wall segment of the maze, 2 m long and 3 m high.
# Output: assets/models/wall_straight.obj and wall_straight.mtl.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_wall_straight.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "wall_straight"

# All sizes are in metres. The geometry is written in Blender space: X is the length of the
# wall, Y is its thickness and Z is the height. After export Z becomes the game Y axis.

# The segment fills one side of a 2 m maze cell, centred on the origin.
HALF_LENGTH = 1.0
# Total height. The origin is on the floor, in the middle of the base.
HEIGHT = 3.0

# The body is the thin middle part. The plinth at the bottom and the coping at the top are
# thicker, so they stand out by 0.04 m on both sides.
BODY_HALF_THICKNESS = 0.1
TRIM_HALF_THICKNESS = 0.14
PLINTH_HEIGHT = 0.25
COPING_HEIGHT = 0.15


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # Heights at which one part ends and the next one starts.
    plinth_top = PLINTH_HEIGHT
    coping_bottom = HEIGHT - COPING_HEIGHT

    # All three boxes have the same length, so both ends of the wall (x = -1 and x = +1)
    # are flat and the segment can butt against a pillar.

    # Plinth. Its underside lies on the floor and is never seen.
    common.add_box(
        vertices,
        faces,
        (-HALF_LENGTH, -TRIM_HALF_THICKNESS, 0.0),
        (HALF_LENGTH, TRIM_HALF_THICKNESS, plinth_top),
        skip=("-z",),
    )
    # Body. Its underside is covered by the plinth and its top by the coping.
    common.add_box(
        vertices,
        faces,
        (-HALF_LENGTH, -BODY_HALF_THICKNESS, plinth_top),
        (HALF_LENGTH, BODY_HALF_THICKNESS, coping_bottom),
        skip=("-z", "+z"),
    )
    # Coping. All six sides can be seen: the underside shows where it overhangs the body.
    common.add_box(
        vertices,
        faces,
        (-HALF_LENGTH, -TRIM_HALF_THICKNESS, coping_bottom),
        (HALF_LENGTH, TRIM_HALF_THICKNESS, HEIGHT),
    )

    model = common.create_mesh_object(NAME, vertices, faces)
    common.box_project_uvs(model.data)
    common.assign_textured_material(model, "wall_stone", "wall_stone.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the wall: from the front and a little to the
        # side, and from close to the floor near one end.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 1.5),
            camera_positions=[(3.0, -6.5, 2.4), (-3.2, -3.4, 0.5)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
