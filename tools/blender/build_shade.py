# Builds the two models of the shade: the tall hooded figure that follows the player through
# the maze whenever the flashlight is not on it. shade is the cloth: a cloak from the ground
# to the shoulders whose hem is torn into tongues, two sleeves that hang to the knees with
# nothing in them, and a pointed hood. shade_hollow is what the cloth is wrapped around:
# an oval where a face would be, and the two openings of the cuffs. The game draws those
# faces as a piece of the night sky and not as cloth.
# Output: assets/models/shade.obj, shade.mtl, shade_hollow.obj and shade_hollow.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_shade.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

from mathutils import Vector

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "shade"
HOLLOW_NAME = "shade_hollow"

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

# The torn hem. Every corner of the two lowest rings is lifted by its number of this list,
# in metres, and pulled in by a quarter of that. Where the number is near 0 the cloth
# still goes into the ground: a tongue. Between two tongues the hem hangs up to 12 cm
# over the ground, and the dark under the cloak shows.
HEM_LIFT = (0.0, 0.14, 0.04, 0.2, 0.02, 0.12, 0.22, 0.05, 0.17, 0.0, 0.1, 0.19, 0.03, 0.15, 0.01,
            0.09)
HEM_LIFT_RINGS = 2
HEM_PULL_IN = 0.25

# Under a lifted hem the inside of the cloak is in view, and the game draws no back of
# a face. So the bands between the lowest rings are built twice: once looking out and
# once looking in (add_inward_band). This many bands, counted from the ground.
INWARD_BANDS = 2

# The front of the hood. The corners of the hood rings that look forwards (within
# FACE_HALF_ANGLE of the front) do not lie on the round outline: they are pulled back onto
# the straight line between the two corners beside them, FACE_RIM_ANGLE from the front. So
# the hood is cut off flat where a face would be, under its brow and over its cowl. The
# rings between the two heights have it.
FACE_LOW = 1.66
FACE_HIGH = 1.95
FACE_HALF_ANGLE = 50.0
FACE_RIM_ANGLE = 67.5

# The night in the hood: an oval on that flat front, this far in front of the cloth (like
# a chalk mark on a wall), with its middle at this height and with this many corners. It
# is the size of a face, and its edge is the only border the night has: no cloth frames
# it. The middle lies at the height of a ring, where the flat front has a ridge, and no
# triangle crosses that height, so every triangle lies flat on the cloth behind it. It
# belongs to the model shade_hollow.
NIGHT_HEIGHT = 1.81
NIGHT_HALF_WIDTH = 0.08
NIGHT_HALF_HEIGHT = 0.1
NIGHT_SIDES = 16
NIGHT_GAP = 0.004

# The front of the figure, as an angle around Z: -Y.
FRONT_ANGLE = 270.0

# A sleeve: a tube of this many sides from the shoulder past the elbow to the knee, as
# three points for the right arm (the left one is its mirror image). It gets wider towards
# the cuff, from the first radius to the second. The rings of the tube are turned by
# SLEEVE_TURN (radians), so no edge of it points straight at the player.
SLEEVE_SIDES = 6
SLEEVE_PATH = ((0.31, -0.02, 1.44), (0.38, -0.05, 1.09), (0.40, -0.08, 0.74))
SLEEVE_RADIUS_TOP = 0.07
SLEEVE_RADIUS_GROWTH = 0.045
SLEEVE_TURN = 0.3

# The cuff holds no hand, only the night: a flat face of this radius closes the sleeve
# this far above its lower end. It belongs to the model shade_hollow.
CUFF_RADIUS = 0.1
CUFF_INSET = 0.03

# The picture shade.png goes once around the figure, with the height on its v axis. The
# faces of the body are given their place in it corner by corner (add_band). The faces of
# the sleeves and of the cuffs are projected onto their own plane instead, with this many
# units of u and v per metre: the density the body has along its height.
UV_UNITS_PER_METRE = 1.0 / (HEIGHT + HEM_DEPTH)
# Where the picture is dark: the middle of the front of the hood (make_textures.py,
# SHADE_FACE_ROWS and SHADE_FACE_COLUMN). The cuffs take their piece from there.
DARK_UV = (0.75, 0.86)


