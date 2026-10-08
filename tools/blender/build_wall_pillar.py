# Builds the model wall_pillar: a square post for the grid corners of the maze, where wall
# segments meet. It hides the ends of the segments.
# Output: assets/models/wall_pillar.obj and wall_pillar.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_wall_pillar.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "wall_pillar"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis.

# The pillar is 0.15 m taller than the wall (3.0 m), so its top is not in the same plane as
# the top of the wall. Two faces in one plane flicker, because the depth test cannot decide
# which of them is in front.
HEIGHT = 3.15

# The shaft is 0.3 x 0.3 m: wider than the thickest part of the wall (0.28 m), so the end
# of a wall segment disappears inside it.
SHAFT_HALF_WIDTH = 0.15
# The base and the cap are 0.4 x 0.4 m.
TRIM_HALF_WIDTH = 0.2

# The base ends above the plinth of the wall (0.25 m) and the cap starts above the
# underside of the coping (2.85 m) and below the top of the wall (3.0 m). None of the
# horizontal faces of the pillar shares a plane with a horizontal face of the wall.
BASE_HEIGHT = 0.35
CAP_HEIGHT = 0.25


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    cap_bottom = HEIGHT - CAP_HEIGHT

    # Base. Its underside lies on the floor and is never seen.
    common.add_box(
        vertices,
        faces,
        (-TRIM_HALF_WIDTH, -TRIM_HALF_WIDTH, 0.0),
        (TRIM_HALF_WIDTH, TRIM_HALF_WIDTH, BASE_HEIGHT),
        skip=("-z",),
    )
    # Shaft. Its underside is covered by the base and its top by the cap.
    common.add_box(
        vertices,
        faces,
        (-SHAFT_HALF_WIDTH, -SHAFT_HALF_WIDTH, BASE_HEIGHT),
        (SHAFT_HALF_WIDTH, SHAFT_HALF_WIDTH, cap_bottom),
        skip=("-z", "+z"),
    )
    # Cap. All six sides can be seen: the underside shows where it overhangs the shaft.
    common.add_box(
        vertices,
        faces,
        (-TRIM_HALF_WIDTH, -TRIM_HALF_WIDTH, cap_bottom),
        (TRIM_HALF_WIDTH, TRIM_HALF_WIDTH, HEIGHT),
    )

    model = common.create_mesh_object(NAME, vertices, faces)
    common.box_project_uvs(model.data)
    common.assign_textured_material(
        model, "wall_stone", "wall_stone.png", "wall_stone_normal.png"
    )
    common.export_obj(NAME)

    if shots:
        # Two views from a corner direction, so two sides are visible: the whole pillar,
        # and the cap from above.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 1.6),
            camera_positions=[(4.0, -5.4, 2.2), (3.0, -4.0, 5.5)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
