# Builds the five models of the lever, which is a shepherd's crook on a wall: crook_post is
# the oak board with its iron straps, the pin and the ring under it, crook_handle is the
# crook that the game turns up and down, crook_rope_slack and crook_rope_taut are the rope
# from the pin through the ring into the turf, before and after the pull, and slab_ring is
# the iron ring at the foot of the wall that the lever sinks.
# Output: assets/models/crook_post, crook_handle, crook_rope_slack, crook_rope_taut and
# slab_ring, each as .obj and .mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_crook.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

# The lever is several models and not one, because the game draws the crook with a model
# matrix of its own (the board never moves, the crook turns around its pin) and swaps the
# slack rope for the taut one when the crook is down.

# All sizes are in metres. The geometry is written in Blender space: X is the width, Z is
# the height and Y is the depth. The exporter maps a Blender point (x, y, z) to the game
# point (x, z, -y), so the game +Z axis (out of the wall, towards the player) is the
# Blender -Y axis. That is why every depth below is written with a minus sign in front of
# the Blender Y coordinate.
# The origin of the post and of the ropes is the middle of the back of the board, the
# point that is fixed to the face of the wall. The origin of the crook is its pivot. The
# origin of the slab ring is the point of the wall face behind its plate.

# ---- the post ---------------------------------------------------------------------------

# The board: 0.16 m wide, 0.56 m high and 0.03 m thick.
BOARD_HALF_WIDTH = 0.08
BOARD_HALF_HEIGHT = 0.28
BOARD_THICKNESS = 0.03
# Two iron straps hold the board, this far above and below its middle. A strap is a little
# wider than the board and 4 mm thick, with a square stud near each end.
STRAP_HEIGHTS = (0.19, -0.19)
STRAP_HALF_WIDTH = 0.086
STRAP_HALF_HEIGHT = 0.018
STRAP_THICKNESS = 0.004
STUD_OFFSET = 0.055
STUD_HALF_SIZE = 0.008
STUD_THICKNESS = 0.008
# The block in the middle of the board that holds the pin: 0.064 m wide and 0.08 m high,
# and its front is 0.072 m away from the wall.
BLOCK_HALF_WIDTH = 0.032
BLOCK_HALF_HEIGHT = 0.04
BLOCK_FRONT = 0.072
# The iron pin the crook turns on. It runs through the block from side to side, a little
# wider than the block, so its two ends show.
PIN_HALF_LENGTH = 0.042
PIN_HALF_THICKNESS = 0.006
# The staple under the board and the ring that hangs from it. The rope runs through it.
STAPLE_HALF_WIDTH = 0.012
STAPLE_DEPTH = 0.014
STAPLE_HEIGHTS = (-0.285, -0.265)
RING_CENTRE = (0.0, -0.042, -0.305)
RING_RADIUS = 0.03
RING_THICKNESS = 0.006
# A ring is a tube with RING_SIDES sides bent into RING_PIECES straight pieces.
RING_PIECES = 12
RING_SIDES = 5

# ---- the crook --------------------------------------------------------------------------

# The crook is a tube with six sides, swept along a path ring by ring like the body of the
# flask: a straight shaft from just behind the pin out to SHAFT_LENGTH, then a hook that
# bends upwards and back over HOOK_DEGREES. In the model the shaft points straight out of
# the wall, along the game +Z axis.
CROOK_RADIUS = 0.011
CROOK_SIDES = 6
# Where the rings of the shaft are, measured from the pivot. The first one lies behind the
# pivot, inside the block of the post, so the end of the tube is never seen.
SHAFT_RINGS = (-0.01, 0.05, 0.1, 0.15)
SHAFT_LENGTH = 0.15
HOOK_RADIUS = 0.042
HOOK_DEGREES = 250.0
HOOK_PIECES = 14

# ---- the rope ---------------------------------------------------------------------------