def add_ring(vertices, number):
    """Appends the SIDES corners of ring `number` of the outline.

    Returns the index of its first corner. The corners go counter clockwise as seen from
    above.
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

        # The flat front of the hood: as far forward as the corners at its rim.
        depth = reach * narrow * math.sin(angle)
        from_front = abs((degrees - FRONT_ANGLE + 180.0) % 360.0 - 180.0)
        if FACE_LOW <= height <= FACE_HIGH and from_front <= FACE_HALF_ANGLE:
            depth = front_depth(number) - forward

        # The torn hem: after the folds, which are measured at the height of the outline.
        lifted = height
        if number < HEM_LIFT_RINGS:
            lifted += HEM_LIFT[corner]
            reach *= 1.0 - HEM_LIFT[corner] * HEM_PULL_IN
            depth *= 1.0 - HEM_LIFT[corner] * HEM_PULL_IN

        vertices.append((reach * math.cos(angle), forward + depth, lifted))
    return first


def front_depth(number):
    """The y of the flat front of the hood at ring `number`: that of its rim corners."""
    _, radius, narrow, forward = RINGS[number]
    return forward - radius * narrow * math.cos(math.radians(FACE_RIM_ANGLE))


def picture_height(height):
    """The v of a height on the figure: 0 at the hem, 1 at the tip of the hood."""
    return (height + HEM_DEPTH) / (HEIGHT + HEM_DEPTH)


def add_band(cloth, rings, number):
    """Appends the SIDES faces between ring `number` and the ring above it.

    cloth is a pair of lists (faces, uvs). The picture goes once around the figure: u is
    the number of the corner divided by SIDES, and the last face ends at u = 1 and not at
    u = 0. v is the height of the outline, also where the hem is lifted: the picture is
    stretched there, not cut.
    """
    faces, uvs = cloth
    lower = rings[number]
    upper = rings[number + 1]
    low = picture_height(RINGS[number][0])
    high = picture_height(RINGS[number + 1][0])
    for corner in range(SIDES):
        following = (corner + 1) % SIDES
        left = corner / SIDES
        right = (corner + 1) / SIDES
        # Counter clockwise as seen from outside, so the normal points outwards.
        faces.append((lower + corner, lower + following, upper + following, upper + corner))
        uvs.append(((left, low), (right, low), (right, high), (left, high)))


def add_inward_band(cloth, rings, number):
    """Appends the faces of add_band once more, seen from inside the cloak.

    rings are corners of their own in the same places: Blender keeps only one of two
    faces that share all their corners. The corners of a face go the other way round, and
    u still grows to the right of whoever looks at the face, so the picture is not
    a mirror image for the normal map.
    """
    faces, uvs = cloth
    lower = rings[number]
    upper = rings[number + 1]
    low = picture_height(RINGS[number][0])
    high = picture_height(RINGS[number + 1][0])
    for corner in range(SIDES):
        following = (corner + 1) % SIDES
        left = corner / SIDES
        right = (corner + 1) / SIDES
        faces.append((lower + following, lower + corner, upper + corner, upper + following))
        uvs.append(((left, low), (right, low), (right, high), (left, high)))


def add_night(vertices, hollow):
    """Appends the oval of night on the flat front of the hood to hollow.

    A fan of NIGHT_SIDES triangles around its middle, looking forwards.
    """
    faces, uvs = hollow
    # The flat front, from its lower ring to its upper one: the height and the y of each.
    front = [(RINGS[number][0], front_depth(number)) for number in range(len(RINGS))
             if FACE_LOW <= RINGS[number][0] <= FACE_HIGH]

    def depth_at(height):
        # The y of the cloth at a height: a straight line from ring to ring.
        for (low, low_depth), (high, high_depth) in zip(front, front[1:]):
            if low <= height <= high:
                share = (height - low) / (high - low)
                return low_depth + share * (high_depth - low_depth)
        raise ValueError(f"the night in the hood leaves the flat front at {height} m")

    def add_corner(x, height):
        vertices.append((x, depth_at(height) - NIGHT_GAP, height))
        # Its place in the picture: around the middle of the dark, to the right and up as
        # someone sees it who looks at the face.
        uv = (DARK_UV[0] + x * UV_UNITS_PER_METRE,
              DARK_UV[1] + (height - NIGHT_HEIGHT) * UV_UNITS_PER_METRE)
        return len(vertices) - 1, uv

    middle = add_corner(0.0, NIGHT_HEIGHT)
    rim = []
    for corner in range(NIGHT_SIDES):
        angle = corner / NIGHT_SIDES * 2.0 * math.pi
        # Rounded, so the two corners at the height of the middle lie exactly on it.
        rim.append(add_corner(NIGHT_HALF_WIDTH * math.cos(angle),
                              round(NIGHT_HEIGHT + NIGHT_HALF_HEIGHT * math.sin(angle), 6)))
    for corner in range(NIGHT_SIDES):
        # Counter clockwise as seen from the front, where +X is to the right.
        triangle = (middle, rim[corner], rim[(corner + 1) % NIGHT_SIDES])
        faces.append(tuple(index for index, _ in triangle))
        uvs.append(tuple(uv for _, uv in triangle))


def add_projected_face(vertices, model, corners, inside, uv_centre):
    """Appends one flat face that looks away from the point `inside`.

    model is a pair of lists (faces, uvs). The piece of the picture lies around
    `uv_centre`: the face is projected onto its own plane, as common.face_project_uvs
    does, so the picture is never a mirror image.
    """
    faces, uvs = model
    points = [Vector(vertices[corner]) for corner in corners]
    centre = sum(points, Vector()) / len(points)

    # The direction the face looks in, as the average over its edges (Newell's method):
    # the four corners of a side of a bent tube do not lie in one plane.
    normal = Vector()
    for place, point in enumerate(points):
        after = points[(place + 1) % len(points)]
        normal += Vector(((point.y - after.y) * (point.z + after.z),
                          (point.z - after.z) * (point.x + after.x),
                          (point.x - after.x) * (point.y + after.y)))
    normal.normalize()
    if normal.dot(centre - Vector(inside)) < 0.0:
        # The corners went clockwise as seen from outside. Turn them around.
        corners = corners[::-1]
        points.reverse()
        normal = -normal

    # `up` is the height as far as the face allows, `right` points to the right for
    # someone who looks at the face from outside.
    up = Vector((0.0, 0.0, 1.0)) - normal * normal.z
    if up.length < 0.000001:
        up = Vector((0.0, 1.0, 0.0))
    up.normalize()
    right = up.cross(normal)
    faces.append(tuple(corners))
    uvs.append(tuple(
        (uv_centre[0] + (point - centre).dot(right) * UV_UNITS_PER_METRE,
         uv_centre[1] + (point - centre).dot(up) * UV_UNITS_PER_METRE)
        for point in points
    ))


def ring_around(vertices, centre, along, radius, sides, turn):
    """Appends a ring of `sides` corners around `centre`, across the direction `along`.

    Returns the indices of the corners.
    """
    along = along.normalized()
    # Two directions across `along`. The first is Blender Y as far as `along` allows.
    side = Vector((0.0, 1.0, 0.0))
    side = (side - along * side.dot(along)).normalized()
    other = along.cross(side)
    ring = []
    for corner in range(sides):
        angle = corner / sides * 2.0 * math.pi + turn
        point = centre + other * (math.cos(angle) * radius) + side * (math.sin(angle) * radius)
        vertices.append(tuple(point))
        ring.append(len(vertices) - 1)
    return ring


def add_sleeve(vertices, cloth, hollow, mirror):
    """Appends one sleeve to cloth and the opening of its cuff to hollow.

    mirror is 1 for the right arm (Blender +X) and -1 for the left one.
    """
    path = [Vector((mirror * x, y, z)) for x, y, z in SLEEVE_PATH]
    last = len(path) - 1

    # One ring per point of the path, across the direction the path has there.
    rings = []
    for number, point in enumerate(path):
        along = path[min(last, number + 1)] - path[max(0, number - 1)]
        radius = SLEEVE_RADIUS_TOP + SLEEVE_RADIUS_GROWTH * number / last
        rings.append(ring_around(vertices, point, along, radius, SLEEVE_SIDES, SLEEVE_TURN))

    for number in range(last):
        inside = (path[number] + path[number + 1]) / 2.0
        for side in range(SLEEVE_SIDES):
            following = (side + 1) % SLEEVE_SIDES
            corners = [rings[number][side], rings[number][following],
                       rings[number + 1][following], rings[number + 1][side]]
            # Every side shows another strip of the folds, at the height it hangs at.
            uv_centre = ((side + 0.5) / SLEEVE_SIDES, picture_height(inside.z))
            add_projected_face(vertices, cloth, corners, inside, uv_centre)

    # The upper end sticks a few centimetres out of the shoulder, so it gets a lid. The
    # player looks down on it, and without the lid would look into the tube.
    add_projected_face(vertices, cloth, list(rings[0]), path[1],
                       (0.5 / SLEEVE_SIDES, picture_height(path[0].z)))

    # The opening of the cuff: a flat face a little way up the sleeve, looking out of it.
    along = (path[last] - path[last - 1]).normalized()
    centre = path[last] - along * CUFF_INSET
    cuff = ring_around(vertices, centre, along, CUFF_RADIUS, SLEEVE_SIDES, 0.0)
    add_projected_face(vertices, hollow, cuff, centre - along, DARK_UV)


def finish(name, vertices, model, shots):
    """Turns the lists of one model into a textured mesh and exports it."""
    common.reset_scene()
    faces, uvs = model

    # Only the corners this model uses, in the order they were made: the two models are
    # built from one list of corners, and a corner without a face must not be exported.
    used = sorted({corner for face in faces for corner in face})
    place = {corner: index for index, corner in enumerate(used)}
    mesh_vertices = [vertices[corner] for corner in used]
    mesh_faces = [tuple(place[corner] for corner in face) for face in faces]

    mesh = common.create_mesh_object(name, mesh_vertices, mesh_faces)

    # The texture coordinates are written by hand, as for the flask: the picture is used
    # once and not repeated every few metres.
    uv_layer = mesh.data.uv_layers.new(name="uv")
    for polygon, face_uvs in zip(mesh.data.polygons, uvs):
        for loop_index, uv in zip(polygon.loop_indices, face_uvs):
            uv_layer.data[loop_index].uv = uv

    # Both models wear the picture of the cloth. The game shows the sky on the hollow and
    # falls back to this picture, which is dark there, when it draws no sky.
    common.assign_textured_material(mesh, "shade", "shade.png", "shade_normal.png")
    common.export_obj(name)

    if shots:
        # Three views aimed at the chest: from the front, from the side and from where
        # a player stands who looks up at it.
        common.render_review_shots(
            name,
            target=(0.0, 0.0, 1.05),
            camera_positions=[(0.0, -5.6, 1.4), (4.6, -3.0, 1.7), (1.2, -3.4, 1.7)],
        )


def build(shots):
    vertices = []
    # The two models: the faces of each and, in the same order, their texture coordinates.
    cloth = ([], [])
    hollow = ([], [])

    rings = [add_ring(vertices, number) for number in range(len(RINGS))]
    for number in range(len(RINGS) - 1):
        add_band(cloth, rings, number)

    # The tip of the hood is closed by one small face. Its piece of the picture is a
    # point at the top edge: the face is two centimetres wide.
    top = tuple(rings[-1] + corner for corner in range(SIDES))
    cloth[0].append(top)
    cloth[1].append(tuple((corner / SIDES, 1.0) for corner in range(SIDES)))
    # The hem has no face under it: it is in the ground.

    # The inside of the cloak above the torn hem.
    inner_rings = [add_ring(vertices, number) for number in range(INWARD_BANDS + 1)]
    for number in range(INWARD_BANDS):
        add_inward_band(cloth, inner_rings, number)

    add_night(vertices, hollow)

    add_sleeve(vertices, cloth, hollow, 1.0)
    add_sleeve(vertices, cloth, hollow, -1.0)

    finish(NAME, vertices, cloth, shots)
    finish(HOLLOW_NAME, vertices, hollow, shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
