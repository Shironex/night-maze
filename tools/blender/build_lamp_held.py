# Builds the lamp the player holds: a dented tin flashlight with cloth tape around its
# grip, a wide head, a switch, and a small window in the body where the splinter that
# feeds it shows. It comes as three models that fit into each other, because the game
# gives each a glow of its own: lamp_held is the tin with the tape, the frame of the
# window and the switch, lamp_held_lens the lens and lamp_held_window the glass of the
# window. All three wear lamp_held.png.
# Output: assets/models/lamp_held.obj, lamp_held_lens.obj, lamp_held_window.obj and their
# .mtl files.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_lamp_held.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from mathutils import Vector

import blender_common as common
import make_textures as textures

NAME = "lamp_held"
LENS_NAME = "lamp_held_lens"
WINDOW_NAME = "lamp_held_window"

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# The lamp lies along Blender +Y, which is -Z in the game: the way the player looks. Its
# origin is the middle of its butt, and the axis of the tube runs through the origin, so
# the game can put the lamp on the axis of its beam and turn it about that axis only. The
# window and the switch are on top (+Z).
#
# The numbers below are measured along that axis, from the butt (`out`), and across it
# (`radius`). The lamp is LENGTH long with the rim of its head, and no corner may lie
# further from the axis than HEAD_RADIUS: the game is told these two (HELD_LAMP_ in
# src/game/Lighting.hpp), and the script stops if the model outgrows them.
LENGTH = 0.224
HEAD_RADIUS = 0.0335

# The tube is turned on a wheel: every ring of its outline has this many corners. The
# rings are turned by half a side, so a flat side is on top, where the window sits.
SIDES = 8
TURN = math.pi / SIDES

# The lens: a flat disc, sunk 5 mm into the rim of the head. The game hangs the glow of
# the beam on it (HELD_LAMP_LENS_OUT in src/game/Lighting.hpp is LENS_OUT).
LENS_OUT = 0.219
LENS_RADIUS = 0.0285

# The outline of the tin from the lens to the butt, one ring per line: its radius and how
# far out it is. It starts inside the rim, at the edge of the lens, goes forward to the
# front of the rim, around it, back along the head and the body, and ends at the flat
# butt, which CAP closes.
OUTLINE = (
    (0.0285, 0.219),  # the edge of the lens, inside the rim
    (0.0285, 0.224),  # the front of the rim, inside
    (0.0335, 0.222),  # the front of the rim, outside
    (0.0335, 0.205),  # the head is widest here
    (0.0300, 0.195),
    (0.0205, 0.160),  # the head narrows to the body
    (0.0182, 0.140),
    (0.0182, 0.115),  # the body, a straight tube
    (0.0182, 0.080),
    (0.0182, 0.050),
    (0.0182, 0.020),
    (0.0195, 0.012),  # the cap of the butt is a little wider
    (0.0190, 0.004),
    (0.0160, 0.000),  # the edge of the flat butt
)
BODY_RADIUS = 0.0182

# The dents: single corners of a ring moved towards the axis. For the number of a ring in
# OUTLINE, the corners that are moved and how far. A face with a moved corner is no
# longer flat, so it is built as two triangles.
DENTS = {
    7: ((1, 0.004), (2, 0.006)),
    8: ((5, 0.005),),
    9: ((6, 0.004), (7, 0.003)),
}
# A dent is a knock, not a hole: the script stops outside of this range.
DENT_RANGE = (0.003, 0.006)

# The cloth tape around the grip: a sleeve over the body, and one more turn at each of
# its ends, a little thicker.
TAPE_OUT = (0.026, 0.100)
TAPE_RADIUS = 0.0198
TAPE_END_RADIUS = 0.0204
TAPE_END_LENGTH = 0.007

