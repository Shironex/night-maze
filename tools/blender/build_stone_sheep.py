# Builds the model stone_sheep: a sheep of the flock that went to stone with the hedge. A
# lumpy body of stone wool, a narrow head, two ears, four short legs and a stub of a tail.
# It is the compass of the maze: every one of them looks towards the gate, so the head end
# has to read from far away. That is why the head is a long cone that points a little down
# and the tail is short.
# Output: assets/models/stone_sheep.obj and stone_sheep.mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_stone_sheep.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "stone_sheep"

# All sizes are in metres. The geometry is written in Blender space: X is the length of
# the sheep with the head at +X, Y is its width and Z is the height. After export the head
# still looks along the game +X axis, which is east: the game turns the model about the
# vertical axis to the direction of the gate (game::sheepYawDegrees).
#
# The numbers below are the ones of the sketch, which is a quarter larger than the sheep
# of the game. build() measures the sketch and scales it so that the model is LENGTH long
# from the nose to the end of the tail, and puts the origin in the middle of that length,
# on the ground. The box of the game (STONE_SHEEP_LENGTH and STONE_SHEEP_WIDTH in
# src/game/StoneSheep.hpp) is exactly as long, and the script stops if the model is wider
# than WIDTH.
LENGTH = 0.9
WIDTH = 0.4

# The lumps of the wool come from a random number generator with this seed.
SEED = 11

# The body: a surface turned on a wheel, as (radius, place along the length) from the
# chest to the rump, with BODY_SIDES corners per ring. Every corner is moved in or out by
# the generator, between LUMP_MIN and LUMP_MIN + LUMP_RANGE of its radius. The middle line
# of the body is BODY_HEIGHT above the ground.
BODY_PROFILE = (
    (0.0, 0.43),
    (0.13, 0.38),
    (0.2, 0.26),
    (0.225, 0.08),
    (0.22, -0.1),
    (0.2, -0.26),
    (0.13, -0.37),
    (0.0, -0.41),
)
BODY_SIDES = 9
BODY_HEIGHT = 0.5
LUMP_MIN = 0.86
LUMP_RANGE = 0.26

# The head: a cone of six sides from inside the chest to the nose, which is lower.
HEAD_BASE = (0.34, 0.0, 0.6)
HEAD_TIP = (0.62, 0.0, 0.5)
HEAD_RADII = (0.1, 0.055)
HEAD_SIDES = 6
HEAD_TURN = 0.3

# The ears: two small cones that leave the head sideways. The y values are for the left
# ear and are mirrored for the right one.
EAR_BASE = (0.44, 0.05, 0.62)
EAR_TIP = (0.41, 0.15, 0.6)
EAR_RADII = (0.026, 0.008)
EAR_SIDES = 4

# The legs: four square posts under the body.
LEG_ALONG = 0.23
LEG_ACROSS = 0.1
LEG_HALF = 0.036
LEG_TOP = 0.33
# They reach this far below the ground. The game puts the origin on the ground under the
# middle of the sheep, and on a slope two of the feet are lower than that.
LEG_BOTTOM = -0.13

# The tail: a short cone that hangs down behind.
TAIL_BASE = (-0.38, 0.0, 0.55)
TAIL_TIP = (-0.48, 0.0, 0.45)
TAIL_RADII = (0.05, 0.02)
TAIL_SIDES = 5

# One repeat of the picture covers this many metres of the sheep. Its curls are then
# about 5 cm across (see build_stone_sheep_textures in make_textures.py).
METRES_PER_UV_UNIT = 1.0

MASK = 0xFFFFFFFF


def lump_generator(seed):
    """Returns a function that gives the next random number from 0 up to 1.

    It is the small generator of the sketch (mulberry32), written out here because the
    numbers of Python's own generator differ. All sums and products are cut to 32 bits.
    """
    state = seed & MASK

    def following():
        nonlocal state
        state = (state + 0x6D2B79F5) & MASK
        t = ((state ^ (state >> 15)) * (1 | state)) & MASK
        t = ((t + (((t ^ (t >> 7)) * (61 | t)) & MASK)) & MASK) ^ t
        return (t ^ (t >> 14)) / 4294967296.0

    return following


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def normalized(a):
    length = math.sqrt(dot(a, a))
    return (a[0] / length, a[1] / length, a[2] / length)


def add_face(vertices, faces, corners, outwards):
    """Appends one flat face that looks along `outwards`.

    `corners` are numbers of vertices. Two neighbours that are the same vertex count once
    (the last ring of the body is one point). A face of four corners is split into two
    triangles here: the lumps bend it, and each half then has a normal of its own.
    """
    ring = [corner for i, corner in enumerate(corners) if corner != corners[i - 1]]
    if len(ring) < 3:
        return
    normal = cross(
        sub(vertices[ring[1]], vertices[ring[0]]), sub(vertices[ring[2]], vertices[ring[0]])
    )
    if dot(normal, outwards) < 0.0:
        ring.reverse()
    if len(ring) == 4:
        faces.append((ring[0], ring[1], ring[2]))
        faces.append((ring[0], ring[2], ring[3]))
    else:
        faces.append(tuple(ring))


