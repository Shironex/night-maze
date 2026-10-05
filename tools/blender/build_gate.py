# Builds the model gate: a closed wooden gate that fills one side of a maze cell, between
# two pillars, in the place of a wall segment.
# Output: assets/models/gate.obj and gate.mtl.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "gate"

# All sizes are in metres. The geometry is written in Blender space: X is the width of the
# gate, Y is its thickness and Z is the height. After export Z becomes the game Y axis.
# The gate stands like the model wall_straight: along X, centred on the origin, with the
# origin on the floor in the middle of the base.

# The gate fills one side of a 2 m maze cell. Its two ends disappear inside the pillars.
HALF_LENGTH = 1.0
# Lower than the wall (3.0 m), and at a height where the pillar has no horizontal face.
HEIGHT = 2.75

# The planks are 0.08 m thick. The iron bands stand out by 0.02 m on both sides, so the
# gate is 0.12 m thick in total.
PLANK_HALF_THICKNESS = 0.04
BAND_HALF_THICKNESS = 0.06

# Three iron bands. The texture gate_wood.png paints the iron at the same heights: its
# bands are 40 pixels high around the rows 96 and 352, and with 256 pixels per metre and a
# repeat every 2 m that is 0.375 m, 1.375 m and 2.375 m (see make_textures.py).
BAND_CENTRES = (0.375, 1.375, 2.375)
BAND_HALF_HEIGHT = 20 / 256
# The upper and lower edge of a band are slanted over this height. It is more than the
# 0.02 m the band stands out, so a slanted face is steeper than 45 degrees: box_project_uvs
# then treats it like the front of the gate and it gets the iron rows of the texture. A
# level edge would be textured from above, with the wood at the bottom of the picture.
BAND_SLANT = 0.03


def add_band(vertices, faces, centre):
    """Appends one iron band at the height `centre`: a raised strip on both sides."""
    bottom = centre - BAND_HALF_HEIGHT
    top = centre + BAND_HALF_HEIGHT

    # -1 is the side facing Blender -Y, +1 the side facing Blender +Y.
    for side in (-1.0, 1.0):
        wood_y = side * PLANK_HALF_THICKNESS
        iron_y = side * BAND_HALF_THICKNESS

        # The outline of the strip seen from its end: it leaves the planks at `bottom`,
        # reaches its full thickness after the slant, and returns to the planks at `top`.
        # The same four corners at the left end get the indices first ... first + 3, at
        # the right end first + 4 ... first + 7.
        first = len(vertices)
        for x in (-HALF_LENGTH, HALF_LENGTH):
            vertices.extend(
                [
                    (x, wood_y, bottom),  # 0
                    (x, iron_y, bottom + BAND_SLANT),  # 1
                    (x, iron_y, top - BAND_SLANT),  # 2
                    (x, wood_y, top),  # 3
                ]
            )

        # Counter clockwise as seen from outside for the side facing -Y.
        strip = [
            (0, 4, 5, 1),  # lower slant
            (1, 5, 6, 2),  # flat face
            (2, 6, 7, 3),  # upper slant
            (0, 1, 2, 3),  # left end
            (4, 7, 6, 5),  # right end
        ]
        for corners in strip:
            if side > 0.0:
                # The side facing +Y is a mirror image, so the order turns around.
                corners = tuple(reversed(corners))
            faces.append(tuple(first + corner for corner in corners))


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # The planks: one box for the whole gate. The separate planks are painted in the
    # texture. Its underside lies on the floor and is never seen.
    common.add_box(
        vertices,
        faces,
        (-HALF_LENGTH, -PLANK_HALF_THICKNESS, 0.0),
        (HALF_LENGTH, PLANK_HALF_THICKNESS, HEIGHT),
        skip=("-z",),
    )

    for centre in BAND_CENTRES:
        add_band(vertices, faces, centre)

    model = common.create_mesh_object(NAME, vertices, faces)
    common.box_project_uvs(model.data)
    common.assign_textured_material(model, "gate_wood", "gate_wood.png", "gate_wood_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the gate: from the front and a little to the
        # side, and from close to the floor near one end.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 1.4),
            camera_positions=[(3.0, -6.5, 2.4), (-2.2, -2.6, 0.6)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