# The window on top of the body: an iron frame of four bars around a pane of glass. The
# bars reach FRAME_SINK into the body, so they stand on the tin where it is dented and
# where the round body falls away under their outer edges.
WINDOW_OUT = (0.077, 0.123)
WINDOW_HALF_WIDTH = 0.0095
FRAME_BAR = 0.005
FRAME_SIDE_BAR = 0.003
FRAME_RISE = 0.003
FRAME_SINK = 0.008
# The glass lies this far above the top of the body, below the top of the frame.
GLASS_RISE = 0.0012

# The switch: a block of iron behind the window.
SWITCH_OUT = (0.045, 0.060)
SWITCH_HALF_WIDTH = 0.005
SWITCH_RISE = 0.005
SWITCH_SINK = 0.002

# The picture lamp_held.png is cut into two bands and three pieces (make_textures.py, the
# LAMP_ constants). The tin and the tape go once around the tube: u is the number of the
# corner divided by SIDES, and v runs along the lamp inside the band, PICTURE_MARGIN rows
# away from its edges. Every other face gets a small part from the middle of its piece.
PICTURE_MARGIN = 6
TIN_ROWS = (0, textures.LAMP_TAPE_ROW)
TAPE_ROWS = (textures.LAMP_TAPE_ROW, textures.LAMP_PIECE_ROW)
# The parts of the picture, each as (left, bottom, right, top) in pixels.
PARTS = {
    "tin": (0, TIN_ROWS[0], textures.SIZE, TIN_ROWS[1]),
    "tape": (0, TAPE_ROWS[0], textures.SIZE, TAPE_ROWS[1]),
    "iron": (0, textures.LAMP_PIECE_ROW, textures.LAMP_LENS_COLUMN, textures.SIZE),
    "lens": (textures.LAMP_LENS_COLUMN, textures.LAMP_PIECE_ROW, textures.LAMP_WINDOW_COLUMN,
             textures.SIZE),
    "window": (textures.LAMP_WINDOW_COLUMN, textures.LAMP_PIECE_ROW, textures.SIZE, textures.SIZE),
}
# One metre of a face that takes a piece is this many units of u and v. The largest such
# face is 4.6 cm long, so it covers 47 pixels and stays inside its piece.
UV_UNITS_PER_METRE = 2.0


class Model:
    """The lists of one model: its faces and, for each, its part of the picture and its
    texture coordinates. The corners of all three models are in one list."""

    def __init__(self):
        self.faces = []
        self.parts = []
        self.uvs = []

    def add(self, corners, part, uvs):
        self.faces.append(tuple(corners))
        self.parts.append(part)
        self.uvs.append(tuple(uvs))


def lay(across, up, out):
    """Returns the Blender point of a place on the lamp: `across` and `up` from the axis
    as the rings are turned, `out` along it. The tube is turned around an upright axis
    like a pot on a wheel, and this lays it down along +Y with the side that was at the
    back of the wheel on top."""
    return (-across, out, up)


def face_normal(points):
    """The direction a face looks in, as the average over its edges (Newell's method):
    the four corners of a dented side do not lie in one plane."""
    normal = Vector()
    for place, point in enumerate(points):
        after = points[(place + 1) % len(points)]
        normal += Vector(((point.y - after.y) * (point.z + after.z),
                          (point.z - after.z) * (point.x + after.x),
                          (point.x - after.x) * (point.y + after.y)))
    return normal.normalized()


def add_ring(vertices, radius, out, dents=()):
    """Appends the SIDES corners of one ring and returns their indices."""
    moved = dict(dents)
    ring = []
    for corner in range(SIDES):
        angle = TURN + corner / SIDES * 2.0 * math.pi
        reach = radius - moved.get(corner, 0.0)
        vertices.append(lay(reach * math.cos(angle), reach * math.sin(angle), out))
        ring.append(len(vertices) - 1)
    return ring