# The rope is a tube with five sides. It hangs from the underside of the block, runs down
# through the ring and from there out into the turf.
ROPE_RADIUS = 0.007
ROPE_SIDES = 5
ROPE_TOP = (0.0, -0.05, -0.045)
ROPE_AT_RING = (0.0, -0.042, -0.305)
# The game hangs the lever this far above the ground (LEVER_MOUNT_HEIGHT in
# src/game/Interactables.hpp). The rope has to know it, because it ends in the ground:
# when one side changes, the other has to change too.
MOUNT_HEIGHT = 1.2
# Where the rope meets the turf: 0.26 m in front of the wall, a little to the right, and
# 4 cm under the ground of the wall. From there a tail goes straight down, so the rope
# still ends in the ground where the turf falls away from the wall.
ROPE_IN_TURF = (0.03, -0.26, -MOUNT_HEIGHT - 0.04)
ROPE_TAIL = 0.15
ROPE_PIECES_TO_RING = 6
ROPE_PIECES_TO_TURF = 10
# The slack rope sags: this far out of the wall between the block and the ring, and this
# far out and down between the ring and the turf. The taut rope is two straight lines.
SLACK_ABOVE_RING = 0.025
SLACK_OUT = 0.07
SLACK_DOWN = 0.06

# ---- the slab ring ----------------------------------------------------------------------

# A plate on the wall, a lug on the plate and a ring through the lug that hangs flat
# against the wall. It is two and a half times the ring of the post, so it can be read
# from a few steps away.
SLAB_PLATE_HALF_WIDTH = 0.035
SLAB_PLATE_HALF_HEIGHT = 0.03
SLAB_PLATE_THICKNESS = 0.012
SLAB_LUG_HALF_SIZE = 0.013
SLAB_LUG_FRONT = 0.034
SLAB_RING_RADIUS = 0.075
SLAB_RING_THICKNESS = 0.011
# The ring hangs from the lug: its highest point is the middle of the lug.
SLAB_RING_CENTRE = (0.0, -0.023, -SLAB_RING_RADIUS)

# ---- the pictures -----------------------------------------------------------------------

# No model here has a picture of its own. Each wears a part of a picture another model
# already has, and a model has one material, so the oak and the iron of the post come out
# of the same picture: gate_wood.png, which holds planks and iron bands. The numbers below
# are pixels of the 512 px pictures (make_textures.py), row 0 at the bottom.
#   "centre":    the middle of the part of the picture the faces are put on
#   "half":      how far that part reaches from its middle, across and up
#   "metres":    how many metres one whole picture covers, across and up
#   "along_u":   True when a tube runs along the picture from left to right
PICTURE_SIZE = 512.0
UV_RULES = {
    # One plank of the gate, between its two iron bands (GATE_BAND_CENTRES): the grain
    # runs upwards, and the dark gaps beside the plank stay outside.
    "oak": {"centre": (224, 224), "half": (28, 104), "metres": (1.5, 1.5), "along_u": False},
    # A piece of the lower iron band, between two of its rivets.
    "iron": {"centre": (64, 96), "half": (24, 16), "metres": (2.2, 2.2), "along_u": True},
    # The cord wound around the neck of the flask (FLASK_CORD_ROWS): once along the
    # picture, with the turns of the cord around the rope.
    "rope": {"centre": (256, 363.5), "half": (254, 23.5), "metres": (1.45, 0.47), "along_u": True},
    # The whole iron picture of the slab ring, one repeat per 0.6 m.
    "slab": {"centre": (256, 256), "half": (256, 256), "metres": (0.6, 0.6), "along_u": True},
}

# ---- what the game does with the models -------------------------------------------------

# These numbers are not used for the geometry, only for the check in check_pick_box below.
# They must stay in step with the constants of the game.
# The game places the pivot this far in front of the wall face, in the middle of the board
# (LEVER_PIVOT_DEPTH in src/game/Interaction.hpp): on the pin, inside the block.
PIVOT_DEPTH = 0.05
# The game turns the crook by this angle up or down around the game X axis
# (LEVER_HANDLE_UP_DEGREES and LEVER_HANDLE_DOWN_DEGREES in src/game/Interaction.hpp).
HANDLE_SWING_DEGREES = 55.0
# The box the game tests the mouse ray against when the player clicks on the lever: its
# width, its height (both centred on the origin of the post) and how far it reaches out
# from the wall (LEVER_BOX_WIDTH, LEVER_BOX_HEIGHT and LEVER_BOX_DEPTH in
# src/game/Interactables.hpp).
PICK_BOX_WIDTH = 0.3
PICK_BOX_HEIGHT = 0.64
PICK_BOX_DEPTH = 0.26
PICK_BOX_TOLERANCE = 0.000001


