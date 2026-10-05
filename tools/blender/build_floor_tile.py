# Builds the model floor_tile: the floor of one maze cell, a flat 2 x 2 m square.
# Output: assets/models/floor_tile.obj and floor_tile.mtl.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_floor_tile.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "floor_tile"

# One maze cell is 2 x 2 m and the tile is centred on the origin, so it reaches 1 m in
# every direction.
HALF_SIZE = 1.0


def build(shots):
    common.reset_scene()

    # The tile lies in the Blender X-Y plane at height 0, which is the game floor (y = 0).
    # The corners go counter clockwise as seen from above, so the face looks up: the
    # normal is Blender +Z, which is +Y in the game.
    vertices = [
        (-HALF_SIZE, -HALF_SIZE, 0.0),
        (HALF_SIZE, -HALF_SIZE, 0.0),
        (HALF_SIZE, HALF_SIZE, 0.0),
        (-HALF_SIZE, HALF_SIZE, 0.0),
    ]
    faces = [(0, 1, 2, 3)]

    model = common.create_mesh_object(NAME, vertices, faces)
    common.box_project_uvs(model.data)
    common.assign_textured_material(
        model, "floor_stone", "floor_stone.png", "floor_stone_normal.png"
    )
    common.export_obj(NAME)

    if shots:
        # From a standing height at an angle, and straight from above.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.0),
            camera_positions=[(2.0, -3.4, 1.7), (0.0, 0.0, 4.0)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
