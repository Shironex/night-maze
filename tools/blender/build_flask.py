# Builds the model flask: a small shepherd's flask of glazed stoneware with a cork and a
# cord loop at its neck. The player will pick it up to get stamina back.
# Output: assets/models/flask.obj and flask.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_flask.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "flask"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. The flask is 0.25 m tall and its origin is on
# the floor, in the middle of its base, like the origin of the crystals.
HEIGHT = 0.25

# The flask is turned on a wheel: every ring of its outline has this many corners.
SIDES = 8

# The outline of the flask from the foot to the lip, one ring per line: the height of the
# ring, its radius along X, and how much narrower it is along Y (1 = round). The body is
# pressed flat, like a flask that is carried against the hip. The neck is round, because
# a cork has to fit into it.
BODY_RINGS = (
    (0.000, 0.046, 0.62),  # the foot it stands on
    (0.035, 0.078, 0.62),
    (0.085, 0.088, 0.62),  # the belly, the widest place
    (0.135, 0.070, 0.66),
    (0.168, 0.034, 0.85),  # the shoulder
    (0.185, 0.024, 1.00),  # the neck, where the cord is tied
    (0.205, 0.030, 1.00),  # the lip flares out
    (0.212, 0.028, 1.00),  # the top of the lip
)

# The cork: a short plug that stands out of the lip and is a little wider at the top.
CORK_BOTTOM = (0.212, 0.019, 1.00)
CORK_TOP = (HEIGHT, 0.021, 1.00)

# The cord loop the flask hangs from: a ring that stands upright beside the neck and
# touches it. The ring has LOOP_SEGMENTS straight pieces, and the cord itself is a bar
# with three sides, which is the cheapest shape that still has a thickness.
LOOP_CENTRE = (0.052, 0.0, 0.186)
LOOP_RADIUS = 0.030
LOOP_SEGMENTS = 6
CORD_RADIUS = 0.006
CORD_SIDES = 3

# The picture of the flask is used once, not repeated: v is the height on the flask, from
# 0 at the foot to 1 at the top of the cork, and u goes once around it. make_textures.py
# paints the bands of the picture at these heights (FLASK_CORD_ROWS and FLASK_CORK_ROW
# there). The flat parts and the cord loop get a small piece of the band they belong to:
# the middle of that piece (u, v) is listed here.
FOOT_UV_CENTRE = (0.5, 0.10)   # unglazed clay
LIP_UV_CENTRE = (0.5, 0.80)    # the glaze of the lip
CORK_UV_CENTRE = (0.5, 0.93)   # the cork
CORD_V_RANGE = (0.69, 0.73)    # inside the band of the wound cord


def add_ring(vertices, ring):
    """Appends the SIDES corners of one ring of the outline and returns its first index.

    The corners go counter clockwise as seen from above.
    """
    height, radius, narrow = ring
    first = len(vertices)
    for corner in range(SIDES):
        angle = math.radians(corner * 360.0 / SIDES)
        vertices.append((radius * math.cos(angle), radius * narrow * math.sin(angle), height))
    return first


def add_band(faces, uvs, lower, upper, lower_height, upper_height):
    """Appends the SIDES faces between two rings, with their texture coordinates.

    uvs gets one entry per face: the (u, v) of its four corners, in the order of the
    corners. u is the number of the corner divided by SIDES, v the height divided by the
    height of the flask. The last face ends at u = 1 and not at u = 0, so the picture
    goes once around and is not squeezed backwards into the last face.
    """
    for corner in range(SIDES):
        following = (corner + 1) % SIDES
        # Counter clockwise as seen from outside, so the normal points outwards.
        faces.append((lower + corner, lower + following, upper + following, upper + corner))
        left = corner / SIDES
        right = (corner + 1) / SIDES
        low = lower_height / HEIGHT
        high = upper_height / HEIGHT
        uvs.append(((left, low), (right, low), (right, high), (left, high)))


def flat_uvs(vertices, face, centre, mirror=1.0):
    """Returns the (u, v) of the corners of a level face: its x and y around `centre`.

    One metre is one unit of u and v, so the small face covers a small piece of the
    picture. mirror = -1 turns x around, for a face that is seen from below.
    """
    return tuple(
        (centre[0] + mirror * vertices[index][0], centre[1] + vertices[index][1]) for index in face
    )