# ---- small vector helpers: a point is a tuple (x, y, z) ---------------------------------


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, factor):
    return (a[0] * factor, a[1] * factor, a[2] * factor)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def unit(a):
    return mul(a, 1.0 / math.sqrt(dot(a, a)))


def mix(a, b, t):
    return add(a, mul(sub(b, a), t))


def centroid(points):
    return mul((sum(p[0] for p in points), sum(p[1] for p in points), sum(p[2] for p in points)),
               1.0 / len(points))


def newell(points):
    """The normal of a polygon that need not be flat (not of length 1)."""
    normal = (0.0, 0.0, 0.0)
    for here, there in zip(points, points[1:] + points[:1]):
        normal = add(normal, ((here[1] - there[1]) * (here[2] + there[2]),
                              (here[2] - there[2]) * (here[0] + there[0]),
                              (here[0] - there[0]) * (here[1] + there[1])))
    return normal


class Mesh:
    """The lists of one model: its corners, its faces and where each face lies on its picture.

    Every face remembers the name of its rule in UV_RULES and, for each corner, two lengths
    in metres measured on the face itself, from the middle of its part: `across` and `up`
    for a flat face, `around` and `along` for a face of a tube. finish() turns them into
    texture coordinates.
    """

    def __init__(self):
        self.vertices = []
        self.faces = []
        self.flat = []

    def add_face(self, indices, rule, lengths, tube):
        self.faces.append(tuple(indices))
        self.flat.append((rule, list(lengths), tube))

    def add_flat_face(self, indices, rule):
        """A flat face, laid on its picture like common.face_project_uvs does it."""
        points = [self.vertices[index] for index in indices]
        normal = unit(newell(points))
        # The height direction with its part along the normal removed lies in the face.
        up = sub((0.0, 0.0, 1.0), mul(normal, normal[2]))
        if dot(up, up) < 0.000000000001:
            # A level face has no height direction. Blender Y is used instead.
            up = (0.0, 1.0, 0.0)
        up = unit(up)
        right = cross(up, normal)
        middle = centroid(points)
        lengths = [(dot(sub(p, middle), right), dot(sub(p, middle), up)) for p in points]
        self.add_face(indices, rule, lengths, tube=False)

    def add_box(self, low, high, rule, skip=()):
        first_face = len(self.faces)
        faces = []
        common.add_box(self.vertices, faces, low, high, skip)
        for face in faces:
            self.add_flat_face(face, rule)
        return first_face

    def add_tube(self, path, radius, sides, rule, side, closed=False, caps=False):
        """Appends a tube along `path`, ring by ring.

        `side` is a direction that never points along the path: the first corner of every
        ring lies across it, so the rings do not twist against each other. A closed tube
        is a ring: the last point of the path is its first point again. `caps` closes the
        two ends of an open tube.
        """
        last = len(path) - 1
        rings = []
        for index, point in enumerate(path):
            # The direction of the path here: from the point before to the point after.
            before = path[index - 1 if index > 0 else (last - 1 if closed else 0)]
            after = path[index + 1 if index < last else (1 if closed else last)]
            tangent = unit(sub(after, before))
            across = unit(sub(side, mul(tangent, dot(side, tangent))))
            normal = cross(tangent, across)
            if closed and index == last:
                rings.append(rings[0])
                continue
            ring = []
            for corner in range(sides):
                angle = math.tau * corner / sides
                ring.append(len(self.vertices))
                self.vertices.append(add(point, add(mul(normal, math.cos(angle) * radius),
                                                    mul(across, math.sin(angle) * radius))))
            rings.append(ring)

        # The lengths on the picture: how far along the path, and how far around the tube
        # (the width of one side is a chord of the circle), both from the middle.
        along = [0.0]
        for here, there in zip(path, path[1:]):
            along.append(along[-1] + math.sqrt(dot(sub(there, here), sub(there, here))))
        side_width = 2.0 * radius * math.sin(math.pi / sides)
        for index in range(last):
            middle = mix(path[index], path[index + 1], 0.5)
            for corner in range(sides):
                following = (corner + 1) % sides
                indices = [rings[index][corner], rings[index][following],
                           rings[index + 1][following], rings[index + 1][corner]]
                low = (corner - sides / 2.0) * side_width
                lengths = [(low, along[index] - along[-1] / 2.0),
                           (low + side_width, along[index] - along[-1] / 2.0),
                           (low + side_width, along[index + 1] - along[-1] / 2.0),
                           (low, along[index + 1] - along[-1] / 2.0)]
                # The face has to look away from the middle of the tube. The renderer
                # draws no back faces, so a face the wrong way round would be a hole.
                points = [self.vertices[i] for i in indices]
                if dot(newell(points), sub(centroid(points), middle)) < 0.0:
                    indices.reverse()
                    lengths.reverse()
                self.add_face(indices, rule, lengths, tube=True)
        if caps:
            for ring, inside in ((rings[0], path[1]), (rings[-1], path[-2])):
                points = [self.vertices[i] for i in ring]
                looks_inside = dot(newell(points), sub(centroid(points), inside)) < 0.0
                self.add_flat_face(list(reversed(ring)) if looks_inside else ring, rule)