def add_body(vertices, faces):
    """Appends the body: the profile turned on a wheel, every corner moved by the seed."""
    following = lump_generator(SEED)
    rings = []
    for radius, along in BODY_PROFILE:
        if radius == 0.0:
            vertices.append((along, 0.0, BODY_HEIGHT))
            rings.append([len(vertices) - 1] * BODY_SIDES)
            continue
        ring = []
        for side in range(BODY_SIDES):
            angle = side / BODY_SIDES * math.tau
            lumped = radius * (LUMP_MIN + following() * LUMP_RANGE)
            # The wheel of the sketch turns about the height and is then laid down: its
            # axis becomes X, and what was its x goes down.
            vertices.append(
                (along, lumped * math.sin(angle), BODY_HEIGHT - lumped * math.cos(angle))
            )
            ring.append(len(vertices) - 1)
        rings.append(ring)

    for index in range(len(BODY_PROFILE) - 1):
        radius_step = BODY_PROFILE[index + 1][0] - BODY_PROFILE[index][0]
        along_step = BODY_PROFILE[index + 1][1] - BODY_PROFILE[index][1]
        for side in range(BODY_SIDES):
            following_side = (side + 1) % BODY_SIDES
            angle = (side + 0.5) / BODY_SIDES * math.tau
            # Away from the axis, and towards the head where the body gets thinner.
            outwards = (
                radius_step,
                -along_step * math.sin(angle),
                along_step * math.cos(angle),
            )
            add_face(
                vertices,
                faces,
                (
                    rings[index][side],
                    rings[index][following_side],
                    rings[index + 1][following_side],
                    rings[index + 1][side],
                ),
                outwards,
            )


def add_cone(vertices, faces, base, tip, radii, sides, turn=0.0):
    """Appends a cone with a flat end from `base` to `tip`, closed at both ends."""
    axis = normalized(sub(tip, base))
    # A direction across the cone: X, or Y for a cone that runs along X.
    across = (1.0, 0.0, 0.0) if abs(axis[0]) < 0.9 else (0.0, 1.0, 0.0)
    lean = dot(across, axis)
    side_way = normalized(sub(across, (axis[0] * lean, axis[1] * lean, axis[2] * lean)))
    third = cross(axis, side_way)

    rings = []
    for centre, radius in ((base, radii[0]), (tip, radii[1])):
        ring = []
        for side in range(sides):
            angle = side / sides * math.tau + turn
            vertices.append(
                tuple(
                    centre[i]
                    + third[i] * math.cos(angle) * radius
                    + side_way[i] * math.sin(angle) * radius
                    for i in range(3)
                )
            )
            ring.append(len(vertices) - 1)
        rings.append(ring)

    middle = tuple((base[i] + tip[i]) / 2.0 for i in range(3))
    for side in range(sides):
        following_side = (side + 1) % sides
        corners = (
            rings[0][side],
            rings[0][following_side],
            rings[1][following_side],
            rings[1][side],
        )
        centre = tuple(sum(vertices[corner][i] for corner in corners) / 4.0 for i in range(3))
        add_face(vertices, faces, corners, sub(centre, middle))
    # The two ends. The one at the base lies inside the body, but the renderer draws no
    # back faces, so without it one could look into the cone from behind.
    add_face(vertices, faces, tuple(rings[1]), axis)
    add_face(vertices, faces, tuple(rings[0]), (-axis[0], -axis[1], -axis[2]))


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    add_body(vertices, faces)
    add_cone(vertices, faces, HEAD_BASE, HEAD_TIP, HEAD_RADII, HEAD_SIDES, HEAD_TURN)
    for side in (-1.0, 1.0):
        add_cone(
            vertices,
            faces,
            (EAR_BASE[0], side * EAR_BASE[1], EAR_BASE[2]),
            (EAR_TIP[0], side * EAR_TIP[1], EAR_TIP[2]),
            EAR_RADII,
            EAR_SIDES,
        )
        for end in (-1.0, 1.0):
            common.add_box(
                vertices,
                faces,
                (end * LEG_ALONG - LEG_HALF, side * LEG_ACROSS - LEG_HALF, LEG_BOTTOM),
                (end * LEG_ALONG + LEG_HALF, side * LEG_ACROSS + LEG_HALF, LEG_TOP),
                skip=("-z",),
            )
    add_cone(vertices, faces, TAIL_BASE, TAIL_TIP, TAIL_RADII, TAIL_SIDES)

    # From the size of the sketch to the size of the game: LENGTH from nose to tail, the
    # origin under the middle of that length.
    lowest = min(vertex[0] for vertex in vertices)
    highest = max(vertex[0] for vertex in vertices)
    scale = LENGTH / (highest - lowest)
    middle = (lowest + highest) / 2.0
    vertices = [
        ((vertex[0] - middle) * scale, vertex[1] * scale, vertex[2] * scale)
        for vertex in vertices
    ]
    widest = max(abs(vertex[1]) for vertex in vertices)
    if widest > WIDTH / 2.0:
        raise RuntimeError(f"{NAME}: {2.0 * widest:.3f} m wide, the box is {WIDTH} m")
    print(
        f"{NAME}: scale {scale:.4f}, {2.0 * widest:.3f} m wide, "
        f"{max(vertex[2] for vertex in vertices):.3f} m high"
    )

    model = common.create_mesh_object(NAME, vertices, faces)
    common.face_project_uvs(model.data, METRES_PER_UV_UNIT)
    common.assign_textured_material(
        model, "stone_sheep", "stone_sheep.png", "stone_sheep_normal.png"
    )
    common.export_obj(NAME)

    if shots:
        # From the side with the head on the right, from the front left, and from above.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.3),
            camera_positions=[(0.2, -1.9, 0.6), (1.5, 1.2, 0.9), (-0.3, -0.5, 2.2)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