def add_cord_loop(vertices, faces, uvs):
    """Appends the cord loop: a ring of LOOP_SEGMENTS bars with CORD_SIDES sides each."""
    centre_x, centre_y, centre_z = LOOP_CENTRE
    first = len(vertices)

    for segment in range(LOOP_SEGMENTS):
        # Where the middle of the cord is at this corner of the loop. The loop stands
        # upright in the X-Z plane.
        around = math.radians(segment * 360.0 / LOOP_SEGMENTS)
        out_x = math.cos(around)
        out_z = math.sin(around)
        for side in range(CORD_SIDES):
            # The corners of the bar lie on a small circle around the middle of the
            # cord, in the plane that holds the outward direction and Y.
            turn = math.radians(side * 360.0 / CORD_SIDES)
            reach = LOOP_RADIUS + CORD_RADIUS * math.cos(turn)
            vertices.append(
                (
                    centre_x + reach * out_x,
                    centre_y + CORD_RADIUS * math.sin(turn),
                    centre_z + reach * out_z,
                )
            )

    v_low, v_high = CORD_V_RANGE
    for segment in range(LOOP_SEGMENTS):
        here = first + segment * CORD_SIDES
        there = first + ((segment + 1) % LOOP_SEGMENTS) * CORD_SIDES
        for side in range(CORD_SIDES):
            following = (side + 1) % CORD_SIDES
            # Counter clockwise as seen from outside the cord.
            faces.append((here + side, here + following, there + following, there + side))
            # u runs along the cord, v across it.
            left = segment / LOOP_SEGMENTS
            right = (segment + 1) / LOOP_SEGMENTS
            uvs.append(((left, v_low), (left, v_high), (right, v_high), (right, v_low)))


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []
    # The texture coordinates of every face, in the order of `faces`.
    uvs = []

    # The body: one band of faces between every two rings of the outline.
    rings = [add_ring(vertices, ring) for ring in BODY_RINGS]
    for index in range(len(BODY_RINGS) - 1):
        add_band(
            faces, uvs, rings[index], rings[index + 1], BODY_RINGS[index][0], BODY_RINGS[index + 1][0]
        )

    # The underside of the foot: the lowest ring in the opposite order, so it faces down.
    foot = tuple(rings[0] + corner for corner in reversed(range(SIDES)))
    faces.append(foot)
    uvs.append(flat_uvs(vertices, foot, FOOT_UV_CENTRE, mirror=-1.0))

    # The cork. The level ring between the lip and the cork closes the neck.
    cork_bottom = add_ring(vertices, CORK_BOTTOM)
    cork_top = add_ring(vertices, CORK_TOP)
    for corner in range(SIDES):
        following = (corner + 1) % SIDES
        rim = (rings[-1] + corner, rings[-1] + following, cork_bottom + following, cork_bottom + corner)
        faces.append(rim)
        uvs.append(flat_uvs(vertices, rim, LIP_UV_CENTRE))
    add_band(faces, uvs, cork_bottom, cork_top, CORK_BOTTOM[0], CORK_TOP[0])
    top = tuple(cork_top + corner for corner in range(SIDES))
    faces.append(top)
    uvs.append(flat_uvs(vertices, top, CORK_UV_CENTRE))

    add_cord_loop(vertices, faces, uvs)

    model = common.create_mesh_object(NAME, vertices, faces)

    # The texture coordinates are written by hand here: the two helpers of
    # blender_common repeat a texture every few metres, and this picture is used once.
    # A loop is one corner of one face, and from_pydata keeps the order of both lists.
    uv_layer = model.data.uv_layers.new(name="uv")
    for polygon, face_uvs in zip(model.data.polygons, uvs):
        for loop_index, uv in zip(polygon.loop_indices, face_uvs):
            uv_layer.data[loop_index].uv = uv

    common.assign_textured_material(model, "flask", "flask.png", "flask_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views aimed at the middle of the flask: its wide side, and from above.
        common.render_review_shots(
            NAME,
            target=(0.01, 0.0, 0.125),
            camera_positions=[(0.25, -0.62, 0.22), (0.45, -0.35, 0.5)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