def texture_coordinates(rule_name, lengths, tube, mirror):
    rule = UV_RULES[rule_name]
    coordinates = []
    for first, second in lengths:
        first = -first if mirror else first
        # A tube that runs along the picture: `along` is u and `around` is v.
        across, up = (second, first) if tube and rule["along_u"] else (first, second)
        coordinates.append(((rule["centre"][0] / PICTURE_SIZE) + across / rule["metres"][0],
                            (rule["centre"][1] / PICTURE_SIZE) + up / rule["metres"][1]))
    return coordinates


def turns_left(coordinates):
    """True when the corners go counter clockwise on the picture, like they do on the face."""
    area = 0.0
    for here, there in zip(coordinates, coordinates[1:] + coordinates[:1]):
        area += here[0] * there[1] - there[0] * here[1]
    return area > 0.0


def finish(name, mesh, material_name, target, cameras, shots):
    """Turns the lists into a textured model, exports it and renders the review shots.

    `material_name` is also the name of the two pictures: <material_name>.png and
    <material_name>_normal.png in assets/textures.
    """
    uvs = []
    for rule_name, lengths, tube in mesh.flat:
        coordinates = texture_coordinates(rule_name, lengths, tube, mirror=False)
        if not turns_left(coordinates):
            # The picture would be a mirror image on this face, and the loader computes
            # the tangents of the normal map from the texture coordinates.
            coordinates = texture_coordinates(rule_name, lengths, tube, mirror=True)
        rule = UV_RULES[rule_name]
        for u, v in coordinates:
            if (abs(u * PICTURE_SIZE - rule["centre"][0]) > rule["half"][0] + 0.001
                    or abs(v * PICTURE_SIZE - rule["centre"][1]) > rule["half"][1] + 0.001):
                raise ValueError(f"A face of {name} leaves the '{rule_name}' part of its picture")
        uvs.append(coordinates)

    model = common.create_mesh_object(name, mesh.vertices, mesh.faces)
    # The texture coordinates are written by hand, like for the flask: a loop is one
    # corner of one face, and from_pydata keeps the order of both lists.
    uv_layer = model.data.uv_layers.new(name="uv")
    for polygon, face_uvs in zip(model.data.polygons, uvs):
        for loop_index, uv in zip(polygon.loop_indices, face_uvs):
            uv_layer.data[loop_index].uv = uv

    common.assign_textured_material(
        model, material_name, material_name + ".png", material_name + "_normal.png"
    )
    common.export_obj(name)

    if shots:
        common.render_review_shots(name, target=target, camera_positions=cameras)


def circle_path(centre, radius, pieces):
    """A closed circle that stands flat against the wall (in the X-Z plane)."""
    return [(centre[0] + radius * math.cos(math.tau * piece / pieces), centre[1],
             centre[2] + radius * math.sin(math.tau * piece / pieces))
            for piece in range(pieces + 1)]