def add_band(vertices, model, part, first, second, v_first, v_second, split=False):
    """Appends the SIDES faces between two rings, with their texture coordinates.

    first and second are (ring, radius, out), in the order of an outline whose outside is
    on the left of the direction of travel. u is the number of the corner divided by
    SIDES: the last face ends at u = 1 and not at u = 0, so the picture goes once around.
    split builds every face with a moved corner as two triangles.
    """
    ring, radius, out = first
    next_ring, next_radius, next_out = second
    for corner in range(SIDES):
        following = (corner + 1) % SIDES
        corners = [ring[corner], ring[following], next_ring[following], next_ring[corner]]
        left = corner / SIDES
        right = (corner + 1) / SIDES
        uvs = [(left, v_first), (right, v_first), (right, v_second), (left, v_second)]

        # Where the outside of this face is: across the outline, at the angle of the
        # middle of the face.
        angle = TURN + (corner + 0.5) / SIDES * 2.0 * math.pi
        along = next_out - out
        outside = Vector(lay(-along * math.cos(angle), -along * math.sin(angle),
                             next_radius - radius))
        if face_normal([Vector(vertices[index]) for index in corners]).dot(outside) < 0.0:
            # The corners went clockwise as seen from outside. Turn them around.
            corners.reverse()
            uvs.reverse()

        if split:
            model.add(corners[:3], part, uvs[:3])
            model.add((corners[0], corners[2], corners[3]), part, (uvs[0], uvs[2], uvs[3]))
        else:
            model.add(corners, part, uvs)


def piece_centre(part):
    """The middle of a part of the picture, as (u, v)."""
    left, bottom, right, top = PARTS[part]
    return ((left + right) / 2.0 / textures.SIZE, (bottom + top) / 2.0 / textures.SIZE)


def add_piece_face(vertices, model, part, corners, towards):
    """Appends one flat face that looks in the direction `towards`, with a small part from
    the middle of its piece of the picture.

    The face is projected onto its own plane, as common.piece_project_uvs does: `up` is
    the height as far as the face allows and `right` points to the right for someone who
    looks at the face from outside, so the picture is never a mirror image.
    """
    points = [Vector(vertices[corner]) for corner in corners]
    normal = face_normal(points)
    if normal.dot(Vector(towards)) < 0.0:
        corners = corners[::-1]
        points.reverse()
        normal = -normal

    up = Vector((0.0, 0.0, 1.0)) - normal * normal.z
    if up.length < 0.000001:
        # A level face has no height direction. Blender Y is used instead.
        up = Vector((0.0, 1.0, 0.0))
    up.normalize()
    right = up.cross(normal)
    centre = sum(points, Vector()) / len(points)
    middle = piece_centre(part)
    model.add(corners, part, [
        (middle[0] + (point - centre).dot(right) * UV_UNITS_PER_METRE,
         middle[1] + (point - centre).dot(up) * UV_UNITS_PER_METRE)
        for point in points
    ])


def add_disc(vertices, model, part, ring, out, towards):
    """Appends a fan of SIDES triangles that closes a ring around a corner on the axis."""
    vertices.append(lay(0.0, 0.0, out))
    middle = len(vertices) - 1
    for corner in range(SIDES):
        following = (corner + 1) % SIDES
        add_piece_face(vertices, model, part, [middle, ring[corner], ring[following]], towards)


def add_block(vertices, model, part, low, high):
    """Appends a box of iron (common.add_box) without its underside, which is in the tin."""
    faces = []
    first = len(vertices)
    common.add_box(vertices, faces, low, high, skip=("-z",))
    middle = (Vector(low) + Vector(high)) / 2.0
    for face in faces:
        points = [Vector(vertices[corner]) for corner in face]
        centre = sum(points, Vector()) / len(points)
        add_piece_face(vertices, model, part, list(face), centre - middle)
    assert len(vertices) == first + 8


def band_v(rows, share):
    """The v of a place in a band of the picture: share 0 is PICTURE_MARGIN rows above its
    lower edge and share 1 as many rows below its upper edge."""
    low = rows[0] + PICTURE_MARGIN
    high = rows[1] - PICTURE_MARGIN
    return (low + share * (high - low)) / textures.SIZE


