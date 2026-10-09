# Builds the two models of the stile, the place every night begins: a step stile in the
# border wall of the start cell, as dry-stone walls have them.
#   stile       one wall segment with a notch in its top and four flat through-stones
#               that climb to it. It takes the place of the plain segment.
#   stile_post  the oak post beside the steps, with the iron hook the lamp hung on. The
#               hook is empty: the lamp is in the hand of the player.
# Output: assets/models/<name>.obj and <name>.mtl for each of the two.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_stile.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import build_wall_straight as wall
from mathutils import Vector

NAME = "stile"
POST_NAME = "stile_post"

# All sizes are in metres. The geometry is written in Blender space: X is the length of the
# wall, Y is its thickness and Z is the height. After export Z becomes the game Y axis and
# -Y the game +Z axis, so everything at a negative Y stands on the side of the wall that
# faces into the start cell. Both models have the origin of the wall segment: the middle
# of its base. The game draws them with the matrix of that segment.

# ---- the wall with the notch ----
# The notch is 0.7 m wide. Left and right of it the wall keeps its full height.
NOTCH_START = 0.15
NOTCH_END = 0.85
# The body of the wall ends here under the notch, and two coping stones lie on it: the
# wall is 2.4 m high there. The second stone is a little thinner and lower, so the two
# never share a plane with each other or with the coping of the wall.
NOTCH_BODY_TOP = 2.25
NOTCH_STONE_JOINT = 0.52
NOTCH_FIRST_STONE = (0.15, 2.4)  # half thickness, top
NOTCH_SECOND_STONE = (0.14, 2.39)

# ---- the through-stones ----
# Each step: x of its middle, height of its middle, half length along the wall, how far it
# stands out from the middle of the wall, thickness. They climb towards the notch.
STEPS = (
    (-0.52, 0.5, 0.27, 0.42, 0.11),
    (-0.22, 0.95, 0.24, 0.4, 0.1),
    (0.08, 1.4, 0.28, 0.44, 0.12),
    (0.38, 1.85, 0.23, 0.39, 0.1),
)
# A through-stone goes through the whole wall and shows 2 cm on its far face.
STEP_FAR_END = 0.12
# Every stone is turned a little about a point under its middle, so no two lie alike. The
# turns come from a random number generator with this seed.
STEP_SEED = 91
STEP_TURN_CENTRE_Y = -0.26
STEP_MAX_TURN_Z_AXIS = math.radians(6.0) / 2.0  # about the Y axis: one end dips
STEP_MAX_TURN_X_AXIS = math.radians(5.0) / 2.0  # about the X axis: the stone leans

# ---- the post ----
# The start cell is a corner of the maze, so a wall can stand across either end of this
# segment, and its plinth reaches 0.86 m from the middle. The post stands 1 cm clear of
# that, in front of the far end of the notch.
POST_X = (0.75, 0.85)
POST_Y = (-0.37, -0.27)
# The post reaches below the ground like the milestone: the wall is lowered to the lowest
# ground under its own footprint, and the post stands 0.3 m in front of it.
POST_BOTTOM = -0.15
POST_TOP = 3.25
# The cap: a small board on top, 2 cm wider on every side.
CAP_OVERHANG = 0.02
CAP_TOP = 3.29

# The hook: a thin square bar that leaves the post level, bends down and curls up again.
# The points are the middle line of the bar. X is measured from the face of the post.
HOOK_HEIGHT = 2.98
HOOK_PATH = (
    (0.0, 0.0),
    (-0.07, 0.0),
    (-0.11, -0.03),
    (-0.12, -0.08),
    (-0.10, -0.12),
    (-0.07, -0.11),
    (-0.055, -0.08),
)
HOOK_RADIUS = 0.008
HOOK_SIDES = 4