def post_mesh():
    """Returns the mesh of the post and the number of its first face that the pick box of
    the lever does not have to hold: the ring under the board."""
    mesh = Mesh()
    front = -BOARD_THICKNESS

    # The board: from the wall (Blender y = 0) out. Its back lies on the wall.
    mesh.add_box((-BOARD_HALF_WIDTH, front, -BOARD_HALF_HEIGHT),
                 (BOARD_HALF_WIDTH, 0.0, BOARD_HALF_HEIGHT), "oak", skip=("+y",))
    # The straps lie on the front of the board and the studs on the straps, so each has
    # its own back hidden.
    for height in STRAP_HEIGHTS:
        mesh.add_box((-STRAP_HALF_WIDTH, front - STRAP_THICKNESS, height - STRAP_HALF_HEIGHT),
                     (STRAP_HALF_WIDTH, front, height + STRAP_HALF_HEIGHT), "iron", skip=("+y",))
        for x in (-STUD_OFFSET, STUD_OFFSET):
            mesh.add_box(
                (x - STUD_HALF_SIZE, front - STRAP_THICKNESS - STUD_THICKNESS, height - STUD_HALF_SIZE),
                (x + STUD_HALF_SIZE, front - STRAP_THICKNESS, height + STUD_HALF_SIZE),
                "iron", skip=("+y",))
    # The block that holds the pin, and the pin through it at the depth of the pivot.
    mesh.add_box((-BLOCK_HALF_WIDTH, -BLOCK_FRONT, -BLOCK_HALF_HEIGHT),
                 (BLOCK_HALF_WIDTH, front, BLOCK_HALF_HEIGHT), "oak", skip=("+y",))
    mesh.add_box((-PIN_HALF_LENGTH, -PIVOT_DEPTH - PIN_HALF_THICKNESS, -PIN_HALF_THICKNESS),
                 (PIN_HALF_LENGTH, -PIVOT_DEPTH + PIN_HALF_THICKNESS, PIN_HALF_THICKNESS), "iron")
    # The staple under the board, and the ring in it.
    mesh.add_box((-STAPLE_HALF_WIDTH, front - STAPLE_DEPTH, STAPLE_HEIGHTS[0]),
                 (STAPLE_HALF_WIDTH, front, STAPLE_HEIGHTS[1]), "iron", skip=("+y",))
    ring_first_vertex = len(mesh.vertices)
    mesh.add_tube(circle_path(RING_CENTRE, RING_RADIUS, RING_PIECES), RING_THICKNESS,
                  RING_SIDES, "iron", (0.0, 1.0, 0.0), closed=True)
    return mesh, ring_first_vertex


def handle_mesh():
    mesh = Mesh()
    path = [(0.0, -out, 0.0) for out in SHAFT_RINGS]
    # The hook: its centre lies HOOK_RADIUS above the end of the shaft, and it starts at
    # the end of the shaft, which is the lowest point of the circle.
    for piece in range(1, HOOK_PIECES + 1):
        angle = math.radians(HOOK_DEGREES * piece / HOOK_PIECES)
        path.append((0.0, -SHAFT_LENGTH - HOOK_RADIUS * math.sin(angle),
                     HOOK_RADIUS - HOOK_RADIUS * math.cos(angle)))
    mesh.add_tube(path, CROOK_RADIUS, CROOK_SIDES, "oak", (1.0, 0.0, 0.0), caps=True)
    return mesh


def rope_mesh(taut):
    mesh = Mesh()
    path = []
    for piece in range(ROPE_PIECES_TO_RING + 1):
        t = piece / ROPE_PIECES_TO_RING
        sag = 0.0 if taut else math.sin(math.pi * t)
        path.append(add(mix(ROPE_TOP, ROPE_AT_RING, t), (0.0, -SLACK_ABOVE_RING * sag, 0.0)))
    for piece in range(1, ROPE_PIECES_TO_TURF + 1):
        t = piece / ROPE_PIECES_TO_TURF
        sag = 0.0 if taut else math.sin(math.pi * t)
        path.append(add(mix(ROPE_AT_RING, ROPE_IN_TURF, t),
                        (0.0, -SLACK_OUT * sag, -SLACK_DOWN * sag)))
    path.append(add(ROPE_IN_TURF, (0.0, 0.0, -ROPE_TAIL)))
    mesh.add_tube(path, ROPE_RADIUS, ROPE_SIDES, "rope", (1.0, 0.0, 0.0))
    return mesh