def add_tin(vertices, body):
    """Appends the tin: the bands of OUTLINE and the flat butt."""
    rings = [
        (add_ring(vertices, radius, out, DENTS.get(number, ())), radius, out)
        for number, (radius, out) in enumerate(OUTLINE)
    ]

    # The way along the outline, from the edge of the lens to each ring. The picture is
    # laid along that way and not along the lamp, or the front of the rim, which faces
    # forward, would get a line of the picture and no area. v grows towards the lens, so
    # u grows to the right of whoever looks at the lamp from outside.
    way = [0.0]
    for (radius, out), (next_radius, next_out) in zip(OUTLINE, OUTLINE[1:]):
        way.append(way[-1] + math.hypot(next_radius - radius, next_out - out))

    for number in range(len(OUTLINE) - 1):
        dented = number in DENTS or number + 1 in DENTS
        add_band(vertices, body, "tin", rings[number], rings[number + 1],
                 band_v(TIN_ROWS, 1.0 - way[number] / way[-1]),
                 band_v(TIN_ROWS, 1.0 - way[number + 1] / way[-1]), split=dented)

    add_disc(vertices, body, "tin", rings[-1][0], OUTLINE[-1][1], (0.0, -1.0, 0.0))


def add_tape(vertices, body):
    """Appends the tape: the sleeve and the thicker turn at each of its ends."""
    start, end = TAPE_OUT

    def sleeve(radius, low, high):
        # From the end nearer to the lens to the other one, like the outline of the tin.
        rings = [(add_ring(vertices, radius, out), radius, out) for out in (high, low)]
        add_band(vertices, body, "tape", rings[0], rings[1],
                 band_v(TAPE_ROWS, (high - start) / (end - start)),
                 band_v(TAPE_ROWS, (low - start) / (end - start)))

    sleeve(TAPE_RADIUS, start, end)
    sleeve(TAPE_END_RADIUS, end - TAPE_END_LENGTH, end)
    sleeve(TAPE_END_RADIUS, start, start + TAPE_END_LENGTH)


def add_window(vertices, body, window):
    """Appends the frame of the window and the switch to body, and the glass to window."""
    top = BODY_RADIUS
    low = top - FRAME_SINK
    high = top + FRAME_RISE
    near, far = WINDOW_OUT
    half = WINDOW_HALF_WIDTH
    inner = half - FRAME_SIDE_BAR

    # The two bars across the lamp and the two along it.
    add_block(vertices, body, "iron", (-half, near, low), (half, near + FRAME_BAR, high))
    add_block(vertices, body, "iron", (-half, far - FRAME_BAR, low), (half, far, high))
    add_block(vertices, body, "iron", (-half, near, low), (-inner, far, high))
    add_block(vertices, body, "iron", (inner, near, low), (half, far, high))

    add_block(vertices, body, "iron",
              (-SWITCH_HALF_WIDTH, SWITCH_OUT[0], top - SWITCH_SINK),
              (SWITCH_HALF_WIDTH, SWITCH_OUT[1], top + SWITCH_RISE))

    # The glass: one face that looks up, from bar to bar.
    glass = top + GLASS_RISE
    first = len(vertices)
    vertices.extend([
        (-inner, near + FRAME_BAR, glass),
        (inner, near + FRAME_BAR, glass),
        (inner, far - FRAME_BAR, glass),
        (-inner, far - FRAME_BAR, glass),
    ])
    add_piece_face(vertices, window, "window", [first, first + 1, first + 2, first + 3],
                   (0.0, 0.0, 1.0))