# ---- the picture of the post ----
# The post wears gate_wood.png, the picture of the gate: upright oak planks 64 pixels wide
# with two iron bands across them (make_textures.py). The post is cut out of one plank and
# the hook out of a band, so one picture gives the model both of its materials.
PICTURE_SIZE = 512.0
PIXELS_PER_METRE = PICTURE_SIZE / common.METRES_PER_UV_UNIT
# The middle of the fourth plank from the left.
PLANK_CENTRE = 3 * 64 + 32
# The upper band: the row of its middle and half of its height. On the post it lies at
# the height of the hook, as the iron ring the hook is forged to.
BAND_CENTRE = 352
BAND_HALF_HEIGHT = 20
# The wood under the band may use the rows from here up to the band. The lower band of
# the picture ends at row 116.
WOOD_FIRST_ROW = 122
# The hook takes its iron from the band, beside the rivet in the middle of the plank.
HOOK_PIXEL = (PLANK_CENTRE + 15, BAND_CENTRE)
# The cap takes its wood from the middle of the plank, between the two bands.
CAP_PIXEL = (PLANK_CENTRE, 224)


def add_notched_wall(vertices, faces):
    """Appends the wall: plinth, three pieces of body, the coping and the notch stones."""
    half = wall.HALF_LENGTH
    body = wall.BODY_HALF_THICKNESS
    trim = wall.TRIM_HALF_THICKNESS
    plinth_top = wall.PLINTH_HEIGHT
    coping_bottom = wall.HEIGHT - wall.COPING_HEIGHT

    # Plinth. Its underside lies on the floor and is never seen.
    common.add_box(vertices, faces, (-half, -trim, 0.0), (half, trim, plinth_top), skip=("-z",))
    # The body left of the notch, under it and right of it. Every underside is covered by
    # the plinth and every top by a coping stone. The two tall pieces keep their ends:
    # above the notch they are its two sides.
    common.add_box(
        vertices, faces, (-half, -body, plinth_top), (NOTCH_START, body, coping_bottom),
        skip=("-z", "+z"),
    )
    common.add_box(
        vertices, faces, (NOTCH_START, -body, plinth_top), (NOTCH_END, body, NOTCH_BODY_TOP),
        skip=("-z", "+z"),
    )
    common.add_box(
        vertices, faces, (NOTCH_END, -body, plinth_top), (half, body, coping_bottom),
        skip=("-z", "+z"),
    )
    # The coping left and right of the notch, with all six sides.
    common.add_box(vertices, faces, (-half, -trim, coping_bottom), (NOTCH_START, trim, wall.HEIGHT))
    common.add_box(vertices, faces, (NOTCH_END, -trim, coping_bottom), (half, trim, wall.HEIGHT))
    # The two stones in the notch, with all six sides.
    common.add_box(
        vertices, faces, (NOTCH_START, -NOTCH_FIRST_STONE[0], NOTCH_BODY_TOP),
        (NOTCH_STONE_JOINT, NOTCH_FIRST_STONE[0], NOTCH_FIRST_STONE[1]),
    )
    common.add_box(
        vertices, faces, (NOTCH_STONE_JOINT, -NOTCH_SECOND_STONE[0], NOTCH_BODY_TOP),
        (NOTCH_END, NOTCH_SECOND_STONE[0], NOTCH_SECOND_STONE[1]),
    )


def add_steps(vertices, faces):
    """Appends the four through-stones, each a box turned a little about its own middle."""
    rnd = wall.random_numbers(STEP_SEED)
    for x, z, half_length, out, thickness in STEPS:
        centre = (x, STEP_TURN_CENTRE_Y, z)
        # The order of the rnd() calls is part of the model.
        about_y = (rnd() - 0.5) * 2.0 * STEP_MAX_TURN_Z_AXIS
        about_x = (rnd() - 0.5) * 2.0 * STEP_MAX_TURN_X_AXIS

        first = len(vertices)
        common.add_box(
            vertices, faces, (x - half_length, -out, z - thickness / 2.0),
            (x + half_length, STEP_FAR_END, z + thickness / 2.0),
        )
        for index in range(first, len(vertices)):
            vertices[index] = turned(vertices[index], centre, about_x, about_y)