def slab_ring_mesh():
    mesh = Mesh()
    mesh.add_box((-SLAB_PLATE_HALF_WIDTH, -SLAB_PLATE_THICKNESS, -SLAB_PLATE_HALF_HEIGHT),
                 (SLAB_PLATE_HALF_WIDTH, 0.0, SLAB_PLATE_HALF_HEIGHT), "slab", skip=("+y",))
    mesh.add_box((-SLAB_LUG_HALF_SIZE, -SLAB_LUG_FRONT, -SLAB_LUG_HALF_SIZE),
                 (SLAB_LUG_HALF_SIZE, -SLAB_PLATE_THICKNESS, SLAB_LUG_HALF_SIZE), "slab",
                 skip=("+y",))
    mesh.add_tube(circle_path(SLAB_RING_CENTRE, SLAB_RING_RADIUS, RING_PIECES),
                  SLAB_RING_THICKNESS, RING_SIDES, "slab", (0.0, 1.0, 0.0), closed=True)
    return mesh


def check_pick_box(post, ring_first_vertex, handle):
    """Stops the script if the lever would stick out of its pick box.

    A part outside of the pick box could be seen but not clicked. What the box holds: the
    board with its straps, studs, block, pin and staple, as they are, and every corner of
    the crook turned up by the full swing, straight out, and turned down by the full
    swing. What it does not hold, on purpose: the ring under the board, whose lower edge
    hangs about 2 cm below the box, and the rope, which runs down to the ground.
    """
    def fits(across, out, up):
        return (abs(across) <= PICK_BOX_WIDTH / 2 + PICK_BOX_TOLERANCE
                and abs(up) <= PICK_BOX_HEIGHT / 2 + PICK_BOX_TOLERANCE
                and out <= PICK_BOX_DEPTH + PICK_BOX_TOLERANCE)

    reach_out = 0.0
    reach_up = 0.0
    ok = all(fits(x, -y, z) for x, y, z in post.vertices[:ring_first_vertex])
    for degrees in (-HANDLE_SWING_DEGREES, 0.0, HANDLE_SWING_DEGREES):
        turn = math.radians(degrees)
        for x, y, z in handle.vertices:
            # Turned around the X axis in the pivot, then moved out to the pivot.
            out = PIVOT_DEPTH - (y * math.cos(turn) - z * math.sin(turn))
            up = y * math.sin(turn) + z * math.cos(turn)
            reach_out = max(reach_out, out)
            reach_up = max(reach_up, abs(up))
            ok = ok and fits(x, out, up)
    print(f"The crook reaches {reach_out:.4f} m out of the wall and {reach_up:.4f} m up or down")
    if not ok:
        raise ValueError("The lever does not fit into its pick box (src/game/Interactables.hpp)")


def build(shots):
    post, ring_first_vertex = post_mesh()
    handle = handle_mesh()
    check_pick_box(post, ring_first_vertex, handle)

    # The views are from in front of the wall (the front is Blender -Y).
    common.reset_scene()
    finish("crook_post", post, "gate_wood", (0.0, -0.04, -0.03),
           [(0.55, -0.95, 0.3), (-0.6, -0.6, -0.3)], shots)
    common.reset_scene()
    finish("crook_handle", handle, "gate_wood", (0.0, -0.1, 0.03),
           [(0.4, -0.35, 0.2), (-0.35, -0.5, -0.15)], shots)
    for name, taut in (("crook_rope_slack", False), ("crook_rope_taut", True)):
        common.reset_scene()
        finish(name, rope_mesh(taut), "flask", (0.0, -0.15, -0.7),
               [(1.6, -1.3, -0.4), (0.3, -0.9, -0.2)], shots)
    common.reset_scene()
    finish("slab_ring", slab_ring_mesh(), "lever_iron", (0.0, -0.02, -0.07),
           [(0.25, -0.45, 0.1), (-0.4, -0.2, -0.1)], shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
