# Builds the model milestone: a knee high stone that stands against the wall of the last
# cell before the exit gate, where the lamplighters turned in.
# Output: assets/models/milestone.obj and milestone.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_milestone.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "milestone"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The stone is upright, with its origin on the
# ground in the middle of its base. It has no collision box in the game: it stands so
# close to the wall that the body of the player cannot reach its middle.

# The stone and its foot reach this far below the ground. The game puts the origin on the
# ground at the middle of the stone, and on a slope one side of it is lower than that.
BOTTOM = -0.15

# The foot: a low block that is wider than the stone.
FOOT_HALF = 0.16
FOOT_TOP = 0.08

# The body of the stone: 0.25 m wide and half a metre high, about as high as a knee.
BODY_HALF = 0.125
BODY_TOP = 0.5

# The worn top: the four sides lean in up to a small level square.
TOP_HALF = 0.07
TOP = 0.62


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # The foot. Its underside is in the ground.
    common.add_box(
        vertices, faces, (-FOOT_HALF, -FOOT_HALF, BOTTOM), (FOOT_HALF, FOOT_HALF, FOOT_TOP),
        skip=("-z",),
    )
    # The body. Its underside is in the ground and its top is under the worn top.
    common.add_box(
        vertices, faces, (-BODY_HALF, -BODY_HALF, BOTTOM), (BODY_HALF, BODY_HALF, BODY_TOP),
        skip=("-z", "+z"),
    )

    # The worn top: a ring of four corners at the top of the body, a smaller ring above
    # it, the four slanted sides between them and the level square that closes it.
    first = len(vertices)
    for half, height in ((BODY_HALF, BODY_TOP), (TOP_HALF, TOP)):
        vertices.extend(
            [
                (-half, -half, height),
                (half, -half, height),
                (half, half, height),
                (-half, half, height),
            ]
        )
    for corner in range(4):
        following = (corner + 1) % 4
        # Counter clockwise as seen from outside: along the lower ring, then up.
        faces.append((first + corner, first + following, first + 4 + following, first + 4 + corner))
    faces.append((first + 4, first + 5, first + 6, first + 7))

    model = common.create_mesh_object(NAME, vertices, faces)
    # face_project_uvs: the sides of the worn top are slanted. The density is the one of
    # the walls, so the stone is one block of the same stone.
    common.face_project_uvs(model.data, common.METRES_PER_UV_UNIT)
    common.assign_textured_material(model, "wall_stone", "wall_stone.png", "wall_stone_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the stone: from the side and from above.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.3),
            camera_positions=[(0.9, -1.4, 0.7), (0.5, -0.6, 1.5)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