def turned(point, centre, about_x, about_y):
    """Turns a point about `centre`: first about the X axis, then about the Y axis."""
    x, y, z = wall.sub(point, centre)
    cos_x, sin_x = math.cos(about_x), math.sin(about_x)
    y, z = y * cos_x - z * sin_x, y * sin_x + z * cos_x
    cos_y, sin_y = math.cos(about_y), math.sin(about_y)
    x, z = x * cos_y + z * sin_y, -x * sin_y + z * cos_y
    return wall.add((x, y, z), centre)


def add_post(vertices, faces, pixels):
    """Appends the post, its cap and the hook.

    `pixels` gets one entry per face, in the order of `faces`: None for a side of the
    post, whose place in the picture follows from its height (post_row), or the pixel
    of the picture the face lies around.
    """
    band_low = HOOK_HEIGHT - BAND_HALF_HEIGHT / PIXELS_PER_METRE
    band_high = HOOK_HEIGHT + BAND_HALF_HEIGHT / PIXELS_PER_METRE

    # The post: three boxes on top of each other, the middle one being the iron band. Only
    # their four sides are needed: the underside is in the ground, the top is under the
    # cap and the boxes cover each other.
    for bottom, top in ((POST_BOTTOM, band_low), (band_low, band_high), (band_high, POST_TOP)):
        before = len(faces)
        common.add_box(
            vertices, faces, (POST_X[0], POST_Y[0], bottom), (POST_X[1], POST_Y[1], top),
            skip=("-z", "+z"),
        )
        pixels.extend([None] * (len(faces) - before))

    # The cap, with all six sides: its underside shows where it overhangs the post.
    before = len(faces)
    common.add_box(
        vertices, faces, (POST_X[0] - CAP_OVERHANG, POST_Y[0] - CAP_OVERHANG, POST_TOP),
        (POST_X[1] + CAP_OVERHANG, POST_Y[1] + CAP_OVERHANG, CAP_TOP),
    )
    pixels.extend([CAP_PIXEL] * (len(faces) - before))

    # The hook: a ring of HOOK_SIDES corners around every point of its middle line, and
    # two triangles between every two corners of neighbouring rings. Triangles, because
    # the rings turn with the bar and four such corners do not lie in one plane.
    middle_y = (POST_Y[0] + POST_Y[1]) / 2.0
    path = [(POST_X[0] + along, middle_y, HOOK_HEIGHT + up) for along, up in HOOK_PATH]
    before = len(faces)
    rings = []
    for index, point in enumerate(path):
        # The bar runs from the point before to the point after, and lies in the X-Z
        # plane, so Y is always across it.
        along = wall.unit(wall.sub(path[min(index + 1, len(path) - 1)], path[max(index - 1, 0)]))
        side = (0.0, 1.0, 0.0)
        other = wall.cross(along, side)
        ring = []
        for corner in range(HOOK_SIDES):
            angle = corner * 2.0 * math.pi / HOOK_SIDES
            ring.append(len(vertices))
            vertices.append(
                wall.add(
                    point,
                    wall.add(
                        wall.scaled(other, math.cos(angle) * HOOK_RADIUS),
                        wall.scaled(side, math.sin(angle) * HOOK_RADIUS),
                    ),
                )
            )
        rings.append(ring)
    for index in range(len(rings) - 1):
        middle = wall.between(path[index], path[index + 1], 0.5)
        for corner in range(HOOK_SIDES):
            following = (corner + 1) % HOOK_SIDES
            a, b = rings[index][corner], rings[index][following]
            c, d = rings[index + 1][following], rings[index + 1][corner]
            wall.add_face_away_from(vertices, faces, (a, b, c), middle)
            wall.add_face_away_from(vertices, faces, (a, c, d), middle)
    # The free end is closed. The other end is inside the post.
    wall.add_face_away_from(vertices, faces, tuple(rings[-1]), path[-2])
    pixels.extend([HOOK_PIXEL] * (len(faces) - before))


