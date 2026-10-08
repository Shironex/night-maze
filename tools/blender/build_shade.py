# Builds the model shade: the tall hooded figure that follows the player through the maze
# whenever the flashlight is not on it. A cloak from the ground to the shoulders and a
# pointed hood with a hollow where a face would be.
# Output: assets/models/shade.obj and shade.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_shade.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "shade"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis, and the front of the figure (Blender -Y) looks
# along the game +Z axis: the game turns the model around the vertical axis from there.
# The figure is 2.1 m tall, taller than the player (1.8 m), and its origin is on the
# ground in the middle of its base.
HEIGHT = 2.1

# The cloak goes this far below the origin. The ground of the maze is uneven, and a hem
# that ended exactly at the origin would hang in the air on the low side.
HEM_DEPTH = 0.1

# The figure is built like the flask, ring over ring: every ring of its outline has this
# many corners.
SIDES = 16

# The outline from the hem to the tip of the hood, one ring per line: the height of the
# ring, its radius along X (from side to side), how much narrower it is along Y (from
# front to back, 1 = round) and how far it is moved along Y (negative = forwards).
# The body is wide and flat like a person seen from the front, the hood is round, and
# its tip leans back.
RINGS = (
    (-HEM_DEPTH, 0.46, 0.84, 0.00),  # the hem, in the ground
    (0.00, 0.45, 0.84, 0.00),
    (0.35, 0.40, 0.82, 0.00),
    (0.80, 0.34, 0.76, 0.00),
    (1.20, 0.31, 0.70, 0.00),
    (1.46, 0.37, 0.60, 0.00),  # the shoulders, the widest place of the upper body
    (1.58, 0.31, 0.72, -0.01),  # the hood lies on them like a cowl: there is no neck
    (1.69, 0.255, 0.96, -0.03),
    (1.81, 0.24, 1.06, -0.05),  # the hood at its widest
    (1.92, 0.21, 1.10, -0.04),
    (2.01, 0.15, 1.10, -0.01),
    (2.07, 0.08, 1.10, 0.03),
    (HEIGHT, 0.03, 1.00, 0.07),  # the tip of the hood
)

# The folds of the cloak: the radius of a ring swings by this share around the figure,
# FOLD_COUNT times, and the swing fades out towards the shoulders. With flat faces this
# is what gives the cloak ridges that catch the light.
FOLD_DEPTH = 0.07
FOLD_COUNT = 4
FOLD_TOP = 1.46

# The hollow of the hood. The corners of the hood rings that look forwards (within this
# angle of the front) are pulled in to this share of their radius, so the hood has a dark
# opening and no face. The rings between the two heights have it.
FACE_LOW = 1.66
FACE_HIGH = 1.95
FACE_HALF_ANGLE = 50.0
FACE_INSET = 0.3

# The front of the figure, as an angle around Z: -Y.
FRONT_ANGLE = 270.0


def add_ring(vertices, number):
    """Appends the SIDES corners of ring `number` of the outline and returns its first index.

    The corners go counter clockwise as seen from above.
    """
    height, radius, narrow, forward = RINGS[number]
    first = len(vertices)
    for corner in range(SIDES):
        degrees = corner * 360.0 / SIDES
        angle = math.radians(degrees)
        reach = radius

        # The folds: deepest at the ground, gone at the shoulders. Every other ring is
        # turned a little, so a fold does not run down as a straight ruler line.
        if height < FOLD_TOP:
            fade = 1.0 - max(height, 0.0) / FOLD_TOP
            reach *= 1.0 + FOLD_DEPTH * fade * math.cos(FOLD_COUNT * angle + 0.6 * number)

        # The hollow of the hood.
        from_front = abs((degrees - FRONT_ANGLE + 180.0) % 360.0 - 180.0)
        if FACE_LOW <= height <= FACE_HIGH and from_front <= FACE_HALF_ANGLE:
            reach *= FACE_INSET

        vertices.append(
            (reach * math.cos(angle), forward + reach * narrow * math.sin(angle), height)
        )
    return first


def picture_height(height):
    """The v of a height on the figure: 0 at the hem, 1 at the tip of the hood."""
    return (height + HEM_DEPTH) / (HEIGHT + HEM_DEPTH)


def add_band(faces, uvs, lower, upper, lower_height, upper_height):
    """Appends the SIDES faces between two rings, with their texture coordinates.

    The picture goes once around the figure: u is the number of the corner divided by
    SIDES, and the last face ends at u = 1 and not at u = 0. v is the height.
    """
    for corner in range(SIDES):
        following = (corner + 1) % SIDES
        # Counter clockwise as seen from outside, so the normal points outwards.
        faces.append((lower + corner, lower + following, upper + following, upper + corner))
        left = corner / SIDES
        right = (corner + 1) / SIDES
        low = picture_height(lower_height)
        high = picture_height(upper_height)
        uvs.append(((left, low), (right, low), (right, high), (left, high)))


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []
    # The texture coordinates of every face, in the order of `faces`.
    uvs = []

    rings = [add_ring(vertices, number) for number in range(len(RINGS))]
    for number in range(len(RINGS) - 1):
        add_band(faces, uvs, rings[number], rings[number + 1], RINGS[number][0], RINGS[number + 1][0])

    # The tip of the hood is closed by one small face. Its piece of the picture is a
    # point at the top edge: the face is two centimetres wide.
    top = tuple(rings[-1] + corner for corner in range(SIDES))
    faces.append(top)
    uvs.append(tuple((corner / SIDES, 1.0) for corner in range(SIDES)))
    # The hem has no face under it: it is in the ground.

    model = common.create_mesh_object(NAME, vertices, faces)

    # The texture coordinates are written by hand, as for the flask: the picture is used
    # once and not repeated every few metres.
    uv_layer = model.data.uv_layers.new(name="uv")
    for polygon, face_uvs in zip(model.data.polygons, uvs):
        for loop_index, uv in zip(polygon.loop_indices, face_uvs):
            uv_layer.data[loop_index].uv = uv

    common.assign_textured_material(model, "shade", "shade.png", "shade_normal.png")
    common.export_obj(NAME)

    if shots:
        # Three views aimed at the chest: from the front, from the side and from where
        # a player stands who looks up at it.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 1.05),
            camera_positions=[(0.0, -5.6, 1.4), (4.6, -3.0, 1.7), (1.2, -3.4, 1.7)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