def check(vertices, models):
    """Stops the script if the lamp breaks one of the rules the game relies on."""
    for corners in DENTS.values():
        for _, depth in corners:
            if not DENT_RANGE[0] <= depth <= DENT_RANGE[1]:
                raise ValueError(f"a dent of {depth} m is outside of {DENT_RANGE}")

    for model in models:
        for face, part, uvs in zip(model.faces, model.parts, model.uvs):
            points = [Vector(vertices[corner]) for corner in face]
            for point in points:
                if not -0.000001 <= point.y <= LENGTH + 0.000001:
                    raise ValueError(f"the corner {tuple(point)} is not on the {LENGTH} m of the lamp")
                if math.hypot(point.x, point.z) > HEAD_RADIUS + 0.000001:
                    raise ValueError(f"the corner {tuple(point)} is outside of the head")

            # Every face stays inside its part of the picture.
            left, bottom, right, top = PARTS[part]
            for u, v in uvs:
                if not (left <= u * textures.SIZE <= right and bottom <= v * textures.SIZE <= top):
                    raise ValueError(f"a face of the {part} leaves its part of the picture at {(u, v)}")

            # And it shows the picture as it is, not as a mirror image: the loader
            # computes the tangents from the texture coordinates, and a mirrored face
            # would turn the relief of the normal map inside out. u grows along `along`
            # and v along `up`, and the two must turn the same way as the corners do.
            edge = points[1] - points[0]
            other = points[2] - points[0]
            du1, dv1 = (uvs[1][0] - uvs[0][0], uvs[1][1] - uvs[0][1])
            du2, dv2 = (uvs[2][0] - uvs[0][0], uvs[2][1] - uvs[0][1])
            area = du1 * dv2 - du2 * dv1
            if abs(area) < 1e-12:
                raise ValueError(f"a face of the {part} has no area in the picture")
            along = (edge * dv2 - other * dv1) / area
            up = (other * du1 - edge * du2) / area
            if along.cross(up).dot(face_normal(points)) < 0.0:
                raise ValueError(f"a face of the {part} shows its picture mirrored")


def create(name, vertices, model):
    """Turns the lists of one model into a textured mesh in the scene."""
    # Only the corners this model uses, in the order they were made: the three models
    # are built from one list of corners, and a corner without a face must not be
    # exported.
    used = sorted({corner for face in model.faces for corner in face})
    place = {corner: index for index, corner in enumerate(used)}
    mesh = common.create_mesh_object(
        name,
        [vertices[corner] for corner in used],
        [tuple(place[corner] for corner in face) for face in model.faces],
    )

    # The texture coordinates are written by hand, as for the flask. A loop is one corner
    # of one face, and from_pydata keeps the order of both lists.
    uv_layer = mesh.data.uv_layers.new(name="uv")
    for polygon, face_uvs in zip(mesh.data.polygons, model.uvs):
        for loop_index, uv in zip(polygon.loop_indices, face_uvs):
            uv_layer.data[loop_index].uv = uv

    common.assign_textured_material(mesh, NAME, "lamp_held.png", "lamp_held_normal.png")


def build(shots):
    vertices = []
    body = Model()
    lens = Model()
    window = Model()

    add_tin(vertices, body)
    add_tape(vertices, body)
    add_window(vertices, body, window)
    # The lens closes the rim, with corners of its own in the places of the first ring.
    add_disc(vertices, lens, "lens", add_ring(vertices, LENS_RADIUS, LENS_OUT), LENS_OUT,
             (0.0, 1.0, 0.0))

    models = ((NAME, body), (LENS_NAME, lens), (WINDOW_NAME, window))
    check(vertices, [model for _, model in models])

    for name, model in models:
        common.reset_scene()
        create(name, vertices, model)
        common.export_obj(name)

    if shots:
        # The three models together, as the game draws them. Three views aimed at the
        # middle of the lamp: from behind and above, as the player holds it, from the
        # front, into the lens, and from the side.
        common.reset_scene()
        for name, model in models:
            create(name, vertices, model)
        common.render_review_shots(
            NAME,
            target=(0.0, 0.11, 0.0),
            camera_positions=[(-0.16, -0.22, 0.20), (0.14, 0.50, 0.10), (0.42, 0.11, 0.08)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