def post_row(height):
    """The row of the picture for a height on the side of the post.

    The band and the wood above it have the size they have on the gate. The wood below
    the band is one long plank: 3 m of post on the 0.8 m of wood between the two bands,
    stretched along the grain only.
    """
    band_low = HOOK_HEIGHT - BAND_HALF_HEIGHT / PIXELS_PER_METRE
    if height >= band_low:
        return BAND_CENTRE + (height - HOOK_HEIGHT) * PIXELS_PER_METRE
    share = (height - POST_BOTTOM) / (band_low - POST_BOTTOM)
    return WOOD_FIRST_ROW + share * (BAND_CENTRE - BAND_HALF_HEIGHT - WOOD_FIRST_ROW)


def post_uvs(mesh, pixels):
    """Gives every face of the post model its piece of gate_wood.png.

    The face is projected onto its own plane, like common.face_project_uvs does. A side of
    the post (None in `pixels`) lies in the middle of the plank, at the rows post_row gives
    for its heights. Every other face lies around its pixel, in the size of the gate.
    """
    uv_layer = mesh.uv_layers.new(name="uv")
    axis = Vector(((POST_X[0] + POST_X[1]) / 2.0, (POST_Y[0] + POST_Y[1]) / 2.0, 0.0))

    # from_pydata keeps the order of the faces, so face number i has pixels[i].
    for polygon, pixel in zip(mesh.polygons, pixels):
        normal = polygon.normal
        up = Vector((0.0, 0.0, 1.0)) - normal * normal.z
        if up.length < 0.000001:
            # A level face has no height direction. Blender Y is used instead.
            up = Vector((0.0, 1.0, 0.0))
        up.normalize()
        right = up.cross(normal)

        for loop_index in polygon.loop_indices:
            position = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            if pixel is None:
                column = PLANK_CENTRE + (position - axis).dot(right) * PIXELS_PER_METRE
                row = post_row(position.z)
            else:
                offset = position - polygon.center
                column = pixel[0] + offset.dot(right) * PIXELS_PER_METRE
                row = pixel[1] + offset.dot(up) * PIXELS_PER_METRE
            uv_layer.data[loop_index].uv = (column / PICTURE_SIZE, row / PICTURE_SIZE)


def build(shots):
    common.reset_scene()
    vertices = []
    faces = []
    add_notched_wall(vertices, faces)
    add_steps(vertices, faces)
    model = common.create_mesh_object(NAME, vertices, faces)
    # face_project_uvs: the steps are turned, so their faces are slanted. The density is
    # the one of the walls, so the stone looks as on the plain wall beside it.
    common.face_project_uvs(model.data, common.METRES_PER_UV_UNIT)
    common.assign_textured_material(model, "wall_stone", "wall_stone.png", "wall_stone_normal.png")
    common.export_obj(NAME)
    print(NAME, "has", sum(len(face) - 2 for face in faces), "triangles")
    if shots:
        # From inside the cell, a little to the side, and from the floor in front of the
        # steps, looking up at the notch.
        common.render_review_shots(
            NAME, target=(0.1, 0.0, 1.5), camera_positions=[(-2.4, -5.0, 2.2), (0.6, -2.2, 1.0)]
        )

    common.reset_scene()
    vertices = []
    faces = []
    pixels = []
    add_post(vertices, faces, pixels)
    model = common.create_mesh_object(POST_NAME, vertices, faces)
    post_uvs(model.data, pixels)
    common.assign_textured_material(model, "gate_wood", "gate_wood.png", "gate_wood_normal.png")
    common.export_obj(POST_NAME)
    print(POST_NAME, "has", sum(len(face) - 2 for face in faces), "triangles")
    if shots:
        # The whole post, and its top with the band and the hook.
        common.render_review_shots(
            POST_NAME, target=(0.75, -0.32, 2.95),
            camera_positions=[(-0.6, -5.5, 2.4), (0.25, -1.1, 3.1)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
